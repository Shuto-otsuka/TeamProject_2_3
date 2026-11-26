#include <GraphicsEngine/PostProcess/Outline/Outline.hlsli>
#include <GraphicsEngine/Model/Model.hlsli>
#include <GraphicsEngine/Shader/Normal.hlsli>
#include <GraphicsEngine/Shader/RootSignature.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>

struct OutlineStyleBindings
{
	uint outline_updates_;
	uint outline_styles_;
};
ConstantBuffer<OutlineStyleBindings> style_bindings : register(b0, space1);

struct OutlineMaskBindings
{
	uint outline_mask_;
	uint outline_widest_;
};
ConstantBuffer<OutlineMaskBindings> mask_bindings : register(b0, space1);

struct OutlineResolveBindings
{
	uint outline_mask_;
	uint outline_styles_;
	uint geometry_depth_;
	uint geometry_visibility_id_;
	uint geometry_instances_;
	uint material_normal_roughness_;
	uint outline_source_;
	uint outline_widest_;
};
ConstantBuffer<OutlineResolveBindings> resolve_bindings : register(b0, space1);

struct OutlineCanvasBindings
{
	uint outline_mask_;
	uint outline_styles_;
	uint outline_source_;
	uint outline_widest_;
};
ConstantBuffer<OutlineCanvasBindings> canvas_bindings : register(b0, space1);

[numthreads(64, 1, 1)]
void Style(uint3 dtid : SV_DispatchThreadID)
{
	StructuredBuffer<OutlineStyleUpdate> updates = ResourceDescriptorHeap[style_bindings.outline_updates_];
	uint update_count;
	uint update_stride;
	updates.GetDimensions(update_count, update_stride);
	if (dtid.x >= update_count)
	{
		return;
	}
	RWStructuredBuffer<OutlineStyleStructuredBuffer> styles = ResourceDescriptorHeap[style_bindings.outline_styles_];
	styles[updates[dtid.x].index_] = updates[dtid.x].data_;
}

[numthreads(8, 8, 1)]
void Mask(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<uint> mask = ResourceDescriptorHeap[mask_bindings.outline_mask_];
	uint width;
	uint height;
	mask.GetDimensions(width, height);
	if (dtid.x == 0 && dtid.y == 0)
	{
		RWStructuredBuffer<uint> widest = ResourceDescriptorHeap[mask_bindings.outline_widest_];
		widest[0] = 0;
	}
	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}
	mask[dtid.xy] = 0;
}

[numthreads(8, 8, 1)]
void Resolve(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<uint4> source = ResourceDescriptorHeap[resolve_bindings.outline_source_];
	uint width;
	uint height;
	source.GetDimensions(width, height);
	bool inside = dtid.x < width && dtid.y < height;

	float outline_width = 0.0;
	if (inside)
	{
		Texture2D<uint> mask = ResourceDescriptorHeap[resolve_bindings.outline_mask_];
		Texture2D<float> depth = ResourceDescriptorHeap[resolve_bindings.geometry_depth_];
		Texture2D<uint2> visibility_id = ResourceDescriptorHeap[resolve_bindings.geometry_visibility_id_];
		StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[resolve_bindings.geometry_instances_];
		StructuredBuffer<OutlineStyleStructuredBuffer> styles = ResourceDescriptorHeap[resolve_bindings.outline_styles_];

		int2 pixel = int2(dtid.xy);
		uint record = mask.Load(int3(pixel, 0));
		float center_depth = depth.Load(int3(pixel, 0));

		uint actor = MODEL_INVALID;
		bool opaque = false;
		if (record != 0)
		{
			actor = record & 0x1FFFF;
		}
		else if (center_depth != 0.0 && visibility_id.Load(int3(pixel, 0)).x != 0)
		{
			actor = instances[visibility_id.Load(int3(pixel, 0)).x - 1].shading_.actor_;
			opaque = true;
		}

		uint4 outline = uint4(0, 0, 0, 0);
		if (actor != MODEL_INVALID)
		{
			OutlineStyleStructuredBuffer style = styles[actor];
			bool contour = false;
			if (dispatch_bindings.constants_ != 0 && (style.flags_ & OUTLINE_STYLE_SELECTED) != 0)
			{
				uint3 selection = uint3(float3(0.95, 0.55, 0.05) * 255.0 + 0.5);
				outline.x = selection.r | (selection.g << 8) | (selection.b << 16) | (255u << 24);
				outline_width = 3.0 * (GetSceneConstantBuffer().display_size_.y / float(height));
			}
			else if ((style.flags_ & OUTLINE_STYLE_OUTLINE) != 0)
			{
				outline.x = style.color_;
				outline_width = style.width_;
				contour = opaque && (style.flags_ & OUTLINE_STYLE_CONTOUR) != 0;
			}

			if (outline_width > 0.0)
			{
				outline.y = asuint(outline_width);
				outline.z = actor + 1;
				if (contour)
				{
					outline.w = OUTLINE_SOURCE_CONTOUR;

					Texture2D<uint> surface_normal_roughness = ResourceDescriptorHeap[resolve_bindings.material_normal_roughness_];
					uint center_packed = surface_normal_roughness.Load(int3(pixel, 0));
					float3 center_normal = OctNormalDecode(float2(center_packed & 0xFFF, (center_packed >> 12) & 0xFFF) / 4095.0);
					int2 pixel_max = int2(width - 1, height - 1);
					int2 offsets[4] = { int2(1, 0), int2(-1, 0), int2(0, 1), int2(0, -1) };
					for (uint offset_index = 0; offset_index < 4; offset_index++)
					{
						int2 neighbor = clamp(pixel + offsets[offset_index], int2(0, 0), pixel_max);
						float neighbor_depth = depth.Load(int3(neighbor, 0));
						if (neighbor_depth == 0.0 || visibility_id.Load(int3(neighbor, 0)).x == 0 || instances[visibility_id.Load(int3(neighbor, 0)).x - 1].shading_.actor_ != actor)
						{
							continue;
						}
						uint neighbor_packed = surface_normal_roughness.Load(int3(neighbor, 0));
						float3 neighbor_normal = OctNormalDecode(float2(neighbor_packed & 0xFFF, (neighbor_packed >> 12) & 0xFFF) / 4095.0);
						if (center_depth - neighbor_depth > center_depth * 0.05 || dot(center_normal, neighbor_normal) < 0.7)
						{
							outline.w |= OUTLINE_SOURCE_EDGE;
						}
					}
				}
			}
		}
		source[dtid.xy] = outline;
	}

	float wave_widest = WaveActiveMax(outline_width);
	if (WaveIsFirstLane() && wave_widest > 0.0)
	{
		RWStructuredBuffer<uint> widest = ResourceDescriptorHeap[resolve_bindings.outline_widest_];
		InterlockedMax(widest[0], asuint(wave_widest));
	}
}

[numthreads(8, 8, 1)]
void Canvas(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<uint4> source = ResourceDescriptorHeap[canvas_bindings.outline_source_];
	uint width;
	uint height;
	source.GetDimensions(width, height);
	bool inside = dtid.x < width && dtid.y < height;

	float outline_width = 0.0;
	if (inside)
	{
		Texture2D<uint> mask = ResourceDescriptorHeap[canvas_bindings.outline_mask_];
		StructuredBuffer<OutlineStyleStructuredBuffer> styles = ResourceDescriptorHeap[canvas_bindings.outline_styles_];

		uint record = mask.Load(int3(dtid.xy, 0));
		uint4 outline = uint4(0, 0, 0, 0);
		if (record != 0)
		{
			uint actor = record & 0x1FFFF;
			OutlineStyleStructuredBuffer style = styles[actor];
			if ((style.flags_ & OUTLINE_STYLE_SELECTED) != 0)
			{
				uint3 selection = uint3(float3(0.95, 0.55, 0.05) * 255.0 + 0.5);
				outline.x = selection.r | (selection.g << 8) | (selection.b << 16) | (255u << 24);
				outline_width = 3.0 * (GetSceneConstantBuffer().display_size_.y / float(height));
			}
			else if ((style.flags_ & OUTLINE_STYLE_OUTLINE) != 0)
			{
				outline.x = style.color_;
				outline_width = style.width_;
			}
			if (outline_width > 0.0)
			{
				outline.y = asuint(outline_width);
				outline.z = actor + 1;
			}
		}
		source[dtid.xy] = outline;
	}

	float wave_widest = WaveActiveMax(outline_width);
	if (WaveIsFirstLane() && wave_widest > 0.0)
	{
		RWStructuredBuffer<uint> widest = ResourceDescriptorHeap[canvas_bindings.outline_widest_];
		InterlockedMax(widest[0], asuint(wave_widest));
	}
}
