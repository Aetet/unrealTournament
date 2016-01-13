// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLocalPlayer.h"
#include "SUTHUDSettingsDialog.h"
#include "../SUWindowsStyle.h"
#include "UTPlayerInput.h"
#include "Scalability.h"
#include "UTWorldSettings.h"
#include "UTGameEngine.h"
#include "../SUTUtils.h"

#if !UE_SERVER


void SUTHUDSettingsDialog::Construct(const FArguments& InArgs)
{
	AUTPlayerController* PC = Cast<AUTPlayerController>(InArgs._PlayerOwner->PlayerController);
	if (PC && PC->MyUTHUD)
	{
		TargetHUD = PC->MyUTHUD;
	}
	bInGame = TargetHUD.IsValid();

	SUTDialogBase::Construct(SUTDialogBase::FArguments()
							.PlayerOwner(InArgs._PlayerOwner)
							.DialogTitle(InArgs._DialogTitle)
							.DialogSize(InArgs._DialogSize)
							.bDialogSizeIsRelative(InArgs._bDialogSizeIsRelative)
							.DialogPosition(InArgs._DialogPosition)
							.DialogAnchorPoint(InArgs._DialogAnchorPoint)
							.ContentPadding(InArgs._ContentPadding)
							.ButtonMask(InArgs._ButtonMask)
							.bShadow(!bInGame) //Don't shadow if ingame
							.OnDialogResult(InArgs._OnDialogResult)
						);

	//If main menu and no HUD, use the default
	if (!TargetHUD.IsValid())
	{
		// We need to create a hud..

		FActorSpawnParameters SpawnInfo;
		SpawnInfo.Owner = PlayerOwner->PlayerController;
		SpawnInfo.ObjectFlags |= RF_Transient;	
		TargetHUD = PlayerOwner->GetWorld()->SpawnActor<AUTHUD>(AUTHUD::StaticClass(), SpawnInfo );
	}

	//Since TargetHUD might be the CDO, Save off all the property data if we need to revert
	Old_HUDWidgetOpacity = TargetHUD->HUDWidgetOpacity;
	Old_HUDWidgetBorderOpacity = TargetHUD->HUDWidgetBorderOpacity;
	Old_HUDWidgetSlateOpacity = TargetHUD->HUDWidgetSlateOpacity;
	Old_HUDWidgetWeaponbarInactiveOpacity = TargetHUD->HUDWidgetWeaponbarInactiveOpacity;
	Old_HUDWidgetWeaponBarScaleOverride = TargetHUD->HUDWidgetWeaponBarScaleOverride;
	Old_HUDWidgetWeaponBarInactiveIconOpacity = TargetHUD->HUDWidgetWeaponBarInactiveIconOpacity;
	Old_HUDWidgetWeaponBarEmptyOpacity = TargetHUD->HUDWidgetWeaponBarEmptyOpacity;
	Old_HUDWidgetScaleOverride = TargetHUD->HUDWidgetScaleOverride;
	Old_bUseWeaponColors = TargetHUD->bUseWeaponColors;
	Old_bDrawChatKillMsg = TargetHUD->bDrawChatKillMsg;
	Old_bDrawPopupKillMsg = TargetHUD->bDrawPopupKillMsg;

	FVector2D ViewportSize;
	GetPlayerOwner()->ViewportClient->GetViewportSize(ViewportSize);

	if (DialogContent.IsValid())
	{
		if (TargetHUD.IsValid() )
		{
			DialogContent->AddSlot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.Padding(10.0f, 0.0f, 10.0f, 0.0f)
				.VAlign(VAlign_Fill)
				.HAlign(HAlign_Fill)
				.AutoHeight()
				[
					SNew(STextBlock)
					.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
					.Justification(ETextJustify::Center)
					.WrapTextAt(InArgs._DialogSize.X * 0.6)
					.Text(bInGame ? FText::GetEmpty() : NSLOCTEXT("SUTHUDSettingsDialog", "MainMenuInstruction", "If you modify these options while in game, you will instantly see the effects of your changes."))
				]
				+SVerticalBox::Slot()
					.Padding(FMargin(10.0f, 10.0f, 10.0f, 0.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.Padding(10.0f, 0.0f, 10.0f, 0.0f)
						.AutoWidth()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						[
							SAssignNew(HUDOpacityLabel, STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(this, &SUTHUDSettingsDialog::GetHUDOpacityLabel)
							.ToolTip(SUTUtils::CreateTooltip(NSLOCTEXT("SUTHUDSettingsDialog","HUDOpacityTT","Adjusts how transparent the HUD should be.")))
						]
						+ SHorizontalBox::Slot()
						.Padding(10.0f, 0.0f, 10.0f, 0.0f)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Right)
						[
							SNew(SBox)
							.WidthOverride(300.0f)
							.Content()
							[
								SAssignNew(HUDOpacity, SSlider)
								.Style(SUWindowsStyle::Get(),"UT.Common.Slider")
								.Orientation(Orient_Horizontal)
								.Value(TargetHUD->HUDWidgetOpacity)
								.OnValueChanged(this, &SUTHUDSettingsDialog::OnHUDOpacityChanged)
							]
						]
					]

				+SVerticalBox::Slot()
					.Padding(FMargin(10.0f, 10.0f, 10.0f, 0.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.Padding(10.0f, 0.0f, 10.0f, 0.0f)
						.AutoWidth()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						[
							SAssignNew(HUDOpacityLabel, STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(this, &SUTHUDSettingsDialog::GetHUDBorderOpacityLabel)
							.ToolTip(SUTUtils::CreateTooltip(NSLOCTEXT("SUTHUDSettingsDialog","HUDBorderOpacityTT","Adjusts how transparent the hard edge border around each element of the HUD should be.")))
						]
						+ SHorizontalBox::Slot()
						.Padding(10.0f, 0.0f, 10.0f, 0.0f)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Right)
						[
							SNew(SBox)
							.WidthOverride(300.0f)
							.Content()
							[
								SAssignNew(HUDOpacity, SSlider)
								.Style(SUWindowsStyle::Get(),"UT.Common.Slider")
								.Orientation(Orient_Horizontal)
								.Value(TargetHUD->HUDWidgetBorderOpacity)
								.OnValueChanged(this, &SUTHUDSettingsDialog::OnHUDBorderOpacityChanged)
							]
						]
					]

				+SVerticalBox::Slot()
					.Padding(FMargin(10.0f, 10.0f, 10.0f, 0.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.Padding(10.0f, 0.0f, 10.0f, 0.0f)
						.AutoWidth()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						[
							SAssignNew(HUDOpacityLabel, STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(this, &SUTHUDSettingsDialog::GetHUDSlateOpacityLabel)
							.ToolTip(SUTUtils::CreateTooltip(NSLOCTEXT("SUTHUDSettingsDialog","HUDSlateOpacityTT","Adjusts how transparent the background portion of each HUD element should be.")))
						]
						+ SHorizontalBox::Slot()
						.Padding(10.0f, 0.0f, 10.0f, 0.0f)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Right)
						[
							SNew(SBox)
							.WidthOverride(300.0f)
							.Content()
							[
								SAssignNew(HUDOpacity, SSlider)
								.Style(SUWindowsStyle::Get(),"UT.Common.Slider")
								.Orientation(Orient_Horizontal)
								.Value(TargetHUD->HUDWidgetSlateOpacity)
								.OnValueChanged(this, &SUTHUDSettingsDialog::OnHUDSlateOpacityChanged)
							]
						]
					]

				+SVerticalBox::Slot()
					.Padding(FMargin(10.0f, 10.0f, 10.0f, 0.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.Padding(10.0f, 0.0f, 10.0f, 0.0f)
						.AutoWidth()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						[
							SAssignNew(HUDOpacityLabel, STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(this, &SUTHUDSettingsDialog::GetHUDScaleLabel)
							.ToolTip(SUTUtils::CreateTooltip(NSLOCTEXT("SUTHUDSettingsDialog","HUDScaleTT","Makes the HUD elements bigger or smaller.")))
						]
						+ SHorizontalBox::Slot()
						.Padding(10.0f, 0.0f, 10.0f, 0.0f)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Right)
						[
							SNew(SBox)
							.WidthOverride(300.0f)
							.Content()
							[
								SAssignNew(HUDOpacity, SSlider)
								.Style(SUWindowsStyle::Get(),"UT.Common.Slider")
								.Orientation(Orient_Horizontal)
								.Value(TargetHUD->HUDWidgetScaleOverride / 2.0f)
								.OnValueChanged(this, &SUTHUDSettingsDialog::OnHUDScaleChanged)
							]
						]
					]

				+SVerticalBox::Slot()
					.Padding(FMargin(10.0f, 10.0f, 10.0f, 0.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.Padding(10.0f, 0.0f, 10.0f, 0.0f)
						.AutoWidth()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						[
							SAssignNew(HUDOpacityLabel, STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(this, &SUTHUDSettingsDialog::GetWeaponBarOpacityLabel)
							.ToolTip(SUTUtils::CreateTooltip(NSLOCTEXT("SUTHUDSettingsDialog","HUDWeaponBarOpaictyTT","Adjusts how transparent the Weapon Bar should be.  NOTE this is applied in addition to the normal transparency setting.")))
						]
						+ SHorizontalBox::Slot()
						.Padding(10.0f, 0.0f, 10.0f, 0.0f)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Right)
						[
							SNew(SBox)
							.WidthOverride(300.0f)
							.Content()
							[
								SAssignNew(HUDOpacity, SSlider)
								.Orientation(Orient_Horizontal)
								.Style(SUWindowsStyle::Get(),"UT.Common.Slider")
								.Value(TargetHUD->HUDWidgetWeaponbarInactiveOpacity)
								.OnValueChanged(this, &SUTHUDSettingsDialog::OnWeaponBarOpacityChanged)
							]
						]
					]

				+SVerticalBox::Slot()
					.Padding(FMargin(10.0f, 10.0f, 10.0f, 0.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.Padding(10.0f, 0.0f, 10.0f, 0.0f)
						.AutoWidth()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						[
							SAssignNew(HUDOpacityLabel, STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(this, &SUTHUDSettingsDialog::GetWeaponBarIconOpacityLabel)
							.ToolTip(SUTUtils::CreateTooltip(NSLOCTEXT("SUTHUDSettingsDialog","HUDWeaponBarIconOpacityTT","Adjusts how transparent the icons on the Weapon Bar should be.")))
						]
						+ SHorizontalBox::Slot()
						.Padding(10.0f, 0.0f, 10.0f, 0.0f)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Right)
						[
							SNew(SBox)
							.WidthOverride(300.0f)
							.Content()
							[
								SAssignNew(HUDOpacity, SSlider)
								.Orientation(Orient_Horizontal)
								.Style(SUWindowsStyle::Get(),"UT.Common.Slider")
								.Value(TargetHUD->HUDWidgetWeaponBarInactiveIconOpacity)
								.OnValueChanged(this, &SUTHUDSettingsDialog::OnWeaponBarIconOpacityChanged)
							]
						]
					]

				+SVerticalBox::Slot()
					.Padding(FMargin(10.0f, 10.0f, 10.0f, 0.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.Padding(10.0f, 0.0f, 10.0f, 0.0f)
						.AutoWidth()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						[
							SAssignNew(HUDOpacityLabel, STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(this, &SUTHUDSettingsDialog::GetWeaponBarEmptyOpacityLabel)
							.ToolTip(SUTUtils::CreateTooltip(NSLOCTEXT("SUTHUDSettingsDialog","HUDWeaponBarEmptyOpacityTT","Adjusts how transparent an empty Weapon Bar slot should be.")))
						]
						+ SHorizontalBox::Slot()
						.Padding(10.0f, 0.0f, 10.0f, 0.0f)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Right)
						[
							SNew(SBox)
							.WidthOverride(300.0f)
							.Content()
							[
								SAssignNew(HUDOpacity, SSlider)
								.Style(SUWindowsStyle::Get(),"UT.Common.Slider")
								.Orientation(Orient_Horizontal)
								.Value(TargetHUD->HUDWidgetWeaponBarEmptyOpacity)
								.OnValueChanged(this, &SUTHUDSettingsDialog::OnWeaponBarEmptyOpacityChanged)
							]
						]
					]


				+SVerticalBox::Slot()
					.Padding(FMargin(10.0f, 10.0f, 10.0f, 0.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.Padding(10.0f, 0.0f, 10.0f, 0.0f)
						.AutoWidth()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						[
							SAssignNew(HUDOpacityLabel, STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(this, &SUTHUDSettingsDialog::GetWeaponBarScaleLabel)
							.ToolTip(SUTUtils::CreateTooltip(NSLOCTEXT("SUTHUDSettingsDialog","HUDWeaponBarScaleTT","Adjusts how big or small the Weapon Bar should be.")))
						]
						+ SHorizontalBox::Slot()
						.Padding(10.0f, 0.0f, 10.0f, 0.0f)
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Right)
						[
							SNew(SBox)
							.WidthOverride(300.0f)
							.Content()
							[
								SAssignNew(HUDOpacity, SSlider)
								.Style(SUWindowsStyle::Get(),"UT.Common.Slider")
								.Orientation(Orient_Horizontal)
								.Value(TargetHUD->HUDWidgetWeaponBarScaleOverride / 2.0f)
								.OnValueChanged(this, &SUTHUDSettingsDialog::OnWeaponBarScaleChanged)
							]
						]
					]
				+ SVerticalBox::Slot()
					.Padding(FMargin(10.0f, 10.0f, 10.0f, 0.0f))
					.HAlign(HAlign_Right)
					[
						SAssignNew(DrawChatKillMsg, SCheckBox)
						.ForegroundColor(FLinearColor::White)
						.IsChecked(TargetHUD->bDrawChatKillMsg ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
						.OnCheckStateChanged(this, &SUTHUDSettingsDialog::OnDrawChatKillMsgMsgChanged)
						.Style(SUWindowsStyle::Get(), "UT.Common.CheckBox")
						.Content()
						[
							SNew(STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(NSLOCTEXT("SUTHUDSettingsDialog", "ChatKillMessages", "Show Kill Messages In Chat"))
						]
					]
				+ SVerticalBox::Slot()
					.Padding(FMargin(10.0f, 10.0f, 10.0f, 0.0f))
					.HAlign(HAlign_Right)
					[
						SAssignNew(DrawPopupKillMsg, SCheckBox)
						.ForegroundColor(FLinearColor::White)
						.IsChecked(TargetHUD->bDrawPopupKillMsg ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
						.OnCheckStateChanged(this, &SUTHUDSettingsDialog::OnDrawPopupKillMsgChanged)
						.Style(SUWindowsStyle::Get(), "UT.Common.CheckBox")
						.Content()
						[
							SNew(STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(NSLOCTEXT("SUTHUDSettingsDialog", "DrawPopupKillMsg", "Popup Kill Messages"))
						]
					]
				+ SVerticalBox::Slot()
					.Padding(FMargin(10.0f, 10.0f, 10.0f, 0.0f))
					.HAlign(HAlign_Right)
					[
						SAssignNew(UseWeaponColor, SCheckBox)
						.ForegroundColor(FLinearColor::White)
						.IsChecked(TargetHUD->bUseWeaponColors ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
						.OnCheckStateChanged(this, &SUTHUDSettingsDialog::OnUseWeaponColorChanged)
						.Style(SUWindowsStyle::Get(), "UT.Common.CheckBox")
						.Content()
						[
							SNew(STextBlock)
							.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
							.Text(NSLOCTEXT("SUTHUDSettingsDialog", "UseWeaponColors", "Colorize Icons"))
							.ToolTip(SUTUtils::CreateTooltip(NSLOCTEXT("SUTHUDSettingsDialog", "HUDWeaponColorsTT", "Whether to colorize weapon bar icons.")))
						]
					]
			];
		}
		else
		{
			DialogContent->AddSlot()
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.TextStyle(SUWindowsStyle::Get(),"UWindows.Standard.Dialog.TextStyle")
					.Text(NSLOCTEXT("SUTHUDSettingsDialog","NeedsHUD","No HUD available!  Please use in game."))
				]
			];
		}
	}
}

FText SUTHUDSettingsDialog::GetHUDOpacityLabel() const
{
	return FText::Format(NSLOCTEXT("HUDSETTINGS","OpacityLabel","General Opacity ({0})"), FText::AsNumber(TargetHUD.IsValid() ? TargetHUD->HUDWidgetOpacity : 0.f));
}

FText SUTHUDSettingsDialog::GetHUDBorderOpacityLabel() const
{
	return FText::Format(NSLOCTEXT("HUDSETTINGS", "BorderOpacityLabel", "Border Opacity ({0})"), FText::AsNumber(TargetHUD.IsValid() ? TargetHUD->HUDWidgetBorderOpacity : 0.f));
}

FText SUTHUDSettingsDialog::GetHUDSlateOpacityLabel() const
{
	return FText::Format(NSLOCTEXT("HUDSETTINGS", "SlateOpacityLabel", "Slate Opacity ({0})"), FText::AsNumber(TargetHUD.IsValid() ? TargetHUD->HUDWidgetSlateOpacity : 0.f));
}

FText SUTHUDSettingsDialog::GetHUDScaleLabel() const
{
	return FText::Format(NSLOCTEXT("HUDSETTINGS", "ScaleLabel", "Scale ({0})"), FText::AsNumber(TargetHUD.IsValid() ? TargetHUD->HUDWidgetScaleOverride : 0.f));
}

FText SUTHUDSettingsDialog::GetWeaponBarOpacityLabel() const
{
	return FText::Format(NSLOCTEXT("HUDSETTINGS", "WeaponBarOpacityLabel", "Weapon Bar Opacity ({0})"), FText::AsNumber(TargetHUD.IsValid() ? TargetHUD->HUDWidgetWeaponbarInactiveOpacity : 0.f));
}

FText SUTHUDSettingsDialog::GetWeaponBarIconOpacityLabel() const
{
	return FText::Format(NSLOCTEXT("HUDSETTINGS", "WeaponBarIconOpacityLabel", "Weapon Bar Icon/Label Opacity ({0})"), FText::AsNumber(TargetHUD.IsValid() ? TargetHUD->HUDWidgetWeaponBarInactiveIconOpacity : 0.f));
}

FText SUTHUDSettingsDialog::GetWeaponBarEmptyOpacityLabel() const
{
	return FText::Format(NSLOCTEXT("HUDSETTINGS", "WeaponBarEmptyOpacityLabel", "Weapon Bar Empty Slot Opacity ({0})"), FText::AsNumber(TargetHUD.IsValid() ? TargetHUD->HUDWidgetWeaponBarEmptyOpacity : 0.f));
}


FText SUTHUDSettingsDialog::GetWeaponBarScaleLabel() const
{
	return FText::Format(NSLOCTEXT("HUDSETTINGS", "WeaponBarScaleLabel", "Weapon Bar Scale ({0})"), FText::AsNumber(TargetHUD.IsValid() ? TargetHUD->HUDWidgetWeaponBarScaleOverride : 0.f));
}


void SUTHUDSettingsDialog::OnHUDOpacityChanged(float NewValue)
{
	if (TargetHUD.IsValid())
	{
		TargetHUD->HUDWidgetOpacity = float(int32(NewValue * 100.0f)) / 100.0f;
	}
}

void SUTHUDSettingsDialog::OnHUDBorderOpacityChanged(float NewValue)
{
	if (TargetHUD.IsValid())
	{
		TargetHUD->HUDWidgetBorderOpacity = float(int32(NewValue * 100.0f)) / 100.0f;
	}
}

void SUTHUDSettingsDialog::OnHUDSlateOpacityChanged(float NewValue)
{
	if (TargetHUD.IsValid())
	{
		TargetHUD->HUDWidgetSlateOpacity = float(int32(NewValue * 100.0f)) / 100.0f;
	}
}

void SUTHUDSettingsDialog::OnHUDScaleChanged(float NewValue)
{
	if (TargetHUD.IsValid())
	{
		TargetHUD->HUDWidgetScaleOverride = 2.0 * float(int32(NewValue * 100.0f)) / 100.0f;
	}
}

void SUTHUDSettingsDialog::OnWeaponBarOpacityChanged(float NewValue)
{
	if (TargetHUD.IsValid())
	{
		TargetHUD->HUDWidgetWeaponbarInactiveOpacity = float(int32(NewValue * 100.0f)) / 100.0f;
	}
}

void SUTHUDSettingsDialog::OnWeaponBarIconOpacityChanged(float NewValue)
{
	if (TargetHUD.IsValid())
	{
		TargetHUD->HUDWidgetWeaponBarInactiveIconOpacity = float(int32(NewValue * 100.0f)) / 100.0f;
	}
}

void SUTHUDSettingsDialog::OnWeaponBarEmptyOpacityChanged(float NewValue)
{
	if (TargetHUD.IsValid())
	{
		TargetHUD->HUDWidgetWeaponBarEmptyOpacity = float(int32(NewValue * 100.0f)) / 100.0f;
	}
}



void SUTHUDSettingsDialog::OnWeaponBarScaleChanged(float NewValue)
{
	if (TargetHUD.IsValid())
	{
		TargetHUD->HUDWidgetWeaponBarScaleOverride = 2.0 * float(int32(NewValue * 100.0f)) / 100.0f;
	}
}

void SUTHUDSettingsDialog::OnUseWeaponColorChanged(ECheckBoxState NewState)
{
	if (TargetHUD.IsValid())
	{
		TargetHUD->bUseWeaponColors = NewState == ECheckBoxState::Checked;
	}
}

void SUTHUDSettingsDialog::OnDrawPopupKillMsgChanged(ECheckBoxState NewState)
{
	if (TargetHUD.IsValid())
	{
		TargetHUD->bDrawPopupKillMsg = NewState == ECheckBoxState::Checked;
	}
}

void SUTHUDSettingsDialog::OnDrawChatKillMsgMsgChanged(ECheckBoxState NewState)
{
	if (TargetHUD.IsValid())
	{
		TargetHUD->bDrawChatKillMsg = NewState == ECheckBoxState::Checked;
	}
}

FReply SUTHUDSettingsDialog::OKClick()
{
	//Save the new values to all AUTHUD CDOs
	if (TargetHUD.IsValid())
	{
		for (TObjectIterator<AUTHUD> It(EObjectFlags::RF_NoFlags,true); It; ++It)
		{
			if (It->HasAnyFlags(RF_ClassDefaultObject))
			{
				AUTHUD* HudCDO = (*It);
				HudCDO->HUDWidgetOpacity = TargetHUD->HUDWidgetOpacity;
				HudCDO->HUDWidgetBorderOpacity = TargetHUD->HUDWidgetBorderOpacity;
				HudCDO->HUDWidgetSlateOpacity = TargetHUD->HUDWidgetSlateOpacity;
				HudCDO->HUDWidgetWeaponbarInactiveOpacity = TargetHUD->HUDWidgetWeaponbarInactiveOpacity;
				HudCDO->HUDWidgetWeaponBarScaleOverride = TargetHUD->HUDWidgetWeaponBarScaleOverride;
				HudCDO->HUDWidgetWeaponBarInactiveIconOpacity = TargetHUD->HUDWidgetWeaponBarInactiveIconOpacity;
				HudCDO->HUDWidgetWeaponBarEmptyOpacity = TargetHUD->HUDWidgetWeaponBarEmptyOpacity;
				HudCDO->HUDWidgetScaleOverride = TargetHUD->HUDWidgetScaleOverride;
				HudCDO->bUseWeaponColors = TargetHUD->bUseWeaponColors;
				HudCDO->bDrawChatKillMsg = TargetHUD->bDrawChatKillMsg;
				HudCDO->bDrawPopupKillMsg = TargetHUD->bDrawPopupKillMsg;

				if (HudCDO == AUTHUD::StaticClass()->GetDefaultObject<AUTHUD>())
				{
					HudCDO->SaveConfig();
				}
			}
		}
	}

	TargetHUD.Reset();
	GetPlayerOwner()->HideHUDSettings();
	return FReply::Handled();
}

FReply SUTHUDSettingsDialog::CancelClick()
{
	if (TargetHUD.IsValid())
	{
		TargetHUD->HUDWidgetOpacity = Old_HUDWidgetOpacity;
		TargetHUD->HUDWidgetBorderOpacity = Old_HUDWidgetBorderOpacity;
		TargetHUD->HUDWidgetSlateOpacity = Old_HUDWidgetSlateOpacity;
		TargetHUD->HUDWidgetWeaponbarInactiveOpacity = Old_HUDWidgetWeaponbarInactiveOpacity;
		TargetHUD->HUDWidgetWeaponBarScaleOverride = Old_HUDWidgetWeaponBarScaleOverride;
		TargetHUD->HUDWidgetWeaponBarInactiveIconOpacity = Old_HUDWidgetWeaponBarInactiveIconOpacity;
		TargetHUD->HUDWidgetWeaponBarEmptyOpacity = Old_HUDWidgetWeaponBarEmptyOpacity;
		TargetHUD->HUDWidgetScaleOverride = Old_HUDWidgetScaleOverride;
		TargetHUD->bUseWeaponColors = Old_bUseWeaponColors;
		TargetHUD->bDrawChatKillMsg = Old_bDrawChatKillMsg;
		TargetHUD->bDrawPopupKillMsg = Old_bDrawPopupKillMsg;
	}

	TargetHUD.Reset();
	GetPlayerOwner()->HideHUDSettings();
	return FReply::Handled();
}


FReply SUTHUDSettingsDialog::OnButtonClick(uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_OK) OKClick();
	else if (ButtonID == UTDIALOG_BUTTON_CANCEL) CancelClick();
	return FReply::Handled();
}

#endif