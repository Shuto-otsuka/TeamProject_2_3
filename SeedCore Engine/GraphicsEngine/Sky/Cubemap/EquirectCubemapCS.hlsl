#include <GraphicsEngine/Sky/Sky.hlsli>
#include <GraphicsEngine/Shader/RootSignature.hlsli>

struct SkyEquirectBindings
{
	uint sky_cubemap_;
};
ConstantBuffer<SkyEquirectBindings> bindings : register(b0, space1);

/**
* [EN]
* Projects an equirectangular HDR (Texture2D) onto the six faces of the
* environment cube (mip 0). One thread per destination cube texel. The
* panorama is usually far denser than the cube - its width covers the four
* side faces - so each cube texel averages an n x n grid of panorama samples
* spread over its own footprint, n being that density ratio (1 to 8). This box
* filter keeps the cube free of the aliasing a single point sample would bake
* in, which the temporal resolve would otherwise show as shimmer. Without a
* skymap the cube is filled with black.
*
* ---------------------------------------------------------------------
*
* [JP]
* パノラマ HDR（Texture2D）を environment キューブの 6 面（ミップ 0）へ投影
* する。出力キューブテクセル 1 つにつき 1 スレッド。パノラマは普通キューブより
* ずっと細かい（幅が側面 4 面分にあたる）ので、各キューブテクセルは自分の範囲に
* 散らした n x n 点のパノラマのサンプルを平均する。n はその細かさの比（1〜8）。
* この箱フィルタで、1 点だけ読んだときに焼き込まれるエイリアスを防ぐ。
* 焼き込まれると、時間方向の解決でちらつきとして見える。スカイマップが無ければ
* キューブを黒で埋める。
*/
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	RWTexture2DArray<float4> destination = ResourceDescriptorHeap[bindings.sky_cubemap_];
	uint width;
	uint height;
	uint element_count;
	destination.GetDimensions(width, height, element_count);
	if (id.x >= width || id.y >= height)
	{
		return;
	}

	float4 color = float4(0.0, 0.0, 0.0, 1.0);
	if (dispatch_bindings.constants_ != 0xFFFFFFFF)
	{
		Texture2D<float4> source = ResourceDescriptorHeap[dispatch_bindings.constants_];
		uint source_width;
		uint source_height;
		source.GetDimensions(source_width, source_height);
		uint sample_count = clamp((uint)ceil(float(source_width) / float(width * 4)), 1u, 8u);

		float4 sum = float4(0.0, 0.0, 0.0, 0.0);
		for (uint sample_y = 0; sample_y < sample_count; sample_y++)
		{
			for (uint sample_x = 0; sample_x < sample_count; sample_x++)
			{
				float2 texel = float2(id.xy) + (float2(sample_x, sample_y) + 0.5) / float(sample_count);
				float2 uv = texel / float(width) * 2.0 - 1.0;
				sum += source.SampleLevel(sampler_linear_clamp, EquirectangularUv(CubeFaceDirection(id.z, uv)), 0);
			}
		}
		color = sum / float(sample_count * sample_count);
	}
	destination[id] = color;
}
