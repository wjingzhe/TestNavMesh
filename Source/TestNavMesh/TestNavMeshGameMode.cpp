// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "TestNavMeshGameMode.h"
#include "TestNavMeshPlayerController.h"
#include "TestNavMeshCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATestNavMeshGameMode::ATestNavMeshGameMode()
{
	// use our custom PlayerController class
	PlayerControllerClass = ATestNavMeshPlayerController::StaticClass();

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/TopDownCPP/Blueprints/TopDownCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}