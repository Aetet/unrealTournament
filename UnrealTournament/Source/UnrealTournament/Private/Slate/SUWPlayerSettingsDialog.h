// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateBasics.h"
#include "SUWDialog.h"

#if !UE_SERVER
class SUWPlayerSettingsDialog : public SUWDialog, public FGCObject
{
public:

	SLATE_BEGIN_ARGS(SUWPlayerSettingsDialog)
	: _DialogSize(FVector2D(0.5f,0.8f))
	, _bDialogSizeIsRelative(true)
	, _DialogPosition(FVector2D(0.5f,0.5f))
	, _DialogAnchorPoint(FVector2D(0.5f,0.5f))
	, _ContentPadding(FVector2D(10.0f, 5.0f))
	{}
	SLATE_ARGUMENT(TWeakObjectPtr<class UUTLocalPlayer>, PlayerOwner)												
	SLATE_ARGUMENT(FVector2D, DialogSize)										
	SLATE_ARGUMENT(bool, bDialogSizeIsRelative)									
	SLATE_ARGUMENT(FVector2D, DialogPosition)									
	SLATE_ARGUMENT(FVector2D, DialogAnchorPoint)								
	SLATE_ARGUMENT(FVector2D, ContentPadding)									
	SLATE_EVENT(FDialogResultDelegate, OnDialogResult)							
	SLATE_END_ARGS()


	void Construct(const FArguments& InArgs);
	virtual ~SUWPlayerSettingsDialog();

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

protected:

	/** world for rendering the player preview */
	class UWorld* PlayerPreviewWorld;
	/** view state for player preview (needed for LastRenderTime to work) */
	FSceneViewStateReference ViewState;
	/** preview actors */
	class AUTCharacter* PlayerPreviewMesh;
	/** render target for player mesh and cosmetic items */
	class UUTCanvasRenderTarget2D* PlayerPreviewTexture;
	/** material for the player preview since Slate doesn't support rendering the target directly */
	class UMaterialInstanceDynamic* PlayerPreviewMID;
	/** Slate brush to render the preview */
	FSlateBrush* PlayerPreviewBrush;

	/** counter for displaying weapon dialog since we need to display the "Loading Content" message first */
	int32 WeaponConfigDelayFrames;

	class UDirectionalLightComponent* PreviewLight;

	TSharedPtr<SEditableTextBox> PlayerName;
	TSharedPtr<SSlider> WeaponBobScaling, ViewBobScaling;
	FLinearColor SelectedPlayerColor;
	TSharedPtr<SWidget> PlayerColorPicker;

	TArray<TSharedPtr<FString>> HatList;
	TArray<FString> HatPathList;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > HatComboBox;
	TSharedPtr<STextBlock> SelectedHat;
	void OnHatSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	
	TArray<TSharedPtr<FString>> EyewearList;
	TArray<FString> EyewearPathList;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > EyewearComboBox;
	TSharedPtr<STextBlock> SelectedEyewear;
	void OnEyewearSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	TArray<TSharedPtr<FString>> CharacterList;
	TArray<FString> CharacterPathList;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > CharacterComboBox;
	TSharedPtr<STextBlock> SelectedCharacter;
	void OnCharacterSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	TSharedPtr<STextBlock> SelectedFlag;
	TArray<TSharedPtr<FString>> CountyFlagNames;
	TSharedPtr< SComboBox< TSharedPtr<FString> > > CountryFlagComboBox;
	void OnFlagSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	int32 Emote1Index;
	int32 Emote2Index;
	int32 Emote3Index;

	void OnEmote1Committed(int32 NewValue, ETextCommit::Type CommitInfo);
	void OnEmote2Committed(int32 NewValue, ETextCommit::Type CommitInfo);
	void OnEmote3Committed(int32 NewValue, ETextCommit::Type CommitInfo);
	TOptional<int32> GetEmote1Value() const;
	TOptional<int32> GetEmote2Value() const;
	TOptional<int32> GetEmote3Value() const;

	virtual FReply OnButtonClick(uint16 ButtonID);

	FReply OKClick();
	FReply CancelClick();
	FReply WeaponConfigClick();
	FReply PlayerColorClicked(const FGeometry& Geometry, const FPointerEvent& Event);
	FReply CloseColorPicker();
	FLinearColor GetSelectedPlayerColor() const
	{
		return SelectedPlayerColor;
	}
	void PlayerColorChanged(FLinearColor NewValue);
	void OnNameTextChanged(const FText& NewText);

	virtual void DragPlayerPreview(FVector2D MouseDelta);
	virtual void RecreatePlayerPreview();
	virtual void UpdatePlayerRender(UCanvas* C, int32 Width, int32 Height);

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObject(PlayerPreviewTexture);
		Collector.AddReferencedObject(PlayerPreviewMID);
		Collector.AddReferencedObject(PlayerPreviewWorld);
		Collector.AddReferencedObject(PreviewLight);
	}
};
#endif