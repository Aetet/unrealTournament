// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTOnlineGameSettingsBase.h"
#include "SlateBasics.h"
#include "../Base/SUTWindowBase.h"

#if !UE_SERVER

const int32 PING_ALLOWANCE = 30;
const int32 PLAYER_ALLOWANCE = 5;

class FServerSearchInfo
{
public:
	FOnlineSessionSearchResult SearchResult;
	int32 Ping;
	int32 NoPlayers;

	int32 ServerTrustLevel;
	bool bServerIsTrainingGround;

	TWeakObjectPtr<AUTServerBeaconClient> Beacon;

	bool bHasFriends;

	FServerSearchInfo(const FOnlineSessionSearchResult& inSearchResult, int32 inPing, int32 inNoPlayers)
	{
		SearchResult = inSearchResult;
		Ping = inPing;
		NoPlayers = inNoPlayers;
		bHasFriends = false;
		
		if (SearchResult.IsValid())
		{
			SearchResult.Session.SessionSettings.Get(SETTING_TRUSTLEVEL, ServerTrustLevel);
			SearchResult.Session.SessionSettings.Get(SETTING_TRAININGGROUND, bServerIsTrainingGround);

			if (ServerTrustLevel > 0) bServerIsTrainingGround = false;
		}
	}

	static TSharedRef<FServerSearchInfo> Make(const FOnlineSessionSearchResult& inSearchResult, int32 inPing, int32 inNoPlayers)
	{
		return MakeShareable(new FServerSearchInfo(inSearchResult, inPing, inNoPlayers));
	}

};

struct FInstanceTracker
{
public:
	TSharedPtr<FServerSearchInfo> ServerData;
	TSharedPtr<FServerInstanceData> InstanceData;

	FInstanceTracker(TSharedPtr<FServerSearchInfo> inServerData,TSharedPtr<FServerInstanceData> inInstanceData)
		: ServerData(inServerData)
		, InstanceData(inInstanceData)
	{
	}

	int32 GetPing()
	{
		return ServerData.IsValid() ? ServerData->Ping : MAX_int32;
	}

	bool HasMatchStarted()
	{
		return InstanceData.IsValid() ? InstanceData->MatchData.bMatchHasBegun : false;
	}

	bool HasMatchEnded()
	{
		return InstanceData.IsValid() ? InstanceData->MatchData.bMatchHasBegun : false;
	}

};

struct FServerSearchPingTracker
{
	TSharedPtr<FServerSearchInfo> Server;
	TWeakObjectPtr<AUTServerBeaconClient> Beacon;
	float PingStartTime;

	FServerSearchPingTracker(TSharedPtr<FServerSearchInfo> inServer, AUTServerBeaconClient* inBeacon, float inPingStartTime)
		: Server(inServer)
		, Beacon(inBeacon)
		, PingStartTime(inPingStartTime)
	{
	};
};


class UNREALTOURNAMENT_API SUTQuickMatchWindow : public SUTWindowBase
{
public:

	SLATE_BEGIN_ARGS(SUTQuickMatchWindow)
	{}

	SLATE_ARGUMENT(FString, QuickMatchType)
	SLATE_END_ARGS()

public:

	~SUTQuickMatchWindow();

	/** needed for every widget */
	void Construct(const FArguments& InArgs, TWeakObjectPtr<UUTLocalPlayer> InPlayerOwner);
	virtual void BuildWindow();

	// Called to cancel the quickmatch process
	void Cancel();

	virtual void TellSlateIWantKeyboardFocus();

	virtual void OnClosed();


protected:
	bool bWaitingForMatch;

	/** Holds a reference to the SCanvas widget that makes up the dialog */
	TSharedPtr<SCanvas> Canvas;

	/** Holds a reference to the SOverlay that defines the content for this dialog */
	TSharedPtr<SOverlay> WindowContent;


	TWeakObjectPtr<AUTBaseGameMode> DefaultGameModeObject;

private:

	bool bCancelQuickmatch;

	FText MinorStatusText;
	bool bSearchInProgress;
	
	FDelegateHandle OnFindSessionCompleteHandle;
	FDelegateHandle OnCancelFindSessionCompleteHandle;

	TSharedPtr<class FUTOnlineGameSearchBase> SearchSettings;
	TWeakObjectPtr<class UUTLocalPlayer> PlayerOwner;
	FString QuickMatchType;
	float StartTime;

	FText GetStatusText() const;
	FText GetMinorStatusText() const;

	// This should hold the highest rank of anyone in the party, or the player's rank if partyless
	int32 MatchTargetRank;

	// Begin the search for a HUB to join
	void BeginQuickmatch();
	void OnInitialFindCancel(bool bWasSuccessful);
	void OnSearchCancelled(bool bWasSuccessful);

	virtual void OnServerBeaconResult(AUTServerBeaconClient* Sender, FServerBeaconInfo ServerInfo);
	virtual void OnServerBeaconFailure(AUTServerBeaconClient* Sender);
	void PingNextBatch();
	virtual void PingServer(TSharedPtr<FServerSearchInfo> ServerToPing);

	void OnFindSessionsComplete(bool bWasSuccessful);

	// Hold a list of instances sorted by their server's position.  
	TArray<FInstanceTracker> Instances;

	// Contains a list of all possible server
	TArray<TSharedPtr<FServerSearchInfo>> ServerList;

	// Contains a list of servers that returned a ping
	TArray<TSharedPtr<FServerSearchInfo>> FinalList;

	TArray<FServerSearchPingTracker> PingTrackers;

	void NoAvailableMatches();
	void CollectInstances();
	void FindBestMatch();

	virtual bool SupportsKeyboardFocus() const override;

	FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);
	FReply OnCancelClick();

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime );

public:
	virtual void AttemptQuickMatch(TSharedPtr<FServerSearchInfo> DesiredServer, TSharedPtr<FServerInstanceData> DesiredInstance);
	void FindHUBToJoin();

protected:
	virtual void RequestQuickPlayResults(AUTServerBeaconClient* Beacon, const FName& CommandCode, const FString& InstanceGuid);
	virtual void RequestJoinInstanceResult(EInstanceJoinResult::Type Result, const FString& Params);

	TSharedPtr<FServerSearchInfo> ConnectingServer;
	TSharedPtr<FServerInstanceData> ConnectingInstance;

	bool HasFriendsInInstances(const TArray<TSharedPtr<FServerInstanceData>>& Instances, TWeakObjectPtr<UUTLocalPlayer> LocalPlayer);
	int32 CountFriendsInInstance(const TArray<FUTFriend>& FriendsList, TSharedPtr<FServerInstanceData> InstanceToCheck, TWeakObjectPtr<UUTLocalPlayer> LocalPlayer);

	bool bWaitingForResponseFromHub;
	float HubResponseWaitTime;

private:
	int32 QMStats_NumHubsConsidered;
	int32 QMStats_NumInstancesConsidered;
	int32 QMStats_NumRejectedForRank;
	int32 QMStats_NumRejectedForPartySize;
	int32 QMStats_NumRejectedForGameType;
	int32 QMStats_NumRejectedForJoinable;
	int32 QMStats_NumPingFailures;
	int32 QMStats_NumInstancesSpooled;
	int32 QMStats_NumAttemptedJoins;
	int32 QMStats_RankCheck;
	FString QMStats_FinalResult;


};

#endif