#include <GraphicsEngine/Shape/Fullscreen/Fullscreen.hlsli>
#include <GraphicsEngine/Shader/Sampler.hlsli>

struct HUDComposeConstantBuffer
{
	float4 output_rect_;
	float fade_;
	float3 background_;
};

struct HUDComposeBindings
{
	uint hud_display_color_;
	uint hud_color_;
	uint hud_constants_;
};
ConstantBuffer<HUDComposeBindings> bindings : register(b0, space1);

float4 main(FullscreenOutput input) : SV_Target0
{
	ConstantBuffer<HUDComposeConstantBuffer> constants = ResourceDescriptorHeap[bindings.hud_constants_];
	float2 texcoord = (input.position.xy - constants.output_rect_.xy) / constants.output_rect_.zw;
	if (any(texcoord < 0.0) || any(texcoord >= 1.0))
	{
		return float4(0.0, 0.0, 0.0, 1.0);
	}

	Texture2D<float4> display_color = ResourceDescriptorHeap[bindings.hud_display_color_];
	Texture2D<float4> hud_color = ResourceDescriptorHeap[bindings.hud_color_];
	float3 display = display_color.SampleLevel(sampler_linear_clamp, texcoord, 0.0).rgb;
	display = any(display > 0.0) ? display : constants.background_;
	float4 hud = hud_color.SampleLevel(sampler_linear_clamp, texcoord, 0.0);
	return float4((hud.rgb + display * (1.0 - hud.a)) * (1.0 - constants.fade_), 1.0);
}
