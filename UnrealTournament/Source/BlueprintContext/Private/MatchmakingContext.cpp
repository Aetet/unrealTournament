// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "BlueprintContext.h"

#include "UTMatchmaking.h"
#include "UTLocalPlayer.h"
#include "UTPartyGameState.h"
#include "MatchmakingContext.h"
#include "PartyContext.h"
#include "BlueprintContextLibrary.h"

#define LOCTEXT_NAMESPACE "UTMatchmakingContext"

REGISTER_CONTEXT(UMatchmakingContext);

UMatchmakingContext::UMatchmakingContext()
{
}

void UMatchmakingContext::Initialize()
{
	UUTGameInstance* GameInstance = GetGameInstance<UUTGameInstance>();
	if (GameInstance)
	{
		UUTMatchmaking* UTMatchmaking = GameInstance->GetMatchmaking();
		if (UTMatchmaking)
		{
			UTMatchmaking->OnMatchmakingStarted().AddUObject(this, &ThisClass::OnMatchmakingStartedInternal);
			UTMatchmaking->OnMatchmakingStateChange().AddUObject(this, &ThisClass::OnMatchmakingStateChangeInternal);
			UTMatchmaking->OnMatchmakingComplete().AddUObject(this, &ThisClass::OnMatchmakingCompleteInternal);
			UTMatchmaking->OnPartyStateChange().AddUObject(this, &ThisClass::OnPartyStateChangeInternal);
		}
	}
}

void UMatchmakingContext::Finalize()
{
	UUTGameInstance* GameInstance = GetGameInstance<UUTGameInstance>();
	if (GameInstance)
	{
		UUTMatchmaking* UTMatchmaking = GameInstance->GetMatchmaking();
		if (UTMatchmaking)
		{
			UTMatchmaking->OnMatchmakingStarted().RemoveAll(this);
			UTMatchmaking->OnMatchmakingStateChange().RemoveAll(this);
			UTMatchmaking->OnMatchmakingComplete().RemoveAll(this);
			UTMatchmaking->OnPartyStateChange().RemoveAll(this);
		}
	}
}

void UMatchmakingContext::OnMatchmakingStartedInternal(bool bRanked)
{
	OnMatchmakingStarted.Broadcast();

	UUTLocalPlayer* LocalPlayer = GetOwningPlayer<UUTLocalPlayer>();
	if (LocalPlayer)
	{
		LocalPlayer->ShowMatchmakingDialog();
	}
}

void UMatchmakingContext::OnMatchmakingStateChangeInternal(EMatchmakingState::Type OldState, EMatchmakingState::Type NewState, const FMMAttemptState& MMState)
{
	OnMatchmakingStateChange.Broadcast(OldState, NewState);
}

void UMatchmakingContext::OnMatchmakingCompleteInternal(EMatchmakingCompleteResult EndResult)
{
	OnMatchmakingComplete.Broadcast(EndResult);
}

void UMatchmakingContext::OnPartyStateChangeInternal(EUTPartyState NewPartyState)
{
	if (NewPartyState == EUTPartyState::Matchmaking)
	{
		UUTLocalPlayer* LocalPlayer = GetOwningPlayer<UUTLocalPlayer>();
		if (LocalPlayer)
		{
			LocalPlayer->ShowMatchmakingDialog();
		}
	}
	else if (NewPartyState == EUTPartyState::Menus || NewPartyState == EUTPartyState::TravelToServer)
	{
		UUTLocalPlayer* LocalPlayer = GetOwningPlayer<UUTLocalPlayer>();
		if (LocalPlayer)
		{
			LocalPlayer->HideMatchmakingDialog();
		}
	}
	else if (NewPartyState == EUTPartyState::CustomMatch)
	{
		UUTLocalPlayer* LocalPlayer = GetOwningPlayer<UUTLocalPlayer>();
		if (LocalPlayer)
		{
			LocalPlayer->HideMatchmakingDialog();
		}
	}
	else if (NewPartyState == EUTPartyState::QuickMatching)
	{
		UUTLocalPlayer* LocalPlayer = GetOwningPlayer<UUTLocalPlayer>();
		if (LocalPlayer)
		{
			LocalPlayer->ShowMatchmakingDialog();
		}
	}
}

void UMatchmakingContext::StartMatchmaking(int32 InPlaylistId)
{
	UUTLocalPlayer* LocalPlayer = GetOwningPlayer<UUTLocalPlayer>();
	if (LocalPlayer)
	{
		if (!LocalPlayer->IsPartyLeader())
		{
			// Show error dialog
			LocalPlayer->MessageBox(LOCTEXT("NotPartyLeaderTitle", "Must Be The Party Leader"), LOCTEXT("NotPartyLeader", "You must be the party leader to initiate matchmaking."));
			return;
		}

		UUTGameInstance* GameInstance = GetGameInstance<UUTGameInstance>();
		if (GameInstance && GameInstance->GetPlaylistManager())
		{
			int32 TeamCount = 0;
			int32 TeamSize = 0;
			int32 MaxPartySize = 0;
			if (!GameInstance->GetPlaylistManager()->GetMaxTeamInfoForPlaylist(InPlaylistId, TeamCount, TeamSize, MaxPartySize))
			{
				// Playlist probably doesn't exist :/ Show an error dialog
				LocalPlayer->MessageBox(LOCTEXT("NoPlaylistTitle", "Invalid Playlist"), LOCTEXT("NoPlaylist", "This playlist doesn't exist, please choose another."));
				return;
			}

			UPartyContext* PartyContext = Cast<UPartyContext>(UBlueprintContextLibrary::GetContext(LocalPlayer->GetWorld(), UPartyContext::StaticClass()));
			if (PartyContext)
			{
				if (MaxPartySize < PartyContext->GetPartySize())
				{
					// Show error dialog
					LocalPlayer->MessageBox(LOCTEXT("PartyTooLargeTitle", "Party Too Large"), LOCTEXT("PartyTooLarge", "This playlist does not support your party size."));
					return; 
				}
			}
			
		}

		// Also should verify it's an accepted region
		UUTProfileSettings* ProfileSettings = LocalPlayer->GetProfileSettings();
		if (ProfileSettings)
		{
			if (ProfileSettings->MatchmakingRegion.IsEmpty())
			{
				// Show selection dialog
				LocalPlayer->ShowRegionSelectDialog(InPlaylistId);
				return;
			}
		}

		LocalPlayer->StartMatchmaking(InPlaylistId);
	}
}

void UMatchmakingContext::AttemptReconnect(const FString& OldSessionId)
{
	UUTLocalPlayer* LocalPlayer = GetOwningPlayer<UUTLocalPlayer>();
	if (LocalPlayer)
	{
		if (!LocalPlayer->IsPartyLeader())
		{
			// Show error dialog
			LocalPlayer->MessageBox(LOCTEXT("ReconnectNotPartyLeaderTitle", "Must Be The Party Leader"), LOCTEXT("ReconnectNotPartyLeader", "You must be the party leader to initiate matchmaking."));
			return;
		}

		LocalPlayer->AttemptMatchmakingReconnect(OldSessionId);
	}
}

#undef LOCTEXT_NAMESPACE