#include "UserProject/Script/AnnounceController.h"

void AnnounceController::OnStart()
{
}

void AnnounceController::OnTick(float elapsedTime)
{
	SeedCore::Image* image = GetActor().GetComponent<SeedCore::Image>();
	if (isAnnounce)
	{
		//タイマー経過、フェード
		announceTimer += elapsedTime;
		if (announceTimer > announceTime)
		{
			float fadeRate = (announceTimer - announceTime) / fadeTime;
			image->color_.a = 1.0f - fadeRate;
			if (fadeRate >= 1.0f)
			{
				image->color_.a = 0.0f;
				isAnnounce = false;
			}
		}
	}
}

void AnnounceController::StartAnnounce(Type type)
{
	//アナウンス開始
	isAnnounce = true;
	announceTimer = 0.0f;
	SeedCore::Image* image = GetActor().GetComponent<SeedCore::Image>();
	image->color_.a = 1.0f;

	//指定された画像をセット
	switch (type)
	{
	case AnnounceController::Type::FIRST_PERSON:
		image->textureID_ = firstPersonImage;
		break;
	case AnnounceController::Type::THIRD_PERSON:
		image->textureID_ = thirdPersonImage;
		break;
	case AnnounceController::Type::COST_RESET:
		image->textureID_ = costResetImage;
		break;
	case AnnounceController::Type::COST_OVER:
		image->textureID_ = costOverImage;
		break;
	case AnnounceController::Type::BALL_RESET:
		image->textureID_ = ballResetImage;
		break;
	case AnnounceController::Type::CHECK_POINT:
		image->textureID_ = checkPointImage;
		break;
	case AnnounceController::Type::DEATH:
		image->textureID_ = deathImage;
		break;
	default:
		break;
	}
}
