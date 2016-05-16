// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLocalPlayer.h"
#include "UTGauntletGameState.h"
#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "SUTBuyWindow.h"
#include "../SUWindowsStyle.h"
#include "../Widgets/SUTButton.h"
#include "../SUTUtils.h"
#include "UTPlayerState.h"


#if !UE_SERVER


void SUTBuyWindow::CreateDesktop()
{
	CollectItems();

	int32 CurrentCurrency = 0;
	if (PlayerOwner.IsValid() && PlayerOwner->PlayerController)
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
		if (PC && PC->UTPlayerState)
		{
			CurrentCurrency = int32(PC->UTPlayerState->GetAvailableCurrency());
		}
	}

	ChildSlot
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[
		SNew(SOverlay)
		
		// Shadowed background
		+SOverlay::Slot()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			SNew(SImage)
			.Image(SUWindowsStyle::Get().GetBrush("UT.TopMenu.Shadow"))
		]

		// Actual Menu
		+ SOverlay::Slot()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			// Top Bar

			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(1.0)
				[
					SNew(SBox)
					.HeightOverride(16)
					[
						SNew(SImage)
						.Image(SUWindowsStyle::Get().GetBrush("UT.Loadout.UpperBar"))
					]
				]
			]

			// Title Tab

			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.Padding(32.0f,0.0f,0.0f,0.0f)
				.AutoWidth()
				[			
					SNew(SBox)
					.WidthOverride(350)
					.HeightOverride(56)
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						[
							SNew(SImage)
							.Image(SUWindowsStyle::Get().GetBrush("UT.Loadout.Tab"))
						]
						+SOverlay::Slot()
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.Padding(5.0,2.0,5.0,0.0)
							.HAlign(HAlign_Center)
							[
								SNew(STextBlock)
								.Text(NSLOCTEXT("SUTBuyWindow","BuyTitle","Select Powerup"))
								.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
							]
						]
					]
				]
			]

			// The Menu itself...

			+SVerticalBox::Slot()
			.Padding(10.0f,30.0f, 10.0f,10.0f)
			.HAlign(HAlign_Center)
			.FillHeight(1.0)
			[
				// The List portion
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Center)
					.AutoWidth()
					.Padding(10.0,0.0,20.0,0.0)
					[
						SNew(SBox).WidthOverride(1400)
						[
							SNew(SOverlay)
							+SOverlay::Slot()
							[
								SNew(SImage)
								.Image(SUWindowsStyle::Get().GetBrush("UT.Loadout.List.Normal"))
							]
							+SOverlay::Slot()
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot()
								.HAlign(HAlign_Right)
								.AutoHeight()
								.Padding(10.0,0.0,10.0,0.0)
								[
									SNew(STextBlock)
									.Text(FText::Format(NSLOCTEXT("SUTBuyWindow","Status","{0} - Available Items"), FText::AsNumber(CurrentCurrency)))
									.TextStyle(SUWindowsStyle::Get(),"UT.Dialog.TitleTextStyle")
								]

								+SVerticalBox::Slot()
								.HAlign(HAlign_Center)
								[
									SNew(SScrollBox)
									+SScrollBox::Slot()
									[
										SAssignNew(AvailableItemsPanel, SGridPanel)
									]
								]
							]
						]
					]
				]

				// The Info portion
				+SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(20.0,20.0,20.0,0.0)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.FillWidth(1.0)
					[
						SNew(SBox)
						.WidthOverride(1400)
						.HeightOverride(250)
						[
							SNew(SOverlay)
							+SOverlay::Slot()
							[
								SNew(SImage)
								.Image(SUWindowsStyle::Get().GetBrush("UT.Loadout.List.Normal"))
							]
							+SOverlay::Slot()
							[
								SNew(SHorizontalBox)
							
								+SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(SVerticalBox)
									+SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SBox)
										.WidthOverride(256)
										.HeightOverride(128)
										[
											SNew(SImage)
											.Image(this, &SUTBuyWindow::GetItemImage)
										]
									]
								]
								+SHorizontalBox::Slot()
								.FillWidth(1.0)
								[
									SNew(SRichTextBlock)
									.TextStyle(SUWindowsStyle::Get(),"UT.Hub.RulesText")
									.Justification(ETextJustify::Left)
									.DecoratorStyleSet( &SUWindowsStyle::Get() )
									.AutoWrapText( true )
									.Text(this, &SUTBuyWindow::GetItemDescriptionText)
								]
							]
						]
					]	
				]
			]

		
			// The ButtonBar
			+ SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Bottom)
			.Padding(5.0f, 5.0f, 5.0f, 5.0f)
			[
				SNew(SBox)
				.HeightOverride(48)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.FillWidth(1.0)
						[
							SNew(SOverlay)
							+ SOverlay::Slot()
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								.HAlign(HAlign_Fill)
								[
									SNew(SImage)
									.Image(SUWindowsStyle::Get().GetBrush("UT.TopMenu.DarkFill"))
								]
							]
							+ SOverlay::Slot()
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								.HAlign(HAlign_Right)
								[
									SNew(SUniformGridPanel)
									.SlotPadding(FMargin(0.0f,0.0f, 10.0f, 10.0f))

									+SUniformGridPanel::Slot(0,0)
									.HAlign(HAlign_Fill)
									[
										SNew(SBox)
										.HeightOverride(140)
										[
											SNew(SButton)
											.HAlign(HAlign_Center)
											.ButtonStyle(SUWindowsStyle::Get(), "UT.BottomMenu.Button")
											.ContentPadding(FMargin(5.0f, 5.0f, 5.0f, 5.0f))
											.Text(NSLOCTEXT("SUTLoadoutWindow","Cancel","Cancel Change"))
											.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
											.OnClicked(this, &SUTBuyWindow::OnCancelledClicked)
										]
									]

								]

							]
						]

					]
					+ SOverlay::Slot()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Left)
						.Padding(10.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(SCanvas)
						]
					]
				]
			]

		]

	];

	RefreshAvailableItemsList();
}

void SUTBuyWindow::CollectItems()
{
	// Create the loadout information.
	AUTGameState* GameState = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
	if (GameState && GameState->AvailableLoadout.Num() > 0)
	{
		for (int32 i=0; i < GameState->AvailableLoadout.Num(); i++)
		{
			if (GameState->AvailableLoadout[i]->ItemClass && !GameState->AvailableLoadout[i]->bDefaultInclude)
			{
				TSharedPtr<FLoadoutData> D = FLoadoutData::Make(GameState->AvailableLoadout[i]);
				AvailableItems.Add( D );
			}
		}
	}
}

const FSlateBrush* SUTBuyWindow::GetItemImage() const
{
	if (CurrentItem.IsValid() && CurrentItem->LoadoutInfo.IsValid())
	{
		return CurrentItem->LoadoutInfo->GetItemImage();
	}

	return SUWindowsStyle::Get().GetBrush("UWindows.Lobby.MatchBadge");
}

FText SUTBuyWindow::GetItemDescriptionText() const
{
	if (CurrentItem.IsValid() && CurrentItem->DefaultObject.IsValid())
	{
		return CurrentItem->DefaultObject->MenuDescription;
	}

	return NSLOCTEXT("UTWeapon","DefaultDescription","This space let intentionally blank");
}

FReply SUTBuyWindow::OnCancelledClicked()
{
	PlayerOwner->CloseLoadout();
	return FReply::Handled();
}


void SUTBuyWindow::RefreshAvailableItemsList()
{
	AvailableItems.Sort(FCompareByCost());
	if (AvailableItemsPanel.IsValid())
	{
		AvailableItemsPanel->ClearChildren();
		for (int32 Idx = 0; Idx < AvailableItems.Num(); Idx++)	
		{
			int32 Row = Idx / 5;
			int32 Col = Idx % 5;

			TSharedPtr<SUTButton> But;

			// Add the cell...
			AvailableItemsPanel->AddSlot(Col, Row).Padding(5.0, 5.0, 5.0, 5.0)
				[
					SNew(SBox)
					.WidthOverride(256)
					.HeightOverride(200)
					[
						SAssignNew(But,SUTButton)
						.OnClicked(this, &SUTBuyWindow::OnAvailableClick, Idx)
						.ButtonStyle(SUWindowsStyle::Get(), "UT.ComplexButton")
						.IsToggleButton(true)
						.ToolTip(SUTUtils::CreateTooltip(AvailableItems[Idx]->DefaultObject->DisplayName))
						.UTOnMouseOver(this, &SUTBuyWindow::AvailableUpdateItemInfo)
						.WidgetTag(Idx)
												
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBox)
								.WidthOverride(256)
								.HeightOverride(128)
								[
									SNew(SImage)
									.Image(AvailableItems[Idx]->Image.Get())
								]
							]
							+ SVerticalBox::Slot()
							.HAlign(HAlign_Center)
							.AutoHeight()
							[
								SNew(STextBlock)
								.Text(AvailableItems[Idx]->DefaultObject->DisplayName)
								.TextStyle(SUWindowsStyle::Get(), "UT.Hub.MapsText")
								.ColorAndOpacity(FLinearColor::Black)
							]

							+ SVerticalBox::Slot()
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Bottom)
							.AutoHeight()
							[
								SNew(STextBlock)
								.Text(FText::AsNumber(int32(AvailableItems[Idx]->LoadoutInfo->CurrentCost)))
								.TextStyle(SUWindowsStyle::Get(), "UT.Hub.MapsText")
								.ColorAndOpacity(FLinearColor::Black)
							]
						]
					]
				];

			AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerOwner->PlayerController->PlayerState);
			if (PS && PS->BoostClass == AvailableItems[Idx]->LoadoutInfo->ItemClass)
			{
				But->BePressed();
			}
		}
	}
}

void SUTBuyWindow::AvailableUpdateItemInfo(int32 Index)
{
	if (Index >= 0 && Index <= AvailableItems.Num())
	{
		CurrentItem = AvailableItems[Index];
	}
}

FReply SUTBuyWindow::OnAvailableClick(int32 Index)
{
	if (Index >= 0 && Index <= AvailableItems.Num())
	{
		if (PlayerOwner.IsValid() && PlayerOwner->PlayerController)
		{
			AUTGauntletGameState* GameState = PlayerOwner->GetWorld()->GetGameState<AUTGauntletGameState>();
			AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
			if (PC && PC->UTPlayerState && GameState)
			{
				if (GameState)
				{
					for (int LoadoutInfoIndex = 0; LoadoutInfoIndex < GameState->AvailableLoadout.Num(); ++LoadoutInfoIndex)
					{
						if (GameState->AvailableLoadout[LoadoutInfoIndex]->ItemClass == AvailableItems[Index]->LoadoutInfo->ItemClass)
						{
							PC->UTPlayerState->ServerSetBoostItem(LoadoutInfoIndex);
							PlayerOwner->CloseLoadout();

							break;
						}
					}
				}
			}
		}
	}			
	

	return FReply::Handled();
}

FReply SUTBuyWindow::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		OnCancelledClicked();
	}

	return FReply::Unhandled();
}


#endif