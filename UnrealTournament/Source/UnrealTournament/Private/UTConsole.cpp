// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTConsole.h"

static FName NAME_Open = FName(TEXT("Open"));
static FName NAME_Typing = FName(TEXT("Typing"));
static const uint32 MAX_AUTOCOMPLETION_LINES = 20;

UUTConsole::UUTConsole(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}


void UUTConsole::FakeGotoState(FName NextStateName)
{
#if !UE_SERVER

	if (NextStateName == NAME_Open)
	{
		UWorld *World = GetOuterUGameViewportClient()->GetWorld();			
		UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(GEngine->GetFirstGamePlayer(World));
		if (LP)
		{
			bReopenMenus = LP->GetCurrentMenu().IsValid();
			LP->HideMenu();
		}
	
	}
	else if( NextStateName == NAME_None )
	{
		if (bReopenMenus)
		{
			UWorld *World = GetOuterUGameViewportClient()->GetWorld();			
			UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(GEngine->GetFirstGamePlayer(World));
			if (LP)
			{
				LP->ShowMenu();
			}
		}
		bReopenMenus = false;
	}
#endif
	Super::FakeGotoState(NextStateName);
}
