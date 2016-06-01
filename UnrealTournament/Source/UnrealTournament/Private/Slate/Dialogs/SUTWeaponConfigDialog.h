// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateBasics.h"
#include "../Base/SUTDialogBase.h"

#if !UE_SERVER

class UNREALTOURNAMENT_API SUTWeaponConfigDialog : public SUTDialogBase, public FGCObject
{
public:
	SLATE_BEGIN_ARGS(SUTWeaponConfigDialog)
		: _DialogSize(FVector2D(1800,1000))
		, _bDialogSizeIsRelative(false)
		, _DialogPosition(FVector2D(0.5f, 0.5f))
		, _DialogAnchorPoint(FVector2D(0.5f, 0.5f))
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

	TSharedPtr<class SUTTabWidget> TabWidget;

	TSharedPtr<SBoxPanel> GroupControlsBox;
	TSharedPtr<SCheckBox> ClassicGroups;
	TSharedPtr<SNumericEntryBox<int32>> GroupEdit;
	TSharedPtr<SCheckBox> AutoWeaponSwitch;

	TArray<UClass*> WeaponClassList;
	TArray<TWeakObjectPtr<UClass>> WeakWeaponClassList;
	TSharedPtr< SListView<TWeakObjectPtr<UClass>> > WeaponList;

	TWeakObjectPtr<UClass> SelectedWeapon;
	void OnWeaponChanged(TWeakObjectPtr<UClass> NewSelectedWeapon, ESelectInfo::Type SelectInfo);
	TSharedRef<ITableRow> GenerateWeaponListRow(TWeakObjectPtr<UClass> WeaponClass, const TSharedRef<STableViewBase>& OwningList);
	FReply WeaponPriorityUp();
	FReply WeaponPriorityDown();

	TMap<UClass*, int32> WeaponGroups;

	TArray< TSharedPtr<FText> > WeaponHandList;
	TArray<FText> WeaponHandDesc;
	TSharedPtr< SComboBox< TSharedPtr<FText> > > WeaponHand;
	TSharedPtr<STextBlock> SelectedWeaponHand;

	TSharedRef<SWidget> GenerateHandListWidget(TSharedPtr<FText> InItem);
	void OnHandSelected(TSharedPtr<FText> NewSelection, ESelectInfo::Type SelectInfo);


	/** render target for the WYSIWYG crosshair */
	class UUTCanvasRenderTarget2D* CrosshairPreviewTexture;
	/** material for the crosshair preview since Slate doesn't support rendering the target directly */
	class UMaterialInstanceDynamic* CrosshairPreviewMID;
	/** Slate brush to render the preview */
	FSlateBrush* CrosshairPreviewBrush;

	virtual void UpdateCrosshairRender(UCanvas* C, int32 Width, int32 Height);

	bool bCustomWeaponCrosshairs;
	ECheckBoxState GetCustomWeaponCrosshairs() const;
	void SetCustomWeaponCrosshairs(ECheckBoxState NewState);

	TSharedPtr<SImage> CrosshairImage;
	TSharedPtr<SOverlay> ColorOverlay;
	TSharedPtr<class SUTColorPicker> ColorPicker;
	TSharedPtr<STextBlock> CrosshairText;

	TSharedPtr<FCrosshairInfo> SelectedCrosshairInfo;

	TArray<TSharedPtr<FCrosshairInfo> > CrosshairInfos;
	TSharedPtr< SListView<TSharedPtr<FCrosshairInfo> > > CrosshairInfosList;

	TMap<FString, TWeakObjectPtr<UClass>> WeaponMap;
	TMap<FString, TWeakObjectPtr<UClass>> CrosshairMap;

	TArray<UClass*> CrosshairClassList;
	TArray<TWeakObjectPtr<UClass>> WeakCrosshairList;

	TSharedPtr<SComboBox< TWeakObjectPtr<UClass>  > > CrosshairComboBox;
	void OnCrosshairSelected(TWeakObjectPtr<UClass> NewSelection, ESelectInfo::Type SelectInfo);
	TSharedRef<SWidget> GenerateCrosshairRow(TWeakObjectPtr<UClass>  CrosshairClass);

	TOptional<float> GetCrosshairScale() const
	{
		return SelectedCrosshairInfo.IsValid() ? TOptional<float>(SelectedCrosshairInfo->Scale) : TOptional<float>(1.0f);
	}

	void SetCrosshairScale(float InScale)
	{
		if (SelectedCrosshairInfo.IsValid())
		{
			SelectedCrosshairInfo->Scale = InScale;
		}
	}

	FLinearColor GetCrosshairColor() const
	{
		return SelectedCrosshairInfo.IsValid() ? SelectedCrosshairInfo->Color : FLinearColor::White;
	}

	void SetCrosshairColor(FLinearColor NewColor)
	{
		if (SelectedCrosshairInfo.IsValid())
		{
			SelectedCrosshairInfo->Color = NewColor;
		}
	}

	TSharedPtr<SComboBox< TSharedPtr<FString>  > > WeaponSkinComboBox;
	TSharedPtr<STextBlock> WeaponSkinText;
	TArray<TSharedPtr<FString>> WeaponSkinList;

	TMap< FString, TArray<UUTWeaponSkin*> > WeaponToSkinListMap;
	TMap< FString, FString > WeaponSkinSelection;
	void OnWeaponSkinSelected(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	TSharedRef<SWidget> GenerateStringListWidget(TSharedPtr<FString> InItem);
	void UpdateAvailableWeaponSkins();

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	virtual FReply OnButtonClick(uint16 ButtonID);
	FReply OKClick();
	FReply CancelClick();

	bool CanMoveWeaponPriorityUp() const;
	bool CanMoveWeaponPriorityDown() const;

	TOptional<int32> GetWeaponGroup() const;
	void SetWeaponGroup(int32 NewGroup);

	TSharedPtr<STextBlock> WeaponsInGroupText;
	void UpdateWeaponsInGroup();

	FReply DefaultGroupsClicked();

	TArray<TSharedPtr<FText>> QuickSlotTexts;
	TSharedPtr< SComboBox< TSharedPtr<FText> > > QuickSlotComboBox;
	
	TSharedRef<SWidget> GenerateQuickslotListWidget(TSharedPtr<FText> InItem);
	void OnQuickslotSelected(TSharedPtr<FText> NewSelection, ESelectInfo::Type SelectInfo);
	TSharedPtr<STextBlock> SelectedWeaponQuickslot;

	TArray<FString> WeaponWheelClassnames;

};
#endif