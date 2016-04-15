// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once 

/**
 *
 **/

#include "UTHUDWidget_WeaponCrosshair.generated.h"

UCLASS()
class UNREALTOURNAMENT_API UUTHUDWidget_WeaponCrosshair : public UUTHUDWidget
{
	GENERATED_UCLASS_BODY()

public:

	virtual void Draw_Implementation(float DeltaTime);

protected:
	float LastHitTime;
	float LastHitMagnitude;

	int32 FlashStage;
	float FlashTime;
	float FlashDuration;

private:
};
