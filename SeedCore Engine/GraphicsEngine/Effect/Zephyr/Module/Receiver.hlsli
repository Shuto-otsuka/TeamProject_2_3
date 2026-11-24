// @node Emitter
// @category 受け口
// @receiver
// @input float Rate = 10.0 constant
// @input int MaxParticles = 1000 constant

// @node Spawn
// @category 受け口
// @receiver
// @input shape Shape
// @input float Lifetime = 2.0
// @input float3 Velocity = (0.0, 3.0, 0.0)
// @input color Color = (1.0, 1.0, 1.0, 1.0)
// @input float3 Size = (0.1, 0.1, 0.1)

// @node Simulate
// @category 受け口
// @receiver
// @input force Force multiple
// @input position Position multiple

// @node Render
// @category 受け口
// @receiver
// @input renderer Shape

// @node Billboard
// @category 描画
// @output renderer Shape