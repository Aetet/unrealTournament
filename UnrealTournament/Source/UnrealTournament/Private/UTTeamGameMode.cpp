// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTTeamGameMode.h"
#include "UTTeamInfo.h"
#include "UTTeamPlayerStart.h"
#include "SlateBasics.h"
#include "UTAnalytics.h"
#include "Runtime/Analytics/Analytics/Public/Analytics.h"
#include "Runtime/Analytics/Analytics/Public/Interfaces/IAnalyticsProvider.h"
#include "UTGameMessage.h"
#include "UTCTFGameMessage.h"

UUTTeamInterface::UUTTeamInterface(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

AUTTeamGameMode::AUTTeamGameMode(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	NumTeams = 2;
	bBalanceTeams = true;
	new(TeamColors) FLinearColor(1.0f, 0.05f, 0.0f, 1.0f);
	new(TeamColors) FLinearColor(0.1f, 0.1f, 1.0f, 1.0f);
	new(TeamColors) FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);
	new(TeamColors) FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);

	TeamNames.Add(NSLOCTEXT("UTTeamGameMode","Team0Name","Red"));
	TeamNames.Add(NSLOCTEXT("UTTeamGameMode","Team1Name","Blue"));
	TeamNames.Add(NSLOCTEXT("UTTeamGameMode","Team2Name","Gold"));
	TeamNames.Add(NSLOCTEXT("UTTeamGameMode","Team3Name","Green"));

	TeamMomentumPct = 0.75f;
	bTeamGame = true;
	bHasBroadcastDominating = false;
	bAnnounceTeam = true;
	bHighScorerPerTeamBasis = true;
}

void AUTTeamGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	bBalanceTeams = EvalBoolOptions(ParseOption(Options, TEXT("BalanceTeams")), bBalanceTeams);

	if (bAllowURLTeamCountOverride)
	{
		NumTeams = GetIntOption(Options, TEXT("NumTeams"), NumTeams);
	}
	NumTeams = FMath::Max<uint8>(NumTeams, 2);

	if (TeamClass == NULL)
	{
		TeamClass = AUTTeamInfo::StaticClass();
	}
	for (uint8 i = 0; i < NumTeams; i++)
	{
		AUTTeamInfo* NewTeam = GetWorld()->SpawnActor<AUTTeamInfo>(TeamClass);
		NewTeam->TeamIndex = i;
		if (TeamColors.IsValidIndex(i))
		{
			NewTeam->TeamColor = TeamColors[i];
		}

		if (TeamNames.IsValidIndex(i))
		{
			NewTeam->TeamName = TeamNames[i];
		}

		Teams.Add(NewTeam);
		checkSlow(Teams[i] == NewTeam);
	}

	MercyScore = FMath::Max(0, GetIntOption(Options, TEXT("MercyScore"), MercyScore));

	// TDM never kills off players going in to overtime
	bOnlyTheStrongSurvive = false;
}

void AUTTeamGameMode::InitGameState()
{
	Super::InitGameState();
	Cast<AUTGameState>(GameState)->Teams = Teams;
}

void AUTTeamGameMode::AnnounceMatchStart()
{
	if (bAnnounceTeam)
	{
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			AUTPlayerController* NextPlayer = Cast<AUTPlayerController>(*Iterator);
			AUTTeamInfo* Team = (NextPlayer && Cast<AUTPlayerState>(NextPlayer->PlayerState)) ? Cast<AUTPlayerState>(NextPlayer->PlayerState)->Team : NULL;
			if (Team)
			{
				int32 Switch = (Team->TeamIndex == 0) ? 9 : 10;
				NextPlayer->ClientReceiveLocalizedMessage(UUTGameMessage::StaticClass(), Switch, NextPlayer->PlayerState, NULL, NULL);
			}
		}
	}
	else
	{
		Super::AnnounceMatchStart();
	}
}

APlayerController* AUTTeamGameMode::Login(class UPlayer* NewPlayer, const FString& Portal, const FString& Options, const TSharedPtr<class FUniqueNetId>& UniqueId, FString& ErrorMessage)
{
	APlayerController* PC = Super::Login(NewPlayer, Portal, Options, UniqueId, ErrorMessage);

	if (PC != NULL && !PC->PlayerState->bOnlySpectator)
	{
		uint8 DesiredTeam = uint8(FMath::Clamp<int32>(GetIntOption(Options, TEXT("Team"), 255), 0, 255));
		ChangeTeam(PC, DesiredTeam, false);
	}

	return PC;
}

bool AUTTeamGameMode::ChangeTeam(AController* Player, uint8 NewTeam, bool bBroadcast)
{
	if (Player == NULL)
	{
		return false;
	}
	else
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(Player->PlayerState);
		if (PS == NULL || PS->bOnlySpectator)
		{
			return false;
		}
		else
		{
			bool bForceTeam = false;
			if (!Teams.IsValidIndex(NewTeam))
			{
				bForceTeam = true;
			}
			else
			{
				// see if someone is willing to switch
				for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
				{
					AUTPlayerController* NextPlayer = Cast<AUTPlayerController>(*Iterator);
					AUTPlayerState* SwitchingPS = NextPlayer ? Cast<AUTPlayerState>(NextPlayer->PlayerState) : NULL;
					if (SwitchingPS && SwitchingPS->bPendingTeamSwitch && (SwitchingPS->Team == Teams[NewTeam]) && Teams.IsValidIndex(1-NewTeam))
					{
						// Found someone who wants to leave team, so just replace them
						MovePlayerToTeam(NextPlayer, SwitchingPS, 1 - NewTeam);
						SwitchingPS->HandleTeamChanged(NextPlayer);
						MovePlayerToTeam(Player, PS, NewTeam);
						return true;
					}
				}

				if (bBalanceTeams)
				{
					for (int32 i = 0; i < Teams.Num(); i++)
					{
						// don't allow switching to a team with more players, or equal players if the player is on a team now
						if (i != NewTeam && Teams[i]->GetSize() - ((PS->Team != NULL && PS->Team->TeamIndex == i) ? 1 : 0)  < Teams[NewTeam]->GetSize())
						{
							bForceTeam = true;
							break;
						}
					}
				}
			}
			if (bForceTeam)
			{
				NewTeam = PickBalancedTeam(PS, NewTeam);
			}
		
			if (MovePlayerToTeam(Player, PS, NewTeam))
			{
				return true;
			}

			// temp logging to track down intermittent issue of not being able to change teams in reasonable situations
			UE_LOG(UT, Log, TEXT("Player %s denied from team change:"), *PS->PlayerName);
			for (int32 i = 0; i < Teams.Num(); i++)
			{
				UE_LOG(UT, Log, TEXT("Team (%i) size: %i"), i, Teams[i]->GetSize());
			}
			PS->bPendingTeamSwitch = true;
			return false;
		}
	}
}

bool AUTTeamGameMode::MovePlayerToTeam(AController* Player, AUTPlayerState* PS, uint8 NewTeam)
{
	if (Teams.IsValidIndex(NewTeam) && (PS->Team == NULL || PS->Team->TeamIndex != NewTeam))
	{
		if (PS->Team != NULL)
		{
			PS->Team->RemoveFromTeam(Player);
		}
		Teams[NewTeam]->AddToTeam(Player);
		PS->bPendingTeamSwitch = false;
		return true;
	}
	return false;
}

uint8 AUTTeamGameMode::PickBalancedTeam(AUTPlayerState* PS, uint8 RequestedTeam)
{
	TArray< AUTTeamInfo*, TInlineAllocator<4> > BestTeams;
	int32 BestSize = -1;

	for (int32 i = 0; i < Teams.Num(); i++)
	{
		int32 TestSize = Teams[i]->GetSize();
		if (Teams[i] == PS->Team)
		{
			// player will be leaving this team so count it's size as post-departure
			TestSize--;
		}
		if (BestTeams.Num() == 0 || TestSize < BestSize)
		{
			BestTeams.Empty();
			BestTeams.Add(Teams[i]);
			BestSize = TestSize;
		}
		else if (TestSize == BestSize)
		{
			BestTeams.Add(Teams[i]);
		}
	}

	for (int32 i = 0; i < BestTeams.Num(); i++)
	{
		if (BestTeams[i]->TeamIndex == RequestedTeam)
		{
			return RequestedTeam;
		}
	}

	return BestTeams[FMath::RandHelper(BestTeams.Num())]->TeamIndex;
}

void AUTTeamGameMode::DefaultTimer()
{
	Super::DefaultTimer();

	// check if bots should switch teams for balancing
	if (bBalanceTeams && NumBots > 0)
	{
		struct FTeamSizeSort
		{
			bool operator()(AUTTeamInfo& A, AUTTeamInfo& B) const
			{
				return (A.GetSize() > B.GetSize());
			}
		};
		TArray<AUTTeamInfo*> SortedTeams = UTGameState->Teams;
		SortedTeams.Sort(FTeamSizeSort());

		for (int32 i = 1; i < SortedTeams.Num(); i++)
		{
			if (SortedTeams[i - 1]->GetSize() > SortedTeams[i]->GetSize() + 1)
			{
				TArray<AController*> Members = SortedTeams[i - 1]->GetTeamMembers();
				for (AController* C : Members)
				{
					AUTBot* B = Cast<AUTBot>(C);
					if (B != NULL && B->GetPawn() == NULL)
					{
						ChangeTeam(B, SortedTeams[i]->GetTeamNum(), true);
					}
				}
			}
		}
	}
}

void AUTTeamGameMode::ModifyDamage_Implementation(int32& Damage, FVector& Momentum, APawn* Injured, AController* InstigatedBy, const FHitResult& HitInfo, AActor* DamageCauser, TSubclassOf<UDamageType> DamageType)
{
	if (InstigatedBy != NULL && InstigatedBy != Injured->Controller && Cast<AUTGameState>(GameState)->OnSameTeam(Injured, InstigatedBy))
	{
		Damage *= TeamDamagePct;
		Momentum *= TeamMomentumPct;
	}
	Super::ModifyDamage_Implementation(Damage, Momentum, Injured, InstigatedBy, HitInfo, DamageCauser, DamageType);
}

float AUTTeamGameMode::RatePlayerStart(APlayerStart* P, AController* Player)
{
	float Result = Super::RatePlayerStart(P, Player);
	if (bUseTeamStarts && Player != NULL)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(Player->PlayerState);
		if (PS != NULL && PS->Team != NULL && (Cast<AUTTeamPlayerStart>(P) == NULL || ((AUTTeamPlayerStart*)P)->TeamNum != PS->Team->TeamIndex))
		{
			// return low positive rating so it can be used as a last resort
			Result *= 0.05;
		}
	}
	return Result;
}

bool AUTTeamGameMode::CheckScore(AUTPlayerState* Scorer)
{
	AUTTeamInfo* WinningTeam = NULL;

	if (MercyScore > 0)
	{
		int32 Spread = Scorer->Team->Score;
		for (AUTTeamInfo* OtherTeam : Teams)
		{
			if (OtherTeam != Scorer->Team)
			{
				Spread = FMath::Min<int32>(Spread, Scorer->Team->Score - OtherTeam->Score);
			}
		}
		if (Spread >= MercyScore)
		{
			EndGame(Scorer, FName(TEXT("MercyScore")));
			return true;
		}
	}

	// Unlimited play
	if (GoalScore <= 0)
	{
		return false;
	}

	for (int32 i = 0; i < Teams.Num(); i++)
	{
		if (Teams[i]->Score >= GoalScore)
		{
			WinningTeam = Teams[i];
			break;
		}
	}

	if (WinningTeam != NULL)
	{
		AUTPlayerState* BestPlayer = FindBestPlayerOnTeam(WinningTeam->GetTeamNum());
		if (BestPlayer == NULL) BestPlayer = Scorer;
		EndGame(BestPlayer, TEXT("fraglimit")); 
		return true;
	}
	else
	{
		return false;
	}
}

#if !UE_SERVER
void AUTTeamGameMode::CreateConfigWidgets(TSharedPtr<class SVerticalBox> MenuSpace, TArray< TSharedPtr<TAttributePropertyBase> >& ConfigProps)
{
	Super::CreateConfigWidgets(MenuSpace, ConfigProps);

	TSharedPtr< TAttributePropertyBool > BalanceTeamsAttr = MakeShareable(new TAttributePropertyBool(this, &bBalanceTeams));
	ConfigProps.Add(BalanceTeamsAttr);

	MenuSpace->AddSlot()
	.Padding(0.0f, 5.0f, 0.0f, 5.0f)
	.AutoHeight()
	.VAlign(VAlign_Top)
	.HAlign(HAlign_Left)
	[
		SNew(SCheckBox)
		.IsChecked(BalanceTeamsAttr.ToSharedRef(), &TAttributePropertyBool::GetAsCheckBox)
		.OnCheckStateChanged(BalanceTeamsAttr.ToSharedRef(), &TAttributePropertyBool::SetFromCheckBox)
		.Type(ESlateCheckBoxType::CheckBox)
		.Content()
		[
			SNew(STextBlock)
			.ColorAndOpacity(FLinearColor::White)
			.Text(NSLOCTEXT("UTTeamGameMode", "BalanceTeams", "Balance Teams").ToString())
		]
	];
}
#endif

AUTPlayerState* AUTTeamGameMode::FindBestPlayerOnTeam(int TeamNumToTest)
{
	AUTPlayerState* Best = NULL;
	for (int i = 0; i < UTGameState->PlayerArray.Num(); i++)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
		if (PS != NULL && PS->GetTeamNum() == TeamNumToTest && (Best == NULL || Best->Score < PS->Score))
		{
			Best = PS;
		}
	}
	return Best;
}

void AUTTeamGameMode::BroadcastScoreUpdate(APlayerState* ScoringPlayer, AUTTeamInfo* ScoringTeam)
{
	// find best competing score - assume this is called after scores are updated.
	int32 BestScore = 0;
	for (int32 i = 0; i < Teams.Num(); i++)
	{
		if ((Teams[i] != ScoringTeam) && (Teams[i]->Score >= BestScore))
		{
			BestScore = Teams[i]->Score;
		}
	}
	if (ScoringTeam->Score == BestScore + 2)
	{
		BroadcastLocalized(this, UUTCTFGameMessage::StaticClass(), 8, ScoringPlayer, NULL, ScoringTeam);
	}
	else if (ScoringTeam->Score >= ((MercyScore > 0) ? (BestScore + MercyScore - 1) : (BestScore + 4)))
	{
		BroadcastLocalized(this, UUTCTFGameMessage::StaticClass(), bHasBroadcastDominating ? 2 : 9, ScoringPlayer, NULL, ScoringTeam);
		bHasBroadcastDominating = true;
	}
	else
	{
		bHasBroadcastDominating = false; // since other team scored, need new reminder if mercy rule might be hit again
		BroadcastLocalized(this, UUTCTFGameMessage::StaticClass(), 2, ScoringPlayer, NULL, ScoringTeam);
	}
}

void AUTTeamGameMode::PlayEndOfMatchMessage()
{
	int32 IsFlawlessVictory = (UTGameState->WinningTeam->Score > 3) ? 1 : 0;
	for (int32 i = 0; i < Teams.Num(); i++)
	{
		if ((Teams[i] != UTGameState->WinningTeam) && (Teams[i]->Score > 0))
		{
			IsFlawlessVictory = 0;
			break;
		}
	}

	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* Controller = *Iterator;
		if (Controller && Controller->IsA(AUTPlayerController::StaticClass()))
		{
			AUTPlayerController* PC = Cast<AUTPlayerController>(Controller);
			if (PC && Cast<AUTPlayerState>(PC->PlayerState) && !PC->PlayerState->bOnlySpectator)
			{
				PC->ClientReceiveLocalizedMessage(VictoryMessageClass, 2*IsFlawlessVictory + ((UTGameState->WinningTeam == Cast<AUTPlayerState>(PC->PlayerState)->Team) ? 1 : 0), UTGameState->WinnerPlayerState, PC->PlayerState, UTGameState->WinningTeam);
			}
		}
	}
}

void AUTTeamGameMode::SendEndOfGameStats(FName Reason)
{
	if (FUTAnalytics::IsAvailable())
	{
		if (GetWorld()->GetNetMode() != NM_Standalone)
		{
			TArray<FAnalyticsEventAttribute> ParamArray;
			ParamArray.Add(FAnalyticsEventAttribute(TEXT("Reason"), Reason.ToString()));
			ParamArray.Add(FAnalyticsEventAttribute(TEXT("TeamCount"), UTGameState->Teams.Num()));
			for (int i=0;i<UTGameState->Teams.Num();i++)
			{
				FString TeamName = FString::Printf(TEXT("TeamScore%i"), i);
				ParamArray.Add(FAnalyticsEventAttribute(TeamName, UTGameState->Teams[i]->Score));
			}
			FUTAnalytics::GetProvider().RecordEvent(TEXT("EndTeamMatch"), ParamArray);
		}
	}
	
	if (!bDisableCloudStats)
	{
		UpdateSkillRating();

		const double CloudStatsStartTime = FPlatformTime::Seconds();
		for (int32 i = 0; i < GetWorld()->GameState->PlayerArray.Num(); i++)
		{
			AUTPlayerState* PS = Cast<AUTPlayerState>(GetWorld()->GameState->PlayerArray[i]);
			
			PS->ModifyStat(FName(TEXT("MatchesPlayed")), 1, EStatMod::Delta);
			PS->ModifyStat(FName(TEXT("TimePlayed")), UTGameState->ElapsedTime, EStatMod::Delta);

			if (UTGameState->WinningTeam == PS->Team)
			{
				PS->ModifyStat(FName(TEXT("Wins")), 1, EStatMod::Delta);
			}
			else
			{
				PS->ModifyStat(FName(TEXT("Losses")), 1, EStatMod::Delta);
			}

			PS->AddMatchToStats(GetClass()->GetPathName(), &Teams, &GetWorld()->GameState->PlayerArray, &InactivePlayerArray);
			if (PS != nullptr)
			{
				PS->WriteStatsToCloud();
			}
		}
		const double CloudStatsTime = FPlatformTime::Seconds() - CloudStatsStartTime;
		UE_LOG(UT, Log, TEXT("Cloud stats write time %.3f"), CloudStatsTime);
	}
}

void AUTTeamGameMode::FindAndMarkHighScorer()
{
	// Some game modes like Duel may not want every team to have a high scorer
	if (!bHighScorerPerTeamBasis)
	{
		Super::FindAndMarkHighScorer();
		return;
	}

	for (int32 i = 0; i < Teams.Num(); i++)
	{
		int32 BestScore = 0;

		for (int32 PlayerIdx = 0; PlayerIdx < Teams[i]->GetTeamMembers().Num(); PlayerIdx++)
		{
			if (Teams[i]->GetTeamMembers()[PlayerIdx] != nullptr)
			{
				AUTPlayerState *PS = Cast<AUTPlayerState>(Teams[i]->GetTeamMembers()[PlayerIdx]->PlayerState);
				if (PS != nullptr)
				{
					if (BestScore == 0 || PS->Score > BestScore)
					{
						BestScore = PS->Score;
					}
				}
			}
		}

		for (int32 PlayerIdx = 0; PlayerIdx < Teams[i]->GetTeamMembers().Num(); PlayerIdx++)
		{
			if (Teams[i]->GetTeamMembers()[PlayerIdx] != nullptr)
			{
				AUTPlayerState *PS = Cast<AUTPlayerState>(Teams[i]->GetTeamMembers()[PlayerIdx]->PlayerState);
				if (PS != nullptr)
				{
					PS->bHasHighScore = (BestScore == PS->Score);
					AUTCharacter *UTChar = Cast<AUTCharacter>(Teams[i]->GetTeamMembers()[PlayerIdx]->GetPawn());
					if (UTChar)
					{
						UTChar->bHasHighScore = (BestScore == PS->Score);
					}
				}
			}
		}
	}
}

void AUTTeamGameMode::UpdateLobbyBadge()
{
	TArray<int32> Scores;
	Scores.Add( UTGameState->Teams.Num() > 0 ? UTGameState->Teams[0]->Score : 0);
	Scores.Add( UTGameState->Teams.Num() > 1 ? UTGameState->Teams[1]->Score : 0);

	FString Update = FString::Printf(TEXT("<UWindows.Standard.MatchBadge.Header>%s</>\n\n<UWindows.Standard.MatchBadge.Red>%i</><UWindows.Standard.MatchBadge> - <UWindows.Standard.MatchBadge.Blue>%i</>"), *DisplayName.ToString(), Scores[0], Scores[1]);

	LobbyBeacon->Lobby_UpdateBadge(LobbyInstanceID, Update);

}
