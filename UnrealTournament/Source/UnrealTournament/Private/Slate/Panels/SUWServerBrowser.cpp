// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLocalPlayer.h"
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "Slate/SlateGameResources.h"
#include "../SUWPanel.h"
#include "../SUWindowsDesktop.h"
#include "../SUWindowsStyle.h"
#include "../SUTStyle.h"
#include "SUWServerBrowser.h"
#include "Online.h"
#include "UTBaseGameMode.h"
#include "UTOnlineGameSearchBase.h"
#include "UTOnlineGameSettingsBase.h"
#include "OnlineSubsystemTypes.h"
#include "../SUWMessageBox.h"
#include "../SUWInputBox.h"
#include "UTGameEngine.h"
#include "UTServerBeaconClient.h"
#include "../SUWScaleBox.h"
#include "Engine/UserInterfaceSettings.h"
#include "UnrealNetwork.h"

#if !UE_SERVER
/** List Sort helpers */

struct FCompareServerByName		{FORCEINLINE bool operator()( const TSharedPtr< FServerData > A, const TSharedPtr< FServerData > B ) const {return ( A->Name < B->Name);}};
struct FCompareServerByNameDesc	{FORCEINLINE bool operator()( const TSharedPtr< FServerData > A, const TSharedPtr< FServerData > B ) const {return ( A->Name > B->Name);}};

struct FCompareServerByIP		{FORCEINLINE bool operator()( const TSharedPtr< FServerData > A, const TSharedPtr< FServerData > B ) const {return ( A->IP < B->IP);}};
struct FCompareServerByIPDesc	{FORCEINLINE bool operator()( const TSharedPtr< FServerData > A, const TSharedPtr< FServerData > B ) const {return ( A->IP > B->IP);}};

struct FCompareServerByGameMode		{FORCEINLINE bool operator()( const TSharedPtr< FServerData > A, const TSharedPtr< FServerData > B ) const {return ( A->GameModePath > B->GameModePath);}};
struct FCompareServerByGameModeDesc	{FORCEINLINE bool operator()( const TSharedPtr< FServerData > A, const TSharedPtr< FServerData > B ) const {return ( A->GameModePath > B->GameModePath);}};

struct FCompareServerByMap		{FORCEINLINE bool operator()( const TSharedPtr< FServerData > A, const TSharedPtr< FServerData > B ) const	{ return ( A->Map < B->Map); }};
struct FCompareServerByMapDesc	{FORCEINLINE bool operator()( const TSharedPtr< FServerData > A, const TSharedPtr< FServerData > B ) const	{ return ( A->Map > B->Map); }};

struct FCompareServerByNumPlayers		{FORCEINLINE bool operator()( const TSharedPtr< FServerData > A, const TSharedPtr< FServerData > B ) const { return ( A->NumPlayers < B->NumPlayers);}};
struct FCompareServerByNumPlayersDesc	{FORCEINLINE bool operator()( const TSharedPtr< FServerData > A, const TSharedPtr< FServerData > B ) const { return ( A->NumPlayers > B->NumPlayers);}};

struct FCompareServerByNumSpectators		{FORCEINLINE bool operator()( const TSharedPtr< FServerData > A, const TSharedPtr< FServerData > B ) const {return ( A->NumSpectators < B->NumSpectators);}};
struct FCompareServerByNumSpectatorsDesc	{FORCEINLINE bool operator()( const TSharedPtr< FServerData > A, const TSharedPtr< FServerData > B ) const {return ( A->NumSpectators> B->NumSpectators);}};

struct FCompareServerByNumFriends { FORCEINLINE bool operator()(const TSharedPtr< FServerData > A, const TSharedPtr< FServerData > B) const { return (A->NumFriends < B->NumFriends); } };
struct FCompareServerByNumFriendsDesc { FORCEINLINE bool operator()(const TSharedPtr< FServerData > A, const TSharedPtr< FServerData > B) const { return (A->NumFriends > B->NumFriends); } };

struct FCompareServerByVersion		{FORCEINLINE bool operator()( const TSharedPtr< FServerData > A, const TSharedPtr< FServerData > B ) const {return ( A->Version < B->Version);}};
struct FCompareServerByVersionDesc	{FORCEINLINE bool operator()( const TSharedPtr< FServerData > A, const TSharedPtr< FServerData > B ) const {return ( A->Version > B->Version);}};

struct FCompareServerByPing		
{
	FORCEINLINE bool operator()	( const TSharedPtr< FServerData > A, const TSharedPtr< FServerData > B ) const 
	{
		return (B->Ping < 0 || (A->Ping >= 0 && A->Ping < B->Ping));	
	}
};

struct FCompareServerByPingDesc	
{
	FORCEINLINE bool operator()( const TSharedPtr< FServerData > A, const TSharedPtr< FServerData > B ) const 
	{
		return ( A->Ping < 0 || (B->Ping >= 0 && A->Ping > B->Ping));	
	}
};

struct FCompareHub
{
	FORCEINLINE bool operator()	( const TSharedPtr< FServerData > A, const TSharedPtr< FServerData > B ) const 
	{
		// Sorts like this.. First by trust level (where Epic = 0, Trusted = 1 and wild west = 2.. grr)
		// then by ping.  So an Epic hub with Ping of 35ms vs a trusted hub with a ping of 250ms vs a wild west hub with a ping of 11ms would be
		// 0.035 vs 100.250 vs 1000.011

		float AValue = ( (A->TrustLevel == 0) ? 0.0f : ( (A->TrustLevel == 1) ? 100.0f : 1000.0f) ) + (float(A->Ping) / 1000.0f);
		float BValue = ( (B->TrustLevel == 0) ? 0.0f : ( (B->TrustLevel == 1) ? 100.0f : 1000.0f) ) + (float(B->Ping) / 1000.0f);
		return AValue < BValue;
	}
};

struct FCompareRulesByRule		{FORCEINLINE bool operator()( const TSharedPtr< FServerRuleData > A, const TSharedPtr< FServerRuleData > B ) const {return ( A->Rule > B->Rule);	}};
struct FCompareRulesByRuleDesc	{FORCEINLINE bool operator()( const TSharedPtr< FServerRuleData > A, const TSharedPtr< FServerRuleData > B ) const {return ( A->Rule < B->Rule);	}};
struct FCompareRulesByValue		{FORCEINLINE bool operator()( const TSharedPtr< FServerRuleData > A, const TSharedPtr< FServerRuleData > B ) const {return ( A->Value > B->Value);	}};
struct FCompareRulesByValueDesc	{FORCEINLINE bool operator()( const TSharedPtr< FServerRuleData > A, const TSharedPtr< FServerRuleData > B ) const {return ( A->Value < B->Value);	}};

struct FComparePlayersByName		{FORCEINLINE bool operator()( const TSharedPtr< FServerPlayerData > A, const TSharedPtr< FServerPlayerData > B ) const {return ( A->PlayerName > B->PlayerName);	}};
struct FComparePlayersByNameDesc	{FORCEINLINE bool operator()( const TSharedPtr< FServerPlayerData > A, const TSharedPtr< FServerPlayerData > B ) const {return ( A->PlayerName < B->PlayerName);	}};
struct FComparePlayersByScore		{FORCEINLINE bool operator()( const TSharedPtr< FServerPlayerData > A, const TSharedPtr< FServerPlayerData > B ) const {return ( A->Score > B->Score);	}};
struct FComparePlayersByScoreDesc	{FORCEINLINE bool operator()( const TSharedPtr< FServerPlayerData > A, const TSharedPtr< FServerPlayerData > B ) const {return ( A->Score < B->Score);	}};

SUWServerBrowser::~SUWServerBrowser()
{
	if (PlayerOwner.IsValid())
	{
		PlayerOwner->RemovePlayerOnlineStatusChangedDelegate(PlayerOnlineStatusChangedDelegate);
		PlayerOnlineStatusChangedDelegate.Reset();
	}
}

void SUWServerBrowser::ConstructPanel(FVector2D ViewportSize)
{
	Tag = FName(TEXT("ServerBrowser"));


	bWantsAFullRefilter = false;
	bHideUnresponsiveServers = true;	
	bShowingHubs = false;
	bAutoRefresh = false;
	TSharedRef<SScrollBar> ExternalScrollbar = SNew(SScrollBar);

	OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem) OnlineIdentityInterface = OnlineSubsystem->GetIdentityInterface();
	if (OnlineSubsystem) OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();

	this->ChildSlot
	[
		SNew(SBorder)
		.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Dark"))
		[
			SNew( SVerticalBox )
		
			 //Login panel

			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(64)
				[
					SNew( SHorizontalBox )
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.Padding(0.0f,0.0f,15.0f,0.0f)
					.AutoWidth()
					[
						SAssignNew( ServerListControlBox, SHorizontalBox )
						.Visibility(EVisibility::Collapsed)
					]

					+SHorizontalBox::Slot()
					.Padding(0.0f, 0.0f, 0.0f, 0.0f)
					.VAlign(VAlign_Fill)
					.HAlign(HAlign_Fill)
					[
						SNew(SBorder)
						.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Medium"))
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.VAlign(VAlign_Center)
							.HAlign(HAlign_Fill)
							.Padding(15.0f, 0.0f, 16.0f, 0.0f)
							[
								SNew(SHorizontalBox)
								+SHorizontalBox::Slot()
								.VAlign(VAlign_Center)
								.AutoWidth()
								[
									SNew(STextBlock)
									.Text(NSLOCTEXT("SUWServerBrowser","QuickFilter","Filter Results by:"))
									.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
								]
								+SHorizontalBox::Slot()
								.Padding(16.0f, 0.0f, 0.0f, 0.0f)
								.FillWidth(1.0)
								[
									SNew(SBox)
									.HeightOverride(36)
									[

										SNew(SOverlay)
										+SOverlay::Slot()
										[
											SNew(SVerticalBox)
											+SVerticalBox::Slot()
											.Padding(8.0, 5.0)
											[
												SAssignNew(FilterMsg, STextBlock)
												.Text(FText::FromString(TEXT("type your filter text here")))
												.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
												.ColorAndOpacity(FLinearColor(1.0,1.0,1.0,0.3))
											]
										]
										+SOverlay::Slot()
										[
											SAssignNew(QuickFilterText, SEditableTextBox)
											.Style(SUTStyle::Get(),"UT.EditBox")
											.OnTextCommitted(this, &SUWServerBrowser::OnQuickFilterTextCommited)
											.OnTextChanged(this, &SUWServerBrowser::OnFilterTextChanged)
											.ClearKeyboardFocusOnCommit(false)
											.MinDesiredWidth(300.0f)
											.Text(FText::GetEmpty())
										]
									]
								]
								+SHorizontalBox::Slot()
								.Padding(16.0f, 0.0f, 0.0f, 0.0f)
								.AutoWidth()
								[
									SAssignNew(HideUnresponsiveServersCheckbox, SCheckBox)
									.Style(SUTStyle::Get(), "UT.CheckBox")
									.ForegroundColor(FLinearColor::White)
									.IsChecked(this, &SUWServerBrowser::ShouldHideUnresponsiveServers)
									.OnCheckStateChanged(this, &SUWServerBrowser::OnHideUnresponsiveServersChanged)
								]
								+SHorizontalBox::Slot()
								.VAlign(VAlign_Center)
								.Padding(16.0f, 0.0f, 30.0f, 0.0f)
								.AutoWidth()
								[
									SNew(STextBlock)
									.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
									.Text(NSLOCTEXT("SUWSeverBrowser", "HideUnresponsive", "Hide Unresponsive Servers"))
								]

							]
						]
					]
				]
			]

			// The Server Browser itself
			+SVerticalBox::Slot()
			.FillHeight(1)
			.HAlign(HAlign_Fill)
			[
				SNew(SOverlay)
				+SOverlay::Slot()
				[
					BuildServerBrowser()
				]

				+SOverlay::Slot()
				[
					BuildLobbyBrowser()
				]
			]

			 //Connect Panel

			+SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			[
				SNew(SBox)
				.HeightOverride(48)
				[
					SNew( SHorizontalBox )
					+SHorizontalBox::Slot().AutoWidth().Padding(10.0f,0.0f,10.0f,0.0f)
					[
						SNew(SBox)
						.VAlign(VAlign_Center)
						[
							// Press rebuild to clear out the old data items and create the new ones (however many are specified by SEditableTextBox)
							SAssignNew(RefreshButton, SUTButton)
							.ButtonStyle(SUTStyle::Get(), "UT.Button.MenuBar")
							.ContentPadding(FMargin(10.0f, 5.0f, 15.0f, 5.0))

							.Text( NSLOCTEXT("SUWServerBrowser","Refresh","Refresh"))
							.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")
							.OnClicked(this, &SUWServerBrowser::OnRefreshClick)
						]
					]
					+SHorizontalBox::Slot().HAlign(HAlign_Fill)
					[
						SNew(SBorder)
						.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Dark"))
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot()
							.VAlign(VAlign_Center)
							.Padding(20.0f, 0.0f, 20.0f, 0.0f)
							[
								SAssignNew(StatusText, STextBlock)
								.Text(this, &SUWServerBrowser::GetStatusText)
								.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small.Bold")
							]
						]
					]

					+SHorizontalBox::Slot().AutoWidth()
					.VAlign(VAlign_Center)
					[
						// Press rebuild to clear out the old data items and create the new ones (however many are specified by SEditableTextBox)
						SAssignNew(SpectateButton, SUTButton)
						.ButtonStyle(SUTStyle::Get(), "UT.Button.MenuBar")
						.ContentPadding(FMargin(10.0f, 5.0f, 10.0f, 5.0))
						.Text(NSLOCTEXT("SUWServerBrowser","Spectate","Spectate"))
						.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")
						.OnClicked(this, &SUWServerBrowser::OnJoinClick,true)
					]


					+SHorizontalBox::Slot().AutoWidth() .VAlign(VAlign_Center)
					.VAlign(VAlign_Center)
					[
						// Press rebuild to clear out the old data items and create the new ones (however many are specified by SEditableTextBox)
						SAssignNew(JoinButton, SUTButton)
						.ButtonStyle(SUTStyle::Get(), "UT.Button.MenuBar")
						.ContentPadding(FMargin(10.0f, 5.0f, 10.0f, 5.0))
						.Text(NSLOCTEXT("SUWServerBrowser","Join","Join"))
						.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")
						.OnClicked(this, &SUWServerBrowser::OnJoinClick,false)
					]
				]
			]
		]

	];

	bDescendingSort = false;
	CurrentSortColumn = FName(TEXT("ServerPing"));

	AllInternetServers.Empty();
	AllHubServers.Empty();

	FilteredServersSource.Empty();
	FilteredHubsSource.Empty();

	InternetServerList->RequestListRefresh();
	HUBServerList->RequestListRefresh();

	if (PlayerOwner->IsLoggedIn())
	{
		SetBrowserState(EBrowserState::BrowserIdle);
	}
	else
	{	
		SetBrowserState(EBrowserState::NotLoggedIn);
	}

	OnRefreshClick();
	
	if (!PlayerOnlineStatusChangedDelegate.IsValid())
	{
		PlayerOnlineStatusChangedDelegate = PlayerOwner->RegisterPlayerOnlineStatusChangedDelegate(FPlayerOnlineStatusChanged::FDelegate::CreateSP(this, &SUWServerBrowser::OwnerLoginStatusChanged));
	}

	UUTGameUserSettings* GS = Cast<UUTGameUserSettings>(GEngine->GetGameUserSettings());
	if (GS)
	{
		const TIndirectArray<SHeaderRow::FColumn>& Columns = HeaderRow->GetColumns();		
		for (int32 i=0;i<Columns.Num();i++)
		{
			if (GS->GetServerBrowserColumnWidth(i) > 0.0)
			{
				HeaderRow->SetColumnWidth(Columns[i].ColumnId, GS->GetServerBrowserColumnWidth(i) );
			}
		}

		if (GS->GetServerBrowserRulesColumnWidth(0) > 0.0)
		{
			RulesHeaderRow->SetColumnWidth(TEXT("Rule"), GS->GetServerBrowserRulesColumnWidth(0));
			RulesHeaderRow->SetColumnWidth(TEXT("Value"), GS->GetServerBrowserRulesColumnWidth(1));
		}

		if (GS->GetServerBrowserPlayersColumnWidth(0) > 0.0)
		{
			PlayersHeaderRow->SetColumnWidth(TEXT("Name"), GS->GetServerBrowserPlayersColumnWidth(0));
			PlayersHeaderRow->SetColumnWidth(TEXT("Score"), GS->GetServerBrowserPlayersColumnWidth(1));
		}

		if (GS->GetServerBrowserSplitterPositions(0) > 0.0)
		{
			VertSplitter->SlotAt(0).SizeValue.Set(GS->GetServerBrowserSplitterPositions(0));
			VertSplitter->SlotAt(1).SizeValue.Set(GS->GetServerBrowserSplitterPositions(1));
		}

		if (GS->GetServerBrowserSplitterPositions(2) > 0.0)
		{
			HorzSplitter->SlotAt(0).SizeValue.Set(GS->GetServerBrowserSplitterPositions(2));
			HorzSplitter->SlotAt(1).SizeValue.Set(GS->GetServerBrowserSplitterPositions(3));
		}
	}	

	BrowserTypeChanged();
	AddGameFilters();
}

TSharedRef<SWidget> SUWServerBrowser::BuildPlayerList()
{
	return SNew(SBorder)
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.Padding(5,0,5,0)
		.FillWidth(1)
		[
			// The list view being tested
			SAssignNew( PlayersList, SListView< TSharedPtr<FServerPlayerData> > )
			// List view items are this tall
			.ItemHeight(24)
			// When the list view needs to generate a widget for some data item, use this method
			.OnGenerateRow( this, &SUWServerBrowser::OnGenerateWidgetForPlayersList )
			.SelectionMode(ESelectionMode::Single)
			.ListItemsSource( &PlayersListSource)

			.HeaderRow
			(
				SAssignNew(PlayersHeaderRow, SHeaderRow) 
				.Style(SUTStyle::Get(), "UT.List.Header")

				+ SHeaderRow::Column("Name")
						.OnSort(this, &SUWServerBrowser::OnPlayerSort)
						.HeaderContent()
						[
							SNew(STextBlock)
								.Text(NSLOCTEXT("SUWServerBrowser","PlayerNameColumn", "Name"))
								.ToolTipText( NSLOCTEXT("SUWServerBrowser","PlayerNameColumnToolTip", "This player's name.") )
								.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
						]

				+ SHeaderRow::Column("Score") 
						.DefaultLabel(NSLOCTEXT("SUWServerBrowser","PlayerScoreColumn", "Score")) 
						.HAlignCell(HAlign_Center) 
						.HAlignHeader(HAlign_Center)
						.OnSort(this, &SUWServerBrowser::OnPlayerSort)
						.HeaderContent()
						[
							SNew(STextBlock)
								.Text(NSLOCTEXT("SUWServerBrowser","PlayerScoreColumn", "IP"))
								.ToolTipText( NSLOCTEXT("SUWServerBrowser","PlayerScoreColumnToolTip", "This player's score.") )
								.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
						]
			)
		]
	];

}

TSharedRef<SWidget> SUWServerBrowser::BuildServerBrowser()
{
	return 	SAssignNew(InternetServerBrowser, SBox) 
		.HeightOverride(500.0f)
		[
			SAssignNew( VertSplitter, SSplitter )
			.Orientation(Orient_Horizontal)
			.OnSplitterFinishedResizing(this, &SUWServerBrowser::VertSplitterResized)

			+ SSplitter::Slot()
			.Value(0.85)
			[
				SNew(SBorder)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.Padding(5,0,5,0)
					.FillWidth(1)
					[
						// The list view being tested
						SAssignNew(InternetServerList, SListView< TSharedPtr<FServerData> >)
						// List view items are this tall
						.ItemHeight(24)
						// Tell the list view where to get its source data
						.ListItemsSource(&FilteredServersSource)
						// When the list view needs to generate a widget for some data item, use this method
						.OnGenerateRow(this, &SUWServerBrowser::OnGenerateWidgetForList)
						.OnSelectionChanged(this, &SUWServerBrowser::OnServerListSelectionChanged)
						.OnMouseButtonDoubleClick(this, &SUWServerBrowser::OnListMouseButtonDoubleClick)
						.SelectionMode(ESelectionMode::Single)
						.HeaderRow
						(
							SAssignNew(HeaderRow, SHeaderRow)
							.Style(SUTStyle::Get(), "UT.List.Header")

							+ SHeaderRow::Column("ServerName")
								.OnSort(this, &SUWServerBrowser::OnSort)
								.HeaderContent()
								[
									SNew(STextBlock)
									.Text(NSLOCTEXT("SUWServerBrowser", "ServerNameColumn", "Server Name"))
									.ToolTipText(NSLOCTEXT("SUWServerBrowser", "ServerNameColumnToolTip", "The name of this server."))
									.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
								]

							+ SHeaderRow::Column("ServerGame")
								.DefaultLabel(NSLOCTEXT("SUWServerBrowser", "ServerGameColumn", "Game"))
								.HAlignCell(HAlign_Center)
								.OnSort(this, &SUWServerBrowser::OnSort)
								.HeaderContentPadding(FMargin(5.0))
								.HeaderContent()
								[
									SNew(SBox)
									.HAlign(HAlign_Center)
									[
										SNew(STextBlock)
										.Text(NSLOCTEXT("SUWServerBrowser", "ServerGameColumn", "Game"))
										.ToolTipText(NSLOCTEXT("SUWServerBrowser", "ServerGameColumnToolTip", "The Game type."))
										.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
									]
								]

							+ SHeaderRow::Column("ServerMap")
								.DefaultLabel(NSLOCTEXT("SUWServerBrowser", "ServerMapColumn", "Map"))
								.HAlignCell(HAlign_Center)
								.OnSort(this, &SUWServerBrowser::OnSort)
								.HeaderContent()
								[
									SNew(SBox)
									.HAlign(HAlign_Center)
									[
										SNew(STextBlock)
										.Text(NSLOCTEXT("SUWServerBrowser", "ServerMapColumn", "Map"))
										.ToolTipText(NSLOCTEXT("SUWServerBrowser", "ServerMapColumnToolTip", "The current map."))
										.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
									]
								]

							+ SHeaderRow::Column("ServerNumPlayers")
								.DefaultLabel(NSLOCTEXT("SUWServerBrowser", "ServerNumPlayerColumn", "Players"))
								.HAlignCell(HAlign_Center)
								.OnSort(this, &SUWServerBrowser::OnSort)
								.HeaderContent()
								[
									SNew(SBox)
									.HAlign(HAlign_Center)
									[
										SNew(STextBlock)
										.Text(NSLOCTEXT("SUWServerBrowser", "ServerNumPlayerColumn", "Players"))
										.ToolTipText(NSLOCTEXT("SUWServerBrowser", "ServerNumPlayerColumnToolTip", "The # of Players on this server."))
										.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
									]
								]

							+ SHeaderRow::Column("ServerNumSpecs")
								.DefaultLabel(NSLOCTEXT("SUWServerBrowser", "ServerNumSpecsColumn", "Spectators"))
								.HAlignCell(HAlign_Center)
								.OnSort(this, &SUWServerBrowser::OnSort)
								.HeaderContent()
								[
									SNew(SBox)
									.HAlign(HAlign_Center)
									[
										SNew(STextBlock)
										.Text(NSLOCTEXT("SUWServerBrowser", "ServerNumSpecsColumn", "Spectators"))
										.ToolTipText(NSLOCTEXT("SUWServerBrowser", "ServerNumSpecsColumnToolTip", "The # of spectators on this server."))
										.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
									]
								]

							+ SHeaderRow::Column("ServerNumFriends")
								.DefaultLabel(NSLOCTEXT("SUWServerBrowser", "ServerNumFriendsColumn", "Friends"))
								.HAlignCell(HAlign_Center)
								.OnSort(this, &SUWServerBrowser::OnSort)
								.HeaderContent()
								[
									SNew(SBox)
									.HAlign(HAlign_Center)
									[
										SNew(STextBlock)
										.Text(NSLOCTEXT("SUWServerBrowser", "ServerNumFriendsColumn", "Friends"))
										.ToolTipText(NSLOCTEXT("SUWServerBrowser", "ServerNumFriendsColumnToolTip", "The # of friends on this server."))
										.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
									]
								]

							+ SHeaderRow::Column("ServerFlags")
								.DefaultLabel(NSLOCTEXT("SUWServerBrowser", "ServerFlagsColumn", "Options"))
								.HAlignCell(HAlign_Center)
								.OnSort(this, &SUWServerBrowser::OnSort)
								.HeaderContent()
								[
									SNew(SBox)
									.HAlign(HAlign_Center)
									[
										SNew(STextBlock)
										.Text(NSLOCTEXT("SUWServerBrowser", "ServerFlagsColumn", "Options"))
										.ToolTipText(NSLOCTEXT("SUWServerBrowser", "ServerFlagsColumnToolTip", "Server Options"))
										.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
									]
								]

							+ SHeaderRow::Column("ServerPing")
								.DefaultLabel(NSLOCTEXT("SUWServerBrowser", "ServerPingColumn", "Ping"))
								.HAlignCell(HAlign_Center)
								.OnSort(this, &SUWServerBrowser::OnSort)
								.HeaderContent()
								[
									SNew(SBox)
									.HAlign(HAlign_Center)
									[
										SNew(STextBlock)
										.Text(NSLOCTEXT("SUWServerBrowser", "ServerPingColumn", "Ping"))
										.ToolTipText(NSLOCTEXT("SUWServerBrowser", "ServerPingColumnToolTip", "Your connection speed to the server."))
										.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
									]
								]
						)
					]
				]
			]
			+ SSplitter::Slot()
			. Value(0.15)
			[

				SAssignNew( HorzSplitter,SSplitter )
				.Orientation(Orient_Vertical)
				.OnSplitterFinishedResizing(this, &SUWServerBrowser::HorzSplitterResized)

				// Game Rules

				+ SSplitter::Slot()
				.Value(0.5)
				[
					SNew(SBorder)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.Padding(5,0,5,0)
						.FillWidth(1)
						[
							// The list view being tested
							SAssignNew( RulesList, SListView< TSharedPtr<FServerRuleData> > )
							// List view items are this tall
							.ItemHeight(24)
							// When the list view needs to generate a widget for some data item, use this method
							.OnGenerateRow( this, &SUWServerBrowser::OnGenerateWidgetForRulesList )
							.SelectionMode(ESelectionMode::Single)
							.ListItemsSource( &RulesListSource)

							.HeaderRow
							(
								SAssignNew(RulesHeaderRow, SHeaderRow) 
								.Style(SUTStyle::Get(),"UT.List.Header")

								+ SHeaderRow::Column("Rule")
									.OnSort(this, &SUWServerBrowser::OnRuleSort)
									.HeaderContent()
									[
										SNew(STextBlock)
										.Text(NSLOCTEXT("SUWServerBrowser","RuleRuleColumn", "Rule"))
										.ToolTipText( NSLOCTEXT("SUWServerBrowser","RuleRuleColumnToolTip", "The name of the rule.") )
										.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
									]

								+ SHeaderRow::Column("Value") 
									.DefaultLabel(NSLOCTEXT("SUWServerBrowser","RuleValueColumn", "Value")) 
									.HAlignCell(HAlign_Center) 
									.HAlignHeader(HAlign_Center)
									.OnSort(this, &SUWServerBrowser::OnRuleSort)
									.HeaderContent()
									[
										SNew(STextBlock)
										.Text(NSLOCTEXT("SUWServerBrowser","RuleValueColumn", "Value"))
										.ToolTipText( NSLOCTEXT("SUWServerBrowser","RuleValueColumnToolTip", "The Value") )
										.TextStyle(SUTStyle::Get(), "UT.Font.ServerBrowser.List.Header")
									]

							)
						]
					]
				]

				// Player Info

				+ SSplitter::Slot()
				.Value(0.5)
				[
					BuildPlayerList()
				]

			]
		];
}

TSharedRef<SWidget> SUWServerBrowser::BuildLobbyBrowser()
{
	return SAssignNew(LobbyBrowser, SVerticalBox)
		+SVerticalBox::Slot()
		.Padding(64.0, 15.0, 64.0, 15.0)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBox).WidthOverride(1792).HeightOverride(860)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.Padding(0.0f,0.0f,6.0f,0.0f)
					.AutoWidth()
					[
						SNew(SBox).WidthOverride(826)
						[
							SNew(SBorder)
							.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Dark"))
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot()
								.AutoHeight()
								.HAlign(HAlign_Fill)
								[
									SNew(SBox).HeightOverride(860)
									[
										// The list view being tested
										SAssignNew(HUBServerList, SListView< TSharedPtr<FServerData> >)
										// List view items are this tall
										.ItemHeight(64)
										// When the list view needs to generate a widget for some data item, use this method
										.OnGenerateRow(this, &SUWServerBrowser::OnGenerateWidgetForHUBList)
										.SelectionMode(ESelectionMode::Single)
										.ListItemsSource(&FilteredHubsSource)
										.OnMouseButtonDoubleClick(this, &SUWServerBrowser::OnListMouseButtonDoubleClick)
										.OnSelectionChanged(this, &SUWServerBrowser::OnHUBListSelectionChanged)								
									]
								]
							]
						]
					]
					+SHorizontalBox::Slot()
					.Padding(6.0f,0.0f,0.0f,0.0f)
					.AutoWidth()
					[

						SNew(SVerticalBox)
						+SVerticalBox::Slot()
						.HAlign(HAlign_Fill)
						.AutoHeight()
						[
							SNew(SBox).HeightOverride(200)
							[
								SNew(SBorder)
								.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.SuperDark"))
								[
									SNew(SScrollBox)
									+ SScrollBox::Slot()
									.Padding(FMargin(0.0f, 5.0f, 0.0f, 5.0f))
									[
										SAssignNew(LobbyInfoText, SRichTextBlock)
										.Text(NSLOCTEXT("HUBBrowser", "NoneSelected", "No Server Selected!"))
										.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
										.Justification(ETextJustify::Center)
										.DecoratorStyleSet(&SUTStyle::Get())
										.AutoWrapText(true)
									]
								]
							]
						]
						+SVerticalBox::Slot()
						.HAlign(HAlign_Fill)
						.FillHeight(1.0)
						.Padding(0.0,12.0,0.0,0.0)
						[
							SNew(SBox).HeightOverride(648)
							[
								SAssignNew(LobbyMatchPanel, SUMatchPanel).PlayerOwner(PlayerOwner).bExpectServerData(true)
								.OnJoinMatchDelegate(this, &SUWServerBrowser::JoinQuickInstance)
							]
						]
					]
				]
			]
		];
}

ECheckBoxState SUWServerBrowser::ShouldHideUnresponsiveServers() const
{
	return bHideUnresponsiveServers ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SUWServerBrowser::OnHideUnresponsiveServersChanged(const ECheckBoxState NewState)
{
	bHideUnresponsiveServers = NewState == ECheckBoxState::Checked;
	if (bShowingHubs)
	{
		FilterAllHUBs();
	}
	else
	{
		FilterAllServers();
	}
}

void SUWServerBrowser::OwnerLoginStatusChanged(UUTLocalPlayer* LocalPlayerOwner, ELoginStatus::Type NewStatus, const FUniqueNetId& UniqueID)
{
	if (NewStatus == ELoginStatus::LoggedIn)
	{
		SetBrowserState(EBrowserState::BrowserIdle);
		RefreshButton->SetContent( SNew(STextBlock).Text(NSLOCTEXT("SUWServerBrowser","Refresh","Refresh")).TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium"));
		if (bAutoRefresh)
		{
			bAutoRefresh = false;
			OnRefreshClick();
		}
	}
	else
	{
		SetBrowserState(EBrowserState::NotLoggedIn);
	}
}


void SUWServerBrowser::AddGameFilters()
{
	TArray<FString> GameTypes;
	GameTypes.Add(TEXT("All"));
	for (int32 i=0;i<AllInternetServers.Num();i++)
	{
		int32 idx = GameTypes.Find(AllInternetServers[i]->GameModeName);
		if (idx < 0)
		{
			GameTypes.Add(AllInternetServers[i]->GameModeName);
		}
	}

	for (int32 i=0;i<PingList.Num();i++)
	{
		int32 idx = GameTypes.Find(PingList[i]->GameModeName);
		if (idx < 0)
		{
			GameTypes.Add(PingList[i]->GameModeName);
		}
	}

	if ( GameFilter.IsValid() ) 
	{
		TSharedPtr<SVerticalBox> Menu = NULL;
		SAssignNew(Menu, SVerticalBox);
		if (Menu.IsValid())
		{

			for (int32 i=0;i<GameTypes.Num();i++)
			{
				(*Menu).AddSlot()
				.AutoHeight()
				[
					SNew(SButton)
					.ButtonStyle(SUTStyle::Get(), "UT.ContextMenu.Item")
					.ContentPadding(FMargin(10.0f, 5.0f))
					.Text(FText::FromString(GameTypes[i]))
					.TextStyle(SUTStyle::Get(), "UT.Font.ContextMenuItem")
					.OnClicked(this, &SUWServerBrowser::OnGameFilterSelection, GameTypes[i])
				];
			}
			GameFilter->SetMenuContent(Menu.ToSharedRef());
		}
	}

	GameFilterText = NSLOCTEXT("SUWServerBrowser", "GameFilterAll", "All");
}

FText SUWServerBrowser::GetGameFilterText() const
{
	return GameFilterText;
}

FReply SUWServerBrowser::OnGameFilterSelection(FString Filter)
{
	GameFilterText = FText::FromString(Filter);
	GameFilter->SetIsOpen(false);
	FilterAllServers();
	return FReply::Handled();
}

void SUWServerBrowser::OnSort(EColumnSortPriority::Type Priority, const FName& ColumnName, EColumnSortMode::Type SortType)
{
	if (ColumnName == CurrentSortColumn)
	{
		bDescendingSort = !bDescendingSort;
	}

	SortServers(ColumnName);
}

void SUWServerBrowser::OnRuleSort(EColumnSortPriority::Type Priority, const FName& ColumnName, EColumnSortMode::Type SortType)
{
	if (ColumnName == CurrentRulesSortColumn)
	{
		bDescendingRulesSort = !bDescendingRulesSort;
	}

	CurrentRulesSortColumn = ColumnName;


	if (ColumnName == FName("Rule")) bDescendingRulesSort ? RulesListSource.Sort(FCompareRulesByRuleDesc()) : RulesListSource.Sort(FCompareRulesByRule());
	else if (ColumnName == FName("Value")) bDescendingRulesSort ? RulesListSource.Sort(FCompareRulesByValueDesc()) : RulesListSource.Sort(FCompareRulesByValue());

	RulesList->RequestListRefresh();

}

void SUWServerBrowser::OnPlayerSort(EColumnSortPriority::Type Priority, const FName& ColumnName, EColumnSortMode::Type SortType)
{
	if (ColumnName == CurrentRulesSortColumn)
	{
		bDescendingPlayersSort = !bDescendingPlayersSort;
	}

	CurrentRulesSortColumn = ColumnName;

	if (ColumnName == FName("Name")) bDescendingPlayersSort ? PlayersListSource.Sort(FComparePlayersByNameDesc()) : PlayersListSource.Sort(FComparePlayersByName());
	else if (ColumnName == FName("Score")) bDescendingPlayersSort ? PlayersListSource.Sort(FComparePlayersByScoreDesc()) : PlayersListSource.Sort(FComparePlayersByScore());

	PlayersList->RequestListRefresh();

}



void SUWServerBrowser::SortServers(FName ColumnName)
{
	if (ColumnName == FName(TEXT("ServerName"))) bDescendingSort ? FilteredServersSource.Sort(FCompareServerByNameDesc()) : FilteredServersSource.Sort(FCompareServerByName());
	else if (ColumnName == FName(TEXT("ServerIP"))) bDescendingSort ? FilteredServersSource.Sort(FCompareServerByIPDesc()) : FilteredServersSource.Sort(FCompareServerByIP());
	else if (ColumnName == FName(TEXT("ServerGame"))) bDescendingSort ? FilteredServersSource.Sort(FCompareServerByGameModeDesc()) : FilteredServersSource.Sort(FCompareServerByGameMode());
	else if (ColumnName == FName(TEXT("ServerMap"))) bDescendingSort ? FilteredServersSource.Sort(FCompareServerByMapDesc()) : FilteredServersSource.Sort(FCompareServerByMap());
	else if (ColumnName == FName(TEXT("ServerVer"))) bDescendingSort ? FilteredServersSource.Sort(FCompareServerByVersionDesc()) : FilteredServersSource.Sort(FCompareServerByVersion());
	else if (ColumnName == FName(TEXT("ServerNumPlayers"))) bDescendingSort ? FilteredServersSource.Sort(FCompareServerByNumPlayersDesc()) : FilteredServersSource.Sort(FCompareServerByNumPlayers());
	else if (ColumnName == FName(TEXT("ServerNumSpecs"))) bDescendingSort ? FilteredServersSource.Sort(FCompareServerByNumSpectatorsDesc()) : FilteredServersSource.Sort(FCompareServerByNumSpectators());
	else if (ColumnName == FName(TEXT("ServerNumFriends"))) bDescendingSort ? FilteredServersSource.Sort(FCompareServerByNumFriendsDesc()) : FilteredServersSource.Sort(FCompareServerByNumFriends());
	else if (ColumnName == FName(TEXT("ServerPing"))) bDescendingSort ? FilteredServersSource.Sort(FCompareServerByPingDesc()) : FilteredServersSource.Sort(FCompareServerByPing());

	InternetServerList->RequestListRefresh();
	CurrentSortColumn = ColumnName;
}

void SUWServerBrowser::SortHUBs()
{
	FilteredHubsSource.Sort(FCompareHub());
	HUBServerList->RequestListRefresh();
}

TSharedRef<ITableRow> SUWServerBrowser::OnGenerateWidgetForList( TSharedPtr<FServerData> InItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	return SNew( SServerBrowserRow, OwnerTable).ServerData( InItem ).Style(SUTStyle::Get(),"UT.List.Row");
}

TSharedRef<ITableRow> SUWServerBrowser::OnGenerateWidgetForRulesList( TSharedPtr<FServerRuleData> InItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	return SNew( SServerRuleRow, OwnerTable).ServerRuleData( InItem ).Style(SUTStyle::Get(),"UT.List.Row");
}

TSharedRef<ITableRow> SUWServerBrowser::OnGenerateWidgetForPlayersList( TSharedPtr<FServerPlayerData> InItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	return SNew( SServerPlayerRow, OwnerTable).ServerPlayerData( InItem ).Style(SUTStyle::Get(),"UT.List.Row");
}

FText SUWServerBrowser::GetStatusText() const
{
	if (BrowserState == EBrowserState::NotLoggedIn)
	{
		return NSLOCTEXT("SUWServerBrowser","NotLoggedIn","Login Required!");
	}
	else if (BrowserState == EBrowserState::AuthInProgress) 
	{
		return NSLOCTEXT("SUWServerBrowser","Auth","Authenticating...");
	}
	else 
	{
		int32 PingCount = PingList.Num() + PingTrackers.Num();


		FFormatNamedArguments Args;
		Args.Add( TEXT("PingCount"), FText::AsNumber(PingCount) );
		Args.Add( TEXT("FilteredHubCount"), FText::AsNumber(FilteredHubsSource.Num() > 0 ? FilteredHubsSource.Num() : AllHubServers.Num()) );
		Args.Add( TEXT("HubCount"), FText::AsNumber(AllHubServers.Num()) );
		Args.Add( TEXT("FilteredServerCount"), FText::AsNumber(FilteredServersSource.Num()> 0 ? FilteredServersSource.Num() : AllInternetServers.Num()) );
		Args.Add( TEXT("ServerCount"), FText::AsNumber(AllInternetServers.Num()) );
		Args.Add( TEXT("TotalPlayers"), FText::AsNumber(TotalPlayersPlaying) );

		int32 PlayerCount = 0;
		if (bShowingHubs)
		{
			for (int32 i=0; i < FilteredHubsSource.Num(); i++)
			{
				PlayerCount += FilteredHubsSource[i]->NumPlayers + FilteredHubsSource[i]->NumSpectators;
			}
		}
		else
		{
			for (int32 i=0; i < FilteredServersSource.Num(); i++)
			{
				PlayerCount += FilteredServersSource[i]->NumPlayers + FilteredServersSource[i]->NumSpectators;
			}
		}

		Args.Add( TEXT("PlayerCount"), FText::AsNumber(PlayerCount) );

		if (PingCount > 0)
		{
			return FText::Format( NSLOCTEXT("SUWServerBrowser","PingingStatusMsg","Showing {FilteredHubCount} of {HubCount} Hubs, {FilteredServerCount} of {ServerCount} Servers - Pinging {PingCount} Servers..."), Args);
		}
		else
		{
			return FText::Format( NSLOCTEXT("SUWServerBrowser","IdleStatusMsg","Showing {FilteredHubCount} of {HubCount} Hubs, {FilteredServerCount} of {ServerCount} Servers -- {PlayerCount} of {TotalPlayers} Players"), Args);
		}
	}
}

void SUWServerBrowser::SetBrowserState(FName NewBrowserState)
{
	BrowserState = NewBrowserState;
	if (BrowserState == EBrowserState::NotLoggedIn) 
	{
		RefreshButton->SetContent( SNew(STextBlock).Text(NSLOCTEXT("SUWServerBrowser","Login","Login")).TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium"));
		RefreshButton->SetVisibility(EVisibility::All);

		JoinButton->SetEnabled(false);
		SpectateButton->SetEnabled(false);

	}
	else if (BrowserState == EBrowserState::BrowserIdle) 
	{
		RefreshButton->SetVisibility(EVisibility::All);
	}
	else if (BrowserState == EBrowserState::AuthInProgress) 
	{
		RefreshButton->SetVisibility(EVisibility::Hidden);

		JoinButton->SetEnabled(false);
		SpectateButton->SetEnabled(false);
	}
	else if (BrowserState == EBrowserState::RefreshInProgress) 
	{
		RefreshButton->SetVisibility(EVisibility::Hidden);
		JoinButton->SetEnabled(false);
		SpectateButton->SetEnabled(false);
	}
}

FReply SUWServerBrowser::OnRefreshClick()
{
	if (PlayerOwner->IsLoggedIn())
	{
		RefreshServers();
	}
	else
	{
		bAutoRefresh = true;
		PlayerOwner->LoginOnline(TEXT(""),TEXT(""),false);
	}
	return FReply::Handled();
}

void SUWServerBrowser::RefreshServers()
{
	bWantsAFullRefilter = true;
	if (PlayerOwner->IsLoggedIn() && OnlineSessionInterface.IsValid() && BrowserState == EBrowserState::BrowserIdle)
	{
		SetBrowserState(EBrowserState::RefreshInProgress);

		bNeedsRefresh = false;
		CleanupQoS();

		// Search for Internet Servers

		SearchSettings = MakeShareable(new FUTOnlineGameSearchBase(false));
		SearchSettings->MaxSearchResults = 10000;
		SearchSettings->bIsLanQuery = false;
		SearchSettings->QuerySettings.Set(SETTING_GAMEINSTANCE, 1, EOnlineComparisonOp::NotEquals);												// Must not be a Hub server instance

		TSharedRef<FUTOnlineGameSearchBase> SearchSettingsRef = SearchSettings.ToSharedRef();

		FOnFindSessionsCompleteDelegate Delegate;
		Delegate.BindSP(this, &SUWServerBrowser::OnFindSessionsComplete);
		OnFindSessionCompleteDelegate = OnlineSessionInterface->AddOnFindSessionsCompleteDelegate_Handle(Delegate);

		OnlineSessionInterface->FindSessions(0, SearchSettingsRef);
	}
	else
	{
		SetBrowserState(EBrowserState::NotLoggedIn);	
	}
}

void SUWServerBrowser::FoundServer(FOnlineSessionSearchResult& Result)
{
	FString ServerName;
	Result.Session.SessionSettings.Get(SETTING_SERVERNAME,ServerName);
	FString ServerIP;
	OnlineSessionInterface->GetResolvedConnectString(Result,FName(TEXT("GamePort")), ServerIP);
	// game class path
	FString ServerGamePath;
	Result.Session.SessionSettings.Get(SETTING_GAMEMODE,ServerGamePath);
	// game name
	FString ServerGameName;

	// prefer using the name in the client's language, if available
	// TODO: would be nice to not have to load the class, but the localization system doesn't guarantee any particular lookup location for the data,
	//		so we have no way to know where it is
	UClass* GameClass = LoadClass<AUTBaseGameMode>(NULL, *ServerGamePath, NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
	if (GameClass != NULL)
	{
		ServerGameName = GameClass->GetDefaultObject<AUTBaseGameMode>()->DisplayName.ToString();
	}
	else
	{
		// FIXME: legacy compatibility, remove later
		if (!Result.Session.SessionSettings.Get(SETTING_GAMENAME, ServerGameName))
		{
			ServerGameName = ServerGamePath;
		}
	}

	FString ServerMap;
	Result.Session.SessionSettings.Get(SETTING_MAPNAME,ServerMap);
				
	int32 ServerNoPlayers = 0;
	Result.Session.SessionSettings.Get(SETTING_PLAYERSONLINE, ServerNoPlayers);
				
	int32 ServerNoSpecs = 0;
	Result.Session.SessionSettings.Get(SETTING_SPECTATORSONLINE, ServerNoSpecs);
				
	int32 ServerMaxPlayers = 0;
	Result.Session.SessionSettings.Get(SETTING_UTMAXPLAYERS, ServerMaxPlayers);
				
	int32 ServerMaxSpectators = 0;
	Result.Session.SessionSettings.Get(SETTING_UTMAXSPECTATORS, ServerMaxSpectators);

	int32 ServerNumMatches = 0;
	Result.Session.SessionSettings.Get(SETTING_NUMMATCHES, ServerNumMatches);
				
	int32 ServerMinRank = 0;
	Result.Session.SessionSettings.Get(SETTING_MINELO, ServerMinRank);

	int32 ServerMaxRank = 0;
	Result.Session.SessionSettings.Get(SETTING_MAXELO, ServerMaxRank);

	FString ServerVer; 
	Result.Session.SessionSettings.Get(SETTING_SERVERVERSION, ServerVer);
				
	int32 ServerFlags = 0x0000;
	Result.Session.SessionSettings.Get(SETTING_SERVERFLAGS, ServerFlags);
				
	uint32 ServerPing = -1;

	int32 ServerTrustLevel = 2;
	Result.Session.SessionSettings.Get(SETTING_TRUSTLEVEL, ServerTrustLevel);

	FString BeaconIP;
	OnlineSessionInterface->GetResolvedConnectString(Result,FName(TEXT("BeaconPort")), BeaconIP);
	TSharedRef<FServerData> NewServer = FServerData::Make( ServerName, ServerIP, BeaconIP, ServerGamePath, ServerGameName, ServerMap, ServerNoPlayers, ServerNoSpecs, ServerMaxPlayers, ServerMaxSpectators, ServerNumMatches, ServerMinRank, ServerMaxRank, ServerVer, ServerPing, ServerFlags,ServerTrustLevel);
	NewServer->SearchResult = Result;

	if (PingList.Num() == 0 || ServerGamePath != LOBBY_GAME_PATH )
	{
		PingList.Add( NewServer );
	}
	else
	{
		PingList.Insert(NewServer,0);
	}

	TotalPlayersPlaying += ServerNoPlayers + ServerNoSpecs;

}

void SUWServerBrowser::OnFindSessionsComplete(bool bWasSuccessful)
{
	OnlineSessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionCompleteDelegate);

	if (bWasSuccessful)
	{
		TotalPlayersPlaying = 0;
		if (SearchSettings->SearchResults.Num() > 0)
		{
			for (int32 ServerIndex = 0; ServerIndex < SearchSettings->SearchResults.Num(); ServerIndex++)
			{
				FoundServer(SearchSettings->SearchResults[ServerIndex]);
			}
		}

		// If a server exists in either of the lists but not in the PingList, then let's kill it.
		ExpireDeadServers();

		if ( FParse::Param(FCommandLine::Get(), TEXT("DumpServers")) )
		{
			UE_LOG(UT,Log,TEXT("Received a list of %i Internet Servers....."), PingList.Num());
			for (int32 i=0;i<PingList.Num();i++)
			{
				UE_LOG(UT,Log,TEXT("Received Server %i - %s %s  : Players %i/%i"), i, *PingList[i]->Name, *PingList[i]->IP, PingList[i]->NumPlayers, PingList[i]->MaxPlayers);
			}
			UE_LOG(UT,Log, TEXT("----------------------------------------------"));
		}


		AddGameFilters();
		InternetServerList->RequestListRefresh();
		HUBServerList->RequestListRefresh();
		PingNextServer();
	}
	else
	{
		UE_LOG(UT,Log,TEXT("Server List Request Failed!!!"));
	}

	SetBrowserState(EBrowserState::BrowserIdle);

/*
	// Search for LAN Servers

	LanSearchSettings = MakeShareable(new FUTOnlineGameSearchBase(false));
	LanSearchSettings->MaxSearchResults = 10000;
	LanSearchSettings->bIsLanQuery = true;
	LanSearchSettings->TimeoutInSeconds = 5.0;
	LanSearchSettings->QuerySettings.Set(SETTING_GAMEINSTANCE, 1, EOnlineComparisonOp::NotEquals);												// Must not be a Hub server instance

	TSharedRef<FUTOnlineGameSearchBase> LanSearchSettingsRef = LanSearchSettings.ToSharedRef();
	FOnFindSessionsCompleteDelegate Delegate;
	Delegate.BindSP(this, &SUWServerBrowser::OnFindLANSessionsComplete);
	OnFindLANSessionCompleteDelegate = OnlineSessionInterface->AddOnFindSessionsCompleteDelegate_Handle(Delegate);
	OnlineSessionInterface->FindSessions(0, LanSearchSettingsRef);
*/
}


void SUWServerBrowser::OnFindLANSessionsComplete(bool bWasSuccessful)
{
	OnlineSessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindLANSessionCompleteDelegate);

	if (bWasSuccessful)
	{
		TotalPlayersPlaying = 0;
		if (LanSearchSettings->SearchResults.Num() > 0)
		{
			for (int32 ServerIndex = 0; ServerIndex < LanSearchSettings->SearchResults.Num(); ServerIndex++)
			{
				FoundServer(LanSearchSettings->SearchResults[ServerIndex]);
			}
		}

		if ( FParse::Param(FCommandLine::Get(), TEXT("DumpServers")) )
		{
			UE_LOG(UT,Log,TEXT("Received a list of %i Servers....."), PingList.Num());
			for (int32 i=0;i<PingList.Num();i++)
			{
				UE_LOG(UT,Log,TEXT("Received Server %i - %s %s  : Players %i/%i"), i, *PingList[i]->Name, *PingList[i]->IP, PingList[i]->NumPlayers, PingList[i]->MaxPlayers);
			}
			UE_LOG(UT,Log, TEXT("----------------------------------------------"));
		}

		AddGameFilters();
		InternetServerList->RequestListRefresh();
		HUBServerList->RequestListRefresh();
		PingNextServer();
	}
	else
	{
		UE_LOG(UT,Log,TEXT("Server List Request Failed!!!"));
	}

	SetBrowserState(EBrowserState::BrowserIdle);
}


void SUWServerBrowser::ExpireDeadServers()
{
	int32 i = 0;
	while (i < AllInternetServers.Num())
	{
		bool bFound = false;
		for (int32 j=0; j < PingList.Num(); j++)
		{
			if (AllInternetServers[i]->SearchResult.Session.SessionInfo->GetSessionId() == PingList[j]->SearchResult.Session.SessionInfo->GetSessionId())	
			{
				bFound = true;
				break;
			}
		}

		if (bFound)
		{
			i++;
		}
		else
		{
			AllInternetServers.RemoveAt(i);
		}
	
	} 

	i = 0;
	while (i < AllHubServers.Num())
	{
		if (!AllHubServers[i]->bFakeHUB)
		{
			bool bFound = false;
			for (int32 j=0; j < PingList.Num(); j++)
			{
				if (AllHubServers[i]->SearchResult.Session.SessionInfo->GetSessionId() == PingList[j]->SearchResult.Session.SessionInfo->GetSessionId())	
				{
					bFound = true;
					break;
				}
			}

			if (bFound)
			{
				i++;
			}
			else
			{
				AllHubServers.RemoveAt(i);
			}
		}
		else
		{
			i++;
		}

	} 
}


void SUWServerBrowser::CleanupQoS()
{
	for (int32 i=0;i<PingTrackers.Num();i++)
	{
		if (PingTrackers[i].Beacon.IsValid())
		{
			PingTrackers[i].Beacon->DestroyBeacon();

		}
	}
	PingTrackers.Empty();
	PingList.Empty();
}

void SUWServerBrowser::PingNextServer()
{

	if (PingList.Num() <= 0 && PingTrackers.Num() <= 0)
	{
		if (bWantsAFullRefilter)
		{
			// We are done.  Perform all filtering again since we can grab the real best ping
			if (bShowingHubs)
			{
				FilterAllHUBs();
			}
			else
			{
				FilterAllServers();
			}
		}

		bWantsAFullRefilter = false;
	}

	while (PingList.Num() > 0 && PingTrackers.Num() < PlayerOwner->ServerPingBlockSize)
	{
		PingServer(PingList[0]);
		PingList.RemoveAt(0,1);
	}

}

void SUWServerBrowser::PingServer(TSharedPtr<FServerData> ServerToPing)
{
	// Build the beacon
	AUTServerBeaconClient* Beacon = PlayerOwner->GetWorld()->SpawnActor<AUTServerBeaconClient>(AUTServerBeaconClient::StaticClass());
	if (Beacon)
	{
		Beacon->OnServerRequestResults = FServerRequestResultsDelegate::CreateSP(this, &SUWServerBrowser::OnServerBeaconResult );
		Beacon->OnServerRequestFailure = FServerRequestFailureDelegate::CreateSP(this, &SUWServerBrowser::OnServerBeaconFailure);
		FURL BeaconURL(nullptr, *ServerToPing->BeaconIP, TRAVEL_Absolute);
		Beacon->InitClient(BeaconURL);
		PingTrackers.Add(FServerPingTracker(ServerToPing, Beacon));
	}
}

void SUWServerBrowser::AddServer(TSharedPtr<FServerData> Server)
{
	Server->UpdateFriends(PlayerOwner);
	for (int32 i=0; i < AllInternetServers.Num() ; i++)
	{
		if (AllInternetServers[i]->SearchResult.Session.SessionInfo->GetSessionId() == Server->SearchResult.Session.SessionInfo->GetSessionId())
		{
			// Same session id, so see if they are the same

			if (AllInternetServers[i] != Server)
			{
				AllInternetServers[i]->Update(Server);
			}

			return; 
		}
	}

	AllInternetServers.Add(Server);
	FilterServer(Server);
}

void SUWServerBrowser::AddHub(TSharedPtr<FServerData> Hub)
{
	bool bIsBeginner = GetPlayerOwner()->IsConsideredABeginnner();

	Hub->UpdateFriends(PlayerOwner);

	bool ServerIsTrainingGround;
	Hub->SearchResult.Session.SessionSettings.Get(SETTING_TRAININGGROUND, ServerIsTrainingGround);

	int32 ServerTrustLevel; 
	Hub->SearchResult.Session.SessionSettings.Get(SETTING_TRUSTLEVEL, ServerTrustLevel);

	// Only trusted servers can be training grounds.  TODO: Move this to the MCP.
	if ( ServerTrustLevel >0 ) ServerIsTrainingGround = 0;

	if ( !bIsBeginner && ServerIsTrainingGround == 1 )
	{
		Hub->Flags |= SERVERFLAG_Restricted;
	}

	if (HUBServerList->GetNumItemsSelected() > 0)
	{
		TArray<TSharedPtr<FServerData>> Hubs = HUBServerList->GetSelectedItems();
		if (Hubs.Num() >= 1 && Hubs[0].IsValid() && Hubs[0]->SearchResult.IsValid() && Hubs[0]->SearchResult.Session.SessionInfo->GetSessionId() == Hub->SearchResult.Session.SessionInfo->GetSessionId())
		{
			AddHUBInfo(Hub);
		}
	}

	for (int32 i=0; i < AllHubServers.Num() ; i++)
	{
		if (AllHubServers[i].IsValid() && AllHubServers[i]->SearchResult.IsValid() && !AllHubServers[i]->bFakeHUB)
		{
			if (AllHubServers[i]->SearchResult.Session.SessionInfo->GetSessionId() == Hub->SearchResult.Session.SessionInfo->GetSessionId())
			{
				if (AllHubServers[i] != Hub)
				{
					AllHubServers[i]->Update(Hub);
				}
				return; 
			}
		}
	}

	AllHubServers.Add(Hub);
	FilterHUB(Hub);
}

void SUWServerBrowser::OnServerBeaconFailure(AUTServerBeaconClient* Sender)
{
	for (int32 i=0; i < PingTrackers.Num(); i++)
	{
		if (PingTrackers[i].Beacon == Sender)
		{

			AddServer(PingTrackers[i].Server);
			PingTrackers[i].Beacon->DestroyBeacon();
			PingTrackers.RemoveAt(i,1);

			PingNextServer();
		}
	}
}

void SUWServerBrowser::OnServerBeaconResult(AUTServerBeaconClient* Sender, FServerBeaconInfo ServerInfo)
{
	for (int32 i=0; i < PingTrackers.Num(); i++)
	{
		if (PingTrackers[i].Beacon == Sender)
		{
			// Matched... store the data.
			PingTrackers[i].Server->Ping = Sender->Ping;
			PingTrackers[i].Server->MOTD = ServerInfo.MOTD;
			PingTrackers[i].Server->Map = ServerInfo.CurrentMap;

			PingTrackers[i].Server->Players.Empty();
			TArray<FString> PlayerData;
			int32 Cnt = ServerInfo.ServerPlayers.ParseIntoArray(PlayerData, TEXT("\t"), true);
			for (int32 p=0;p+2 < Cnt; p+=3)
			{
				FString Name = PlayerData[p];
				FString Score = PlayerData[p+1];
				FString Id = PlayerData[p+2];

				PingTrackers[i].Server->AddPlayer(Name, Score, Id);
			}

			PingTrackers[i].Server->Rules.Empty();
			TArray<FString> RulesData;
			Cnt = ServerInfo.ServerRules.ParseIntoArray(RulesData, TEXT("\t"), true);
			for (int32 r=0; r+1 < Cnt; r+=2)
			{
				FString Rule = RulesData[r];
				FString Value = RulesData[r+1];

				PingTrackers[i].Server->AddRule(Rule, Value);
			}

			TArray<FString> BrokenIP;
			if (PingTrackers[i].Server->IP.ParseIntoArray(BrokenIP,TEXT(":"),true) == 2)
			{
				PingTrackers[i].Server->AddRule(TEXT("IP"), BrokenIP[0]);
				PingTrackers[i].Server->AddRule(TEXT("Port"), BrokenIP[1]);
			}
			else
			{
				PingTrackers[i].Server->AddRule(TEXT("IP"), PingTrackers[i].Server->IP);
			}

			PingTrackers[i].Server->AddRule(TEXT("Version"), PingTrackers[i].Server->Version);

			UE_LOG(UT,Log,TEXT("Got Ping:  %s %i"), *PingTrackers[i].Server->GetBrowserName().ToString(), PingTrackers[i].Beacon->Instances.Num() );

			PingTrackers[i].Server->HUBInstances.Empty();
			for (int32 InstIndex=0; InstIndex < PingTrackers[i].Beacon->Instances.Num(); InstIndex++ )
			{
				PingTrackers[i].Server->HUBInstances.Add(PingTrackers[i].Beacon->Instances[InstIndex]);	
			}

			if (PingTrackers[i].Server->GameModePath == LOBBY_GAME_PATH)
			{
				AddHub(PingTrackers[i].Server);
			}
			else
			{
				AddServer(PingTrackers[i].Server);
			}

			PingTrackers[i].Beacon->DestroyBeacon();
			PingTrackers.RemoveAt(i,1);

			// Look to see if there are more servers to Ping...
			PingNextServer();

			RulesList->RequestListRefresh();
			PlayersList->RequestListRefresh();
		}
	}
}

void SUWServerBrowser::OnListMouseButtonDoubleClick(TSharedPtr<FServerData> SelectedServer)
{
	if (SelectedServer->IP.Left(1) == "@")
	{
		ShowServers(SelectedServer->BeaconIP);
	}
	else
	{
		ConnectTo(*SelectedServer,false);
	}
}


FReply SUWServerBrowser::OnJoinClick(bool bSpectate)
{
	TArray<TSharedPtr<FServerData>> SelectedItems = (bShowingHubs ? HUBServerList->GetSelectedItems() : InternetServerList->GetSelectedItems());
	if (SelectedItems.Num() > 0)
	{
		ConnectTo(*SelectedItems[0],bSpectate);
	}
	return FReply::Handled();
}

void SUWServerBrowser::RestrictedWarning()
{
	PlayerOwner->MessageBox(NSLOCTEXT("SUWServerBrowser","RestrictedServerTitle","Unable to join server"), NSLOCTEXT("SUWServerBrowser","RestrictedServerMsg","Sorry, but your skill level is too high to join the hub or server you have selected.  Please choose another one."));
}

void SUWServerBrowser::ConnectTo(FServerData ServerData,bool bSpectate)
{
	if ((ServerData.Flags & SERVERFLAG_Restricted) > 0)
	{
		RestrictedWarning();
		return;
	}

	SetBrowserState(EBrowserState::BrowserIdle);	

	// Flag the browser as needing a refresh the next time it is shown
	bNeedsRefresh = true;
	PlayerOwner->JoinSession(ServerData.SearchResult, bSpectate);
	CleanupQoS();
}

void SUWServerBrowser::FilterAllServers()
{
	FilteredServersSource.Empty();
	if (AllInternetServers.Num() > 0)
	{
		int32 BestPing = AllInternetServers[0]->Ping;
		for (int32 i=0;i<AllInternetServers.Num();i++)
		{
			if (AllInternetServers[i]->Ping < BestPing) BestPing = AllInternetServers[i]->Ping;
		}

		for (int32 i=0;i<AllInternetServers.Num();i++)
		{
			FilterServer(AllInternetServers[i], false);
		}
		SortServers(CurrentSortColumn);
	}
}

void SUWServerBrowser::FilterServer(TSharedPtr< FServerData > NewServer, bool bSortAndUpdate, int32 BestPing)
{
	FString GameFilterString = GameFilterText.ToString();
	if (GameFilterString.IsEmpty() || GameFilterString == TEXT("All") || NewServer->GameModeName == GameFilterString)
	{
		if (QuickFilterText->GetText().IsEmpty() || NewServer->Name.Find(QuickFilterText->GetText().ToString()) >= 0)
		{

			if ( !IsUnresponsive(NewServer, BestPing) )
			{
				FilteredServersSource.Add(NewServer);
			}
		}
	}

	if (bSortAndUpdate)
	{
		SortServers(CurrentSortColumn);
	}
}

void SUWServerBrowser::FilterAllHUBs()
{
	FilteredHubsSource.Empty();
	if (AllHubServers.Num() > 0)
	{
		int32 BestPing = AllHubServers[0]->Ping;
		for (int32 i=0;i<AllHubServers.Num();i++)
		{
			if (AllHubServers[i]->Ping < BestPing) BestPing = AllHubServers[i]->Ping;
		}

		for (int32 i=0;i<AllHubServers.Num();i++)
		{
			FilterHUB(AllHubServers[i], false, BestPing);
		}
		SortHUBs();
	}
}


void SUWServerBrowser::FilterHUB(TSharedPtr< FServerData > NewServer, bool bSortAndUpdate, int32 BestPing)
{
	if (QuickFilterText->GetText().IsEmpty() || NewServer->Name.Find(QuickFilterText->GetText().ToString()) >= 0)
	{
		int32 BaseRank = PlayerOwner->GetBaseELORank();
		if (NewServer->bFakeHUB)
		{
			FilteredHubsSource.Add(NewServer);
		}
		else
		{
			if ( (NewServer->MinRank <= 0 || BaseRank >= NewServer->MinRank) && (NewServer->MaxRank <= 0 || BaseRank <= NewServer->MaxRank))
			{
				if ( !IsUnresponsive(NewServer, BestPing) )
				{
					FilteredHubsSource.Add(NewServer);
				}
			}
		}
	}

	if (bSortAndUpdate)
	{
		SortHUBs();
	}
}

bool SUWServerBrowser::IsUnresponsive(TSharedPtr<FServerData> Server, int32 BestPing)
{
	// If we aren't hiding unresponsive servers, we don't care so just return false.
	if (!bHideUnresponsiveServers)
	{
		return false;
	}

	if (Server->Ping >= 0)			
	{
		int32  WorstPing = FMath::Max<int32>(BestPing * 2, 100);
		if ( Server->NumPlayers > 0 || Server->Ping <= WorstPing )
		{
			return false;
		}
	}

	return true;
}

void SUWServerBrowser::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	bool bRequiresUpdate = false;
	UUTGameUserSettings* GS = Cast<UUTGameUserSettings>(GEngine->GetGameUserSettings());
	if (GS != NULL)
	{
		const TIndirectArray<SHeaderRow::FColumn>& Columns = HeaderRow->GetColumns();		
		for (int32 i=0;i<Columns.Num();i++)
		{
			if (GS->GetServerBrowserColumnWidth(i) != Columns[i].GetWidth())
			{
				GS->SetServerBrowserColumnWidth(i, Columns[i].GetWidth());
				bRequiresUpdate = true;
				break;
			}
		}

		const TIndirectArray<SHeaderRow::FColumn>& RulesColumns = RulesHeaderRow->GetColumns();		
		for (int32 i=0;i<RulesColumns.Num();i++)
		{
			if (GS->GetServerBrowserRulesColumnWidth(i) != RulesColumns[i].GetWidth())
			{
				GS->SetServerBrowserRulesColumnWidth(i, RulesColumns[i].GetWidth());
				bRequiresUpdate = true;
				break;
			}
		}

		const TIndirectArray<SHeaderRow::FColumn>& PlayersColumns = PlayersHeaderRow->GetColumns();		
		for (int32 i=0;i<PlayersColumns.Num();i++)
		{
			if (GS->GetServerBrowserPlayersColumnWidth(i) != PlayersColumns[i].GetWidth())
			{
				GS->SetServerBrowserPlayersColumnWidth(i, PlayersColumns[i].GetWidth());
				bRequiresUpdate = true;
				break;
			}
		}

		if (bRequiresUpdate && !bRequireSave)
		{
			bRequireSave = true;
		}

		if (!bRequiresUpdate && bRequireSave)
		{
			GS->SaveConfig();
			bRequireSave = false;
		}
	}

	if (BrowserState != EBrowserState::BrowserIdle) 
	{
		JoinButton->SetEnabled(false);
		SpectateButton->SetEnabled(false);
	}
	

	if (!RandomHUB.IsValid() && AllInternetServers.Num() > 0)
	{
		int32 NumPlayers = 0;
		int32 NumSpectators = 0;
		int32 NumFriends = 0;
		TallyInternetServers(NumPlayers, NumSpectators, NumFriends);

		RandomHUB = FServerData::Make( TEXT("[Internet] Individual Servers"), TEXT("@RandomServers"), TEXT("ALL"), LOBBY_GAME_PATH, TEXT("HUB"), TEXT(""),NumPlayers,NumSpectators,0,0,AllInternetServers.Num(),0,0,TEXT(""),0,0x00,0);
		RandomHUB->NumFriends = NumFriends;
		RandomHUB->MOTD = TEXT("Browse a random collection of servers on the internet.");
		RandomHUB->bFakeHUB = true;

		AllHubServers.Add( RandomHUB );
		FilterAllHUBs();
		HUBServerList->RequestListRefresh();
	}

	if (RandomHUB.IsValid() && RandomHUB->NumMatches != AllInternetServers.Num())
	{
		int32 NumPlayers = 0;
		int32 NumSpectators = 0;
		int32 NumFriends = 0;
		TallyInternetServers(NumPlayers, NumSpectators, NumFriends);

		AllHubServers.Remove(RandomHUB);
		RandomHUB = FServerData::Make( TEXT("[Internet] Individual Servers"), TEXT("@RandomServers"), TEXT("ALL"), LOBBY_GAME_PATH, TEXT("HUB"), TEXT(""),NumPlayers,NumSpectators,0,0,AllInternetServers.Num(),0,0,TEXT(""),0,0x00,0);
		RandomHUB->NumFriends = NumFriends;
		RandomHUB->MOTD = TEXT("Browse a random collection of servers on the internet.");
		RandomHUB->bFakeHUB = true;
		AllHubServers.Add(RandomHUB);
		FilterAllHUBs();
		HUBServerList->RequestListRefresh();
	}		

}

void SUWServerBrowser::TallyInternetServers(int32& Players, int32& Spectators, int32& Friends)
{
	Players = 0;
	Spectators = 0;
	Friends = 0;

	for (int32 i=0;i<AllInternetServers.Num();i++)
	{
		Players += AllInternetServers[i]->NumPlayers;
		Spectators += AllInternetServers[i]->NumSpectators;
		Friends += AllInternetServers[i]->NumFriends;
	}
}


void SUWServerBrowser::VertSplitterResized()
{
	UUTGameUserSettings* GS = Cast<UUTGameUserSettings>(GEngine->GetGameUserSettings());
	if (GS != NULL)
	{

		FChildren const* const SplitterChildren = VertSplitter->GetChildren();
		for (int32 SlotIndex = 0; SlotIndex < SplitterChildren->Num(); ++SlotIndex)
		{
			SSplitter::FSlot const& SplitterSlot = VertSplitter->SlotAt(SlotIndex);
			GS->SetServerBrowserSplitterPositions(0+SlotIndex, SplitterSlot.SizeValue.Get());
		}
		GS->SaveConfig();
		bRequireSave = false;
	}
}

void SUWServerBrowser::HorzSplitterResized()
{
	UUTGameUserSettings* GS = Cast<UUTGameUserSettings>(GEngine->GetGameUserSettings());
	if (GS != NULL)
	{

		FChildren const* const SplitterChildren = HorzSplitter->GetChildren();
		for (int32 SlotIndex = 0; SlotIndex < SplitterChildren->Num(); ++SlotIndex)
		{
			SSplitter::FSlot const& SplitterSlot = HorzSplitter->SlotAt(SlotIndex);
			GS->SetServerBrowserSplitterPositions(0+SlotIndex, SplitterSlot.SizeValue.Get());
		}
		GS->SaveConfig();
		bRequireSave = false;
	}
}

void SUWServerBrowser::OnServerListSelectionChanged(TSharedPtr<FServerData> SelectedItem, ESelectInfo::Type SelectInfo)
{
	if (SelectedItem.IsValid())
	{
		PingServer(SelectedItem);

		RulesListSource.Empty();
		for (int32 i=0;i<SelectedItem->Rules.Num();i++)
		{
			RulesListSource.Add(SelectedItem->Rules[i]);
		}
		RulesList->RequestListRefresh();

		PlayersListSource.Empty();
		if (SelectedItem.IsValid())
		{
			for (int32 i=0;i<SelectedItem->Players.Num();i++)
			{
				PlayersListSource.Add(SelectedItem->Players[i]);
			}
		}
		PlayersList->RequestListRefresh();

		JoinButton->SetEnabled(true);
		SpectateButton->SetEnabled(true);
	}
}

void SUWServerBrowser::OnQuickFilterTextCommited(const FText& NewText, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter)
	{
		if (bShowingHubs)
		{
			FilterAllHUBs();
		}
		else
		{
			FilterAllServers();
		}
	}
	FilterMsg->SetVisibility( NewText.IsEmpty() ? EVisibility::Visible : EVisibility::Hidden);
}

void SUWServerBrowser::OnFilterTextChanged(const FText& NewText)
{
	FilterMsg->SetVisibility( NewText.IsEmpty() ? EVisibility::Visible : EVisibility::Hidden);
}


FReply SUWServerBrowser::BrowserTypeChanged()
{
	ShowHUBs();
	return FReply::Handled();
}

void SUWServerBrowser::ShowServers(FString InitialGameType)
{
	bShowingHubs = false;
	BuildServerListControlBox();
	LobbyBrowser->SetVisibility(EVisibility::Hidden);
	InternetServerBrowser->SetVisibility(EVisibility::All);
	ServerListControlBox->SetVisibility(EVisibility::All);
	FilterAllServers();
	AddGameFilters();
	InternetServerList->RequestListRefresh();
}

void SUWServerBrowser::ShowHUBs()
{
	bShowingHubs = true;
	BuildServerListControlBox();
	LobbyBrowser->SetVisibility(EVisibility::All);
	InternetServerBrowser->SetVisibility(EVisibility::Hidden);
	ServerListControlBox->SetVisibility(EVisibility::Collapsed);
	FilterAllHUBs();
	HUBServerList->RequestListRefresh();
}


TSharedRef<ITableRow> SUWServerBrowser::OnGenerateWidgetForHUBList(TSharedPtr<FServerData> InItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	bool bPassword = (InItem->Flags & SERVERFLAG_RequiresPassword )  > 0;
	bool bRestricted = (InItem->Flags & SERVERFLAG_Restricted )  > 0;

	return SNew(STableRow<TSharedPtr<FServerData>>, OwnerTable)
		.Style(SUTStyle::Get(),"UT.List.Row")
		[
			SNew(SBox)
			.HeightOverride(64)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.Padding(10.0, 5.0, 10.0, 5.0)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.0,0.0,5.0,0.0)
					[
						AddHUBBadge(InItem)
					]
					
					+SHorizontalBox::Slot()
					.FillWidth(1)
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot()
						.VAlign(VAlign_Top)
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(InItem->Name))
							.TextStyle(SUWindowsStyle::Get(), "UWindows.Standard.HUBBrowser.TitleText")
						]
						+SVerticalBox::Slot()
						.FillHeight(1.0)
						.VAlign(VAlign_Fill)
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot()
							.FillWidth(1.0)
							.Padding(5.0,0.0,20.0,0.0)
							.VAlign(VAlign_Top)
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot()
								.HAlign(HAlign_Fill)
								.AutoHeight()
								[
									SNew(SHorizontalBox)
/*
									+SHorizontalBox::Slot()
									.Padding(0.0,0.0,20.0,0.0)
									.VAlign(VAlign_Center)
									.AutoWidth()
									[
										SNew(SBox)
										.WidthOverride(125)
										.HeightOverride(24)
										[
											AddStars(InItem)
										]
									]
*/
									+SHorizontalBox::Slot()
									.Padding(0.0,0.0,20.0,0.0)
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SNew(STextBlock)
										.Text(InItem->bFakeHUB ? 
											TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(InItem.Get(), &FServerData::GetNumServers)) :
											TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(InItem.Get(), &FServerData::GetNumMatches)))
										.TextStyle(SUWindowsStyle::Get(), "UWindows.Standard.HUBBrowser.NormalText")
									]

									+SHorizontalBox::Slot()
									.Padding(0.0,0.0,20.0,0.0)
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SNew(STextBlock)
										.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(InItem.Get(), &FServerData::GetNumPlayers)))
										.TextStyle(SUWindowsStyle::Get(), "UWindows.Standard.HUBBrowser.NormalText")
									]

									+SHorizontalBox::Slot()
									.Padding(0.0,0.0,20.0,0.0)
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SNew(STextBlock)
										.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(InItem.Get(), &FServerData::GetNumFriends)))
										.TextStyle(SUWindowsStyle::Get(), "UWindows.Standard.HUBBrowser.NormalText")
									]
									+ SHorizontalBox::Slot()
									.FillWidth(1.0)
									+ SHorizontalBox::Slot()
									.Padding(0.0, 0.0, 20.0, 0.0)
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SNew(SBox)
										.HeightOverride(18)
										.WidthOverride(18)
										[
											SNew(SImage)
											.Visibility(bPassword || bRestricted ? EVisibility::HitTestInvisible : EVisibility::Hidden)
											.Image(SUTStyle::Get().GetBrush("UT.Icon.Lock.Small"))
										]
									]
									+SHorizontalBox::Slot()
									.Padding(10.0,0.0,20.0,0.0)
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SNew(SVerticalBox)
										+SVerticalBox::Slot()
										.HAlign(HAlign_Right)
										[
											SNew(STextBlock)
											.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(InItem.Get(), &FServerData::GetHubPing)))
											.TextStyle(SUWindowsStyle::Get(), "UWindows.Standard.HUBBrowser.SmallText")
										]
									]
								]
							]
						]
					]

				]
			]
		];
}

TSharedRef<SWidget> SUWServerBrowser::AddHUBBadge(TSharedPtr<FServerData> HUB)
{

	if (HUB->bFakeHUB || HUB->TrustLevel > 1)
	{
		return 	SNew(SBox)						// First the overlaid box that controls everything....
			.HeightOverride(54)
			.WidthOverride(54)
			[
				SNew(SImage)
				.Image(SUWindowsStyle::Get().GetBrush("UT.Icon.Globe"))
			];
	}
	else
	{
		if (HUB->TrustLevel == 0)
		{
			return 	SNew(SBox).HeightOverride(54).WidthOverride(54)
				[
					SNew(SImage)
					.Image(SUWindowsStyle::Get().GetBrush("UT.Icon.Epic"))
				];
		}
		else
		{
			return 	SNew(SBox).HeightOverride(54).WidthOverride(54)
				[
					SNew(SImage)
					.Image(SUWindowsStyle::Get().GetBrush("UT.Icon.Raxxy"))
				];
		}
	
	}

}

TSharedRef<SWidget> SUWServerBrowser::AddStars(TSharedPtr<FServerData> HUB)
{
	TSharedPtr<SHorizontalBox> StarBox;

	SAssignNew(StarBox, SHorizontalBox);

	if (StarBox.IsValid())
	{
		for (int32 i=0;i<5;i++)
		{
			StarBox->AddSlot()
			.AutoWidth()
			[
				SNew(SBox)
				.WidthOverride(24)
				.HeightOverride(24)
				[
					SNew(SImage)
					.Image(SUWindowsStyle::Get().GetBrush("UWindows.Standard.Star24"))
				]
			];
		}
	}

	return StarBox.ToSharedRef();
}


void SUWServerBrowser::AddHUBInfo(TSharedPtr<FServerData> HUB)
{
	if (LobbyInfoText.IsValid())
	{
		LobbyInfoText->SetText(FText::FromString(HUB->MOTD));
	}
	LobbyMatchPanel->SetServerData(HUB);

}

void SUWServerBrowser::OnHUBListSelectionChanged(TSharedPtr<FServerData> SelectedItem, ESelectInfo::Type SelectInfo)
{
	if (SelectedItem.IsValid())
	{
		if (!SelectedItem->bFakeHUB)
		{
			PingServer(SelectedItem);
		}

		AddHUBInfo(SelectedItem);
		JoinButton->SetEnabled(true);
	}
}

void SUWServerBrowser::BuildServerListControlBox()
{
	if (ServerListControlBox.IsValid())
	{
		ServerListControlBox->ClearChildren();
		if (!bShowingHubs)
		{
			ServerListControlBox->AddSlot()
				.VAlign(VAlign_Center)
				.Padding(0.0f,0.0f,15.0f,0.0f)
				.AutoWidth()
				[
					SNew(SUTButton)
					.ContentPadding(FMargin(10.0f, 5.0f, 15.0f, 5.0))
					.ButtonStyle(SUTStyle::Get(), "UT.Button.MenuBar")
					.Text(NSLOCTEXT("SUWServerBrowser","ShowLobbies","Return to hub list..."))
					.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")
					.OnClicked(this, &SUWServerBrowser::BrowserTypeChanged)
				];

			ServerListControlBox->AddSlot()
				.AutoWidth()
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.HAlign(HAlign_Fill)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.Padding(0.0f,0.0f,5.0f,0.0f)
						.AutoWidth()
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text(NSLOCTEXT("SUWServerBrowser","GameFilter","Game Mode:"))
								.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")
							]
						]
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SAssignNew(GameFilter, SUTComboButton)
							.HasDownArrow(false)
							.ButtonStyle(SUTStyle::Get(), "UT.Button.MenuBar")
							.ContentPadding(FMargin(10.0f, 0.0f, 10.0f, 0.0))
							.Text(this, &SUWServerBrowser::GetGameFilterText)
							.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium.Bold")
						]
					]
				];
		}
	}

}

void SUWServerBrowser::OnShowPanel(TSharedPtr<SUWindowsDesktop> inParentWindow)
{
	SUWPanel::OnShowPanel(inParentWindow);
	if (bNeedsRefresh)
	{
		RefreshServers();
	}
}

FName SUWServerBrowser::GetBrowserState()
{
	return BrowserState;
}

void SUWServerBrowser::JoinQuickInstance(const FString& InstanceGuid, bool bAsSpectator)
{
	TArray<TSharedPtr<FServerData>> SelectedHubs = HUBServerList->GetSelectedItems();

	if (SelectedHubs.Num() > 0 && SelectedHubs[0].IsValid())
	{
		if ((SelectedHubs[0]->Flags & SERVERFLAG_Restricted) > 0)
		{
			RestrictedWarning();
			return;
		}

		PlayerOwner->AttemptJoinInstance(SelectedHubs[0], InstanceGuid, bAsSpectator);
	}

}

#endif
