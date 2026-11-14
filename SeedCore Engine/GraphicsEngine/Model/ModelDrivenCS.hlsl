#include <GraphicsEngine/Model/Model.hlsli>
#include <GraphicsEngine/Raytracing/BottomLevel.hlsli>
#include <GraphicsEngine/Raytracing/TopLevel.hlsli>
#include <GraphicsEngine/Shader/RootSignature.hlsli>

struct ModelUpdateBindings
{
    uint model_unit_updates_;
    uint model_mesh_updates_;
    uint model_cluster_updates_;
    uint model_placement_updates_;
    uint model_material_updates_;
    uint model_trace_updates_;
    uint model_units_;
    uint model_meshes_;
    uint model_clusters_;
    uint model_placements_;
    uint model_materials_;
    uint model_traces_;
};
ConstantBuffer<ModelUpdateBindings> update_bindings : register(b0, space1);

struct ModelExpandBindings
{
    uint model_units_;
    uint model_meshes_;
    uint model_clusters_;
    uint model_placements_;
    uint model_materials_;
    uint model_softbodies_;
    uint model_constants_;
    uint geometry_instances_;
    uint geometry_counts_;
    uint model_streaming_;
};
ConstantBuffer<ModelExpandBindings> expand_bindings : register(b0, space1);

struct ModelDeformBindings
{
	uint model_deforms_;
	uint model_units_;
	uint model_meshes_;
	uint model_bones_;
	uint model_morphs_;
	uint bottom_level_table_;
	uint bottom_level_vertices_;
};
ConstantBuffer<ModelDeformBindings> deform_bindings : register(b0, space1);

struct ModelRaytracingBindings
{
	uint model_units_;
	uint model_traces_;
	uint model_meshes_;
	uint model_materials_;
	uint top_level_instances_;
	uint top_level_counts_;
};
ConstantBuffer<ModelRaytracingBindings> raytracing_bindings : register(b0, space1);

float3x3 ModelInverseTranspose(float3x3 matrix_3x3)
{
    float3 row0 = matrix_3x3[0];
    float3 row1 = matrix_3x3[1];
    float3 row2 = matrix_3x3[2];
    float3 cofactor0 = cross(row1, row2);
    float3 cofactor1 = cross(row2, row0);
    float3 cofactor2 = cross(row0, row1);
    float determinant = dot(row0, cofactor0);
    float inverse_determinant = abs(determinant) > 1e-20 ? 1.0 / determinant : 0.0;
    return float3x3(cofactor0, cofactor1, cofactor2) * inverse_determinant;
}

/**
* [EN]
* Writes the rows the CPU found changed into one of the Model's resident
* tables. The dispatch constant picks the table (0 units, 1 meshes,
* 2 clusters, 3 placements, 4 materials, 5 raytracing units); one thread
* writes one row.
*
* [JP]
* CPU が変わったと判定した行を、Model の常駐の表の 1 つへ書き込む。ディスパッチの
* 定数が表を選ぶ（0 ユニット、1 メッシュ、2 クラスタ、3 配置、4 マテリアル、
* 5 レイトレーシングのユニット）。
* 1 スレッドが 1 行を書く。
*/
[numthreads(64, 1, 1)]
void Update(uint3 id : SV_DispatchThreadID)
{
    uint table = dispatch_bindings.constants_;
    uint update_count;
    uint update_stride;
    if (table == 0)
    {
        StructuredBuffer<ModelUnitUpdate> updates = ResourceDescriptorHeap[update_bindings.model_unit_updates_];
        updates.GetDimensions(update_count, update_stride);
        if (id.x >= update_count)
        {
            return;
        }
        RWStructuredBuffer<ModelUnitStructuredBuffer> units = ResourceDescriptorHeap[update_bindings.model_units_];
        units[updates[id.x].index_] = updates[id.x].data_;
    }
    else if (table == 1)
    {
        StructuredBuffer<ModelMeshUpdate> updates = ResourceDescriptorHeap[update_bindings.model_mesh_updates_];
        updates.GetDimensions(update_count, update_stride);
        if (id.x >= update_count)
        {
            return;
        }
        RWStructuredBuffer<ModelMeshStructuredBuffer> meshes = ResourceDescriptorHeap[update_bindings.model_meshes_];
        meshes[updates[id.x].index_] = updates[id.x].data_;
    }
    else if (table == 2)
    {
        StructuredBuffer<ModelClusterUpdate> updates = ResourceDescriptorHeap[update_bindings.model_cluster_updates_];
        updates.GetDimensions(update_count, update_stride);
        if (id.x >= update_count)
        {
            return;
        }
        RWStructuredBuffer<ModelClusterStructuredBuffer> clusters = ResourceDescriptorHeap[update_bindings.model_clusters_];
        clusters[updates[id.x].index_] = updates[id.x].data_;
    }
    else if (table == 3)
    {
        StructuredBuffer<ModelPlacementUpdate> updates = ResourceDescriptorHeap[update_bindings.model_placement_updates_];
        updates.GetDimensions(update_count, update_stride);
        if (id.x >= update_count)
        {
            return;
        }
        RWStructuredBuffer<ModelPlacementStructuredBuffer> placements = ResourceDescriptorHeap[update_bindings.model_placements_];
        placements[updates[id.x].index_] = updates[id.x].data_;
    }
    else if (table == 4)
    {
        StructuredBuffer<ModelMaterialUpdate> updates = ResourceDescriptorHeap[update_bindings.model_material_updates_];
        updates.GetDimensions(update_count, update_stride);
        if (id.x >= update_count)
        {
            return;
        }
        RWStructuredBuffer<ModelMaterialStructuredBuffer> materials = ResourceDescriptorHeap[update_bindings.model_materials_];
        materials[updates[id.x].index_] = updates[id.x].data_;
    }
    else
    {
        StructuredBuffer<ModelRaytracingUpdate> updates = ResourceDescriptorHeap[update_bindings.model_trace_updates_];
        updates.GetDimensions(update_count, update_stride);
        if (id.x >= update_count)
        {
            return;
        }
        RWStructuredBuffer<ModelRaytracingStructuredBuffer> traces = ResourceDescriptorHeap[update_bindings.model_traces_];
        traces[updates[id.x].index_] = updates[id.x].data_;
    }
}

/**
* [EN]
* First dispatch of Model.Submit.Expand, one thread per word of
* Model.Streaming. Each sub-mesh row holds the finest LOD any view wants
* (reset to "none") and the largest on-screen texel density any view wants
* (reset to 0), which Expand narrows with atomics this frame.
*
* [JP]
* Model.Submit.Expand の最初のディスパッチ。Model.Streaming の 1 ワードに 1 スレッド。
* サブメッシュの行は、どれかのビューが望むいちばん細かい LOD（「無し」に戻す）と、
* どれかのビューが望むいちばん大きい画面上のテクセル密度（0 に戻す）を持ち、
* このフレームの Expand がアトミックで絞り込む。
*/
[numthreads(64, 1, 1)]
void Clear(uint3 id : SV_DispatchThreadID)
{
    RWStructuredBuffer<uint> streaming = ResourceDescriptorHeap[expand_bindings.model_streaming_];
    uint capacity;
    uint stride;
    streaming.GetDimensions(capacity, stride);
    if (id.x >= capacity)
    {
        return;
    }
    streaming[id.x] = (id.x & 1) == 0 ? MODEL_INVALID : 0;
}

/**
* [EN]
* Second dispatch of Model.Submit.Expand. Appends the Softbody instances the CPU
* built to Geometry's shared instance list, one thread per instance. The CPU
* lays them out opaque, transparent in that order; each thread takes a
* slot in its section with one InterlockedAdd and counts the instances that
* do not fit as overflow.
*
* [JP]
* Model.Submit.Expand の 2 つ目のディスパッチ。CPU が作った Softbody のインスタンスを、
* Geometry の共有のインスタンスの一覧へ書き足す。1 インスタンスに 1 スレッド。
* CPU はそれらを不透明・半透明の順に並べる。各スレッドは自分の区間の枠を
* InterlockedAdd 1 回で取り、入りきらない分は溢れとして数える。
*/
[numthreads(64, 1, 1)]
void Prepare(uint3 id : SV_DispatchThreadID)
{
    ConstantBuffer<ModelExpandConstantBuffer> constants = ResourceDescriptorHeap[expand_bindings.model_constants_];
    uint softbody_count = constants.softbody_opaque_count_ + constants.softbody_transparent_count_;
    if (id.x >= softbody_count)
    {
        return;
    }

    RWStructuredBuffer<uint> counts = ResourceDescriptorHeap[expand_bindings.geometry_counts_];
    StructuredBuffer<ModelStructuredBuffer> softbodies = ResourceDescriptorHeap[expand_bindings.model_softbodies_];
    RWStructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[expand_bindings.geometry_instances_];

    uint counter = MODEL_COUNT_OPAQUE;
    uint section_base = 0;
    uint section_end = constants.transparent_base_;
    if (id.x >= constants.softbody_opaque_count_)
    {
        counter = MODEL_COUNT_TRANSPARENT;
        section_base = constants.transparent_base_;
        section_end = constants.capacity_;
    }

    uint slot;
    InterlockedAdd(counts[counter], 1, slot);
    if (section_base + slot >= section_end)
    {
        InterlockedAdd(counts[MODEL_COUNT_OVERFLOW], 1);
        return;
    }
    instances[section_base + slot] = softbodies[id.x];
}

/**
* [EN]
* Third dispatch of Model.Submit.Expand, one thread per unit slot. For every
* sub-mesh of an active unit it drops the sub-mesh when no view's frustum
* holds its bounds, records in Model.Streaming the LOD the closest view needs
* and the largest on-screen texel density (whether or not they are resident,
* so the CPU can stream them in a few frames later), picks the LOD cluster
* the closest view needs among the resident ones (skinned sub-meshes stay on
* LOD 0), and writes one
* ModelStructuredBuffer per placement and per run of up to
* MODEL_MESHLETS_PER_INSTANCE meshlets into its section (opaque or
* transparent). A fur material writes that set once per layer, the base
* surface plus fur_shell_count_ shells, each tagged with its layer index.
* Each section is reserved with one InterlockedAdd per sub-mesh; what does
* not fit is counted as overflow.
*
* [JP]
* Model.Submit.Expand の 3 つ目のディスパッチ。ユニットの枠 1 つにつき 1 スレッド。
* 有効なユニットのサブメッシュごとに、どのビューの視錐台にも境界が入らなければ
* 落とし、いちばん近いビューが要る LOD と、いちばん大きい画面上のテクセル密度を
* Model.Streaming に記録し（常駐しているかに関係なく。CPU が数フレーム後に常駐させる）、
* 常駐しているクラスタの中から、いちばん近いビューが要る LOD を選び
* （スキンのサブメッシュは LOD 0 のまま）、配置ごと・最大
* MODEL_MESHLETS_PER_INSTANCE 個の meshlet ごとに ModelStructuredBuffer を 1 つずつ、
* その区間（不透明か半透明）へ書く。Fur のマテリアルは、その組をベースの面と
* fur_shell_count_ 枚のシェルの層ごとに 1 回ずつ書き、層の番号を付ける。各区間は
* サブメッシュごとに InterlockedAdd 1 回で確保し、入りきらない分は溢れとして数える。
*/
[numthreads(64, 1, 1)]
void Expand(uint3 id : SV_DispatchThreadID)
{
    ConstantBuffer<ModelExpandConstantBuffer> constants = ResourceDescriptorHeap[expand_bindings.model_constants_];
    if (id.x >= constants.unit_count_)
    {
        return;
    }

    StructuredBuffer<ModelUnitStructuredBuffer> units = ResourceDescriptorHeap[expand_bindings.model_units_];
    ModelUnitStructuredBuffer unit = units[id.x];
    if ((unit.flags_ & MODEL_UNIT_ACTIVE) == 0)
    {
        return;
    }

    StructuredBuffer<ModelMeshStructuredBuffer> meshes = ResourceDescriptorHeap[expand_bindings.model_meshes_];
    StructuredBuffer<ModelClusterStructuredBuffer> clusters = ResourceDescriptorHeap[expand_bindings.model_clusters_];
    StructuredBuffer<ModelPlacementStructuredBuffer> placements = ResourceDescriptorHeap[expand_bindings.model_placements_];
    StructuredBuffer<ModelMaterialStructuredBuffer> materials = ResourceDescriptorHeap[expand_bindings.model_materials_];
    RWStructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[expand_bindings.geometry_instances_];
    RWStructuredBuffer<uint> counts = ResourceDescriptorHeap[expand_bindings.geometry_counts_];
    RWStructuredBuffer<uint> streaming = ResourceDescriptorHeap[expand_bindings.model_streaming_];

    for (uint mesh_index = unit.mesh_offset_; mesh_index < unit.mesh_offset_ + unit.mesh_count_; mesh_index++)
    {
        ModelMeshStructuredBuffer mesh = meshes[mesh_index];
        if (mesh.cluster_count_ == 0 || mesh.placement_count_ == 0)
        {
            continue;
        }

        float4x4 lod_world = mul(placements[mesh.placement_offset_].placement_, unit.world_);
        float world_scale = max(max(length(lod_world[0].xyz), length(lod_world[1].xyz)), length(lod_world[2].xyz));
        float3 instance_position = lod_world[3].xyz;
        float3 bounds_center = mul(float4(mesh.position_min_ + mesh.position_extent_ * 0.5, 1.0), lod_world).xyz;
        float bounds_radius = length(mesh.position_extent_) * 0.5 * world_scale;

        bool visible = false;
        float pixels_per_unit = 0.0;
        float texture_pixels_per_unit = 0.0;
        for (uint view_index = 0; view_index < constants.view_count_; view_index++)
        {
            bool inside = true;
            for (uint plane_index = 0; plane_index < 6; plane_index++)
            {
                float4 plane = constants.planes_[view_index * 6 + plane_index];
                if (dot(plane.xyz, bounds_center) + plane.w < -bounds_radius)
                {
                    inside = false;
                }
            }
            if (!inside)
            {
                continue;
            }
            visible = true;
            float4 camera = constants.cameras_[view_index];
            float view_distance = max(length(instance_position - camera.xyz), 0.0001);
            pixels_per_unit = max(pixels_per_unit, camera.w / view_distance);
            float texture_distance = max(length(bounds_center - camera.xyz) - bounds_radius, 0.0001);
            texture_pixels_per_unit = max(texture_pixels_per_unit, camera.w / texture_distance);
        }
        if (!visible)
        {
            continue;
        }

        bool skinned = mesh.skin_index_ != MODEL_INVALID && unit.bone_offset_ != MODEL_INVALID;

        uint desired_lod = 0;
        if (!skinned)
        {
            for (uint lod = 0; lod < mesh.cluster_count_; lod++)
            {
                if (clusters[mesh.cluster_offset_ + lod].lod_error_ * world_scale * pixels_per_unit <= 1.0)
                {
                    desired_lod = lod;
                }
            }
        }
        InterlockedMin(streaming[mesh_index * 2], desired_lod);
        InterlockedMax(streaming[mesh_index * 2 + 1], asuint(world_scale * texture_pixels_per_unit));
        uint selected_cluster = MODEL_INVALID;
        if (skinned)
        {
            selected_cluster = mesh.cluster_offset_;
        }
        else
        {
            for (uint cluster_index = mesh.cluster_offset_; cluster_index < mesh.cluster_offset_ + mesh.cluster_count_; cluster_index++)
            {
                ModelClusterStructuredBuffer candidate = clusters[cluster_index];
                if ((candidate.flags_ & MODEL_CLUSTER_RESIDENT) == 0)
                {
                    continue;
                }
                if (selected_cluster == MODEL_INVALID || candidate.lod_error_ * world_scale * pixels_per_unit <= 1.0)
                {
                    selected_cluster = cluster_index;
                }
            }
        }
        if (selected_cluster == MODEL_INVALID)
        {
            continue;
        }

        ModelClusterStructuredBuffer cluster = clusters[selected_cluster];
        if (cluster.meshlet_count_ == 0)
        {
            continue;
        }

        ModelMaterialStructuredBuffer material = materials[unit.material_offset_ + mesh.surface_index_];

        ModelStructuredBuffer instance = (ModelStructuredBuffer)0;
        instance.shading_ = material.shading_;
        instance.shading_.selected_ = (unit.flags_ & MODEL_UNIT_SELECTED) != 0 ? 1 : 0;
        instance.shading_.actor_ = id.x;
        instance.texture_ = material.texture_;
        instance.extension_ = material.extension_;

        instance.geometry_.vertex_buffer_index_ = cluster.vertex_buffer_index_;
        instance.geometry_.meshlet_buffer_index_ = cluster.meshlet_buffer_index_;
        instance.geometry_.meshlet_bound_buffer_index_ = cluster.meshlet_bound_buffer_index_;
        instance.geometry_.vertex_indices_buffer_index_ = cluster.vertex_indices_buffer_index_;
        instance.geometry_.primitive_indices_buffer_index_ = cluster.primitive_indices_buffer_index_;

        instance.streaming_.position_min_ = mesh.position_min_;
        instance.streaming_.position_extent_ = mesh.position_extent_;
        instance.streaming_.texcoord_min_u_ = mesh.texcoord_min_u_;
        instance.streaming_.texcoord_min_v_ = mesh.texcoord_min_v_;
        instance.streaming_.texcoord_extent_ = mesh.texcoord_extent_;
        instance.streaming_.lod_error_ = cluster.lod_error_ * world_scale * pixels_per_unit;
        instance.streaming_.lod_level_ = selected_cluster - mesh.cluster_offset_;

        instance.skining_.skin_vertex_buffer_index_ = mesh.skin_vertex_buffer_index_;
        instance.skining_.skin_index_ = skinned ? mesh.skin_index_ : MODEL_INVALID;
        instance.skining_.bone_offset_ = skinned ? unit.bone_offset_ + mesh.skin_bone_offset_ : 0;

        instance.morph_.morph_delta_buffer_index_ = MODEL_INVALID;
        instance.morph_.vertex_morph_source_buffer_index_ = MODEL_INVALID;
        if (mesh.morph_target_count_ > 0 && unit.morph_weight_offset_ != MODEL_INVALID && (cluster.flags_ & MODEL_CLUSTER_STANDALONE_VERTICES) == 0)
        {
            instance.morph_.morph_delta_buffer_index_ = mesh.morph_delta_buffer_index_;
            instance.morph_.vertex_morph_source_buffer_index_ = mesh.vertex_morph_source_buffer_index_;
            instance.morph_.morph_delta_offset_ = mesh.morph_delta_offset_;
            instance.morph_.morph_vertex_offset_ = mesh.vertex_offset_;
            instance.morph_.morph_vertex_count_ = mesh.vertex_count_;
            instance.morph_.morph_target_count_ = mesh.morph_target_count_;
            instance.morph_.morph_weight_offset_ = unit.morph_weight_offset_ + mesh.morph_weight_offset_;
        }

        uint chunk_count = (cluster.meshlet_count_ + MODEL_MESHLETS_PER_INSTANCE - 1) / MODEL_MESHLETS_PER_INSTANCE;
        bool transparent = material.shading_.blend_ != 0;
        bool fur = !transparent && material.shading_.shading_model_ == SHADING_MODEL_FUR;
        uint layer_count = fur ? material.shading_.fur_shell_count_ + 1 : 1;
        uint instance_count = chunk_count * mesh.placement_count_ * layer_count;
        uint counter = transparent ? MODEL_COUNT_TRANSPARENT : MODEL_COUNT_OPAQUE;
        uint section_base = transparent ? constants.transparent_base_ : 0;
        uint section_end = transparent ? constants.capacity_ : constants.transparent_base_;

        uint first;
        InterlockedAdd(counts[counter], instance_count, first);
        if (section_base + first + instance_count > section_end)
        {
            InterlockedAdd(counts[MODEL_COUNT_OVERFLOW], instance_count);
            continue;
        }

        uint write_index = section_base + first;
        for (uint placement_index = 0; placement_index < mesh.placement_count_; placement_index++)
        {
            float4x4 placement = placements[mesh.placement_offset_ + placement_index].placement_;
            float4x4 world = mul(placement, unit.world_);
            float3x3 inverse_transpose = ModelInverseTranspose((float3x3)world);
            float3 inverse_translation = -mul(world[3].xyz, transpose(inverse_transpose));

            instance.transform_.world_ = world;
            instance.transform_.inverse_transpose_world_ = float4x4(float4(inverse_transpose[0], inverse_translation.x), float4(inverse_transpose[1], inverse_translation.y), float4(inverse_transpose[2], inverse_translation.z), float4(0.0, 0.0, 0.0, 1.0));
            instance.transform_.previous_world_ = mul(placement, unit.previous_world_);

            for (uint layer = 0; layer < layer_count; layer++)
            {
                instance.geometry_.layer_ = layer;
                for (uint chunk_index = 0; chunk_index < chunk_count; chunk_index++)
                {
                    uint meshlet_offset = chunk_index * MODEL_MESHLETS_PER_INSTANCE;
                    instance.geometry_.meshlet_offset_ = meshlet_offset;
                    instance.geometry_.meshlet_count_ = min(MODEL_MESHLETS_PER_INSTANCE, cluster.meshlet_count_ - meshlet_offset);
                    instances[write_index] = instance;
                    write_index++;
                }
            }
        }
    }
}

/**
* [EN]
* Deforms the raytracing proxy positions of animated Models into
* BottomLevel.Vertices, at the offset BottomLevel.Table holds for the job's
* BLAS key; a key with no offset this frame writes nothing. One job covers
* one SubMesh's compact proxy vertex range (id.y picks the job, id.x the
* vertex): morph
* deltas are added first, then the skin matrix is applied, matching the
* order the rasterized path uses. A non-finite result keeps the bind pose so
* a broken pose never corrupts the dynamic BLAS.
*
* ---------------------------------------------------------------------
*
* [JP]
* アニメーションする Model のレイトレーシング用プロキシの位置を、
* BottomLevel.Vertices の中で、BottomLevel.Table がジョブの BLAS の key に対して
* 持つオフセットの位置へ変形して書く。このフレームにオフセットが無い key は何も
* 書かない。1 ジョブが 1 SubMesh のコンパクトなプロキシ頂点の範囲を
* 受け持つ（id.y がジョブ、id.x が頂点）。ラスタライズの経路と同じ順で、先にモーフの
* 差分を足し、次にスキンの行列を掛ける。結果が有限でなければバインドポーズのまま
* にして、壊れたポーズが動的な BLAS を壊さないようにする。
*/
[numthreads(64, 1, 1)]
void Deform(uint3 id : SV_DispatchThreadID)
{
    return;
	StructuredBuffer<ModelDeformStructuredBuffer> deforms = ResourceDescriptorHeap[deform_bindings.model_deforms_];
	ModelDeformStructuredBuffer deform = deforms[id.y];
	if (id.x >= deform.vertex_count_)
	{
		return;
	}

	StructuredBuffer<ModelUnitStructuredBuffer> units = ResourceDescriptorHeap[deform_bindings.model_units_];
	StructuredBuffer<ModelMeshStructuredBuffer> meshes = ResourceDescriptorHeap[deform_bindings.model_meshes_];
	StructuredBuffer<float3> positions = ResourceDescriptorHeap[deform.position_buffer_index_];
	StructuredBuffer<BottomLevelTableStructuredBuffer> table = ResourceDescriptorHeap[deform_bindings.bottom_level_table_];
	uint vertex_offset = BottomLevelFind(table, deform.bottom_level_).vertex_offset_;
	if (vertex_offset == 0xFFFFFFFF)
	{
		return;
	}
	RWStructuredBuffer<float3> vertices = ResourceDescriptorHeap[deform_bindings.bottom_level_vertices_];

	ModelUnitStructuredBuffer unit = units[deform.unit_index_];
	ModelMeshStructuredBuffer mesh = meshes[unit.mesh_offset_ + deform.sub_mesh_index_];

	uint vertex_index = deform.vertex_offset_ + id.x;
	float3 base_position = positions[vertex_index];
	float3 position = base_position;

	if (deform.morph_target_count_ > 0 && unit.morph_weight_offset_ != MODEL_INVALID)
	{
		StructuredBuffer<float3> morph_deltas = ResourceDescriptorHeap[deform.morph_delta_buffer_index_];
		StructuredBuffer<float> morphs = ResourceDescriptorHeap[deform_bindings.model_morphs_];
		uint weight_offset = unit.morph_weight_offset_ + mesh.morph_weight_offset_;
		for (uint target = 0; target < deform.morph_target_count_; target++)
		{
			position += morph_deltas[deform.morph_delta_offset_ + target * deform.vertex_count_ + id.x] * morphs[weight_offset + target];
		}
	}

	if (deform.skin_vertex_buffer_index_ != MODEL_INVALID && mesh.skin_index_ != MODEL_INVALID && unit.bone_offset_ != MODEL_INVALID)
	{
		StructuredBuffer<CompressedModelSkin> skin_vertices = ResourceDescriptorHeap[deform.skin_vertex_buffer_index_];
		StructuredBuffer<ModelBoneMatrix> bone_matrices = ResourceDescriptorHeap[deform_bindings.model_bones_];
		ExpandedModelSkin skin = DecodeSkinVertex(skin_vertices[vertex_index]);
		if (dot(skin.weights_, 1.0) >= 1e-5)
		{
			uint bone_offset = unit.bone_offset_ + mesh.skin_bone_offset_;
			float4x4 skin_matrix =
				LoadBoneMatrix(bone_matrices[bone_offset + skin.joints_.x]) * skin.weights_.x +
				LoadBoneMatrix(bone_matrices[bone_offset + skin.joints_.y]) * skin.weights_.y +
				LoadBoneMatrix(bone_matrices[bone_offset + skin.joints_.z]) * skin.weights_.z +
				LoadBoneMatrix(bone_matrices[bone_offset + skin.joints_.w]) * skin.weights_.w;
			position = mul(float4(position, 1.0), skin_matrix).xyz;
		}
	}

	if (!all(isfinite(position)))
	{
		position = base_position;
	}

	vertices[vertex_offset + deform.output_offset_ + id.x] = position;
}

/**
* [EN]
* Hands every Model unit that has a raytracing row to TopLevel.Instances,
* one thread per unit. Each thread takes an instance slot with one
* InterlockedAdd and writes the TLAS transform, the BLAS key and the
* hit row raytracing shaders read: geometry buffers, the surface table and
* where this unit's materials live in Model's material table. Units with no
* BLAS or no meshes are skipped.
*
* ---------------------------------------------------------------------
*
* [JP]
* レイトレーシングの行を持つ Model のユニットを、TopLevel.Instances へ渡す。
* 1 ユニットに 1 スレッド。各スレッドは InterlockedAdd 1 回でインスタンスの枠を
* 取り、TLAS の変換行列、BLAS の key、レイトレーシングのシェーダが読むヒットの行（ジオメトリの
* バッファ、サーフェスの表、Model のマテリアルの表の中でこのユニットのマテリアル
* がある場所）を書く。BLAS が無いユニットとメッシュが無いユニットは飛ばす。
*/
[numthreads(64, 1, 1)]
void Raytracing(uint3 id : SV_DispatchThreadID)
{
	if (id.x >= dispatch_bindings.constants_)
	{
		return;
	}

	StructuredBuffer<ModelRaytracingStructuredBuffer> traces = ResourceDescriptorHeap[raytracing_bindings.model_traces_];
	ModelRaytracingStructuredBuffer trace = traces[id.x];
	if (all(trace.bottom_level_ == 0))
	{
		return;
	}

	StructuredBuffer<ModelUnitStructuredBuffer> units = ResourceDescriptorHeap[raytracing_bindings.model_units_];
	ModelUnitStructuredBuffer unit = units[id.x];
	if (unit.mesh_count_ == 0)
	{
		return;
	}

	RWStructuredBuffer<TopLevelInstanceStructuredBuffer> instances = ResourceDescriptorHeap[raytracing_bindings.top_level_instances_];
	RWStructuredBuffer<uint> counts = ResourceDescriptorHeap[raytracing_bindings.top_level_counts_];
	uint capacity;
	uint stride;
	instances.GetDimensions(capacity, stride);

	uint slot;
	InterlockedAdd(counts[0], 1, slot);
	if (slot >= capacity)
	{
		return;
	}

	float4x4 world = unit.world_;
	TopLevelInstanceStructuredBuffer instance;
	instance.transform_[0] = float4(world[0][0], world[1][0], world[2][0], world[3][0]);
	instance.transform_[1] = float4(world[0][1], world[1][1], world[2][1], world[3][1]);
	instance.transform_[2] = float4(world[0][2], world[1][2], world[2][2], world[3][2]);
	instance.bottom_level_ = trace.bottom_level_;
	instance.instance_mask_ = 0xFF;
	instance.flags_ = 0;
	StructuredBuffer<ModelMeshStructuredBuffer> meshes = ResourceDescriptorHeap[raytracing_bindings.model_meshes_];
	StructuredBuffer<ModelMaterialStructuredBuffer> materials = ResourceDescriptorHeap[raytracing_bindings.model_materials_];
	for (uint mesh_index = unit.mesh_offset_; mesh_index < unit.mesh_offset_ + unit.mesh_count_; mesh_index++)
	{
		if (materials[unit.material_offset_ + meshes[mesh_index].surface_index_].shading_.blend_ != 0)
		{
			instance.flags_ = 0x8;
		}
	}
	instance.hit_.vertex_buffer_index_ = trace.vertex_buffer_index_;
	instance.hit_.index_buffer_index_ = trace.index_buffer_index_;
	instance.hit_.surface_buffer_index_ = trace.surface_buffer_index_;
	instance.hit_.material_buffer_index_ = raytracing_bindings.model_materials_;
	instance.hit_.material_offset_ = unit.material_offset_;
	instance.hit_.top_level_hit_structured_buffer_padding_0_ = 0;
	instance.hit_.texcoord_min_ = trace.texcoord_min_;
	instance.hit_.texcoord_extent_ = trace.texcoord_extent_;
	instance.hit_.top_level_hit_structured_buffer_padding_1_ = 0;
	instance.hit_.top_level_hit_structured_buffer_padding_2_ = 0;
	instances[slot] = instance;
}
