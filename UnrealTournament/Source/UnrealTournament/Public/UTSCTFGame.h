// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "UTCTFRoundGame.h"
#include "UTSCTFGameState.h"
#include "UTCTFScoring.h"
#include "UTSCTFGame.generated.h"

class AUTSCTFFlagBase;
class AUTSCTFGameState;

class AUTSCTFFlagBase;

UCLASS()
class UNREALTOURNAMENT_API AUTSCTFGame : public AUTCTFRoundGame
{
	GENERATED_UCLASS_BODY()
	
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;


	virtual void FlagTeamChanged(uint8 NewTeamIndex);
	virtual void ScoreKill_Implementation(AController* Killer, AController* Other, APawn* KilledPawn, TSubclassOf<UDamageType> DamageType) override;

	virtual bool IsTeamStillAlive(uint8 TeamNum);
	bool CanFlagTeamSwap(uint8 NewTeamNum);

	virtual void BroadcastVictoryConditions();

	virtual void InitRound();

	virtual int32 GetFlagCapScore() override
	{
		return 1;
	}


protected:

	UPROPERTY()
	AUTSCTFGameState* SCTFGameState;

	// How long does a flag have to sit idle for it to return to the neutral position.  Use ?FlagSwapTime=x to set.  Set to 0 to be instantly pick-up-able 
	UPROPERTY()
	int32 FlagSwapTime;

	// How long in to the match should the flag by spawned.  Use ?FlagSpawnDelay=x to set.
	UPROPERTY()
	int32 FlagSpawnDelay;

	void SpawnInitalFlag();
};