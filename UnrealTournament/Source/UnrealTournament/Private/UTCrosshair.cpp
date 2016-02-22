// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTCrosshair.h"

UUTCrosshair::UUTCrosshair(const class FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer)
{
	CrosshairName = NSLOCTEXT("UUTCrosshair", "NoName", "No Name");
	OffsetAdjust = FVector2D(0.f, 0.f);
}

void UUTCrosshair::DrawCrosshair_Implementation(UCanvas* Canvas, AUTWeapon* Weapon, float DeltaTime, float Scale, FLinearColor Color)
{
	float X = (Canvas->SizeX * 0.5f) - (CrosshairIcon.UL * Scale * 0.5f) - 1.f + OffsetAdjust.X;
	float Y = (Canvas->SizeY * 0.5f) - (CrosshairIcon.VL * Scale * 0.5f) - 1.f + OffsetAdjust.Y;

	Canvas->DrawColor = Color.ToFColor(false);
	Canvas->DrawIcon(CrosshairIcon, X, Y, Scale);
}

void UUTCrosshair::DrawPreviewCrosshair_Implementation(UCanvas* Canvas, AUTWeapon* Weapon, float DeltaTime, float Scale, FLinearColor Color)
{
	//Make sure we round since slate cursor preview could be have an odd size causing aliasing 
	float X = FMath::RoundToFloat((Canvas->SizeX * 0.5f) - (CrosshairIcon.UL * Scale * 0.5f));
	float Y = FMath::RoundToFloat((Canvas->SizeY * 0.5f) - (CrosshairIcon.VL * Scale * 0.5f));

	Canvas->DrawColor = Color.ToFColor(false);
	Canvas->DrawIcon(CrosshairIcon, X, Y, Scale);
}
