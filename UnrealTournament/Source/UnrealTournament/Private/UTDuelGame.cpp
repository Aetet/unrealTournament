// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTHUD_TeamDM.h"
#include "UTTimedPowerup.h"
#include "UTPickupWeapon.h"
#include "UTDuelGame.h"
#include "Slate/Panels/SUDuelSettings.h"


AUTDuelGame::AUTDuelGame(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	HUDClass = AUTHUD_DM::StaticClass();

	HUDClass = AUTHUD_TeamDM::StaticClass();
	DisplayName = NSLOCTEXT("UTGameMode", "Duel", "Duel");
	PowerupDuration = 10.f;
	GoalScore = 0;
	TimeLimit = 15.f;
	MaxReadyWaitTime = 60;
	bForceRespawn = true;
	bAnnounceTeam = false;
	bHighScorerPerTeamBasis = false;
	bHasRespawnChoices = true;
	bWeaponStayActive = false;
}

void AUTDuelGame::InitGameState()
{
	Super::InitGameState();

	UTGameState = Cast<AUTGameState>(GameState);
	if (UTGameState != NULL)
	{
		UTGameState->bWeaponStay = false;
		UTGameState->bAllowTeamSwitches = false;
	}
}

bool AUTDuelGame::ChangeTeam(AController* Player, uint8 NewTeam, bool bBroadcast)
{
	// don't allow team changes in Duel once have initial team
	if (Player == NULL)
	{
		return false;
	}
	else
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(Player->PlayerState);
		if (PS == NULL || PS->bOnlySpectator || PS->Team)
		{
			return false;
		}
	}
	return Super::ChangeTeam(Player, NewTeam, bBroadcast);
}

bool AUTDuelGame::CheckRelevance_Implementation(AActor* Other)
{
	AUTTimedPowerup* Powerup = Cast<AUTTimedPowerup>(Other);
	if (Powerup)
	{
		Powerup->TimeRemaining = PowerupDuration;
	}
	
	// @TODO FIXMESTEVE - don't check for weapon stay - once have deployable base class, remove all deployables from duel
	AUTPickupWeapon* PickupWeapon = Cast<AUTPickupWeapon>(Other);
	if (PickupWeapon != NULL && PickupWeapon->WeaponType != NULL && !PickupWeapon->WeaponType.GetDefaultObject()->bWeaponStay)
	{
		PickupWeapon->WeaponType = nullptr;
		PickupWeapon->bDisplayRespawnTimer = false;
	}

	return Super::CheckRelevance_Implementation(Other);
}

void AUTDuelGame::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	GameSession->MaxPlayers = 2;
	BotFillCount = FMath::Min<int32>(BotFillCount, 2);
	bForceRespawn = true;
	bBalanceTeams = true;
}

void AUTDuelGame::PlayEndOfMatchMessage()
{
	// individual winner, not team
	AUTGameMode::PlayEndOfMatchMessage();
}

#if !UE_SERVER
void AUTDuelGame::CreateConfigWidgets(TSharedPtr<class SVerticalBox> MenuSpace, bool bCreateReadOnly, TArray< TSharedPtr<TAttributePropertyBase> >& ConfigProps)
{
	TSharedPtr< TAttributeProperty<int32> > TimeLimitAttr = MakeShareable(new TAttributeProperty<int32>(this, &TimeLimit, TEXT("TimeLimit")));
	ConfigProps.Add(TimeLimitAttr);
	TSharedPtr< TAttributeProperty<int32> > GoalScoreAttr = MakeShareable(new TAttributeProperty<int32>(this, &GoalScore, TEXT("GoalScore")));
	ConfigProps.Add(GoalScoreAttr);
	TSharedPtr< TAttributeProperty<float> > BotSkillAttr = MakeShareable(new TAttributeProperty<float>(this, &GameDifficulty, TEXT("Difficulty")));
	ConfigProps.Add(BotSkillAttr);

	// TODO: BotSkill should be a list box with the usual items; this is a simple placeholder
	MenuSpace->AddSlot()
	.Padding(10.0f, 5.0f, 10.0f, 5.0f)
	.AutoHeight()
	.VAlign(VAlign_Top)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock)
			.TextStyle(SUWindowsStyle::Get(),"UWindows.Standard.NormalText")
			.Text(NSLOCTEXT("UTGameMode", "BotSkill", "Bot Skill"))
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Right)
			[
				SNew(SBox)
				.WidthOverride(150.0f)
				[
					bCreateReadOnly ?
					StaticCastSharedRef<SWidget>(
						SNew(STextBlock)
						.TextStyle(SUWindowsStyle::Get(),"UWindows.Standard.NormalText")
						.Text(BotSkillAttr.ToSharedRef(), &TAttributeProperty<float>::GetAsText)
					) :
					StaticCastSharedRef<SWidget>(
						SNew(SNumericEntryBox<float>)
						.LabelPadding(FMargin(10.0f, 0.0f))
						.Value(BotSkillAttr.ToSharedRef(), &TAttributeProperty<float>::GetOptional)
						.OnValueChanged(BotSkillAttr.ToSharedRef(), &TAttributeProperty<float>::Set)
						.AllowSpin(true)
						.Delta(1)
						.MinValue(0)
						.MaxValue(7)
						.MinSliderValue(0)
						.MaxSliderValue(7)
					)
				]
			]
		]
	];
	MenuSpace->AddSlot()
	.Padding(10.0f, 5.0f, 10.0f, 5.0f)
	.AutoHeight()
	.VAlign(VAlign_Top)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock)
			.TextStyle(SUWindowsStyle::Get(),"UWindows.Standard.NormalText")
			.Text(NSLOCTEXT("UTGameMode", "GoalScore", "Goal Score"))
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Right)
			[
				SNew(SBox)
				.WidthOverride(150.0f)
				[
					bCreateReadOnly ?
					StaticCastSharedRef<SWidget>(
						SNew(STextBlock)
						.TextStyle(SUWindowsStyle::Get(),"UWindows.Standard.NormalText")
						.Text(GoalScoreAttr.ToSharedRef(), &TAttributeProperty<int32>::GetAsText)
					) :
					StaticCastSharedRef<SWidget>(
						SNew(SNumericEntryBox<int32>)
						.Value(GoalScoreAttr.ToSharedRef(), &TAttributeProperty<int32>::GetOptional)
						.OnValueChanged(GoalScoreAttr.ToSharedRef(), &TAttributeProperty<int32>::Set)
						.AllowSpin(true)
						.Delta(1)
						.MinValue(0)
						.MaxValue(999)
						.MinSliderValue(0)
						.MaxSliderValue(99)
					)
				]
			]
		]
	];
	MenuSpace->AddSlot()
	.Padding(10.0f, 5.0f, 10.0f, 5.0f)
	.AutoHeight()
	.VAlign(VAlign_Top)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(STextBlock)
			.TextStyle(SUWindowsStyle::Get(), "UWindows.Standard.NormalText")
			.Text(NSLOCTEXT("UTGameMode", "TimeLimit", "Time Limit"))
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Right)
			[
				SNew(SBox)
				.WidthOverride(150.0f)
				[
					bCreateReadOnly ?
					StaticCastSharedRef<SWidget>(
						SNew(STextBlock)
						.TextStyle(SUWindowsStyle::Get(), "UWindows.Standard.NormalText")
						.Text(TimeLimitAttr.ToSharedRef(), &TAttributeProperty<int32>::GetAsText)
					) :
					StaticCastSharedRef<SWidget>(
						SNew(SNumericEntryBox<int32>)
						.Value(TimeLimitAttr.ToSharedRef(), &TAttributeProperty<int32>::GetOptional)
						.OnValueChanged(TimeLimitAttr.ToSharedRef(), &TAttributeProperty<int32>::Set)
						.AllowSpin(true)
						.Delta(1)
						.MinValue(0)
						.MaxValue(999)
						.MinSliderValue(0)
						.MaxSliderValue(60)
					)
				]
			]
		]
	];
}

#endif

void AUTDuelGame::UpdateLobbyMatchStats()
{
	Super::UpdateLobbyMatchStats();
	if (LobbyBeacon)
	{
		FString MatchStats = FString::Printf(TEXT("ElpasedTime=%i"), GetWorld()->GetGameState()->ElapsedTime);
		LobbyBeacon->UpdateMatch(MatchStats);
	}
}

void AUTDuelGame::UpdateSkillRating()
{
	for (int i = 0; i < UTGameState->PlayerArray.Num(); i++)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
		if (PS != nullptr && !PS->bOnlySpectator)
		{
			PS->UpdateTeamSkillRating(FName(TEXT("SkillRating")), UTGameState->WinnerPlayerState == PS);
		}
	}
}

void AUTDuelGame::UpdateLobbyBadge()
{
	AUTPlayerState* RedPlayer = NULL;
	AUTPlayerState* BluePlayer = NULL;
	AUTPlayerState* P;

	for (int i=0;i<UTGameState->PlayerArray.Num();i++)
	{
		P = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
		if (P && !P->bIsSpectator && !P->bOnlySpectator)
		{
			if (P->GetTeamNum() == 0) RedPlayer = P;
			else if (P->GetTeamNum() == 1) BluePlayer = P;
		}
	}

	if (RedPlayer && BluePlayer)
	{
		FString Update = TEXT("");
		if (RedPlayer->Score >= BluePlayer->Score)
		{
			Update = FString::Printf(TEXT("<UWindows.Standard.MatchBadge.Header>%s</>\n\n<UWindows.Standard.MatchBadge.Red>%s</>\n<UWindows.Standard.MatchBadge.Red>(%i)</>\n<UWindows.Standard.MatchBadge.Small>-vs-</>\n<UWindows.Standard.MatchBadge.Blue>%s></>\n<UWindows.Standard.MatchBadge.Blue>(%i)</>\n"), *DisplayName.ToString(), 
				*RedPlayer->PlayerName, 	int(RedPlayer->Score),
				*BluePlayer->PlayerName,	int(BluePlayer->Score));
		}
		else
		{
			Update = FString::Printf(TEXT("<UWindows.Standard.MatchBadge.Header>%s</>\n\n<UWindows.Standard.MatchBadge.Blue>%s</>\n<<UWindows.Standard.MatchBadge.Blue>(%i)</>\n<UWindows.Standard.MatchBadge.Small>-vs-</>\n<UWindows.Standard.MatchBadge.Red>%s</>\n<UWindows.Standard.MatchBadge.Red>(%i)</>\n"), *DisplayName.ToString(), 
				*BluePlayer->PlayerName, 	int(RedPlayer->Score),
				*RedPlayer->PlayerName,	int(BluePlayer->Score));
		}

		LobbyBeacon->Lobby_UpdateBadge(LobbyInstanceID, Update);


	}


}


