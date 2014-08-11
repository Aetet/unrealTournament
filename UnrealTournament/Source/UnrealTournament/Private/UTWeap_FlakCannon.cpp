// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTWeap_FlakCannon.h"
#include "UTProj_FlakShard.h"
#include "UTProj_FlakShell.h"
#include "UTProj_FlakShardMain.h"

AUTWeap_FlakCannon::AUTWeap_FlakCannon(const FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{ 
	// UI
	Group = 7;
	IconCoordinates = FTextureUVs(131.000000, 429.000000, 132.000000, 52.000000);

	BringUpTime = 0.68f;

	// Firing
	ProjClass.SetNumZeroed(2);
	FireInterval.SetNum(2);
	FireInterval[0] = 1.0;
	FireInterval[1] = 1.0;

	AmmoCost.SetNum(2);
	AmmoCost[0] = 1;
	AmmoCost[1] = 1;

	Ammo = 10;
	MaxAmmo = 30;

	FireOffset = FVector(75.f, 18.f, -15.f);

	// MultiShot

	MultiShotLocationSpread.SetNum(1);
	MultiShotLocationSpread[0] = FVector(0, 3, 3);

	MultiShotRotationSpread.SetNum(1);
	MultiShotRotationSpread[0] = 1.5f;

	MultiShotAngle.SetNum(1);
	MultiShotAngle[0] = FRotator(0, 3, 0);

	MultiShotCount.SetNum(1);
	MultiShotCount[0] = 9;

	MultiShotProjClass.SetNumZeroed(1);
}

FVector AUTWeap_FlakCannon::GetFireLocationForMultiShot_Implementation(int32 MultiShotIndex, const FVector& FireLocation, const FRotator& FireRotation)
{
	if (MultiShotIndex > 0 && MultiShotLocationSpread.IsValidIndex(CurrentFireMode))
	{
		// Randomize each projectile's spawn location if needed.
		return FireLocation + FireRotation.RotateVector(FMath::VRand() * MultiShotLocationSpread[CurrentFireMode]);
	}

	// Main projectile fires straight from muzzle center
	return FireLocation;
}

FRotator AUTWeap_FlakCannon::GetFireRotationForMultiShot_Implementation(int32 MultiShotIndex, const FVector& FireLocation, const FRotator& FireRotation)
{
	if (MultiShotIndex > 0 && MultiShotAngle.IsValidIndex(CurrentFireMode))
	{
		// Each additional projectile can have own fragment of firing cone.
		// This way there are no empty spots in firing cone due to randomness.
		// While still randomish, the pattern is predictable, which is good for pro gaming.

		// Get direction at fragment of firing cone
		const float Alpha = (float)(MultiShotIndex - 1) / (float)(MultiShotCount[CurrentFireMode] - 1);
		const FRotator ConeSector = FRotator(0, 0, 360.f * Alpha);
		FVector FireDirection = ConeSector.RotateVector(MultiShotAngle[CurrentFireMode].Vector());

		// Randomize each projectile's spawn rotation if needed 
		if (MultiShotRotationSpread.IsValidIndex(CurrentFireMode))
		{
			FireDirection = FMath::VRandCone(FireDirection, FMath::DegreesToRadians(MultiShotRotationSpread[CurrentFireMode]));
		}

		// Return firing cone rotated by player's firing rotation
		return FireRotation.RotateVector(FireDirection).Rotation();
	}

	// Main projectile fires straight at crosshair
	return FireRotation;
}

AUTProjectile* AUTWeap_FlakCannon::FireProjectile()
{
	if (GetUTOwner() == NULL || Role < ROLE_Authority || !MultiShotCount.IsValidIndex(CurrentFireMode) || MultiShotCount[CurrentFireMode] <= 1)
	{
		return Super::FireProjectile();
	}
	else
	{
		// try and fire a projectile
		checkSlow(ProjClass.IsValidIndex(CurrentFireMode) && ProjClass[CurrentFireMode] != NULL);

		// increment 3rd person muzzle flash count
		UTOwner->IncrementFlashCount(CurrentFireMode);

		// Setup spawn parameters
		FActorSpawnParameters Params;
		Params.Instigator = UTOwner;
		Params.Owner = UTOwner;

		// Get muzzle location and rotation
		const FVector SpawnLocation = GetFireStartLoc();
		const FRotator SpawnRotation = GetAdjustedAim(SpawnLocation);

		// Fire projectiles
		AUTProjectile* MainProjectile = NULL;
		for (int32 i = 0; i < MultiShotCount[CurrentFireMode]; ++i)
		{
			// Get firing location and rotation for this projectile
			const FVector MultiShotLocation = GetFireLocationForMultiShot(i, SpawnLocation, SpawnRotation);
			const FRotator MultiShotRotation = GetFireRotationForMultiShot(i, SpawnLocation, SpawnRotation);

			// Get projectile class
			TSubclassOf<AUTProjectile> ProjectileClass = ProjClass[CurrentFireMode];
			if (i != 0 && MultiShotProjClass.IsValidIndex(CurrentFireMode) && MultiShotProjClass[CurrentFireMode] != NULL)
			{
				ProjectileClass = MultiShotProjClass[CurrentFireMode];
			}

			// Spawn projectile
			AUTProjectile* MultiShot = GetWorld()->SpawnActor<AUTProjectile>(ProjectileClass, MultiShotLocation, MultiShotRotation, Params);
			if (MainProjectile == NULL)
			{
				MainProjectile = MultiShot;
			}
		}

		return MainProjectile;
	}
}
