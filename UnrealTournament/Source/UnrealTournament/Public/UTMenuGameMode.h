// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTPlayerController.h"
#include "UTMenuGameMode.generated.h"

UCLASS(Config = Game)
class UNREALTOURNAMENT_API AUTMenuGameMode : public AGameMode
{
	GENERATED_UCLASS_BODY()

public:
	virtual void RestartGame();
	virtual void BeginGame();
	virtual void GenericPlayerInitialization(AController* C) override;
	virtual void RestartPlayer(AController* aPlayer);
	virtual TSubclassOf<AGameMode> SetGameMode(const FString& MapName, const FString& Options, const FString& Portal); // FIXME: waiting on engine: override
	void Logout( AController* Exiting );
};





