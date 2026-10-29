#pragma once
#include <SeedCore/ScScript.h>
#include <SeedCore/ScMath.h>

class SwitchSystem :public SeedCore::SeedScript
{
public:
	enum class Mode
	{
		Momentary,
		Toggle,
		Latch,
	};

	enum class Shape
	{
		Sphere,
		Box,
	};

public:
	SC_REFLECTION_FIELD_EX("動作")
	Mode mode_ = Mode::Momentary;

	SC_REFLECTION_FIELD_EX("判定の形")
	Shape shape_ = Shape::Sphere;

	SC_REFLECTION_FIELD_EX("判定サイズ")
	SeedCore::Vector3 detectSize_ = { 1.0f,1.0f,1.0f };

	SC_REFLECTION_FIELD_EX("判定オフセット")
	SeedCore::Vector3 detectOffset_ = { 0.0f,0.0f,0.0f };

	SC_REFLECTION_FIELD_EX("対象タグ")
	SeedCore::DynamicArray<SeedCore::String> targetTags_;

	SC_REFLECTION_CLAMPED_EX("クールタイム", 0.0f, 10000.0f)
	float cooltime_ = 1.0f;

	SC_REFLECTION_FIELD_EX("判定範囲を描画するか")
	bool isDebugAreaDraw_ = true;

public:
	void OnStart();

	void OnTick(float elapsedTime);

	void OnEditorTick(float elapsedTime);

public:
	bool IsOn()const;

private:
	bool isOn_ = false;

	bool wasInside_ = false;

	float cooltimer_ = 0.0f;

	SeedCore::Uint32 offMaterialID_ = 0;

	SeedCore::Uint32 onMaterialID_ = 0;
};
REGISTER_COMPONENT(SwitchSystem);