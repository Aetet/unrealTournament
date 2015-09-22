// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "GameFramework/PlayerState.h"
#include "UTLobbyGameMode.h"
#include "UTLobbyMatchInfo.h"
#include "Net/UnrealNetwork.h"
#include "UTGameEngine.h"
#include "UTLevelSummary.h"
#include "UTReplicatedMapInfo.h"
#include "UTReplicatedGameRuleset.h"


AUTLobbyMatchInfo::AUTLobbyMatchInfo(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer
	.DoNotCreateDefaultSubobject(TEXT("Sprite")))
{
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bAlwaysRelevant = true;
	bReplicateMovement = false;

	// Note: this is very important to set to false. Though all replication infos are spawned at run time, during seamless travel
	// they are held on to and brought over into the new world. In ULevel::InitializeNetworkActors, these PlayerStates may be treated as map/startup actors
	// and given static NetGUIDs. This also causes their deletions to be recorded and sent to new clients, which if unlucky due to name conflicts,
	// may end up deleting the new PlayerStates they had just spaned.
	bNetLoadOnClient = false;

	bSpectatable = true;
	bJoinAnytime = true;
	bMapChanged = false;

	BotSkillLevel = -1;

}


void AUTLobbyMatchInfo::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AUTLobbyMatchInfo, OwnerId);
	DOREPLIFETIME(AUTLobbyMatchInfo, CurrentState);
	DOREPLIFETIME(AUTLobbyMatchInfo, bPrivateMatch);
	DOREPLIFETIME(AUTLobbyMatchInfo, MatchStats);
	DOREPLIFETIME(AUTLobbyMatchInfo, CurrentRuleset);
	DOREPLIFETIME(AUTLobbyMatchInfo, Players);
	DOREPLIFETIME(AUTLobbyMatchInfo, MatchBadge);
	DOREPLIFETIME(AUTLobbyMatchInfo, InitialMap);
	DOREPLIFETIME(AUTLobbyMatchInfo, PlayersInMatchInstance);
	DOREPLIFETIME(AUTLobbyMatchInfo, bJoinAnytime);
	DOREPLIFETIME(AUTLobbyMatchInfo, bSpectatable);
	DOREPLIFETIME(AUTLobbyMatchInfo, bRankLocked);
	DOREPLIFETIME(AUTLobbyMatchInfo, BotSkillLevel);
	DOREPLIFETIME(AUTLobbyMatchInfo, AverageRank);
	DOREPLIFETIME(AUTLobbyMatchInfo, Redirects);
	DOREPLIFETIME(AUTLobbyMatchInfo, AllowedPlayerList);

	DOREPLIFETIME_CONDITION(AUTLobbyMatchInfo, DedicatedServerName, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTLobbyMatchInfo, bDedicatedMatch, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTLobbyMatchInfo, bQuickPlayMatch, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTLobbyMatchInfo, PrivateKey, COND_InitialOnly);
}

void AUTLobbyMatchInfo::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	SetLobbyMatchState(ELobbyMatchState::Initializing);

	UniqueMatchID = FGuid::NewGuid();
	PrivateKey = FGuid::NewGuid();
	MatchBadge = TEXT("Loading...");
}

bool AUTLobbyMatchInfo::CheckLobbyGameState()
{
	LobbyGameState = GetWorld()->GetGameState<AUTLobbyGameState>();
	return LobbyGameState != NULL;
}


void AUTLobbyMatchInfo::SetLobbyMatchState(FName NewMatchState)
{
	if ((CurrentState != ELobbyMatchState::Recycling || NewMatchState == ELobbyMatchState::Dead) && CurrentState != ELobbyMatchState::Dead)
	{
		// When the client receives it's startup info, it will attempt to switch the match's state from Setup to waiting for players
		// but if we are Gamenching due to quickstart we don't want that.
		//if (NewMatchState != ELobbyMatchState::WaitingForPlayers || CurrentState != ELobbyMatchState::Launching)
		//{
			CurrentState = NewMatchState;
			if (CurrentState == ELobbyMatchState::Recycling)
			{
				FTimerHandle TempHandle; 
				GetWorldTimerManager().SetTimer(TempHandle, this, &AUTLobbyMatchInfo::RecycleMatchInfo, 120.0, false);
			}
		//}
	}
}

void AUTLobbyMatchInfo::RecycleMatchInfo()
{
	if (CheckLobbyGameState())
	{
		LobbyGameState->RemoveMatch(this);
	}
}

TArray<int32> AUTLobbyMatchInfo::GetTeamSizes() const
{
	// get the team sizes;
	TArray<int32> TeamSizes;
	if (CurrentRuleset.IsValid() && CurrentRuleset->bTeamGame)
	{
		TeamSizes.SetNumZeroed(2);

		for (int32 i = 0; i < Players.Num(); i++)
		{
			if (Players[i]->DesiredTeamNum >= 0 && Players[i]->DesiredTeamNum < 255)
			{
				TeamSizes.SetNumZeroed(FMath::Max<int32>(TeamSizes.Num(), Players[i]->DesiredTeamNum));
				TeamSizes[Players[i]->DesiredTeamNum]++;
			}
		}
	}
	return TeamSizes;
}

void AUTLobbyMatchInfo::AddPlayer(AUTLobbyPlayerState* PlayerToAdd, bool bIsOwner, bool bIsSpectator)
{
	if (bIsOwner && !OwnerId.IsValid())
	{
		OwnerId = PlayerToAdd->UniqueId;
		SetLobbyMatchState(ELobbyMatchState::Setup);
	}
	else
	{
		// Look to see if this player is already in the match

		for (int32 PlayerIdx = 0; PlayerIdx < Players.Num(); PlayerIdx++)
		{
			if (Players[PlayerIdx].IsValid() && Players[PlayerIdx] == PlayerToAdd)
			{
				return;
			}
		}

		if (IsBanned(PlayerToAdd->UniqueId))
		{
			PlayerToAdd->ClientMatchError(NSLOCTEXT("LobbyMessage","Banned","You do not have permission to enter this match."));
			return;
		}
		
		if (CurrentRuleset.IsValid() && CurrentRuleset->bTeamGame)
		{
			TArray<int32> TeamSizes = GetTeamSizes();
			int32 BestTeam = 0;
			for (int32 i = 1; i < TeamSizes.Num(); i++)
			{
				if (TeamSizes[i] < TeamSizes[BestTeam])
				{
					BestTeam = i;
				}
			}
			PlayerToAdd->DesiredTeamNum = BestTeam;

		}
		else
		{
			PlayerToAdd->DesiredTeamNum = 0;
		}
	}

	if (bIsSpectator)
	{
		PlayerToAdd->DesiredTeamNum = 255;
	}
	
	Players.Add(PlayerToAdd);
	PlayerToAdd->AddedToMatch(this);
	PlayerToAdd->ChatDestination = ChatDestinations::Match;

	// Players default to ready
	PlayerToAdd->bReadyToPlay = true;
	UpdateRank();
}

bool AUTLobbyMatchInfo::RemovePlayer(AUTLobbyPlayerState* PlayerToRemove)
{
	// Owners remove everyone and kill the match
	if (OwnerId == PlayerToRemove->UniqueId)
	{
		// The host is removing this match, notify everyone.
		for (int32 i=0;i<Players.Num();i++)
		{
			if (Players[i].IsValid())
			{
				Players[i]->RemovedFromMatch(this);
			}
		}
		Players.Empty();

		// We are are not launching, kill this lobby otherwise keep it around
		if ( CurrentState == ELobbyMatchState::Launching || !IsInProgress() )
		{
			SetLobbyMatchState(ELobbyMatchState::Dead);
			return true;
		}

		return false;
	}

	// Else just remove this one player
	else
	{
		Players.Remove(PlayerToRemove);
		PlayerToRemove->RemovedFromMatch(this);
		UpdateRank();
	}

	return false;

}

bool AUTLobbyMatchInfo::MatchIsReadyToJoin(AUTLobbyPlayerState* Joiner)
{
	if (Joiner && CheckLobbyGameState())
	{
		if (	CurrentState == ELobbyMatchState::WaitingForPlayers || 
		   (    CurrentState == ELobbyMatchState::Setup && OwnerId == Joiner->UniqueId) ||
		   (	CurrentState == ELobbyMatchState::Launching && (Joiner->CurrentMatch == this || bJoinAnytime || OwnerId == Joiner->UniqueId) )
		   )
		{
			return ( OwnerId.IsValid() );
		}
	}

	return false;
}


FText AUTLobbyMatchInfo::GetActionText()
{
	if (CurrentState == ELobbyMatchState::Dead)
	{
		return NSLOCTEXT("SUMatchPanel","Dead","!! DEAD - BUG !!");
	}
	else if (CurrentState == ELobbyMatchState::Setup)
	{
		return NSLOCTEXT("SUMatchPanel","Setup","Initializing...");
	}
	else if (CurrentState == ELobbyMatchState::WaitingForPlayers)
	{
		if (MatchHasRoom())
		{
			return NSLOCTEXT("SUMatchPanel","ClickToJoin","Click to Join");
		}
		else
		{
			return NSLOCTEXT("SUMatchPanel","Full","Match is Full");
		}
	}
	else if (CurrentState == ELobbyMatchState::Launching)
	{
		return NSLOCTEXT("SUMatchPanel","Launching","Launching...");
	}
	else if (CurrentState == ELobbyMatchState::InProgress)
	{
		if (bJoinAnytime)
		{
			return NSLOCTEXT("SUMatchPanel","ClickToJoin","Click to Join");
		}
		else if (bSpectatable)
		{
			return NSLOCTEXT("SUMatchPanel","Spectate","Click to Spectate");
		}
		else 
		{
			return NSLOCTEXT("SUMatchPanel","InProgress","In Progress...");
		}
	}
	else if (CurrentState == ELobbyMatchState::Returning)
	{
		return NSLOCTEXT("SUMatchPanel","MatchOver","!! Match is over !!");
	}

	return FText::GetEmpty();
}

void AUTLobbyMatchInfo::SetSettings(AUTLobbyGameState* GameState, AUTLobbyMatchInfo* MatchToCopy)
{
	if (MatchToCopy)
	{
		SetRules(MatchToCopy->CurrentRuleset, MatchToCopy->InitialMap);
		BotSkillLevel = MatchToCopy->BotSkillLevel;
	}

	SetLobbyMatchState(ELobbyMatchState::Setup);
}

bool AUTLobbyMatchInfo::ServerMatchIsReadyForPlayers_Validate() { return true; }
void AUTLobbyMatchInfo::ServerMatchIsReadyForPlayers_Implementation()
{
	SetLobbyMatchState(ELobbyMatchState::WaitingForPlayers);
}

bool AUTLobbyMatchInfo::ServerManageUser_Validate(int32 CommandID, AUTLobbyPlayerState* Target){ return true; }
void AUTLobbyMatchInfo::ServerManageUser_Implementation(int32 CommandID, AUTLobbyPlayerState* Target)
{
	// make sure target is in this lobby's player list
	bool bFoundTarget = false;
	if (Target && !Target->IsPendingKillPending())
	{
		for (int32 i = 0; i < Players.Num(); i++)
		{
			if (Target == Players[i])
			{
				bFoundTarget = true;
				break;
			}
		}
	}
	if (!bFoundTarget)
	{
		return;
	}

	// process command
	if (CommandID == 0)
	{
		if (CurrentRuleset->bTeamGame)
		{
			if (Target->DesiredTeamNum != 255)
			{
				Target->DesiredTeamNum = 1 - Target->DesiredTeamNum;
			}
			else
			{
				Target->DesiredTeamNum = 0;
			}
			UE_LOG(UT, Log, TEXT("Changing %s to team %i"), *Target->PlayerName, Target->DesiredTeamNum)
		}
	}
	else if (CommandID == 1)
	{
		Target->DesiredTeamNum = 255;
		UE_LOG(UT,Log,TEXT("Changing %s to spectator"), *Target->PlayerName, Target->DesiredTeamNum)
	}
	else if (CommandID > 1)
	{
		// Right now we only have kicks and bans.
		RemovePlayer(Target);
		Target->UninviteFromMatch(this);
		if (CommandID == 3)
		{
			BannedIDs.Add(Target->UniqueId);
		}
	}
}

bool AUTLobbyMatchInfo::ServerStartMatch_Validate() { return true; }
void AUTLobbyMatchInfo::ServerStartMatch_Implementation()
{
	if (CheckLobbyGameState() && LobbyGameState->CanLaunch())
	{
#if !UE_BUILD_SHIPPING
		if (FParse::Param(FCommandLine::Get(), TEXT("NoMinPlayers")))
		{
			CurrentRuleset->MinPlayersToStart = 1;
		}
#endif
		if (Players.Num() < CurrentRuleset->MinPlayersToStart)
		{
			GetOwnerPlayerState()->ClientMatchError(NSLOCTEXT("LobbyMessage", "NotEnoughPlayers","There are not enough players in the match to start."));
			return;
		}

		if (NumPlayersInMatch() > CurrentRuleset->MaxPlayers)
		{
			GetOwnerPlayerState()->ClientMatchError(NSLOCTEXT("LobbyMessage", "TooManyPlayers","There are too many players in this match to start."));
			return;
		}

		if (NumSpectatorsInMatch() > LobbyGameState->MaxSpectatorsInInstance)
		{
			GetOwnerPlayerState()->ClientMatchError(NSLOCTEXT("LobbyMessage", "TooManySpectators","There are too many spectators in this match to start."));
			return;
		}

		AUTTeamGameMode* TeamGame = Cast<AUTTeamGameMode>(CurrentRuleset->GetDefaultGameModeObject());
		if (TeamGame != NULL)
		{
			bool bBalanceTeams = AUTBaseGameMode::EvalBoolOptions(TeamGame->ParseOption(CurrentRuleset->GameOptions, TEXT("BalanceTeams")), TeamGame->bBalanceTeams);
			if (bBalanceTeams)
			{
				 // don't allow starting if the teams aren't balanced
				TArray<int32> TeamSizes = GetTeamSizes();
				for (int32 i = 0; i < TeamSizes.Num(); i++)
				{
					for (int32 j = 0; j < TeamSizes.Num(); j++)
					{
						if (FMath::Abs<int32>(TeamSizes[i] - TeamSizes[j]) > 1)
						{
							GetOwnerPlayerState()->ClientMatchError(NSLOCTEXT("LobbyMessage", "UnbalancedTeams", "The teams must be balanced to start the match."));
							return;
						}
					}
				}
			}
		}

		// TODO: need to check for ready ups on server side

		if (CurrentState == ELobbyMatchState::WaitingForPlayers)
		{
			LaunchMatch(false, 2);
			return;
		}
	}

	GetOwnerPlayerState()->ClientMatchError(NSLOCTEXT("LobbyMessage", "TooManyInstances","All available game instances are taken.  Please wait a bit and try starting again."));
}

void AUTLobbyMatchInfo::LaunchMatch(bool bQuickPlay, int32 DebugCode)
{
	for (int32 i=0;i<Players.Num();i++)
	{
		Players[i]->bReadyToPlay = true;
	}

	if (CheckLobbyGameState() && CurrentRuleset.IsValid() && InitialMapInfo.IsValid())
	{
		if (bQuickPlay) 
		{
			AUTLobbyGameMode* LobbyGame = Cast<AUTLobbyGameMode>(LobbyGameState->AuthorityGameMode);
			BotSkillLevel = (LobbyGame && LobbyGame->bTrainingGround) ? 1 : 3;
		}

		// build all of the data needed to launch the map.

		FString GameURL = FString::Printf(TEXT("%s?Game=%s?MaxPlayers=%i"),*InitialMap, *CurrentRuleset->GameMode, CurrentRuleset->MaxPlayers);
		GameURL += CurrentRuleset->GameOptions;

		if (!CurrentRuleset->bCustomRuleset)
		{
			// Custom rules already have their bot info set

			int32 OptimalPlayerCount = CurrentRuleset->bTeamGame ? InitialMapInfo->OptimalTeamPlayerCount : InitialMapInfo->OptimalPlayerCount;

			if (BotSkillLevel >= 0)
			{
				GameURL += FString::Printf(TEXT("?BotFill=%i?Difficulty=%i"), OptimalPlayerCount, FMath::Clamp<int32>(BotSkillLevel,0,7));			
			}
			else
			{
				GameURL += TEXT("?BotFill=0");
			}
		}

		if (!bSpectatable) GameURL += TEXT("?MaxSpectators=0");
		if (!bJoinAnytime) GameURL += TEXT("?NoJIP");

		LobbyGameState->LaunchGameInstance(this, GameURL, DebugCode);
	}
}

bool AUTLobbyMatchInfo::ServerAbortMatch_Validate() { return true; }
void AUTLobbyMatchInfo::ServerAbortMatch_Implementation()
{
	if (CheckLobbyGameState())
	{
		LobbyGameState->TerminateGameInstance(this, true);
	}

	SetLobbyMatchState(ELobbyMatchState::WaitingForPlayers);
	TWeakObjectPtr<AUTLobbyPlayerState> OwnerPS = GetOwnerPlayerState();
	if (OwnerPS.IsValid()) OwnerPS->bReadyToPlay = false;
}

void AUTLobbyMatchInfo::GameInstanceReady(FGuid inGameInstanceGUID)
{
	GameInstanceGUID = inGameInstanceGUID.ToString();

	UWorld* World = GetWorld();
	if (World == NULL) return;

	AUTLobbyGameMode* GM = World->GetAuthGameMode<AUTLobbyGameMode>();
	if (GM)
	{
		for (int32 i=0;i<Players.Num();i++)
		{
			if (Players[i].IsValid() && !Players[i]->bPendingKillPending)	//  Just in case.. they shouldn't be here anyway..
			{
				// Tell the client to connect to the instance

				Players[i]->ClientConnectToInstance(GameInstanceGUID, Players[i]->DesiredTeamNum == 255);
			}
		}
	}

	SetLobbyMatchState(ELobbyMatchState::InProgress);

	for (int32 i = 0; i < NotifyBeacons.Num(); i++)
	{
		if (NotifyBeacons[i])
		{
			NotifyBeacons[i]->ClientJoinQuickplay(GameInstanceGUID);
		}
	}

	NotifyBeacons.Empty();
}

void AUTLobbyMatchInfo::RemoveFromMatchInstance(AUTLobbyPlayerState* PlayerState)
{
	for (int32 i=0; i<PlayersInMatchInstance.Num();i++)
	{
		if (PlayersInMatchInstance[i].PlayerID == PlayerState->UniqueId)
		{
			PlayersInMatchInstance.RemoveAt(i);
			break;
		}
	}
}


bool AUTLobbyMatchInfo::IsInProgress()
{
	return CurrentState == ELobbyMatchState::InProgress || CurrentState == ELobbyMatchState::Launching || CurrentState == ELobbyMatchState::Completed;
}

bool AUTLobbyMatchInfo::ShouldShowInDock()
{
	if (bDedicatedMatch)
	{
		// dedicated instances always show unless they are dead
		return (CurrentState != ELobbyMatchState::Aborting && CurrentState != ELobbyMatchState::Dead && CurrentState != ELobbyMatchState::Recycling);
	}
	else
	{
		return (OwnerId.IsValid() || bQuickPlayMatch) && //(Players.Num() > 0 || PlayersInMatchInstance.Num() > 0) && 
				CurrentRuleset.IsValid() && 
				(CurrentState == ELobbyMatchState::InProgress || CurrentState == ELobbyMatchState::Launching || CurrentState == ELobbyMatchState::WaitingForPlayers);
	}
}

bool AUTLobbyMatchInfo::ServerSetLobbyMatchState_Validate(FName NewMatchState) { return true; }
void AUTLobbyMatchInfo::ServerSetLobbyMatchState_Implementation(FName NewMatchState)
{
	SetLobbyMatchState(NewMatchState);
}

bool AUTLobbyMatchInfo::ServerSetAllowJoinInProgress_Validate(bool bAllow) { return true; }
void AUTLobbyMatchInfo::ServerSetAllowJoinInProgress_Implementation(bool bAllow)
{
	bJoinAnytime = bAllow;
}

bool AUTLobbyMatchInfo::ServerSetAllowSpectating_Validate(bool bAllow) { return true; }
void AUTLobbyMatchInfo::ServerSetAllowSpectating_Implementation(bool bAllow)
{
	bSpectatable = bAllow;
}

bool AUTLobbyMatchInfo::ServerSetRankLocked_Validate(bool bLocked) { return true; }
void AUTLobbyMatchInfo::ServerSetRankLocked_Implementation(bool bLocked)
{
	bRankLocked = bLocked;
}

bool AUTLobbyMatchInfo::ServerSetPrivateMatch_Validate(bool bIsPrivate) { return true; }
void AUTLobbyMatchInfo::ServerSetPrivateMatch_Implementation(bool bIsPrivate)
{
	bPrivateMatch = bIsPrivate;
}



FText AUTLobbyMatchInfo::GetDebugInfo()
{

	FText Owner = NSLOCTEXT("UTLobbyMatchInfo","NoOwner","NONE");
	if (OwnerId.IsValid())
	{
		if (Players.Num() > 0 && Players[0].IsValid()) Owner = FText::FromString(Players[0]->PlayerName);
		else Owner = FText::FromString(OwnerId.ToString());
	}


	FFormatNamedArguments Args;
	Args.Add(TEXT("OwnerName"), Owner);
	Args.Add(TEXT("CurrentState"), FText::FromName(CurrentState));
	Args.Add(TEXT("CurrentRuleSet"), FText::FromString(CurrentRuleset.IsValid() ? CurrentRuleset->Title : TEXT("None")));
	Args.Add(TEXT("ShouldShowInDock"), FText::AsNumber(ShouldShowInDock()));
	Args.Add(TEXT("InProgress"), FText::AsNumber(IsInProgress()));
	Args.Add(TEXT("MatchStats"), FText::FromString(MatchStats));


	return FText::Format(NSLOCTEXT("UTLobbyMatchInfo","DebugFormat","Owner [{OwnerName}] State [{CurrentState}] RuleSet [{CurrentRuleSet}] Flags [{ShouldShowInDock}, {InProgress}]  Stats: {MatchStats}"), Args);
}

void AUTLobbyMatchInfo::OnRep_CurrentRuleset()
{
	OnRep_Update();
	OnRulesetUpdatedDelegate.ExecuteIfBound();
}


void AUTLobbyMatchInfo::OnRep_Update()
{
	// Let the UI know
	OnMatchInfoUpdatedDelegate.ExecuteIfBound();
}

void AUTLobbyMatchInfo::OnRep_InitialMap()
{
	bMapChanged = true;
	GetMapInformation();
}

void AUTLobbyMatchInfo::SetRedirects()
{
	// Copy any required redirects in to match info.  The UI will pickup on the replication and pull them.
	AUTBaseGameMode* BaseGame = Cast<AUTBaseGameMode>(GetWorld()->GetAuthGameMode());
	if (BaseGame != NULL)
	{
		Redirects.Empty();
		for (int32 i = 0; i < CurrentRuleset->RequiredPackages.Num(); i++)
		{
			FPackageRedirectReference Redirect;
			if (BaseGame->FindRedirect(CurrentRuleset->RequiredPackages[i], Redirect))
			{
				Redirects.Add(Redirect);
			}
		}
		// automatically add redirects for the map, game mode and mutator pak files (if any)
		FPackageRedirectReference Redirect;
		FString MapFullName;
		if (FPackageName::SearchForPackageOnDisk(InitialMap + FPackageName::GetMapPackageExtension(), &MapFullName) && BaseGame->FindRedirect(GetModPakFilenameFromPkg(MapFullName), Redirect))
		{
			Redirects.Add(Redirect);
		}
		if (BaseGame->FindRedirect(GetModPakFilenameFromPath(CurrentRuleset->GameMode), Redirect))
		{
			Redirects.Add(Redirect);
		}
		FString AllMutators = BaseGame->ParseOption(CurrentRuleset->GameOptions, TEXT("Mutator"));
		while (AllMutators.Len() > 0)
		{
			FString MutPath;
			int32 Pos = AllMutators.Find(TEXT(","));
			if (Pos > 0)
			{
				MutPath = AllMutators.Left(Pos);
				AllMutators = AllMutators.Right(AllMutators.Len() - Pos - 1);
			}
			else
			{
				MutPath = AllMutators;
				AllMutators.Empty();
			}
			if (BaseGame->FindRedirect(GetModPakFilenameFromPath(MutPath), Redirect))
			{
				Redirects.Add(Redirect);
			}
		}

		if (InitialMapInfo.IsValid() && InitialMapInfo->Redirect.PackageName != TEXT(""))
		{
			Redirects.Add(InitialMapInfo->Redirect);
		}
	}
}

void AUTLobbyMatchInfo::AssignTeams()
{
	if (CurrentRuleset.IsValid())
	{
		for (int32 i = 0 ; i < Players.Num(); i++)
		{
			if (!Players[i]->bIsSpectator)
			{
				if (CurrentRuleset->bTeamGame)
				{
					Players[i]->DesiredTeamNum = i % 2;
				}
				else 
				{
					Players[i]->DesiredTeamNum = 0;
				}
			}
		}
	}
}

void AUTLobbyMatchInfo::SetRules(TWeakObjectPtr<AUTReplicatedGameRuleset> NewRuleset, const FString& StartingMap)
{
	bool bOldTeamGame = CurrentRuleset.IsValid() ? CurrentRuleset->bTeamGame : false;
	CurrentRuleset = NewRuleset;

	if (bOldTeamGame != CurrentRuleset->bTeamGame) AssignTeams();

	InitialMap = StartingMap;
	GetMapInformation();

	SetRedirects();

	bMapChanged = true;
}

bool AUTLobbyMatchInfo::ServerSetRules_Validate(const FString& RulesetTag, const FString& StartingMap,int32 NewBotSkillLevel) { return true; }
void AUTLobbyMatchInfo::ServerSetRules_Implementation(const FString&RulesetTag, const FString& StartingMap,int32 NewBotSkillLevel)
{
	if ( CheckLobbyGameState() )
	{
		TWeakObjectPtr<AUTReplicatedGameRuleset> NewRuleSet = LobbyGameState->FindRuleset(RulesetTag);

		if (NewRuleSet.IsValid())
		{
			SetRules(NewRuleSet, StartingMap);
			if (!InitialMapInfo.IsValid())
			{
				MatchBadge = FString::Printf(TEXT("Setting up a %s match."), *NewRuleSet->Title);
			}
			else
			{
				MatchBadge = FString::Printf(TEXT("Setting up a %s match on %s."), *NewRuleSet->Title, *InitialMapInfo->Title);
			
			}
		}

		BotSkillLevel = NewBotSkillLevel;

	}
}

void AUTLobbyMatchInfo::SetMatchStats(FString Update)
{
	MatchStats = Update;
	OnRep_MatchStats();
}

void AUTLobbyMatchInfo::OnRep_MatchStats()
{
	int32 GameTime;
	if ( FParse::Value(*MatchStats, TEXT("GameTime="), GameTime) )
	{
		MatchGameTime = GameTime;
	}
}

bool AUTLobbyMatchInfo::ServerCreateCustomRule_Validate(const FString& GameMode, const FString& StartingMap, const FString& Description, const TArray<FString>& GameOptions, int32 DesiredSkillLevel, int32 DesiredPlayerCount, bool bTeamGame) { return true; }
void AUTLobbyMatchInfo::ServerCreateCustomRule_Implementation(const FString& GameMode, const FString& StartingMap, const FString& Description, const TArray<FString>& GameOptions, int32 DesiredSkillLevel, int32 DesiredPlayerCount, bool bTeamGame)
{
	bool bOldTeamGame = CurrentRuleset.IsValid() ? CurrentRuleset->bTeamGame : false;

	// We need to build a one off custom replicated ruleset just for this hub.  :)
	AUTLobbyGameState* GameState = GetWorld()->GetGameState<AUTLobbyGameState>();

	FActorSpawnParameters Params;
	Params.Owner = GameState;
	AUTReplicatedGameRuleset* NewReplicatedRuleset = GetWorld()->SpawnActor<AUTReplicatedGameRuleset>(Params);
	if (NewReplicatedRuleset)
	{
		// Look up the game mode in the AllowedGameData array and get the text description.
		NewReplicatedRuleset->Title = TEXT("Custom Rule");
		NewReplicatedRuleset->Tooltip = TEXT("");
		NewReplicatedRuleset->Description = Description;
		int32 PlayerCount = 20;

		if (DesiredSkillLevel >= 0)
		{
			//NewReplicatedRuleset->
		}

		NewReplicatedRuleset->GameMode = GameMode;

		FString FinalGameOptions = TEXT("");

		AUTGameMode* CustomGameModeDefaultObject = NewReplicatedRuleset->GetDefaultGameModeObject();
		if (CustomGameModeDefaultObject)
		{
			NewReplicatedRuleset->Title = FString::Printf(TEXT("Custom %s"), *CustomGameModeDefaultObject->DisplayName.ToString());

			TArray< TSharedPtr<TAttributePropertyBase> > AllowedProps;
			CustomGameModeDefaultObject->CreateGameURLOptions(AllowedProps);

			for (int32 i=0; i<GameOptions.Num();i++)
			{
				if (!GameOptions[i].IsEmpty())
				{
					FString Sanitized = GameOptions[i].Replace(TEXT(" "), TEXT(""));
					Sanitized = Sanitized.Replace(TEXT("?"),TEXT(""));
					Sanitized = Sanitized.Replace(TEXT("?"),TEXT(""));
					Sanitized = Sanitized.Replace(TEXT("|"),TEXT(""));
					Sanitized = Sanitized.Replace(TEXT(";"),TEXT(""));
					Sanitized = Sanitized.Replace(TEXT("<"),TEXT(""));
					Sanitized = Sanitized.Replace(TEXT(">"),TEXT(""));
					TArray<FString> Split;
					Sanitized.ParseIntoArray(Split, TEXT("="),true);
					if (Split.Num() == 2)
					{
						// TODO: this doesn't handle mutators, etc
						//TSharedPtr<TAttributePropertyBase> Prop = CustomGameModeDefaultObject->FindGameURLOption(AllowedProps, Split[0]);
						//if (Prop.IsValid())
						{
							FinalGameOptions += TEXT("?") + Sanitized;
						}
					}
				}
			}
		}

		int32 OptimalPlayerCount = 4;

		InitialMap = StartingMap;
		GetMapInformation();

		if (InitialMapInfo.IsValid())
		{
			OptimalPlayerCount = ( CustomGameModeDefaultObject && CustomGameModeDefaultObject->bTeamGame) ? InitialMapInfo->OptimalTeamPlayerCount : InitialMapInfo->OptimalPlayerCount;
		}

		NewReplicatedRuleset->MaxPlayers = DesiredPlayerCount > 0 ? DesiredPlayerCount : OptimalPlayerCount;
		if (DesiredSkillLevel >= 0)
		{
			FinalGameOptions += FString::Printf(TEXT("?BotFill=%i?Difficulty=%i"), NewReplicatedRuleset->MaxPlayers, FMath::Clamp<int32>(DesiredSkillLevel,0,7));				
		}
		else
		{
			FinalGameOptions += TEXT("?BotFill=0");
		}
		NewReplicatedRuleset->GameOptions = FinalGameOptions;
		NewReplicatedRuleset->MinPlayersToStart = 2;
		NewReplicatedRuleset->DisplayTexture = "Texture2D'/Game/RestrictedAssets/UI/GameModeBadges/GB_Custom.GB_Custom'";
		NewReplicatedRuleset->bCustomRuleset = true;

		// Add code to setup the required packages array
		CurrentRuleset = NewReplicatedRuleset;
		NewReplicatedRuleset->bTeamGame = bTeamGame;

		if (CurrentRuleset->bTeamGame != bOldTeamGame) AssignTeams();


		if (!InitialMapInfo.IsValid())
		{
			MatchBadge = FString::Printf(TEXT("Setting up a custom match."));
		}
		else
		{
			MatchBadge = FString::Printf(TEXT("Setting up a custom match on %s."), *InitialMapInfo->Title);
		}
		MatchBadge = TEXT("Setting up a custom match.");

		SetRedirects();
	}
}

bool AUTLobbyMatchInfo::IsBanned(FUniqueNetIdRepl Who)
{
	for (int32 i=0;i<BannedIDs.Num();i++)
	{
		if (Who == BannedIDs[i])
		{
			return true;
		}
	}

	return false;
}

void AUTLobbyMatchInfo::GetMapInformation()
{
	if ( CheckLobbyGameState() )
	{
		InitialMapInfo = LobbyGameState->GetMapInfo(InitialMap);
		if (InitialMapInfo.IsValid()) return;
	}

	// We need to keep trying this until we get a valid map info.
	FTimerHandle TempHandle;
	GetWorldTimerManager().SetTimer(TempHandle, this, &AUTLobbyMatchInfo::GetMapInformation, 0.25);
}

int32 AUTLobbyMatchInfo::NumPlayersInMatch()
{
	int32 ActualPlayerCount = 0;
	for (int32 i = 0; i < Players.Num(); i++)
	{
		if (Players[i].IsValid() && Players[i]->DesiredTeamNum != 255) ActualPlayerCount++;
	}

	if (CurrentState == ELobbyMatchState::Launching || CurrentState == ELobbyMatchState::WaitingForPlayers)
	{
		return ActualPlayerCount;
	}
	else if (CurrentState == ELobbyMatchState::InProgress)
	{
		int32 Cnt = ActualPlayerCount;
		for (int32 i=0; i < PlayersInMatchInstance.Num(); i++)
		{
			if (!PlayersInMatchInstance[i].bIsSpectator)
			{
				Cnt++;
			}
		}

		return Cnt;
	}
	return 0;
}

int32 AUTLobbyMatchInfo::NumSpectatorsInMatch()
{
	int32 ActualPlayerCount = 0;
	for (int32 i = 0; i < Players.Num(); i++)
	{
		if (Players[i].IsValid() && Players[i]->DesiredTeamNum == 255) ActualPlayerCount++;
	}

	if (CurrentState == ELobbyMatchState::Launching || CurrentState == ELobbyMatchState::WaitingForPlayers)
	{
		return ActualPlayerCount;
	}
	else if (CurrentState == ELobbyMatchState::InProgress)
	{
		int32 Cnt = Players.Num();
		for (int32 i=0; i < PlayersInMatchInstance.Num(); i++)
		{
			if (PlayersInMatchInstance[i].bIsSpectator)
			{
				Cnt++;
			}
		}

		return Cnt;
	}

	return 0;
}


bool AUTLobbyMatchInfo::IsMatchofType(const FString& MatchType)
{
	if (CurrentRuleset.IsValid() && !CurrentRuleset->bCustomRuleset)
	{
		if (MatchType == CurrentRuleset->UniqueTag)
		{
			return true;
		}
	}

	return false;
}

bool AUTLobbyMatchInfo::MatchHasRoom(bool bForSpectator)
{
	if (CurrentRuleset.IsValid())
	{
		if (CurrentState == MatchState::InProgress)	
		{
			if (bForSpectator && CheckLobbyGameState())
			{
				return NumSpectatorsInMatch() < LobbyGameState->MaxSpectatorsInInstance;
			}

			return NumPlayersInMatch() < CurrentRuleset->MaxPlayers;
		}
	
	}
	return true;
}

bool AUTLobbyMatchInfo::SkillTest(int32 Rank, bool bForceLock)
{
	if (bRankLocked || bForceLock)
	{
		// Beginners should always join a beginner match
		if (Rank < 1400 && AverageRank < 1400) return true;

		return (Rank >= AverageRank - 400) && (Rank <= AverageRank + 400);
	}

	return true;
}


bool AUTLobbyMatchInfo::CanAddPlayer(int32 ELORank, bool bForceRankLock)
{
	return SkillTest(ELORank, bForceRankLock) && MatchHasRoom();
}


void AUTLobbyMatchInfo::UpdateRank()
{
	if (CurrentState == ELobbyMatchState::InProgress)
	{
		if (PlayersInMatchInstance.Num() > 0)
		{
			MinRank = PlayersInMatchInstance[0].PlayerRank;
			MaxRank = PlayersInMatchInstance[0].PlayerRank;
			AverageRank = PlayersInMatchInstance[0].PlayerRank;

			for (int32 i=1; i < PlayersInMatchInstance.Num(); i++)
			{
				int32 PlayerRank = PlayersInMatchInstance[i].PlayerRank;
				if (PlayerRank < MinRank) MinRank = PlayerRank;
				if (PlayerRank > MaxRank) MaxRank = PlayerRank;
				AverageRank += PlayerRank;
			}
		
			AverageRank = int32( float(AverageRank) / float(PlayersInMatchInstance.Num()));
		}
	}
	else
	{
		if (Players.Num() > 0)
		{
			MinRank = Players[0]->AverageRank;
			MaxRank = Players[0]->AverageRank;
			AverageRank = Players[0]->AverageRank;

			for (int32 i=1; i < Players.Num(); i++)
			{
				int32 PlayerRank = Players[i]->AverageRank;
				if (PlayerRank < MinRank) MinRank = PlayerRank;
				if (PlayerRank > MaxRank) MaxRank = PlayerRank;
				AverageRank += PlayerRank;
			}
		
			AverageRank = int32( float(AverageRank) / float(Players.Num()));
		}
	}
}

void AUTLobbyMatchInfo::OnRep_RedirectsChanged()
{
	bRedirectsHaveChanged = true;
}

void AUTLobbyMatchInfo::FillPlayerColumnsForDisplay(TArray<FMatchPlayerListStruct>& FirstColumn, TArray<FMatchPlayerListStruct>& SecondColumn, FString& Spectators)
{
	if (CurrentRuleset.IsValid())
	{
		if (CurrentRuleset->bTeamGame)
		{
			for (int32 i=0; i < Players.Num(); i++)
			{
				if (Players[i].IsValid())
				{
					if (Players[i]->GetTeamNum() == 0) FirstColumn.Add( FMatchPlayerListStruct(Players[i]->PlayerName, Players[i]->UniqueId.ToString(), TEXT("0"),0) );
					else if (Players[i]->GetTeamNum() == 1) SecondColumn.Add( FMatchPlayerListStruct(Players[i]->PlayerName, Players[i]->UniqueId.ToString() ,TEXT("0"),1) );
					else 
					{
						Spectators = Spectators.IsEmpty() ? Players[i]->PlayerName : FString::Printf(TEXT(", %s"), *Players[i]->PlayerName);
					}
				}

			}

			for (int32 i=0; i < PlayersInMatchInstance.Num(); i++)
			{
				if (PlayersInMatchInstance[i].TeamNum == 0) FirstColumn.Add( FMatchPlayerListStruct(PlayersInMatchInstance[i].PlayerName, PlayersInMatchInstance[i].PlayerID.ToString(), FString::Printf(TEXT("%i"),PlayersInMatchInstance[i].PlayerScore),0) );
				else if (PlayersInMatchInstance[i].TeamNum == 1) SecondColumn.Add(FMatchPlayerListStruct(PlayersInMatchInstance[i].PlayerName, PlayersInMatchInstance[i].PlayerID.ToString(), FString::Printf(TEXT("%i"),PlayersInMatchInstance[i].PlayerScore),1) );
				else 
				{
					Spectators = Spectators.IsEmpty() ? PlayersInMatchInstance[i].PlayerName : FString::Printf(TEXT(", %s"), *PlayersInMatchInstance[i].PlayerName);
				}
			}
		}
		else
		{
			int32 cnt=0;
			for (int32 i=0; i < Players.Num(); i++)
			{
				if (Players[i].IsValid())
				{
					if (Players[i]->bIsSpectator) 
					{
						Spectators = Spectators.IsEmpty() ? Players[i]->PlayerName : FString::Printf(TEXT("%s, %s"),*Spectators, *Players[i]->PlayerName);
					}
					else 
					{
						if (cnt % 2 == 0) 
						{
							FirstColumn.Add( FMatchPlayerListStruct(Players[i]->PlayerName, Players[i]->UniqueId.ToString(), TEXT("0"),0));
						}
						else
						{
							SecondColumn.Add( FMatchPlayerListStruct(Players[i]->PlayerName, Players[i]->UniqueId.ToString(), TEXT("0"),0));
						}
						cnt++;
					}
				}
			}

			for (int32 i=0; i < PlayersInMatchInstance.Num(); i++)
			{
				if (PlayersInMatchInstance[i].bIsSpectator) 
				{
					Spectators = Spectators.IsEmpty() ? PlayersInMatchInstance[i].PlayerName : FString::Printf(TEXT("%s, %s"), *Spectators , *PlayersInMatchInstance[i].PlayerName);
				}
				else
				{
					if (cnt % 2 == 0) 
					{
						FirstColumn.Add( FMatchPlayerListStruct(PlayersInMatchInstance[i].PlayerName, PlayersInMatchInstance[i].PlayerID.ToString(), FString::Printf(TEXT("%i"),PlayersInMatchInstance[i].PlayerScore),PlayersInMatchInstance[i].TeamNum));
					}
					else
					{
						SecondColumn.Add( FMatchPlayerListStruct(PlayersInMatchInstance[i].PlayerName, PlayersInMatchInstance[i].PlayerID.ToString(), FString::Printf(TEXT("%i"),PlayersInMatchInstance[i].PlayerScore),PlayersInMatchInstance[i].TeamNum));
					}
					cnt++;
				}
			}
		}
	}

	if (FirstColumn.Num() > 0) FirstColumn.Sort(FMatchPlayerListCompare());
	if (SecondColumn.Num() > 0) SecondColumn.Sort(FMatchPlayerListCompare());

}

void AUTLobbyMatchInfo::GetPlayerData(TArray<FMatchPlayerListStruct>& PlayerData)
{
	TArray<FMatchPlayerListStruct> ColumnA;
	TArray<FMatchPlayerListStruct> ColumnB;
	FString Specs;

	FillPlayerColumnsForDisplay(ColumnA, ColumnB, Specs);
	int32 Max = FMath::Max<int32>(ColumnA.Num(), ColumnB.Num());

	for (int32 i=0; i < Max; i++)
	{
		if (i < ColumnA.Num()) PlayerData.Add(FMatchPlayerListStruct(ColumnA[i].PlayerName, ColumnA[i].PlayerId, ColumnA[i].PlayerScore, ColumnA[i].TeamNum));
		if (i < ColumnB.Num()) PlayerData.Add(FMatchPlayerListStruct(ColumnB[i].PlayerName, ColumnB[i].PlayerId, ColumnB[i].PlayerScore, ColumnB[i].TeamNum));
	}
}

int32 AUTLobbyMatchInfo::CountFriendsInMatch(const TArray<FUTFriend>& Friends)
{
	int32 NumFriends = 0;

	for (int32 i=0; i < Players.Num(); i++)
	{
		for (int32 j = 0 ; j < Friends.Num(); j++)
		{
			if (Players[i].IsValid() && Players[i]->UniqueId.ToString() == Friends[j].UserId)
			{
				NumFriends++;
				break;
			}
		}
	}

	for (int32 i=0; i < PlayersInMatchInstance.Num(); i++)
	{
		for (int32 j = 0 ; j < Friends.Num(); j++)
		{
			if (PlayersInMatchInstance[i].PlayerID.ToString() == Friends[j].UserId)
			{
				NumFriends++;
				break;
			}
		}
	}

	return NumFriends;
}

uint32 AUTLobbyMatchInfo::GetMatchFlags()
{
	uint32 Flags = 0x00;
	if (CurrentState == ELobbyMatchState::Launching || CurrentState == ELobbyMatchState::InProgress)
	{
		Flags = Flags | MATCH_FLAG_InProgress;
	}

	if (bPrivateMatch)
	{
		Flags = Flags | MATCH_FLAG_Private;
	}

	if (bRankLocked)
	{
		Flags = Flags | MATCH_FLAG_Ranked;
	}

	if (!bJoinAnytime)
	{
		Flags = Flags | MATCH_FLAG_NoJoinInProgress;
	}

	if (!bSpectatable)
	{
		Flags = Flags | MATCH_FLAG_NoSpectators;
	}

	return Flags;
}

bool AUTLobbyMatchInfo::ServerInvitePlayer_Validate(AUTLobbyPlayerState* Who, bool bInvite) { return true; }
void AUTLobbyMatchInfo::ServerInvitePlayer_Implementation(AUTLobbyPlayerState* Who, bool bInvite)
{
	if (!Who)
	{
		return;
	}
	if (bInvite)
	{
		if (AllowedPlayerList.Find(Who->UniqueId.ToString()) == INDEX_NONE)
		{
			AllowedPlayerList.Add(Who->UniqueId.ToString());
			Who->InviteToMatch(this);
		}
	}
	else
	{
		if (AllowedPlayerList.Find(Who->UniqueId.ToString()) != INDEX_NONE)
		{
			AllowedPlayerList.Remove(Who->UniqueId.ToString());
			Who->UninviteFromMatch(this);
		}
	}
}
