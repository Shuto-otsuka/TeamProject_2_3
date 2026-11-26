#include <GraphicsEngine/Shader/Scene.hlsli>
#include <GraphicsEngine/PostProcess/ToneMapping/ToneMapping.hlsli>

struct ToneMappingConstantBuffer
{
	uint enabled_;
	uint mode_;
	float2 tone_mapping_padding_0_;
};

struct ToneMappingResolveBindings
{
	uint color_grading_color_;
	uint tone_mapping_constants_;
	uint tone_mapping_color_;
};
ConstantBuffer<ToneMappingResolveBindings> bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<float4> output = ResourceDescriptorHeap[bindings.tone_mapping_color_];

	uint width, height;
	output.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	float2 uv = (float2(dtid.xy) + 0.5) / float2(width, height);

	ConstantBuffer<ToneMappingConstantBuffer> constants = ResourceDescriptorHeap[bindings.tone_mapping_constants_];
	Texture2D<float4> source = ResourceDescriptorHeap[bindings.color_grading_color_];
	float3 color = source.Load(int3(dtid.xy, 0)).rgb;

	if (constants.enabled_ != 0)
	{
		uint mode = constants.mode_;
		if (mode == 1)
		{
			color = ReinhardToneMap(color);
		}
		else if (mode == 2)
		{
			color = AcesFilmicToneMap(color);
		}
		else if (mode == 3)
		{
			color = PbrNeutralToneMap(color);
		}
	}

	color = saturate(color);
	color = LinearToSrgb(color);

	output[dtid.xy] = float4(color, 1.0);
}
