#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>

class CheckPointAreaController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);

	int GetStep() const { return step; }
	bool GetFirstPerson() const { return firstPerson; }

	SC_REFLECTION_FIELD_EX("何個目のチェックポイントか")
		int step = 0;
	SC_REFLECTION_FIELD()
		bool firstPerson = false;
};
REGISTER_COMPONENT(CheckPointAreaController);
