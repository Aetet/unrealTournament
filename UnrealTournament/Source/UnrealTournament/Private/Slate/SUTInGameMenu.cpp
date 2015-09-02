// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "../Public/UnrealTournament.h"
#include "../Public/UTLocalPlayer.h"
#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "SUWindowsDesktop.h"
#include "SUWindowsMainMenu.h"
#include "SUWindowsStyle.h"
#include "SUWDialog.h"
#include "SUWSystemSettingsDialog.h"
#include "SUWPlayerSettingsDialog.h"
#include "SUWHUDSettingsDialog.h"
#include "SUWWeaponConfigDialog.h"
#include "SUWControlSettingsDialog.h"
#include "SUWInputBox.h"
#include "SUWMessageBox.h"
#include "SUWScaleBox.h"
#include "UTGameEngine.h"
#include "SUTInGameMenu.h"
#include "Panels/SUInGameHomePanel.h"
#include "UTAnalytics.h"
#include "Runtime/Analytics/Analytics/Public/Interfaces/IAnalyticsProvider.h"

#if !UE_SERVER

/****************************** [ Build Sub Menus ] *****************************************/

void SUTInGameMenu::BuildLeftMenuBar()
{
	if (LeftMenuBar.IsValid())
	{
		AUTGameState* GS = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
		AUTPlayerState* PS = PlayerOwner->PlayerController ? Cast<AUTPlayerState>(PlayerOwner->PlayerController->PlayerState) : NULL;
		bool bIsSpectator = PS && PS->bOnlySpectator;
		if (GS && GS->bTeamGame && !bIsSpectator)
		{
			LeftMenuBar->AddSlot()
			.Padding(5.0f,0.0f,0.0f,0.0f)
			.AutoWidth()
			[
				SNew(SButton)
				.ButtonStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button")
				.OnClicked(this, &SUTInGameMenu::OnTeamChangeClick)
				.ContentPadding(FMargin(25.0,0.0,25.0,5.0))
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("SUWindowsDesktop","MenuBar_ChangeTeam","CHANGE TEAM"))
						.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.TextStyle")
					]
				]
			];
		}
		if (GS && GS->GetMatchState() == MatchState::WaitingToStart)
		{
			if (GS->GetNetMode() == NM_Standalone)
			{
				LeftMenuBar->AddSlot()
					.Padding(5.0f, 0.0f, 0.0f, 0.0f)
					.AutoWidth()
					[
						SNew(SButton)
						.ButtonStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button")
						.OnClicked(this, &SUTInGameMenu::OnReadyChangeClick)
						.ContentPadding(FMargin(25.0, 0.0, 25.0, 5.0))
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(NSLOCTEXT("SUWindowsDesktop", "MenuBar_StartMatch", "START MATCH"))
								.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.TextStyle")
							]
						]
					];
			}
			else if (!bIsSpectator)
			{
				LeftMenuBar->AddSlot()
					.Padding(5.0f, 0.0f, 0.0f, 0.0f)
					.AutoWidth()
					[
						SNew(SButton)
						.ButtonStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button")
						.OnClicked(this, &SUTInGameMenu::OnReadyChangeClick)
						.ContentPadding(FMargin(25.0, 0.0, 25.0, 5.0))
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(NSLOCTEXT("SUWindowsDesktop", "MenuBar_ChangeReady", "CHANGE READY"))
								.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.TextStyle")
							]
						]
					];
			}
		}
		else if (GS && GS->GetMatchState() == MatchState::MapVoteHappening)
		{
			LeftMenuBar->AddSlot()
			.Padding(5.0f,0.0f,0.0f,0.0f)
			.AutoWidth()
			[
				SNew(SButton)
				.ButtonStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button")
				.OnClicked(this, &SUTInGameMenu::OnMapVoteClick)
				.ContentPadding(FMargin(25.0,0.0,25.0,5.0))
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &SUTInGameMenu::GetMapVoteTitle)
						.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.TextStyle")
					]
				]
			];

			PlayerOwner->OpenMapVote(NULL);
		}
	}
}

void SUTInGameMenu::BuildExitMenu(TSharedPtr<SComboButton> ExitButton, TSharedPtr<SVerticalBox> MenuSpace)
{
	MenuSpace->AddSlot()
	.AutoHeight()
	[
		SNew(SButton)
		.ButtonStyle(SUWindowsStyle::Get(), "UT.ContextMenu.Button")
		.ContentPadding(FMargin(10.0f, 5.0f))
		.Text(NSLOCTEXT("SUTMenuBase", "MenuBar_Exit_ReturnToGame", "Close Menu"))
		.TextStyle(SUWindowsStyle::Get(), "UT.ContextMenu.TextStyle")
		.OnClicked(this, &SUTInGameMenu::OnCloseMenu, ExitButton)
	];

	MenuSpace->AddSlot()
	.VAlign(VAlign_Center)
	.HAlign(HAlign_Fill)
	.Padding(FMargin(0.0f, 2.0f))
	[
		SNew(SImage)
		.Image(SUWindowsStyle::Get().GetBrush("UT.ContextMenu.Spacer"))
	];

	AUTGameState* GameState = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
	if ( GameState && GameState->HubGuid.IsValid() )
	{
		MenuSpace->AddSlot()
		.AutoHeight()
		[
			SNew(SButton)
			.ButtonStyle(SUWindowsStyle::Get(), "UT.ContextMenu.Button")
			.ContentPadding(FMargin(10.0f, 5.0f))
			.Text(NSLOCTEXT("SUTInGameMenu", "MenuBar_ReturnToLobby", "Return to Hub"))
			.TextStyle(SUWindowsStyle::Get(), "UT.ContextMenu.TextStyle")
			.OnClicked(this, &SUTInGameMenu::OnReturnToLobby, ExitButton)
		];
	}
	
	MenuSpace->AddSlot()
	.AutoHeight()
	[
		SNew(SButton)
		.ButtonStyle(SUWindowsStyle::Get(), "UT.ContextMenu.Button")
		.ContentPadding(FMargin(10.0f, 5.0f))
		.Text(NSLOCTEXT("SUTInGameMenu","MenuBar_ReturnToMainMenu","Return to Main Menu"))
		.TextStyle(SUWindowsStyle::Get(), "UT.ContextMenu.TextStyle")
		.OnClicked(this, &SUTInGameMenu::OnReturnToMainMenu, ExitButton)
	];

}

FReply SUTInGameMenu::OnCloseMenu(TSharedPtr<SComboButton> MenuButton)
{
	if (MenuButton.IsValid()) MenuButton->SetIsOpen(false);
	CloseMenus();

	return FReply::Handled();
}

FReply SUTInGameMenu::OnReturnToLobby(TSharedPtr<SComboButton> MenuButton)
{
	if (MenuButton.IsValid()) MenuButton->SetIsOpen(false);
	AUTGameState* GameState = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
	if ( GameState && GameState->HubGuid.IsValid() )
	{
		CloseMenus();
		AUTBasePlayerController* PC = Cast<AUTBasePlayerController>(PlayerOwner->PlayerController);
		if (PC)
		{
			WriteQuitMidGameAnalytics();
			PlayerOwner->CloseMapVote();
			PC->ConnectToServerViaGUID(GameState->HubGuid.ToString(),-1, false, false);
		}
	}

	return FReply::Handled();
}

void SUTInGameMenu::WriteQuitMidGameAnalytics()
{
	if (FUTAnalytics::IsAvailable() && PlayerOwner->GetWorld()->GetNetMode() != NM_Standalone)
	{
		AUTGameState* GameState = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
		if (GameState->HasMatchStarted() && !GameState->HasMatchEnded())
		{
			AUTBasePlayerController* PC = Cast<AUTBasePlayerController>(PlayerOwner->PlayerController);
			if (PC)
			{
				AUTPlayerState* PS = Cast<AUTPlayerState>(PC->PlayerState);
				if (PS)
				{
					extern float ENGINE_API GAverageFPS;
					TArray<FAnalyticsEventAttribute> ParamArray;
					ParamArray.Add(FAnalyticsEventAttribute(TEXT("FPS"), GAverageFPS));
					ParamArray.Add(FAnalyticsEventAttribute(TEXT("Kills"), PS->Kills));
					ParamArray.Add(FAnalyticsEventAttribute(TEXT("Deaths"), PS->Deaths));
					FUTAnalytics::GetProvider().RecordEvent(TEXT("QuitMidGame"), ParamArray);
				}
			}
		}
	}
}

FReply SUTInGameMenu::OnReturnToMainMenu(TSharedPtr<SComboButton> MenuButton)
{
	if (MenuButton.IsValid()) MenuButton->SetIsOpen(false);	

	WriteQuitMidGameAnalytics();
	PlayerOwner->CloseMapVote();
	CloseMenus();
	PlayerOwner->ReturnToMainMenu();
	return FReply::Handled();
}

FReply SUTInGameMenu::OnTeamChangeClick()
{
	AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
	if (PC)
	{
		PC->ServerSwitchTeam();
	}
	return FReply::Handled();
}

FReply SUTInGameMenu::OnReadyChangeClick()
{
	AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
	if (PC)
	{
//		PC->PlayMenuSelectSound();
		PC->ServerRestartPlayer();
		PlayerOwner->HideMenu();
	}
	return FReply::Handled();
}

FReply SUTInGameMenu::OnSpectateClick()
{
	ConsoleCommand(TEXT("ChangeTeam 255"));
	return FReply::Handled();
}


void SUTInGameMenu::SetInitialPanel()
{
	SAssignNew(HomePanel, SUInGameHomePanel, PlayerOwner);

	if (HomePanel.IsValid())
	{
		ActivatePanel(HomePanel);
	}
}

FReply SUTInGameMenu::OpenHUDSettings(TSharedPtr<SComboButton> MenuButton)
{
	if (MenuButton.IsValid()) MenuButton->SetIsOpen(false);
	CloseMenus();
	PlayerOwner->ShowHUDSettings();
	return FReply::Handled();
}

FReply SUTInGameMenu::OnMapVoteClick()
{
	AUTGameState* GameState = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
	if ( GameState )
	{
		PlayerOwner->OpenMapVote(GameState);
	}

	return FReply::Handled();
}

FText SUTInGameMenu::GetMapVoteTitle() const
{
	AUTGameState* GameState = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
	if ( GameState )
	{
		return FText::Format(NSLOCTEXT("SUTInGameMenu","MapVoteFormat","MAP VOTE ({0})"), FText::AsNumber(GameState->VoteTimer));
	}

	return NSLOCTEXT("SUWindowsDesktop","MenuBar_MapVote","MAP VOTE");
}

#endif