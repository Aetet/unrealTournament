// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "../Public/UnrealTournament.h"
#include "../Public/UTLocalPlayer.h"
#include "SUTQuickMatch.h"
#include "SUWindowsStyle.h"
#include "UTOnlineGameSearchBase.h"
#include "UTOnlineGameSettingsBase.h"
#include "OnlineSubsystemTypes.h"
#include "UTGameEngine.h"
#include "UTServerBeaconClient.h"
#include "Engine/UserInterfaceSettings.h"


#if !UE_SERVER

void SUTQuickMatch::Construct(const FArguments& InArgs)
{
	PlayerOwner = InArgs._PlayerOwner;
	QuickMatchType = InArgs._QuickMatchType;

	checkSlow(PlayerOwner != NULL);

	StartTime = PlayerOwner->GetWorld()->GetTimeSeconds();

	FVector2D ViewportSize;
	GetPlayerOwner()->ViewportClient->GetViewportSize(ViewportSize);

	FVector2D DesignedRez(1920,1920 * (ViewportSize.Y / ViewportSize.X));
	FVector2D DesignedSize(800,220);
	FVector2D Pos = (DesignedRez * 0.5f) - (DesignedSize * 0.5f);
	ChildSlot
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			SAssignNew(Canvas,SCanvas)

			// We use a Canvas Slot to position and size the dialog.  
			+SCanvas::Slot()
			.Position(Pos)
			.Size(DesignedSize)
			.VAlign(VAlign_Top)
			.HAlign(HAlign_Left)
			[
				// This is our primary overlay.  It controls all of the various elements of the dialog.  This is not
				// the content overlay.  This comes below.
				SNew(SOverlay)				

				// this is the background image
				+SOverlay::Slot()							
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.VAlign(VAlign_Fill)
					.HAlign(HAlign_Fill)
					[
						SNew(SImage)
						.Image(SUWindowsStyle::Get().GetBrush("UT.Dialog.Background"))
					]
				]

				// This will define a vertical box that holds the various components of the dialog box.
				+ SOverlay::Slot()							
				[
					SNew(SVerticalBox)

					// The title bar
					+ SVerticalBox::Slot()						
					.Padding(0.0f, 5.0f, 0.0f, 5.0f)
					.AutoHeight()
					.VAlign(VAlign_Top)
					.HAlign(HAlign_Center)
					[
						SNew(SBox)
						.HeightOverride(46)
						[
							SNew(STextBlock)
							.Text(NSLOCTEXT("QuickMatch","SearchingForGame","FINDING A GAME TO JOIN"))
							.TextStyle(SUWindowsStyle::Get(), "UT.Dialog.TitleTextStyle")
						]
					]
					+ SVerticalBox::Slot()
					.Padding(12.0f, 25.0f, 12.0f, 25.0f)
					.AutoHeight()
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text(this, &SUTQuickMatch::GetStatusText)
							.TextStyle(SUWindowsStyle::Get(), "UT.Dialog.BodyTextStyle")
						]
						+ SVerticalBox::Slot()
						.HAlign(HAlign_Center)
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(this, &SUTQuickMatch::GetMinorStatusText)
							.TextStyle(SUWindowsStyle::Get(), "UT.Dialog.BodyTextTiny")
						]
						+SVerticalBox::Slot()
						.HAlign(HAlign_Center)
						.AutoHeight()
						.Padding(0.0f,10.0f,0.0f,0.0f)
						[
							SNew(SButton)
							.ButtonStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button")
							//.OnClicked(this, &SUWindowsMainMenu::OnTutorialClick)
							.ContentPadding(FMargin(25.0, 0.0, 25.0, 5.0))
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(NSLOCTEXT("QuickMatchg", "CancelText", "ESC to Cancel").ToString())
									.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
								]
							]
						]
					]

				]
			]
		];


	FindHUBToJoin();

}

FText SUTQuickMatch::GetStatusText() const
{
	int32 DeltaSeconds = int32(PlayerOwner->GetWorld()->GetTimeSeconds() - StartTime);
	return FText::Format(NSLOCTEXT("QuickMatch","StatusFormatStr","Searching for a game... ({0})"), FText::AsNumber(DeltaSeconds));
}

FText SUTQuickMatch::GetMinorStatusText() const
{
	return MinorStatusText;
}


void SUTQuickMatch::FindHUBToJoin()
{
	OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem) OnlineIdentityInterface = OnlineSubsystem->GetIdentityInterface();
	if (OnlineSubsystem) OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();

	if (OnlineSessionInterface.IsValid())
	{
		// Setup our Find complete Delegate
		FOnFindSessionsCompleteDelegate Delegate;
		Delegate.BindSP(this, &SUTQuickMatch::OnFindSessionsComplete);
		OnFindSessionCompleteHandle = OnlineSessionInterface->AddOnFindSessionsCompleteDelegate_Handle(Delegate);

		// Now look for official hubs

		SearchSettings = MakeShareable(new FUTOnlineGameSearchBase(false));
		SearchSettings->MaxSearchResults = 10000;
		FString GameVer = FString::Printf(TEXT("%i"), GetDefault<UUTGameEngine>()->GameNetworkVersion);
		SearchSettings->QuerySettings.Set(SETTING_SERVERVERSION, GameVer, EOnlineComparisonOp::Equals);			// Must equal the game version

		SearchSettings->QuerySettings.Set(SETTING_GAMEINSTANCE, 1, EOnlineComparisonOp::NotEquals);				// Must not be a lobby server instance

		FString GameMode = TEXT("/Script/UnrealTournament.UTLobbyGameMode");
		SearchSettings->QuerySettings.Set(SETTING_GAMEMODE, GameMode, EOnlineComparisonOp::Equals);				// Must be a lobby server
		
		// TODO: Add the search setting for TrustLevel

		TSharedRef<FUTOnlineGameSearchBase> SearchSettingsRef = SearchSettings.ToSharedRef();

		OnlineSessionInterface->FindSessions(0, SearchSettingsRef);
		bSearchInProgress = true;

		MinorStatusText = NSLOCTEXT("QuickMatch","Status_GettingServerList","Retreiving Server List from MCP...");
	}
}


void SUTQuickMatch::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		// Itterate through all of the hubs and first the best one...

		if (SearchSettings->SearchResults.Num() > 0)
		{
			for (int32 ServerIndex = 0; ServerIndex < SearchSettings->SearchResults.Num(); ServerIndex++)
			{
				int32 NoPlayers;
				SearchSettings->SearchResults[ServerIndex].Session.SessionSettings.Get(SETTING_PLAYERSONLINE, NoPlayers);
				TSharedRef<FServerSearchInfo> NewServer = FServerSearchInfo::Make(SearchSettings->SearchResults[ServerIndex],0, NoPlayers);
				ServerList.Add(NewServer);
			}

			if (ServerList.Num() > 0)
			{
				PingNextBatch();
				return;
			}
		}
	}
	// We get here, we just force the find best match call.  This will fail and error out but insures and clean up happens
	FindBestMatch();
}

void SUTQuickMatch::NoAvailableMatches()
{
	PlayerOwner->CancelQuickMatch();
	PlayerOwner->MessageBox(NSLOCTEXT("QuickMatch", "NoServersTitle", "ONLINE FAILURE"), NSLOCTEXT("QuickMatch", "NoServerTitle", "The Online System is down for maintence, please try again in a few minutes."));
}

void SUTQuickMatch::PingNextBatch()
{
	if (ServerList.Num() == 0 && PingTrackers.Num() == 0)
	{
		FindBestMatch();
	}

	while (ServerList.Num() > 0 && PingTrackers.Num() < 10)
	{
		int32 Cnt = ServerList.Num() + PingTrackers.Num();
		MinorStatusText = FText::Format( NSLOCTEXT("QuickMatch","Status_PingingFormat","Pinging Servers ({0})"), FText::AsNumber(Cnt) );
		PingServer(ServerList[0]);
		ServerList.RemoveAtSwap(0);
	}
}

void SUTQuickMatch::PingServer(TSharedPtr<FServerSearchInfo> ServerToPing)
{
	// Build the beacon
	AUTServerBeaconClient* Beacon = PlayerOwner->GetWorld()->SpawnActor<AUTServerBeaconClient>(AUTServerBeaconClient::StaticClass());
	if (Beacon)
	{
		FString BeaconIP;
		OnlineSessionInterface->GetResolvedConnectString(ServerToPing->SearchResult, FName(TEXT("BeaconPort")), BeaconIP);
		FString BeaconNetDriverName = FString::Printf(TEXT("BeaconDriver%s"), *BeaconIP);
		
		Beacon->SetBeaconNetDriverName(BeaconNetDriverName);
		Beacon->OnServerRequestResults = FServerRequestResultsDelegate::CreateSP(this, &SUTQuickMatch::OnServerBeaconResult);
		Beacon->OnServerRequestFailure = FServerRequestFailureDelegate::CreateSP(this, &SUTQuickMatch::OnServerBeaconFailure);
		FURL BeaconURL(nullptr, *BeaconIP, TRAVEL_Absolute);
		Beacon->InitClient(BeaconURL);
		PingTrackers.Add(FServerSearchPingTracker(ServerToPing, Beacon));
	}
}

void SUTQuickMatch::OnServerBeaconFailure(AUTServerBeaconClient* Sender)
{
	for (int32 i = 0; i < PingTrackers.Num(); i++)
	{
		if (PingTrackers[i].Beacon == Sender)
		{
			// Server is not responding, so ignore it...
			PingTrackers[i].Beacon->DestroyBeacon();
			PingTrackers.RemoveAt(i, 1);

			PingNextBatch();
		}
	}
}

void SUTQuickMatch::OnServerBeaconResult(AUTServerBeaconClient* Sender, FServerBeaconInfo ServerInfo)
{
	for (int32 i = 0; i < PingTrackers.Num(); i++)
	{
		if (PingTrackers[i].Beacon == Sender)
		{
			PingTrackers[i].Server->Ping = Sender->Ping;

			// Insert sort it in to the final list of servers by ping.
			
			bool bInserted = false;
			for (int32 Idx=0; Idx < FinalList.Num(); Idx++)
			{
				if (FinalList[Idx]->Ping > Sender->Ping)
				{
					// Insert here..

					FinalList.Insert(PingTrackers[i].Server, Idx);
					bInserted = true;
					break;
				}
			}

			if (!bInserted) FinalList.Add(PingTrackers[i].Server);

			PingTrackers[i].Beacon->DestroyBeacon();
			PingTrackers.RemoveAt(i, 1);

			// Look to see if there are more servers to Ping...
			PingNextBatch();
		}
	}
}

void SUTQuickMatch::FindBestMatch()
{
	if (FinalList.Num() > 0)
	{
		// We know the first server has the best ping and is the oldest server (most likely to have players).  So now search forward to find a server within
		// 40ms of this ping that has the most players.

		TSharedPtr<FServerSearchInfo> BestServer = FinalList[0];

		for (int32 i=1;i<FinalList.Num();i++)
		{
			if (FinalList[i]->Ping - FinalList[0]->Ping < 40)
			{
				if (BestServer->NoPlayers < FinalList[i]->NoPlayers)
				{
					BestServer = FinalList[i];
				}
			}
			else
			{
				break;
			}
		
		}

		PlayerOwner->CancelQuickMatch();
		PlayerOwner->JoinSession(BestServer->SearchResult, false, QuickMatchType);
	}
	else
	{
		NoAvailableMatches();
	}
}

void SUTQuickMatch::Cancel()
{
	if (OnlineSessionInterface.IsValid())
	{
		OnlineSessionInterface->ClearOnCancelFindSessionsCompleteDelegate_Handle(OnFindSessionCompleteHandle);
	}

}




#endif