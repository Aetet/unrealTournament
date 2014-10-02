// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Slate.h"

#if !UE_SERVER

class SUWDialog : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SUWDialog)
	: _DialogSize(FVector2D(400,200))
	, _bDialogSizeIsRelative(false)
	, _DialogPosition(FVector2D(0.5f,0.5f))
	, _DialogAnchorPoint(FVector2D(0.5f,0.5f))
	, _ContentPadding(FVector2D(10.0f, 5.0f))
	, _ButtonMask(UTDIALOG_BUTTON_OK)
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

	/** needed for every widget */
	void Construct(const FArguments& InArgs);

	virtual void OnDialogOpened();
	virtual void OnDialogClosed();

	inline TWeakObjectPtr<class UUTLocalPlayer> GetPlayerOwner()
	{
		return PlayerOwner;
	}
	
	/** utility to generate a simple text widget for list and combo boxes given a string value */
	TSharedRef<SWidget> GenerateStringListWidget(TSharedPtr<FString> InItem);

protected:

	/** Holds a reference to the SCanvas widget that makes up the dialog */
	TSharedPtr<SCanvas> Canvas;

	/** Holds a reference to the SOverlay that defines the content for this dialog */
	TSharedPtr<SOverlay> DialogContent;

	/** Holds a reference to the SUniformGridPanel that is the button bar */
	TSharedPtr<SUniformGridPanel> ButtonBar;

	// The actual position of this dialog  
	FVector2D ActualPosition;

	// The actual size of this dialog
	FVector2D ActualSize;

	/** Our forward facing dialog result delegate.  OnButtonClick will make this call if it's defined. */
	FDialogResultDelegate OnDialogResult;

	/**
	 *	Called when the dialog wants to build the button bar.  It will iterate over the ButtonMask and add any buttons needed.
	 **/
	TSharedRef<class SWidget> BuildButtonBar(uint16 ButtonMask);

	/**
	 *	Adds a button to the button bar
	 **/
	void BuildButton(TSharedPtr<SUniformGridPanel> Bar, FText ButtonText, uint16 ButtonID, uint32 &ButtonCount);

	// Our internal OnClick handler.  
	virtual FReply OnButtonClick(uint16 ButtonID);	

	// The current tab index.  Used to determine when tab and shift tab should work :(
	int32 TabStop;
	// Stores a list of widgets that are tab'able
	TArray<TSharedPtr<SWidget>> TabTable;

private:
	TWeakObjectPtr<class UUTLocalPlayer> PlayerOwner;
	TSharedPtr<class SWidget> GameViewportWidget;

	// HACKS needed to keep window focus
	virtual bool SupportsKeyboardFocus() const override;
	virtual FReply OnKeyboardFocusReceived(const FGeometry& MyGeometry, const FKeyboardFocusEvent& InKeyboardFocusEvent) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent) override;
	virtual FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent) override;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

};

#endif