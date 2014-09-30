// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTResetInterface.h"
#include "UTPathBuilderInterface.h"
#include "UTPlayerState.h"
#include "UTPickup.generated.h"

UENUM()
enum EPickupClassification
{
	PC_Minor,
	PC_Major,
	PC_Super,
};

USTRUCT()
struct FPickupReplicatedState
{
	GENERATED_USTRUCT_BODY()

	/** whether the pickup is currently active */
	UPROPERTY(BlueprintReadOnly, Category = Pickup)
	uint32 bActive : 1;
	/** plays taken effects when received on client as true
	 * only has meaning when !bActive
	 */
	UPROPERTY()
	uint32 bRepTakenEffects : 1;
	/** counter used to make sure same-frame toggles replicate correctly (activated while player is standing on it so immediate pickup) */
	UPROPERTY()
	uint8 ChangeCounter;
};

UCLASS(abstract, Blueprintable, meta = (ChildCanTick))
class UNREALTOURNAMENT_API AUTPickup : public AActor, public IUTResetInterface, public IUTPathBuilderInterface
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Pickup)
	TSubobjectPtr<UCapsuleComponent> Collision;
	// hack: UMaterialBillboardComponent isn't exposed, can't use native subobject
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = Pickup)
	UMaterialBillboardComponent* TimerSprite;
	//UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Pickup)
	//TSubobjectPtr<UMaterialBillboardComponent> TimerSprite;

	/** respawn time for the pickup; if it's <= 0 then the pickup doesn't respawn until the round resets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Pickup)
	float RespawnTime;
	/** if set, pickup begins play with its respawn time active */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Pickup)
	uint32 bDelayedSpawn : 1;
	/** whether to display TimerSprite/TimerText on the pickup while it is respawning */
	uint32 bDisplayRespawnTimer : 1;
	/** one-shot particle effect played when the pickup is taken */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects)
	UParticleSystem* TakenParticles;
	/** one-shot sound played when the pickup is taken */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects)
	USoundBase* TakenSound;
	/** one-shot particle effect played when the pickup respawns */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects)
	UParticleSystem* RespawnParticles;
	/** one-shot sound played when the pickup respawns */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects)
	USoundBase* RespawnSound;
	/** all components with any of these tags will be hidden when the pickup is taken
	 * if the array is empty, the entire pickup Actor is hidden
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Effects)
	TArray<FName> TakenHideTags;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = Pickup)
	FPickupReplicatedState State;

	UPROPERTY(BlueprintReadOnly, Category = Effects)
	UMaterialInstanceDynamic* TimerMI;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pickup)
	TEnumAsByte<EPickupClassification> PickupType;

	// Holds the PRI of the last player to pick this item up.  Used to give a controlling bonus to score
	UPROPERTY(BlueprintReadOnly, Category = Game)
	AUTPlayerState* LastPickedUpBy;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	virtual void PreNetReceive();
	virtual void PostNetReceive();

	UFUNCTION()
	virtual void OnOverlapBegin(AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult);

	UFUNCTION(BlueprintNativeEvent)
	void ProcessTouch(APawn* TouchedBy);

	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly)
	void GiveTo(APawn* Target);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Pickup)
	void StartSleeping();
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Pickup)
	void WakeUp();
	/** used for the timer-based call to WakeUp() so clients can perform different behavior to handle possible sync issues */
	UFUNCTION()
	void WakeUpTimer();
	UFUNCTION(BlueprintCallable, Category = Pickup)
	virtual void PlayTakenEffects(bool bReplicate);
	UFUNCTION(BlueprintCallable, Category = Pickup)
	virtual void PlayRespawnEffects();
	/** sets the hidden state of the pickup - note that this doesn't necessarily mean the whole object (e.g. item mesh but not holder) */
	UFUNCTION(BlueprintCallable, Category = Pickup)
	virtual void SetPickupHidden(bool bNowHidden);

	virtual void Reset_Implementation() override;

	virtual void PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker);
	
	// Handle creating the MID and hiding timer sprite by default
	void SetupTimerSprite();

	/** base desireability for AI acquisition/inventory searches (i.e. BotDesireability()) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AI)
	float BaseDesireability;

	/** returns how much the given Pawn (generally AI controlled) wants this item, where anything >= 1.0 is considered very important
	 * called only when either the pickup is active now, or if timing is enabled and the pickup will become active by the time Asker can cross the distance
	 * note that it isn't necessary for this function to modify the weighting based on the path distance as that is handled internally;
	 * the distance is supplied so that the code can make timing decisions and cost/benefit analysis
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = AI)
	float BotDesireability(APawn* Asker, float PathDistance);

protected:
	/** used to replicate remaining respawn time to newly joining clients */
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_RespawnTimeRemaining)
	float RespawnTimeRemaining;

	UFUNCTION()
	virtual void OnRep_RespawnTimeRemaining();
};
