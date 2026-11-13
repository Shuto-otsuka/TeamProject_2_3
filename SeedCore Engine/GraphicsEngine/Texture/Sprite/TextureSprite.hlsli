#ifndef __TEXTURE_SPRITE_HLSL__
#define __TEXTURE_SPRITE_HLSL__

#include <GraphicsEngine/Texture/Texture.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>

struct TextureSpriteBindings
{
	uint texture_units_;
	uint texture_quads_;
	uint texture_draw_list_;
	uint texture_constants_;
	uint outline_styles_;
	uint outline_mask_;
};
ConstantBuffer<TextureSpriteBindings> bindings : register(b0, space1);

/**
* [EN]
* Per-vertex output of TextureSpriteMS/TextureSpriteVS: the texcoord inside the
* quad's UV range and the quad's index into Texture.Quads, from which the
* pixel shader reads the quad and its unit.
*/
struct TextureSpriteOutput
{
	float4 position : SV_Position;
	float2 texcoord : TEXCOORD0;
	nointerpolation uint quad_index : BLENDINDICES0;
};

#endif // __TEXTURE_SPRITE_HLSL__
