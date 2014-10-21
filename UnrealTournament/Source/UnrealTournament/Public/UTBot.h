// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "AIController.h"
#include "UTRecastNavMesh.h"

#include "UTBot.generated.h"

USTRUCT(BlueprintType)
struct FBotPersonality
{
	GENERATED_USTRUCT_BODY()

	/** overall skill modifier, generally [-1, +1]
	 * NOTE: this is applied to the bot's Skill property and shouldn't be queried directly
	 */
	UPROPERTY(EditAnywhere, Category = Personality)
	float SkillModifier;
	/** aggressiveness (not a modifier) [-1, 1] */
	UPROPERTY(EditAnywhere, Category = Personality)
	float Aggressiveness;
	/** tactical ability (both a modifier for general tactics and a standalone value for advanced or specialized tactics) [-1, 1] */
	UPROPERTY(EditAnywhere, Category = Personality)
	float Tactics;
	/** likelihood of jumping/dodging, particularly in combat */
	UPROPERTY(EditAnywhere, Category = Personality)
	float Jumpiness;
	/** reaction time (skill modifier) [-1, +1], positive is better (lower reaction time)
	 * affects enemy acquisition and incoming fire avoidance
	 */
	UPROPERTY(EditAnywhere, Category = Personality)
	float ReactionTime;
	/** modifier to aim accuracy (skill modifier) [-1, +1] */
	UPROPERTY(EditAnywhere, Category = Personality)
	float Accuracy;
	/** modifies likelihood of bot detecting stimulus, particularly at long ranges and/or edges of vision (skill modifier) [-1, +1] */
	UPROPERTY(EditAnywhere, Category = Personality)
	float Alertness;
	/** favorite weapon; bot will bias towards acquiring and using this weapon */
	UPROPERTY(EditAnywhere, Category = Personality)
	FName FavoriteWeapon;
};

USTRUCT(BlueprintType)
struct FBotCharacter : public FBotPersonality
{
	GENERATED_USTRUCT_BODY()

	/** bot's name */
	UPROPERTY(EditAnywhere, Category = Display)
	FString PlayerName;
	
	// TODO: audio/visual details (mesh, voice pack, etc)

	/** transient runtime tracking of how many times this entry has been used to avoid unnecessary duplicates */
	UPROPERTY(Transient, BlueprintReadWrite, Category = Game)
	uint8 SelectCount;
};

struct UNREALTOURNAMENT_API FBestInventoryEval : public FUTNodeEvaluator
{
	float BestWeight;
	AActor* BestPickup;

	virtual float Eval(APawn* Asker, const FNavAgentProperties& AgentProps, const UUTPathNode* Node, const FVector& EntryLoc, int32 TotalDistance) override;
	virtual bool GetRouteGoal(AActor*& OutGoal, FVector& OutGoalLoc) const override;

	FBestInventoryEval()
		: BestWeight(0.0f), BestPickup(NULL)
	{}
};
struct UNREALTOURNAMENT_API FRandomDestEval : public FUTNodeEvaluator
{
	virtual float Eval(APawn* Asker, const FNavAgentProperties& AgentProps, const UUTPathNode* Node, const FVector& EntryLoc, int32 TotalDistance) override
	{
		return (TotalDistance > 0) ? FMath::FRand() * 1.5f : 0.1f;
	}
};

UENUM()
enum EAIEnemyUpdateType
{
	EUT_Seen, // bot saw enemy
	EUT_HeardExact, // bot heard enemy close enough/distinct enough to know exact location
	EUT_HeardApprox, // bot heard enemy but isn't sure exactly where that is
	EUT_TookDamage, // bot got hit by enemy
	EUT_DealtDamage, // bot hit enemy
};

USTRUCT()
struct FBotEnemyInfo
{
	GENERATED_USTRUCT_BODY()

protected:
	/** enemy Pawn */
	UPROPERTY()
	APawn* Pawn;
	/** cached cast */
	UPROPERTY()
	AUTCharacter* UTChar;
public:
	/** the AI's view of the enemy's effective health (multiplier of default health, includes armor if AI has seen an armor indicator) */
	UPROPERTY()
	float EffectiveHealthPct;
	/** if true EffectiveHealthPct is exact value as of LastFullUpdateTime */
	UPROPERTY()
	bool bHasExactHealth;
	/** last location we know the enemy was at */
	UPROPERTY()
	FVector LastKnownLoc;
	/** last location we saw the enemy at */
	UPROPERTY()
	FVector LastSeenLoc;
	/** location of player that last saw this enemy */
	UPROPERTY()
	FVector LastSeeingLoc;
	/** last time the enemy was seen */
	UPROPERTY()
	float LastSeenTime;
	/** last time we were aware of something about the enemy in a way that tells us their exact location (seen, short range audio, etc) */
	UPROPERTY()
	float LastFullUpdateTime;
	/** last time we were hit by this enemy (or last time this enemy hit anyone on the team, for team lists) */
	UPROPERTY()
	float LastHitByTime;
	/** last time we got any update about this enemy, including those that don't give us their location */
	UPROPERTY()
	float LastUpdateTime;
	/** only set for bot enemy list (not team list) - indicates bot has discarded this enemy, don't pick it again without an update */
	UPROPERTY()
	bool bLostEnemy;

	void Update(EAIEnemyUpdateType UpdateType, const FVector& ViewerLoc = FVector::ZeroVector);

	inline APawn* GetPawn() const
	{
		return Pawn;
	}
	inline AUTCharacter* GetUTChar() const
	{
		return UTChar;
	}
	/** returns if this entry still points to a valid Enemy
	 * if TeamHolder is passed, returns false if Enemy is on same team as TeamHolder, otherwise no team check
	 */
	bool IsValid(AActor* TeamHolder = NULL) const;

	/** returns if this enemy was seen recently enough that we can assume they're still visible */
	bool IsCurrentlyVisible(float WorldTime) const
	{
		return (WorldTime - LastSeenTime) < 0.25f; // max sight interval in UTBot
	}

	FBotEnemyInfo()
		: Pawn(NULL), UTChar(NULL), EffectiveHealthPct(1.0f), bHasExactHealth(false), LastSeenTime(-100000.0f), LastFullUpdateTime(-100000.0f), LastUpdateTime(-100000.0f), bLostEnemy(false)
	{}
	FBotEnemyInfo(APawn* InPawn, EAIEnemyUpdateType UpdateType, const FVector& ViewerLoc = FVector::ZeroVector)
		: Pawn(InPawn), UTChar(Cast<AUTCharacter>(InPawn)), EffectiveHealthPct(1.0f), bHasExactHealth(false), LastSeenTime(-100000.0f), LastFullUpdateTime(-100000.0f), LastUpdateTime(-100000.0f), bLostEnemy(false)
	{
		Update(UpdateType, ViewerLoc);
	}
};

USTRUCT()
struct FBotEnemyRating
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString PlayerName;
	UPROPERTY()
	float Rating;

	FBotEnemyRating()
	{}
	FBotEnemyRating(APawn* InEnemy, float InRating)
		: PlayerName((InEnemy != NULL && InEnemy->PlayerState != NULL) ? InEnemy->PlayerState->PlayerName : GetNameSafe(InEnemy)), Rating(InRating)
	{}
};

UCLASS()
class UNREALTOURNAMENT_API AUTBot : public AAIController, public IUTTeamInterface
{
	GENERATED_UCLASS_BODY()

	/** bot considers a height difference of greater than this to be a relevant combat advantage to the higher player */
	UPROPERTY(EditDefaultsOnly, Category = Environment)
	float TacticalHeightAdvantage;

	UPROPERTY(BlueprintReadWrite, Category = Personality)
	FBotPersonality Personality;
	/** core skill rating, generally 0 - 7 */
	UPROPERTY(BlueprintReadWrite, Category = Skill)
	float Skill;
	/** reaction time (in real seconds) for enemy positional tracking (i.e. use enemy's position this far in the past as basis) */
	UPROPERTY(BlueprintReadWrite, Category = Skill)
	float TrackingReactionTime;
	/** maximum vision range in UU */
	UPROPERTY(BlueprintReadWrite, Category = Skill)
	float SightRadius;
	/** vision angle (cosine, compared against dot product of target dir to view dir) */
	UPROPERTY(BlueprintReadWrite, Category = Skill)
	float PeripheralVision;
	/** if set bot will have a harder time noticing and targeting enemies that are significantly above or below */
	UPROPERTY(BlueprintReadWrite, Category = Skill)
	bool bSlowerZAcquire;
	/** if set bot will lead targets with projectile weapons */
	UPROPERTY(BlueprintReadWrite, Category = Skill)
	bool bLeadTarget;

	/** rotation rate towards focal point */
	UPROPERTY(BlueprintReadOnly, Category = Skill)
	FRotator RotationRate;

	/** whether to call SeePawn() for friendlies */
	UPROPERTY(BlueprintReadWrite, Category = AI)
	bool bSeeFriendly;

	/** aggression value for most recent combat action after all personality/enemy strength/squad/weapon modifiers */
	UPROPERTY()
	float CurrentAggression;

	/** debugging string set during decision logic */
	UPROPERTY()
	FString GoalString;

private:
	/** current action, if any */
	UPROPERTY()
	class UUTAIAction* CurrentAction;
	/** node or Actor bot is currently moving to */
	UPROPERTY()
	FRouteCacheItem MoveTarget;
protected:
	/** path link that connects currently occupied node to MoveTarget */
	UPROPERTY()
	FUTPathLink CurrentPath;
	/** cache of internal points for current MoveTarget */
	TArray<FComponentBasedPosition> MoveTargetPoints;
	/** last reached MoveTargetPoint in current move (only valid while MoveTarget.IsValid())
	 * this is used when adjusting around obstacles
	 */
	UPROPERTY()
	FVector LastReachedMovePoint;
	/** true when moving to AdjustLoc instead of MoveTarget/MoveTargetPoints */
	UPROPERTY()
	bool bAdjusting;
	/** when bAdjusting is true, temporary intermediate move point that bot uses to get more on path or around minor avoidable obstacles */
	UPROPERTY()
	FVector AdjustLoc;
public:
	inline const FRouteCacheItem& GetMoveTarget() const
	{
		return MoveTarget;
	}
	/** get next point bot is moving to in order to reach MoveTarget, or ZeroVector if no MoveTarget */
	inline FVector GetMovePoint() const
	{
		if (!MoveTarget.IsValid())
		{
			return FVector::ZeroVector;
		}
		else if (bAdjusting)
		{
			return AdjustLoc;
		}
		else if (MoveTargetPoints.Num() <= 1)
		{
			return MoveTarget.GetLocation(GetPawn());
		}
		else
		{
			return MoveTargetPoints[0].Get();
		}
	}
	void SetMoveTarget(const FRouteCacheItem& NewMoveTarget, const TArray<FComponentBasedPosition>& NewMovePoints = TArray<FComponentBasedPosition>());
	/** set move target and force direct move to that point (don't query the navmesh) */
	inline void SetMoveTargetDirect(const FRouteCacheItem& NewMoveTarget)
	{
		TArray<FComponentBasedPosition> NewMovePoints;
		NewMovePoints.Add(FComponentBasedPosition(NewMoveTarget.GetLocation(GetPawn())));
		SetMoveTarget(NewMoveTarget, NewMovePoints);
		if (GetCharacter() != NULL)
		{
			MoveTimer = 1.0f + (NewMoveTarget.GetLocation(GetPawn()) - GetPawn()->GetActorLocation()).Size() / GetCharacter()->CharacterMovement->MaxWalkSpeed;
		}
	}
	inline void ClearMoveTarget()
	{
		MoveTarget.Clear();
		MoveTargetPoints.Empty();
		bAdjusting = false;
		CurrentPath = FUTPathLink();
		MoveTimer = -1.0f;
	}
	inline const FUTPathLink& GetCurrentPath() const
	{
		return CurrentPath;
	}
	inline UUTAIAction* GetCurrentAction() const
	{
		return CurrentAction;
	}
	/** change current action - can be NULL to set no action (bot will call WhatToDoNext() during its tick to try to acquire a new action)
	 * calling with an action in progress will reset it (calls Ended() and then Started())
	 */
	virtual void StartNewAction(UUTAIAction* NewAction);
	/** time remaining until giving up on a move in progress */
	UPROPERTY()
	float MoveTimer;
	/** cache of last found route */
	UPROPERTY()
	TArray<FRouteCacheItem> RouteCache;

	/** evaluate enemy list (on Team if available, otherwise bot's personal list) and pick best enemy to focus on */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = AI)
	virtual void PickNewEnemy();
	/** sets current enemy
	 * NOTE: this ignores enemy ratings and forces the enemy, call PickNewEnemy() to go through the normal process
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = AI)
	virtual void SetEnemy(APawn* NewEnemy);
	inline APawn* GetEnemy() const
	{
		return Enemy;
	}
	/** set target that the bot will shoot instead of Enemy (game objectives and the like) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = AI)
	virtual void SetTarget(AActor* NewTarget);
	inline AActor* GetTarget() const
	{
		return (Target != NULL) ? Target : Enemy;
	}

	/** updates some or all of bot's information on the passed in enemy based on the update type */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = AI)
	virtual void UpdateEnemyInfo(APawn* NewEnemy, EAIEnemyUpdateType UpdateType);

	/** fire mode bot wants to use for next shot; this is determined early so bot can decide whether to lead, etc */
	UPROPERTY()
	uint8 NextFireMode;

	/** notification of incoming projectile that is reasonably likely to be targeted at this bot
	 * used to prepare evasive actions, if bot sees it coming and its reaction time is good enough
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = AI)
	virtual void ReceiveProjWarning(class AUTProjectile* Incoming);
	/** notification of incoming instant hit shot that is reasonably likely to have targeted this bot
	 * used to prepare evasive actions for NEXT shot (kind of late for current shot) if bot is aware enough
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = AI)
	virtual void ReceiveInstantWarning(AUTCharacter* Shooter, const FVector& FireDir);
	/** sets timer for ProcessIncomingWarning() as appropriate for the bot's skill, etc
	 * pass a shooter with NULL for projectile for instant hit warnings
	 */
	virtual void SetWarningTimer(AUTProjectile* Incoming, AUTCharacter* Shooter, float TimeToImpact);
protected:
	/** projectile for ProcessIncomingWarning() call on a pending timer */
	UPROPERTY()
	AUTProjectile* WarningProj;
	/** shooter for ProcessIncomingWarning() call on a pending timer */
	UPROPERTY()
	AUTCharacter* WarningShooter;

	/** called on a timer to react to expected incoming weapons fire, either projectile in flight or about to shoot instant hit */
	UFUNCTION()
	virtual void ProcessIncomingWarning();

	/** enemies bot has personally received updates on
	 * team list is authoritative on known info but this is used to differentiate between "my team has told me this" and "I have personally seen this"
	 * which can be relevant in enemy ratings depending on the game and bot personality
	 */
	UPROPERTY()
	TArray<FBotEnemyInfo> LocalEnemyList;

	/** debugging for last enemy selection */
	UPROPERTY()
	float LastPickEnemyTime;
	UPROPERTY()
	TArray<FBotEnemyRating> LastPickEnemyRatings;
private:
	UPROPERTY()
	AUTCharacter* UTChar;
	/** squad for game and objective specific logic */
	UPROPERTY()
	class AUTSquadAI* Squad;
protected:
	/** cached reference to navigation network */
	UPROPERTY()
	AUTRecastNavMesh* NavData;
	/** current enemy, generally also Target unless there's a game objective or other special target */
	UPROPERTY(BlueprintReadOnly, Category = AI)
	APawn* Enemy;
	/** last time Enemy was set, used for acquisition/reaction time calculations */
	UPROPERTY(BlueprintReadOnly, Category = AI)
	float LastEnemyChangeTime;
	/** weapon target, automatically set to Enemy if NULL but can be set to non-Pawn destructible objects (shootable triggers, game objectives, etc) */
	UPROPERTY(BlueprintReadOnly, Category = AI)
	AActor* Target;
	/** set to force selection of new fire mode next frame for weapon targeting */
	bool bPickNewFireMode;
	/** valid only when target is set to a class that supports position history; set in UpdateControlRotation() to target velocity being used by aiming logic (generally, its velocity from a short time in the past) */
	FVector TrackedVelocity;
	/** when leading with projectiles and predicted shot is blocked we iteratively refine over several frames */
	float LastIterativeLeadCheck;
	/** aim target where projectile leading was blocked, so continue tracing each frame looking for best target location */
	UPROPERTY()
	AActor* BlockedAimTarget;
	/** applying checks for splash damage, headshots, toss adjust, etc are done at a reduced rate for performance */
	float TacticalAimUpdateInterval;
	float LastTacticalAimUpdateTime;
	/** last result of tactical aim update applied to predicted target loc in subsequent frames until next update */
	FVector TacticalAimOffset;
	/** focal point after aim tracking and weapon targeting */
	UPROPERTY()
	FVector FinalFocalPoint;

	/** AI actions */
	UPROPERTY()
	TSubobjectPtr<UUTAIAction> WaitForMoveAction;
	UPROPERTY()
	TSubobjectPtr<UUTAIAction> WaitForLandingAction;
	UPROPERTY()
	TSubobjectPtr<UUTAIAction> RangedAttackAction;
	UPROPERTY()
	TSubobjectPtr<UUTAIAction> TacticalMoveAction;
	UPROPERTY()
	TSubobjectPtr<UUTAIAction> ChargeAction;

public:
	inline AUTCharacter* GetUTChar() const
	{
		return UTChar;
	}
	inline AUTSquadAI* GetSquad() const
	{
		return Squad;
	}
	virtual void SetSquad(AUTSquadAI* NewSquad);

	/** set when planning on wall dodging next time we hit a wall during current fall */
	bool bPlannedWallDodge;

	/** sets base bot skill and all parameters derived from skill */
	virtual void InitializeSkill(float NewBaseSkill);
	/** set PeripheralVision based on skill and controlled Pawn */
	virtual void SetPeripheralVision();

	virtual void SetPawn(APawn* InPawn) override;
	virtual void Possess(APawn* InPawn) override;
	virtual void PawnPendingDestroy(APawn* InPawn) override;
	virtual void Destroyed() override;
	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true) override;
	/** apply any adjustments to our aim based on the weapon we're using (projectile leading, toss, splash damage, headshots, etc)
	 * TrackedVelocity has already been set to the velocity of the target the bot should use for prediction (not necessarily its current velocity due to the history-based aiming model)
	 * @param TargetLoc - predicted target location
	 * @param FocalPoint (in/out) - the point the bot should look at to fire the weapon in the desired direction (initial value is from weapon's CanAttack(), generally TargetLoc unless weapon applied special indirect aiming like bouncing around a corner)
	 */
	virtual void ApplyWeaponAimAdjust(FVector TargetLoc, FVector& FocalPoint);
	virtual void Tick(float DeltaTime) override;
	virtual void UTNotifyKilled(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, const UDamageType* DamageType);
	virtual FVector GetFocalPoint() const
	{
		return FinalFocalPoint;
	}

	virtual uint8 GetTeamNum() const;

	// UTCharacter notifies
	virtual void NotifyWalkingOffLedge();
	virtual void NotifyMoveBlocked(const FHitResult& Impact);
	virtual void NotifyLanded(const FHitResult& Hit);

	// causes the bot decision logic to be run within one frame
	virtual void WhatToDoNext();

	virtual bool CanSee(APawn* Other, bool bMaySkipChecks);
	virtual bool LineOfSightTo(const class AActor* Other, FVector ViewPoint = FVector(ForceInit), bool bAlternateChecks = false) const override;
	// UT version allows specifying an alternate root loc for Other while still supporting checking head, sides, etc - used often when AI is guessing where a target is
	virtual bool UTLineOfSightTo(const AActor* Other, FVector ViewPoint = FVector(ForceInit), bool bAlternateChecks = false, FVector TargetLocation = FVector::ZeroVector) const;
	virtual void SeePawn(APawn* Other);

	virtual void NotifyTakeHit(AController* InstigatedBy, int32 Damage, FVector Momentum, const FDamageEvent& DamageEvent);

	/** called by timer and also by weapons when ready to fire. Bot sets appropriate fire mode it wants to shoot (if any) */
	virtual void CheckWeaponFiring(bool bFromWeapon = true);
	/** return if bot thinks it needs to turn to attack TargetLoc (intentionally inaccurate for low skill bots) */
	virtual bool NeedToTurn(const FVector& TargetLoc);

	/** find pickup with distance modified rating greater than value passed in
	 * handles avoiding redundant searches in the same frame so it isn't necessary to manually throttle
	 */
	virtual bool FindInventoryGoal(float MinWeight);

	/** tries to perform an evasive action in the indicated direction, most commonly a dodge but if dodge is not available or low skill, possibly strafe that way instead
	 * this function may interrupt the bot's current action
	 */
	virtual bool TryEvasiveAction(FVector DuckDir);

	/** if bot should defend a specific point, set the appropriate move or camp action
	 * @return whether an action was set
	 */
	virtual bool ShouldDefendPosition();

	void SwitchToBestWeapon();
	/** rate the passed in weapon (must be owned by this bot) */
	virtual float RateWeapon(AUTWeapon* W);
	/** returns whether the passed in class is the bot's favorite weapon (bonus to desire to pick up, select, and improved accuracy/effectiveness) */
	virtual bool IsFavoriteWeapon(TSubclassOf<AUTWeapon> TestClass);
	/** returns whether bot really wants to find a better weapon (i.e. because have none with ammo or current is a weak starting weapon) */
	virtual bool NeedsWeapon();

	virtual void DisplayDebug(class UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;

	/** rating for enemy to focus on
	 * bot targets enemy with highest rating
	 */
	virtual float RateEnemy(const FBotEnemyInfo& EnemyInfo);
	/** returns a value indicating the relative strength of other
	 * > 0 means other is stronger than controlled pawn
	 */
	virtual float RelativeStrength(APawn* Other);

	/** decide best approach to attack current Enemy */
	virtual void ChooseAttackMode();
	/** pick a retreat tactic/destination */
	virtual void DoRetreat();
	/** core enemy attack logic */
	virtual void FightEnemy(bool bCanCharge, float EnemyStrength);
	/** set action to charge current enemy */
	virtual void DoCharge();
	/** start an appropriate action to perform in-combat maneuvers (strafe, get in better shooting position, dodge, etc) */
	virtual void DoTacticalMove();
	/** do a stationary (or minor strafing, if skilled enough) attack on the given target. Priority is accuracy, not evasion */
	virtual void DoRangedAttackOn(AActor* NewTarget);

	// action accessors
	inline void StartWaitForMove()
	{
		StartNewAction(WaitForMoveAction);
	}

	/** convenience redirect to AUTWeapon::CanAttack(), see that for details */
	virtual bool CanAttack(AActor* Target, const FVector& TargetLoc, bool bDirectOnly, bool bPreferCurrentMode = false, uint8* BestFireMode = NULL, FVector* OptimalTargetLoc = NULL);

	/** check for line of sight to target DeltaTime from now */
	virtual bool CheckFutureSight(float DeltaTime);

	/** get info on enemy, from team if available or local list if not
	 * returned pointer is from an array so it is only guaranteed valid until next enemy update
	 */
	const FBotEnemyInfo* GetEnemyInfo(APawn* TestEnemy, bool bCheckTeam);
	/** returns where bot thinks enemy is
	 * if bAllowPrediction, allow AI to guess at enemy's current position if it doesn't have line of sight (if false, return last known location)
	 * if the bot nor its teammates have ever contacted this enemy, then it returns ZeroVector
	 */
	virtual FVector GetEnemyLocation(APawn* TestEnemy, bool bAllowPrediction);
	/** return if the passed in Enemy is visible to this bot
	 * NOTE: this is a cached value, taken from enemy list; it does not trace
	 */
	virtual bool IsEnemyVisible(APawn* TestEnemy);
	/** return if the bot has other visible enemies than Enemy */
	virtual bool HasOtherVisibleEnemy();

	/** returns true if we haven't noted any update from Enemy in the specified amount of time
	 * NOTE: SetEnemy() counts as an update for purposes of this function
	 */
	virtual bool LostContact(float MaxTime);

	/** return if bot is stopped and isn't planning on moving, used for some decisions and for certain skill/accuracy checks */
	bool IsStopped() const
	{
		return !MoveTarget.IsValid() && GetPawn() != NULL && GetPawn()->GetVelocity().IsZero() && (GetCharacter() == NULL || GetCharacter()->CharacterMovement->GetCurrentAcceleration().IsZero());
	}

	/** return if bot is currently charging (moving towards in an attack pose) its current target */
	virtual bool IsCharging() const
	{
		return ((GetTarget() != NULL && MoveTarget.Actor == GetTarget()) || CurrentAction == ChargeAction);
	}

protected:
	/** timer to call CheckWeaponFiring() */
	UFUNCTION()
	void CheckWeaponFiringTimed();

	virtual void ExecuteWhatToDoNext();
	bool bPendingWhatToDoNext;
	/** set during ExecuteWhatToDoNext() to catch decision loops */
	bool bExecutingWhatToDoNext;

	/** used to interleave sight checks so not all bots are checking at once */
	float SightCounter;

	/** FindInventoryGoal() transients */
	float LastFindInventoryTime;
	float LastFindInventoryWeight;
};