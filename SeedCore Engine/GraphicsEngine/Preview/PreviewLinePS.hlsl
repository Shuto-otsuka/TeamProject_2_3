#include <GraphicsEngine/Preview/Preview.hlsli>

float4 main(PreviewLineOutput input) : SV_Target0
{
	return input.color;
}
