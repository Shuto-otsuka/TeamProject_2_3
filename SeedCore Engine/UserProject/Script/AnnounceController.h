#pragma once
#include <FoundationEngine/Prelude.h>
#include <FoundationEngine/SeedScript.h>
#include<SeedCore/ScComponent.h>

class AnnounceController : public SeedCore::SeedScript
{
public:
	void OnStart();
	void OnTick(float elapsedTime);

	enum class Type
	{
		FIRST_PERSON,
		THIRD_PERSON,
		COST_RESET,
		COST_OVER,
		BALL_RESET,
		CHECK_POINT,
		DEATH
	};

	SC_PAYLOAD_FIELD(Texture)
		SeedCore::Uint32 firstPersonImage;
	SC_PAYLOAD_FIELD(Texture)
		SeedCore::Uint32 thirdPersonImage;
	SC_PAYLOAD_FIELD(Texture)
		SeedCore::Uint32 costResetImage;
	SC_PAYLOAD_FIELD(Texture)
		SeedCore::Uint32 costOverImage;
	SC_PAYLOAD_FIELD(Texture)
		SeedCore::Uint32 ballResetImage;
	SC_PAYLOAD_FIELD(Texture)
		SeedCore::Uint32 checkPointImage;
	SC_PAYLOAD_FIELD(Texture)
		SeedCore::Uint32 deathImage;
	SC_REFLECTION_FIELD()
		float announceTime = 1.0f;
	SC_REFLECTION_FIELD()
		float fadeTime = 0.3f;

	void StartAnnounce(Type type);

private:
	float announceTimer = 0.0f;
	bool isAnnounce = false;
};
REGISTER_COMPONENT(AnnounceController);
