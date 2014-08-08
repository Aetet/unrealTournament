// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UTImpactEffect.generated.h"

/** encapsulates all of the components of an impact or explosion effect (particles, sound, decals, etc)
 * contains functionality to LOD by distance and settings
 * this class is an Actor primarily for the editability features and should not be directly spawned
 */
UCLASS(Blueprintable, Abstract)
class AUTImpactEffect : public AActor
{
	GENERATED_UCLASS_BODY()

	/** if > 0, effect is not played if beyond this distance */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Effects)
	float CullDistance;
	/** if set, check that spawn location is visible to at least one local player before spawning */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Effects)
	bool bCheckInView;
	/** if set, always spawn when caused by local human player */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Effects)
	bool bForceForLocalPlayer;
	/** if set, attach to hit component (if any) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Effects)
	bool bAttachToHitComp;

	/** one shot audio played with the effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Effects)
	USoundBase* Audio;

	/** if given a value, materials in created components will have this parameter set to the world time they were created */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Effects)
	FName WorldTimeParam;

	void CreateEffectComponents(UWorld* World, const FTransform& BaseTransform, UPrimitiveComponent* HitComp, AActor* SpawnedBy, AController* InstigatedBy, USceneComponent* CurrentAttachment, FName TemplateName, const TArray<USceneComponent*>& NativeCompList, const TArray<USCS_Node*>& BPNodes) const;

	/** after deciding to spawn the effect as a whole, this event can be used to filter out individual components
	 * note that the component passed in is a template and shouldn't be modified
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic, Category = Effects)
	bool ShouldCreateComponent(const USceneComponent* TestComp, FName CompTemplateName, const FTransform& BaseTransform, UPrimitiveComponent* HitComp, AActor* SpawnedBy, AController* InstigatedBy) const;

	/** called on each created component for further modification
	 * HACK: the return value is unused; it is there to force the function out of the blueprint event graph as the event graph is not safe to use on default objects
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic, Category = Effects)
	bool ComponentCreated(USceneComponent* NewComp, UPrimitiveComponent* HitComp, AActor* SpawnedBy, AController* InstigatedBy) const;

	/** spawns the effect's components as appropriate for the viewing distance, system settings, etc
	 * some aspects may be replicated (particularly audio), so call even on dedicated server and let the effect sort it out
	 * @param HitComp - component that was hit to cause the effect (if any)
	 * @param SpawnedBy - calling Actor, if any (for example, the projectile that exploded), commonly used for LastRenderTime checks to avoid the effect
	 * @param InstigatedBy - Controller that instigated the effect, if any - commonly used to prioritize effects created by local players
	 */
	UFUNCTION(BlueprintNativeEvent, Category = Effects)
	bool SpawnEffect(UWorld* World, const FTransform& InTransform, UPrimitiveComponent* HitComp = NULL, AActor* SpawnedBy = NULL, AController* InstigatedBy = NULL, ESoundReplicationType SoundReplication = SRT_IfSourceNotReplicated) const;

	// Blueprint redirect with more BP-friendly parameters
#if CPP
private:
#endif
	/** spawns the effect's components as appropriate for the viewing distance, system settings, etc
	 * some aspects may be replicated (particularly audio), so call even on dedicated server and let the effect sort it out
	 * @param HitComp - component that was hit to cause the effect (if any)
	 * @param SpawnedBy - calling Actor, if any (for example, the projectile that exploded), commonly used for LastRenderTime checks to avoid the effect
	 * @param InstigatedBy - Controller that instigated the effect, if any - commonly used to prioritize effects created by local players
	 */
	UFUNCTION(BlueprintCallable, meta = (HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject", FriendlyName = "SpawnEffect"), Category = Effects)
	static void CallSpawnEffect(UObject* WorldContextObject, const AUTImpactEffect* Effect, const FTransform& InTransform, UPrimitiveComponent* HitComp = NULL, AActor* SpawnedBy = NULL, AController* InstigatedBy = NULL, ESoundReplicationType SoundReplication = SRT_IfSourceNotReplicated);

	virtual void PostInitializeComponents() override
	{
		Super::PostInitializeComponents();
		// we allow placing these so artists can more easily test their effects, but they should never be spawned this way in a normal game situation
		if (GetWorld() != NULL && GetWorld()->WorldType == EWorldType::Game)
		{
			if (GAreScreenMessagesEnabled)
			{
				GEngine->AddOnScreenDebugMessage((uint64)-1, 3.0f, FColor(255, 0, 0), TEXT("UTImpactEffects should not be spawned! Use the SpawnEffect function instead."));
			}
			UE_LOG(UT, Warning, TEXT("UTImpactEffects should not be spawned! Use the SpawnEffect function instead."));
			Destroy();
		}
	}
};