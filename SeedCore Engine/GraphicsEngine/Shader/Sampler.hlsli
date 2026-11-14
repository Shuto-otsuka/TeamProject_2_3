#ifndef __SAMPLER_HLSL__
#define __SAMPLER_HLSL__

SamplerState sampler_point_clamp   : register(s0);
SamplerState sampler_point_wrap    : register(s1);
SamplerState sampler_point_mirror  : register(s2);
SamplerState sampler_linear_clamp  : register(s3);
SamplerState sampler_linear_wrap   : register(s4);
SamplerState sampler_linear_mirror : register(s5);
SamplerState sampler_border_black  : register(s6);
SamplerState sampler_border_white  : register(s7);

#endif // __SAMPLER_HLSL__