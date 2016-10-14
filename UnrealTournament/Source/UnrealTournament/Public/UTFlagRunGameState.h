// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTCTFRoundGameState.h"
#include "UTFlagRunGameState.generated.h"

UCLASS()
class UNREALTOURNAMENT_API AUTFlagRunGameState : public AUTCTFRoundGameState
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Replicated)
		uint32 bRedToCap : 1;

	UPROPERTY(Replicated)
		int32 GoldBonusThreshold;

	UPROPERTY(Replicated)
		int32 SilverBonusThreshold;

	UPROPERTY()
		FText GoldBonusText;

	UPROPERTY()
		FText SilverBonusText;

	UPROPERTY()
		FText BronzeBonusText;

	UPROPERTY()
		FText GoldBonusTimedText;

	UPROPERTY()
		FText SilverBonusTimedText;

	UPROPERTY(Replicated)
		bool bAttackersCanRally;

	UPROPERTY()
		bool bHaveEstablishedFlagRunner;

	UPROPERTY(ReplicatedUsing = OnBonusLevelChanged)
		uint8 BonusLevel;

	UPROPERTY(Replicated)
		int32 OffenseKillsNeededForPowerup;

	UPROPERTY(Replicated)
		int32 DefenseKillsNeededForPowerup;

	UPROPERTY(Replicated)
		bool bIsDefenseAbleToGainPowerup;

	UPROPERTY(Replicated)
		bool bIsOffenseAbleToGainPowerup;

	UPROPERTY(Replicated)
		bool bUsePrototypePowerupSelect;

	UPROPERTY(Replicated)
		bool bAllowBoosts;

	UPROPERTY(Replicated)
		int32 FlagRunMessageSwitch;

	UPROPERTY(Replicated)
		class AUTTeamInfo* FlagRunMessageTeam;


	UPROPERTY(BlueprintReadOnly, Replicated)
		class AUTRallyPoint* CurrentRallyPoint;

	UFUNCTION()
		void OnBonusLevelChanged();

	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

	virtual void UpdateSelectablePowerups();

	virtual bool IsTeamOnOffense(int32 TeamNumber) const override;
	virtual bool IsTeamOnDefense(int32 TeamNumber) const override;

	UFUNCTION(BlueprintCallable, Category = Team)
		virtual bool IsTeamOnDefenseNextRound(int32 TeamNumber) const;

	UFUNCTION(BlueprintCallable, Category = GameState)
		FString GetPowerupSelectWidgetPath(int32 TeamNumber);

	UFUNCTION(BlueprintCallable, Category = GameState)
		virtual TSubclassOf<class AUTInventory> GetSelectableBoostByIndex(AUTPlayerState* PlayerState, int Index) const override;

	UFUNCTION(BlueprintCallable, Category = GameState)
		virtual bool IsSelectedBoostValid(AUTPlayerState* PlayerState) const override;

	UFUNCTION(BlueprintCallable, Category = Team)
		virtual int GetKillsNeededForPowerup(int32 TeamNumber) const;

	UFUNCTION(BlueprintCallable, Category = Team)
		virtual bool IsTeamAbleToEarnPowerup(int32 TeamNumber) const;

	//Handles precaching all game announcement sounds for the local player
	virtual void PrecacheAllPowerupAnnouncements(class UUTAnnouncer* Announcer) const;

	virtual FText GetRoundStatusText(bool bForScoreboard) override;

protected:
	virtual void UpdateTimeMessage() override;

	virtual void CachePowerupAnnouncement(class UUTAnnouncer* Announcer, TSubclassOf<AUTInventory> PowerupClass) const;

	virtual void AddModeSpecificOverlays();

	UPROPERTY()
		TArray<TSubclassOf<class AUTInventory>> DefenseSelectablePowerups;

	UPROPERTY()
		TArray<TSubclassOf<class AUTInventory>> OffenseSelectablePowerups;
};
