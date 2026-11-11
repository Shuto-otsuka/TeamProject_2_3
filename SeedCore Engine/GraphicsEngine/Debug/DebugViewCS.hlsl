#include <GraphicsEngine/Shader/Scene.hlsli>
#include <GraphicsEngine/Shader/Normal.hlsli>

struct DebugViewResolveBindings
{
	uint geometry_depth_;
	uint geometry_visibility_id_;
	uint material_surface_base_color_;
	uint material_surface_normal_roughness_;
	uint material_velocity_;
	uint material_surface_radiance_;
	uint hud_display_color_;
};
ConstantBuffer<DebugViewResolveBindings> bindings : register(b0, space1);

/**
* [EN]
* Replaces DisplayColor with a debug image of the view's render-resolution
* buffers, one thread per display pixel. The dispatch constant is the
* RenderViewMode: Unlit (1) base color, Depth (3) linear view depth over 50
* units with near white and far black, Meshlet (4) one color per
* instance/meshlet pair from a hash of the VisibilityId, Normal (5),
* Roughness (6), Metallic (7), Emissive (8) the emitted radiance, Velocity (9)
* remapped around gray. Surface modes show the background black.
*/
[numthreads(8, 8, 1)]
void Resolve(uint3 id : SV_DispatchThreadID)
{
	RWTexture2D<float4> display_color = ResourceDescriptorHeap[bindings.hud_display_color_];
	uint display_width;
	uint display_height;
	display_color.GetDimensions(display_width, display_height);
	if (id.x >= display_width || id.y >= display_height)
	{
		return;
	}

	Texture2D<float> depth_texture = ResourceDescriptorHeap[bindings.geometry_depth_];
	uint render_width;
	uint render_height;
	depth_texture.GetDimensions(render_width, render_height);
	float2 uv = (float2(id.xy) + 0.5) / float2(display_width, display_height);
	uint2 pixel = min(uint2(uv * float2(render_width, render_height)), uint2(render_width - 1, render_height - 1));
	float depth = depth_texture.Load(int3(pixel, 0));
	uint mode = dispatch_bindings.constants_;

	float3 color = float3(0.0, 0.0, 0.0);
	if (mode == 3)
	{
		SceneConstantBuffer scene = GetSceneConstantBuffer();
		float4 view_position = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0), scene.inverse_projection_);
		float view_z = depth > 0.0 ? view_position.z / view_position.w : 1e30;
		color = 1.0 - saturate(view_z / 50.0);
	}
	else if (mode == 9)
	{
		Texture2D<float2> velocity = ResourceDescriptorHeap[bindings.material_velocity_];
		color = float3(velocity.Load(int3(pixel, 0)) * 0.5 + 0.5, 0.0);
	}
	else if (depth > 0.0)
	{
		if (mode == 4)
		{
			Texture2D<uint2> visibility_id = ResourceDescriptorHeap[bindings.geometry_visibility_id_];
			uint2 packed = visibility_id.Load(int3(pixel, 0));
			uint hash = packed.x * 747796405u + (packed.y >> 7) * 2891336453u;
			hash = ((hash >> ((hash >> 28) + 4)) ^ hash) * 277803737u;
			hash = (hash >> 22) ^ hash;
			color = float3(hash & 0xFF, (hash >> 8) & 0xFF, (hash >> 16) & 0xFF) / 255.0;
		}
		else if (mode == 1 || mode == 7)
		{
			Texture2D<float4> surface_base_color = ResourceDescriptorHeap[bindings.material_surface_base_color_];
			float4 base_color = surface_base_color.Load(int3(pixel, 0));
			color = mode == 1 ? base_color.rgb : base_color.aaa;
		}
		else if (mode == 5 || mode == 6)
		{
			Texture2D<uint> surface_normal_roughness = ResourceDescriptorHeap[bindings.material_surface_normal_roughness_];
			uint packed = surface_normal_roughness.Load(int3(pixel, 0));
			float3 normal = OctNormalDecode(float2(packed & 0xFFF, (packed >> 12) & 0xFFF) / 4095.0);
			color = mode == 5 ? normal * 0.5 + 0.5 : (float)(packed >> 24) / 255.0;
		}
		else if (mode == 8)
		{
			Texture2D<float4> surface_radiance = ResourceDescriptorHeap[bindings.material_surface_radiance_];
			color = surface_radiance.Load(int3(pixel, 0)).rgb;
		}
	}
	display_color[id.xy] = float4(color, 1.0);
}
