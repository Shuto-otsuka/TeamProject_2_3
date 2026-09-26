struct PSInput
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
};

cbuffer LetterConstantBuffer : register(b0)
{
	float2 letter_min_;
	float2 letter_max_;
};

Texture2D letter_texture : register(t0);
SamplerState letter_sampler : register(s0);

float4 main(PSInput input) : SV_Target
{
	float2 pixel = input.position.xy;
	if (any(pixel < letter_min_) || any(pixel > letter_max_))
	{
		return float4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	float2 uv = (pixel - letter_min_) / (letter_max_ - letter_min_);
	return float4(letter_texture.Sample(letter_sampler, uv).rgb, 1.0f);
}
