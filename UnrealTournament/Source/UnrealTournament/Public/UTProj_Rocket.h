// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTProjectile.h"
#include "UTProj_Rocket.generated.h"

UCLASS()
class UNREALTOURNAMENT_API AUTProj_Rocket : public AUTProjectile
{
	GENERATED_UCLASS_BODY()

	/** If set, rocket seeks this target. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_TargetActor, Category = RocketSeeking)
	AActor* TargetActor;

	UFUNCTION()
		void OnRep_TargetActor();

	/**The speed added to velocity in the direction of the target*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RocketSeeking)
		float AdjustmentSpeed;

	/** If true, lead tracked target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RocketSeeking)
	bool bLeadTarget;

	UPROPERTY(BlueprintReadOnly, Category = RocketSeeking)
		bool bRocketTeamSet;

	/** Reward announcement when kill with air rocket. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Announcement)
		TSubclassOf<class UUTRewardMessage> AirRocketRewardClass;

	/** Following rockets in burst. */
	UPROPERTY()
		TArray<AUTProj_Rocket*> FollowerRockets;

	UPROPERTY()
		class UMaterialInstanceDynamic* MeshMI;

	virtual void Tick(float DeltaTime) override;
	virtual void Explode_Implementation(const FVector& HitLocation, const FVector& HitNormal, UPrimitiveComponent* HitComp) override;
	virtual void OnRep_Instigator() override;

	virtual void DamageImpactedActor_Implementation(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FVector& HitLocation, const FVector& HitNormal);

};
