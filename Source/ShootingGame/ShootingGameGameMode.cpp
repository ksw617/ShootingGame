// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShootingGameGameMode.h"
#include "ShootingGameCharacter.h"
#include "UObject/ConstructorHelpers.h"

AShootingGameGameMode::AShootingGameGameMode()
{																			///Game/Blueprints/BP_ShootingPlayer
	// set default pawn class to our Blueprinted character					//Script/Engine.Blueprint'/Game/Blueprints/BP_ShootingPlayer.BP_ShootingPlayer'
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/Blueprints/BP_ShootingPlayer"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
