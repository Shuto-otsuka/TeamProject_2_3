#include <GraphicsEngine/Preview/Preview.hlsli>
#include <GraphicsEngine/Shader/Sampler.hlsli>

/**
* [EN]
* Preview.Draw.Mesh pixel shader, unlit: the material's base color factor
* times its base color texture, cut out below the alpha cutoff, encoded to
* sRGB for the RGBA8 output the panel shows.
*
* ---------------------------------------------------------------------
*
* [JP]
* Preview.Draw.Mesh のピクセルシェーダー（Unlit）。マテリアルのベースカラーの係数に
* ベースカラーのテクスチャを掛け、アルファの切り抜き値を下回る所は切り抜き、パネルが
* 表示する RGBA8 の出力に合わせて sRGB にして返す。
*/
float4 main(PreviewOutput input) : SV_Target0
{
	StructuredBuffer<ModelStructuredBuffer> instances = ResourceDescriptorHeap[mesh_bindings.preview_instances_];
	ModelStructuredBuffer instance = instances[input.instance_index];

	float4 base_color = instance.texture_.base_color_;
	if (instance.texture_.base_color_texture_index_ != MODEL_INVALID)
	{
		Texture2D<float4> base_color_texture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.texture_.base_color_texture_index_)];
		base_color *= base_color_texture.Sample(sampler_linear_wrap, input.texcoord);
	}
	clip(base_color.a - instance.texture_.alpha_cutoff_);

	return float4(pow(saturate(base_color.rgb), 1.0 / 2.2), 1.0);
}
