#include <GraphicsEngine/Shape/Fullscreen/Fullscreen.hlsli>
#include <GraphicsEngine/PostProcess/Outline/Outline.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>

struct OutlineComposeBindings
{
	uint outline_source_;
	uint outline_widest_;
};
ConstantBuffer<OutlineComposeBindings> bindings : register(b0, space1);

float4 main(FullscreenOutput input) : SV_Target0
{
	StructuredBuffer<uint> widest_buffer = ResourceDescriptorHeap[bindings.outline_widest_];
	float widest = asfloat(widest_buffer[0]);
	if (widest <= 0.0)
	{
		discard;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	Texture2D<uint4> source = ResourceDescriptorHeap[bindings.outline_source_];
	uint source_width;
	uint source_height;
	source.GetDimensions(source_width, source_height);
	float2 display_to_source = float2(source_width, source_height) / scene.display_size_;
	int2 source_last = int2(source_width, source_height) - 1;
	int2 display_pixel = int2(input.position.xy);

	uint4 own = source.Load(int3(clamp(int2((float2(display_pixel) + 0.5) * display_to_source), int2(0, 0), source_last), 0));
	bool covered = own.z != 0;
	if (covered && (own.w & OUTLINE_SOURCE_CONTOUR) == 0)
	{
		discard;
	}

	float reach = covered ? asfloat(own.y) * 0.5 : widest;
	int search_radius = (int)ceil(reach);
	bool found = false;
	float nearest_distance = reach;
	uint nearest_color = 0;
	for (int offset_y = -search_radius; offset_y <= search_radius; offset_y++)
	{
		for (int offset_x = -search_radius; offset_x <= search_radius; offset_x++)
		{
			float offset_distance = length(float2(offset_x, offset_y));
			if (offset_distance > reach || (found && offset_distance >= nearest_distance))
			{
				continue;
			}
			uint4 candidate = source.Load(int3(clamp(int2((float2(display_pixel + int2(offset_x, offset_y)) + 0.5) * display_to_source), int2(0, 0), source_last), 0));
			bool hit = covered ? candidate.z == own.z && (candidate.w & OUTLINE_SOURCE_EDGE) != 0 : candidate.z != 0 && offset_distance <= asfloat(candidate.y);
			if (hit)
			{
				found = true;
				nearest_distance = offset_distance;
				nearest_color = candidate.x;
			}
		}
	}
	if (!found)
	{
		discard;
	}
	return float4((nearest_color >> uint4(0, 8, 16, 24)) & 0xFF) / 255.0;
}
