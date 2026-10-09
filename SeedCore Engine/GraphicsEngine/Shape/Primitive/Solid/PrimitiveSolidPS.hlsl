#include "PrimitiveSolid.hlsli"
#include "../../../Shader/ShaderResources.hlsli"
#include "../../../Shader/Sampler.hlsli"

/**
* [EN]
* Colors one pixel of a filled shape: the instance color, multiplied by its
* texture when it has one, shaded with a half-Lambert term against a fixed
* light so the faces of an unlit shape still read as a solid. Back faces
* only reach here on double-sided shapes, and use the flipped normal.
*
* ---------------------------------------------------------------------
*
* [JP]
* 面で描く形の1ピクセルの色を決める。インスタンスの色に、テクスチャが
* あればそれを掛け、固定の向きの光でハーフランバートの陰を付けて、照明を
* 受けない形でも面の向きが分かるようにする。裏面がここに来るのは両面の
* 形だけで、そのときは反転した法線を使う。
*/
float4 main(PrimitiveSolidMSOutput input, bool is_front_face : SV_IsFrontFace) : SV_Target0
{
	StructuredBuffer<PrimitiveSolidStructuredBuffer> instances = GetPrimitiveSolidStructuredBuffer(shader_resource_indices.primitive_solid_.instance_index_);
	PrimitiveSolidStructuredBuffer instance = instances[input.instance_index];

	/// [EN] texture_index_ is 0xFFFFFFFF when the shape has no texture, so the color alone is used.
	/// [JP] テクスチャが無い形の texture_index_ は 0xFFFFFFFF なので、そのときは色だけを使う。
	float4 albedo = instance.color_;
	if (instance.texture_index_ != 0xFFFFFFFF)
	{
		Texture2D<float4> albedo_texture = ResourceDescriptorHeap[instance.texture_index_];
		albedo *= albedo_texture.Sample(sampler_linear_wrap, input.texcoord);
	}

	/// [EN] Half-Lambert maps the cosine from [-1, 1] to [0, 1], so faces turned away from the light stay visible instead of going black.
	/// [JP] ハーフランバートは cos を [-1, 1] から [0, 1] へ移すので、光と反対を向いた面も真っ黒にならずに見える。
	float3 normal = normalize(is_front_face ? input.normal : -input.normal);
	float3 light_direction = normalize(float3(0.4, 1.0, 0.3));
	float shade = 0.5 + 0.5 * dot(normal, light_direction);

	return float4(albedo.rgb * shade, albedo.a);
}