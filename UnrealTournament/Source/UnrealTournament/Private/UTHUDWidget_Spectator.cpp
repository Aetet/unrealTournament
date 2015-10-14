// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTHUDWidget_Spectator.h"
#include "UTCarriedObject.h"


UUTHUDWidget_Spectator::UUTHUDWidget_Spectator(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	DesignedResolution = 1080;
	Position=FVector2D(0,0);
	Size=FVector2D(1920.0f,108.0f);
	ScreenPosition=FVector2D(0.0f, 0.85f);
	Origin=FVector2D(0.0f,0.0f);

	static ConstructorHelpers::FObjectFinder<UTexture2D> Tex(TEXT("Texture2D'/Game/RestrictedAssets/UI/Textures/UTScoreboard01.UTScoreboard01'"));
	TextureAtlas = Tex.Object;
}

bool UUTHUDWidget_Spectator::ShouldDraw_Implementation(bool bShowScores)
{
	if (!bShowScores && UTHUDOwner && UTHUDOwner->UTPlayerOwner && UTHUDOwner->UTPlayerOwner->UTPlayerState && UTGameState)
	{
		if (UTGameState->IsMatchAtHalftime() || UTGameState->HasMatchEnded() || !UTGameState->HasMatchStarted())
		{
			return true;
		}
		return (UTHUDOwner->UTPlayerOwner->UTPlayerState->bOnlySpectator || (UTCharacterOwner ? UTCharacterOwner->IsDead() : (UTHUDOwner->UTPlayerOwner->GetPawn() == NULL)));
	}
	return false;
}

void UUTHUDWidget_Spectator::DrawSimpleMessage(FText SimpleMessage, float DeltaTime, bool bViewingMessage)
{
	if (SimpleMessage.IsEmpty() || (TextureAtlas == NULL))
	{
		return;
	}
	float Scaling = bViewingMessage ? FMath::Max(1.f, 3.f - 6.f*(GetWorld()->GetTimeSeconds() - ViewCharChangeTime)) : 1.f;
	float ScreenWidth = (Canvas->ClipX / RenderScale);
	float BackgroundWidth = ScreenWidth;
	float TextPosition = 360.f;
	float MessageOffset = 0.f;
	float YOffset = 0.f;
	if (bViewingMessage && UTHUDOwner->LargeFont)
	{
		float YL = 0.0f;
		Canvas->StrLen(UTHUDOwner->LargeFont, SimpleMessage.ToString(), BackgroundWidth, YL);
		BackgroundWidth = Scaling* (FMath::Max(BackgroundWidth, 128.f) + 64.f);
		MessageOffset = (ScreenWidth - BackgroundWidth) * (UTGameState->HasMatchEnded() ? 0.5f : 1.f);
		TextPosition = 32.f + MessageOffset;
		YOffset = -32.f;
	}

	// Draw the Background
	bMaintainAspectRatio = false;
	DrawTexture(TextureAtlas, MessageOffset, YOffset, BackgroundWidth, Scaling * 108.0f, 4, 2, 124, 128, 1.0);
	if (bViewingMessage)
	{
		DrawText(FText::FromString("Now Viewing"), TextPosition, YOffset + 14.f, UTHUDOwner->SmallFont, Scaling, 1.f, GetMessageColor(), ETextHorzPos::Left, ETextVertPos::Center);
	}
	else
	{
		bMaintainAspectRatio = true;

		// Draw the Logo
		DrawTexture(TextureAtlas, 20, 54, 301, 98, 162, 14, 301, 98.0, 1.0f, FLinearColor::White, FVector2D(0.0, 0.5));

		// Draw the Spacer Bar
		DrawTexture(TextureAtlas, 341, 54, 4, 99, 488, 13, 4, 99, 1.0f, FLinearColor::White, FVector2D(0.0, 0.5));
	}
	DrawText(SimpleMessage, TextPosition, YOffset + 50.f, UTHUDOwner->LargeFont, Scaling, 1.f, GetMessageColor(), ETextHorzPos::Left, ETextVertPos::Center);
}

void UUTHUDWidget_Spectator::Draw_Implementation(float DeltaTime)
{
	Super::Draw_Implementation(DeltaTime);

	bool bViewingMessage = false;
	FText SpectatorMessage = GetSpectatorMessageText(bViewingMessage);
	DrawSimpleMessage(SpectatorMessage, DeltaTime, bViewingMessage);
}

FText UUTHUDWidget_Spectator::GetSpectatorMessageText(bool &bViewingMessage)
{
	FText SpectatorMessage;
	if (UTGameState)
	{
		AUTPlayerState* UTPS = UTHUDOwner->UTPlayerOwner->UTPlayerState;
		if (!UTGameState->HasMatchStarted())
		{
			// Look to see if we are waiting to play and if we must be ready.  If we aren't, just exit cause we don
			if (UTGameState->IsMatchInCountdown())
			{
				if (UTPS && UTPS->RespawnChoiceA && UTPS->RespawnChoiceB)
				{
					SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "Choose Start", "Choose your start position");
				}
				else if (UTGameState->bForcedBalance)
				{
					SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "BalanceTeams", "Balancing teams - match is about to start.");
				}
				else
				{
					SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "MatchStarting", "Match is about to start");
				}
			}
			else if (UTGameState->PlayersNeeded > 0)
			{
				SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "WaitingForPlayers", "Waiting for players to join.");
			}
			else if (UTPS && UTPS->bReadyToPlay)
			{
				SpectatorMessage = (UTGameState->bTeamGame && UTGameState->bAllowTeamSwitches)
					? NSLOCTEXT("UUTHUDWidget_Spectator", "IsReadyTeam", "You are ready, press [ALTFIRE] to change teams.")
					: NSLOCTEXT("UUTHUDWidget_Spectator", "IsReady", "You are ready to play.");
			}
			else if (UTPS && UTPS->bCaster)
			{
				SpectatorMessage = (UTGameState->AreAllPlayersReady())
					? NSLOCTEXT("UUTHUDWidget_Spectator", "WaitingForCaster", "All players are ready. Press [Enter] to start match.")
					: NSLOCTEXT("UUTHUDWidget_Spectator", "WaitingForReady", "Waiting for players to ready up.");
			}
			else if (UTPS && UTPS->bOnlySpectator)
			{
				SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "WaitingForReady", "Waiting for players to ready up.");
			}
			else if (UTHUDOwner->GetScoreboard() && UTHUDOwner->GetScoreboard()->IsInteractive())
			{
				SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "CloseMenu", "Press [ESC] to close menu.");
			}
			else
			{
				SpectatorMessage = (UTGameState->bTeamGame && UTGameState->bAllowTeamSwitches)
					? NSLOCTEXT("UUTHUDWidget_Spectator", "GetReadyTeam", "Press [FIRE] to ready up, [ALTFIRE] to change teams.")
					: NSLOCTEXT("UUTHUDWidget_Spectator", "GetReady", "Press [FIRE] when you are ready.");
			}
		}
		else if (!UTGameState->HasMatchEnded())
		{
			if (UTGameState->IsMatchAtHalftime())
			{
				if (UTGameState->bCasterControl && UTGameState->bStopGameClock == true && UTPS != nullptr)
				{
					SpectatorMessage = (UTPS->bCaster)
						? NSLOCTEXT("UUTHUDWidget_Spectator", "WaitingCasterHalfTime", "Press [Enter] to start next half.")
						: NSLOCTEXT("UUTHUDWidget_Spectator", "WaitingForCasterHalfTime", "Waiting for caster to start next half.");
				}
				else
				{
					FFormatNamedArguments Args;
					Args.Add("Time", FText::AsNumber(UTGameState->RemainingTime));
					SpectatorMessage = FText::Format(NSLOCTEXT("UUTHUDWidget_Spectator", "HalfTime", "HALFTIME - Game resumes in {Time}"), Args);
				}
			}
			else if (UTPS && UTPS->bOnlySpectator)
			{
				AActor* ViewActor = UTHUDOwner->UTPlayerOwner->GetViewTarget();
				AUTCharacter* ViewCharacter = Cast<AUTCharacter>(ViewActor);
				if (!ViewCharacter)
				{
					AUTCarriedObject* Flag = Cast<AUTCarriedObject>(ViewActor);
					if (Flag && Flag->Holder)
					{
						ViewCharacter = Cast<AUTCharacter>(Flag->AttachmentReplication.AttachParent);
					}
				}
				if (ViewCharacter && ViewCharacter->PlayerState)
				{
					if (LastViewedPS != ViewCharacter->PlayerState)
					{
						ViewCharChangeTime = ViewCharacter->GetWorld()->GetTimeSeconds();
						LastViewedPS = Cast<AUTPlayerState>(ViewCharacter->PlayerState);
					}
					FFormatNamedArguments Args;
					Args.Add("PlayerName", FText::AsCultureInvariant(ViewCharacter->PlayerState->PlayerName));
					bViewingMessage = true;
					SpectatorMessage = FText::Format(NSLOCTEXT("UUTHUDWidget_Spectator", "SpectatorPlayerWatching", "{PlayerName}"), Args);
				}
				else if (!UTHUDOwner->UTPlayerOwner->bHasUsedSpectatingBind)
				{
					SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "SpectatorCameraChange", "Press [ENTER] to view camera binds.");
				}
				else
				{
					LastViewedPS = NULL;
				}
			}
			else if (UTPS && (UTCharacterOwner ? UTCharacterOwner->IsDead() : (UTHUDOwner->UTPlayerOwner->GetPawn() == NULL)))
			{
				if (UTPS->RespawnTime > 0.0f)
				{
					FFormatNamedArguments Args;
					static const FNumberFormattingOptions RespawnTimeFormat = FNumberFormattingOptions()
						.SetMinimumFractionalDigits(0)
						.SetMaximumFractionalDigits(0);
					Args.Add("RespawnTime", FText::AsNumber(UTHUDOwner->UTPlayerOwner->UTPlayerState->RespawnTime + 1, &RespawnTimeFormat));
					SpectatorMessage = FText::Format(NSLOCTEXT("UUTHUDWidget_Spectator", "RespawnWaitMessage", "You can respawn in {RespawnTime}..."), Args);
				}
				else
				{
					SpectatorMessage = (UTPS->RespawnChoiceA != nullptr)
						? NSLOCTEXT("UUTHUDWidget_Spectator", "ChooseRespawnMessage", "Choose a respawn point with [FIRE] or [ALT-FIRE]")
						: NSLOCTEXT("UUTHUDWidget_Spectator", "RespawnMessage", "Press [FIRE] to respawn...");
				}
			}
		}
		else
		{
			AActor* ViewActor = UTHUDOwner->UTPlayerOwner->GetViewTarget();
			AUTCharacter* ViewCharacter = Cast<AUTCharacter>(ViewActor);
			if (ViewCharacter && ViewCharacter->PlayerState)
			{
				FFormatNamedArguments Args;
				Args.Add("PlayerName", FText::AsCultureInvariant(ViewCharacter->PlayerState->PlayerName));
				bViewingMessage = true;
				AUTPlayerState* PS = Cast<AUTPlayerState>(ViewCharacter->PlayerState);
				if (UTGameState->bTeamGame && PS && PS->Team && (!UTGameState->GameModeClass || !UTGameState->GameModeClass->GetDefaultObject<AUTTeamGameMode>() || UTGameState->GameModeClass->GetDefaultObject<AUTTeamGameMode>()->bAnnounceTeam))
				{
					SpectatorMessage = (PS->Team->TeamIndex == 0)
						? FText::Format(NSLOCTEXT("UUTHUDWidget_Spectator", "SpectatorPlayerWatching", "Red Team Led by {PlayerName}"), Args)
						: FText::Format(NSLOCTEXT("UUTHUDWidget_Spectator", "SpectatorPlayerWatching", "Blue Team Led by {PlayerName}"), Args);
				}
				else
				{
					SpectatorMessage = FText::Format(NSLOCTEXT("UUTHUDWidget_Spectator", "SpectatorPlayerWatching", "{PlayerName}"), Args);
				}
			}
		}
	}
	return SpectatorMessage;
}

float UUTHUDWidget_Spectator::GetDrawScaleOverride()
{
	return 1.0;
}