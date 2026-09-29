using System;
using SeedCore;

public enum TestState
{
	Idle,
	Walk,
	Run,
}

public class TestScript : SeedScript
{
	[SeedReflectionField]
	public Int32 count_ = 3;

	[SeedReflectionField("移動速度")]
	public Single speed_ = 5.0f;

	[SeedReflectionRange("ジャンプ力", 0, 20)]
	public Single jumpPower_ = 10.0f;

	[SeedReflectionField("重力を使う")]
	public Boolean useGravity_ = true;

	[SeedReflectionFieldCondition(nameof(useGravity_))]
	[SeedReflectionField("重力倍率")]
	public Single gravityScale_ = 1.0f;

	[SeedReflectionFieldCondition("speed_ > 0")]
	[SeedReflectionRange("加速度", 0.0f, 1.0f)]
	public Single acceleration_ = 0.5f;

	[SeedSerializeField]
	public Int32 saveOnly_;

	[SeedReflectionField("バイト")]
	public Byte smallNumber_ = 7;

	[SeedReflectionRange("倍精度", 0.0, 10.0)]
	public Double precise_ = 2.5;

	[SeedReflectionField("名前")]
	public String displayName_ = "テスト";

	[SeedReflectionField("位置")]
	public Vector3 offset_ = new Vector3(1.0f, 2.0f, 3.0f);

	[SeedReflectionField("サイズ")]
	public Vector2 size_ = new Vector2(4.0f, 5.0f);

	[SeedReflectionField("色")]
	public Color tint_ = new Color(1.0f, 0.5f, 0.0f, 1.0f);

	[SeedReflectionField("状態")]
	public TestState state_ = TestState.Walk;

	[SeedPayloadField("テクスチャ", PayloadType.Texture)]
	public UInt32 texture_;

	public Single notReflected_;

	void OnStart()
	{
		Log.Notice("TestScript: OnStart");
	}

	void OnTick(Single elapsedTime)
	{
		Log.Notice($"TestScript: OnTick {elapsedTime}");
	}

	void OnCollisionEnter(Entity other)
	{
		Log.Notice("TestScript: OnCollisionEnter");
	}
}
