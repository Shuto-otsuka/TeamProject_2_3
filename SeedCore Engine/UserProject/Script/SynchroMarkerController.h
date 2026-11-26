#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include <SeedCore/ScComponent.h>

class SynchroMarkerController :public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);

	SC_PAYLOAD_FIELD_EX("追従アクター",Actor)
		SeedCore::Uint32 followActorID;

private:
	SeedCore::Actor followActor;
	SeedCore::Vector3 beforeFollowActorPosition;
};
REGISTER_COMPONENT(SynchroMarkerController);
