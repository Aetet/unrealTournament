// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTMultiKillMessage.h"
#include "UTSpreeMessage.h"
#include "UTRemoteRedeemer.h"
#include "UTGameMessage.h"
#include "Net/UnrealNetwork.h"
#include "UTTimerMessage.h"
#include "UTReplicatedLoadoutInfo.h"
#include "UTMutator.h"
#include "UTReplicatedMapInfo.h"
#include "UTPickup.h"
#include "UTArmor.h"
#include "StatNames.h"
#include "UTGameEngine.h"
#include "UTBaseGameMode.h"

AUTGameState::AUTGameState(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	MultiKillMessageClass = UUTMultiKillMessage::StaticClass();
	SpreeMessageClass = UUTSpreeMessage::StaticClass();
	MultiKillDelay = 3.0f;
	SpawnProtectionTime = 2.f;
	bWeaponStay = true;
	bAllowTeamSwitches = true;
	bCasterControl = false;
	bForcedBalance = false;
	KickThreshold=51.0f;
	TauntSelectionIndex = 0;

	ServerName = TEXT("My First Server");
	ServerMOTD = TEXT("Welcome!");
	SecondaryAttackerStat = NAME_None;

	GameScoreStats.Add(NAME_AttackerScore);
	GameScoreStats.Add(NAME_DefenderScore);
	GameScoreStats.Add(NAME_SupporterScore);
	GameScoreStats.Add(NAME_Suicides);

	GameScoreStats.Add(NAME_UDamageTime);
	GameScoreStats.Add(NAME_BerserkTime);
	GameScoreStats.Add(NAME_InvisibilityTime);
	GameScoreStats.Add(NAME_UDamageCount);
	GameScoreStats.Add(NAME_BerserkCount);
	GameScoreStats.Add(NAME_InvisibilityCount);
	GameScoreStats.Add(NAME_KegCount);
	GameScoreStats.Add(NAME_BootJumps);
	GameScoreStats.Add(NAME_ShieldBeltCount);
	GameScoreStats.Add(NAME_ArmorVestCount);
	GameScoreStats.Add(NAME_ArmorPadsCount);
	GameScoreStats.Add(NAME_HelmetCount);

	TeamStats.Add(NAME_TeamKills);
	TeamStats.Add(NAME_UDamageTime);
	TeamStats.Add(NAME_BerserkTime);
	TeamStats.Add(NAME_InvisibilityTime);
	TeamStats.Add(NAME_UDamageCount);
	TeamStats.Add(NAME_BerserkCount);
	TeamStats.Add(NAME_InvisibilityCount);
	TeamStats.Add(NAME_KegCount);
	TeamStats.Add(NAME_BootJumps);
	TeamStats.Add(NAME_ShieldBeltCount);
	TeamStats.Add(NAME_ArmorVestCount);
	TeamStats.Add(NAME_ArmorPadsCount);
	TeamStats.Add(NAME_HelmetCount);

	WeaponStats.Add(NAME_ImpactHammerKills);
	WeaponStats.Add(NAME_EnforcerKills);
	WeaponStats.Add(NAME_BioRifleKills);
	WeaponStats.Add(NAME_ShockBeamKills);
	WeaponStats.Add(NAME_ShockCoreKills);
	WeaponStats.Add(NAME_ShockComboKills);
	WeaponStats.Add(NAME_LinkKills);
	WeaponStats.Add(NAME_LinkBeamKills);
	WeaponStats.Add(NAME_MinigunKills);
	WeaponStats.Add(NAME_MinigunShardKills);
	WeaponStats.Add(NAME_FlakShardKills);
	WeaponStats.Add(NAME_FlakShellKills);
	WeaponStats.Add(NAME_RocketKills);
	WeaponStats.Add(NAME_SniperKills);
	WeaponStats.Add(NAME_SniperHeadshotKills);
	WeaponStats.Add(NAME_RedeemerKills);
	WeaponStats.Add(NAME_InstagibKills);
	WeaponStats.Add(NAME_TelefragKills);

	WeaponStats.Add(NAME_ImpactHammerDeaths);
	WeaponStats.Add(NAME_EnforcerDeaths);
	WeaponStats.Add(NAME_BioRifleDeaths);
	WeaponStats.Add(NAME_ShockBeamDeaths);
	WeaponStats.Add(NAME_ShockCoreDeaths);
	WeaponStats.Add(NAME_ShockComboDeaths);
	WeaponStats.Add(NAME_LinkDeaths);
	WeaponStats.Add(NAME_LinkBeamDeaths);
	WeaponStats.Add(NAME_MinigunDeaths);
	WeaponStats.Add(NAME_MinigunShardDeaths);
	WeaponStats.Add(NAME_FlakShardDeaths);
	WeaponStats.Add(NAME_FlakShellDeaths);
	WeaponStats.Add(NAME_RocketDeaths);
	WeaponStats.Add(NAME_SniperDeaths);
	WeaponStats.Add(NAME_SniperHeadshotDeaths);
	WeaponStats.Add(NAME_RedeemerDeaths);
	WeaponStats.Add(NAME_InstagibDeaths);
	WeaponStats.Add(NAME_TelefragDeaths);

	WeaponStats.Add(NAME_BestShockCombo);
	WeaponStats.Add(NAME_AirRox);
	WeaponStats.Add(NAME_AmazingCombos);
	WeaponStats.Add(NAME_FlakShreds);
	WeaponStats.Add(NAME_AirSnot);

	RewardStats.Add(NAME_MultiKillLevel0);
	RewardStats.Add(NAME_MultiKillLevel1);
	RewardStats.Add(NAME_MultiKillLevel2);
	RewardStats.Add(NAME_MultiKillLevel3);

	RewardStats.Add(NAME_SpreeKillLevel0);
	RewardStats.Add(NAME_SpreeKillLevel1);
	RewardStats.Add(NAME_SpreeKillLevel2);
	RewardStats.Add(NAME_SpreeKillLevel3);
	RewardStats.Add(NAME_SpreeKillLevel4);

	MovementStats.Add(NAME_RunDist);
	MovementStats.Add(NAME_SprintDist);
	MovementStats.Add(NAME_InAirDist);
	MovementStats.Add(NAME_SwimDist);
	MovementStats.Add(NAME_TranslocDist);
	MovementStats.Add(NAME_NumDodges);
	MovementStats.Add(NAME_NumWallDodges);
	MovementStats.Add(NAME_NumJumps);
	MovementStats.Add(NAME_NumLiftJumps);
	MovementStats.Add(NAME_NumFloorSlides);
	MovementStats.Add(NAME_NumWallRuns);
	MovementStats.Add(NAME_NumImpactJumps);
	MovementStats.Add(NAME_NumRocketJumps);
	MovementStats.Add(NAME_SlideDist);
	MovementStats.Add(NAME_WallRunDist);

	WeaponStats.Add(NAME_EnforcerShots);
	WeaponStats.Add(NAME_BioRifleShots);
	WeaponStats.Add(NAME_ShockRifleShots);
	WeaponStats.Add(NAME_LinkShots);
	WeaponStats.Add(NAME_MinigunShots);
	WeaponStats.Add(NAME_FlakShots);
	WeaponStats.Add(NAME_RocketShots);
	WeaponStats.Add(NAME_SniperShots);
	WeaponStats.Add(NAME_RedeemerShots);
	WeaponStats.Add(NAME_InstagibShots);

	WeaponStats.Add(NAME_EnforcerHits);
	WeaponStats.Add(NAME_BioRifleHits);
	WeaponStats.Add(NAME_ShockRifleHits);
	WeaponStats.Add(NAME_LinkHits);
	WeaponStats.Add(NAME_MinigunHits);
	WeaponStats.Add(NAME_FlakHits);
	WeaponStats.Add(NAME_RocketHits);
	WeaponStats.Add(NAME_SniperHits);
	WeaponStats.Add(NAME_RedeemerHits);
	WeaponStats.Add(NAME_InstagibHits);

	HighlightMap.Add(HighlightNames::TopScorer, NSLOCTEXT("AUTGameMode", "HighlightTopScore", "Top Score overall with <UT.MatchSummary.HighlightText.Value>{0}</> points."));
	HighlightMap.Add(HighlightNames::TopScorerRed, NSLOCTEXT("AUTGameMode", "HighlightTopScoreRed", "Red Team Top Score with <UT.MatchSummary.HighlightText.Value>{0}</> points."));
	HighlightMap.Add(HighlightNames::TopScorerBlue, NSLOCTEXT("AUTGameMode", "HighlightTopScoreBlue", "Blue Team Top Score with <UT.MatchSummary.HighlightText.Value>{0}</> points."));
	HighlightMap.Add(HighlightNames::MostKills, NSLOCTEXT("AUTGameMode", "MostKills", "Most Kills with <UT.MatchSummary.HighlightText.Value>{0}</>."));
	HighlightMap.Add(HighlightNames::LeastDeaths, NSLOCTEXT("AUTGameMode", "LeastDeaths", "Least Deaths with <UT.MatchSummary.HighlightText.Value>{0}</>."));
	HighlightMap.Add(HighlightNames::BestKD, NSLOCTEXT("AUTGameMode", "BestKD", "Best Kill/Death ratio <UT.MatchSummary.HighlightText.Value>{0}</>."));
	HighlightMap.Add(HighlightNames::MostWeaponKills, NSLOCTEXT("AUTGameMode", "MostWeaponKills", "Most Kills (<UT.MatchSummary.HighlightText.Value>{0}</>) with <UT.MatchSummary.HighlightText.Value>{1}</>"));
	HighlightMap.Add(HighlightNames::BestCombo, NSLOCTEXT("AUTGameMode", "BestCombo", "Most Impressive Shock Combo."));
	HighlightMap.Add(HighlightNames::MostHeadShots, NSLOCTEXT("AUTGameMode", "MostHeadShots", "Most Headshots (<UT.MatchSummary.HighlightText.Value>{0}</>)."));
	HighlightMap.Add(HighlightNames::MostAirRockets, NSLOCTEXT("AUTGameMode", "MostAirRockets", "Most Air Rockets (<UT.MatchSummary.HighlightText.Value>{0}</>)."));
	HighlightMap.Add(HighlightNames::ParticipationAward, NSLOCTEXT("AUTGameMode", "ParticipationAward", "Participation Award."));

	HighlightMap.Add(NAME_AmazingCombos, NSLOCTEXT("AUTGameMode", "AmazingCombos", "Amazing Combos (<UT.MatchSummary.HighlightText.Value>{0}</>)."));
	HighlightMap.Add(NAME_SniperHeadshotKills, NSLOCTEXT("AUTGameMode", "SniperHeadshotKills", "Headshot Kills (<UT.MatchSummary.HighlightText.Value>{0}</>)."));
	HighlightMap.Add(NAME_AirRox, NSLOCTEXT("AUTGameMode", "AirRox", "Air Rocket Kills (<UT.MatchSummary.HighlightText.Value>{0}</>)."));
	HighlightMap.Add(NAME_FlakShreds, NSLOCTEXT("AUTGameMode", "FlakShreds", "Flak Shred Kills (<UT.MatchSummary.HighlightText.Value>{0}</>)."));
	HighlightMap.Add(NAME_AirSnot, NSLOCTEXT("AUTGameMode", "AirSnot", "Air Snot Kills (<UT.MatchSummary.HighlightText.Value>{0}</>)."));
	HighlightMap.Add(NAME_MultiKillLevel0, NSLOCTEXT("AUTGameMode", "MultiKillLevel0", "Double Kill (<UT.MatchSummary.HighlightText.Value>{0}</>)."));
	HighlightMap.Add(NAME_MultiKillLevel1, NSLOCTEXT("AUTGameMode", "MultiKillLevel1", "Multi Kill (<UT.MatchSummary.HighlightText.Value>{0}</>)."));
	HighlightMap.Add(NAME_MultiKillLevel2, NSLOCTEXT("AUTGameMode", "MultiKillLevel2", "Ultra Kill (<UT.MatchSummary.HighlightText.Value>{0}</>)."));
	HighlightMap.Add(NAME_MultiKillLevel3, NSLOCTEXT("AUTGameMode", "MultiKillLevel3", "Monster Kill (<UT.MatchSummary.HighlightText.Value>{0}</>)."));
	HighlightMap.Add(NAME_SpreeKillLevel0, NSLOCTEXT("AUTGameMode", "SpreeKillLevel0", "Killing Spree (<UT.MatchSummary.HighlightText.Value>{0}</>)."));
	HighlightMap.Add(NAME_SpreeKillLevel1, NSLOCTEXT("AUTGameMode", "SpreeKillLevel1", "Rampage Spree (<UT.MatchSummary.HighlightText.Value>{0}</>)."));
	HighlightMap.Add(NAME_SpreeKillLevel2, NSLOCTEXT("AUTGameMode", "SpreeKillLevel2", "Dominating Spree (<UT.MatchSummary.HighlightText.Value>{0}</>)."));
	HighlightMap.Add(NAME_SpreeKillLevel3, NSLOCTEXT("AUTGameMode", "SpreeKillLevel3", "Unstoppable Spree (<UT.MatchSummary.HighlightText.Value>{0}</>)."));
	HighlightMap.Add(NAME_SpreeKillLevel4, NSLOCTEXT("AUTGameMode", "SpreeKillLevel4", "Godlike Spree (<UT.MatchSummary.HighlightText.Value>{0}</>)."));

	HighlightPriority.Add(HighlightNames::TopScorer, 10.f);
	HighlightPriority.Add(HighlightNames::TopScorerRed, 5.f);
	HighlightPriority.Add(HighlightNames::TopScorerBlue, 5.f);
	HighlightPriority.Add(HighlightNames::MostKills, 3.3f);
	HighlightPriority.Add(HighlightNames::LeastDeaths, 1.f);
	HighlightPriority.Add(HighlightNames::BestKD, 2.f);
	HighlightPriority.Add(HighlightNames::MostWeaponKills, 2.f);
	HighlightPriority.Add(HighlightNames::BestCombo, 2.f);
	HighlightPriority.Add(HighlightNames::MostHeadShots, 2.f);
	HighlightPriority.Add(HighlightNames::MostAirRockets, 2.f);

	HighlightPriority.Add(NAME_AmazingCombos, 1.f);
	HighlightPriority.Add(NAME_SniperHeadshotKills, 1.f);
	HighlightPriority.Add(NAME_AirRox, 1.f);
	HighlightPriority.Add(NAME_FlakShreds, 1.f);
	HighlightPriority.Add(NAME_AirSnot, 1.f);
	HighlightPriority.Add(NAME_MultiKillLevel0, 0.5f);
	HighlightPriority.Add(NAME_MultiKillLevel1, 0.5f);
	HighlightPriority.Add(NAME_MultiKillLevel2, 1.5f);
	HighlightPriority.Add(NAME_MultiKillLevel3, 2.5f);
	HighlightPriority.Add(NAME_SpreeKillLevel0, 2.f);
	HighlightPriority.Add(NAME_SpreeKillLevel1, 2.5f);
	HighlightPriority.Add(NAME_SpreeKillLevel2, 3.f);
	HighlightPriority.Add(NAME_SpreeKillLevel3, 3.5f);
	HighlightPriority.Add(NAME_SpreeKillLevel4, 4.f);
	HighlightPriority.Add(HighlightNames::ParticipationAward, 0.1f);
}

void AUTGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AUTGameState, RemainingMinute);
	DOREPLIFETIME(AUTGameState, WinnerPlayerState);
	DOREPLIFETIME(AUTGameState, WinningTeam);
	DOREPLIFETIME(AUTGameState, bStopGameClock);
	DOREPLIFETIME(AUTGameState, TimeLimit);  
	DOREPLIFETIME(AUTGameState, RespawnWaitTime);  
	DOREPLIFETIME_CONDITION(AUTGameState, ForceRespawnTime, COND_InitialOnly);  
	DOREPLIFETIME_CONDITION(AUTGameState, bTeamGame, COND_InitialOnly);  
	DOREPLIFETIME(AUTGameState, TeamSwapSidesOffset);
	DOREPLIFETIME_CONDITION(AUTGameState, bIsInstanceServer, COND_InitialOnly);
	DOREPLIFETIME(AUTGameState, PlayersNeeded);  
	DOREPLIFETIME(AUTGameState, AvailableLoadout);
	DOREPLIFETIME(AUTGameState, HubGuid);

	DOREPLIFETIME_CONDITION(AUTGameState, bAllowTeamSwitches, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTGameState, bWeaponStay, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTGameState, GoalScore, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTGameState, RemainingTime, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTGameState, OverlayEffects, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTGameState, OverlayEffects1P, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTGameState, SpawnProtectionTime, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTGameState, NumTeams, COND_InitialOnly);

	DOREPLIFETIME_CONDITION(AUTGameState, ServerName, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTGameState, ServerDescription, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTGameState, ServerMOTD, COND_InitialOnly);

	DOREPLIFETIME(AUTGameState, ServerSessionId);
	DOREPLIFETIME(AUTGameState, NumWinnersToShow);

	DOREPLIFETIME(AUTGameState, MapVoteList);
	DOREPLIFETIME(AUTGameState, VoteTimer);

	DOREPLIFETIME_CONDITION(AUTGameState, bCasterControl, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTGameState, bPlayPlayerIntro, COND_InitialOnly);
	DOREPLIFETIME(AUTGameState, bForcedBalance);
}

void AUTGameState::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	NumTeams = Teams.Num();
	Super::PreReplication(ChangedPropertyTracker);
}

void AUTGameState::AddOverlayMaterial(UMaterialInterface* NewOverlay, UMaterialInterface* NewOverlay1P)
{
	AddOverlayEffect(FOverlayEffect(NewOverlay), FOverlayEffect(NewOverlay1P));
}

void AUTGameState::AddOverlayEffect(const FOverlayEffect& NewOverlay, const FOverlayEffect& NewOverlay1P)
{
	if (NewOverlay.IsValid() && Role == ROLE_Authority)
	{
		if (NetUpdateTime > 0.0f)
		{
			UE_LOG(UT, Warning, TEXT("UTGameState::AddOverlayMaterial() called after startup; may not take effect on clients"));
		}
		for (int32 i = 0; i < ARRAY_COUNT(OverlayEffects); i++)
		{
			if (OverlayEffects[i] == NewOverlay)
			{
				OverlayEffects1P[i] = NewOverlay1P;
				return;
			}
			else if (!OverlayEffects[i].IsValid())
			{
				OverlayEffects[i] = NewOverlay;
				OverlayEffects1P[i] = NewOverlay1P;
				return;
			}
		}
		UE_LOG(UT, Warning, TEXT("UTGameState::AddOverlayMaterial(): Ran out of slots, couldn't add %s"), *NewOverlay.ToString());
	}
}

void AUTGameState::OnRep_OverlayEffects()
{
	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		AUTCharacter* UTC = Cast<AUTCharacter>(It->Get());
		if (UTC != NULL)
		{
			UTC->UpdateCharOverlays();
			UTC->UpdateWeaponOverlays();
		}
	}
}

void AUTGameState::BeginPlay()
{
	Super::BeginPlay();

	// HACK: temporary hack around config property replication bug; force to be different from defaults
	ServerName += TEXT(" ");
	ServerMOTD += TEXT(" ");

	if (GetNetMode() == NM_Client)
	{
		// hook up any TeamInfos that were received prior
		for (FActorIterator It(GetWorld()); It; ++It)
		{
			AUTTeamInfo* Team = Cast<AUTTeamInfo>(*It);
			if (Team != NULL)
			{
				Team->ReceivedTeamIndex();
			}
		}
	}
	else
	{
		TArray<UObject*> AllCharacters;
		GetObjectsOfClass(AUTCharacter::StaticClass(), AllCharacters, true, RF_NoFlags);
		for (int32 i = 0; i < AllCharacters.Num(); i++)
		{
			if (AllCharacters[i]->HasAnyFlags(RF_ClassDefaultObject))
			{
				checkSlow(AllCharacters[i]->IsA(AUTCharacter::StaticClass()));
				AddOverlayMaterial(((AUTCharacter*)AllCharacters[i])->TacComOverlayMaterial);
			}
		}

		TArray<UObject*> AllInventory;
		GetObjectsOfClass(AUTInventory::StaticClass(), AllInventory, true, RF_NoFlags);
		for (int32 i = 0; i < AllInventory.Num(); i++)
		{
			if (AllInventory[i]->HasAnyFlags(RF_ClassDefaultObject))
			{
				checkSlow(AllInventory[i]->IsA(AUTInventory::StaticClass()));
				((AUTInventory*)AllInventory[i])->AddOverlayMaterials(this);
			}
		}
	}
}

float AUTGameState::GetClockTime()
{
	if (IsMatchInOvertime())
	{
		return ElapsedTime-TimeLimit;
	}
	return (TimeLimit > 0.f) ? RemainingTime : ElapsedTime;
}

void AUTGameState::OnRep_RemainingTime()
{
	// if we received RemainingTime, it takes precedence
	// note that this relies on all variables being received prior to any notifies being called
	RemainingMinute = 0;
}

void AUTGameState::DefaultTimer()
{
	Super::DefaultTimer();
	if (IsMatchAtHalftime())
	{
		// no elapsed time - it was incremented in super
		ElapsedTime--;
	}
	else if (IsMatchInProgress())
	{
		for (int32 i = 0; i < PlayerArray.Num(); i++)
		{
			AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerArray[i]);
			if (PS)
			{
				PS->ElapsedTime++;
			}
		}
	}

	if (GetWorld()->GetNetMode() == NM_Client)
	{
		if (RemainingMinute > 0)
		{
			RemainingTime = RemainingMinute;
			RemainingMinute = 0;
		}

		// might have been deferred while waiting for teams to replicate
		if (TeamSwapSidesOffset != PrevTeamSwapSidesOffset)
		{
			OnTeamSideSwap();
		}
	}

	if ((RemainingTime > 0) && !bStopGameClock && TimeLimit > 0)
	{
		if (IsMatchInProgress())
		{
			RemainingTime--;
			if (GetWorld()->GetNetMode() != NM_Client)
			{
				int32 RepTimeInterval = (RemainingTime > 60) ? 60 : 12;
				if (RemainingTime % RepTimeInterval == 0)
				{
					RemainingMinute = RemainingTime;
				}
			}
		}
		else if (!HasMatchStarted())
		{
			AUTGameMode* Game = GetWorld()->GetAuthGameMode<AUTGameMode>();
			if (Game == NULL || Game->NumPlayers + Game->NumBots > Game->MinPlayersToStart)
			{
				RemainingTime--;
				if (GetWorld()->GetNetMode() != NM_Client)
				{
					// during pre-match bandwidth isn't at a premium, let's be accurate
					RemainingMinute = RemainingTime;
				}
			}
		}

		CheckTimerMessage();
	}
}

void AUTGameState::CheckTimerMessage()
{
	if (GetWorld()->GetNetMode() != NM_DedicatedServer && IsMatchInProgress())
	{
		int32 TimerMessageIndex = -1;
		switch (RemainingTime)
		{
			case 300: TimerMessageIndex = 13; break;		// 5 mins remain
			case 180: TimerMessageIndex = 12; break;		// 3 mins remain
			case 60: TimerMessageIndex = 11; break;		// 1 min remains
			case 30: TimerMessageIndex = 10; break;		// 30 seconds remain
			default:
				if (RemainingTime <= 10)
				{
					TimerMessageIndex = RemainingTime - 1;
				}
				break;
		}

		if (TimerMessageIndex >= 0)
		{
			for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
			{
				AUTPlayerController* PC = Cast<AUTPlayerController>(It->PlayerController);
				if (PC != NULL)
				{
					PC->ClientReceiveLocalizedMessage(UUTTimerMessage::StaticClass(), TimerMessageIndex);
				}
			}
		}
	}
}

bool AUTGameState::OnSameTeam(const AActor* Actor1, const AActor* Actor2)
{
	const IUTTeamInterface* TeamInterface1 = Cast<IUTTeamInterface>(Actor1);
	const IUTTeamInterface* TeamInterface2 = Cast<IUTTeamInterface>(Actor2);
	if (TeamInterface1 == NULL || TeamInterface2 == NULL)
	{
		return false;
	}
	else if (TeamInterface1->IsFriendlyToAll() || TeamInterface2->IsFriendlyToAll())
	{
		return true;
	}
	else
	{
		uint8 TeamNum1 = TeamInterface1->GetTeamNum();
		uint8 TeamNum2 = TeamInterface2->GetTeamNum();

		if (TeamNum1 == 255 || TeamNum2 == 255)
		{
			return false;
		}
		else
		{
			return TeamNum1 == TeamNum2;
		}
	}
}

void AUTGameState::ChangeTeamSides(uint8 Offset)
{
	TeamSwapSidesOffset += Offset;
	OnTeamSideSwap();
}

void AUTGameState::OnTeamSideSwap()
{
	if (TeamSwapSidesOffset != PrevTeamSwapSidesOffset && Teams.Num() > 0 && (Role == ROLE_Authority || Teams.Num() == NumTeams))
	{
		uint8 TotalOffset;
		if (TeamSwapSidesOffset < PrevTeamSwapSidesOffset)
		{
			// rollover
			TotalOffset = uint8(uint32(TeamSwapSidesOffset + 255) - uint32(PrevTeamSwapSidesOffset));
		}
		else
		{
			TotalOffset = TeamSwapSidesOffset - PrevTeamSwapSidesOffset;
		}
		for (FActorIterator It(GetWorld()); It; ++It)
		{
			IUTTeamInterface* TeamObj = Cast<IUTTeamInterface>(*It);
			if (TeamObj != NULL)
			{
				uint8 Team = TeamObj->GetTeamNum();
				if (Team != 255)
				{
					TeamObj->Execute_SetTeamForSideSwap(*It, (Team + TotalOffset) % Teams.Num());
				}
			}
			// check for script interface
			else if (It->GetClass()->ImplementsInterface(UUTTeamInterface::StaticClass()))
			{
				// a little hackery to ignore if relevant functions haven't been implemented
				static FName NAME_ScriptGetTeamNum(TEXT("ScriptGetTeamNum"));
				UFunction* GetTeamNumFunc = It->GetClass()->FindFunctionByName(NAME_ScriptGetTeamNum);
				if (GetTeamNumFunc != NULL && GetTeamNumFunc->Script.Num() > 0)
				{
					uint8 Team = IUTTeamInterface::Execute_ScriptGetTeamNum(*It);
					if (Team != 255)
					{
						IUTTeamInterface::Execute_SetTeamForSideSwap(*It, (Team + TotalOffset) % Teams.Num());
					}
				}
				else
				{
					static FName NAME_SetTeamForSideSwap(TEXT("SetTeamForSideSwap"));
					UFunction* SwapFunc = It->GetClass()->FindFunctionByName(NAME_SetTeamForSideSwap);
					if (SwapFunc != NULL && SwapFunc->Script.Num() > 0)
					{
						UE_LOG(UT, Warning, TEXT("Unable to execute SetTeamForSideSwap() for %s because GetTeamNum() must also be implemented"), *It->GetName());
					}
				}
			}
		}
		if (Role == ROLE_Authority)
		{
			// re-initialize all AI squads, in case objectives have changed sides
			for (AUTTeamInfo* Team : Teams)
			{
				Team->ReinitSquads();
			}
		}

		TeamSideSwapDelegate.Broadcast(TotalOffset);
		PrevTeamSwapSidesOffset = TeamSwapSidesOffset;
	}
}

void AUTGameState::SetTimeLimit(int32 NewTimeLimit)
{
	TimeLimit = NewTimeLimit;
	RemainingTime = TimeLimit;
	RemainingMinute = TimeLimit;

	ForceNetUpdate();
}

void AUTGameState::SetGoalScore(int32 NewGoalScore)
{
	GoalScore = NewGoalScore;
	ForceNetUpdate();
}

void AUTGameState::SetWinner(AUTPlayerState* NewWinner)
{
	WinnerPlayerState = NewWinner;
	WinningTeam	= NewWinner != NULL ?  NewWinner->Team : 0;
	ForceNetUpdate();
}

/** Returns true if P1 should be sorted before P2.  */
bool AUTGameState::InOrder( AUTPlayerState* P1, AUTPlayerState* P2 )
{
	// spectators are sorted last
    if( P1->bOnlySpectator )
    {
		return P2->bOnlySpectator;
    }
    else if ( P2->bOnlySpectator )
	{
		return true;
	}

	// sort by Score
    if( P1->Score < P2->Score )
	{
		return false;
	}
    if( P1->Score == P2->Score )
    {
		// if score tied, use deaths to sort
		if ( P1->Deaths > P2->Deaths )
			return false;

		// keep local player highest on list
		if ( (P1->Deaths == P2->Deaths) && (Cast<APlayerController>(P2->GetOwner()) != NULL) )
		{
			ULocalPlayer* LP2 = Cast<ULocalPlayer>(Cast<APlayerController>(P2->GetOwner())->Player);
			if ( LP2 != NULL )
			{
				// make sure ordering is consistent for splitscreen players
				ULocalPlayer* LP1 = Cast<ULocalPlayer>(Cast<APlayerController>(P2->GetOwner())->Player);
				return ( LP1 != NULL );
			}
		}
	}
    return true;
}

/** Sort the PRI Array based on InOrder() prioritization.  */
void AUTGameState::SortPRIArray()
{
	for (int32 i=0; i<PlayerArray.Num()-1; i++)
	{
		AUTPlayerState* P1 = Cast<AUTPlayerState>(PlayerArray[i]);
		for (int32 j=i+1; j<PlayerArray.Num(); j++)
		{
			AUTPlayerState* P2 = Cast<AUTPlayerState>(PlayerArray[j]);
			if( !InOrder( P1, P2 ) )
			{
				PlayerArray[i] = P2;
				PlayerArray[j] = P1;
				P1 = P2;
			}
		}
	}
}

bool AUTGameState::IsMatchInCountdown() const
{
	return GetMatchState() == MatchState::CountdownToBegin;
}

bool AUTGameState::HasMatchStarted() const
{
	return Super::HasMatchStarted() && GetMatchState() != MatchState::CountdownToBegin && GetMatchState() != MatchState::PlayerIntro;
}

bool AUTGameState::IsMatchInProgress() const
{
	FName MatchState = GetMatchState();
	return (MatchState == MatchState::InProgress || MatchState == MatchState::MatchIsInOvertime);
}

bool AUTGameState::IsMatchAtHalftime() const
{	
	return false;	
}

bool AUTGameState::IsMatchInOvertime() const
{
	FName MatchState = GetMatchState();
	if (MatchState == MatchState::MatchEnteringOvertime || MatchState == MatchState::MatchIsInOvertime)
	{
		return true;
	}

	return false;
}

bool AUTGameState::IsMatchIntermission() const
{
	return GetMatchState() == MatchState::MatchIntermission;
}

void AUTGameState::OnWinnerReceived()
{
}

FName AUTGameState::OverrideCameraStyle(APlayerController* PCOwner, FName CurrentCameraStyle)
{
	if (HasMatchEnded())
	{
		return FName(TEXT("FreeCam"));
	}
	// FIXME: shouldn't this come from the Pawn?
	else if (Cast<AUTRemoteRedeemer>(PCOwner->GetPawn()) != nullptr)
	{
		return FName(TEXT("FirstPerson"));
	}
	else
	{
		return CurrentCameraStyle;
	}
}

FText AUTGameState::ServerRules()
{
	if (GameModeClass != NULL && GameModeClass->IsChildOf(AUTGameMode::StaticClass()))
	{
		return GameModeClass->GetDefaultObject<AUTGameMode>()->BuildServerRules(this);
	}
	else
	{
		return FText();
	}
}

void AUTGameState::ReceivedGameModeClass()
{
	Super::ReceivedGameModeClass();

	TSubclassOf<AUTGameMode> UTGameClass(*GameModeClass);
	if (UTGameClass != NULL)
	{
		// precache announcements
		for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
		{
			AUTPlayerController* UTPC = Cast<AUTPlayerController>(It->PlayerController);
			if (UTPC != NULL && UTPC->Announcer != NULL)
			{
				UTGameClass.GetDefaultObject()->PrecacheAnnouncements(UTPC->Announcer);
			}
		}
	}
}

FText AUTGameState::GetGameStatusText()
{
	if (!IsMatchInProgress())
	{
		if (HasMatchEnded())
		{
			return NSLOCTEXT("UTGameState", "PostGame", "Game Over");
		}
		else if (GetMatchState() == MatchState::MapVoteHappening)
		{
			return NSLOCTEXT("UTGameState", "Mapvote", "Map Vote");
		}
		else
		{
			return NSLOCTEXT("UTGameState", "PreGame", "Pre-Game");
		}
	}

	return FText::GetEmpty();
}

void AUTGameState::OnRep_MatchState()
{
	Super::OnRep_MatchState();

	for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
	{
		if (It->PlayerController != NULL)
		{
			AUTHUD* Hud = Cast<AUTHUD>(It->PlayerController->MyHUD);
			if (Hud != NULL)
			{
				Hud->NotifyMatchStateChange();
			}
		}
	}
}

// By default, do nothing.  
void AUTGameState::OnRep_ServerName()
{
}

// By default, do nothing.  
void AUTGameState::OnRep_ServerMOTD()
{
}

void AUTGameState::AddPlayerState(APlayerState* PlayerState)
{
	// assign spectating ID to this player
	AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerState);
	// NOTE: in the case of a rejoining player, this function gets called for both the original and new PLayerStates
	//		we will migrate the initially selected ID to avoid unnecessary ID shuffling
	//		and thus need to check here if that has happened and avoid assigning a new one
	if (PS != NULL && !PS->bOnlySpectator && PS->SpectatingID == 0)
	{
		TArray<APlayerState*> PlayerArrayCopy = PlayerArray;
		PlayerArrayCopy.Sort([](const APlayerState& A, const APlayerState& B) -> bool
		{
			if (Cast<AUTPlayerState>(&A) == NULL)
			{
				return false;
			}
			else if (Cast<AUTPlayerState>(&B) == NULL)
			{
				return true;
			}
			else
			{
				return ((AUTPlayerState*)&A)->SpectatingID < ((AUTPlayerState*)&B)->SpectatingID;
			}
		});
		// find first gap in assignments from player leaving, give it to this player
		// if none found, assign PlayerArray.Num() + 1
		bool bFound = false;
		for (int32 i = 0; i < PlayerArrayCopy.Num(); i++)
		{
			AUTPlayerState* OtherPS = Cast<AUTPlayerState>(PlayerArrayCopy[i]);
			if (OtherPS == NULL || OtherPS->SpectatingID != uint8(i + 1))
			{
				PS->SpectatingID = uint8(i + 1);
				bFound = true;
				break;
			}
		}
		if (!bFound)
		{
			PS->SpectatingID = uint8(PlayerArrayCopy.Num() + 1);
		}
	}

	Super::AddPlayerState(PlayerState);
}

void AUTGameState::CompactSpectatingIDs()
{
	if (Role == ROLE_Authority)
	{
		// get sorted list of UTPlayerStates that have been assigned an ID
		TArray<AUTPlayerState*> PlayerArrayCopy;
		for (APlayerState* PS : PlayerArray)
		{
			AUTPlayerState* UTPS = Cast<AUTPlayerState>(PS);
			if (UTPS != NULL && UTPS->SpectatingID > 0)
			{
				PlayerArrayCopy.Add(UTPS);
			}
		}
		PlayerArrayCopy.Sort([](const AUTPlayerState& A, const AUTPlayerState& B) -> bool
		{
			return A.SpectatingID < B.SpectatingID;
		});

		// fill in gaps from IDs at the end of the list
		for (int32 i = 0; i < PlayerArrayCopy.Num(); i++)
		{
			if (PlayerArrayCopy[i]->SpectatingID != uint8(i + 1))
			{
				AUTPlayerState* MovedPS = PlayerArrayCopy.Pop(false);
				MovedPS->SpectatingID = uint8(i + 1);
				PlayerArrayCopy.Insert(MovedPS, i);
			}
		}

		// now do the same for SpectatingIDTeam
		for (AUTTeamInfo* Team : Teams)
		{
			PlayerArrayCopy.Reset();
			const TArray<AController*> Members = Team->GetTeamMembers();
			for (AController* C : Members)
			{
				if (C)
				{
					AUTPlayerState* UTPS = Cast<AUTPlayerState>(C->PlayerState);
					if (UTPS != NULL && UTPS->SpectatingIDTeam)
					{
						PlayerArrayCopy.Add(UTPS);
					}
				}
			}
			PlayerArrayCopy.Sort([](const AUTPlayerState& A, const AUTPlayerState& B) -> bool
			{
				return A.SpectatingIDTeam < B.SpectatingIDTeam;
			});

			for (int32 i = 0; i < PlayerArrayCopy.Num(); i++)
			{
				if (PlayerArrayCopy[i]->SpectatingIDTeam != uint8(i + 1))
				{
					AUTPlayerState* MovedPS = PlayerArrayCopy.Pop(false);
					MovedPS->SpectatingIDTeam = uint8(i + 1);
					PlayerArrayCopy.Insert(MovedPS, i);
				}
			}
		}
	}
}

int32 AUTGameState::GetMaxSpectatingId()
{
	int32 MaxSpectatingID = 0;
	for (int32 i = 0; i<PlayerArray.Num() - 1; i++)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerArray[i]);
		if (PS && (PS->SpectatingID > MaxSpectatingID))
		{
			MaxSpectatingID = PS->SpectatingID;
		}
	}
	return MaxSpectatingID;
}

int32 AUTGameState::GetMaxTeamSpectatingId(int32 TeamNum)
{
	int32 MaxSpectatingID = 0;
	for (int32 i = 0; i<PlayerArray.Num() - 1; i++)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerArray[i]);
		if (PS && (PS->GetTeamNum() == TeamNum) && (PS->SpectatingIDTeam > MaxSpectatingID))
		{
			MaxSpectatingID = PS->SpectatingIDTeam;
		}
	}
	return MaxSpectatingID;

}

void AUTGameState::AddLoadoutItem(const FLoadoutInfo& Item)
{
	FActorSpawnParameters Params;
	Params.Owner = this;
	AUTReplicatedLoadoutInfo* NewLoadoutInfo = GetWorld()->SpawnActor<AUTReplicatedLoadoutInfo>(Params);
	if (NewLoadoutInfo)
	{
		NewLoadoutInfo->ItemClass = Item.ItemClass;	
		NewLoadoutInfo->RoundMask = Item.RoundMask;
		NewLoadoutInfo->CurrentCost = Item.InitialCost;
		NewLoadoutInfo->bDefaultInclude = Item.bDefaultInclude;
		NewLoadoutInfo->bPurchaseOnly = Item.bPurchaseOnly;

		AvailableLoadout.Add(NewLoadoutInfo);
	}
}

void AUTGameState::AdjustLoadoutCost(TSubclassOf<AUTInventory> ItemClass, float NewCost)
{
	for (int32 i=0; i < AvailableLoadout.Num(); i++)
	{
		if (AvailableLoadout[i]->ItemClass == ItemClass)
		{
			AvailableLoadout[i]->CurrentCost = NewCost;
			return;
		}
	}
}

bool AUTGameState::IsTempBanned(const TSharedPtr<const FUniqueNetId>& UniqueId)
{
	for (int32 i=0; i< TempBans.Num(); i++)
	{
		if (*TempBans[i] == *UniqueId)
		{
			return true;
		}
	}
	return false;
}

void AUTGameState::VoteForTempBan(AUTPlayerState* BadGuy, AUTPlayerState* Voter)
{
	AUTGameMode* Game = GetWorld()->GetAuthGameMode<AUTGameMode>();
	if (Game && Game->NumPlayers > 0)
	{
		BadGuy->LogBanRequest(Voter);
		Game->BroadcastLocalized(Voter, UUTGameMessage::StaticClass(), 13, Voter, BadGuy);

		float Perc = (float(BadGuy->CountBanVotes()) / float(Game->NumPlayers)) * 100.0f;
		BadGuy->KickPercent = int8(Perc);
		UE_LOG(UT,Log,TEXT("VoteForTempBan %f %i %i %i"),Perc,BadGuy->KickPercent,BadGuy->CountBanVotes(),Game->NumPlayers);

		if ( Perc >=  KickThreshold )
		{
			AUTPlayerController* PC = Cast<AUTPlayerController>(BadGuy->GetOwner());
			if (PC)
			{
				AUTGameSession* GS = Cast<AUTGameSession>(Game->GameSession);
				if (GS)
				{
					GS->KickPlayer(PC,NSLOCTEXT("UTGameState","TempKickBan","The players on this server have decided you need to leave."));
					TempBans.Add(BadGuy->UniqueId.GetUniqueNetId());				
				}
			}
		}
	}
}

void AUTGameState::GetAvailableGameData(TArray<UClass*>& GameModes, TArray<UClass*>& MutatorList)
{
	UE_LOG(UTLoading, Log, TEXT("Iterate through UClasses"));
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* CurrentClass = (*It);
		// non-native classes are detected by asset search even if they're loaded for consistency
		if (!CurrentClass->HasAnyClassFlags(CLASS_Abstract | CLASS_HideDropDown) && CurrentClass->HasAnyClassFlags(CLASS_Native))
		{
			if (CurrentClass->IsChildOf(AUTGameMode::StaticClass()))
			{
				if (!CurrentClass->GetDefaultObject<AUTGameMode>()->bHideInUI)
				{
					GameModes.Add(CurrentClass);
				}
			}
			else if (CurrentClass->IsChildOf(AUTMutator::StaticClass()) && !CurrentClass->GetDefaultObject<AUTMutator>()->DisplayName.IsEmpty())
			{
				MutatorList.Add(CurrentClass);
			}
		}
	}

	{
		UE_LOG(UTLoading, Log, TEXT("Load Gamemode blueprints"));
		TArray<FAssetData> AssetList;
		GetAllBlueprintAssetData(AUTGameMode::StaticClass(), AssetList);
		for (const FAssetData& Asset : AssetList)
		{
			static FName NAME_GeneratedClass(TEXT("GeneratedClass"));
			const FString* ClassPath = Asset.TagsAndValues.Find(NAME_GeneratedClass);
			if ((ClassPath != NULL) && !ClassPath->IsEmpty())
			{
				UE_LOG(UTLoading, Log, TEXT("load gamemode object classpath %s"), **ClassPath);
				UClass* TestClass = LoadObject<UClass>(NULL, **ClassPath);
				if (TestClass != NULL && !TestClass->HasAnyClassFlags(CLASS_Abstract) && TestClass->IsChildOf(AUTGameMode::StaticClass()) && !TestClass->GetDefaultObject<AUTGameMode>()->bHideInUI)
				{
					GameModes.AddUnique(TestClass);
				}
			}
		}
	}

	{
		UE_LOG(UTLoading, Log, TEXT("Load Mutator blueprints"));
		TArray<FAssetData> AssetList;
		GetAllBlueprintAssetData(AUTMutator::StaticClass(), AssetList);
		for (const FAssetData& Asset : AssetList)
		{
			static FName NAME_GeneratedClass(TEXT("GeneratedClass"));
			const FString* ClassPath = Asset.TagsAndValues.Find(NAME_GeneratedClass);
			if ((ClassPath != NULL) && !ClassPath->IsEmpty())
			{
				UE_LOG(UTLoading, Log, TEXT("load mutator object classpath %s"), **ClassPath);
				UClass* TestClass = LoadObject<UClass>(NULL, **ClassPath);
				if (TestClass != NULL && !TestClass->HasAnyClassFlags(CLASS_Abstract) && TestClass->IsChildOf(AUTMutator::StaticClass()) && !TestClass->GetDefaultObject<AUTMutator>()->DisplayName.IsEmpty())
				{
					MutatorList.AddUnique(TestClass);
				}
			}
		}
	}
}

/**
 *	returns a list of MapAssets give a constrained set of map prefixes.
 **/
void AUTGameState::ScanForMaps(const TArray<FString>& AllowedMapPrefixes, TArray<FAssetData>& MapList)
{
	TArray<FAssetData> MapAssets;
	GetAllAssetData(UWorld::StaticClass(), MapAssets);
	for (const FAssetData& Asset : MapAssets)
	{
		FString MapPackageName = Asset.PackageName.ToString();
		// ignore /Engine/ as those aren't real gameplay maps and make sure expected file is really there
		if ( !MapPackageName.StartsWith(TEXT("/Engine/")) && IFileManager::Get().FileSize(*FPackageName::LongPackageNameToFilename(MapPackageName, FPackageName::GetMapPackageExtension())) > 0 )
		{
			// Look to see if this is allowed.
			bool bMapIsAllowed = AllowedMapPrefixes.Num() == 0;
			for (int32 i=0; i<AllowedMapPrefixes.Num();i++)
			{
				if ( Asset.AssetName.ToString().StartsWith(AllowedMapPrefixes[i] + TEXT("-")) )
				{
					bMapIsAllowed = true;
					break;
				}
			}

			if (bMapIsAllowed)
			{
				MapList.Add(Asset);
			}
		}
	}
}

/**
 *	Creates a replicated map info that represents the data regarding a map.
 **/
AUTReplicatedMapInfo* AUTGameState::CreateMapInfo(const FAssetData& MapAsset)
{
	const FString* Title = MapAsset.TagsAndValues.Find(NAME_MapInfo_Title); 
	const FString* Author = MapAsset.TagsAndValues.Find(NAME_MapInfo_Author);
	const FString* Description = MapAsset.TagsAndValues.Find(NAME_MapInfo_Description);
	const FString* Screenshot = MapAsset.TagsAndValues.Find(NAME_MapInfo_ScreenshotReference);

	const FString* OptimalPlayerCountStr = MapAsset.TagsAndValues.Find(NAME_MapInfo_OptimalPlayerCount);
	int32 OptimalPlayerCount = 6;
	if (OptimalPlayerCountStr != NULL)
	{
		OptimalPlayerCount = FCString::Atoi(**OptimalPlayerCountStr);
	}

	const FString* OptimalTeamPlayerCountStr = MapAsset.TagsAndValues.Find(NAME_MapInfo_OptimalTeamPlayerCount);
	int32 OptimalTeamPlayerCount = 10;
	if (OptimalTeamPlayerCountStr != NULL)
	{
		OptimalTeamPlayerCount = FCString::Atoi(**OptimalTeamPlayerCountStr);
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	AUTReplicatedMapInfo* MapInfo = GetWorld()->SpawnActor<AUTReplicatedMapInfo>(Params);
	if (MapInfo)
	{
		MapInfo->MapPackageName = MapAsset.PackageName.ToString();
		MapInfo->MapAssetName = MapAsset.AssetName.ToString();
		MapInfo->Title = (Title != NULL && !Title->IsEmpty()) ? *Title : *MapAsset.AssetName.ToString();
		MapInfo->Author = (Author != NULL) ? *Author : FString();
		MapInfo->Description = (Description != NULL) ? *Description : FString();
		MapInfo->MapScreenshotReference = (Screenshot != NULL) ? *Screenshot : FString();
		MapInfo->OptimalPlayerCount = OptimalPlayerCount;
		MapInfo->OptimalTeamPlayerCount = OptimalTeamPlayerCount;

		if (Role == ROLE_Authority)
		{
			// Look up it's redirect information if it has any.
			AUTBaseGameMode* BaseGameMode = Cast<AUTBaseGameMode>(GetWorld()->GetAuthGameMode());
			if (BaseGameMode)
			{
				BaseGameMode->FindRedirect(MapInfo->MapPackageName, MapInfo->Redirect);
			}
		}
	}

	return MapInfo;

}

void AUTGameState::CreateMapVoteInfo(const FString& MapPackage,const FString& MapTitle, const FString& MapScreenshotReference)
{
	FActorSpawnParameters Params;
	Params.Owner = this;
	AUTReplicatedMapInfo* MapVoteInfo = GetWorld()->SpawnActor<AUTReplicatedMapInfo>(Params);
	if (MapVoteInfo)
	{
		UE_LOG(UT,Verbose,TEXT("Creating Map Vote for map %s [%s]"), *MapPackage, *MapTitle);

		MapVoteInfo->MapPackageName= MapPackage;
		MapVoteInfo->Title = MapTitle;
		MapVoteInfo->MapScreenshotReference = MapScreenshotReference;
		MapVoteList.Add(MapVoteInfo);
	}
}

void AUTGameState::SortVotes()
{
	for (int32 i=0; i<MapVoteList.Num()-1; i++)
	{
		AUTReplicatedMapInfo* V1 = Cast<AUTReplicatedMapInfo>(MapVoteList[i]);
		for (int32 j=i+1; j<MapVoteList.Num(); j++)
		{
			AUTReplicatedMapInfo* V2 = Cast<AUTReplicatedMapInfo>(MapVoteList[j]);
			if( V2 && (!V1 || (V2->VoteCount > V1->VoteCount)) )
			{
				MapVoteList[i] = V2;
				MapVoteList[j] = V1;
				V1 = V2;
			}
		}
	}
}

bool AUTGameState::GetImportantPickups_Implementation(TArray<AUTPickup*>& PickupList)
{
	for (FActorIterator It(GetWorld()); It; ++It)
	{
		AUTPickup* Pickup = Cast<AUTPickup>(*It);
		AUTPickupInventory* PickupInventory = Cast<AUTPickupInventory>(*It);

		if ((PickupInventory && PickupInventory->GetInventoryType() && PickupInventory->GetInventoryType()->GetDefaultObject<AUTInventory>()->bShowPowerupTimer
			&& ((PickupInventory->GetInventoryType()->GetDefaultObject<AUTInventory>()->HUDIcon.Texture != NULL) || PickupInventory->GetInventoryType()->IsChildOf(AUTArmor::StaticClass())))
			|| (Pickup && Pickup->bDelayedSpawn && Pickup->RespawnTime >= 0.0f && Pickup->HUDIcon.Texture != nullptr))
		{
			if (!Pickup->bOverride_TeamSide)
			{
				Pickup->TeamSide = NearestTeamSide(Pickup);
			}
			PickupList.Add(Pickup);
		}
	}

	//Sort the list by by respawn time 
	//TODO: powerup priority so different armors sort properly
	PickupList.Sort([](const AUTPickup& A, const AUTPickup& B) -> bool
	{
		return A.RespawnTime > B.RespawnTime;
	});

	return true;
}

void AUTGameState::OnRep_ServerSessionId()
{
	UUTGameEngine* GameEngine = Cast<UUTGameEngine>(GEngine);
	if (GameEngine && ServerSessionId != TEXT(""))
	{
		for(auto It = GameEngine->GetLocalPlayerIterator(GetWorld()); It; ++It)
		{
			UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(*It);
			if (LocalPlayer)
			{
				LocalPlayer->VerifyGameSession(ServerSessionId);
			}
		}
	}
}

float AUTGameState::GetStatsValue(FName StatsName)
{
	return StatsData.FindRef(StatsName);
}

void AUTGameState::SetStatsValue(FName StatsName, float NewValue)
{
	LastScoreStatsUpdateTime = GetWorld()->GetTimeSeconds();
	StatsData.Add(StatsName, NewValue);
}

void AUTGameState::ModifyStatsValue(FName StatsName, float Change)
{
	LastScoreStatsUpdateTime = GetWorld()->GetTimeSeconds();
	float CurrentValue = StatsData.FindRef(StatsName);
	StatsData.Add(StatsName, CurrentValue + Change);
}

bool AUTGameState::AreAllPlayersReady()
{
	if (!HasMatchStarted())
	{
		for (int32 i = 0; i < PlayerArray.Num(); i++)
		{
			AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerArray[i]);
			if (PS != NULL && !PS->bOnlySpectator && !PS->bReadyToPlay)
			{
				return false;
			}
		}
	}
	return true;
}

bool AUTGameState::IsAllowedSpawnPoint_Implementation(AUTPlayerState* Chooser, APlayerStart* DesiredStart) const
{
	return true;
}

void AUTGameState::ClearHighlights()
{
	for (int32 i = 0; i < PlayerArray.Num() - 1; i++)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerArray[i]);
		if (PS)
		{
			for (int32 j = 0; j < 5; j++)
			{
				PS->MatchHighlights[j] = NAME_None;
			}
		}
	}
}

void AUTGameState::UpdateMatchHighlights()
{
	ClearHighlights();
	UpdateHighlights();
}

void AUTGameState::UpdateHighlights_Implementation()
{
	// add highlights to each player in order of highlight priority, filling to 5 if possible
	AUTPlayerState* TopScorer[2] = { NULL, NULL };
	AUTPlayerState* MostKills = NULL;
	AUTPlayerState* LeastDeaths = NULL;
	AUTPlayerState* BestKDPS = NULL;
	AUTPlayerState* BestComboPS = NULL;
	AUTPlayerState* MostHeadShotsPS = NULL;
	AUTPlayerState* MostAirRoxPS = NULL;

	//Collect all the weapons
	TArray<AUTWeapon *> StatsWeapons;
	if (StatsWeapons.Num() == 0)
	{
		for (FActorIterator It(GetWorld()); It; ++It)
		{
			AUTPickupWeapon* Pickup = Cast<AUTPickupWeapon>(*It);
			if (Pickup && Pickup->GetInventoryType())
			{
				StatsWeapons.AddUnique(Pickup->GetInventoryType()->GetDefaultObject<AUTWeapon>());
			}
		}
	}

	for (int32 i = 0; i < PlayerArray.Num() - 1; i++)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerArray[i]);
		if (PS && !PS->bOnlySpectator)
		{
			int32 TeamIndex = PS->Team ? PS->Team->TeamIndex : 0;

			// @TODO FIXMESTEVE support tie scores!
			if (PS->Score >(TopScorer[TeamIndex] ? TopScorer[TeamIndex]->Score : 0))
			{
				TopScorer[TeamIndex] = PS;
			}
			if (PS->Kills > (MostKills ? MostKills->Kills : 0))
			{
				MostKills = PS;
			}
			if (!LeastDeaths || (PS->Deaths < LeastDeaths->Deaths))
			{
				LeastDeaths = PS;
			}
			if (PS->Kills > 0)
			{
				if (!BestKDPS)
				{
					BestKDPS = PS;
				}
				else if (PS->Deaths == 0)
				{
					if ((BestKDPS->Deaths > 0) || (PS->Kills > BestKDPS->Kills))
					{
						BestKDPS = PS;
					}
				}
				else if ((BestKDPS->Deaths > 0) && (PS->Kills / PS->Deaths > BestKDPS->Kills / BestKDPS->Deaths))
				{
					BestKDPS = PS;
				}
			}

			//Figure out what weapon killed the most
			PS->FavoriteWeapon = nullptr;
			int32 BestKills = 0;
			for (AUTWeapon* Weapon : StatsWeapons)
			{
				int32 Kills = Weapon->GetWeaponKillStats(PS);
				if (Kills > BestKills)
				{
					BestKills = Kills;
					PS->FavoriteWeapon = Weapon->GetClass();
				}
			}

			if (PS->GetStatsValue(NAME_BestShockCombo) > (BestComboPS ? BestComboPS->GetStatsValue(NAME_BestShockCombo) : 0.f))
			{
				BestComboPS = PS;
			}
			if (PS->GetStatsValue(NAME_SniperHeadshotKills) > (MostHeadShotsPS ? MostHeadShotsPS->GetStatsValue(NAME_SniperHeadshotKills) : 0.f))
			{
				MostHeadShotsPS = PS;
			}
			if (PS->GetStatsValue(NAME_AirRox) > (MostAirRoxPS ? MostAirRoxPS->GetStatsValue(NAME_AirRox) : 0.f))
			{
				MostAirRoxPS = PS;
			}
		}
	}

	SetTopScorerHighlights(TopScorer[0], TopScorer[1]);
	if (MostKills)
	{
		MostKills->AddMatchHighlight(HighlightNames::MostKills, MostKills->Kills);
	}
	if (LeastDeaths)
	{
		LeastDeaths->AddMatchHighlight(HighlightNames::LeastDeaths, LeastDeaths->Deaths);
	}
	if (BestKDPS)
	{
		BestKDPS->AddMatchHighlight(HighlightNames::BestKD, (BestKDPS->Deaths > 0) ? BestKDPS->Kills/BestKDPS->Deaths : BestKDPS->Kills);
	}
	if (BestComboPS)
	{
		BestComboPS->AddMatchHighlight(HighlightNames::BestCombo, BestComboPS->GetStatsValue(NAME_BestShockCombo));
	}
	if (MostHeadShotsPS)
	{
		MostHeadShotsPS->AddMatchHighlight(HighlightNames::MostHeadShots, MostHeadShotsPS->GetStatsValue(NAME_SniperHeadshotKills));
	}
	if (MostAirRoxPS)
	{
		MostAirRoxPS->AddMatchHighlight(HighlightNames::MostAirRockets, MostAirRoxPS->GetStatsValue(NAME_AirRox));
	}

	for (int32 i = 0; i < PlayerArray.Num() - 1; i++)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerArray[i]);
		if (PS)
		{
			// only add low priority highlights if not enough high priority highlights
			AddMinorHighlights(PS);

			// remove fourth highlight if not major
			if ((PS->MatchHighlights[4] != NAME_None) && (HighlightPriority.FindRef(PS->MatchHighlights[4]) < 2.f))
			{
				PS->MatchHighlights[4] = NAME_None;
				PS->MatchHighlightData[4] = 0.f;
			}

			// remove fifth highlight if not major
			if ((PS->MatchHighlights[4] != NAME_None) && (HighlightPriority.FindRef(PS->MatchHighlights[4]) < 3.f))
			{
				PS->MatchHighlights[4] = NAME_None;
				PS->MatchHighlightData[4] = 0.f;
			}

			if (PS->MatchHighlights[0] == NAME_None)
			{
				PS->MatchHighlights[0] = HighlightNames::ParticipationAward;
			}
		}
	}
}

void AUTGameState::SetTopScorerHighlights(AUTPlayerState* TopScorerRed, AUTPlayerState* TopScorerBlue)
{
	AUTGameMode* Game = GetWorld()->GetAuthGameMode<AUTGameMode>();
	if (Game && (Game->NumPlayers + Game->NumBots) < 3)
	{
		// don't show top scorer highlight if 2 or fewer players
		return;
	}
	if (TopScorerBlue == NULL)
	{
		if (TopScorerRed != NULL)
		{
			TopScorerRed->AddMatchHighlight(HighlightNames::TopScorer, int32(TopScorerRed->Score));
		}
	}
	else if (TopScorerRed == NULL)
	{
		if (TopScorerBlue != NULL)
		{
			TopScorerBlue->AddMatchHighlight(HighlightNames::TopScorer, int32(TopScorerBlue->Score));
		}
	}
	else if (TopScorerBlue->Score == TopScorerRed->Score)
	{
		TopScorerBlue->AddMatchHighlight(HighlightNames::TopScorerBlue, int32(TopScorerBlue->Score));
		TopScorerRed->AddMatchHighlight(HighlightNames::TopScorerRed, int32(TopScorerRed->Score));
	}
	else if (TopScorerBlue->Score > TopScorerRed->Score)
	{
		TopScorerBlue->AddMatchHighlight(HighlightNames::TopScorer, int32(TopScorerBlue->Score));
		TopScorerRed->AddMatchHighlight(HighlightNames::TopScorerRed, int32(TopScorerRed->Score));
	}
	else
	{
		TopScorerRed->AddMatchHighlight(HighlightNames::TopScorer, int32(TopScorerRed->Score));
		TopScorerBlue->AddMatchHighlight(HighlightNames::TopScorerBlue, int32(TopScorerBlue->Score));
	}
}

void AUTGameState::AddMinorHighlights_Implementation(AUTPlayerState* PS)
{
	// skip if already filled with major highlights
	if (PS->MatchHighlights[3] != NAME_None)
	{
		return;
	}

	// sprees and multikills
	FName SpreeStatsNames[5] = { NAME_SpreeKillLevel4, NAME_SpreeKillLevel3, NAME_SpreeKillLevel2, NAME_SpreeKillLevel1, NAME_SpreeKillLevel0 };
	for (int32 i = 0; i < 5; i++)
	{
		if (PS->GetStatsValue(SpreeStatsNames[i]) > 0)
		{
			PS->AddMatchHighlight(SpreeStatsNames[i], PS->GetStatsValue(SpreeStatsNames[i]));
			if (PS->MatchHighlights[3] != NAME_None)
			{
				return;
			}
			break;
		}
	}
	FName MultiKillsNames[4] = { NAME_MultiKillLevel3, NAME_MultiKillLevel2, NAME_MultiKillLevel1, NAME_MultiKillLevel0 };
	for (int32 i = 0; i < 4; i++)
	{
		if (PS->GetStatsValue(MultiKillsNames[i]) > 0)
		{
			PS->AddMatchHighlight(MultiKillsNames[i], PS->GetStatsValue(MultiKillsNames[i]));
			if (PS->MatchHighlights[3] != NAME_None)
			{
				return;
			}
			break;
		}
	}

	// Most kills with favorite weapon, if needed
	if (PS->FavoriteWeapon)
	{
		AUTWeapon* DefaultWeapon = PS->FavoriteWeapon->GetDefaultObject<AUTWeapon>();
		int32 WeaponKills = DefaultWeapon->GetWeaponKillStats(PS);
		bool bIsBestOverall = true;
		for (int32 i = 0; i < PlayerArray.Num() - 1; i++)
		{
			AUTPlayerState* OtherPS = Cast<AUTPlayerState>(PlayerArray[i]);
			if (OtherPS && (PS != OtherPS) && (DefaultWeapon->GetWeaponKillStats(OtherPS) > WeaponKills))
			{
				bIsBestOverall = false;
				break;
			}
		}
		if (bIsBestOverall)
		{
			PS->AddMatchHighlight(HighlightNames::MostWeaponKills, WeaponKills);
			if (PS->MatchHighlights[3] != NAME_None)
			{
				return;
			}
		}
	}

	// announced kills
	FName AnnouncedKills[5] = { NAME_AmazingCombos, NAME_AirRox, NAME_AirSnot, NAME_SniperHeadshotKills, NAME_FlakShreds };
	for (int32 i = 0; i < 5; i++)
	{
		if (PS->GetStatsValue(AnnouncedKills[i]) > 0)
		{
			PS->AddMatchHighlight(AnnouncedKills[i], PS->GetStatsValue(AnnouncedKills[i]));
			if (PS->MatchHighlights[3] != NAME_None)
			{
				return;
			}
		}
	}
}

TArray<FText> AUTGameState::GetPlayerHighlights_Implementation(AUTPlayerState* PS)
{
	TArray<FText> Highlights;
	FText BestWeaponText = PS->FavoriteWeapon ? PS->FavoriteWeapon->GetDefaultObject<AUTWeapon>()->DisplayName : FText::GetEmpty();
	for (int32 i = 0; i < 5; i++)
	{
		if (PS->MatchHighlights[i] != NAME_None)
		{
			Highlights.Add(FText::Format(HighlightMap.FindRef(PS->MatchHighlights[i]), FText::AsNumber(PS->MatchHighlightData[i]), BestWeaponText));
		}
	}
	return Highlights;
}

float AUTGameState::MatchHighlightScore(AUTPlayerState* PS)
{
	float BestHighlightScore = 0.f;
	for (int32 i = 0; i < 5; i++)
	{
		if (PS->MatchHighlights[i] != NAME_None)
		{
			BestHighlightScore = FMath::Max(BestHighlightScore, HighlightPriority.FindRef(PS->MatchHighlights[i]));
		}
	}
	return BestHighlightScore;
}

void AUTGameState::FillOutRconPlayerList(TArray<FRconPlayerData>& PlayerList)
{
	for (int32 i = 0; i < PlayerList.Num(); i++)
	{
		PlayerList[i].bPendingDelete = true;
	}

	for (int32 i = 0; i < PlayerArray.Num(); i++)
	{
		if (PlayerArray[i] && !PlayerArray[i]->IsPendingKillPending())
		{
			APlayerController* PlayerController = Cast<APlayerController>( PlayerArray[i]->GetOwner() );
			if (PlayerController)
			{
				FString PlayerID = PlayerArray[i]->UniqueId.ToString();

				bool bFound = false;
				for (int32 j = 0; j < PlayerList.Num(); j++)
				{
					if (PlayerList[j].PlayerID == PlayerID)
					{
						PlayerList[j].bPendingDelete = false;
						bFound = true;
						break;
					}
				}

				if (!bFound)
				{
					AUTPlayerState* UTPlayerState = Cast<AUTPlayerState>(PlayerArray[i]);
					int32 Rank = UTPlayerState ? UTPlayerState->AverageRank : 0;
					FString PlayerIP = PlayerController->GetPlayerNetworkAddress();
					FRconPlayerData PlayerInfo(PlayerArray[i]->PlayerName, PlayerID, PlayerIP, Rank);
					PlayerList.Add( PlayerInfo );
				}
			}
		}
	}
}

