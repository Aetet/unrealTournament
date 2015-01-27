// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTCTFScoreboard.h"
#include "UTTeamScoreboard.h"
#include "UTCTFGameState.h"

UUTCTFScoreboard::UUTCTFScoreboard(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	ScoringPlaysHeader = NSLOCTEXT("CTF", "ScoringPlaysHeader", "SCORING SUMMARY");
	AssistedByText = NSLOCTEXT("CTF", "AssistedBy", "Assists:");
	UnassistedText = NSLOCTEXT("CTF", "Unassisted", "Unassisted");
	CaptureText = NSLOCTEXT("CTF", "Capture", "Capture");
	ScoreText = NSLOCTEXT("CTF", "Score", "Score");
	NoScoringText = NSLOCTEXT("CTF", "NoScoring", "No Scoring");

	ColumnHeaderScoreX = 343;
	ColumnHeaderCapsX = 418;
	ColumnHeaderAssistsX = 457;
	ColumnHeaderReturnsX = 494;
	NumPages = 2;
}

void UUTCTFScoreboard::OpenScoringPlaysPage()
{
	SetPage(1);
}
void UUTCTFScoreboard::PageChanged_Implementation()
{
	GetWorld()->GetTimerManager().ClearTimer(this, &UUTCTFScoreboard::OpenScoringPlaysPage);
}

void UUTCTFScoreboard::Draw_Implementation(float DeltaTime)
{
	if (UTHUDOwner->ScoreboardPage == 1)
	{
		DrawScoringPlays();
	}
	else
	{
		Super::Draw_Implementation(DeltaTime);
	}
}

void UUTCTFScoreboard::DrawGameOptions(float RenderDelta, float& YOffset)
{
	if (UTGameState)
	{
		FText StatusText = UTGameState->GetGameStatusText();
		if (!StatusText.IsEmpty())
		{
			DrawText(StatusText, 1255, 38, MediumFont, 1.0, 1.0, FLinearColor::Yellow, ETextHorzPos::Right, ETextVertPos::Center);
		} 
		DrawText(UTHUDOwner->ConvertTime(FText::GetEmpty(), FText::GetEmpty(), UTGameState->RemainingTime, true, true, true), 1255, 88, ClockFont, 1.0, 1.0, FLinearColor::White, ETextHorzPos::Right, ETextVertPos::Center);
	}
}

void UUTCTFScoreboard::DrawScoreHeaders(float RenderDelta, float& YOffset)
{
	float XOffset = 0.0;
	float Width = 625;  // 20 pixels between them
	float Height = 23;

	FText CH_PlayerName = NSLOCTEXT("UTScoreboard", "ColumnHeader_PlayerName", "PLAYER");
	FText CH_Score = NSLOCTEXT("UTScoreboard", "ColumnHeader_PlayerScore", "SCORE");
	FText CH_Caps = NSLOCTEXT("UTCTFScoreboard", "ColumnHeader_PlayerCaps", "C");
	FText CH_Assists = NSLOCTEXT("UTCTFScoreboard", "ColumnHeader_PlayerAssists", "A");
	FText CH_Returns = NSLOCTEXT("UTCTFScoreboard", "ColumnHeader_PlayerReturns", "R");
	FText CH_Ping = NSLOCTEXT("UTScoreboard", "ColumnHeader_PlayerPing", "PING");
	FText CH_Ready = NSLOCTEXT("UTScoreboard", "ColumnHeader_Ready", "");

	for (int32 i = 0; i < 2; i++)
	{
		// Draw the background Border
		DrawTexture(TextureAtlas, XOffset, YOffset, Width, Height, 149, 138, 32, 32, 1.0, FLinearColor(0.72f, 0.72f, 0.72f, 0.85f));

		DrawText(CH_PlayerName, XOffset + ColumnHeaderPlayerX, YOffset + ColumnHeaderY, TinyFont, 1.0f, 1.0f, FLinearColor::Black, ETextHorzPos::Left, ETextVertPos::Center);
		if (UTGameState && UTGameState->HasMatchStarted())
		{
			DrawText(CH_Score, XOffset + ColumnHeaderScoreX, YOffset + ColumnHeaderY, TinyFont, 1.0f, 1.0f, FLinearColor::Black, ETextHorzPos::Center, ETextVertPos::Center);
			DrawText(CH_Caps, XOffset + ColumnHeaderCapsX, YOffset + ColumnHeaderY, TinyFont, 1.0f, 1.0f, FLinearColor::Black, ETextHorzPos::Center, ETextVertPos::Center);
			DrawText(CH_Assists, XOffset + ColumnHeaderAssistsX, YOffset + ColumnHeaderY, TinyFont, 1.0f, 1.0f, FLinearColor::Black, ETextHorzPos::Center, ETextVertPos::Center);
			DrawText(CH_Returns, XOffset + ColumnHeaderReturnsX, YOffset + ColumnHeaderY, TinyFont, 1.0f, 1.0f, FLinearColor::Black, ETextHorzPos::Center, ETextVertPos::Center);
		}
		else
		{
			DrawText(CH_Ready, XOffset + ColumnHeaderScoreX, YOffset + ColumnHeaderY, TinyFont, 1.0f, 1.0f, FLinearColor::Black, ETextHorzPos::Center, ETextVertPos::Center);
		}

		DrawText(CH_Ping, XOffset + ColumnHeaderPingX, YOffset + ColumnHeaderY, TinyFont, 1.0f, 1.0f, FLinearColor::Black, ETextHorzPos::Center, ETextVertPos::Center);

		XOffset += Width + 20;
	}

	YOffset += Height + 4;
}

void UUTCTFScoreboard::DrawPlayer(int32 Index, AUTPlayerState* PlayerState, float RenderDelta, float XOffset, float YOffset)
{
	if (PlayerState == NULL) return;	// Safeguard

	FLinearColor DrawColor = FLinearColor::White;
	float BarOpacity = 0.3;

	FText PlayerName = FText::FromString(PlayerState->PlayerName);
	FText PlayerScore = FText::AsNumber(int32(PlayerState->Score));
	FText PlayerCaps = FText::AsNumber(PlayerState->FlagCaptures);
	FText PlayerAssists = FText::AsNumber(PlayerState->Assists);
	FText PlayerReturns = FText::AsNumber(PlayerState->FlagReturns);

	int32 Ping = PlayerState->Ping * 4;
	if (UTHUDOwner->UTPlayerOwner->UTPlayerState == PlayerState)
	{
		Ping = PlayerState->ExactPing;
		DrawColor = FLinearColor(0.0f, 0.92f, 1.0f, 1.0f);
		BarOpacity = 0.5;
	}

	FText PlayerPing = FText::Format(NSLOCTEXT("UTScoreboard", "PingFormatText", "{0}ms"), FText::AsNumber(Ping));

	// Draw the position

	// Draw the background border.
	DrawTexture(TextureAtlas, XOffset, YOffset, 625, 36, 149, 138, 32, 32, BarOpacity, FLinearColor::Black);	// NOTE: Once I make these interactable.. have a selection color too
	DrawTexture(TextureAtlas, XOffset + 5, YOffset + 18, 32, 24, 193, 138, 32, 24, 1.0, FLinearColor::White, FVector2D(0.0f, 0.5f));	// Add a function to support additional flags

	// Draw the Text
	DrawText(PlayerName, XOffset + 42, YOffset + ColumnY, MediumFont, 1.0f, 1.0f, DrawColor, ETextHorzPos::Left, ETextVertPos::Center);

	if (UTGameState && UTGameState->HasMatchStarted())
	{
		DrawText(PlayerScore, XOffset + ColumnHeaderScoreX, YOffset + ColumnY, MediumFont, 1.0f, 1.0f, DrawColor, ETextHorzPos::Center, ETextVertPos::Center);
		DrawText(PlayerCaps, XOffset + ColumnHeaderCapsX, YOffset + ColumnY, SmallFont, 1.0f, 1.0f, DrawColor, ETextHorzPos::Center, ETextVertPos::Center);
		DrawText(PlayerAssists, XOffset + ColumnHeaderAssistsX, YOffset + ColumnY, SmallFont, 1.0f, 1.0f, DrawColor, ETextHorzPos::Center, ETextVertPos::Center);
		DrawText(PlayerReturns, XOffset + ColumnHeaderReturnsX, YOffset + ColumnY, SmallFont, 1.0f, 1.0f, DrawColor, ETextHorzPos::Center, ETextVertPos::Center);
	}
	else
	{
		FText PlayerReady = PlayerState->bReadyToPlay ? NSLOCTEXT("UTScoreboard", "READY", "READY") : NSLOCTEXT("UTScoreboard", "NOTREADY", "");
		if (PlayerState->bPendingTeamSwitch)
		{
			PlayerReady = NSLOCTEXT("UTScoreboard", "TEAMSWITCH", "TEAM SWAP");
		}
		DrawText(PlayerReady, XOffset + ColumnHeaderScoreX, YOffset + ColumnY, MediumFont, 1.0f, 1.0f, DrawColor, ETextHorzPos::Center, ETextVertPos::Center);
	}

	DrawText(PlayerPing, XOffset + ColumnHeaderPingX, YOffset + ColumnY, SmallFont, 1.0f, 1.0f, DrawColor, ETextHorzPos::Center, ETextVertPos::Center);
}

void UUTCTFScoreboard::DrawScoringPlays()
{
	AUTCTFGameState* CTFState = Cast<AUTCTFGameState>(UTGameState);
	uint8 CurrentPeriod = 0;
	if (CTFState->HasMatchEnded())
	{
		// show scores for last played period
		for (const FCTFScoringPlay& Play : CTFState->GetScoringPlays())
		{
			CurrentPeriod = FMath::Max<uint8>(CurrentPeriod, Play.Period);
		}
	}
	// TODO: currently there's no intermission between second half and OT
	else if (CTFState->IsMatchAtHalftime())
	{
		CurrentPeriod = 0;
	}
	Canvas->SetLinearDrawColor(FLinearColor::White);
	float YPos = Canvas->ClipY * 0.1f;
	const float XOffset = Canvas->ClipX * ((Canvas->ClipX / Canvas->ClipY > 1.5f) ? 0.2f : 0.1f);
	FFontRenderInfo TextRenderInfo;
	TextRenderInfo.bEnableShadow = true;
	TextRenderInfo.bClipText = true;
	{
		float XL, YL;
		Canvas->TextSize(UTHUDOwner->LargeFont, ScoringPlaysHeader.ToString(), XL, YL, RenderScale, RenderScale);
		Canvas->DrawText(UTHUDOwner->LargeFont, ScoringPlaysHeader, (Canvas->ClipX - XL) * 0.5, YPos, RenderScale, RenderScale, TextRenderInfo);
		YPos += YL * 1.2f;
	}
	TArray<int32> ScoresSoFar;
	ScoresSoFar.SetNumZeroed(CTFState->Teams.Num());
	TMap<FSafePlayerName, int32> CapCount;
	bool bDrewSomething = false;
	for (const FCTFScoringPlay& Play : CTFState->GetScoringPlays())
	{
		if (Play.Team != NULL) // should always be true...
		{
			ScoresSoFar.SetNumZeroed(FMath::Max<int32>(ScoresSoFar.Num(), int32(Play.Team->TeamIndex) + 1));
			ScoresSoFar[Play.Team->TeamIndex]++;
			int32* PlayerCaps = CapCount.Find(Play.ScoredBy);
			if (PlayerCaps != NULL)
			{
				(*PlayerCaps)++;
			}
			else
			{
				CapCount.Add(Play.ScoredBy, 1);
			}
			if (Play.Period >= CurrentPeriod)
			{
				float XL, YL;
				if (!bDrewSomething)
				{
					// draw headers
					Canvas->DrawText(UTHUDOwner->MediumFont, CaptureText, XOffset, YPos, RenderScale, RenderScale, TextRenderInfo);
					Canvas->TextSize(UTHUDOwner->MediumFont, ScoreText.ToString(), XL, YL, RenderScale, RenderScale);
					Canvas->DrawText(UTHUDOwner->MediumFont, ScoreText, Canvas->ClipX * 1.0f - XOffset - XL, YPos, RenderScale, RenderScale, TextRenderInfo);
					YPos += YL * 1.1f;
				}
				bDrewSomething = true;
				// draw this cap
				Canvas->SetLinearDrawColor(Play.Team->TeamColor);
				// scored by
				FString ScoredByLine = Play.ScoredBy.GetPlayerName();
				if (PlayerCaps != NULL)
				{
					ScoredByLine += FString::Printf(TEXT(" (%i)"), *PlayerCaps);
				}
				float PrevYPos = YPos;
				Canvas->TextSize(UTHUDOwner->MediumFont, ScoredByLine, XL, YL, RenderScale, RenderScale);
				Canvas->DrawText(UTHUDOwner->MediumFont, ScoredByLine, XOffset, YPos, RenderScale, RenderScale, TextRenderInfo);
				YPos += YL;
				// assists
				FString AssistLine;
				if (Play.Assists.Num() > 0)
				{
					AssistLine = AssistedByText.ToString() + TEXT(" ");
					for (const FSafePlayerName& Assist : Play.Assists)
					{
						AssistLine += Assist.GetPlayerName() + TEXT(", ");
					}
					AssistLine = AssistLine.LeftChop(2);
				}
				else
				{
					AssistLine = UnassistedText.ToString();
				}
				Canvas->TextSize(UTHUDOwner->MediumFont, AssistLine, XL, YL, RenderScale * 0.6f, RenderScale * 0.6f);
				Canvas->DrawText(UTHUDOwner->MediumFont, AssistLine, XOffset, YPos, RenderScale * 0.6f, RenderScale * 0.6f, TextRenderInfo);
				YPos += YL;
				// time of game
				FString TimeStampLine = UTHUDOwner->ConvertTime(FText::GetEmpty(), FText::GetEmpty(), Play.ElapsedTime, false, true, false).ToString();
				Canvas->TextSize(UTHUDOwner->MediumFont, TimeStampLine, XL, YL, RenderScale * 0.6f, RenderScale * 0.6f);
				Canvas->DrawText(UTHUDOwner->MediumFont, TimeStampLine, XOffset, YPos, RenderScale * 0.6f, RenderScale * 0.6f, TextRenderInfo);
				YPos += YL;
				// team score after this cap
				Canvas->SetLinearDrawColor(FLinearColor::White);
				FString CurrentScoreLine;
				for (int32 Score : ScoresSoFar)
				{
					CurrentScoreLine += FString::Printf(TEXT("%i      "), Score);
				}
				Canvas->TextSize(UTHUDOwner->MediumFont, CurrentScoreLine, XL, YL, RenderScale, RenderScale);
				{
					float XPos = Canvas->ClipX - XOffset - XL;
					for (int32 i = 0; i < ScoresSoFar.Num(); i++)
					{
						Canvas->SetLinearDrawColor(CTFState->Teams[i]->TeamColor);
						FString SingleScorePart = FString::Printf(TEXT("      %i"), ScoresSoFar[i]);
						float SingleXL, SingleYL;
						Canvas->TextSize(UTHUDOwner->MediumFont, SingleScorePart, SingleXL, SingleYL, RenderScale, RenderScale);
						Canvas->DrawText(UTHUDOwner->MediumFont, SingleScorePart, XPos, PrevYPos + (YPos - PrevYPos - YL) * 0.5, RenderScale, RenderScale, TextRenderInfo);
						XPos += SingleXL;
					}
				}
				Canvas->SetLinearDrawColor(FLinearColor::White);

				YPos += YL * 0.25f;
				if (YPos >= Canvas->ClipY)
				{
					// TODO: pagination
					break;
				}
			}
		}
	}
	if (!bDrewSomething)
	{
		float XL, YL;
		Canvas->TextSize(UTHUDOwner->MediumFont, NoScoringText.ToString(), XL, YL, RenderScale, RenderScale);
		Canvas->DrawText(UTHUDOwner->MediumFont, NoScoringText, Canvas->ClipX * 0.5f - XL * 0.5f, YPos, RenderScale, RenderScale, TextRenderInfo);
	}
}
