// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateBasics.h"
#include "SUWDialog.h"
#include "UTAudioSettings.h"

#if !UE_SERVER

class SUWSystemSettingsDialog : public SUWDialog
{
public:
	SLATE_BEGIN_ARGS(SUWSystemSettingsDialog)
	: _DialogSize(FVector2D(0.5f,0.8f))
	, _bDialogSizeIsRelative(true)
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

	TArray< TSharedPtr<FString> > ResList;
	TSharedPtr<STextBlock> SelectedRes;
	TSharedPtr<SCheckBox> Fullscreen;
	TSharedPtr<SCheckBox> SmoothFrameRate;
	TSharedPtr<SEditableTextBox> FrameRateCap;
	TSharedPtr<SSlider> SoundVolumes[EUTSoundClass::MAX];
	/** list of display values for general scalability setting that are all set the same way (e.g. low/medium/high) */
	TArray< TSharedPtr<FString> > GeneralScalabilityList;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > TextureRes;
	TSharedPtr<STextBlock> SelectedTextureRes;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > ShadowQuality;
	TSharedPtr<STextBlock> SelectedShadowQuality;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > PPQuality;
	TSharedPtr<STextBlock> SelectedPPQuality;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > EffectQuality;
	TSharedPtr<STextBlock> SelectedEffectQuality;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > AAQuality;
	TSharedPtr<STextBlock> SelectedAAQuality;
	TSharedPtr<SSlider> FOV;
	TSharedPtr<STextBlock> FOVLabel;
	TSharedPtr<SSlider> DecalLifetime;

	/** range of values passed to PlayerInput->SetMouseSensitivity() which will be normalized to 0.0 - 1.0 for the slider widget */
	FVector2D MouseSensitivityRange;
	/** range of values for decal lifetime which will be normalized to 0.0 - 1.0 for the slider widget
	 * note that the max value (1.0 on the slider) becomes infinite lifetime
	 */
	FVector2D DecalLifetimeRange;

	virtual FReply OnButtonClick(uint16 ButtonID);	


	FReply OKClick();
	FReply CancelClick();
	void OnResolutionSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void OnTextureResolutionSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void OnShadowQualitySelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void OnPPQualitySelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void OnEffectQualitySelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void OnAAQualitySelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void OnFOVChange(float NewValue);

	FString GetFOVLabelText(int32 FOVAngle);

	SVerticalBox::FSlot& AddSectionHeader(const FText& SectionDesc);
	SVerticalBox::FSlot& AddGeneralScalabilityWidget(const FString& Desc, TSharedPtr< SComboBox< TSharedPtr<FString> > >& ComboBox, TSharedPtr<STextBlock>& SelectedItemWidget, void (SUWSystemSettingsDialog::*SelectionFunc)(TSharedPtr<FString>, ESelectInfo::Type), int32 SettingValue);
	SVerticalBox::FSlot& AddGeneralSliderWidget(const FString& Desc, TSharedPtr<SSlider>& SliderWidget, float SettingValue);
};

#endif