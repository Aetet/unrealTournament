// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "../Public/UnrealTournament.h"
#include "../Public/UTLocalPlayer.h"
#include "SUWPanel.h"
#include "SUWindowsStyle.h"

#if !UE_SERVER

void SUWPanel::Construct(const FArguments& InArgs, TWeakObjectPtr<UUTLocalPlayer> InPlayerOwner)
{
	bClosing = false;
	PlayerOwner = InPlayerOwner;
	checkSlow(PlayerOwner != NULL);

	FVector2D ViewportSize;
	GetPlayerOwner()->ViewportClient->GetViewportSize(ViewportSize);
	ConstructPanel(ViewportSize);
}

void SUWPanel::ConstructPanel(FVector2D ViewportSize){}

void SUWPanel::OnShowPanel(TSharedPtr<SUWindowsDesktop> inParentWindow)
{
	ParentWindow = inParentWindow;
}
void SUWPanel::OnHidePanel()
{
	TSharedPtr<SWidget> Panel = this->AsShared();
	bClosing = true;
	ParentWindow->PanelHidden(Panel);
	ParentWindow.Reset();
}

void SUWPanel::ConsoleCommand(FString Command)
{
	if (PlayerOwner.IsValid() && PlayerOwner->PlayerController != NULL)
	{
		PlayerOwner->Exec(PlayerOwner->GetWorld(), *Command, *GLog);
	}
}


TSharedRef<SWidget> SUWPanel::GenerateStringListWidget(TSharedPtr<FString> InItem)
{
	return SNew(SBox)
		.Padding(5)
		[
			SNew(STextBlock)
			.ColorAndOpacity(FLinearColor::Black)
			.Text(FText::FromString(*InItem.Get()))
		];
}


AUTPlayerState* SUWPanel::GetOwnerPlayerState()
{
	AUTBasePlayerController* PC = Cast<AUTBasePlayerController>(PlayerOwner->PlayerController);
	if (PC) 
	{
		return Cast<AUTPlayerState>(PC->PlayerState);
	}
	return NULL;
}


#endif