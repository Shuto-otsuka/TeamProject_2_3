#include <GraphicsEngine/Shape/Fullscreen/Fullscreen.hlsli>
#include <GraphicsEngine/Shader/Sampler.hlsli>

struct SplashConstantBuffer
{
	float2 screen_size_;
	float alpha_;
	float aspect_;

	uint texture_;
	uint3 splash_constant_buffer_padding_0_;
};

struct SplashDrawBindings
{
	uint splash_constants_;
};
ConstantBuffer<SplashDrawBindings> bindings : register(b0, space1);

/**
* [EN]
* Splash.Draw: one splash image (warning, fiction, CRI logo, or the engine
* logo for the time of day), fitted inside the screen with its aspect kept
* and the rest black, faded by alpha_.
*
* ---------------------------------------------------------------------
*
* [JP]
* Splash.Draw。スプラッシュの絵 1 枚（注意・フィクション・CRI のロゴ、または
* 時間帯に合わせたエンジンのロゴ）を、縦横比を保ったまま画面に収め、残りは黒にし、
* alpha_ で薄める。
*/
float4 main(FullscreenOutput input) : SV_Target0
{
	ConstantBuffer<SplashConstantBuffer> constants = ResourceDescriptorHeap[bindings.splash_constants_];
	float screen_aspect = constants.screen_size_.x / constants.screen_size_.y;

	float2 half_size;
	if (constants.aspect_ < screen_aspect)
	{
		half_size.x = 0.5;
		half_size.y = half_size.x * screen_aspect / constants.aspect_;
	}
	else
	{
		half_size.y = 0.5;
		half_size.x = half_size.y * constants.aspect_ / screen_aspect;
	}

	float2 min_uv = float2(0.5, 0.5) - half_size;
	float2 max_uv = float2(0.5, 0.5) + half_size;
	if (any(input.texcoord < min_uv) || any(input.texcoord > max_uv))
	{
		return float4(0.0, 0.0, 0.0, 1.0);
	}

	Texture2D<float4> splash_texture = ResourceDescriptorHeap[constants.texture_];
	float4 color = splash_texture.Sample(sampler_linear_clamp, (input.texcoord - min_uv) / (max_uv - min_uv));
	return float4(color.rgb * color.a * constants.alpha_, 1.0);
}
