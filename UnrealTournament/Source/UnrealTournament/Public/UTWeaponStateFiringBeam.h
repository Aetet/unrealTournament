// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTWeaponStateFiring.h"

#include "UTWeaponStateFiringBeam.generated.h"

UCLASS()
class UNREALTOURNAMENT_API UUTWeaponStateFiringBeam : public UUTWeaponStateFiring
{
	GENERATED_UCLASS_BODY()

	/** beam damage accumulator must reach this value before the damage is applied to the target */
	UPROPERTY(EditDefaultsOnly, Category = Damage)
	int32 MinDamage;

	/** current damage fraction (<= MinDamage) */
	UPROPERTY(BlueprintReadWrite, Category = Damage)
	float Accumulator;

	UPROPERTY(EditDefaultsOnly, Category = Effects)
	UAnimMontage* FiringLoopAnim;
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	UAnimMontage* FiringLoopAnimHands;
	/** jump loop anim to this section when stopping fire (if not specified, anim is hard stopped) */
	UPROPERTY(EditDefaultsOnly, Category = Effects)
	FName EndAnimSection;

	virtual void EndState() override;
	virtual void FireShot() override;
	virtual void EndFiringSequence(uint8 FireModeNum)
	{
		Super::EndFiringSequence(FireModeNum);
		if (FireModeNum == GetOuterAUTWeapon()->GetCurrentFireMode())
		{
			GetOuterAUTWeapon()->GotoActiveState();
		}
	}
	virtual void Tick(float DeltaTime) override;
};