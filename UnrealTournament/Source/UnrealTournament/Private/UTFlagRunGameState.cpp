// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTFlagRunGame.h"
#include "UTFlagRunGameState.h"
#include "UTCTFGameMode.h"
#include "UTPowerupSelectorUserWidget.h"
#include "Net/UnrealNetwork.h"
#include "UTCTFScoring.h"
#include "StatNames.h"
#include "UTCountDownMessage.h"
#include "UTAnnouncer.h"
#include "UTCTFMajorMessage.h"
#include "UTRallyPoint.h"

AUTFlagRunGameState::AUTFlagRunGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bRedToCap = false;
	GoldBonusText = NSLOCTEXT("FlagRun", "GoldBonusText", "\u2605 \u2605 \u2605");
	SilverBonusText = NSLOCTEXT("FlagRun", "SilverBonusText", "\u2605 \u2605");
	GoldBonusTimedText = NSLOCTEXT("FlagRun", "GoldBonusTimeText", "\u2605 \u2605 \u2605 {BonusTime}");
	SilverBonusTimedText = NSLOCTEXT("FlagRun", "SilverBonusTimeText", "\u2605 \u2605 {BonusTime}");
	BronzeBonusText = NSLOCTEXT("FlagRun", "BronzeBonusText", "\u2605");
	BonusLevel = 3;
	bUsePrototypePowerupSelect = false;
	bAttackerLivesLimited = false;
	bDefenderLivesLimited = true;
	FlagRunMessageSwitch = 0;
	FlagRunMessageTeam = nullptr;
	bPlayStatusAnnouncements = true;
}

void AUTFlagRunGameState::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld() && GetWorld()->GetAuthGameMode<AUTFlagRunGame>())
	{
		bUsePrototypePowerupSelect = GetWorld()->GetAuthGameMode<AUTFlagRunGame>()->bAllowPrototypePowerups;
		bAllowBoosts = GetWorld()->GetAuthGameMode<AUTFlagRunGame>()->bAllowBoosts;
	}

	UpdateSelectablePowerups();
	AddModeSpecificOverlays();
}

void AUTFlagRunGameState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AUTFlagRunGameState, bRedToCap);
	DOREPLIFETIME(AUTFlagRunGameState, BonusLevel);
	DOREPLIFETIME(AUTFlagRunGameState, CurrentRallyPoint);
	DOREPLIFETIME(AUTFlagRunGameState, FlagRunMessageSwitch);
	DOREPLIFETIME(AUTFlagRunGameState, FlagRunMessageTeam);
	DOREPLIFETIME(AUTFlagRunGameState, bAttackersCanRally);
	DOREPLIFETIME(AUTFlagRunGameState, GoldBonusThreshold);
	DOREPLIFETIME(AUTFlagRunGameState, SilverBonusThreshold);
	DOREPLIFETIME(AUTFlagRunGameState, bAllowBoosts);
	DOREPLIFETIME(AUTFlagRunGameState, bUsePrototypePowerupSelect);
	DOREPLIFETIME(AUTFlagRunGameState, OffenseKillsNeededForPowerup);
	DOREPLIFETIME(AUTFlagRunGameState, DefenseKillsNeededForPowerup);
	DOREPLIFETIME(AUTFlagRunGameState, bIsDefenseAbleToGainPowerup);
	DOREPLIFETIME(AUTFlagRunGameState, bIsOffenseAbleToGainPowerup);
}

void AUTFlagRunGameState::OnBonusLevelChanged()
{
	if (BonusLevel < 3)
	{
		USoundBase* SoundToPlay = UUTCountDownMessage::StaticClass()->GetDefaultObject<UUTCountDownMessage>()->TimeEndingSound;
		if (SoundToPlay != NULL)
		{
			for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
			{
				AUTPlayerController* PC = Cast<AUTPlayerController>(It->PlayerController);
				if (PC && PC->IsLocalPlayerController())
				{
					PC->UTClientPlaySound(SoundToPlay);
				}
			}
		}
	}
}

void AUTFlagRunGameState::UpdateTimeMessage()
{
	// bonus time countdowns
	if (RemainingTime <= GoldBonusThreshold + 7)
	{
		if (RemainingTime > GoldBonusThreshold)
		{
			for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
			{
				AUTPlayerController* PC = Cast<AUTPlayerController>(It->PlayerController);
				if (PC != NULL)
				{
					PC->ClientReceiveLocalizedMessage(UUTCountDownMessage::StaticClass(), 4000 + RemainingTime - GoldBonusThreshold);
				}
			}
		}
		else if ((RemainingTime <= SilverBonusThreshold + 7) && (RemainingTime > SilverBonusThreshold))
		{
			for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
			{
				AUTPlayerController* PC = Cast<AUTPlayerController>(It->PlayerController);
				if (PC != NULL)
				{
					PC->ClientReceiveLocalizedMessage(UUTCountDownMessage::StaticClass(), 3000 + RemainingTime - SilverBonusThreshold);
				}
			}
		}
	}
}

FText AUTFlagRunGameState::GetRoundStatusText(bool bForScoreboard)
{
	if (bForScoreboard)
	{
		FFormatNamedArguments Args;
		Args.Add("RoundNum", FText::AsNumber(CTFRound));
		Args.Add("NumRounds", FText::AsNumber(NumRounds));
		return (NumRounds > 0) ? FText::Format(FullRoundInProgressStatus, Args) : FText::Format(RoundInProgressStatus, Args);
	}
	else
	{
		if (BonusLevel == 3)
		{
			int32 RemainingBonus = FMath::Max(0, RemainingTime - GoldBonusThreshold);
			if (RemainingBonus < 30)
			{
				FFormatNamedArguments Args;
				Args.Add("BonusTime", FText::AsNumber(RemainingBonus));
				return FText::Format(GoldBonusTimedText, Args);
			}
			return GoldBonusText;
		}
		else if (BonusLevel == 2)
		{
			int32 RemainingBonus = FMath::Max(0, RemainingTime - SilverBonusThreshold);
			if (RemainingBonus < 30)
			{
				FFormatNamedArguments Args;
				Args.Add("BonusTime", FText::AsNumber(RemainingBonus));
				return FText::Format(SilverBonusTimedText, Args);
			}
			return SilverBonusText;
		}
		return BronzeBonusText;
	}
}

void AUTFlagRunGameState::UpdateSelectablePowerups()
{
	if (!bAllowBoosts)
	{
		OffenseSelectablePowerups.Empty();
		DefenseSelectablePowerups.Empty();
		return;
	}
	const int32 RedTeamIndex = 0;
	const int32 BlueTeamIndex = 1;
	const bool bIsRedTeamOffense = IsTeamOnDefenseNextRound(RedTeamIndex);

	TSubclassOf<UUTPowerupSelectorUserWidget> OffensePowerupSelectorWidget;
	TSubclassOf<UUTPowerupSelectorUserWidget> DefensePowerupSelectorWidget;

	if (bIsRedTeamOffense)
	{
		OffensePowerupSelectorWidget = LoadClass<UUTPowerupSelectorUserWidget>(NULL, *GetPowerupSelectWidgetPath(RedTeamIndex), NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
		DefensePowerupSelectorWidget = LoadClass<UUTPowerupSelectorUserWidget>(NULL, *GetPowerupSelectWidgetPath(BlueTeamIndex), NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
	}
	else
	{
		OffensePowerupSelectorWidget = LoadClass<UUTPowerupSelectorUserWidget>(NULL, *GetPowerupSelectWidgetPath(BlueTeamIndex), NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
		DefensePowerupSelectorWidget = LoadClass<UUTPowerupSelectorUserWidget>(NULL, *GetPowerupSelectWidgetPath(RedTeamIndex), NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
	}

	if (OffensePowerupSelectorWidget)
	{
		for (TSubclassOf<class AUTInventory> BoostItem : OffensePowerupSelectorWidget.GetDefaultObject()->SelectablePowerups)
		{
			OffenseSelectablePowerups.Add(BoostItem);
		}
	}

	if (DefensePowerupSelectorWidget)
	{
		for (TSubclassOf<class AUTInventory> BoostItem : DefensePowerupSelectorWidget.GetDefaultObject()->SelectablePowerups)
		{
			DefenseSelectablePowerups.Add(BoostItem);
		}
	}
}

FString AUTFlagRunGameState::GetPowerupSelectWidgetPath(int32 TeamNumber)
{
	if (!bAllowBoosts)
	{
		if (IsTeamOnDefenseNextRound(TeamNumber))
		{
			return TEXT("/Game/RestrictedAssets/Blueprints/BP_PowerupSelector_EmptyDefense.BP_PowerupSelector_EmptyDefense_C");
		}
		else
		{
			return TEXT("/Game/RestrictedAssets/Blueprints/BP_PowerupSelector_EmptyOffense.BP_PowerupSelector_EmptyOffense_C");
		}
	}
	else if (bUsePrototypePowerupSelect)
	{
		if (IsTeamOnDefenseNextRound(TeamNumber))
		{
			return TEXT("/Game/RestrictedAssets/Blueprints/BP_PowerupSelector_Defense_Prototype.BP_PowerupSelector_Defense_Prototype_C");
		}
		else
		{
			return TEXT("/Game/RestrictedAssets/Blueprints/BP_PowerupSelector_Offense_Prototype.BP_PowerupSelector_Offense_Prototype_C");
		}
	}
	else
	{
		if (IsTeamOnDefenseNextRound(TeamNumber))
		{
			return TEXT("/Game/RestrictedAssets/Blueprints/BP_PowerupSelector_Defense.BP_PowerupSelector_Defense_C");
		}
		else
		{
			return TEXT("/Game/RestrictedAssets/Blueprints/BP_PowerupSelector_Offense.BP_PowerupSelector_Offense_C");
		}
	}
}

void AUTFlagRunGameState::AddModeSpecificOverlays()
{
	for (TSubclassOf<class AUTInventory> BoostClass : OffenseSelectablePowerups)
	{
		BoostClass.GetDefaultObject()->AddOverlayMaterials(this);
	}

	for (TSubclassOf<class AUTInventory> BoostClass : DefenseSelectablePowerups)
	{
		BoostClass.GetDefaultObject()->AddOverlayMaterials(this);
	}
}

TSubclassOf<class AUTInventory> AUTFlagRunGameState::GetSelectableBoostByIndex(AUTPlayerState* PlayerState, int Index) const
{
	if (PlayerState && IsTeamOnDefenseNextRound(PlayerState->GetTeamNum()))
	{
		if ((DefenseSelectablePowerups.Num() > 0) && (Index < DefenseSelectablePowerups.Num()))
		{
			return DefenseSelectablePowerups[Index];
		}
	}
	else
	{
		if ((OffenseSelectablePowerups.Num() > 0) && (Index < OffenseSelectablePowerups.Num()))
		{
			return OffenseSelectablePowerups[Index];
		}
	}

	return nullptr;
}

bool AUTFlagRunGameState::IsSelectedBoostValid(AUTPlayerState* PlayerState) const
{
	if (PlayerState == nullptr || PlayerState->BoostClass == nullptr)
	{
		return false;
	}

	return IsTeamOnDefenseNextRound(PlayerState->GetTeamNum()) ? DefenseSelectablePowerups.Contains(PlayerState->BoostClass) : OffenseSelectablePowerups.Contains(PlayerState->BoostClass);
}

void AUTFlagRunGameState::PrecacheAllPowerupAnnouncements(class UUTAnnouncer* Announcer) const
{
	for (TSubclassOf<class AUTInventory> PowerupClass : DefenseSelectablePowerups)
	{
		CachePowerupAnnouncement(Announcer, PowerupClass);
	}

	for (TSubclassOf<class AUTInventory> PowerupClass : OffenseSelectablePowerups)
	{
		CachePowerupAnnouncement(Announcer, PowerupClass);
	}
}

void AUTFlagRunGameState::CachePowerupAnnouncement(class UUTAnnouncer* Announcer, const TSubclassOf<AUTInventory> PowerupClass) const
{
	AUTInventory* Powerup = PowerupClass->GetDefaultObject<AUTInventory>();
	if (Powerup)
	{
		Announcer->PrecacheAnnouncement(Powerup->AnnouncementName);
	}
}

bool AUTFlagRunGameState::IsTeamAbleToEarnPowerup(int32 TeamNumber) const
{
	return IsTeamOnOffense(TeamNumber) ? bIsOffenseAbleToGainPowerup : bIsDefenseAbleToGainPowerup;
}

int AUTFlagRunGameState::GetKillsNeededForPowerup(int32 TeamNumber) const
{
	return IsTeamOnOffense(TeamNumber) ? (OffenseKillsNeededForPowerup - OffenseKills) : (DefenseKillsNeededForPowerup - DefenseKills);
}

void AUTFlagRunGameState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (Role == ROLE_Authority)
	{
		uint8 OffensiveTeam = bRedToCap ? 0 : 1;
		if (FlagBases.IsValidIndex(OffensiveTeam) && FlagBases[OffensiveTeam] != nullptr)
		{
			AUTCTFFlag* Flag = Cast<AUTCTFFlag>(FlagBases[OffensiveTeam]->GetCarriedObject());
			bAttackersCanRally = (CurrentRallyPoint != nullptr) && (CurrentRallyPoint->RallyPointState == RallyPointStates::Powered);
			AUTGameVolume* GV = Flag && Flag->HoldingPawn && Flag->HoldingPawn->UTCharacterMovement ? Cast<AUTGameVolume>(Flag->HoldingPawn->UTCharacterMovement->GetPhysicsVolume()) : nullptr;
			bool bInFlagRoom = GV && (GV->bIsNoRallyZone || GV->bIsTeamSafeVolume);
			bHaveEstablishedFlagRunner = (!bInFlagRoom && Flag && Flag->Holder && Flag->HoldingPawn && (GetWorld()->GetTimeSeconds() - Flag->PickedUpTime > 2.f));
		}
	}
}

bool AUTFlagRunGameState::IsTeamOnOffense(int32 TeamNumber) const
{
	const bool bIsOnRedTeam = (TeamNumber == 0);
	return (bRedToCap == bIsOnRedTeam);
}

bool AUTFlagRunGameState::IsTeamOnDefense(int32 TeamNumber) const
{
	return !IsTeamOnOffense(TeamNumber);
}

bool AUTFlagRunGameState::IsTeamOnDefenseNextRound(int32 TeamNumber) const
{
	//We alternate teams, so if we are on offense now, next round we will be on defense
	return IsTeamOnOffense(TeamNumber);
}