#include <GraphicsEngine/Texture/Billboard/TextureBillboard.hlsli>
#include <GraphicsEngine/Texture/TextureShading.hlsli>
#include <GraphicsEngine/Shader/Sampler.hlsli>
#include <GraphicsEngine/Illumination/VolumetricLight/Froxel.hlsli>
#include <GraphicsEngine/PostProcess/Outline/Outline.hlsli>

/**
* [EN]
* Texture.Billboard pixel shader: the quad's colour from ShadeTexture
* (TextureShading.hlsli), premultiplied for the premultiplied blend into
* Radiance.Color, then passed through the fog in front of it like a
* transparent layer: scaled by the transmittance at its depth in FogVolume,
* plus the light scattered in front of it weighted by its alpha. When the
* unit's actor is outlined, the more opaque half of the quad also records
* itself in Outline.Mask, keeping the nearest fragment per pixel; depth is
* tested before the shader runs, so a hidden billboard records nothing.
*
* ---------------------------------------------------------------------
*
* [JP]
* Texture.Billboard のピクセルシェーダー。四角形の色を ShadeTexture
* （TextureShading.hlsli）で求め、Radiance.Color へのプリマルチプライドの
* ブレンドに合わせてアルファを掛けた色で返す。半透明の層と同じく手前のフォグを
* 通し、FogVolume の自分の深度の透過率を掛けて、手前で散乱した光をアルファの分
* だけ足す。ユニットのアクターが縁取られるときは、四角形のうち不透明寄りの部分が
* Outline.Mask にも自分を記録し、ピクセルごとに一番手前のものが残る。深度は
* シェーダーより先に判定するので、隠れたビルボードは何も記録しない。
*/
[earlydepthstencil]
float4 main(TextureBillboardOutput input) : SV_Target0
{
	StructuredBuffer<TextureQuadStructuredBuffer> quads = ResourceDescriptorHeap[bindings.texture_quads_];
	StructuredBuffer<TextureUnitStructuredBuffer> units = ResourceDescriptorHeap[bindings.texture_units_];
	TextureQuadStructuredBuffer quad = quads[input.quad_index];
	TextureUnitStructuredBuffer unit = units[quad.unit_];
	float4 color = ShadeTexture(unit, quad, input.texcoord);

	StructuredBuffer<OutlineStyleStructuredBuffer> outline_styles = ResourceDescriptorHeap[bindings.outline_styles_];
	if (color.a > 0.5 && unit.actor_ != TEXTURE_INVALID && outline_styles[unit.actor_].flags_ != 0)
	{
		RWTexture2D<uint> outline_mask = ResourceDescriptorHeap[bindings.outline_mask_];
		InterlockedMax(outline_mask[uint2(input.position.xy)], (max(uint(saturate(input.position.z) * 32767.0), 1u) << 17) | unit.actor_);
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	Texture3D<float4> fog_volume = ResourceDescriptorHeap[bindings.raytracing_volumetric_light_fog_volume_];
	uint3 froxel_dimensions;
	fog_volume.GetDimensions(froxel_dimensions.x, froxel_dimensions.y, froxel_dimensions.z);
	float slice = saturate(ViewZToFroxelSlice(input.position.w, scene.near_plane_, scene.far_plane_) - 0.5 / float(froxel_dimensions.z));
	float4 fog = fog_volume.SampleLevel(sampler_linear_clamp, float3(input.position.xy * scene.inverse_screen_size_, slice), 0);
	return float4(color.rgb * fog.a + fog.rgb * color.a, color.a);
}
