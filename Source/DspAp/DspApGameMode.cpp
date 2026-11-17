// Copyright Epic Games, Inc. All Rights Reserved.

#include "DspApGameMode.h"
#include "DspApCharacter.h"
#include "UObject/ConstructorHelpers.h"

ADspApGameMode::ADspApGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
