
// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "../Public/UnrealTournament.h"
#include "../Public/UTLocalPlayer.h"
#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "../SUWPanel.h"
#include "../SUWindowsStyle.h"
#include "SUMidGameInfoPanel.h"
#include "SULobbyMatchSetupPanel.h"
#include "UTLobbyMatchInfo.h"


#if !UE_SERVER

void SULobbyMatchSetupPanel::Construct(const FArguments& InArgs)
{
	bIsHost = InArgs._bIsHost;
	MatchInfo = InArgs._MatchInfo;
	PlayerOwner = InArgs._PlayerOwner;

	OnMatchInfoGameModeChangedDelegate.BindSP(this, &SULobbyMatchSetupPanel::OnMatchGameModeChanged); 
	MatchInfo->OnMatchGameModeChanged = OnMatchInfoGameModeChangedDelegate;

	AUTLobbyGameState* LobbyGameState = GWorld->GetGameState<AUTLobbyGameState>();
	MatchInfo->CurrentGameModeData = LobbyGameState->ResolveGameMode(MatchInfo->MatchGameMode);

	ChildSlot
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.AutoHeight()
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			[
				SNew(SImage)
				.Image(SUWindowsStyle::Get().GetBrush("UWindows.Standard.DarkBackground"))
			]
			+SOverlay::Slot()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.AutoWidth()
				.Padding(10.0f,0.0f,10.0f,0.0f)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("SULobbySetup","GameMode","GAME MODE:"))
					.TextStyle(SUWindowsStyle::Get(),"UWindows.Standard.Dialog.TextStyle")
				]
				+SHorizontalBox::Slot()
				.Padding(5.0f,0.0f,10.0f,0.0f)
				.AutoWidth()
				[
					BuildGameModeWidget()
				]		
				+SHorizontalBox::Slot()
				.Padding(5.0f,10.0f, 10.0f, 0.0f)
				.AutoWidth()
				[
					BuildHostOptionWidgets()
				]

			]
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		.VAlign(VAlign_Fill)
		[
			SAssignNew(GamePanel, SVerticalBox)
		]
	];

	// Trigger the first build.
	ChangeGameModePanel();

}

TSharedRef<SWidget> SULobbyMatchSetupPanel::BuildGameModeWidget()
{
	AUTLobbyGameState* LobbyGameState = GWorld->GetGameState<AUTLobbyGameState>();

	if (LobbyGameState && bIsHost)
	{
		return SNew(SBox)
		.WidthOverride(200)
		[
			SNew(SComboBox< TSharedPtr<FAllowedGameModeData> >)
			.InitiallySelectedItem(MatchInfo.Get()->CurrentGameModeData)
			.ComboBoxStyle(SUWindowsStyle::Get(), "UWindows.Standard.ComboBox")
			.ButtonStyle(SUWindowsStyle::Get(), "UWindows.Standard.Button")
			.OptionsSource(&LobbyGameState->ClientAvailbleGameModes)
			.OnGenerateWidget(this, &SULobbyMatchSetupPanel::GenerateGameModeListWidget)
			.OnSelectionChanged(this, &SULobbyMatchSetupPanel::OnGameModeChanged)
			.Content()
			[
				SAssignNew(CurrentGameMode, STextBlock)
				//.Text(FText::FromString(MatchInfo.Get()->CurrentGameModeData->DisplayName))
				.TextStyle(SUWindowsStyle::Get(),"UWindows.Standard.Dialog.Options.TextStyle")
			]
		];
	}
	else
	{
		return SNew(SBox)
		.WidthOverride(200)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SULobbyMatchSetupPanel::GetGameModeText)
				.TextStyle(SUWindowsStyle::Get(),"UWindows.Standard.Dialog.TextStyle")
			]
		];
	}
}

TSharedRef<SWidget> SULobbyMatchSetupPanel::BuildHostOptionWidgets()
{
	TSharedPtr<SHorizontalBox> Container;

	if (!bIsHost)
	{
		return SNew(SCanvas);
	}

	SNew(SVerticalBox)
	+SVerticalBox::Slot()
	.VAlign(VAlign_Center)
	[
		SAssignNew(Container, SHorizontalBox)
	];


	if (Container.IsValid())
	{
		float MaxPlayerValue = (float(MatchInfo->MaxPlayers) - 2.0f) / 30.0f;

		Container->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.0f, 0.0f, 20.0f, 0.0f )
		[
			SNew(SCheckBox)
			.IsChecked((MatchInfo->bSpectatable ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked))
			.OnCheckStateChanged(this, &SULobbyMatchSetupPanel::AllowSpectatorChanged)
			.Content()
			[
				SNew(STextBlock)
				.TextStyle(SUWindowsStyle::Get(), "UWindows.Standard.Dialog.TextStyle")
				.Text(NSLOCTEXT("SULobbySetup", "AllowSpectators", "Allow Spectating").ToString())
			]
		];

		// MUST BE LAST -- SLATE NO LIKEY THE HIDEY

		Container->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.0f, 0.0f, 20.0f, 0.0f )
		[
			SAssignNew(JIPCheckBox, SCheckBox)
			.IsChecked((MatchInfo->bJoinAnytime ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked))
			.OnCheckStateChanged(this, &SULobbyMatchSetupPanel::JoinAnyTimeChanged)
			.Content()
			[
				SNew(STextBlock)
				.TextStyle(SUWindowsStyle::Get(), "UWindows.Standard.Dialog.TextStyle")
				.Text(NSLOCTEXT("SULobbySetup", "AllowJoininProgress", "Allow Join-In-Progress").ToString())
			]
		];


		Container->AddSlot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.0f, 0.0f, 20.0f, 0.0f )
		[
			SAssignNew(MaxPlayersBox, SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(120)
				[
					SNew(STextBlock)
					.TextStyle(SUWindowsStyle::Get(),"UWindows.Standard.Dialog.TextStyle")
					.Text(this, &SULobbyMatchSetupPanel::GetMaxPlayerLabel)
				]
			]
			+ SHorizontalBox::Slot()
			.Padding(10.0f, 0.0f, 10.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(75.0f)
				.Content()
				[
					SNew(SSlider)
					.Orientation(Orient_Horizontal)
					.Value(MaxPlayerValue)
					.OnValueChanged(this, &SULobbyMatchSetupPanel::OnMaxPlayersChanged)
				]
			]
		];

		JIPCheckBox->SetVisibility(MatchInfo->CurrentGameModeData->DefaultObject->bLobbyAllowJoinInProgress ? EVisibility::Visible : EVisibility::Hidden );
		MaxPlayersBox->SetVisibility(MatchInfo->CurrentGameModeData->DefaultObject->MaxLobbyPlayers <= 2 ? EVisibility::Hidden : EVisibility::Visible);
	}

	return Container.ToSharedRef();
}

FText SULobbyMatchSetupPanel::GetMaxPlayerLabel() const
{
	return FText::Format(NSLOCTEXT("LobbyMatchPanel","MaxPlayerFormat","Max Players: {0}  "), FText::AsNumber(MatchInfo->MaxPlayers));
}

void SULobbyMatchSetupPanel::OnMaxPlayersChanged(float NewValue)
{
	float NoPlayers = (30.0f * NewValue) + 2.0f;
	MatchInfo->SetMaxPlayers(int32(NoPlayers));
}

FString SULobbyMatchSetupPanel::GetGameModeText() const
{

	return (MatchInfo->CurrentGameModeData.IsValid() ? MatchInfo->CurrentGameModeData->DisplayName : TEXT("None"));
}

TSharedRef<SWidget> SULobbyMatchSetupPanel::GenerateGameModeListWidget(TSharedPtr<FAllowedGameModeData> InItem)
{
	return SNew(SBox)
		.Padding(5)
		[
			SNew(STextBlock)
			.ColorAndOpacity(FLinearColor::Black)
			.Text(FText::FromString(InItem.Get()->DisplayName))
		];
}

void SULobbyMatchSetupPanel::OnGameModeChanged(TSharedPtr<FAllowedGameModeData> NewSelection, ESelectInfo::Type SelectInfo)
{
	AUTLobbyGameState* LobbyGameState = GWorld->GetGameState<AUTLobbyGameState>();
	if (LobbyGameState)
	{
		MatchInfo->MatchGameMode = NewSelection->ClassName;
		MatchInfo->ServerMatchGameModeChanged(MatchInfo->MatchGameMode);

		MatchInfo->CurrentGameModeData = LobbyGameState->ResolveGameMode(MatchInfo->MatchGameMode);
		JIPCheckBox->SetVisibility(MatchInfo->CurrentGameModeData->DefaultObject->bLobbyAllowJoinInProgress ? EVisibility::Visible : EVisibility::Hidden );
		MaxPlayersBox->SetVisibility(MatchInfo->CurrentGameModeData->DefaultObject->MaxLobbyPlayers <= 2 ? EVisibility::Hidden : EVisibility::Visible);
		// Reseed the local match info.
	
		MatchInfo->UpdateGameMode();
		ChangeGameModePanel();
	}
}


void SULobbyMatchSetupPanel::ChangeGameModePanel()
{

	AUTLobbyGameState* LobbyGameState = GWorld->GetGameState<AUTLobbyGameState>();
	if (LobbyGameState)
	{
		if (GamePanel.IsValid())
		{
			MatchInfo->BuildAllowedMapsList();
			if (bIsHost)
			{
				MatchInfo->ClientGetDefaultGameOptions();
			}

			if (CurrentGameMode.IsValid())
			{
				CurrentGameMode->SetText(MatchInfo->CurrentGameModeData->DisplayName);
			}

			GamePanel->ClearChildren();
			GamePanel->AddSlot()
				.HAlign(HAlign_Fill)
				.AutoHeight()
				[
					SNew(SULobbyGameSettingsPanel)
					.PlayerOwner(PlayerOwner)
					.MatchInfo(MatchInfo)
					.DefaultGameMode(MatchInfo.Get()->CurrentGameModeData->DefaultObject)
					.bIsHost(bIsHost)
				];
		}
	}
}


void SULobbyMatchSetupPanel::OnMatchGameModeChanged()
{
	if (!bIsHost && MatchInfo.IsValid())
	{
		ChangeGameModePanel();
	}
}

void SULobbyMatchSetupPanel::JoinAnyTimeChanged(ECheckBoxState NewState)
{
	if (MatchInfo.IsValid())
	{
		MatchInfo->SetAllowJoinInProgress(NewState == ESlateCheckBoxState::Checked);
	}
}

void SULobbyMatchSetupPanel::AllowSpectatorChanged(ECheckBoxState NewState)
{
	if (MatchInfo.IsValid())
	{
		MatchInfo->SetAllowSpectating(NewState == ESlateCheckBoxState::Checked);
	}

}


#endif