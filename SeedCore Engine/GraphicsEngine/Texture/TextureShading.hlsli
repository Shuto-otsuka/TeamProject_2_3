#ifndef __TEXTURE_SHADING_HLSL__
#define __TEXTURE_SHADING_HLSL__

#include <GraphicsEngine/Texture/Texture.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>

/**
* [EN]
* Colour of a Texture quad at texcoord, premultiplied by its alpha for the
* premultiplied blend; shared by Texture.Sprite and Texture.Billboard. The
* colour depends on the unit's kind.
* - Texture: the image times the unit colour, its UV scrolling over time when
*   TEXTURE_UNIT_SCROLL is set; nearly transparent pixels are discarded.
* - Font: the MTSDF atlas. The median distance gives the fill, widened by
*   outline_width_ for the outline, and the soft distance (alpha) gives the
*   glow; the distance range in screen pixels follows from unit_range_ and the
*   UV's screen derivatives. A shadow quad draws fill and outline in the
*   shadow colour, without glow.
* - Movie: the frame's colour times the unit colour, at the unit's alpha. A
*   fullscreen movie keeps its aspect ratio inside the view and fills the
*   letterbox bars with black.
*/
float4 ShadeTexture(TextureUnitStructuredBuffer unit, TextureQuadStructuredBuffer quad, float2 texcoord)
{
	SceneConstantBuffer scene = GetSceneConstantBuffer();
	Texture2D<float4> source = ResourceDescriptorHeap[unit.texture_index_];

	float4 color = float4(0.0, 0.0, 0.0, 0.0);
	if (unit.kind_ == TEXTURE_KIND_TEXTURE)
	{
		if ((unit.flags_ & TEXTURE_UNIT_SCROLL) != 0)
		{
			texcoord += unit.scroll_direction_ * unit.scroll_speed_ * scene.total_time_;
		}
		color = source.Sample(sampler_linear_wrap, texcoord) * unit.color_;
		clip(color.a - 0.01);
	}
	else if (unit.kind_ == TEXTURE_KIND_FONT)
	{
		bool shadow = (quad.flags_ & TEXTURE_QUAD_SHADOW) != 0;
		float4 fill_color = shadow ? unit.shadow_color_ : unit.color_;
		float4 outline_color = shadow ? unit.shadow_color_ : unit.outline_color_;
		float glow_power = shadow ? 0.0 : unit.glow_power_;

		float4 mtsdf = source.Sample(sampler_linear_clamp, texcoord);
		float2 screen_texel_size = 1.0 / max(fwidth(texcoord), 0.00001);
		float screen_pixel_range = max(0.5 * dot(unit.unit_range_, screen_texel_size), 1.0);
		float median = max(min(mtsdf.r, mtsdf.g), min(max(mtsdf.r, mtsdf.g), mtsdf.b));
		float signed_distance = (median - 0.5) * screen_pixel_range;
		float soft_distance = (mtsdf.a - 0.5) * screen_pixel_range;
		float fill_alpha = saturate(signed_distance + 0.5);
		float outline_alpha = saturate(signed_distance + unit.outline_width_ + 0.5);
		float3 rgb = lerp(outline_color.rgb, fill_color.rgb, fill_alpha);
		float alpha = lerp(outline_alpha * outline_color.a, fill_alpha * fill_color.a, fill_alpha);
		float glow_falloff = saturate(soft_distance / max(screen_pixel_range, 1.0) + 0.5);
		float glow_alpha = pow(glow_falloff, 2.0) * glow_power * unit.glow_color_.a;
		float final_alpha = alpha + glow_alpha * (1.0 - alpha);
		float3 final_rgb = (rgb * alpha + unit.glow_color_.rgb * glow_alpha * (1.0 - alpha)) / max(final_alpha, 0.0001);
		color = float4(final_rgb, final_alpha);
		clip(color.a - 0.01);
	}
	else
	{
		if (unit.display_ == TEXTURE_DISPLAY_FULLSCREEN)
		{
			uint movie_width;
			uint movie_height;
			source.GetDimensions(movie_width, movie_height);
			float movie_aspect = float(movie_width) / max(float(movie_height), 1.0);
			float view_aspect = scene.display_size_.x / max(scene.display_size_.y, 1.0);
			float2 half_size = movie_aspect > view_aspect ? float2(0.5, 0.5 * view_aspect / movie_aspect) : float2(0.5 * movie_aspect / view_aspect, 0.5);
			float2 minimum = 0.5 - half_size;
			float2 maximum = 0.5 + half_size;
			if (any(texcoord < minimum) || any(texcoord > maximum))
			{
				return float4(0.0, 0.0, 0.0, unit.color_.a);
			}
			texcoord = (texcoord - minimum) / (maximum - minimum);
		}
		color = float4(source.Sample(sampler_linear_clamp, texcoord).rgb * unit.color_.rgb, unit.color_.a);
	}
	return float4(color.rgb * color.a, color.a);
}

#endif // __TEXTURE_SHADING_HLSL__
