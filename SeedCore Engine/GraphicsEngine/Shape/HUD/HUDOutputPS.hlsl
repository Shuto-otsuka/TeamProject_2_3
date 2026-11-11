#include <GraphicsEngine/Shape/Fullscreen/Fullscreen.hlsli>
#include <GraphicsEngine/Shader/Sampler.hlsli>

struct HUDOutputBindings
{
	uint hud_display_color_;
	uint hud_color_;
};
ConstantBuffer<HUDOutputBindings> bindings : register(b0, space1);

float4 main(FullscreenOutput input) : SV_Target0
{
	Texture2D<float4> display_color = ResourceDescriptorHeap[bindings.hud_display_color_];
	Texture2D<float4> hud_color = ResourceDescriptorHeap[bindings.hud_color_];
	float3 display = display_color.SampleLevel(sampler_linear_clamp, input.texcoord, 0.0).rgb;
	float4 hud = hud_color.SampleLevel(sampler_linear_clamp, input.texcoord, 0.0);
	return float4(hud.rgb + display * (1.0 - hud.a), 1.0);
}
