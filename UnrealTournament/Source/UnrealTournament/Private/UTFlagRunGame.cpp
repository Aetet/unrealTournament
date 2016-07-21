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
#include "UTCTFRoundGameState.h"
#include "UTAsymCTFSquadAI.h"
#include "UTWeaponRedirector.h"
#include "UTWeaponLocker.h"
#include "UTFlagRunMessage.h"
#include "UTWeap_Translocator.h"
#include "UTReplicatedEmitter.h"

AUTFlagRunGame::AUTFlagRunGame(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAsymmetricVictoryConditions = true;
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

	static ConstructorHelpers::FObjectFinder<UClass> AfterImageFinder(TEXT("Blueprint'/Game/RestrictedAssets/Weapons/Translocator/TransAfterImage.TransAfterImage_C'"));
	AfterImageType = AfterImageFinder.Object;
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

float AUTFlagRunGame::OverrideRespawnTime(TSubclassOf<AUTInventory> InventoryType)
{
	if (InventoryType == nullptr)
	{
		return 0.f;
	}
	AUTWeapon* WeaponDefault = Cast<AUTWeapon>(InventoryType.GetDefaultObject());
	return (WeaponDefault && !WeaponDefault->bMustBeHolstered) ? 20.f : InventoryType.GetDefaultObject()->RespawnTime;
}

int32 AUTFlagRunGame::GetComSwitch(FName CommandTag, AActor* ContextActor, AUTPlayerController* Instigator, UWorld* World)
{
	if (World == nullptr) return INDEX_NONE;

	AUTCTFGameState* UTCTFGameState = World->GetGameState<AUTCTFGameState>();

	if (Instigator == nullptr || UTCTFGameState == nullptr) 
	{
		return Super::GetComSwitch(CommandTag, ContextActor, Instigator, World);
	}

	AUTPlayerState* UTPlayerState = Cast<AUTPlayerState>(Instigator->PlayerState);
	AUTCharacter* ContextCharacter = ContextActor != nullptr ? Cast<AUTCharacter>(ContextActor) : nullptr;
	AUTPlayerState* ContextPlayerState = ContextCharacter != nullptr ? Cast<AUTPlayerState>(ContextCharacter->PlayerState) : nullptr;
	
	uint8 OffensiveTeamNum = UTCTFGameState->bRedToCap ? 0 : 1;

	if (ContextCharacter)
	{
		bool bContextOnSameTeam = ContextCharacter != nullptr ? World->GetGameState<AUTGameState>()->OnSameTeam(Instigator, ContextCharacter) : false;
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

	AUTCharacter* InstCharacter = Cast<AUTCharacter>(Instigator->GetCharacter());
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

		if (Instigator->GetTeamNum() == OffensiveTeamNum)
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

		if (Instigator->GetTeamNum() == OffensiveTeamNum)
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

		if (Instigator->GetTeamNum() == OffensiveTeamNum)
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

	return Super::GetComSwitch(CommandTag, ContextActor, Instigator, World);
}

void AUTFlagRunGame::HandleRallyRequest(AUTPlayerController* RequestingPC)
{
	AUTCharacter* UTCharacter = RequestingPC->GetUTCharacter();
	AUTPlayerState* UTPlayerState = RequestingPC->UTPlayerState;

	// if can rally, teleport with transloc effect, set last rally time
	AUTCTFGameState* GS = GetWorld()->GetGameState<AUTCTFGameState>();
	AUTTeamInfo* Team = UTPlayerState ? UTPlayerState->Team : nullptr;
	if (Team && UTPlayerState->bCanRally && UTCharacter && GS && IsMatchInProgress() && !GS->IsMatchIntermission() && ((Team->TeamIndex == 0) ? GS->bRedCanRally : GS->bBlueCanRally) && GS->FlagBases.IsValidIndex(Team->TeamIndex) && GS->FlagBases[Team->TeamIndex] != nullptr)
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
		else if ((Team->TeamIndex == 0) == GS->bRedToCap)
		{
			// rally to flag carrier
			AUTCTFFlag* Flag = Cast<AUTCTFFlag>(GS->FlagBases[GS->bRedToCap ? 0 : 1]->GetCarriedObject());
			AUTCharacter* FlagCarrier = Flag ? Flag->HoldingPawn : nullptr;
			if (FlagCarrier != nullptr)
			{
				FVector BestRecentPosition = Flag->RecentPosition[0];
				float OneDist = (FlagCarrier->GetActorLocation() - Flag->RecentPosition[1]).Size();
				if (OneDist < 400.f)
				{
					float ZeroDist = (FlagCarrier->GetActorLocation() - Flag->RecentPosition[0]).Size();
					BestRecentPosition = (OneDist > ZeroDist) ? Flag->RecentPosition[1] : Flag->RecentPosition[0];
				}
				RequestingPC->RallyLocation = BestRecentPosition;
				if (bDelayedRally)
				{
					RequestingPC->BeginRallyTo(FlagCarrier, RequestingPC->RallyLocation, 1.2f);
				}
				else
				{
					CompleteRallyRequest(RequestingPC);
				}
			}
		}
		else
		{
			CompleteRallyRequest(RequestingPC);
		}
	}
}

void AUTFlagRunGame::CompleteRallyRequest(AUTPlayerController* RequestingPC)
{
	AUTCharacter* UTCharacter = RequestingPC->GetUTCharacter();
	AUTPlayerState* UTPlayerState = RequestingPC->UTPlayerState;

	// if can rally, teleport with transloc effect, set last rally time
	AUTCTFGameState* GS = GetWorld()->GetGameState<AUTCTFGameState>();
	AUTTeamInfo* Team = UTPlayerState ? UTPlayerState->Team : nullptr;
	if (!IsMatchInProgress() || (GS && GS->IsMatchIntermission()))
	{
		return;
	}

	if (Team && UTCharacter && !UTCharacter->IsPendingKillPending() && GS && GS->FlagBases.IsValidIndex(Team->TeamIndex) && GS->FlagBases[Team->TeamIndex] != nullptr)
	{
		FVector WarpLocation = FVector::ZeroVector;
		FRotator WarpRotation(0.0f, UTCharacter->GetActorRotation().Yaw, 0.0f);
		float HalfHeight = UTCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
		float Radius = UTCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
		FCollisionShape PlayerCapsule = FCollisionShape::MakeCapsule(Radius, HalfHeight);
		FHitResult Hit;
		float SweepRadius = UTCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
		float RallyDelay = 10.f;
		if ((Team->TeamIndex == 0) == GS->bRedToCap)
		{
			// rally to flag carrier
			AUTCTFFlag* Flag = Cast<AUTCTFFlag>(GS->FlagBases[GS->bRedToCap ? 0 : 1]->GetCarriedObject());
			AUTCharacter* FlagCarrier = Flag ? Flag->HoldingPawn : nullptr;
			if (FlagCarrier == nullptr)
			{
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
				float RecentPosDist = (FlagCarrier->GetActorLocation() - Flag->RecentPosition[0]).Size();
				if ((RecentPosDist < 400.f) && (RecentPosDist > 50.f) && GetWorld()->FindTeleportSpot(UTCharacter, Flag->RecentPosition[0], WarpRotation))
				{
					WarpLocation = Flag->RecentPosition[0];
				}
				else
				{
					FVector CarrierLocation = RequestingPC->RallyLocation;
					WarpLocation = CarrierLocation + FVector(0.f, 0.f, HalfHeight) + 100.f*(FlagCarrier->GetVelocity().IsNearlyZero() ? FlagCarrier->GetActorRotation().Vector() : -1.f *  FlagCarrier->GetVelocity().GetSafeNormal());
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
				return;
			}
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
			UTCharacter->GetCapsuleComponent()->SetCollisionObjectType(SavedObjectType);
			FRotator DesiredRotation = (FlagCarrier->GetActorLocation() - WarpLocation).Rotation();
			WarpRotation.Yaw = DesiredRotation.Yaw;
			RallyDelay = 20.f;
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
						PC->ClientReceiveLocalizedMessage(UUTCTFMajorMessage::StaticClass(), 24, UTPlayerState);
					}
				}
			}
		}
		else
		{
			// defenders rally back to flag, once
			WarpLocation = GS->FlagBases[Team->TeamIndex]->GetActorLocation() + FVector(0.f, 0.f, UTCharacter->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
			FVector EndTrace = WarpLocation - FVector(0.0f, 0.0f, PlayerCapsule.GetCapsuleHalfHeight());
			bool bHitFloor = GetWorld()->SweepSingleByChannel(Hit, WarpLocation, EndTrace, FQuat::Identity, UTCharacter->GetCapsuleComponent()->GetCollisionObjectType(), FCollisionShape::MakeSphere(SweepRadius), FCollisionQueryParams(FName(TEXT("Translocation")), false, UTCharacter), UTCharacter->GetCapsuleComponent()->GetCollisionResponseToChannels());
			if (bHitFloor)
			{
				// need to move teleport destination up, unless close to ceiling
				FVector NewLocation = Hit.Location + FVector(0.0f, 0.0f, PlayerCapsule.GetCapsuleHalfHeight());
				bool bHitCeiling = GetWorld()->SweepSingleByChannel(Hit, WarpLocation, NewLocation, FQuat::Identity, UTCharacter->GetCapsuleComponent()->GetCollisionObjectType(), FCollisionShape::MakeSphere(SweepRadius), FCollisionQueryParams(FName(TEXT("Translocation")), false, UTCharacter), UTCharacter->GetCapsuleComponent()->GetCollisionResponseToChannels());
				if (!bHitCeiling)
				{
					WarpLocation = NewLocation;
				}
			}
			WarpRotation = GS->FlagBases[Team->TeamIndex]->GetActorRotation();
			RallyDelay = 500.f;
			UTPlayerState->AnnounceStatus(StatusMessage::ImOnDefense);
		}

		// teleport
		UPrimitiveComponent* SavedPlayerBase = UTCharacter->GetMovementBase();
		FTransform SavedPlayerTransform = UTCharacter->GetTransform();
		if (UTCharacter->TeleportTo(WarpLocation, WarpRotation))
		{
			RequestingPC->UTClientSetRotation(WarpRotation);
			UTPlayerState->NextRallyTime = GetWorld()->GetTimeSeconds() + RallyDelay;

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
		}
	}
}

void AUTFlagRunGame::HandleMatchIntermission()
{
	Super::HandleMatchIntermission();

	AUTCTFRoundGameState* GS = Cast<AUTCTFRoundGameState>(UTGameState);
	if ((GS == nullptr) || (GS->CTFRound < GS->NumRounds - 2))
	{
		return;
	}

	// Update win requirements if last two rounds
	AUTTeamInfo* NextAttacker = (GS->bRedToCap == GS->IsMatchIntermission()) ? GS->Teams[1] : GS->Teams[0];
	AUTTeamInfo* NextDefender = (GS->bRedToCap == GS->IsMatchIntermission()) ? GS->Teams[0] : GS->Teams[1];
	int32 RequiredTime = NextDefender->SecondaryScore - NextAttacker->SecondaryScore;
	int32 BonusType = 1;
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
				BonusType = (NextAttacker->Score - NextDefender->Score == 2) ? 1 : 2;
				GS->FlagRunMessageSwitch = BonusType + 1;
			}
		}
		else if (NextDefender->Score > NextAttacker->Score)
		{
			GS->FlagRunMessageTeam = NextAttacker;
			GS->FlagRunMessageSwitch = 4;
			if (NextDefender->Score - NextAttacker->Score > 1)
			{
				// compare bonus times, see what level is implied and state for Attackers to have a chance
				if (RequiredTime < 60 * (NextDefender->Score - NextAttacker->Score - 1))
				{
					if (NextDefender->Score - NextAttacker->Score > 2)
					{
						BonusType = (NextDefender->Score - NextAttacker->Score == 4) ? 3 : 2;
					}
					GS->FlagRunMessageSwitch = BonusType + 3;
				}
				else
				{
					// state required time and threshold
					if (RequiredTime >= GS->SilverBonusThreshold)
					{
						BonusType = (RequiredTime >= GS->GoldBonusThreshold) ? 3 : 2;
					}
					GS->FlagRunMessageSwitch = 100 * RequiredTime + BonusType + 3;
				}
			}
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
			if (NextDefender->Score - NextAttacker->Score > 2)
			{
				BonusType = 3;
				bNeedTimeThreshold = (RequiredTime > 120);
			}
			else if (NextDefender->Score - NextAttacker->Score == 2)
			{
				if (RequiredTime < 120)
				{
					BonusType = 2;
					bNeedTimeThreshold = (RequiredTime > 60);
				}
				else
				{
					BonusType = 3;
					bNeedTimeThreshold = (RequiredTime > 120);
				}
			}
			else //(NextDefender->Score - NextAttacker->Score == 1)
			{
				if (RequiredTime < 60)
				{
					BonusType = 1;
					bNeedTimeThreshold = (RequiredTime > 0);
				}
				else
				{
					BonusType = (RequiredTime < 120) ? 2 : 3;
					bNeedTimeThreshold = true;
				}
			}
			GS->FlagRunMessageSwitch = 7 + BonusType + (bNeedTimeThreshold ? 100 * RequiredTime : 0);
		}
	}
}


void AUTFlagRunGame::CheatScore()
{
	if ((GetNetMode() == NM_Standalone) && !bOfflineChallenge && !bBasicTrainingGame && UTGameState)
	{
		UTGameState->SetRemainingTime(FMath::RandHelper(150));
		IntermissionDuration = 12.f;
	}
	Super::CheatScore();
}

