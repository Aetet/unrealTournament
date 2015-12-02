// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateBasics.h"
#include "SUWPanel.h"
#include "Slate/SlateGameResources.h"
#include "Widgets/SUTComboButton.h"

#if !UE_SERVER
class UNREALTOURNAMENT_API SUWindowsDesktop : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUWindowsDesktop)
	{}

	SLATE_ARGUMENT(TWeakObjectPtr<UUTLocalPlayer>, PlayerOwner)

	SLATE_END_ARGS()

	/** needed for every widget */
	void Construct(const FArguments& InArgs);

	/**
	 *	Will be called from the Local Player when the menu has been opened
	 **/
	virtual void OnMenuOpened(const FString& Parameters);
	virtual void OnMenuClosed();
	virtual void CloseMenus();

	virtual FReply OnMenuConsoleCommand(FString Command);
	virtual void ConsoleCommand(FString Command);

	virtual void ActivatePanel(TSharedPtr<class SUWPanel> PanelToActivate);
	virtual void DeactivatePanel(TSharedPtr<class SUWPanel> PanelToDeactivate);

	// Called from the child when the panel decides it's hidden.  Useful for out animations.
	virtual void PanelHidden(TSharedPtr<SWidget> Child);

	virtual void ShowHomePanel();

protected:

	int32 ZOrderIndex;

	TWeakObjectPtr<class UUTLocalPlayer> PlayerOwner;

	TSharedPtr<class SOverlay> Desktop;
	TSharedPtr<class SUWPanel> ActivePanel;
	TSharedPtr<class SWidget> GameViewportWidget;

	virtual void CreateDesktop();

	virtual bool SupportsKeyboardFocus() const override;
	virtual FReply OnFocusReceived( const FGeometry& MyGeometry, const FFocusEvent& InKeyboardFocusEvent ) override;
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyboardEvent ) override;
	virtual FReply OnKeyUp( const FGeometry& MyGeometry, const FKeyEvent& InKeyboardEvent ) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;

	virtual void OnOwnerLoginStatusChanged(UUTLocalPlayer* LocalPlayerOwner, ELoginStatus::Type NewStatus, const FUniqueNetId& UniqueID);

private:
	FDelegateHandle PlayerOnlineStatusChangedDelegate;
	virtual void OwnerLoginStatusChanged(UUTLocalPlayer* LocalPlayerOwner, ELoginStatus::Type NewStatus, const FUniqueNetId& UniqueID);
	

};
#endif