// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTDamageType.h"

UUTDamageType::UUTDamageType(const FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	// hack to allow blueprint damagetypes to have an event graph
	// TODO: disabled at the moment as blueprints have all kinds of issues interacting with functions on default objects due to implicit variable creation
	//GetClass()->ClassFlags &= ~CLASS_Const;

	DamageImpulse = 50000.0f;
	DestructibleImpulse = 50000.0f;
	bForceZMomentum = true;
	GibHealthThreshold = -50;
	GibDamageThreshold = 99;
}

FVector UTGetDamageMomentum(const FDamageEvent& DamageEvent, const AActor* HitActor, const AController* EventInstigator)
{
	if (DamageEvent.IsOfType(FUTPointDamageEvent::ClassID))
	{
		return ((const FUTPointDamageEvent&)DamageEvent).Momentum;
	}
	else if (DamageEvent.IsOfType(FUTRadialDamageEvent::ClassID))
	{
		const FUTRadialDamageEvent& RadialEvent = (const FUTRadialDamageEvent&)DamageEvent;
		float Magnitude = RadialEvent.BaseMomentumMag;
		// default to taking the average of all hit components
		if (RadialEvent.ComponentHits.Num() == 0)
		{
			// don't think this can happen but doesn't hurt to be safe
			return (HitActor->GetActorLocation() - RadialEvent.Origin).SafeNormal() * Magnitude;
		}
		// accommodate origin being same as hit location
		else if (RadialEvent.ComponentHits.Num() == 1 && (RadialEvent.ComponentHits[0].Location - RadialEvent.Origin).IsNearlyZero())
		{
			if ((RadialEvent.ComponentHits[0].TraceStart - RadialEvent.ComponentHits[0].TraceEnd).IsNearlyZero())
			{
				// 'fake' hit generated because no component trace succeeded even though radius check worked
				// in this case, use direction to component center
				return (RadialEvent.ComponentHits[0].Component->GetComponentLocation() - RadialEvent.Origin).SafeNormal() * Magnitude;
			}
			else
			{
				return (RadialEvent.ComponentHits[0].TraceEnd - RadialEvent.ComponentHits[0].TraceStart).SafeNormal() * Magnitude;
			}
		}
		else
		{
			FVector Avg(FVector::ZeroVector);
			for (int32 i = 0; i < RadialEvent.ComponentHits.Num(); i++)
			{
				Avg += RadialEvent.ComponentHits[i].Location;
			}

			return (Avg / RadialEvent.ComponentHits.Num() - RadialEvent.Origin).SafeNormal() * Magnitude;
		}
	}
	else
	{
		TSubclassOf<UDamageType> DamageType = DamageEvent.DamageTypeClass;
		if (DamageType == NULL)
		{
			DamageType = UUTDamageType::StaticClass();
		}
		FHitResult HitInfo;
		FVector MomentumDir;
		DamageEvent.GetBestHitInfo(HitActor, EventInstigator, HitInfo, MomentumDir);
		return MomentumDir * DamageType.GetDefaultObject()->DamageImpulse;
	}
}

void UUTDamageType::ScoreKill_Implementation(AUTPlayerState* KillerState, AUTPlayerState* VictimState, APawn* KilledPawn) const
{
}

bool UUTDamageType::ShouldGib_Implementation(AUTCharacter* Victim) const
{
	return (Victim->Health <= GibHealthThreshold || Victim->LastTakeHitInfo.Damage >= GibDamageThreshold);
}