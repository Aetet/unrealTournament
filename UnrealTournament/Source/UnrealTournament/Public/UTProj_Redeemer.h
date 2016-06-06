// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTProjectile.h"
#include "UTProj_Redeemer.generated.h"

UCLASS(Abstract, meta = (ChildCanTick))
class UNREALTOURNAMENT_API AUTProj_Redeemer : public AUTProjectile
{
	GENERATED_UCLASS_BODY()
		
	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void NotifyClientSideHit(AUTPlayerController* InstigatedBy, FVector HitLocation, AActor* DamageCauser, int32 Damage) override;

	FVector ExplodeHitLocation;
	float ExplodeMomentum;

	float ExplosionTimings[5];
	float ExplosionRadii[6];
	float CollisionFreeRadius;

	void Explode_Implementation(const FVector& HitLocation, const FVector& HitNormal, UPrimitiveComponent* HitComp) override;

	void ExplodeStage(float RangeMultiplier);

	virtual void RedeemerDenied(AController* InstigatedBy);

	/** Capsule collision component */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Projectile)
	UCapsuleComponent* CapsuleComp;

	UFUNCTION()
	virtual void ExplodeTimed();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Detonate)
	int32 ProjHealth;

	/** Replicate to client to play detonate effects client-side. */
	UPROPERTY(Replicated)
	bool bDetonated;

	/** Play/stop effects for shot down Redeemer that explodes after falling out of the sky */
	virtual void PlayShotDownEffects();

	virtual void TornOff() override;
	virtual void OnShotDown();

	UFUNCTION()
	void ExplodeStage1();
	UFUNCTION()
	void ExplodeStage2();
	UFUNCTION()
	void ExplodeStage3();
	UFUNCTION()
	void ExplodeStage4();
	UFUNCTION()
	void ExplodeStage5();
	UFUNCTION()
	void ExplodeStage6();

	virtual void Tick(float DeltaTime) override;

protected:
	/** for outline rendering */
	UPROPERTY()
	UMeshComponent* CustomDepthMesh;
};
