// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UnrealNetwork.h"
#include "UTWeap_GrenadeLauncher.h"

AUTWeap_GrenadeLauncher::AUTWeap_GrenadeLauncher()
{
	DefaultGroup = 3;
	Group = 3;
	WeaponCustomizationTag = EpicWeaponCustomizationTags::GrenadeLauncher;
}

bool AUTWeap_GrenadeLauncher::BeginFiringSequence(uint8 FireModeNum, bool bClientFired)
{
	if (bHasStickyGrenades && FireModeNum == 1)
	{
		if (Role == ROLE_Authority)
		{
			DetonateStickyGrenades();
		}
		
		if (UTOwner && UTOwner->IsLocallyViewed())
		{
			PlayDetonationEffects();
		}

		return true;
	}

	return Super::BeginFiringSequence(FireModeNum, bClientFired);
}

void AUTWeap_GrenadeLauncher::RegisterStickyGrenade(AUTProj_Grenade_Sticky* InGrenade)
{
	ActiveGrenades.AddUnique(InGrenade);
	bHasStickyGrenades = true;
	OnRep_HasStickyGrenades();
	ActiveStickyGrenadeCount = ActiveGrenades.Num();
}

void AUTWeap_GrenadeLauncher::UnregisterStickyGrenade(AUTProj_Grenade_Sticky* InGrenade)
{
	ActiveGrenades.Remove(InGrenade);
	if (ActiveGrenades.Num() == 0)
	{
		bHasStickyGrenades = false;
		OnRep_HasStickyGrenades();
	}
	ActiveStickyGrenadeCount = ActiveGrenades.Num();
}

void AUTWeap_GrenadeLauncher::DetonateStickyGrenades()
{
	for (int i = 0; i < ActiveGrenades.Num(); i++)
	{
		if (ActiveGrenades[i] != nullptr)
		{
			ActiveGrenades[i]->ArmGrenade();
			ActiveGrenades[i]->Explode(ActiveGrenades[i]->GetActorLocation(), FVector(0, 0, 1), nullptr);
		}
	}
	ActiveGrenades.Empty();
	bHasStickyGrenades = false;
	OnRep_HasStickyGrenades();
	ActiveStickyGrenadeCount = ActiveGrenades.Num();
}

void AUTWeap_GrenadeLauncher::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AUTWeap_GrenadeLauncher, bHasStickyGrenades);
	DOREPLIFETIME(AUTWeap_GrenadeLauncher, bHasLoadedStickyGrenades);	
	DOREPLIFETIME(AUTWeap_GrenadeLauncher, ActiveStickyGrenadeCount);
}

void AUTWeap_GrenadeLauncher::BringUp(float OverflowTime)
{
	Super::BringUp(OverflowTime);

	if (bHasStickyGrenades)
	{
		ShowDetonatorUI();
	}
}

bool AUTWeap_GrenadeLauncher::PutDown()
{
	if (Super::PutDown())
	{
		HideDetonatorUI();
		return true;
	}

	return false;
}

void AUTWeap_GrenadeLauncher::OnRep_HasStickyGrenades()
{
	if (bHasStickyGrenades)
	{
		ShowDetonatorUI();
	}
	else
	{
		HideDetonatorUI();
	}
}

void AUTWeap_GrenadeLauncher::Destroyed()
{
	Super::Destroyed();

	HideDetonatorUI();
}

void AUTWeap_GrenadeLauncher::DetachFromOwner_Implementation()
{
	HideDetonatorUI();

	Super::DetachFromOwner_Implementation();
}

void AUTWeap_GrenadeLauncher::DropFrom(const FVector& StartLocation, const FVector& TossVelocity)
{
	// Don't preserve grenade references between owners
	ActiveGrenades.Empty();
	bHasStickyGrenades = false;
	bHasLoadedStickyGrenades = false;
	Super::DropFrom(StartLocation, TossVelocity);
}

void AUTWeap_GrenadeLauncher::OnRep_Ammo()
{
	// Skip auto weapon switch if there's sticky grenades out
	if (bHasStickyGrenades || bHasLoadedStickyGrenades)
	{
		return;
	}

	Super::OnRep_Ammo();
}

void AUTWeap_GrenadeLauncher::ConsumeAmmo(uint8 FireModeNum)
{
	if (FireModeNum == 1)
	{
		bHasLoadedStickyGrenades = true;
	}

	Super::ConsumeAmmo(FireModeNum);
}

void AUTWeap_GrenadeLauncher::FiringInfoUpdated_Implementation(uint8 InFireMode, uint8 FlashCount, FVector InFlashLocation)
{
	Super::FiringInfoUpdated_Implementation(InFireMode, FlashCount, InFlashLocation);
	bHasLoadedStickyGrenades = false;
}

bool AUTWeap_GrenadeLauncher::CanSwitchTo()
{
	if (bHasStickyGrenades)
	{
		return true;
	}

	return Super::CanSwitchTo();
}