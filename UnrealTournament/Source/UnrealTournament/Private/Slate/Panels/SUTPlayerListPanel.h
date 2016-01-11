// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Slate/SlateGameResources.h"
#include "../SUWindowsStyle.h"
#include "../SUTStyle.h"
#include "TAttributeProperty.h"
#include "UTLobbyMatchInfo.h"
#include "../Widgets/SUTMenuAnchor.h"
#include "UTGameState.h"

#if !UE_SERVER

namespace ETrackedPlayerType
{
	enum Type
	{
		Player,
		MatchHeader,
		EveryoneHeader, 
		InstancePlayer,
		InstanceHeader,
		Spectator,
		MAX,
	};
};


class FTrackedPlayer : public TSharedFromThis<FTrackedPlayer>
{
public:
	// What type of cell is this.
	ETrackedPlayerType::Type EntryType;

	// The Unique Id of this player
	FUniqueNetIdRepl PlayerID;

	// This players Name
	FString PlayerName;

	// The ID of this player's Avatar
	FName Avatar;

	// Will be true if this player is in the same match as the owner
	bool bIsInMatch;

	// Will be true if this player is in any match
	bool bIsInAnyMatch;

	// Will be true if this player is the host of a match (only useful in hubs)
	bool bIsHost;

	// Will be true if this player is the owner
	bool bIsOwner;

	// Runtime.. each frame this will be set to pending kill and cleared if we are still relevant
	bool bPendingKill;

	// Will be true if this player is in an instance
	bool bInInstance;

	// This is the last known team num.  This will be used to resort the array when a player's team has changed. 
	uint8 TeamNum;

	bool bIsSpectator;

	float SortOrder;

	TWeakObjectPtr<AUTPlayerState> PlayerState;

	FTrackedPlayer(FString inHeaderText, ETrackedPlayerType::Type inEntryType)
		: EntryType(inEntryType)
		, PlayerName(inHeaderText)
	{
		bPendingKill = false;
		TeamNum = 255;
		bIsInMatch = false;
		bIsHost = false;
		bInInstance = false;
	}

	FTrackedPlayer(TWeakObjectPtr<AUTPlayerState> inPlayerState, FUniqueNetIdRepl inPlayerID, const FString& inPlayerName, uint8 inTeamNum, FName inAvatar, bool inbIsOwner, bool inbIsHost, bool inbIsSpectator)
		: PlayerID(inPlayerID)
		, PlayerName(inPlayerName)
		, Avatar(inAvatar)
		, bIsHost(inbIsHost)
		, bIsOwner(inbIsOwner)
		, TeamNum(inTeamNum)
		, bIsSpectator(inbIsSpectator)
		, PlayerState(inPlayerState)
	{
		bPendingKill = false;
		TeamNum = 255;
		bIsInMatch = false;
		bIsHost = false;
		EntryType = ETrackedPlayerType::Player;
		bInInstance = inPlayerState == NULL;
	}

	static TSharedRef<FTrackedPlayer> Make(TWeakObjectPtr<AUTPlayerState> inPlayerState, FUniqueNetIdRepl inPlayerID, const FString& inPlayerName, uint8 inTeamNum, FName inAvatar, bool inbIsOwner, bool inbIsHost, bool inbIsSpectator)
	{
		return MakeShareable( new FTrackedPlayer(inPlayerState, inPlayerID, inPlayerName, inTeamNum, inAvatar, inbIsOwner, inbIsHost, inbIsSpectator));
	}

	static TSharedRef<FTrackedPlayer> MakeHeader(FString inHeaderText, ETrackedPlayerType::Type inEntryType)
	{
		return MakeShareable( new FTrackedPlayer(inHeaderText, inEntryType));
	}

	FText GetPlayerName()
	{
		return PlayerState.IsValid() ? FText::FromString(PlayerState->PlayerName) : FText::FromString(PlayerName);
	}

	FSlateColor GetNameColor() const
	{
		if (PlayerState.IsValid() && PlayerState->bIsRconAdmin)
		{
			return FSlateColor(FLinearColor::Yellow);
		}

		if (bIsInMatch && !bIsSpectator)
		{
			if (TeamNum == 0) return FSlateColor(FLinearColor(1.0f, 0.05f, 0.0f, 1.0f));
			else if (TeamNum == 1) return FSlateColor(FLinearColor(0.1f, 0.1f, 1.0f, 1.0f));
		}

		return FSlateColor(FLinearColor::Gray);
	}

	FText GetLobbyStatusText()
	{
		if (PlayerState.IsValid() && PlayerState->bIsRconAdmin)
		{
			return NSLOCTEXT("Generic","Admin","ADMIN");
		}

		if (bIsInMatch && PlayerState.IsValid())
		{
			if (bIsHost) 
			{
				return NSLOCTEXT("Generic","Host","HOST");
			}

			bool bReadyToPlay = (PlayerState->bReadyToPlay || PlayerState->GetWorld()->GetGameState<AUTGameState>()->HasMatchStarted());

			return bReadyToPlay ? NSLOCTEXT("Generic","Ready","READY") : NSLOCTEXT("Generic","NotReady","NOT READY");
		}

		return FText::GetEmpty();
	}

	const FSlateBrush* GetAvatar() const
	{
		if (Avatar == NAME_None) 
		{
			return SUTStyle::Get().GetBrush("UT.NoStyle");
		}
		else
		{
			return SUTStyle::Get().GetBrush(Avatar);			
		}
	}
};

DECLARE_DELEGATE_OneParam(FPlayerClicked, FUniqueNetIdRepl);

class SUTTextChatPanel;
class AUTLobbyMatchInfo;

class UNREALTOURNAMENT_API SUTPlayerListPanel : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SUTPlayerListPanel)
	{}
		SLATE_ARGUMENT( TWeakObjectPtr<UUTLocalPlayer>, PlayerOwner )
		SLATE_EVENT(FPlayerClicked, OnPlayerClicked)
	SLATE_END_ARGS()


public:	
	/** needed for every widget */
	void Construct(const FArguments& InArgs);
	void ForceRefresh();

	TSharedPtr<SUTTextChatPanel> ConnectedChatPanel;

protected:

	void CheckFlags(bool &bIsHost, bool &bIsTeamGame);

	TSharedPtr<FTrackedPlayer> MatchHeader;
	TSharedPtr<FTrackedPlayer> SpectatorHeader;
	TSharedPtr<FTrackedPlayer> EveryoneHeader;
	TSharedPtr<FTrackedPlayer> InstanceHeader;

	TSharedPtr<SOverlay> InviteOverlay;
	TSharedPtr<SVerticalBox> InviteBox;

	TWeakObjectPtr<AUTLobbyMatchInfo> InviteInfo;

	bool bNeedsRefresh;

	// The Player Owner that owns this panel
	TWeakObjectPtr<UUTLocalPlayer> PlayerOwner;


	TArray<TSharedPtr<FTrackedPlayer>> TrackedPlayers;
	TSharedRef<ITableRow> OnGenerateWidgetForPlayerList( TSharedPtr<FTrackedPlayer> InItem, const TSharedRef<STableViewBase>& OwnerTable );
	TSharedPtr<SListView<TSharedPtr<FTrackedPlayer>>> PlayerList;

	// Checks to see if a player id is being tracked.  Returns the index into the TrackedPlayer's array or INDEX_NONE
	int32 IsTracked(const FUniqueNetIdRepl& PlayerID);

	// Every frame check the status of the match and update.
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime );

	TSharedRef<SWidget> GetPlayerPortrait(TSharedPtr<FTrackedPlayer> Player);

	// Returns true if a given player is in the owner's match
	bool IsInMatch(AUTPlayerState* PlayerState);

	// Returns true if this player should be shown in the list.

	bool ShouldShowPlayer(FUniqueNetIdRepl PlayerId, uint8 TeamNum, bool bIsInMatch);

	FPlayerClicked PlayerClickedDelegate;

	FReply OnListSelect(TSharedPtr<FTrackedPlayer> Selected);

	void GetMenuContent(FString SearchTag, TArray<FMenuOptionData>& MenuOptions);

	void OnSubMenuSelect(FName Tag, TSharedPtr<FTrackedPlayer> InItem);

	void BuildInvite();

	FReply OnMatchInviteAction(bool bAccept);

};

#endif
