// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTCarriedObject.h"
#include "UTCTFFlag.generated.h"

UCLASS()
class UNREALTOURNAMENT_API AUTCTFFlag : public AUTCarriedObject
{
	GENERATED_UCLASS_BODY()

	// Flag mesh scaling when not held
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GameObject)
	float FlagWorldScale;

	// Flag mesh scaling when held
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GameObject)
	float FlagHeldScale;

	/** How much to blend in cloth when flag is home. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Flag)
	float ClothBlendHome;

	/** How much to blend in cloth when flag is held. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Flag)
	float ClothBlendHeld;

	// The mesh for the flag
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Flag)
	USkeletalMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Flag)
	UMaterialInstanceDynamic* MeshMID;

	/** TODO: test for flag aura effects */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Experimental)
	UStaticMeshComponent* AuraSphere;

	/** played on friendly flag when a capture is scored */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Flag)
	UParticleSystem* CaptureEffect;

	/** played on the location the flag was previously when returning */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Flag)
	UParticleSystem* ReturnSrcEffect;
	/** played on the flag at home when returning */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Flag)
	UParticleSystem* ReturnDestEffect;
	/** used to scale material parameters for return effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Flag)
	UCurveFloat* ReturnParamCurve;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Flag)
	FVector MeshOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Flag)
	FVector HeldOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Flag)
		float NearTeammateDist;

	/** duplicate of Mesh used temporarily as part of return effect */
	UPROPERTY()
	USkeletalMeshComponent* ReturningMesh;
	UPROPERTY()
	UMaterialInstanceDynamic* ReturningMeshMID;
	
	float ReturnEffectTime;

	/** used to trigger the capture effect */
	UPROPERTY(ReplicatedUsing = PlayCaptureEffect)
	uint8 CaptureEffectCount;

	UFUNCTION(BlueprintCallable, Category = Flag)
	virtual void PlayCaptureEffect();

protected:
	/** copy of our mesh rendered to CustomDepth for the outline (which is done in postprocess using the resulting data) */
	UPROPERTY()
	USkeletalMeshComponent* CustomDepthMesh;

public:
	virtual void UpdateOutline() override;

	virtual void PreNetReceive() override;
	virtual void PostNetReceiveLocationAndRotation() override;

	/** plays effects for flag returning
	 * NOTE: for 'despawn' end of effect to work this needs to be called BEFORE moving the flag
	 */
	virtual void PlayReturnedEffects();

	USkeletalMeshComponent* GetMesh() const
	{
		return Mesh;
	}

	virtual void ClientUpdateAttachment(bool bNowAttached) override;
	virtual void OnObjectStateChanged();

	FTimerHandle SendHomeWithNotifyHandle;
	virtual void SendHome() override;
	virtual void SendHomeWithNotify() override;
	virtual void Drop(AController* Killer) override;
	virtual void MoveToHome() override;
	virtual void SetHolder(AUTCharacter* NewHolder) override;

	virtual void Tick(float DeltaTime) override;

	/** World time when flag was last dropped. */
	UPROPERTY()
	float FlagDropTime;

	/** Broadcast delayed flag drop announcement. */
	virtual void DelayedDropMessage();

	virtual void PostInitializeComponents() override;

	/** Return true if is near enough to teammate to prevent gradual autoreturn. */
	virtual bool IsNearTeammate(AUTCharacter* TeamChar);

};