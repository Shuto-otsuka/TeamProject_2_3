#include "PrimitiveWireframe.hlsli"

float4 main(PrimitiveWireframeMSOutput input) : SV_Target0
{
	return input.color;
}
