// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTTeamGameMode.h"
#include "UTHUD_CTF.h"
#include "UTFlagRunGame.h"
#include "UTCTFGameMessage.h"
#include "UTCTFRoleMessage.h"
#include "UTCTFRewardMessage.h"
#include "UTCTFMajorMessage.h"
#include "UTFirstBloodMessage.h"
#include "UTCountDownMessage.h"
#include "UTPickup.h"
#include "UTGameMessage.h"
#include "UTMutator.h"
#include "UTCTFSquadAI.h"
#include "UTWorldSettings.h"
#include "Widgets/SUTTabWidget.h"
#include "Dialogs/SUTPlayerInfoDialog.h"
#include "StatNames.h"
#include "Engine/DemoNetDriver.h"
#include "UTCTFScoreboard.h"
#include "UTShowdownGameMessage.h"
#include "UTShowdownRewardMessage.h"
#include "UTPlayerStart.h"
#include "UTArmor.h"
#include "UTTimedPowerup.h"
#include "UTPlayerState.h"
#include "UTFlagRunHUD.h"
#include "UTGhostFlag.h"
#include "UTFlagRunGameState.h"
#include "UTAsymCTFSquadAI.h"
#include "UTWeaponRedirector.h"
#include "UTWeaponLocker.h"
#include "UTFlagRunMessage.h"
#include "UTWeap_Translocator.h"
#include "UTReplicatedEmitter.h"
#include "UTATypes.h"
#include "UTGameVolume.h"
#include "UTTaunt.h"
#include "Animation/AnimInstance.h"
#include "UTFlagRunGameMessage.h"
#include "UTAnalytics.h"
#include "UTRallyPoint.h"

AUTFlagRunGame::AUTFlagRunGame(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GoldBonusTime = 120;
	SilverBonusTime = 60;
	GoldScore = 3;
	SilverScore = 2;
	BronzeScore = 1;
	DefenseScore = 1;
	DisplayName = NSLOCTEXT("UTGameMode", "FLAGRUN", "Flag Run");
	bHideInUI = false;
	bWeaponStayActive = false;
	bAllowPickupAnnouncements = true;
	LastEntryDefenseWarningTime = 0.f;
	MapPrefix = TEXT("FR");
	GameStateClass = AUTFlagRunGameState::StaticClass();
	bAllowBoosts = false;
	OffenseKillsNeededForPowerUp = 10;
	DefenseKillsNeededForPowerUp = 10;
	bCarryOwnFlag = true;
	bNoFlagReturn = true;
	bGameHasImpactHammer = false;
	FlagPickupDelay = 15;

	ActivatedPowerupPlaceholderObject = FStringAssetReference(TEXT("/Game/RestrictedAssets/Pickups/Powerups/BP_ActivatedPowerup_UDamage.BP_ActivatedPowerup_UDamage_C"));
	RepulsorObject = FStringAssetReference(TEXT("/Game/RestrictedAssets/Pickups/Powerups/BP_Repulsor.BP_Repulsor_C"));

	static ConstructorHelpers::FObjectFinder<UClass> AfterImageFinder(TEXT("Blueprint'/Game/RestrictedAssets/Weapons/Translocator/TransAfterImage.TransAfterImage_C'"));
	AfterImageType = AfterImageFinder.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase> RallyFinalSoundFinder(TEXT("SoundWave'/Game/RestrictedAssets/Audio/Stingers/RallyFailed.RallyFailed'"));
	RallyFailedSound = RallyFinalSoundFinder.Object;
}

void AUTFlagRunGame::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	if (!ActivatedPowerupPlaceholderObject.IsNull())
	{
		ActivatedPowerupPlaceholderClass = Cast<UClass>(StaticLoadObject(UClass::StaticClass(), NULL, *ActivatedPowerupPlaceholderObject.ToStringReference().ToString(), NULL, LOAD_NoWarn));
	}
	if (!RepulsorObject.IsNull())
	{
		RepulsorClass = Cast<UClass>(StaticLoadObject(UClass::StaticClass(), NULL, *RepulsorObject.ToStringReference().ToString(), NULL, LOAD_NoWarn));
	}

	OffenseKillsNeededForPowerUp = FMath::Max(0, UGameplayStatics::GetIntOption(Options, TEXT("OffKillsForPowerup"), OffenseKillsNeededForPowerUp));
	DefenseKillsNeededForPowerUp = FMath::Max(0, UGameplayStatics::GetIntOption(Options, TEXT("DefKillsForPowerup"), DefenseKillsNeededForPowerUp));

	FString InOpt = UGameplayStatics::ParseOption(Options, TEXT("AllowPrototypePowerups"));
	bAllowPrototypePowerups = EvalBoolOptions(InOpt, bAllowPrototypePowerups);

	InOpt = UGameplayStatics::ParseOption(Options, TEXT("Boost"));
	bAllowBoosts = EvalBoolOptions(InOpt, bAllowBoosts);
	if (!bAllowBoosts)
	{
		OffenseKillsNeededForPowerUp = 1000;
		DefenseKillsNeededForPowerUp = 1000;
	}

	InOpt = UGameplayStatics::ParseOption(Options, TEXT("FixedRally"));
	bFixedRally = EvalBoolOptions(InOpt, bFixedRally);

	GameSession->MaxPlayers = 10;
}

int32 AUTFlagRunGame::GetFlagCapScore()
{
	int32 BonusTime = UTGameState->GetRemainingTime();
	if (BonusTime >= GoldBonusTime)
	{
		return GoldScore;
	}
	if (BonusTime >= SilverBonusTime)
	{
		return SilverScore;
	}
	return BronzeScore;
}

void AUTFlagRunGame::AnnounceWin(AUTTeamInfo* WinningTeam, uint8 Reason)
{
	if (Reason == 0)
	{
		int32 BonusType = 100 + BronzeScore;
		if (WinningTeam->RoundBonus >= GoldBonusTime)
		{
			BonusType = 300 + GoldScore;
		}
		else if (WinningTeam->RoundBonus >= SilverBonusTime)
		{
			BonusType = 200 + SilverScore;
		}
		BroadcastLocalized(this, UUTCTFRewardMessage::StaticClass(), BonusType, nullptr, nullptr, WinningTeam);
	}
	else
	{
		BroadcastLocalized(this, UUTCTFRewardMessage::StaticClass(), 400, nullptr, nullptr, WinningTeam);
	}
	BroadcastLocalized(NULL, UUTShowdownGameMessage::StaticClass(), 3 + WinningTeam->TeamIndex);
}

void AUTFlagRunGame::BroadcastCTFScore(APlayerState* ScoringPlayer, AUTTeamInfo* ScoringTeam, int32 OldScore)
{
	int32 BonusType = 100 + BronzeScore;
	if (ScoringTeam->RoundBonus >= GoldBonusTime)
	{
		BonusType = 300 + GoldScore;
	}
	else if (ScoringTeam->RoundBonus >= SilverBonusTime)
	{
		BonusType = 200 + SilverScore;
	}

	BroadcastLocalized(this, UUTCTFRewardMessage::StaticClass(), BonusType, ScoringPlayer, NULL, ScoringTeam);
	BroadcastLocalized(this, UUTCTFGameMessage::StaticClass(), 2, ScoringPlayer, NULL, ScoringTeam);
}

void AUTFlagRunGame::InitGameStateForRound()
{
	Super::InitGameStateForRound();
	AUTFlagRunGameState* FRGS = Cast<AUTFlagRunGameState>(CTFGameState);
	if (FRGS)
	{
		FRGS->OffenseKills = 0;
		FRGS->DefenseKills = 0;
		FRGS->OffenseKillsNeededForPowerup = OffenseKillsNeededForPowerUp;
		FRGS->DefenseKillsNeededForPowerup = DefenseKillsNeededForPowerUp;
		FRGS->bIsOffenseAbleToGainPowerup = true;
		FRGS->bIsDefenseAbleToGainPowerup = true;
		FRGS->bRedToCap = !FRGS->bRedToCap;
	}
}


bool AUTFlagRunGame::AvoidPlayerStart(AUTPlayerStart* P)
{
	return P && P->bIgnoreInASymCTF;
}

int32 AUTFlagRunGame::GetDefenseScore()
{
	return DefenseScore;
}

void AUTFlagRunGame::CheckRoundTimeVictory()
{
	AUTFlagRunGameState* FRGS = Cast<AUTFlagRunGameState>(CTFGameState);
	int32 RemainingTime = UTGameState ? UTGameState->GetRemainingTime() : 100;
	if (RemainingTime <= 0)
	{
		// Round is over, defense wins.
		ScoreAlternateWin((FRGS && FRGS->bRedToCap) ? 1 : 0, 1);
	}
	else
	{
		if (FRGS)
		{
			uint8 OldBonusLevel = FRGS->BonusLevel;
			FRGS->BonusLevel = (RemainingTime >= GoldBonusTime) ? 3 : 2;
			if (RemainingTime < SilverBonusTime)
			{
				FRGS->BonusLevel = 1;
			}
			if (OldBonusLevel != FRGS->BonusLevel)
			{
				FRGS->OnBonusLevelChanged();
				FRGS->ForceNetUpdate();
			}
		}
	}
}

void AUTFlagRunGame::InitGameState()
{
	Super::InitGameState();

	AUTFlagRunGameState* RCTFGameState = Cast<AUTFlagRunGameState>(CTFGameState);
	if (RCTFGameState)
	{
		RCTFGameState->GoldBonusThreshold = GoldBonusTime;
		RCTFGameState->SilverBonusThreshold = SilverBonusTime;
	}
}

void AUTFlagRunGame::InitDelayedFlag(AUTCarriedObject* Flag)
{
	Super::InitDelayedFlag(Flag);
	if (Flag && IsTeamOnOffense(Flag->GetTeamNum()))
	{
		Flag->SetActorHiddenInGame(true);
		FFlagTrailPos NewPosition;
		NewPosition.Location = Flag->GetHomeLocation();
		NewPosition.MidPoints[0] = FVector(0.f);
		Flag->PutGhostFlagAt(NewPosition);
	}
}

void AUTFlagRunGame::InitFlagForRound(AUTCarriedObject* Flag)
{
	if (Flag != nullptr)
	{
		Flag->AutoReturnTime = 8.f;
		Flag->bGradualAutoReturn = true;
		Flag->bDisplayHolderTrail = true;
		Flag->bShouldPingFlag = true;
		Flag->bSlowsMovement = bSlowFlagCarrier;
		Flag->ClearGhostFlags();
		Flag->bSendHomeOnScore = false;
		if (IsTeamOnOffense(Flag->GetTeamNum()))
		{
			Flag->MessageClass = UUTFlagRunGameMessage::StaticClass();
			Flag->SetActorHiddenInGame(false);
			Flag->bEnemyCanPickup = false;
			Flag->bFriendlyCanPickup = true;
			Flag->bTeamPickupSendsHome = false;
			Flag->bEnemyPickupSendsHome = false;
			Flag->bWaitingForFirstPickup = true;
			GetWorldTimerManager().SetTimer(Flag->NeedFlagAnnouncementTimer, Flag, &AUTCarriedObject::SendNeedFlagAnnouncement, 5.f, false);
		}
		else
		{
			Flag->Destroy();
		}
	}
}

void AUTFlagRunGame::NotifyFirstPickup(AUTCarriedObject* Flag)
{
	if (Flag && Flag->HoldingPawn && StartingArmorClass)
	{
		if (!StartingArmorClass.GetDefaultObject()->HandleGivenTo(Flag->HoldingPawn))
		{
			Flag->HoldingPawn->AddInventory(GetWorld()->SpawnActor<AUTInventory>(StartingArmorClass, FVector(0.0f), FRotator(0.f, 0.f, 0.f)), true);
		}
	}
}

void AUTFlagRunGame::IntermissionSwapSides()
{
	// swap sides, if desired
	AUTWorldSettings* Settings = Cast<AUTWorldSettings>(GetWorld()->GetWorldSettings());
	if (Settings != NULL && Settings->bAllowSideSwitching)
	{
		CTFGameState->ChangeTeamSides(1);
	}
	else
	{
		// force update of flags since defender flag gets destroyed
		for (AUTCTFFlagBase* Base : CTFGameState->FlagBases)
		{
			IUTTeamInterface* TeamObj = Cast<IUTTeamInterface>(Base);
			if (TeamObj != NULL)
			{
				TeamObj->Execute_SetTeamForSideSwap(Base, Base->TeamNum);
			}
		}
	}
}

void AUTFlagRunGame::InitFlags()
{
	Super::InitFlags();
	for (AUTCTFFlagBase* Base : CTFGameState->FlagBases)
	{
		if (Base)
		{
			Base->Capsule->SetCapsuleSize(160.f, 134.0f);
		}
	}
}

int32 AUTFlagRunGame::PickCheatWinTeam()
{
	AUTFlagRunGameState* FRGS = Cast<AUTFlagRunGameState>(CTFGameState);
	return (FRGS && FRGS->bRedToCap) ? 0 : 1;
}

bool AUTFlagRunGame::CheckForWinner(AUTTeamInfo* ScoringTeam)
{
	if (Super::CheckForWinner(ScoringTeam))
	{
		return true;
	}

	// Check if a team has an insurmountable lead
	// current implementation assumes 6 rounds and 2 teams
	if (CTFGameState && (CTFGameState->CTFRound >= NumRounds - 2) && Teams[0] && Teams[1])
	{
		AUTFlagRunGameState* GS = Cast<AUTFlagRunGameState>(CTFGameState);
		bSecondaryWin = false;
		if ((CTFGameState->CTFRound == NumRounds - 2) && (FMath::Abs(Teams[0]->Score - Teams[1]->Score) > DefenseScore + GoldScore))
		{
			AUTTeamInfo* BestTeam = (Teams[0]->Score > Teams[1]->Score) ? Teams[0] : Teams[1];
			EndTeamGame(BestTeam, FName(TEXT("scorelimit")));
			return true;
		}
		if (CTFGameState->CTFRound == NumRounds - 1)
		{
			if (GS && GS->bRedToCap)
			{
				// next round is blue cap
				if ((Teams[0]->Score > Teams[1]->Score + GoldScore) || ((GS->TiebreakValue > 60) && (Teams[0]->Score == Teams[1]->Score + GoldScore)))
				{
					EndTeamGame(Teams[0], FName(TEXT("scorelimit")));
					return true;
				}
				if ((Teams[1]->Score > Teams[0]->Score + DefenseScore) || ((Teams[1]->Score == Teams[0]->Score + DefenseScore) && (GS->TiebreakValue < -60)))
				{
					EndTeamGame(Teams[1], FName(TEXT("scorelimit")));
					return true;
				}
			}
			else
			{
				// next round is red cap
				if ((Teams[1]->Score > Teams[0]->Score + GoldScore) || ((GS->TiebreakValue < -60) && (Teams[1]->Score == Teams[0]->Score + GoldScore)))
				{
					EndTeamGame(Teams[1], FName(TEXT("scorelimit")));
					return true;
				}
				if ((Teams[0]->Score > Teams[1]->Score + DefenseScore) || ((Teams[0]->Score == Teams[1]->Score + DefenseScore) && (GS->TiebreakValue > 60)))
				{
					EndTeamGame(Teams[0], FName(TEXT("scorelimit")));
					return true;
				}
			}
		}
	}
	return false;
}

void AUTFlagRunGame::DefaultTimer()
{
	Super::DefaultTimer();

	if (UTGameState && UTGameState->IsMatchInProgress() && !UTGameState->IsMatchIntermission())
	{
		AUTCTFRoundGameState* RCTFGameState = Cast<AUTCTFRoundGameState>(CTFGameState);
		if (RCTFGameState && (RCTFGameState->RemainingPickupDelay <= 0) && (GetWorld()->GetTimeSeconds() - LastEntryDefenseWarningTime > 15.f))
		{
			// check for uncovered routes - support up to 5 entries for now
			AUTGameVolume* EntryRoutes[MAXENTRYROUTES];
			for (int32 i = 0; i < MAXENTRYROUTES; i++)
			{
				EntryRoutes[i] = nullptr;
			}
			// mark routes that need to be covered
			for (TActorIterator<AUTGameVolume> It(GetWorld()); It; ++It)
			{
				AUTGameVolume* GV = *It;
				if ((GV->RouteID > 0) && (GV->RouteID < MAXENTRYROUTES) && GV->bReportDefenseStatus && (GV->VoiceLinesSet != NAME_None))
				{
					EntryRoutes[GV->RouteID] = GV;
				}
			}
			// figure out where defenders are
			bool bFoundInnerDefender = false;
			AUTPlayerState* Speaker = nullptr;
			FString Why = "";
			for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
			{
				AUTCharacter* UTChar = Cast<AUTCharacter>((*Iterator)->GetPawn());
				AUTPlayerState* UTPS = Cast<AUTPlayerState>((*Iterator)->PlayerState);
				if (UTChar && UTChar->LastGameVolume && UTPS && UTPS->Team && IsTeamOnDefense(UTPS->Team->TeamIndex))
				{
					Speaker = UTPS;
					int32 CoveredRoute = UTChar->LastGameVolume->RouteID;
					if ((CoveredRoute == 0) || UTChar->LastGameVolume->bIsTeamSafeVolume)
					{
						bFoundInnerDefender = true;
						break;
					}
					else if ((CoveredRoute > 0) && (CoveredRoute < MAXENTRYROUTES))
					{
						EntryRoutes[CoveredRoute] = nullptr;
					}
					else
					{
						//						UE_LOG(UT, Warning, TEXT("Not in defensive position %s %s routeid %d"), *UTChar->LastGameVolume->GetName(), *UTChar->LastGameVolume->VolumeName.ToString(), UTChar->LastGameVolume->RouteID);
					}
					//Why = Why + FString::Printf(TEXT("%s in position %s routeid %d, "), *UTPS->PlayerName, *UTChar->LastGameVolume->VolumeName.ToString(), UTChar->LastGameVolume->RouteID);
				}
			}
			if (!bFoundInnerDefender && Speaker)
			{
				// warn about any uncovered entries
				for (int32 i = 0; i < MAXENTRYROUTES; i++)
				{
					if (EntryRoutes[i] && (EntryRoutes[i]->VoiceLinesSet != NAME_None))
					{
						LastEntryDefenseWarningTime = GetWorld()->GetTimeSeconds();
						/*
						if (Cast<AUTPlayerController>(Speaker->GetOwner()))
						{
							Cast<AUTPlayerController>(Speaker->GetOwner())->TeamSay(Why);
						}
						else if (Cast<AUTBot>(Speaker->GetOwner()))
						{
							Cast<AUTBot>(Speaker->GetOwner())->Say(Why, true);
						}
						*/
						Speaker->AnnounceStatus(EntryRoutes[i]->VoiceLinesSet, 3);
					}
				}
			}
		}
	}
}

float AUTFlagRunGame::OverrideRespawnTime(TSubclassOf<AUTInventory> InventoryType)
{
	if (InventoryType == nullptr)
	{
		return 0.f;
	}
	AUTWeapon* WeaponDefault = Cast<AUTWeapon>(InventoryType.GetDefaultObject());
	return (WeaponDefault && !WeaponDefault->bMustBeHolstered) ? 20.f : InventoryType.GetDefaultObject()->RespawnTime;
}

int32 AUTFlagRunGame::GetComSwitch(FName CommandTag, AActor* ContextActor, AUTPlayerController* InInstigator, UWorld* World)
{
	if (World == nullptr) return INDEX_NONE;

	AUTFlagRunGameState* GS = Cast<AUTFlagRunGameState>(CTFGameState);

	if (Instigator == nullptr || GS == nullptr)
	{
		return Super::GetComSwitch(CommandTag, ContextActor, InInstigator, World);
	}

	AUTPlayerState* UTPlayerState = Cast<AUTPlayerState>(InInstigator->PlayerState);
	AUTCharacter* ContextCharacter = ContextActor != nullptr ? Cast<AUTCharacter>(ContextActor) : nullptr;
	AUTPlayerState* ContextPlayerState = ContextCharacter != nullptr ? Cast<AUTPlayerState>(ContextCharacter->PlayerState) : nullptr;
	
	uint8 OffensiveTeamNum = GS->bRedToCap ? 0 : 1;

	if (ContextCharacter)
	{
		bool bContextOnSameTeam = ContextCharacter != nullptr ? World->GetGameState<AUTGameState>()->OnSameTeam(InInstigator, ContextCharacter) : false;
		bool bContextIsFlagCarrier = ContextPlayerState != nullptr && ContextPlayerState->CarriedObject != nullptr;

		if (bContextIsFlagCarrier)
		{
			if ( bContextOnSameTeam )
			{
				if ( CommandTag == CommandTags::Intent )
				{
					return GOT_YOUR_BACK_SWITCH_INDEX;
				}

				else if (CommandTag == CommandTags::Attack)
				{
					return GOING_IN_SWITCH_INDEX;
				}

				else if (CommandTag == CommandTags::Defend)
				{
					return ATTACK_THEIR_BASE_SWITCH_INDEX;
				}
			}
			else
			{
				if (CommandTag == CommandTags::Intent)
				{
					return ENEMY_FC_HERE_SWITCH_INDEX;
				}
				else if (CommandTag == CommandTags::Attack)
				{
					return GET_FLAG_BACK_SWITCH_INDEX;
				}
				else if (CommandTag == CommandTags::Defend)
				{
					return BASE_UNDER_ATTACK_SWITCH_INDEX;
				}
			}
		}
	}

	AUTCharacter* InstCharacter = Cast<AUTCharacter>(InInstigator->GetCharacter());
	if (InstCharacter != nullptr && !InstCharacter->IsDead())
	{
		// We aren't dead, look to see if we have the flag...
			
		if (UTPlayerState->CarriedObject != nullptr)
		{
			if (CommandTag == CommandTags::Intent)			
			{
				return GOT_FLAG_SWITCH_INDEX;
			}
			if (CommandTag == CommandTags::Attack)			
			{
				return ATTACK_THEIR_BASE_SWITCH_INDEX;
			}
			if (CommandTag == CommandTags::Defend)			
			{
				return DEFEND_FLAG_CARRIER_SWITCH_INDEX;
			}
		}
	}

	if (CommandTag == CommandTags::Intent)
	{
		// Look to see if I'm on offense or defense...

		if (InInstigator->GetTeamNum() == OffensiveTeamNum)
		{
			return ATTACK_THEIR_BASE_SWITCH_INDEX;
		}
		else
		{
			return AREA_SECURE_SWITCH_INDEX;
		}
	}

	if (CommandTag == CommandTags::Attack)
	{
		// Look to see if I'm on offense or defense...

		if (InInstigator->GetTeamNum() == OffensiveTeamNum)
		{
			return ATTACK_THEIR_BASE_SWITCH_INDEX;
		}
		else
		{
			return ON_OFFENSE_SWITCH_INDEX;
		}
	}

	if (CommandTag == CommandTags::Defend)
	{
		// Look to see if I'm on offense or defense...

		if (InInstigator->GetTeamNum() == OffensiveTeamNum)
		{
			return ON_DEFENSE_SWITCH_INDEX;
		}
		else
		{
			return SPREAD_OUT_SWITCH_INDEX;
		}
	}

	if (CommandTag == CommandTags::Distress)
	{
		return UNDER_HEAVY_ATTACK_SWITCH_INDEX;  
	}

	return Super::GetComSwitch(CommandTag, ContextActor, InInstigator, World);
}

void AUTFlagRunGame::HandleRallyRequest(AUTPlayerController* RequestingPC)
{
	AUTCharacter* UTCharacter = RequestingPC->GetUTCharacter();
	AUTPlayerState* UTPlayerState = RequestingPC->UTPlayerState;

	// if can rally, teleport with transloc effect, set last rally time
	AUTFlagRunGameState* GS = GetWorld()->GetGameState<AUTFlagRunGameState>();
	AUTTeamInfo* Team = UTPlayerState ? UTPlayerState->Team : nullptr;
	if (Team && UTPlayerState->bCanRally && GS->bAttackersCanRally && UTCharacter && GS && IsMatchInProgress() && !GS->IsMatchIntermission() && ((Team->TeamIndex == 0) == GS->bRedToCap) && GS->FlagBases.IsValidIndex(Team->TeamIndex) && GS->FlagBases[Team->TeamIndex] != nullptr)
	{
		if (UTCharacter->GetCarriedObject())
		{
			if (GetWorld()->GetTimeSeconds() - RallyRequestTime > 10.f)
			{
				// requesting rally
				for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
				{
					AUTPlayerController* PC = Cast<AUTPlayerController>(*Iterator);
					if (PC && GS->OnSameTeam(RequestingPC, PC) && PC->UTPlayerState && PC->UTPlayerState->bCanRally)
					{
						PC->ClientReceiveLocalizedMessage(UUTCTFMajorMessage::StaticClass(), 22, UTPlayerState);
					}
				}
				UTPlayerState->AnnounceStatus(StatusMessage::NeedBackup);
				RallyRequestTime = GetWorld()->GetTimeSeconds();
			}
		}
		else
		{
			// rally to flag carrier
			AUTCTFFlag* Flag = Cast<AUTCTFFlag>(GS->FlagBases[GS->bRedToCap ? 0 : 1]->GetCarriedObject());
			AUTCharacter* FlagCarrier = Flag ? Flag->HoldingPawn : nullptr;
			if (FlagCarrier != nullptr)
			{
				if (bFixedRally && CurrentRallyPoint)
				{

					RequestingPC->RallyLocation = CurrentRallyPoint->GetActorLocation();
				}
				else
				{
					FVector BestRecentPosition = Flag->RecentPosition[0];
					float OneDist = (FlagCarrier->GetActorLocation() - Flag->RecentPosition[1]).Size();
					if (OneDist < 400.f)
					{
						float ZeroDist = (FlagCarrier->GetActorLocation() - Flag->RecentPosition[0]).Size();
						BestRecentPosition = (OneDist > ZeroDist) ? Flag->RecentPosition[1] : Flag->RecentPosition[0];
					}
					RequestingPC->RallyLocation = BestRecentPosition;
				}
				RequestingPC->RallyFlagCarrier = FlagCarrier;
				UTCharacter->bTriggerRallyEffect = true;
				UTCharacter->OnTriggerRallyEffect();
				RequestingPC->BeginRallyTo(FlagCarrier, RequestingPC->RallyLocation, 1.2f);
				UTCharacter->SpawnRallyDestinationEffectAt(RequestingPC->RallyLocation);  
				if (UTCharacter->UTCharacterMovement)
				{
					UTCharacter->UTCharacterMovement->StopMovementImmediately();
					UTCharacter->UTCharacterMovement->DisableMovement();
					UTCharacter->DisallowWeaponFiring(true);
				}
			}
		}
	}
}

void AUTFlagRunGame::CompleteRallyRequest(AUTPlayerController* RequestingPC)
{
	AUTCharacter* UTCharacter = RequestingPC->GetUTCharacter();
	AUTPlayerState* UTPlayerState = RequestingPC->UTPlayerState;

	// if can rally, teleport with transloc effect, set last rally time
	AUTFlagRunGameState* GS = GetWorld()->GetGameState<AUTFlagRunGameState>();
	AUTTeamInfo* Team = UTPlayerState ? UTPlayerState->Team : nullptr;
	if (!UTCharacter || !IsMatchInProgress() || !GS || GS->IsMatchIntermission() || UTCharacter->IsPendingKillPending())
	{
		return;
	}
	UTCharacter->bTriggerRallyEffect = false;
	if (UTCharacter->UTCharacterMovement)
	{
		UTCharacter->UTCharacterMovement->SetDefaultMovementMode();
	}
	UTCharacter->DisallowWeaponFiring(false);
	if (!UTCharacter->bCanRally)
	{
		RequestingPC->ClientPlaySound(RallyFailedSound);
		return;
	}

	if (Team && ((Team->TeamIndex == 0) == GS->bRedToCap) && GS->FlagBases.IsValidIndex(Team->TeamIndex) && GS->FlagBases[Team->TeamIndex] != nullptr)
	{
		FVector WarpLocation = FVector::ZeroVector;
		FRotator WarpRotation(0.0f, UTCharacter->GetActorRotation().Yaw, 0.0f);
		float HalfHeight = UTCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
		float Radius = UTCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
		FCollisionShape PlayerCapsule = FCollisionShape::MakeCapsule(Radius, HalfHeight);
		FHitResult Hit;
		float SweepRadius = UTCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
		float RallyDelay = 10.f;
		// rally to flag carrier
		AUTCTFFlag* Flag = Cast<AUTCTFFlag>(GS->FlagBases[GS->bRedToCap ? 0 : 1]->GetCarriedObject());
		AUTCharacter* FlagCarrier = Flag ? Flag->HoldingPawn : nullptr;
		if (!bFixedRally && ((FlagCarrier == nullptr) || (FlagCarrier != RequestingPC->RallyFlagCarrier)))
		{
			RequestingPC->ClientPlaySound(RallyFailedSound);
			return;
		}
		ECollisionChannel SavedObjectType = UTCharacter->GetCapsuleComponent()->GetCollisionObjectType();
		UTCharacter->GetCapsuleComponent()->SetCollisionObjectType(COLLISION_TELEPORTING_OBJECT);
		float Offset = 4.f * Radius;
		bool bHitFloor = true;

		if (GetWorld()->FindTeleportSpot(UTCharacter, RequestingPC->RallyLocation, WarpRotation))
		{
			WarpLocation = RequestingPC->RallyLocation;
		}
		else
		{
			float RecentPosDist = FlagCarrier ? (FlagCarrier->GetActorLocation() - Flag->RecentPosition[0]).Size() : 100.f;
			if ((RecentPosDist < 400.f) && (RecentPosDist > 50.f) && GetWorld()->FindTeleportSpot(UTCharacter, Flag->RecentPosition[0], WarpRotation))
			{
				WarpLocation = Flag->RecentPosition[0];
			}
			else
			{
				FVector CarrierLocation = RequestingPC->RallyLocation;
				WarpLocation = CarrierLocation + FVector(0.f, 0.f, HalfHeight);
				if (FlagCarrier)
				{
					WarpLocation += 100.f*(FlagCarrier->GetVelocity().IsNearlyZero() ? FlagCarrier->GetActorRotation().Vector() : -1.f *  FlagCarrier->GetVelocity().GetSafeNormal());
				}
				if (GetWorld()->SweepSingleByChannel(Hit, CarrierLocation, WarpLocation, FQuat::Identity, UTCharacter->GetCapsuleComponent()->GetCollisionObjectType(), FCollisionShape::MakeCapsule(Radius, HalfHeight), FCollisionQueryParams(FName(TEXT("Translocation")), false, UTCharacter), UTCharacter->GetCapsuleComponent()->GetCollisionResponseToChannels()))
				{
					WarpLocation = Hit.Location;
					bHitFloor = GetWorld()->SweepSingleByChannel(Hit, WarpLocation, WarpLocation - FVector(0.f, 0.f, 3.f* HalfHeight), FQuat::Identity, UTCharacter->GetCapsuleComponent()->GetCollisionObjectType(), FCollisionShape::MakeSphere(SweepRadius), FCollisionQueryParams(FName(TEXT("Translocation")), false, UTCharacter), UTCharacter->GetCapsuleComponent()->GetCollisionResponseToChannels());
				}

				// also move off of flag carrier so can watch
				if (!GetWorld()->FindTeleportSpot(UTCharacter, WarpLocation, WarpRotation) || !bHitFloor)
				{
					for (int32 i = 0; i < 4; i++)
					{
						WarpLocation = CarrierLocation + FVector(Offset * ((i % 2 == 0) ? 1.f : -1.f), Offset * ((i > 1) ? 1.f : -1.f), HalfHeight);
						if (GetWorld()->FindTeleportSpot(UTCharacter, WarpLocation, WarpRotation) && !GetWorld()->SweepSingleByChannel(Hit, CarrierLocation, WarpLocation, FQuat::Identity, UTCharacter->GetCapsuleComponent()->GetCollisionObjectType(), FCollisionShape::MakeCapsule(Radius, HalfHeight), FCollisionQueryParams(FName(TEXT("Translocation")), false, UTCharacter), UTCharacter->GetCapsuleComponent()->GetCollisionResponseToChannels()))
						{
							bHitFloor = GetWorld()->SweepSingleByChannel(Hit, WarpLocation, WarpLocation - FVector(0.f, 0.f, 3.f* HalfHeight), FQuat::Identity, UTCharacter->GetCapsuleComponent()->GetCollisionObjectType(), FCollisionShape::MakeSphere(SweepRadius), FCollisionQueryParams(FName(TEXT("Translocation")), false, UTCharacter), UTCharacter->GetCapsuleComponent()->GetCollisionResponseToChannels());
							if (bHitFloor)
							{
								break;
							}
						}
					}
				}
			}

		}
		if (!bHitFloor)
		{
			RequestingPC->ClientPlaySound(RallyFailedSound);
			return;
		}
		UTCharacter->GetCapsuleComponent()->SetCollisionObjectType(SavedObjectType);
		FRotator DesiredRotation = FlagCarrier ? (FlagCarrier->GetActorLocation() - WarpLocation).Rotation() : (Flag->GetActorLocation() - WarpLocation).Rotation();
		WarpRotation.Yaw = DesiredRotation.Yaw;
		RallyDelay = 15.f;

		// teleport
		UPrimitiveComponent* SavedPlayerBase = UTCharacter->GetMovementBase();
		FTransform SavedPlayerTransform = UTCharacter->GetTransform();
		if (UTCharacter->TeleportTo(WarpLocation, WarpRotation))
		{
			RequestingPC->UTClientSetRotation(WarpRotation);
			UTPlayerState->NextRallyTime = GetWorld()->GetTimeSeconds() + RallyDelay;

			if (TranslocatorClass)
			{
				if (TranslocatorClass->GetDefaultObject<AUTWeap_Translocator>()->DestinationEffect != nullptr)
				{
					FActorSpawnParameters SpawnParams;
					SpawnParams.Instigator = UTCharacter;
					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					SpawnParams.Owner = UTCharacter;
					GetWorld()->SpawnActor<AUTReplicatedEmitter>(TranslocatorClass->GetDefaultObject<AUTWeap_Translocator>()->DestinationEffect, UTCharacter->GetActorLocation(), UTCharacter->GetActorRotation(), SpawnParams);
				}
				UUTGameplayStatics::UTPlaySound(GetWorld(), TranslocatorClass->GetDefaultObject<AUTWeap_Translocator>()->TeleSound, UTCharacter, SRT_All);
			}

			// spawn effects
			FActorSpawnParameters SpawnParams;
			SpawnParams.Instigator = UTCharacter;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParams.Owner = UTCharacter;
			if (AfterImageType != NULL)
			{
				AUTWeaponRedirector* AfterImage = GetWorld()->SpawnActor<AUTWeaponRedirector>(AfterImageType, SavedPlayerTransform.GetLocation(), SavedPlayerTransform.GetRotation().Rotator(), SpawnParams);
				if (AfterImage != NULL)
				{
					AfterImage->InitFor(UTCharacter, FRepCollisionShape(PlayerCapsule), SavedPlayerBase, UTCharacter->GetTransform());
				}
			}

			// announce
			UTCharacter->UTCharacterMovement->UpdatedComponent->UpdatePhysicsVolume(true);
			AActor* RallySpot = UTCharacter->UTCharacterMovement ? UTCharacter->UTCharacterMovement->GetPhysicsVolume() : nullptr;
			if ((RallySpot == nullptr) || (RallySpot == GetWorld()->GetDefaultPhysicsVolume()))
			{
				AUTCTFFlag* CarriedFlag = Cast<AUTCTFFlag>(GS->FlagBases[GS->bRedToCap ? 0 : 1]->GetCarriedObject());
				if (CarriedFlag)
				{
					RallySpot = CarriedFlag;
				}
			}
			for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
			{
				AUTPlayerController* PC = Cast<AUTPlayerController>(*Iterator);
				if (PC)
				{
					if (GS->OnSameTeam(RequestingPC, PC))
					{
						PC->ClientReceiveLocalizedMessage(UUTCTFMajorMessage::StaticClass(), 27, UTPlayerState);
						if (GetWorld()->GetTimeSeconds() - RallyRequestTime < 6.f)
						{
							PC->ClientReceiveLocalizedMessage(UTPlayerState->GetCharacterVoiceClass(), ACKNOWLEDGE_SWITCH_INDEX, UTPlayerState, PC->PlayerState, NULL);
						}
					}
					else
					{
						PC->ClientReceiveLocalizedMessage(UUTCTFMajorMessage::StaticClass(), 24, UTPlayerState, nullptr, RallySpot);
					}
				}
			}
			if (!GetWorldTimerManager().IsTimerActive(EnemyRallyWarningHandle) && (GetWorld()->GetTimeSeconds() - LastEnemyRallyWarning > 10.f))
			{
				GetWorldTimerManager().SetTimer(EnemyRallyWarningHandle, this, &AUTFlagRunGame::WarnEnemyRally, 1.5f, false);
			}
		}
	}
}

void AUTFlagRunGame::WarnEnemyRally()
{
	AUTFlagRunGameState* GS = GetWorld()->GetGameState<AUTFlagRunGameState>();
	if (GS->bAttackersCanRally)
	{
		LastEnemyRallyWarning = GetWorld()->GetTimeSeconds();
		for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
		{
			AUTPlayerState* UTPS = Cast<AUTPlayerState>((*Iterator)->PlayerState);
			if (UTPS && UTPS->Team && ((UTPS->Team->TeamIndex == 0) != GS->bRedToCap))
			{
				UTPS->AnnounceStatus(StatusMessage::EnemyRally);
				break;
			}
		}
	}
}

void AUTFlagRunGame::HandleMatchIntermission()
{
	Super::HandleMatchIntermission();

	AUTFlagRunGameState* GS = Cast<AUTFlagRunGameState>(UTGameState);
	if ((GS == nullptr) || (GS->CTFRound < GS->NumRounds - 2))
	{
		return;
	}

	// Update win requirements if last two rounds
	AUTTeamInfo* NextAttacker = (GS->bRedToCap == GS->IsMatchIntermission()) ? GS->Teams[1] : GS->Teams[0];
	AUTTeamInfo* NextDefender = (GS->bRedToCap == GS->IsMatchIntermission()) ? GS->Teams[0] : GS->Teams[1];
	int32 RequiredTime = (GS->bRedToCap == GS->IsMatchIntermission()) ? GS->TiebreakValue : -1 * GS->TiebreakValue;
	RequiredTime = FMath::Max(RequiredTime, 0);
	GS->FlagRunMessageTeam = nullptr;
	if (GS->CTFRound == GS->NumRounds - 2)
	{
		if (NextAttacker->Score > NextDefender->Score)
		{
			GS->FlagRunMessageTeam = NextDefender;
			if (NextAttacker->Score - NextDefender->Score > 2)
			{
				// Defenders must stop attackers to have a chance
				GS->FlagRunMessageSwitch = 1;
			}
			else
			{
				int32 BonusType = (NextAttacker->Score - NextDefender->Score == 2) ? 1 : 2;
				GS->FlagRunMessageSwitch = BonusType + 1;
			}
		}
		else if (NextDefender->Score > NextAttacker->Score)
		{
			GS->FlagRunMessageTeam = NextAttacker;
			
			int32 BonusType = FMath::Max(1, (NextDefender->Score - NextAttacker->Score) - 1);
			if (RequiredTime > 60)
			{
				BonusType++;
				RequiredTime = 0;
			}
			BonusType = FMath::Min(BonusType, 3);
			GS->FlagRunMessageSwitch = 100 * RequiredTime + BonusType + 3;
		}
	}
	else if (GS->CTFRound == GS->NumRounds - 1)
	{
		bool bNeedTimeThreshold = false;
		GS->FlagRunMessageTeam = NextAttacker;
		if (NextDefender->Score <= NextAttacker->Score)
		{
			GS->FlagRunMessageSwitch = 8;
		}
		else
		{
			int32 BonusType = NextDefender->Score - NextAttacker->Score;
			if (RequiredTime > 60)
			{
				BonusType++;
				RequiredTime = 0;
			}
			GS->FlagRunMessageSwitch = 7 + BonusType + 100 * RequiredTime;
		}
	}
}


void AUTFlagRunGame::CheatScore()
{
	if ((UE_BUILD_DEVELOPMENT || (GetNetMode() == NM_Standalone)) && !bOfflineChallenge && !bBasicTrainingGame && UTGameState)
	{
		UTGameState->SetRemainingTime(FMath::RandHelper(150));
		IntermissionDuration = 12.f;
	}
	Super::CheatScore();
}

void AUTFlagRunGame::UpdateSkillRating()
{
	ReportRankedMatchResults(NAME_FlagRunSkillRating.ToString());
}

uint8 AUTFlagRunGame::GetNumMatchesFor(AUTPlayerState* PS, bool bInRankedSession) const
{
	return PS ? PS->FlagRunMatchesPlayed : 0;
}

int32 AUTFlagRunGame::GetEloFor(AUTPlayerState* PS, bool bInRankedSession) const
{
	return PS ? PS->FlagRunRank : Super::GetEloFor(PS, bInRankedSession);
}

void AUTFlagRunGame::SetEloFor(AUTPlayerState* PS, bool bInRankedSession, int32 NewEloValue, bool bIncrementMatchCount)
{
	if (PS)
	{
		PS->FlagRunRank = NewEloValue;
		if (bIncrementMatchCount && (PS->FlagRunMatchesPlayed < 255))
		{
			PS->FlagRunMatchesPlayed++;
		}
	}
}

void AUTFlagRunGame::GrantPowerupToTeam(int TeamIndex, AUTPlayerState* PlayerToHighlight)
{
	if (!bAllowBoosts)
	{
		return;
	}
	for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
		if (PS && PS->Team)
		{
			if (PS->Team->TeamIndex == TeamIndex)
			{
				if (PS->BoostClass && PS->BoostClass.GetDefaultObject() && PS->BoostClass.GetDefaultObject()->RemainingBoostsGivenOverride > 0)
				{
					PS->SetRemainingBoosts(PS->BoostClass.GetDefaultObject()->RemainingBoostsGivenOverride);
				}
				else
				{
					PS->SetRemainingBoosts(1);
				}
			}
			AUTPlayerController* PC = Cast<AUTPlayerController>(PS->GetOwner());
			if (PC)
			{
				if (PS->Team->TeamIndex == TeamIndex)
				{
					PC->ClientReceiveLocalizedMessage(UUTCTFRewardMessage::StaticClass(), 7, PlayerToHighlight);
				}
				else
				{
					PC->ClientReceiveLocalizedMessage(UUTCTFRoleMessage::StaticClass(), 7, PlayerToHighlight);
				}
			}
		}
	}
}

void AUTFlagRunGame::HandleTeamChange(AUTPlayerState* PS, AUTTeamInfo* OldTeam)
{
	// If a player doesn't have a valid selected boost powerup, lets go ahead and give them the 1st one available in the Powerup List
	if (PS && UTGameState && bAllowBoosts)
	{
		if (!PS->BoostClass || !UTGameState->IsSelectedBoostValid(PS))
		{
			TSubclassOf<class AUTInventory> SelectedBoost = UTGameState->GetSelectableBoostByIndex(PS, 0);
			PS->BoostClass = SelectedBoost;
		}
	}
	Super::HandleTeamChange(PS, OldTeam);
}

void AUTFlagRunGame::ScoreKill_Implementation(AController* Killer, AController* Other, APawn* KilledPawn, TSubclassOf<UDamageType> DamageType)
{
	Super::ScoreKill_Implementation(Killer, Other, KilledPawn, DamageType);
	HandlePowerupUnlocks(KilledPawn, Killer);
}

void AUTFlagRunGame::HandlePowerupUnlocks(APawn* Other, AController* Killer)
{
	AUTPlayerState* KillerPS = Killer ? Cast<AUTPlayerState>(Killer->PlayerState) : nullptr;
	AUTPlayerState* VictimPS = Other ? Cast<AUTPlayerState>(Other->PlayerState) : nullptr;

	UpdatePowerupUnlockProgress(VictimPS, KillerPS);

	const int RedTeamIndex = 0;
	const int BlueTeamIndex = 1;

	AUTFlagRunGameState* RCTFGameState = Cast<AUTFlagRunGameState>(CTFGameState);
	if (RCTFGameState)
	{
		if ((RCTFGameState->OffenseKills >= OffenseKillsNeededForPowerUp) && RCTFGameState->bIsOffenseAbleToGainPowerup)
		{
			RCTFGameState->OffenseKills = 0;

			GrantPowerupToTeam(IsTeamOnOffense(RedTeamIndex) ? RedTeamIndex : BlueTeamIndex, KillerPS);
			RCTFGameState->bIsOffenseAbleToGainPowerup = false;
		}

		if ((RCTFGameState->DefenseKills >= DefenseKillsNeededForPowerUp) && RCTFGameState->bIsDefenseAbleToGainPowerup)
		{
			RCTFGameState->DefenseKills = 0;

			GrantPowerupToTeam(IsTeamOnDefense(RedTeamIndex) ? RedTeamIndex : BlueTeamIndex, KillerPS);
			RCTFGameState->bIsDefenseAbleToGainPowerup = false;
		}
	}
}

void AUTFlagRunGame::UpdatePowerupUnlockProgress(AUTPlayerState* VictimPS, AUTPlayerState* KillerPS)
{
	AUTCTFRoundGameState* RCTFGameState = Cast<AUTCTFRoundGameState>(CTFGameState);

	if (RCTFGameState && VictimPS && VictimPS->Team && KillerPS && KillerPS->Team)
	{
		//No credit for suicides
		if (VictimPS->Team->TeamIndex != KillerPS->Team->TeamIndex)
		{
			if (IsTeamOnDefense(VictimPS->Team->TeamIndex))
			{
				++(RCTFGameState->OffenseKills);
			}
			else
			{
				++(RCTFGameState->DefenseKills);
			}
		}
	}
}

AActor* AUTFlagRunGame::SetIntermissionCameras(uint32 TeamToWatch)
{
	AUTFlagRunGameState* FRGS = Cast<AUTFlagRunGameState>(CTFGameState);
	if (FRGS)
	{
		RemoveLosers(1 - TeamToWatch, (FRGS && FRGS->bRedToCap) ? 0 : 1);

		// place winners around defender base
		PlacePlayersAroundFlagBase(TeamToWatch, (FRGS && FRGS->bRedToCap) ? 1 : 0);
		return FRGS->FlagBases[(FRGS && FRGS->bRedToCap) ? 1 : 0];
	}
	return nullptr;
}

bool AUTFlagRunGame::IsTeamOnOffense(int32 TeamNumber) const
{
	AUTFlagRunGameState* FRGS = Cast<AUTFlagRunGameState>(CTFGameState);
	return FRGS && (FRGS->bRedToCap == (TeamNumber == 0));
}

void AUTFlagRunGame::SendRestartNotifications(AUTPlayerState* PS, AUTPlayerController* PC)
{
	if (PS->Team && IsTeamOnOffense(PS->Team->TeamIndex))
	{
		LastAttackerSpawnTime = GetWorld()->GetTimeSeconds();
		AUTFlagRunGameState* FRGS = Cast<AUTFlagRunGameState>(CTFGameState);
		if (FRGS && (FRGS->GetRemainingTime() < 240) && !FRGS->bAttackersCanRally && (GetWorld()->GetTimeSeconds() > PS->NextRallyTime) && FRGS->bHaveEstablishedFlagRunner)
		{
			PS->AnnounceStatus(StatusMessage::NeedRally);
		}
	}
	if (PC && (PS->GetRemainingBoosts() > 0))
	{
		PC->ClientReceiveLocalizedMessage(UUTCTFRoleMessage::StaticClass(), 20);
	}
}

void AUTFlagRunGame::ScoreObject_Implementation(AUTCarriedObject* GameObject, AUTCharacter* HolderPawn, AUTPlayerState* Holder, FName Reason)
{
	Super::ScoreObject_Implementation(GameObject,HolderPawn,Holder,Reason);

	if (FUTAnalytics::IsAvailable())
	{
		FUTAnalytics::FireEvent_FlagRunRoundEnd(this, false, (UTGameState->WinningTeam != nullptr));
	}

	for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
	{
		AUTPlayerController* UTPC = Cast<AUTPlayerController>(*Iterator);
		if (UTPC)
		{
			UTPC->ClientPlayInstantReplay(HolderPawn, 10.0f);
		}
	}
}

void AUTFlagRunGame::ScoreAlternateWin(int32 WinningTeamIndex, uint8 Reason /* = 0 */)
{
	Super::ScoreAlternateWin(WinningTeamIndex, Reason);

	if (FUTAnalytics::IsAvailable())
	{
		FUTAnalytics::FireEvent_FlagRunRoundEnd(this, IsTeamOnDefense(WinningTeamIndex), (UTGameState->WinningTeam != nullptr));
	}
}