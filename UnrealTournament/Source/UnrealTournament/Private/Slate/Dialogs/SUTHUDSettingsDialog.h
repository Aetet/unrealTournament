// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateBasics.h"
#include "SWidgetSwitcher.h"
#include "../Base/SUTDialogBase.h"
#include "../Widgets/SUTButton.h"
#include "UTAudioSettings.h"

#if !UE_SERVER

class UNREALTOURNAMENT_API SUTHUDSettingsDialog : public SUTDialogBase
{
public:
	SLATE_BEGIN_ARGS(SUTHUDSettingsDialog)
	: _DialogTitle(NSLOCTEXT("SUTHUDSettingsDialog","Title","HUD Settings"))
	, _DialogSize(FVector2D(1000, 700))
	, _bDialogSizeIsRelative(false)
	, _DialogPosition(FVector2D(0.5f,0.5f))
	, _DialogAnchorPoint(FVector2D(0.5f,0.5f))
	, _ContentPadding(FVector2D(10.0f, 5.0f))
	, _ButtonMask(UTDIALOG_BUTTON_OK | UTDIALOG_BUTTON_CANCEL)
	{}
	SLATE_ARGUMENT(TWeakObjectPtr<class UUTLocalPlayer>, PlayerOwner)			
	SLATE_ARGUMENT(FText, DialogTitle)											
	SLATE_ARGUMENT(FVector2D, DialogSize)										
	SLATE_ARGUMENT(bool, bDialogSizeIsRelative)									
	SLATE_ARGUMENT(FVector2D, DialogPosition)									
	SLATE_ARGUMENT(FVector2D, DialogAnchorPoint)								
	SLATE_ARGUMENT(FVector2D, ContentPadding)									
	SLATE_ARGUMENT(uint16, ButtonMask)
	SLATE_EVENT(FDialogResultDelegate, OnDialogResult)							
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
protected:
	SVerticalBox::FSlot& AddSliderWidget(TSharedPtr<STextBlock>& LabelWidget, TSharedPtr<SSlider>& SliderWidget, float StartingValue);

	TSharedPtr<SWidgetSwitcher> TabWidget;
	TSharedPtr<SUTButton> GeneralSettingsTabButton;
	TSharedPtr<SUTButton> WeaponBarSettingsTabButton;
	TSharedPtr<SUTButton> NotificationsSettingsTabButton;

	TSharedRef<SWidget> BuildGeneralTab();
	TSharedRef<SWidget> BuildWeaponBarTab();
	TSharedRef<SWidget> BuildNotificationsTab();

	FReply OnTabClickGeneral();
	FReply OnTabClickWeaponBar();
	FReply OnTabClickNotifications();

	TSharedPtr<SSlider> HUDOpacity;
	TSharedPtr<STextBlock> HUDOpacityLabel;

	TSharedPtr<SSlider> HUDBorderOpacity;
	TSharedPtr<STextBlock> HUDBorderOpacityLabel;

	TSharedPtr<SSlider> HUDSlateOpacity;
	TSharedPtr<STextBlock> HUDSlateOpacityLabel;

	TSharedPtr<SSlider> HUDScale;
	TSharedPtr<STextBlock> HUDScaleLabel;

	TSharedPtr<SSlider> WeaponBarOpacity;
	TSharedPtr<STextBlock> WeaponBarOpacityLabel;

	TSharedPtr<SSlider> WeaponBarIconOpacity;
	TSharedPtr<STextBlock> WeaponBarIconOpacityLabel;

	TSharedPtr<SSlider> WeaponBarEmptyOpacity;
	TSharedPtr<STextBlock> WeaponBarEmptyOpacityLabel;

	TSharedPtr<SCheckBox> UseWeaponColor;

	TSharedPtr<SCheckBox> DrawPopupKillMsg;
	TSharedPtr<SCheckBox> DrawChatKillMsg;
	TSharedPtr<SCheckBox> DrawHUDKillIconMsg;
	TSharedPtr<SCheckBox> DrawPlayKillSoundMsg;
	
	// A reference to the target HUD..
	TWeakObjectPtr<AUTHUD> TargetHUD;

	FReply OnButtonClick(uint16 ButtonID);
	FReply OKClick();
	FReply CancelClick();

	FText GetHUDOpacityLabel() const;
	FText GetHUDBorderOpacityLabel() const;
	FText GetHUDSlateOpacityLabel() const;
	FText GetHUDScaleLabel() const;
	FText GetWeaponBarOpacityLabel() const;
	FText GetWeaponBarIconOpacityLabel() const;
	FText GetWeaponBarEmptyOpacityLabel() const;
	FText GetWeaponBarScaleLabel() const;

	void OnHUDOpacityChanged(float NewValue);
	void OnHUDBorderOpacityChanged(float NewValue);
	void OnHUDSlateOpacityChanged(float NewValue);
	void OnHUDScaleChanged(float NewValue);

	void OnWeaponBarOpacityChanged(float NewValue);
	void OnWeaponBarIconOpacityChanged(float NewValue);
	void OnWeaponBarEmptyOpacityChanged(float NewValue);
	void OnWeaponBarScaleChanged(float NewValue);
	void OnUseWeaponColorChanged(ECheckBoxState NewState);

	void OnDrawCenteredKillMsgChanged(ECheckBoxState NewState);
	void OnDrawChatKillMsgMsgChanged(ECheckBoxState NewState);
	void OnDrawHUDKillIconMsgChanged(ECheckBoxState NewState);
	void OnPlayKillSoundMsgChanged(ECheckBoxState NewState);

private:

	/**The backup vars for when the player cancels*/
	float Old_HUDWidgetOpacity;
	float Old_HUDWidgetBorderOpacity;
	float Old_HUDWidgetSlateOpacity;
	float Old_HUDWidgetWeaponbarInactiveOpacity;
	float Old_HUDWidgetWeaponBarScaleOverride;
	float Old_HUDWidgetWeaponBarInactiveIconOpacity;
	float Old_HUDWidgetWeaponBarEmptyOpacity;
	float Old_HUDWidgetScaleOverride;
	bool Old_bUseWeaponColors;
	bool Old_bDrawChatKillMsg;
	bool Old_bDrawPopupKillMsg;
	bool Old_bDrawHUDKillIconMsg;
	bool Old_bPlayKillSoundMsg;

	bool bInGame;
};

#endif