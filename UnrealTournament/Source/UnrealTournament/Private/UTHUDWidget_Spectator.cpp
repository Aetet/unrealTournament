// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTHUDWidget_Spectator.h"

UUTHUDWidget_Spectator::UUTHUDWidget_Spectator(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	DesignedResolution = 1080;
	Position=FVector2D(0,0);
	Size=FVector2D(1920.0f,108.0f);
	ScreenPosition=FVector2D(0.0f, 0.85f);
	Origin=FVector2D(0.0f,0.0f);

	static ConstructorHelpers::FObjectFinder<UFont> MFont(TEXT("Font'/Game/RestrictedAssets/UI/Fonts/fntScoreboard_Large.fntScoreboard_Large'"));
	MessageFont = MFont.Object;

	static ConstructorHelpers::FObjectFinder<UTexture2D> Tex(TEXT("Texture2D'/Game/RestrictedAssets/UI/Textures/UTScoreboard01.UTScoreboard01'"));
	TextureAtlas = Tex.Object;
}

bool UUTHUDWidget_Spectator::ShouldDraw_Implementation(bool bShowScores)
{
	return (UTGameState != NULL && !UTGameState->HasMatchEnded() && UTHUDOwner->UTPlayerOwner != NULL &&
				UTHUDOwner->UTPlayerOwner->UTPlayerState != NULL && ( ( UTCharacterOwner == NULL && UTPlayerOwner->GetPawn() == NULL ) || ( UTCharacterOwner != NULL && UTCharacterOwner->IsDead()) ) 
				&& (!bShowScores || !UTGameState->HasMatchStarted()));
}

void UUTHUDWidget_Spectator::Draw_Implementation(float DeltaTime)
{
	Super::Draw_Implementation(DeltaTime);

	if (TextureAtlas)
	{
		// Draw the Background
		DrawTexture(TextureAtlas, 0, 0, 1920.0, 108.0f, 4, 2, 124, 128, 1.0);

		// Draw the Logo
		DrawTexture(TextureAtlas, 20, 54, 301, 98, 162, 14, 301, 98.0, 1.0f, FLinearColor::White, FVector2D(0.0, 0.5));

		// Draw the Spacer Bar
		DrawTexture(TextureAtlas, 341, 54, 4, 99, 488, 13, 4, 99, 1.0f, FLinearColor::White, FVector2D(0.0, 0.5));

		FText SpectatorMessage;

		if (!UTGameState->HasMatchStarted())	
		{
			// Look to see if we are waiting to play and if we must be ready.  If we aren't, just exit cause we don
			AUTPlayerState* UTPS = UTHUDOwner->UTPlayerOwner->UTPlayerState;
			if (UTGameState->IsMatchInCountdown())
			{
				if (UTPS != NULL && UTPS->RespawnChoiceA && UTPS->RespawnChoiceB)
				{
					SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "Choose Start", "Choose your start position");
				}
				else
				{
					SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "MatchStarting", "Match is about to start");
				}
			}
			else if (UTPS != NULL && UTPS->bReadyToPlay)
			{
				if (UTGameState->bTeamGame)
				{
					SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "IsReadyTeam", "You are ready to play, [ALTFIRE] to change teams.");
				}
				else
				{
					SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "IsReady", "You are ready to play.");
				}
			}
			else
			{
				if (UTGameState->bTeamGame)
				{
					SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "GetReadyTeam", "Press [FIRE] when you are ready, [ALTFIRE] to change teams.");
				}
				else
				{
					SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "GetReady", "Press [FIRE] when you are ready.");
				}
			}
		}
		else if (!UTGameState->HasMatchEnded())
		{

			if (!UTGameState->IsMatchInOvertime())
			{
				if (UTHUDOwner->UTPlayerOwner->UTPlayerState->bOnlySpectator)
				{
					AActor* ViewActor = UTHUDOwner->UTPlayerOwner->GetViewTarget();
					AUTCharacter* ViewCharacter = Cast<AUTCharacter>(ViewActor);
					if (ViewCharacter && ViewCharacter->PlayerState)
					{
						FFormatNamedArguments Args;
						Args.Add("PlayerName", FText::AsCultureInvariant(ViewCharacter->PlayerState->PlayerName));
						SpectatorMessage = FText::Format(NSLOCTEXT("UUTHUDWidget_Spectator", "SpectatorPlayerWatching", "Now watching {PlayerName}"), Args);						
					}
					else
					{
						SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "SpectatorCameraChange", "Press [FIRE] to change viewpoint...");
					}
				}
				else if (UTHUDOwner->UTPlayerOwner->UTPlayerState->RespawnTime > 0.0f)
				{
					FFormatNamedArguments Args;
					uint32 WaitTime = uint32(UTHUDOwner->UTPlayerOwner->UTPlayerState->RespawnTime) + 1;
					Args.Add("RespawnTime", FText::AsNumber(WaitTime));
					SpectatorMessage = FText::Format(NSLOCTEXT("UUTHUDWidget_Spectator","RespawnWaitMessage","You can respawn in {RespawnTime}..."),Args);
				}
				else
				{
					if (UTHUDOwner->UTPlayerOwner->UTPlayerState->RespawnChoiceA != nullptr)
					{
						SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "ChooseRespawnMessage", "Choose a respawn point with [FIRE] or [ALT-FIRE]");
					}
					else
					{
						SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "RespawnMessage", "Press [FIRE] to respawn...");
					}
				}
			}
			else
			{
				if (UTGameState->bOnlyTheStrongSurvive)
				{
					SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "SpectatorCameraChange", "Press [FIRE] to change viewpoint...");
				}
				else if (UTHUDOwner->UTPlayerOwner->UTPlayerState->RespawnTime > 0.0f)
				{
					FFormatNamedArguments Args;
					uint32 WaitTime = uint32(UTHUDOwner->UTPlayerOwner->UTPlayerState->RespawnTime) + 1;
					Args.Add("RespawnTime", FText::AsNumber(WaitTime));
					SpectatorMessage = FText::Format(NSLOCTEXT("UUTHUDWidget_Spectator","RespawnWaitMessage","You can respawn in {RespawnTime}..."),Args);
				}
				else
				{
					if (UTHUDOwner->UTPlayerOwner->UTPlayerState->RespawnChoiceA != nullptr)
					{
						SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "ChooseRespawnMessage", "Choose a respawn point with [FIRE] or [ALT-FIRE]");
					}
					else
					{
						SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "RespawnMessage", "Press [FIRE] to respawn...");
					}
				}
			}
		}
		DrawText(SpectatorMessage, 360, 50, MessageFont, 1.0, 1.0, FLinearColor::White, ETextHorzPos::Left, ETextVertPos::Center);
	}
}

