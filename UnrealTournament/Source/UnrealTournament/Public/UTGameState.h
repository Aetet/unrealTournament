// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTeamSideSwapDelegate, uint8, Offset);

UCLASS(Config = Game)
class UNREALTOURNAMENT_API AUTGameState : public AGameState
{
	GENERATED_UCLASS_BODY()

	/** server settings */
	UPROPERTY(Replicated, Config, EditAnywhere, BlueprintReadWrite, Category = ServerInfo)
	FString ServerName;
	
	// The message of the day
	UPROPERTY(Replicated, Config, EditAnywhere, BlueprintReadWrite, Category = ServerInfo)
	FString ServerMOTD;

	// A quick string field for the scoreboard and other browsers that contains description of the server
	UPROPERTY(Replicated, Config, EditAnywhere, BlueprintReadWrite, Category = ServerInfo)
	FString ServerDescription;

	/** teams, if the game type has them */
	UPROPERTY(BlueprintReadOnly, Category = GameState)
	TArray<AUTTeamInfo*> Teams;

	/** If TRUE, then we weapon pick ups to stay on their base */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = GameState)
	uint32 bWeaponStay:1;

	/** If TRUE, then the player has to signal ready before they can start */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = GameState)
	uint32 bPlayerMustBeReady:1;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = GameState)
	uint32 bTeamGame : 1;

	/** If true, we will stop the game clock */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = GameState)
	uint32 bStopGameClock : 1;

	/** If a single player's (or team's) score hits this limited, the game is over */
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = GameState)
	uint32 GoalScore;

	/** The maximum amount of time the game will be */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = GameState)
	uint32 TimeLimit;

	/** amount of time after a player spawns where they are immune to damage from enemies */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = GameState)
	float SpawnProtectionTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
	TSubclassOf<UUTLocalMessage> MultiKillMessageClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
	TSubclassOf<UUTLocalMessage> SpreeMessageClass;
	/** amount of time between kills to qualify as a multikill */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
	float MultiKillDelay;

	// Used to sync the time on clients to the server. -- See DefaultTimer()
	UPROPERTY(Replicated)
	uint32 RemainingMinute;

	UPROPERTY(Replicated)
	uint32 bOnlyTheStrongSurvive:1;

	UPROPERTY(Replicated)
	uint32 bViewKillerOnDeath:1;

	/** How much time is remaining in this match. */
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_RemainingTime, BlueprintReadOnly, Category = GameState)
	uint32 RemainingTime;

	/** local world time that game ended (i.e. relative to World->TimeSeconds) */
	UPROPERTY(BlueprintReadOnly, Category = GameState)
	float MatchEndTime;

	UFUNCTION()
	virtual void OnRep_RemainingTime();

	// How long must a player wait before respawning
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = GameState)
	float RespawnWaitTime;

	// How long a player can wait before being forced respawned.  Set to 0 for no delay.
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = GameState)
	float ForceRespawnTime;

	/** offset to level placed team IDs for the purposes of swapping/rotating sides
	 * i.e. if this value is 1 and there are 4 teams, team 0 objects become owned by team 1, team 1 objects become owned by team 2... team 3 objects become owned by team 0
	 */
	UPROPERTY(ReplicatedUsing = OnTeamSideSwap, BlueprintReadOnly, Category = GameState)
	uint8 TeamSwapSidesOffset;
	/** previous value, so we know how much we're changing by */
	UPROPERTY()
	uint8 PrevTeamSwapSidesOffset;

	/** changes team sides; generally offset should be 1 unless it's a 3+ team game and you want to rotate more than one spot */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = GameState)
	virtual void ChangeTeamSides(uint8 Offset = 1);

	UFUNCTION()
	virtual void OnTeamSideSwap();

	UPROPERTY(BlueprintAssignable)
	FTeamSideSwapDelegate TeamSideSwapDelegate;

	UPROPERTY(Replicated, BlueprintReadOnly, ReplicatedUsing = OnWinnerReceived, Category = GameState)
	AUTPlayerState* WinnerPlayerState;

	/** Holds the team of the winning team */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = GameState)
	AUTTeamInfo* WinningTeam;

	UFUNCTION()
	virtual void OnWinnerReceived();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = GameState)
	virtual void SetTimeLimit(uint32 NewTimeLimit);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = GameState)
	virtual void SetGoalScore(uint32 NewGoalScore);

	UFUNCTION()
	virtual void SetWinner(AUTPlayerState* NewWinner);

	/** Called once per second (or so depending on TimeDilation) after RemainingTime() has been replicated */
	virtual void DefaultTimer();

	/** Determines if a player is on the same team */
	UFUNCTION(BlueprintCallable, Category = GameState)
	virtual bool OnSameTeam(const AActor* Actor1, const AActor* Actor2);

	/** Determines if 2 PlayerStates are in score order */
	virtual bool InOrder( class AUTPlayerState* P1, class AUTPlayerState* P2 );

	/** Sorts the Player State Array */
	virtual void SortPRIArray();

	/** Returns true if the match state is InProgress or later */
	UFUNCTION(BlueprintCallable, Category = GameState)
	virtual bool HasMatchStarted() const;

	UFUNCTION(BlueprintCallable, Category = GameState)
	virtual bool IsMatchInProgress() const;

	UFUNCTION(BlueprintCallable, Category = GameState)
	virtual bool IsMatchInOvertime() const;

	virtual void BeginPlay() override;

	/** add an overlay to the OverlayMaterials list */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Effects)
	virtual void AddOverlayMaterial(UMaterialInterface* NewOverlay);
	/** find an overlay in the OverlayMaterials list, return its index */
	int32 FindOverlayMaterial(UMaterialInterface* TestOverlay)
	{
		for (int32 i = 0; i < ARRAY_COUNT(OverlayMaterials); i++)
		{
			if (OverlayMaterials[i] == TestOverlay)
			{
				return i;
			}
		}
		return INDEX_NONE;
	}
	/** get overlay material from index */
	UMaterialInterface* GetOverlayMaterial(int32 Index)
	{
		return (Index >= 0 && Index < ARRAY_COUNT(OverlayMaterials)) ? OverlayMaterials[Index] : NULL;
	}
	/** returns first active overlay material given the passed in flags */
	UMaterialInterface* GetFirstOverlay(uint16 Flags)
	{
		// early out
		if (Flags == 0)
		{
			return NULL;
		}
		else
		{
			for (int32 i = 0; i < ARRAY_COUNT(OverlayMaterials); i++)
			{
				if (Flags & (1 << i))
				{
					return OverlayMaterials[i];
				}
			}
			return NULL;
		}
	}

	/**
	 *	This is called from the UTPlayerCameraManage to allow the game to force an override to the current player camera to make it easier for
	 *  Presentation to be controlled by the server.
	 **/
	
	virtual FName OverrideCameraStyle(APlayerController* PCOwner, FName CurrentCameraStyle);

	// Returns the rules for this server.
	virtual FText ServerRules();

	/** used on clients to know when all TeamInfos have been received */
	UPROPERTY(Replicated)
	uint8 NumTeams;

	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;

	virtual void ReceivedGameModeClass() override;

	virtual FText GetGameStatusText();

protected:
	/** overlay materials, mapped to bits in UTCharacter's OverlayFlags/WeaponOverlayFlags and used to efficiently handle character/weapon overlays
	 * only replicated at startup so set any used materials via BeginPlay()
	 */
	UPROPERTY(Replicated)
	UMaterialInterface* OverlayMaterials[16];

	virtual void HandleMatchHasEnded() override
	{
		MatchEndTime = GetWorld()->TimeSeconds;
	}

public:
	// Will be true if this is an instanced server from a lobby.
	UPROPERTY(Replicated)
	bool bIsInstanceServer;

};



