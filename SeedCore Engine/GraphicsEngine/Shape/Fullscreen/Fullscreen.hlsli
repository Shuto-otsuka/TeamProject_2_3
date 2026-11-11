#ifndef __FULLSCREEN_HLSL__
#define __FULLSCREEN_HLSL__

struct FullscreenOutput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
};

#endif // __FULLSCREEN_HLSL__