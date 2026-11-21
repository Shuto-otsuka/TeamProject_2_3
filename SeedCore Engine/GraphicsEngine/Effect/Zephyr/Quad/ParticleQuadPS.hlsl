#include <GraphicsEngine/Effect/Zephyr/Quad/ParticleQuad.hlsli>

float4 main(ParticleQuadOutput input) : SV_Target0
{
	return input.color;
}
