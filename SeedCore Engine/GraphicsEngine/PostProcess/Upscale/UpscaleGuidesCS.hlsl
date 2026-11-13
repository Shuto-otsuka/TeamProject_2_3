#include <GraphicsEngine/Shader/Scene.hlsli>
#include <GraphicsEngine/Shader/Normal.hlsli>
#include <GraphicsEngine/Model/Model.hlsli>

struct UpscaleGuidesBindings
{
	uint material_surface_base_color_;
	uint material_surface_normal_roughness_;
	uint geometry_visibility_id_;
	uint geometry_depth_;
	uint geometry_instances_;
	uint upscale_normal_roughness_;
	uint upscale_specular_albedo_;
	uint upscale_diffuse_albedo_;
};
ConstantBuffer<UpscaleGuidesBindings> bindings : register(b0, space1);

/**
* [EN]
* Writes the guides NVIDIA DLSS Ray Reconstruction expects: RGB=normal /
* A=roughness decoded from Material.SurfaceNormalRoughness (12-bit
* octahedral normal and 8-bit roughness), the specular albedo from the
* split-sum approximation, and the diffuse albedo. DLSS-RR wants a plain float
* normal and roughness rather than the engine's packing, so this full-screen
* pass runs once per view right before Upscale.DlssRayReconstruction.
*
* [JP]
* NVIDIA DLSS Ray Reconstruction が要求するガイドを書く。RGB=法線 / A=ラフネスは
* Material.SurfaceNormalRoughness（12 ビットの八面体法線と 8 ビットのラフネス）
* から戻したもの、スペキュラアルベドは split-sum の近似、それと拡散アルベド。
* DLSS-RR はエンジン独自のパッキングではなく素の float の法線とラフネスを求める
* ので、各ビューの Upscale.DlssRayReconstruction の直前に 1 回だけ走る全画面の
* パス。
*/
float3 EnvBRDFApprox2(float3 specular_color, float alpha, float n_o_v)
{
	n_o_v = abs(n_o_v);
	float4 x;
	x.x = 1.0;
	x.y = n_o_v;
	x.z = n_o_v * n_o_v;
	x.w = n_o_v * x.z;
	float4 y;
	y.x = 1.0;
	y.y = alpha;
	y.z = alpha * alpha;
	y.w = alpha * y.z;
	float2x2 m1 = float2x2(0.99044f, -1.28514f, 1.29678f, -0.755907f);
	float3x3 m2 = float3x3(1.0f, 2.92338f, 59.4188f, 20.3225f, -27.0302f, 222.592f, 121.563f, 626.13f, 316.627f);
	float2x2 m3 = float2x2(0.0365463f, 3.32707f, 9.0632f, -9.04756f);
	float3x3 m4 = float3x3(1.0f, 3.59685f, -1.36772f, 9.04401f, -16.3174f, 9.22949f, 5.56589f, 19.7886f, -20.2123f);
	float bias = dot(mul(m1, x.xy), y.xy) * rcp(dot(mul(m2, x.xyw), y.xyw));
	float scale = dot(mul(m3, x.xy), y.xy) * rcp(dot(mul(m4, x.xzw), y.xyw));
	bias *= saturate(specular_color.g * 50.0);
	return mad(specular_color, max(0.0, scale), max(0.0, bias));
}

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<float4> normal_roughness = ResourceDescriptorHeap[bindings.upscale_normal_roughness_];
	uint width;
	uint height;
	normal_roughness.GetDimensions(width, height);
	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	uint2 pixel = dtid.xy;
	Texture2D<float4> surface_base_color = ResourceDescriptorHeap[bindings.material_surface_base_color_];
	Texture2D<uint> surface_normal_roughness = ResourceDescriptorHeap[bindings.material_surface_normal_roughness_];
	Texture2D<uint2> visibility_id = ResourceDescriptorHeap[bindings.geometry_visibility_id_];
	Texture2D<float> depth_texture = ResourceDescriptorHeap[bindings.geometry_depth_];
	RWTexture2D<float4> specular_albedo = ResourceDescriptorHeap[bindings.upscale_specular_albedo_];
	RWTexture2D<float4> diffuse_albedo = ResourceDescriptorHeap[bindings.upscale_diffuse_albedo_];

	float depth = depth_texture.Load(int3(pixel, 0));
	if (depth == 0.0 || visibility_id.Load(int3(pixel, 0)).x == 0)
	{
		normal_roughness[pixel] = float4(0.0, 0.0, 0.0, 1.0);
		specular_albedo[pixel] = float4(0.0, 0.0, 0.0, 1.0);
		diffuse_albedo[pixel] = float4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	uint packed = surface_normal_roughness.Load(int3(pixel, 0));
	float3 normal = OctNormalDecode(float2(packed & 0xFFF, (packed >> 12) & 0xFFF) / 4095.0);
	float roughness = (float)(packed >> 24) / 255.0;
	normal_roughness[pixel] = float4(normal, roughness);

	float2 uv = (float2(pixel) + 0.5) / float2(width, height);
	float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
	float4 world = mul(float4(ndc, depth, 1.0), scene.inverse_view_projection_);
	float3 view_direction = normalize(scene.camera_position_.xyz - world.xyz / world.w);
	float n_o_v = dot(normal, view_direction);

	float4 base_color_metallic = surface_base_color.Load(int3(pixel, 0));
	uint instance_index;
	uint meshlet_index;
	uint triangle_index;
	UnpackVisibilityID(visibility_id.Load(int3(pixel, 0)), instance_index, meshlet_index, triangle_index);
	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[bindings.geometry_instances_];
	ModelStructuredBuffer instance = instances[instance_index];
	float dielectric = (instance.texture_.ior_ - 1.0) / (instance.texture_.ior_ + 1.0);
	dielectric *= dielectric;
	float3 dielectric_f0 = saturate(dielectric * instance.extension_.specular_color_ * instance.extension_.specular_factor_);
	float3 specular_color = lerp(dielectric_f0, base_color_metallic.rgb, base_color_metallic.a);

	specular_albedo[pixel] = float4(EnvBRDFApprox2(specular_color, roughness * roughness, n_o_v), 1.0);
	diffuse_albedo[pixel] = float4(base_color_metallic.rgb * (1.0 - base_color_metallic.a), 1.0);
}
