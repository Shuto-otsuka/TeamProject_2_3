#ifndef __ROOT_SIGNATURE_HLSL__
#define __ROOT_SIGNATURE_HLSL__

#include "Sampler.hlsli"

struct ViewBindings
{
    uint scene_;
    uint frame_;
    uint view_;
    uint sampler_;
};
ConstantBuffer<ViewBindings> view_bindings : register(b1, space1);

struct DispatchBindings
{
    uint constants_;
};
ConstantBuffer<DispatchBindings> dispatch_bindings : register(b2, space1);

#endif // __ROOT_SIGNATURE_HLSL__