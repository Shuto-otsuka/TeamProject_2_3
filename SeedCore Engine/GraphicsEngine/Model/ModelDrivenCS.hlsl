#include <GraphicsEngine/Model/Model.hlsli>
#include <GraphicsEngine/Shader/RootSignature.hlsli>

struct ModelUpdateBindings
{
    uint model_unit_updates_;
    uint model_mesh_updates_;
    uint model_cluster_updates_;
    uint model_placement_updates_;
    uint model_material_updates_;
    uint model_units_;
    uint model_meshes_;
    uint model_clusters_;
    uint model_placements_;
    uint model_materials_;
};
ConstantBuffer<ModelUpdateBindings> update_bindings : register(b0, space1);

struct ModelExpandBindings
{
    uint model_units_;
    uint model_meshes_;
    uint model_clusters_;
    uint model_placements_;
    uint model_materials_;
    uint model_softbody_instances_;
    uint model_constants_;
    uint model_instances_;
    uint model_counts_;
};
ConstantBuffer<ModelExpandBindings> expand_bindings : register(b0, space1);

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
* 2 clusters, 3 placements, 4 materials); one thread writes one row.
*
* [JP]
* CPU が変わったと判定した行を、Model の常駐の表の 1 つへ書き込む。ディスパッチの
* 定数が表を選ぶ（0 ユニット、1 メッシュ、2 クラスタ、3 配置、4 マテリアル）。
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
    else
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
}

/**
* [EN]
* First dispatch of Model.Expand. Starts every section's counter at the
* number of Softbody instances the CPU built for it, and copies those
* instances to the front of their sections; the unit dispatch then appends
* after them. The CPU lays the Softbody instances out opaque, transparent,
* fur in that order.
*
* [JP]
* Model.Expand の最初のディスパッチ。各区間のカウンターを、CPU がその区間に作った
* Softbody のインスタンスの数から始め、それらを区間の先頭へ写す。ユニットの
* ディスパッチはその後ろに詰める。CPU は Softbody のインスタンスを、不透明・半透明・
* Fur の順に並べる。
*/
[numthreads(64, 1, 1)]
void Prepare(uint3 id : SV_DispatchThreadID)
{
    ConstantBuffer<ModelExpandConstantBuffer> constants = ResourceDescriptorHeap[expand_bindings.model_constants_];
    RWStructuredBuffer<uint> counts = ResourceDescriptorHeap[expand_bindings.model_counts_];

    if (id.x == 0)
    {
        counts[MODEL_COUNT_OPAQUE] = constants.softbody_opaque_count_;
        counts[MODEL_COUNT_TRANSPARENT] = constants.softbody_transparent_count_;
        counts[MODEL_COUNT_FUR] = constants.softbody_fur_count_;
        counts[MODEL_COUNT_OVERFLOW] = 0;
    }

    uint softbody_count = constants.softbody_opaque_count_ + constants.softbody_transparent_count_ + constants.softbody_fur_count_;
    if (id.x >= softbody_count)
    {
        return;
    }

    StructuredBuffer<ModelStructuredBuffer> softbody_instances = ResourceDescriptorHeap[expand_bindings.model_softbody_instances_];
    RWStructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[expand_bindings.model_instances_];

    uint destination = id.x;
    if (id.x >= constants.softbody_opaque_count_ + constants.softbody_transparent_count_)
    {
        destination = constants.fur_base_ + (id.x - constants.softbody_opaque_count_ - constants.softbody_transparent_count_);
    }
    else if (id.x >= constants.softbody_opaque_count_)
    {
        destination = constants.transparent_base_ + (id.x - constants.softbody_opaque_count_);
    }
    instances[destination] = softbody_instances[id.x];
}

/**
* [EN]
* Second dispatch of Model.Expand, one thread per unit slot. For every
* sub-mesh of an active unit it drops the sub-mesh when no view's frustum
* holds its bounds, picks the LOD cluster the closest view needs among the
* resident ones (skinned sub-meshes stay on LOD 0), and writes one
* ModelStructuredBuffer per placement and per run of up to
* MODEL_MESHLETS_PER_INSTANCE meshlets into its section (opaque, transparent,
* plus fur for the fur shading model). Each section is reserved with one
* InterlockedAdd per sub-mesh; what does not fit is counted as overflow.
*
* [JP]
* Model.Expand の 2 つ目のディスパッチ。ユニットの枠 1 つにつき 1 スレッド。
* 有効なユニットのサブメッシュごとに、どのビューの視錐台にも境界が入らなければ
* 落とし、常駐しているクラスタの中から、いちばん近いビューが要る LOD を選び
* （スキンのサブメッシュは LOD 0 のまま）、配置ごと・最大
* MODEL_MESHLETS_PER_INSTANCE 個の meshlet ごとに ModelStructuredBuffer を 1 つずつ、
* その区間（不透明、半透明、Fur のシェーディングモデルなら Fur にも）へ書く。各区間は
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
    RWStructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[expand_bindings.model_instances_];
    RWStructuredBuffer<uint> counts = ResourceDescriptorHeap[expand_bindings.model_counts_];

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
        }
        if (!visible)
        {
            continue;
        }

        bool skinned = mesh.skin_index_ != MODEL_INVALID && unit.bone_offset_ != MODEL_INVALID;
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
        instance.streaming_.lod_error_ = 0.0;
        instance.streaming_.lod_error_next_ = MODEL_FLOAT_MAX;

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
        uint instance_count = chunk_count * mesh.placement_count_;

        bool transparent = material.shading_.blend_ != 0;
        bool fur = !transparent && material.shading_.shading_model_ == SHADING_MODEL_FUR;
        uint section_count = fur ? 2 : 1;
        for (uint section_index = 0; section_index < section_count; section_index++)
        {
            uint counter = section_index == 1 ? MODEL_COUNT_FUR : (transparent ? MODEL_COUNT_TRANSPARENT : MODEL_COUNT_OPAQUE);
            uint section_base = counter == MODEL_COUNT_FUR ? constants.fur_base_ : (counter == MODEL_COUNT_TRANSPARENT ? constants.transparent_base_ : 0);
            uint section_end = counter == MODEL_COUNT_FUR ? constants.capacity_ : (counter == MODEL_COUNT_TRANSPARENT ? constants.fur_base_ : constants.transparent_base_);

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
