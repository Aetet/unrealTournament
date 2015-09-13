
// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "../Public/UnrealTournament.h"
#include "../Public/UTLocalPlayer.h"
#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "../SUWindowsStyle.h"
#include "../SUTStyle.h"
#include "../Widgets/SUTPopOverAnchor.h"
#include "UTGameEngine.h"
#include "UTLobbyGameState.h"
#include "UTLobbyMatchInfo.h"
#include "SUMatchPanel.h"
#include "UTLobbyPC.h"
#include "UTLobbyPlayerState.h"
#include "SUWServerBrowser.h"
#if !UE_SERVER

struct FMatchComparePlayersByScore {FORCEINLINE bool operator()( const FMatchPlayerListStruct A, const FMatchPlayerListStruct B ) const { return ( A.PlayerScore > B.PlayerScore);}};

void SUMatchPanel::Construct(const FArguments& InArgs)
{
	PlayerOwner = InArgs._PlayerOwner;
	bShowingNoMatches = false;
	OnJoinMatchDelegate = InArgs._OnJoinMatchDelegate;
	bExpectServerData = InArgs._bExpectServerData;

	TSharedPtr<SVerticalBox> VertBox;

	ChildSlot.VAlign(VAlign_Fill).HAlign(HAlign_Fill)
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		[
			SAssignNew(VertBox, SVerticalBox)
		]
	];

	if (!bExpectServerData)
	{
		VertBox->AddSlot()
		.VAlign(VAlign_Center)
		.AutoHeight()
		[
			SNew(SBox)												 
			.HeightOverride(75)
			.WidthOverride(954)
			[
				SNew(SOverlay)
				+SOverlay::Slot()
				[
					SNew(SImage)
					.Image(SUTStyle::Get().GetBrush("UT.HeaderBackground.SuperDark"))
				]
				+SOverlay::Slot()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.Padding(22.0,0.0,0.0,0.0)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(this, &SUMatchPanel::GetServerNameText)
						.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
					]
				]

				+SOverlay::Slot()
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Right)
					[
						SNew(SHorizontalBox)
/*
						+SHorizontalBox::Slot()
						.Padding(0.0,0.0,10.0,0.0)
						.AutoWidth()
						.HAlign(HAlign_Right)
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SCheckBox)
								.Style(SUTStyle::Get(), "UT.CheckBox")
								.IsChecked(ECheckBoxState::Checked)
							]
							+SHorizontalBox::Slot()
							.VAlign(VAlign_Center)
							.AutoWidth()
							[
								SNew(STextBlock)
								.Text(NSLOCTEXT("SUMatchPanel","UnJoinableTitle","Hide unjoinable matches"))
								.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
							]
						]
*/
						+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(10.0,5.0,21.0,5.0)

						.HAlign(HAlign_Right)
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.VAlign(VAlign_Center)
							.AutoHeight()
							[
								SNew(SButton)
								.ButtonStyle(SUTStyle::Get(),"UT.SimpleButton")
								.OnClicked(this, &SUMatchPanel::StartNewMatch)
								.ContentPadding(FMargin(32.0,5.0,32.0,5.0))
								[
									SNew(STextBlock)
									.Text(this, &SUMatchPanel::GetMatchButtonText)
									.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
								]
							]
						]
					]
				]
			]
		];
	}

	VertBox->AddSlot()
	.VAlign(VAlign_Top)
	.AutoHeight()
	.Padding(0,12,0,0)
	[
		SNew(SBox)												
		.WidthOverride(954).HeightOverride(893)
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			[
				SNew(SImage)
				.Image(SUTStyle::Get().GetBrush("UT.HeaderBackground.Medium"))
			]

			+SOverlay::Slot()
			[
				SAssignNew( NoMatchesBox, SVerticalBox)
			]

			+SOverlay::Slot()
			[
				SAssignNew( MatchList, SListView< TSharedPtr<FTrackedMatch> > )
				// List view items are this tall
				.ItemHeight(80)
				// Tell the list view where to get its source data
				.ListItemsSource( &TrackedMatches)
				// When the list view needs to generate a widget for some data item, use this method
				.OnGenerateRow( this, &SUMatchPanel::OnGenerateWidgetForMatchList )
				.SelectionMode(ESelectionMode::Single)
				.OnMouseButtonDoubleClick(this, &SUMatchPanel::OnListMouseButtonDoubleClick)
			]
		]
	];
}

TSharedRef<ITableRow> SUMatchPanel::OnGenerateWidgetForMatchList( TSharedPtr<FTrackedMatch> InItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	int32 Index = TrackedMatches.Find(InItem) + 1;

	return SNew(STableRow<TSharedPtr<FSimpleListData>>, OwnerTable)
		.Style(SUTStyle::Get(),"UT.MatchList.Row")
		.Padding(FMargin(1.0,1.0,1.0,1.0))
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.HAlign(HAlign_Center)
			.AutoHeight()
			[
				SNew(SUTPopOverAnchor)
				.OnGetPopoverWidget(this, &SUMatchPanel::OnGetPopup)
				.AssociatedActor(InItem->MatchInfo)
				.AssociatedString(InItem->MatchId.ToString())
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						SNew(SBox).WidthOverride(78).HeightOverride(78)
						[
							SNew(SOverlay)
							+SOverlay::Slot()
							[
								SNew(SImage)
								.Image(SUTStyle::Get().GetBrush("UT.MatchBadge.Circle.Thin"))
							]
							+SOverlay::Slot()
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot()
								.FillHeight(1.0)
								.VAlign(VAlign_Center)
								[
									SNew(SHorizontalBox)
									+SHorizontalBox::Slot()
									.FillWidth(1.0)
									.HAlign(HAlign_Center)
									[
										SNew(STextBlock)
										.Text(FText::AsNumber(Index))
										.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
									]
								]
							]
						]
					]
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						SNew(SBox).HeightOverride(78).WidthOverride(408).Padding(FMargin(5.0f, 0.0f, 5.0f, 0.0f))
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SHorizontalBox)
								+SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(STextBlock)
									.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(InItem.Get(), &FTrackedMatch::GetRuleTitle)))
									.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium.Bold")
									.ColorAndOpacity(FSlateColor(FLinearColor(0.8f,0.8f,0.8f,1.0f)))
								]
							]
							+SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SHorizontalBox)
								+SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(STextBlock)
									.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(InItem.Get(), &FTrackedMatch::GetMap)))
									.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
									.ColorAndOpacity(FSlateColor(FLinearColor(0.6f,0.6f,0.6f,1.0f)))
								]
							]
						]
					]
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						SNew(SBox).WidthOverride(155).HeightOverride(78)
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.FillHeight(1.0)
							.VAlign(VAlign_Bottom)
							[
								SNew(SRichTextBlock)
								.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Tiny")
								.Justification(ETextJustify::Left)
								.DecoratorStyleSet( &SUTStyle::Get() )
								.AutoWrapText( true )
								.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(InItem.Get(), &FTrackedMatch::GetFlags, PlayerOwner)))
								+ SRichTextBlock::ImageDecorator()
							]
						]
					]
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						SNew(SBox).WidthOverride(103).HeightOverride(78)
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.FillHeight(1.0)
							.VAlign(VAlign_Center)
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot()
								.HAlign(HAlign_Right)
								.AutoHeight()
								[
									SNew(STextBlock)
									.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(InItem.Get(), &FTrackedMatch::GetCurrentPlayerCount)))
									.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium.Bold")
									.ColorAndOpacity(FSlateColor(FLinearColor(0.8f,0.8f,0.8f,1.0f)))
								]
								+SVerticalBox::Slot()
								.AutoHeight()
								.HAlign(HAlign_Right)
								[
									SNew(STextBlock)
									.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(InItem.Get(), &FTrackedMatch::GetMaxPlayers)))
									.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
									.ColorAndOpacity(FSlateColor(FLinearColor(0.5f,0.5f,0.5f,1.0f)))
								]
							]
						]
					]
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.AutoWidth()
					[
						SNew(SBox).WidthOverride(92).HeightOverride(78)
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot()
								.VAlign(VAlign_Center)
								.AutoHeight()
								.Padding(16.0,0.0,0.0,0.0)
								[
									SNew(SBox).WidthOverride(64).HeightOverride(64)
									[

										SNew(SOverlay)
										+SOverlay::Slot()
										[
											SNew(SImage)
											.Image(InItem.Get(), &FTrackedMatch::GetBadge)
										]
										+SOverlay::Slot()
										[
											SNew(SVerticalBox)
											+SVerticalBox::Slot()
											.FillHeight(1.0)
											.VAlign(VAlign_Center)
											[
												SNew(SHorizontalBox)
												+SHorizontalBox::Slot()
												.FillWidth(1.0)
												.HAlign(HAlign_Center)
												[
													SNew(STextBlock)
													.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(InItem.Get(), &FTrackedMatch::GetRank)))
													.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium.Bold")
													.ColorAndOpacity(FSlateColor(FLinearColor(0.0f,0.0f,0.0f,1.0f)))
												]
											]
										]
									]
								]
							]
						]
					]
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.AutoWidth()
					.Padding(0.0,0.0,5.0,0.0)
					[
						SNew(SBox).WidthOverride(115).HeightOverride(78)
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.Padding(0.0,8.0,0.0,0.0)
							.AutoHeight()
							[
								SNew(SButton)
								.ButtonStyle(SUTStyle::Get(),"UT.SimpleButton.Medium")
								.OnClicked(this, &SUMatchPanel::JoinMatchButtonClicked, InItem)
								.IsEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(InItem.Get(), &FTrackedMatch::CanJoin)))
								[
									SNew(SVerticalBox)
									+SVerticalBox::Slot().HAlign(HAlign_Center).AutoHeight()
									[
										SNew(STextBlock)
										.Text(NSLOCTEXT("SUMatchPanel","JoinText","JOIN"))
										.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
									]
								]
							]
							+SVerticalBox::Slot()
							.Padding(0.0,10.0,0.0,0.0)
							.AutoHeight()
							[
								SNew(SButton)
								.ButtonStyle(SUTStyle::Get(),"UT.SimpleButton.Medium")
								.OnClicked(this, &SUMatchPanel::SpectateMatchButtonClicked, InItem)
								.IsEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(InItem.Get(), &FTrackedMatch::CanSpectate)))
								[
									SNew(SVerticalBox)
									+SVerticalBox::Slot().HAlign(HAlign_Center).AutoHeight()
									[
										SNew(STextBlock)
										.Text(NSLOCTEXT("SUMatchPanel","SpectateText","SPECTATE"))
										.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
									]
								]
							
							]
						]
					]
				]

			]
		]; 
}


FText SUMatchPanel::GetServerNameText() const
{
	AUTLobbyGameState* LobbyGameState = GWorld->GetGameState<AUTLobbyGameState>();
	if (LobbyGameState)
	{
		return FText::FromString(LobbyGameState->ServerName);
	}

	return FText::GetEmpty();
}

FText SUMatchPanel::GetMatchButtonText() const
{
	return NSLOCTEXT("Gerneric","NewMatch","START NEW MATCH");
}

int32 SUMatchPanel::IsTrackingMatch(AUTLobbyMatchInfo* Match)
{
	for (int32 i = 0; i < TrackedMatches.Num(); i++)
	{
		if (TrackedMatches[i]->MatchInfo.IsValid() && TrackedMatches[i]->MatchInfo.Get() == Match)
		{
			return i;
		}
	}

	return INDEX_NONE;
}

void SUMatchPanel::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	if (bExpectServerData) return;	

	for (int32 i = 0; i < TrackedMatches.Num(); i++)
	{
		TrackedMatches[i]->bPendingKill = true;
	}

	bool bListNeedsUpdate = false;

	AUTLobbyGameState* GameState = PlayerOwner->GetWorld()->GetGameState<AUTLobbyGameState>();
	if (GameState)
	{
		// Look at the available matches
		for (int32 i = 0; i < GameState->AvailableMatches.Num(); i++)
		{
			AUTLobbyMatchInfo* Match = GameState->AvailableMatches[i];
			if (Match && GameState->IsMatchStillValid(Match) && Match->ShouldShowInDock())
			{
				int32 Idx = IsTrackingMatch(Match);
				if (Idx != INDEX_NONE)
				{
					TrackedMatches[Idx]->bPendingKill = false;
				}
				else
				{
					// We need to add this match
					TrackedMatches.Add(FTrackedMatch::Make(Match));
					bListNeedsUpdate = true;
				}
			}
		}

		// Look at running instances
		for (int32 i = 0; i < GameState->GameInstances.Num(); i++)
		{
			AUTLobbyMatchInfo* Match = GameState->GameInstances[i].MatchInfo;
			if (Match && GameState->IsMatchStillValid(Match) && Match->ShouldShowInDock())
			{
				int32 Idx = IsTrackingMatch(Match);
				if (Idx != INDEX_NONE)
				{
					TrackedMatches[Idx]->bPendingKill = false;
				}
				else
				{
					// We need to add this match
					TrackedMatches.Add(FTrackedMatch::Make(Match));
					bListNeedsUpdate = true;
				}
			}
		}
	
	}

	// Remove any entries that are pending kill
	for (int32 i = TrackedMatches.Num() - 1; i >= 0; i--)
	{
		if (TrackedMatches[i]->bPendingKill) 
		{
			bListNeedsUpdate = true;
			TrackedMatches.RemoveAt(i);
		}
	}

	// Update the friends.
	TArray<FUTFriend> FriendsList;
	PlayerOwner->GetFriendsList(FriendsList);

	for (int32 i=0; i < TrackedMatches.Num(); i++)
	{
		if (TrackedMatches[i]->MatchInfo.IsValid())
		{
			TrackedMatches[i]->NumFriends = TrackedMatches[i]->MatchInfo->CountFriendsInMatch(FriendsList);
		}
	}


	if (bShowingNoMatches && TrackedMatches.Num() > 0)
	{
		bShowingNoMatches = false;
		NoMatchesBox->ClearChildren();
	}
	else if (TrackedMatches.Num() == 0 && !bShowingNoMatches)
	{
		bShowingNoMatches = true;
		NoMatchesBox->AddSlot().AutoHeight().Padding(10.0f,10.0f,0.0f,0.0f)
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("SUMatchPanel","NoMatches","There are no active matches.  Start one!"))
			.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")
		];
	}

	if (bListNeedsUpdate)
	{
		MatchList->RequestListRefresh();	
	}
}
 

FReply SUMatchPanel::StartNewMatch()
{
	AUTLobbyGameState* GameState = PlayerOwner->GetWorld()->GetGameState<AUTLobbyGameState>();
	if (GameState && PlayerOwner->PlayerController && PlayerOwner->PlayerController->PlayerState)
	{
		AUTLobbyPlayerState* LobbyPlayerState = Cast<AUTLobbyPlayerState>(PlayerOwner->PlayerController->PlayerState);
		if (LobbyPlayerState)
		{
			LobbyPlayerState->ServerCreateMatch();
		}
	}
	return FReply::Handled();
}	

TSharedRef<SWidget> SUMatchPanel::OnGetPopupContent(TSharedPtr<SUTPopOverAnchor> Anchor)
{

	// Create the player list..

	TSharedPtr<SVerticalBox> VertBox;
	SAssignNew(VertBox,SVerticalBox);

	TArray<FMatchPlayerListStruct> ColumnA;
	TArray<FMatchPlayerListStruct> ColumnB;
	FString Spectators;

	// Holds a list of mutators running on this map.
	FString RulesList = TEXT("");

	bool bTeamGame = false;

	if (bExpectServerData)
	{
		TSharedPtr<FServerInstanceData> Instance;
		// Find the instance data
		for (int32 i=0; i < ServerData->HUBInstances.Num(); i++)
		{
			if (ServerData->HUBInstances[i]->InstanceId.ToString() == Anchor->AssociatedString)
			{
				Instance = ServerData->HUBInstances[i];
				break;
			}
		}
	
		if (!Instance.IsValid())
		{
			return SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.Padding(0.0,0.0,0.0,5.0)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.Padding(0.0,0.0,5.0,0.0)
				.AutoWidth()
				[
					SNew(SBox).WidthOverride(200)
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot().HAlign(HAlign_Left).AutoHeight().Padding(5.0f,5.0f,5.0f,5.0f)
						[
							SNew(STextBlock)
							.Text(NSLOCTEXT("SUMatchPanel","NoData","Loading...."))
							.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Tiny.Bold")
						]
					]
				]
			];
		}
		else
		{
			Instance->Players.Sort(FMatchComparePlayersByScore());
			for (int32 i = 0; i < Instance->Players.Num(); i++)
			{
				if (Instance->bTeamGame)
				{
					if (Instance->Players[i].TeamNum == 0) ColumnA.Add(Instance->Players[i]);
					else if (Instance->Players[i].TeamNum == 1) ColumnB.Add(Instance->Players[i]);
				}
				else
				{
					if (i%2==0)
					{
						ColumnA.Add(Instance->Players[i]);
					}
					else
					{
						ColumnB.Add(Instance->Players[i]);
					}
				}
			}
			bTeamGame = Instance->bTeamGame;
			RulesList = Instance->MutatorList;
		}
	}
	else
	{
		if (Anchor->AssociatedActor.IsValid())
		{
			AUTLobbyMatchInfo* MatchInfo = Cast<AUTLobbyMatchInfo>(Anchor->AssociatedActor.Get());
			if (MatchInfo)
			{
				if (MatchInfo->CurrentRuleset.IsValid())
				{
					MatchInfo->FillPlayerColumnsForDisplay(ColumnA, ColumnB, Spectators);
					bTeamGame = MatchInfo->CurrentRuleset->bTeamGame;
				}

				RulesList = MatchInfo->CurrentRuleset->Description;
			}
		}
	}

	// Generate a info widget from the UTLobbyMatchInfo


	int32 Max = FMath::Max<int32>(ColumnA.Num(), ColumnB.Num());

	// First we add the player list...

	if (Max > 0)
	{
		if (bTeamGame)
		{
			VertBox->AddSlot()
			.Padding(0.0,0.0,0.0,5.0)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.Padding(0.0,0.0,5.0,0.0)
				.AutoWidth()
				[
					SNew(SBox).WidthOverride(200)
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot().HAlign(HAlign_Left).AutoHeight().Padding(5.0f,0.0f,0.0f,0.0f)
						[
							SNew(STextBlock)
							.Text(NSLOCTEXT("SUMatchPanel","RedTeam","Red Team"))
							.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Tiny.Bold")
						]
					]
				]
				+SHorizontalBox::Slot()
				.Padding(5.0,0.0,0.0,0.0)
				.AutoWidth()
				.HAlign(HAlign_Right)
				[
					SNew(SBox).WidthOverride(200)
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot().HAlign(HAlign_Right).AutoHeight().Padding(0.0f,0.0f,5.0f,0.0f)
						[
							SNew(STextBlock)
							.Text(NSLOCTEXT("SUMatchPanel","BlueTeam","Blue Team"))
							.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Tiny.Bold")
						]
					]
				]
			];
		}
		else
		{
			VertBox->AddSlot()
			.Padding(0.0,0.0,0.0,5.0)
			.HAlign(HAlign_Center)
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("SUMatchPanel","Players","Players in Match"))
				.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Tiny.Bold")
			];
		}

		TSharedPtr<SBox> ListBox;
		TSharedPtr<SVerticalBox> ListVBox;
		SAssignNew(ListBox, SBox).MinDesiredHeight(150)
		[
			SAssignNew(ListVBox, SVerticalBox)				
		];

		for (int32 i = 0; i < Max; i++)
		{
			ListVBox->AddSlot()
			.AutoHeight()
			.Padding(0.0,0.0,0.0,2.0)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.Padding(0.0,0.0,5.0,0.0)
				.AutoWidth()
				[
					SNew(SBox).WidthOverride(200)
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						[
							SNew(SImage)
							.Image(SUTStyle::Get().GetBrush( (i%2==0 ? FName(TEXT("UT.ListBackground.Even")) : FName(TEXT("UT.ListBackground.Odd")))  ))
						]
						+SOverlay::Slot()
						[
							SNew(SOverlay)
							+SOverlay::Slot()
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot().HAlign(HAlign_Left).AutoHeight()
								.Padding(5.0,0.0,0.0,0.0)
								[
									SNew(STextBlock)
									.Text(FText::FromString( (i < ColumnA.Num() ? ColumnA[i].PlayerName : TEXT(""))  ))
									.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
								]

							]
							+SOverlay::Slot()
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot().HAlign(HAlign_Right).AutoHeight()
								.Padding(0.0,0.0,5.0,0.0)
								[
									SNew(STextBlock)
									.Text(FText::FromString( (i < ColumnA.Num() ? ColumnA[i].PlayerScore : TEXT(""))  ))
									.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
								]

							]
						]
					]
				]
				+SHorizontalBox::Slot()
				.Padding(5.0,0.0,0.0,0.0)
				.AutoWidth()
				[
					SNew(SBox).WidthOverride(200)
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						[
							SNew(SImage)
							.Image(SUTStyle::Get().GetBrush( (i%2==0 ? FName(TEXT("UT.ListBackground.Even")) : FName(TEXT("UT.ListBackground.Odd")))  ))
						]
						+SOverlay::Slot()
						[
							SNew(SOverlay)
							+SOverlay::Slot()
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot().HAlign(HAlign_Left).AutoHeight()
								.Padding(5.0,0.0,0.0,0.0)
								[
									SNew(STextBlock)
									.Text(FText::FromString( (i < ColumnB.Num() ? ColumnB[i].PlayerName : TEXT(""))  ))
									.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
								]

							]
							+SOverlay::Slot()
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot().HAlign(HAlign_Right).AutoHeight()
								.Padding(0.0,0.0,5.0,0.0)
								[
									SNew(STextBlock)
									.Text(FText::FromString( (i < ColumnB.Num() ? ColumnB[i].PlayerScore : TEXT(""))  ))
									.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
								]
							]
						]
					]
				]
			];
		}

		VertBox->AddSlot().AutoHeight()
		[
			ListBox.ToSharedRef()
		];

		if (!Spectators.IsEmpty())
		{
			// Add Spectators

			VertBox->AddSlot()
			.HAlign(HAlign_Center)
			.Padding(0.0f,10.0f,0.0f,5.0)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("SUMatchPanel","Spectators","Spectators"))
				.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Tiny.Bold")
			];

			VertBox->AddSlot()
			.HAlign(HAlign_Center)
			.Padding(5.0f,0.0f,5.0f,5.0)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(1.0)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Spectators))
					.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Tiny")
					.AutoWrapText(true)
				]
			];
		}


		if (!RulesList.IsEmpty())
		{
			VertBox->AddSlot().AutoHeight().Padding(0.0,0.0,0.0,5.0).HAlign(HAlign_Fill)
			[
				SNew(SBorder)
				.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.SuperLight"))
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("SUMatchPanel","RulesTitle","Game Rules"))
						.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Tiny.Bold")
					]
				]
			];

			VertBox->AddSlot().AutoHeight().Padding(5.0,0.0,5.0,5.0)
			[
				SNew(SRichTextBlock)
				.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Tiny")
				.Justification(ETextJustify::Left)
				.DecoratorStyleSet( &SUWindowsStyle::Get() )
				.AutoWrapText( true )
				.Text(FText::FromString(RulesList))
			];
		}

	}
	return VertBox.ToSharedRef();

}

TSharedRef<SWidget> SUMatchPanel::OnGetPopup(TSharedPtr<SUTPopOverAnchor> Anchor)
{
	return SNew(SBox).WidthOverride(420)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5.0,5.0,5.0,5.0)
			[
				SNew(SOverlay)
				+SOverlay::Slot()
				[
					SNew(SImage)
					.Image(SUTStyle::Get().GetBrush("UT.HeaderBackground.Medium"))
				]
				+SOverlay::Slot()
				[
					OnGetPopupContent(Anchor)
				]
			]
		];
}

void SUMatchPanel::OnListMouseButtonDoubleClick(TSharedPtr<FTrackedMatch> SelectedMatch)
{

	if (!SelectedMatch->CanJoin()) return;

	if (bExpectServerData)
	{
		if (OnJoinMatchDelegate.IsBound())
		{
			OnJoinMatchDelegate.Execute(SelectedMatch->MatchId.ToString(), false);
		}
	}
	else
	{
		AUTLobbyGameState* GameState = PlayerOwner->GetWorld()->GetGameState<AUTLobbyGameState>();
		if (SelectedMatch.IsValid() && GameState && PlayerOwner->PlayerController && PlayerOwner->PlayerController->PlayerState)
		{
			AUTLobbyPlayerState* LobbyPlayerState = Cast<AUTLobbyPlayerState>(PlayerOwner->PlayerController->PlayerState);
			if (LobbyPlayerState)
			{
				LobbyPlayerState->ServerJoinMatch(SelectedMatch->MatchInfo.Get(), false);
			}
		}
	}
}



FReply SUMatchPanel::JoinMatchButtonClicked(TSharedPtr<FTrackedMatch> InItem)
{
	if (InItem.IsValid() && InItem->CanJoin() )
	{
		if (bExpectServerData)
		{
			if (OnJoinMatchDelegate.IsBound())
			{
				OnJoinMatchDelegate.Execute(InItem->MatchId.ToString(), false);
			}
		}
		else if (InItem.IsValid() && InItem->MatchInfo.IsValid())
		{
			if (PlayerOwner.IsValid() && PlayerOwner->PlayerController && PlayerOwner->PlayerController->PlayerState)
			{
				AUTLobbyPlayerState* LobbyPlayerState = Cast<AUTLobbyPlayerState>(PlayerOwner->PlayerController->PlayerState);
				if (LobbyPlayerState)
				{
					LobbyPlayerState->ServerJoinMatch(InItem->MatchInfo.Get(),false);
				}
			}
		}
	}
	return FReply::Handled();
}
FReply SUMatchPanel::SpectateMatchButtonClicked(TSharedPtr<FTrackedMatch> InItem)
{
	if (InItem.IsValid() && InItem->CanSpectate())
	{
		if (bExpectServerData)
		{
			if (OnJoinMatchDelegate.IsBound())
			{
				OnJoinMatchDelegate.Execute(InItem->MatchId.ToString(), true);
			}
		}
		else if (InItem.IsValid() && InItem->MatchInfo.IsValid())
		{
			if (PlayerOwner.IsValid() && PlayerOwner->PlayerController && PlayerOwner->PlayerController->PlayerState)
			{
				AUTLobbyPlayerState* LobbyPlayerState = Cast<AUTLobbyPlayerState>(PlayerOwner->PlayerController->PlayerState);
				if (LobbyPlayerState)
				{
					LobbyPlayerState->ServerJoinMatch(InItem->MatchInfo.Get(),true);
				}
			}
		}	
	}

	return FReply::Handled();
}


void SUMatchPanel::SetServerData(TSharedPtr<FServerData> inServerData)
{
	TrackedMatches.Empty();

	// Update the friends.
	TArray<FUTFriend> FriendsList;
	PlayerOwner->GetFriendsList(FriendsList);

	ServerData = inServerData;
	for (int32 i=0; i < ServerData->HUBInstances.Num(); i++)
	{
		TSharedPtr<FServerInstanceData> Instance = ServerData->HUBInstances[i];
		if (Instance.IsValid())
		{
			int32 Index = TrackedMatches.Add(FTrackedMatch::Make(Instance->InstanceId, Instance->RulesTitle, Instance->MapName, Instance->NumPlayers, Instance->MaxPlayers, Instance->NumFriends, Instance->Flags, Instance->Rank));

			// Count the # of friends....
			int32 NumFriends = 0;
			for (int32 j=0; j < Instance->Players.Num(); j++)
			{
				for (int32 k=0; k < FriendsList.Num(); k++)
				{
					if (FriendsList[k].UserId == Instance->Players[j].PlayerId)
					{
						NumFriends++;
						break;
					}
				}
			}

			TrackedMatches[Index]->NumFriends = NumFriends;
		}
	}
	MatchList->RequestListRefresh();
}


#endif