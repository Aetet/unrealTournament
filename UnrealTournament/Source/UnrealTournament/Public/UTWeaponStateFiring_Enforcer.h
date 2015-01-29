// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTWeaponStateFiring.h"
#include "UTWeaponStateFiring_Enforcer.generated.h"

/** base class of states that fire the weapon and live in the weapon's FiringState array */
UCLASS(CustomConstructor)
class UUTWeaponStateFiring_Enforcer : public UUTWeaponStateFiring
{
	GENERATED_UCLASS_BODY()

	UUTWeaponStateFiring_Enforcer(const FObjectInitializer& OI)
	: Super(OI)
	{}

	virtual void BeginState(const UUTWeaponState* PrevState) override;
	virtual void EndState() override;

	/** called to fire the shot and consume ammo */
	virtual void FireShot() override;

	virtual void UpdateTiming() override;

	/** Reset the timer if the time remaining on it is greater than the new FireRate */
	virtual void ResetTiming();

	virtual void PutDown() override;

private:

	float LastFiredTime;
};