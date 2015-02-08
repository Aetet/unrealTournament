// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTWeaponState.h"
#include "UTWeaponStateEquipping.h"
#include "UTWeaponStateUnequipping.h"

void UUTWeaponStateUnequipping::BeginState(const UUTWeaponState* PrevState)
{
	const UUTWeaponStateEquipping* PrevEquip = Cast<UUTWeaponStateEquipping>(PrevState);

	// if was previously equipping, pay same amount of time to take back down
	UnequipTime = (PrevEquip != NULL) ? FMath::Min(PrevEquip->PartialEquipTime, GetOuterAUTWeapon()->GetPutDownTime()) : GetOuterAUTWeapon()->GetPutDownTime();
	UnequipTimeElapsed = 0.0f;
	if (UnequipTime <= 0.0f)
	{
		PutDownFinished();
	}
	else
	{
		GetOuterAUTWeapon()->GetWorldTimerManager().SetTimer(PutDownFinishedHandle, this, &UUTWeaponStateUnequipping::PutDownFinished, UnequipTime);
		if (GetOuterAUTWeapon()->PutDownAnim != NULL)
		{
			UAnimInstance* AnimInstance = GetOuterAUTWeapon()->GetMesh()->GetAnimInstance();
			if (AnimInstance != NULL)
			{
				AnimInstance->Montage_Play(GetOuterAUTWeapon()->PutDownAnim, GetOuterAUTWeapon()->PutDownAnim->SequenceLength / UnequipTime);
			}
		}
	}
}

void UUTWeaponStateEquipping::BeginState(const UUTWeaponState* PrevState)
{
	const UUTWeaponStateUnequipping* PrevEquip = Cast<UUTWeaponStateUnequipping>(PrevState);
	// if was previously unequipping, pay same amount of time to bring back up
	EquipTime = (PrevEquip != NULL) ? FMath::Min(PrevEquip->PartialEquipTime, GetOuterAUTWeapon()->GetBringUpTime()) : GetOuterAUTWeapon()->GetBringUpTime();
	PendingFireSequence = -1;
	if (EquipTime <= 0.0f)
	{
		BringUpFinished();
	}
	// else require StartEquip() to start timer/anim so overflow time (if any) can be passed in
}

void UUTWeaponStateEquipping::BringUpFinished()
{
	GetOuterAUTWeapon()->GotoActiveState();
	if (PendingFireSequence >= 0)
	{
		GetOuterAUTWeapon()->bNetDelayedShot = true;
		GetOuterAUTWeapon()->BeginFiringSequence(PendingFireSequence, true);
		PendingFireSequence = -1;
		GetOuterAUTWeapon()->bNetDelayedShot = false;
	}
}

void UUTWeaponStateEquipping::StartEquip(float OverflowTime)
{
	EquipTime -= OverflowTime;
	if (EquipTime <= 0.0f)
	{
		BringUpFinished();
	}
	else
	{
		GetOuterAUTWeapon()->GetWorldTimerManager().SetTimer(BringUpFinishedHandle, this, &UUTWeaponStateEquipping::BringUpFinished, EquipTime);
		if (GetOuterAUTWeapon()->BringUpAnim != NULL)
		{
			UAnimInstance* AnimInstance = GetOuterAUTWeapon()->GetMesh()->GetAnimInstance();
			if (AnimInstance != NULL)
			{
				AnimInstance->Montage_Play(GetOuterAUTWeapon()->BringUpAnim, GetOuterAUTWeapon()->BringUpAnim->SequenceLength / EquipTime);
			}
		}
	}
}

bool UUTWeaponStateEquipping::BeginFiringSequence(uint8 FireModeNum, bool bClientFired)
{
	// on server, might not be quite done equipping yet when client done, so queue firing
	if (bClientFired)
	{
		PendingFireSequence = FireModeNum;
		GetUTOwner()->NotifyPendingServerFire();
	}
	return false;
}
