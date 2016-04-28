// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "SUTMatchmakingDialog.h"
#include "UTGameInstance.h"
#include "UTParty.h"
#include "UTMatchmaking.h"
#include "UTPartyGameState.h"
#include "../SUWindowsStyle.h"

#if !UE_SERVER

void SUTMatchmakingDialog::Construct(const FArguments& InArgs)
{
	SUTDialogBase::Construct(SUTDialogBase::FArguments()
		.PlayerOwner(InArgs._PlayerOwner)
		.DialogTitle(InArgs._DialogTitle)
		.DialogSize(InArgs._DialogSize)
		.bDialogSizeIsRelative(InArgs._bDialogSizeIsRelative)
		.DialogPosition(InArgs._DialogPosition)
		.DialogAnchorPoint(InArgs._DialogAnchorPoint)
		.ContentPadding(InArgs._ContentPadding)
		.ButtonMask(InArgs._ButtonMask)
		.OnDialogResult(InArgs._OnDialogResult)
		);

	if (DialogContent.IsValid())
	{		
		DialogContent->AddSlot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding(0.0f, 5.0f, 0.0f, 5.0f)
			.AutoHeight()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SUTMatchmakingDialog::GetRegionText)
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.ButtonText.White")
				.ColorAndOpacity(FLinearColor::Gray)
			]
			+ SVerticalBox::Slot()
			.Padding(0.0f, 5.0f, 0.0f, 5.0f)
			.AutoHeight()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SUTMatchmakingDialog::GetMatchmakingText)
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.ButtonText.White")
				.ColorAndOpacity(FLinearColor::Gray)
			]
		];
	}
}

FText SUTMatchmakingDialog::GetRegionText() const
{
	UUTGameInstance* GameInstance = Cast<UUTGameInstance>(GetPlayerOwner()->GetGameInstance());
	if (GameInstance)
	{
		UUTParty* Party = GameInstance->GetParties();
		if (Party)
		{
			UUTPartyGameState* PartyState = Party->GetUTPersistentParty();
			if (PartyState)
			{
				FString MatchMakingRegion = PartyState->GetMatchmakingRegion();
				return FText::Format(NSLOCTEXT("Generic", "Region", "Region: {0}"), FText::FromString(MatchMakingRegion));
			}
		}
	}

	return NSLOCTEXT("Generic", "LeaderMatchmaking", "Leader Is Matchmaking");
}

FText SUTMatchmakingDialog::GetMatchmakingText() const
{
	UUTGameInstance* GameInstance = Cast<UUTGameInstance>(GetPlayerOwner()->GetGameInstance());
	if (GameInstance)
	{
		UUTParty* Party = GameInstance->GetParties();
		if (Party)
		{
			UUTPartyGameState* PartyState = Party->GetUTPersistentParty();
			if (PartyState)
			{
				switch (PartyState->GetPartyProgression())
				{
				case EUTPartyState::PostMatchmaking:
					return FText::Format(NSLOCTEXT("Generic", "WaitingOnOtherPlayers", "Server Found. Waiting For {0} Players To Join..."), FText::AsNumber(PartyState->GetMatchmakingPlayersNeeded()));
				}
			}
		}

		UUTMatchmaking* Matchmaking = GameInstance->GetMatchmaking();
		if (Matchmaking)
		{
			int32 MatchmakingEloRange = Matchmaking->GetMatchmakingEloRange();
			if (MatchmakingEloRange > 0)
			{
				return FText::Format(NSLOCTEXT("Generic", "SearchingForServerWithEloRange", "Searching For Server Within ELO Range of {0}..."), FText::AsNumber(MatchmakingEloRange));
			}
		}
	}

	return NSLOCTEXT("Generic", "SearchingForServer", "Searching For Server...");
}

FReply SUTMatchmakingDialog::OnButtonClick(uint16 ButtonID)
{
	OnDialogResult.ExecuteIfBound(SharedThis(this), UTDIALOG_BUTTON_CANCEL);
	GetPlayerOwner()->CloseDialog(SharedThis(this));

	return FReply::Handled();
}

void SUTMatchmakingDialog::Tick(const FGeometry & AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SUTDialogBase::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

#endif