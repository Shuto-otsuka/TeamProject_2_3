#include <GraphicsEngine/Preview/Preview.hlsli>
#include <GraphicsEngine/Shader/Sampler.hlsli>

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
