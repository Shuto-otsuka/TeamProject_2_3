#include "Primitive.hlsli"

float4 main(ColliderLineMSOutput input) : SV_Target0
{
	return input.color;
}
