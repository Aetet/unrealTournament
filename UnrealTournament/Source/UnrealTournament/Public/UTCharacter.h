// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTCarriedObject.h"

#include "UTCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCharacterDiedSignature, class AController*, Killer, const class UDamageType*, DamageType);

/** Replicated movement data of our RootComponent.
* More efficient than engine's FRepMovement
*/
USTRUCT()
struct FRepUTMovement
{
	GENERATED_USTRUCT_BODY()

	/** @TODO FIXMESTEVE version that just replicates XY components, plus could quantize to tens easily */
	UPROPERTY()
	FVector_NetQuantize LinearVelocity;

	UPROPERTY()
	FVector_NetQuantize Location;

	/** @TODO FIXMESTEVE only need a few bits for this - maybe hide in Rotation.Roll */
	//UPROPERTY()
	//FVector_NetQuantize Acceleration;

	UPROPERTY()
	FRotator Rotation;

	FRepUTMovement()
		: LinearVelocity(ForceInit)
		, Location(ForceInit)
		//, Acceleration(ForceInit)
		, Rotation(ForceInit)
	{}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		bOutSuccess = true;

		bool bOutSuccessLocal = true;

		// update location, linear velocity
		Location.NetSerialize(Ar, Map, bOutSuccessLocal);
		bOutSuccess &= bOutSuccessLocal;
		Rotation.SerializeCompressed(Ar);
		LinearVelocity.NetSerialize(Ar, Map, bOutSuccessLocal);
		bOutSuccess &= bOutSuccessLocal;
		//Acceleration.NetSerialize(Ar, Map, bOutSuccessLocal);
		//bOutSuccess &= bOutSuccessLocal;

		return true;
	}

	bool operator==(const FRepUTMovement& Other) const
	{
		if (LinearVelocity != Other.LinearVelocity)
		{
			return false;
		}

		if (Location != Other.Location)
		{
			return false;
		}

		if (Rotation != Other.Rotation)
		{
			return false;
		}
		/*
		if (Acceleration != Other.Acceleration)
		{
			return false;
		}
		*/
		return true;
	}

	bool operator!=(const FRepUTMovement& Other) const
	{
		return !(*this == Other);
	}
};

USTRUCT(BlueprintType)
struct FTakeHitInfo
{
	GENERATED_USTRUCT_BODY()

	/** the amount of damage */
	UPROPERTY(BlueprintReadWrite, Category = TakeHitInfo)
	int32 Damage;
	/** the location of the hit (relative to Pawn center) */
	UPROPERTY(BlueprintReadWrite, Category = TakeHitInfo)
	FVector_NetQuantize RelHitLocation;
	/** how much momentum was imparted */
	UPROPERTY(BlueprintReadWrite, Category = TakeHitInfo)
	FVector_NetQuantize Momentum;
	/** the damage type we were hit with */
	UPROPERTY(BlueprintReadWrite, Category = TakeHitInfo)
	TSubclassOf<UDamageType> DamageType;
	/** shot direction pitch, manually compressed */
	UPROPERTY(BlueprintReadWrite, Category = TakeHitInfo)
	uint8 ShotDirPitch;
	/** shot direction yaw, manually compressed */
	UPROPERTY(BlueprintReadWrite, Category = TakeHitInfo)
	uint8 ShotDirYaw;
	/** if damage was partially or completely absorbed by inventory, the item that did so
	 * used to play different effects
	 */
	UPROPERTY(BlueprintReadWrite, Category = TakeHitInfo)
	TSubclassOf<class AUTInventory> HitArmor;
};

/** ammo counter */
USTRUCT(BlueprintType)
struct FStoredAmmo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Ammo)
	TSubclassOf<class AUTWeapon> Type;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Ammo)
	int32 Amount;
};

USTRUCT()
struct FEmoteRepInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	bool bNewData;

	UPROPERTY()
	int32 EmoteIndex;
};

USTRUCT(BlueprintType)
struct FSavedPosition
{
	GENERATED_USTRUCT_BODY()

	FSavedPosition() : Position(FVector(0.f)), Velocity(FVector(0.f)), Time(0.f) {};

	FSavedPosition(FVector InPos, FVector InVel, float InTime) : Position(InPos), Velocity(InVel), Time(InTime) {};

	/** Position of player at time Time. */
	UPROPERTY()
	FVector Position;

	/** Keep velocity also for bots to use in realistic reaction time based aiming error model. */
	UPROPERTY()
	FVector Velocity;

	float Time;
};

UENUM(BlueprintType)
enum EAllowedSpecialMoveAnims
{
	EASM_Any,
	EASM_UpperBodyOnly, 
	EASM_None,
};

USTRUCT(BlueprintType)
struct FBloodDecalInfo
{
	GENERATED_USTRUCT_BODY()

	/** material to use for the decal */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DecalInfo)
	UMaterialInterface* Material;
	/** Base scale of decal */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DecalInfo)
	FVector2D BaseScale;
	/** range of random scaling applied (always uniform) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DecalInfo)
	FVector2D ScaleMultRange;

	FBloodDecalInfo()
		: Material(NULL), BaseScale(32.0f, 32.0f), ScaleMultRange(0.8f, 1.2f)
	{}
};

UCLASS(config=Game, collapsecategories, hidecategories=(Clothing,Lighting,AutoExposure,LensFlares,AmbientOcclusion,DepthOfField,MotionBlur,Misc,ScreenSpaceReflections,Bloom,SceneColor,Film,AmbientCubemap,AgentPhysics,Attachment,Avoidance,PlanarMovement,AI,Replication,Input,Actor,Tags,GlobalIllumination))
class UNREALTOURNAMENT_API AUTCharacter : public ACharacter, public IUTTeamInterface
{
	GENERATED_UCLASS_BODY()

	//====================================
	// Networking

	/** Used for replication of our RootComponent's position and velocity */
	UPROPERTY(ReplicatedUsing = OnRep_UTReplicatedMovement)
	struct FRepUTMovement UTReplicatedMovement;

	/** Replicated property used to replicate last acknowledged good move timestamp from server, replaces ClientAckGoodMove() */
	UPROPERTY(ReplicatedUsing = OnRep_GoodMoveAckTime)
	float GoodMoveAckTime;

	/** @TODO FIXMESTEVE Temporary different name until engine team makes UpdateSimulatedPosition() virtual */
	virtual void UTUpdateSimulatedPosition(const FVector & NewLocation, const FRotator & NewRotation, const FVector& NewVelocity);

	virtual void PostNetReceiveLocationAndRotation();

	// @TODO FIXMESTEVE move these properties to FNetworkPredictionData_Client_UTChar(?)  also needed by projectiles
	/** Estimated value of server contribution to ping, used when calculating how far to simulated ahead */
	UPROPERTY(EditAnywhere, Category = Network)
	float ServerPingContribution;

	/** Max amount of ping to predict ahead for */
	UPROPERTY(EditAnywhere, Category = Network)
	float MaxPredictionPing;

	/** @TODO FIXMESTEVE USE IT OR LOSE IT! */
	UPROPERTY(BluePrintReadOnly, Category = Network)
	FVector ReplicatedAccel;

	/** UTReplicatedMovement struct replication event */
	UFUNCTION()
	virtual void OnRep_UTReplicatedMovement();

	/** GoodMoveAckTimereplication event */
	UFUNCTION()
	virtual void OnRep_GoodMoveAckTime();

	virtual void PreReplication(IRepChangedPropertyTracker & ChangedPropertyTracker) override;

	/** UTCharacter version of GatherMovement(), gathers into UTReplicatedMovement.  Return true if using UTReplicatedMovement rather than ReplicatedMovement */
	virtual bool GatherUTMovement();

	virtual void OnRep_ReplicatedMovement() override;

	//====================================

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	TSubobjectPtr<class USkeletalMeshComponent> FirstPersonMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	TSubobjectPtr<class UCameraComponent> CharacterCameraComponent;

	/** Cached UTCharacterMovement casted CharacterMovement */
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly)
	class UUTCharacterMovement* UTCharacterMovement;

	UPROPERTY(replicatedUsing=OnRepEmote)
	FEmoteRepInfo EmoteReplicationInfo;
	
	UFUNCTION()
	virtual void OnRepEmote();

	UPROPERTY(replicatedUsing=OnRepEmoteSpeed)
	float EmoteSpeed;
	
	UFUNCTION()
	virtual void OnRepEmoteSpeed();

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerSetEmoteSpeed(float NewEmoteSpeed);

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerFasterEmote();

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerSlowerEmote();

	void PlayEmote(int32 EmoteIndex);

	UFUNCTION()
	void OnEmoteEnded(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY(EditDefaultsOnly, Category = Pawn)
	TArray<UAnimMontage*> EmoteAnimations;
	
	UPROPERTY()
	UAnimMontage* CurrentEmote;

	// Keep track of emote count so we can clear CurrentEmote
	int32 EmoteCount;

	/** Stored past positions of this player.  Used for bot aim error model, and for server side hit resolution. */
	UPROPERTY()
	TArray<FSavedPosition> SavedPositions;	

	/** Maximum interval to hold saved positions for. */
	UPROPERTY()
	float MaxSavedPositionAge;

	/** Called by CharacterMovement after movement */
	virtual void PositionUpdated();

	/** Limit to armor stacking */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pawn")
	int32 MaxStackedArmor;

	/** Find existing armor, make sure total doesn't exceed MaxStackedArmor */
	UFUNCTION(BlueprintCallable, Category = Pawn)
	virtual void CheckArmorStacking();

	/** Remove excess armor from the lowest absorption armor type.  Returns amount of armor removed. */
	virtual int32 ReduceArmorStack(int32 Amount);

	/** counters of ammo for which the pawn doesn't yet have the corresponding weapon in its inventory */
	UPROPERTY()
	TArray<FStoredAmmo> SavedAmmo;

	UFUNCTION(BlueprintCallable, Category = Pawn)
	virtual void AddAmmo(const FStoredAmmo& AmmoToAdd);

	/** returns whether the character (via SavedAmmo, active weapon, or both) has the maximum allowed ammo for the passed in weapon */
	UFUNCTION(BlueprintCallable, Category = Pawn)
	virtual bool HasMaxAmmo(TSubclassOf<AUTWeapon> Type) const;

	UFUNCTION(BlueprintCallable, Category = Pawn)
	virtual int32 GetAmmoAmount(TSubclassOf<AUTWeapon> Type) const;

	// Cheat, only works if called server side
	void AllAmmo();

	// Cheat, only works if called server side
	void UnlimitedAmmo();

	UPROPERTY(replicated)
	bool bUnlimitedAmmo;

	inline class AUTInventory* GetInventory()
	{
		return InventoryList;
	}

	UFUNCTION(BlueprintCallable, Category = "Pawn")
	virtual void AddInventory(AUTInventory* InvToAdd, bool bAutoActivate);

	UFUNCTION(BlueprintCallable, Category = "Pawn")
	virtual void RemoveInventory(AUTInventory* InvToRemove);

	/** find an inventory item of a specified type */
	UFUNCTION(BlueprintCallable, Category = Pawn)
	virtual AUTInventory* K2_FindInventoryType(TSubclassOf<AUTInventory> Type, bool bExactClass = false) const;

	template<typename InvClass>
	inline InvClass* FindInventoryType(TSubclassOf<InvClass> Type, bool bExactClass = false) const
	{
		InvClass* Result = (InvClass*)K2_FindInventoryType(Type, bExactClass);
		checkSlow(Result == NULL || Result->IsA(InvClass::StaticClass()));
		return Result;
	}

	/** toss an inventory item in the direction the player is facing
	 * (the inventory must have a pickup defined)
	 * ExtraVelocity is in the reference frame of the character (X is forward)
	 */
	UFUNCTION(BlueprintCallable, Category = "Pawn")
	virtual void TossInventory(AUTInventory* InvToToss, FVector ExtraVelocity = FVector::ZeroVector);

	/** discards (generally destroys) all inventory items */
	UFUNCTION(BlueprintCallable, Category = "Pawn")
	virtual void DiscardAllInventory();

	/** call to propagate a named character event (jumping, firing, etc) to all inventory items with bCallOwnerEvent = true */
	UFUNCTION(BlueprintCallable, Category = "Pawn")
	virtual void InventoryEvent(FName EventName);

	/** switches weapons; handles client/server sync, safe to call on either side */
	UFUNCTION(BlueprintCallable, Category = "Pawn")
	virtual void SwitchWeapon(AUTWeapon* NewWeapon);

	inline bool IsPendingFire(uint8 FireMode) const
	{
		return (FireMode < PendingFire.Num() && PendingFire[FireMode] != 0);
	}
	/** blueprint accessor to what firemodes the player currently has active */
	UFUNCTION(BlueprintPure, Category = Weapon)
	bool IsTriggerDown(uint8 FireMode);
	/** sets the pending fire flag; generally should be called by whatever weapon processes the firing command, unless it's an explicit single shot */
	inline void SetPendingFire(uint8 FireMode, bool bNowFiring)
	{
		if (PendingFire.Num() < FireMode + 1)
		{
			PendingFire.SetNumZeroed(FireMode + 1);
		}
		PendingFire[FireMode] = bNowFiring ? 1 : 0;
	}

	inline void ClearPendingFire()
	{
		for (int32 i = 0; i < PendingFire.Num(); i++)
		{
			PendingFire[i] = 0;
		}
	}

	inline AUTWeapon* GetWeapon() const
	{
		return Weapon;
	}
	inline TSubclassOf<AUTWeapon> GetWeaponClass() const
	{
		// debug check to make sure this matches as expected
		checkSlow(GetNetMode() == NM_Client || (Weapon == NULL ? WeaponClass == NULL : ((UObject*)Weapon)->GetClass() == WeaponClass));
		
		return WeaponClass;
	}
	inline AUTWeapon* GetPendingWeapon() const
	{
		return PendingWeapon;
	}

	bool IsInInventory(const AUTInventory* TestInv) const;

	/** called by weapon being put down when it has finished being unequipped. Transition PendingWeapon to Weapon and bring it up 
	 * @param OverflowTime - amount of time past end of timer that previous weapon PutDown() used (due to frame delta) - pass onto BringUp() to keep things in sync
	 */
	virtual void WeaponChanged(float OverflowTime = 0.0f);

	/** called when the client's current weapon has been invalidated (removed from inventory, etc) */
	UFUNCTION(Client, Reliable)
	void ClientWeaponLost(AUTWeapon* LostWeapon);

	/** replicated weapon firing info */
	UPROPERTY(BlueprintReadOnly, Replicated, ReplicatedUsing = FiringInfoUpdated, Category = "Weapon")
	uint8 FlashCount;
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Weapon")
	uint8 FireMode;
	UPROPERTY(BlueprintReadOnly, Replicated, ReplicatedUsing = FiringInfoUpdated, Category = "Weapon")
	FVector_NetQuantize FlashLocation;

	/** set info for one instance of firing and plays firing effects; assumed to be a valid shot - call ClearFiringInfo() if the weapon has stopped firing
	 * if a location is not needed (projectile) call IncrementFlashCount() instead
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void SetFlashLocation(const FVector& InFlashLoc, uint8 InFireMode);

	/** set info for one instance of firing and plays firing effects; assumed to be a valid shot - call ClearFiringInfo() if the weapon has stopped firing
	* if a location is needed (instant hit, beam, etc) call SetFlashLocation() instead
	*/
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void IncrementFlashCount(uint8 InFireMode);

	/** clears firing variables; i.e. because the weapon has stopped firing */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void ClearFiringInfo();

	/** called when firing variables are updated to trigger/stop effects */
	UFUNCTION()
	virtual void FiringInfoUpdated();

	UPROPERTY(BlueprintReadWrite, Category = Pawn, Replicated)
	int32 Health;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pawn)
	int32 HealthMax;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pawn)
	int32 SuperHealthMax;

	/** head bone/socket for headshots */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pawn)
	FName HeadBone;
	/** head Z offset from head bone */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pawn)
	float HeadHeight;
	/** radius around head location that counts as headshot at 1.0 head scaling */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pawn)
	float HeadRadius;
	/** head scale factor (generally for use at runtime) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing = HeadScaleUpdated, Category = Pawn)
	float HeadScale;

	/** multiplier to damage caused by this Pawn */
	UPROPERTY(BlueprintReadWrite, Replicated, Category = Pawn)
	float DamageScaling;
	
	/** accessors to FireRateMultiplier */
	UFUNCTION(BlueprintCallable, Category = Pawn)
	float GetFireRateMultiplier();
	UFUNCTION(BlueprintCallable, Category = Pawn)
	void SetFireRateMultiplier(float InMult);
	UFUNCTION()
	void FireRateChanged();

	UPROPERTY(BlueprintReadWrite, Category = Pawn, Replicated, ReplicatedUsing=PlayTakeHitEffects)
	FTakeHitInfo LastTakeHitInfo;

	/** time of last SetLastTakeHitInfo() - authority only */
	UPROPERTY(BlueprintReadOnly, Category = Pawn)
	float LastTakeHitTime;

	/** indicates character is (mostly) invisible so AI only sees at short range, homing effects can't target the character, etc */
	UPROPERTY(BlueprintReadWrite, Category = Pawn)
	bool bInvisible;

	/** whether spawn protection may potentially be applied (still must meet time since spawn check in UTGameMode)
	 * set to false after firing weapon or any other action that is considered offensive
	 */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Pawn)
	bool bSpawnProtectionEligible;

	/** returns whether spawn protection currently applies for this character (valid on client) */
	UFUNCTION(BlueprintCallable, Category = Damage)
	bool IsSpawnProtected();

	/** set temporarily during client reception of replicated properties because replicated position and switches to ragdoll may be processed out of the desired order 
	 * when set, OnRep_ReplicatedMovement() will be called after switching to ragdoll
	 */
	bool bDeferredReplicatedMovement;

	/** set to prevent firing (does not stop already started firing, call StopFiring() for that) */
	UPROPERTY(BlueprintReadWrite, Category = Pawn)
	bool bDisallowWeaponFiring;

	/** Used to replicate bIsDodgeRolling to non owning clients */
	UPROPERTY(ReplicatedUsing = OnRepDodgeRolling)
	bool bRepDodgeRolling;

	UFUNCTION()
	virtual void OnRepDodgeRolling();

	UFUNCTION(BlueprintCallable, Category = "Pawn")
	virtual EAllowedSpecialMoveAnims AllowedSpecialMoveAnims();

	UFUNCTION(BlueprintCallable, Category = Pawn)
	virtual float GetRemoteViewPitch();

	UPROPERTY(ReplicatedUsing = OnRepDrivenVehicle)
	APawn* DrivenVehicle;

	UFUNCTION()
	virtual void OnRepDrivenVehicle();

	void StartDriving(APawn* Vehicle);

	virtual void BaseChange() override;


	virtual bool IsFeigningDeath();

	// AI hooks
	virtual void OnWalkingOffLedge_Implementation() override;

protected:
	/** set when feigning death or other forms of non-fatal ragdoll (knockdowns, etc) */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = PlayFeignDeath, Category = Pawn)
	bool bFeigningDeath;
	/** set during ragdoll recovery (still blending out physics, playing recover anim, etc) */
	UPROPERTY(BlueprintReadOnly, Category = Pawn)
	bool bInRagdollRecovery;

public:
	inline bool IsRagdoll()
	{
		return bFeigningDeath || bInRagdollRecovery || (RootComponent == Mesh && Mesh->IsSimulatingPhysics());
	}

	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;

	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void Restart() override;

	virtual bool ShouldTakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const override
	{
		return bTearOff || Super::ShouldTakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);
	}

	virtual float TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** returns location of head (origin of headshot zone); will force a skeleton update if mesh hasn't been rendered (or dedicated server) so the provided position is accurate */
	virtual FVector GetHeadLocation();
	/** checks for a head shot - called by weapons with head shot bonuses
	* returns true if it's a head shot, false if a miss or if some armor effect prevents head shots
	* if bConsumeArmor is true, the first item that prevents an otherwise valid head shot will be consumed
	*/
	virtual bool IsHeadShot(FVector HitLocation, FVector ShotDirection, float WeaponHeadScaling, bool bConsumeArmor);

	UFUNCTION(BlueprintCallable, Category = Pawn)
	void SetHeadScale(float NewHeadScale);
	
	/** apply HeadScale to mesh */
	UFUNCTION()
	virtual void HeadScaleUpdated();

	/** sends notification to any other server-side Actors (controller, etc) that need to know about being hit */
	virtual void NotifyTakeHit(AController* InstigatedBy, int32 Damage, FVector Momentum, const FDamageEvent& DamageEvent);

	/** Set LastTakeHitInfo from a damage event and call PlayTakeHitEffects() */
	virtual void SetLastTakeHitInfo(int32 Damage, const FVector& Momentum, AUTInventory* HitArmor, const FDamageEvent& DamageEvent);

	/** blood effects (chosen at random when spawning blood)
	 * note that these are intentionally split instead of a UTImpactEffect because the sound, particles, and decals are all handled with separate logic
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Effects)
	TArray<UParticleSystem*> BloodEffects;
	/** blood decal materials placed on nearby world geometry */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Effects)
	TArray<FBloodDecalInfo> BloodDecals;

	/** trace to nearest world geometry and spawn a blood decal at the hit location (if any) */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = Effects)
	virtual void SpawnBloodDecal(const FVector& TraceStart, const FVector& TraceDir);

	/** last time ragdolling corpse spawned a blood decal */
	UPROPERTY(BlueprintReadWrite, Category = Effects)
	float LastDeathDecalTime;

	/** plays clientside hit effects using the data previously stored in LastTakeHitInfo */
	UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic)
	void PlayTakeHitEffects();

	/** called when we die (generally, by running out of health)
	 *  SERVER ONLY - do not do visual effects here!
	 * return true if we can die, false if immortal (gametype effect, powerup, mutator, etc)
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Pawn)
	virtual bool Died(AController* EventInstigator, const FDamageEvent& DamageEvent);

	virtual void FellOutOfWorld(const UDamageType& DmgType) override;

	/** time between StopRagdoll() call and when physics has been fully blended out of our mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ragdoll)
	float RagdollBlendOutTime;
	/** player in feign death can't recover until world time reaches this */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Ragdoll)
	float FeignDeathRecoverStartTime;

	virtual void StartRagdoll();
	virtual void StopRagdoll();

	UFUNCTION(Exec, BlueprintCallable, Category = Pawn)
	virtual void FeignDeath();
	UFUNCTION(Reliable, Server, WithValidation, Category = Pawn)
	void ServerFeignDeath();
	UFUNCTION()
	virtual void PlayFeignDeath();
	/** force feign death/ragdoll state for e.g. knockdown effects */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Pawn)
	virtual void ForceFeignDeath(float MinRecoveryDelay);

	/** Updates Pawn's rotation to the given rotation, assumed to be the Controller's ControlRotation. Respects the bUseControllerRotation* settings. */
	virtual void FaceRotation(FRotator NewControlRotation, float DeltaTime = 0.f) override;

	/** blood explosion played when gibbing */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects)
	TSubclassOf<class AUTImpactEffect> GibExplosionEffect;
	/** type of gib to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects)
	TSubclassOf<class AUTGib> GibClass;
	/** bones to gib when exploding the entire character (i.e. through GibExplosion()) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Effects)
	TArray<FName> GibExplosionBones;

	/** gibs the entire Pawn and destroys it (only the blood/gibs remain) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic)
	void GibExplosion();
	/** spawns a gib at the specified bone */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = Death)
	virtual void SpawnGib(FName BoneName, TSubclassOf<class UUTDamageType> DmgType = NULL);

	/** plays death effects; use LastTakeHitInfo to do damage-specific death effects */
	virtual void PlayDying();
	virtual void TornOff() override
	{
		PlayDying();
	}

	virtual void DeactivateSpawnProtection();

	virtual void AddDefaultInventory(TArray<TSubclassOf<AUTInventory>> DefaultInventoryToAdd);

	UFUNCTION(BlueprintCallable, Category = Pawn)
	bool IsDead();

	/** weapon firing */
	UFUNCTION(BlueprintCallable, Category = "Pawn")
	virtual void StartFire(uint8 FireModeNum);

	UFUNCTION(BlueprintCallable, Category = "Pawn")
	virtual void StopFire(uint8 FireModeNum);

	UFUNCTION(BlueprintCallable, Category = "Pawn")
	virtual void StopFiring();

	// redirect engine version
	virtual void PawnStartFire(uint8 FireModeNum = 0) override
	{
		StartFire(FireModeNum);
	}

	/** Return true if character is currently able to dodge. */
	UFUNCTION(BlueprintCallable, Category = "Pawn|Character")
	bool CanDodge() const;

	/** Dodge requested by controller, return whether dodge occurred. */
	virtual bool Dodge(FVector DodgeDir, FVector DodgeCross);

	/** Dodge just occured in dodge dir, play any sounds/effects desired.
	 * called on server and owning client
	 */
	UFUNCTION(BlueprintNativeEvent)
	void OnDodge(const FVector &DodgeDir);

	/** Landing assist just occurred */
	UFUNCTION(BlueprintImplementableEvent)
	virtual void OnLandingAssist();

	/** Blueprint override for dodge handling. Rteturn true to skip default dodge in C++. */
	UFUNCTION(BlueprintImplementableEvent)
	bool DodgeOverride(const FVector &DodgeDir, const FVector &DodgeCross);

	virtual bool CanJumpInternal_Implementation() const override;

	virtual void CheckJumpInput(float DeltaTime) override;

	virtual void NotifyJumpApex();

	/** Handles moving forward/backward */
	virtual void MoveForward(float Val);

	/** Handles strafing movement, left and right */
	virtual void MoveRight(float Val);

	/** Handles up and down when swimming or flying */
	virtual void MoveUp(float Val);

	/** Also call UTCharacterMovement ClearJumpInput() */
	virtual void ClearJumpInput() override;

	virtual void MoveBlockedBy(const FHitResult& Impact) override;

	UFUNCTION(BlueprintPure, Category = PlayerController)
	virtual APlayerCameraManager* GetPlayerCameraManager();

	/** particle component for muzzle flash */
	UPROPERTY(EditAnywhere, Category = "Effects")
	TArray< TSubclassOf<class AUTReplicatedEmitter> > TeleportEffect;

	/** plays a footstep effect; called via animation when anims are active (in vis range and not server), otherwise on interval via Tick() */
	UFUNCTION(BlueprintCallable, Category = Effects)
	virtual void PlayFootstep(uint8 FootNum);

	/** play jumping sound/effects; should be called on server and owning client */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = Effects)
	void PlayJump();

	/** Landing at faster than this velocity results in damage (note: use positive number) */
	UPROPERTY(Category = "Falling Damage", EditAnywhere, BlueprintReadWrite)
	float MaxSafeFallSpeed;

	/** amount of falling damage dealt if the player's fall speed is double MaxSafeFallSpeed (scaled linearly from there) */
	UPROPERTY(Category = "Falling Damage", EditAnywhere, BlueprintReadWrite)
	float FallingDamageFactor;

	/** amount of damage dealt to other characters we land on per 100 units of speed */
	UPROPERTY(Category = "Falling Damage", EditAnywhere, BlueprintReadWrite)
	float CrushingDamageFactor;

	/** Blueprint override for take falling damage.  Return true to keep TakeFallingDamage() from causing damage.
		FallingSpeed is the Z velocity at landing, and Hit describes the impacted surface. */
	UFUNCTION(BlueprintImplementableEvent)
	bool HandleFallingDamage(float FallingSpeed, const FHitResult& Hit);

	UFUNCTION(BlueprintCallable, Category = Pawn)
		virtual void TakeFallingDamage(const FHitResult& Hit, float FallingSpeed);

	virtual void Landed(const FHitResult& Hit) override;

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	USoundBase* FootstepSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	USoundBase* LandingSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	USoundBase* JumpSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	USoundBase* DodgeSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	USoundBase* PainSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	USoundBase* WallHitSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	USoundBase* DodgeRollSound;

	/** Ambient sound played while sprinting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	USoundBase* SprintAmbientSound;

	/** Running speed to engage sprint sound */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	float SprintAmbientStartSpeed;
	
	/** Ambient sound played while falling fast */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	USoundBase* FallingAmbientSound;

	/** Falling speed to engage falling sound */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	float FallingAmbientStartSpeed;

	UPROPERTY(BlueprintReadWrite, Category = Sounds)
	float LastPainSoundTime;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sounds)
	float MinPainSoundInterval;

	/** Last time we handled  wall hit for gameplay (damage,sound, etc.) */
	UPROPERTY(BlueprintReadWrite, Category = Sounds)
	float LastWallHitNotifyTime;

	/** Whether can play wall hit sound - true when it hasn't yet been played for this fall */
	UPROPERTY(BlueprintReadWrite, Category = Sounds)
	bool bCanPlayWallHitSound;

	/** sets character overlay material; material must be added to the UTGameState's OverlayMaterials at level startup to work correctly (for replication reasons)
	 * multiple overlays can be active at once, but only one will be displayed at a time
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Effects)
	virtual void SetCharacterOverlay(UMaterialInterface* NewOverlay, bool bEnabled);

	/** uses CharOverlayFlags to apply the desired overlay material (if any) to OverlayMesh */
	UFUNCTION()
	virtual void UpdateCharOverlays();

	/** returns the material instance currently applied to the character's overlay mesh, if any
	 * if not NULL, it is safe to change parameters on this material
	 */
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = Effects)
	virtual UMaterialInstanceDynamic* GetCharOverlayMI();

	/** sets weapon overlay material; material must be added to the UTGameState's OverlayMaterials at level startup to work correctly (for replication reasons)
	 * multiple overlays can be active at once, but the default in the weapon code is to only display one at a time
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Effects)
	virtual void SetWeaponOverlay(UMaterialInterface* NewOverlay, bool bEnabled);

	/** uses WeaponOverlayFlags to apply the desired overlay material (if any) to OverlayMesh */
	UFUNCTION()
	virtual void UpdateWeaponOverlays();

	inline int16 GetCharOverlayFlags()
	{
		return CharOverlayFlags;
	}
	inline int16 GetWeaponOverlayFlags()
	{
		return WeaponOverlayFlags;
	}

	/** sets full body material override
	 * only one at a time is allowed
	 * pass NULL to restore default skin
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Effects)
	virtual void SetSkin(UMaterialInterface* NewSkin);
	inline UMaterialInterface* GetSkin()
	{
		return ReplicatedBodyMaterial;
	}

	/** apply skin in ReplicatedBodyMaterial or restore to default if it's NULL */
	UFUNCTION()
	virtual void UpdateSkin();

	/** timed full body color flash implemented via material parameter */
	UPROPERTY(BlueprintReadOnly, Category = Effects)
	const UCurveLinearColor* BodyColorFlashCurve;
	/** time elapsed in BodyColorFlashCurve */
	UPROPERTY(BlueprintReadOnly, Category = Effects)
	float BodyColorFlashElapsedTime;
	/** set timed body color flash (generally for hit effects)
	 * NOT REPLICATED
	 */
	UFUNCTION(BlueprintCallable, Category = Effects)
	virtual void SetBodyColorFlash(const UCurveLinearColor* ColorCurve, bool bRimOnly);

	/** updates time and sets BodyColorFlash in the character material */
	virtual void UpdateBodyColorFlash(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = Team)
	virtual uint8 GetTeamNum() const;

	UFUNCTION(BlueprintCallable, Category = Team)
	virtual FLinearColor GetTeamColor() const;

	virtual void OnRep_PlayerState() override;
	virtual void NotifyTeamChanged();

	virtual void PlayerChangedTeam();
	virtual void PlayerSuicide();

	//--------------------------
	// Weapon bob and eye offset

	/** Current 1st person weapon deflection due to running bob. */
	UPROPERTY(BlueprintReadWrite, Category = WeaponBob)
	FVector CurrentWeaponBob;

	/** Max 1st person weapon bob deflection with axes based on player view */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	FVector WeaponBobMagnitude;

	/** Z deflection of first person weapon when player jumps */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	FVector WeaponJumpBob;

	/** deflection of first person weapon when player dodges */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	FVector WeaponDodgeBob;

	/** Z deflection of first person weapon when player lands */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	FVector WeaponLandBob;

	/** Desired 1st person weapon deflection due to jumping. */
	UPROPERTY(BlueprintReadWrite, Category = WeaponBob)
	FVector DesiredJumpBob;

	/* Current jump bob (interpolating to DesiredJumpBob)*/
	UPROPERTY(BlueprintReadWrite, Category = WeaponBob)
	FVector CurrentJumpBob;

	/** Time used for weapon bob sinusoids, reset on landing. */
	UPROPERTY(BlueprintReadWrite, Category = WeaponBob)
	float BobTime;

	/** Rate of weapon bob when standing still. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float WeaponBreathingBobRate;

	/** Rate of weapon bob when running. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float WeaponRunningBobRate;

	/** How fast to interpolate to Jump/Land bob offset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float WeaponJumpBobInterpRate;

	/** How fast to decay out Land bob offset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float WeaponLandBobDecayRate;

	/** Get Max eye offset land bob deflection at landing velocity Z of FullEyeOffsetLandBobVelZ+EyeOffsetLandBobThreshold */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float WeaponDirChangeDeflection;

	/** Current Eye position offset from base view position - interpolates toward TargetEyeOffset. */
	UPROPERTY(BlueprintReadWrite, Category = WeaponBob)
	FVector EyeOffset;

	/** Eyeoffset due to crouching transition (not scaled). */
	UPROPERTY(BlueprintReadWrite, Category = WeaponBob)
	FVector CrouchEyeOffset;

	/** Target Eye position offset from base view position. */
	UPROPERTY(BlueprintReadWrite, Category = WeaponBob)
	FVector TargetEyeOffset;

	/** How fast EyeOffset interpolates to TargetEyeOffset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float EyeOffsetInterpRate;

	/** How fast CrouchEyeOffset interpolates to 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float CrouchEyeOffsetInterpRate;

	/** How fast TargetEyeOffset decays. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float EyeOffsetDecayRate;

	/** Jump target view bob magnitude. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float EyeOffsetJumpBob;

	/** Jump Landing target view bob magnitude. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float EyeOffsetLandBob;

	/** Jump Landing target view bob Velocity threshold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float EyeOffsetLandBobThreshold;

	/** Jump Landing target weapon bob Velocity threshold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float WeaponLandBobThreshold;

	/** Get Max weapon land bob deflection at landing velocity Z of FullWeaponLandBobVelZ+WeaponLandBobThreshold */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float FullWeaponLandBobVelZ;

	/** Get Max eye offset land bob deflection at landing velocity Z of FullEyeOffsetLandBobVelZ+EyeOffsetLandBobThreshold */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = WeaponBob)
	float FullEyeOffsetLandBobVelZ;

	/** Get Max weapon land bob deflection at landing velocity Z of FullWeaponLandBobVelZ+WeaponLandBobThreshold */
	UPROPERTY()
	float DefaultBaseEyeHeight;

	/** Broadcast when the pawn has died [Server only] */
	UPROPERTY(BlueprintAssignable)
	FCharacterDiedSignature OnDied;

	/** Max distance for enemy player indicator */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category =  HUD)
	float PlayerIndicatorMaxDistance;

	/** Max distance for same team player indicator */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = HUD)
	float TeamPlayerIndicatorMaxDistance;

	virtual void RecalculateBaseEyeHeight() override;

	/** Returns offset to add to first person mesh for weapon bob. */
	FVector GetWeaponBobOffset(float DeltaTime, AUTWeapon* MyWeapon);

	virtual FVector GetPawnViewLocation() const override;

	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;

	virtual void OnEndCrouch(float HeightAdjust, float ScaledHeightAdjust) override;

	virtual void OnStartCrouch(float HeightAdjust, float ScaledHeightAdjust) override;

	virtual void Crouch(bool bClientSimulation = false) override;

	virtual bool TeleportTo(const FVector& DestLocation, const FRotator& DestRotation, bool bIsATest = false, bool bNoCheck = false) override;
	UFUNCTION()
	virtual void OnOverlapBegin(AActor* OtherActor);
	
	UFUNCTION()
	virtual void OnRagdollCollision(AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	virtual bool CanPickupObject(AUTCarriedObject* PendingObject);
	/** @return the current object carried by this pawn */
	UFUNCTION()
	virtual AUTCarriedObject* GetCarriedObject();

	virtual void PostRenderFor(APlayerController *PC, UCanvas *Canvas, FVector CameraPosition, FVector CameraDir);
protected:

	UFUNCTION(BlueprintNativeEvent, Category = "Pawn|Character|InternalEvents", meta = (FriendlyName = "CanDodge"))
	bool CanDodgeInternal() const;

	/** multiplier to firing speed */
	UPROPERTY(Replicated, ReplicatedUsing=FireRateChanged)
	float FireRateMultiplier;

	/** hook to modify damage taken by this Pawn */
	UFUNCTION(BlueprintNativeEvent)
	void ModifyDamageTaken(int32& Damage, FVector& Momentum, AUTInventory*& HitArmor, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser);
	/** hook to modify damage CAUSED by this Pawn - note that EventInstigator may not be equal to Controller if we're in a vehicle, etc */
	UFUNCTION(BlueprintNativeEvent)
	void ModifyDamageCaused(int32& Damage, FVector& Momentum, const FDamageEvent& DamageEvent, AActor* Victim, AController* EventInstigator, AActor* DamageCauser);

	/** switches weapon locally, must execute independently on both server and client */
	virtual void LocalSwitchWeapon(AUTWeapon* NewWeapon);
	/** RPC to do weapon switch */
	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerSwitchWeapon(AUTWeapon* NewWeapon);
	UFUNCTION(Client, Reliable)
	virtual void ClientSwitchWeapon(AUTWeapon* NewWeapon);
	/** utility to redirect to SwitchToBestWeapon() to the character's Controller (human or AI) */
	void SwitchToBestWeapon();

	/** spawn/destroy/replace the current weapon attachment to represent the equipped weapon (through WeaponClass) */
	UFUNCTION()
	virtual void UpdateWeaponAttachment();

	// firemodes with input currently being held down (pending or actually firing)
	UPROPERTY(BlueprintReadOnly, Category = "Pawn")
	TArray<uint8> PendingFire;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Pawn")
	AUTInventory* InventoryList;

	UPROPERTY(BlueprintReadOnly, Category = "Pawn")
	AUTWeapon* PendingWeapon;

	UPROPERTY(BlueprintReadOnly, Category = "Pawn")
	class AUTWeapon* Weapon;

	UPROPERTY(BlueprintReadOnly, Category = "Pawn")
	class AUTWeaponAttachment* WeaponAttachment;

	UPROPERTY(BlueprintReadOnly, Replicated, ReplicatedUsing=UpdateWeaponAttachment, Category = "Pawn")
	TSubclassOf<AUTWeapon> WeaponClass;

	UPROPERTY(EditAnywhere, Category = "Pawn")
	TArray< TSubclassOf<AUTInventory> > DefaultCharacterInventory;

	//================================
	// Ambient sounds

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=AmbientSoundUpdated, Category = "Pawn")
	USoundBase* AmbientSound;

	UPROPERTY(BlueprintReadOnly, Category = "Pawn")
	UAudioComponent* AmbientSoundComp;

	/** Ambient sound played only on owning client */
	UPROPERTY(BlueprintReadOnly, Category = "Pawn")
	USoundBase* LocalAmbientSound;

	/** Volume of Ambient sound played only on owning client */
	UPROPERTY(BlueprintReadOnly, Category = "Pawn")
	float LocalAmbientVolume;

	UPROPERTY(BlueprintReadOnly, Category = "Pawn")
	UAudioComponent* LocalAmbientSoundComp;

public:
	/** sets replicated ambient (looping) sound on this Pawn
	* only one ambient sound can be set at a time
	* pass bClear with a valid NewAmbientSound to remove only if NewAmbientSound == CurrentAmbientSound
	*/
	UFUNCTION(BlueprintCallable, Category = Audio)
	virtual void SetAmbientSound(USoundBase* NewAmbientSound, bool bClear = false);

	UFUNCTION()
	void AmbientSoundUpdated();

	/** sets local (not replicated) ambient (looping) sound on this Pawn
	* only one ambient sound can be set at a time
	* pass bClear with a valid NewAmbientSound to remove only if NewAmbientSound == CurrentAmbientSound
	*/
	UFUNCTION(BlueprintCallable, Category = Audio)
	virtual void SetLocalAmbientSound(USoundBase* NewAmbientSound, float SoundVolume = 0.f, bool bClear = false);

	UFUNCTION()
	void LocalAmbientSoundUpdated();

	//================================
protected:
	/** last time PlayFootstep() was called, for timing footsteps when animations are disabled */
	float LastFootstepTime;

	/** last FootNum for PlayFootstep(), for alternating when animations are disabled */
	uint8 LastFoot;
	
	/** replicated overlays, bits match entries in UTGameState's OverlayMaterials array */
	UPROPERTY(Replicated, ReplicatedUsing = UpdateCharOverlays)
	uint16 CharOverlayFlags;
	UPROPERTY(Replicated, ReplicatedUsing = UpdateWeaponOverlays)
	uint16 WeaponOverlayFlags;
	/** mesh with current active overlay material on it (created dynamically when needed) */
	UPROPERTY()
	USkeletalMeshComponent* OverlayMesh;

	/** replicated character material override */
	UPROPERTY(Replicated, ReplicatedUsing = UpdateSkin)
	UMaterialInterface* ReplicatedBodyMaterial;

	/** runtime material instance for setting body material parameters (team color, etc) */
	UPROPERTY(BlueprintReadOnly, Category = Pawn)
	UMaterialInstanceDynamic* BodyMI;
public:
	/** legacy command for dropping the flag.  Just redirects to UseCarriedObject */
	UFUNCTION(Exec)
	virtual void DropFlag();
protected:
	/** uses the current carried object */
	UFUNCTION(exec)
	virtual void DropCarriedObject();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerDropCarriedObject();

	/** uses the current carried object */
	UFUNCTION(exec)
	virtual void UseCarriedObject();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerUseCarriedObject();


private:
	void ApplyDamageMomentum(float DamageTaken, FDamageEvent const& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser);
};

inline bool AUTCharacter::IsDead()
{
	return bTearOff || bPendingKillPending;
}


