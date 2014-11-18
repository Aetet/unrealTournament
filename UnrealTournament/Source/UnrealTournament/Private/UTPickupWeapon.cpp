// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTPickup.h"
#include "UTPickupInventory.h"
#include "UTPickupWeapon.h"

void AUTPickupWeapon::BeginPlay()
{
	AUTPickup::BeginPlay(); // skip AUTPickupInventory so we can propagate WeaponType as InventoryType

	SetInventoryType(WeaponType);
}

void AUTPickupWeapon::SetInventoryType(TSubclassOf<AUTInventory> NewType)
{
	WeaponType = *NewType;
	if (WeaponType == NULL)
	{
		NewType = NULL;
	}
	Super::SetInventoryType(NewType);
}

bool AUTPickupWeapon::IsTaken(APawn* TestPawn)
{
	for (int32 i = Customers.Num() - 1; i >= 0; i--)
	{
		if (Customers[i].P == NULL || Customers[i].P->bTearOff || Customers[i].P->bPendingKillPending)
		{
			Customers.RemoveAt(i);
		}
		else if (Customers[i].P == TestPawn)
		{
			return (GetWorld()->TimeSeconds < Customers[i].NextPickupTime);
		}
	}
	return false;
}

float AUTPickupWeapon::GetRespawnTimeOffset(APawn* Asker) const
{
	if (!State.bActive)
	{
		return Super::GetRespawnTimeOffset(Asker);
	}
	else
	{
		AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
		if (WeaponType == NULL || !WeaponType.GetDefaultObject()->bWeaponStay || (GS != NULL && !GS->bWeaponStay))
		{
			return Super::GetRespawnTimeOffset(Asker);
		}
		else
		{
			for (int32 i = Customers.Num() - 1; i >= 0; i--)
			{
				if (Customers[i].P == Asker)
				{
					return (Customers[i].NextPickupTime - GetWorld()->TimeSeconds);
				}
			}
			return -100000.0f;
		}
	}
}

void AUTPickupWeapon::ProcessTouch_Implementation(APawn* TouchedBy)
{
	if (State.bActive)
	{
		AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
		if (WeaponType == NULL || !WeaponType.GetDefaultObject()->bWeaponStay || (GS != NULL && !GS->bWeaponStay))
		{
			Super::ProcessTouch_Implementation(TouchedBy);
		}
		// note that we don't currently call AllowPickupBy() and associated GameMode/Mutator overrides in the weapon stay case
		// in part due to client synchronization issues
		else if (!IsTaken(TouchedBy) && Cast<AUTCharacter>(TouchedBy) != NULL && !((AUTCharacter*)TouchedBy)->IsRagdoll())
		{
			new(Customers) FWeaponPickupCustomer(TouchedBy, GetWorld()->TimeSeconds + RespawnTime);
			if (Role == ROLE_Authority)
			{
				GiveTo(TouchedBy);
				if (!GetWorldTimerManager().IsTimerActive(this, &AUTPickupWeapon::CheckTouching))
				{
					GetWorldTimerManager().SetTimer(this, &AUTPickupWeapon::CheckTouching, RespawnTime, false);
				}
			}
			PlayTakenEffects(false);
			if (TouchedBy->IsLocallyControlled())
			{
				AUTPlayerController* PC = Cast<AUTPlayerController>(TouchedBy->Controller);
				if (PC != NULL)
				{
					PC->AddWeaponPickup(this);
				}
			}
		}
	}
}

void AUTPickupWeapon::CheckTouching()
{
	TArray<AActor*> Touching;
	GetOverlappingActors(Touching, APawn::StaticClass());
	for (AActor* TouchingActor : Touching)
	{
		APawn* P = Cast<APawn>(TouchingActor);
		if (P != NULL)
		{
			ProcessTouch(P);
		}
	}
	// see if we should reset the timer
	float NextCheckInterval = 0.0f;
	for (const FWeaponPickupCustomer& PrevCustomer : Customers)
	{
		NextCheckInterval = FMath::Max<float>(NextCheckInterval, PrevCustomer.NextPickupTime - GetWorld()->TimeSeconds);
	}
	if (NextCheckInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(this, &AUTPickupWeapon::CheckTouching, NextCheckInterval, false);
	}
}

#if WITH_EDITOR
void AUTPickupWeapon::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (TimerSprite != NULL && GetWorld() != NULL && GetWorld()->WorldType == EWorldType::Editor)
	{
		TimerSprite->SetVisibility(WeaponType == NULL || !WeaponType.GetDefaultObject()->bWeaponStay);
	}
}
void AUTPickupWeapon::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// only show timer sprite for superweapons
	if (TimerSprite != NULL)
	{
		TimerSprite->SetVisibility(WeaponType == NULL || !WeaponType.GetDefaultObject()->bWeaponStay);
	}
}
#endif