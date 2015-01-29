// Squad AI contains gametype and role specific AI for bots
// for example, an attacker in CTF gets a different Squad than a defender in CTF
// which is different from a defender in Warfare
// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTBot.h"
#include "UTTeamInfo.h"

#include "UTSquadAI.generated.h"

struct UNREALTOURNAMENT_API FSuperPickupEval : public FBestInventoryEval
{
	/** threshold to consider a pickup sufficiently "super" */
	float MinDesireability;
	/** list of pickups to ignore because a teammate has a claim on them */
	TArray<AActor*> ClaimedPickups;

	virtual bool AllowPickup(APawn* Asker, AActor* Pickup, float Desireability, float PickupDist);

	FSuperPickupEval(float InPredictionTime, float InMoveSpeed, int32 InMaxDist = 0, float InMinDesireability = 1.0f, const TArray<AActor*>& InClaimedPickups = TArray<AActor*>())
		: FBestInventoryEval(InPredictionTime, InMoveSpeed, InMaxDist), MinDesireability(InMinDesireability), ClaimedPickups(InClaimedPickups)
	{}
};

extern FName NAME_Attack;
extern FName NAME_Defend;

UCLASS(NotPlaceable)
class AUTSquadAI : public AInfo, public IUTTeamInterface
{
	GENERATED_UCLASS_BODY()

	friend class AUTTeamInfo;

	/** team this squad is on (may be NULL) */
	UPROPERTY(BlueprintReadOnly, Category = Squad)
	AUTTeamInfo* Team;
	/** squad role/orders */
	UPROPERTY(BlueprintReadOnly, Category = Squad)
	FName Orders;
protected:
	/** cached pointer to navigation data */
	AUTRecastNavMesh* NavData;

	/** list of squad members */
	UPROPERTY(BlueprintReadOnly, Category = Squad)
	TArray<AController*> Members;
	/** leader (prefer to stay near this player when objectives permit)*/
	UPROPERTY(BlueprintReadOnly, Category = Squad)
	AController* Leader;
	/** squad objective (target to attack, defend, etc) - may be NULL */
	UPROPERTY(BlueprintReadOnly, Category = Squad)
	AActor* Objective;
	/** precast to AUTGameObjective for convenience */
	UPROPERTY(BlueprintReadOnly, Category = Squad)
	AUTGameObjective* GameObjective;
public:

	inline AController* GetLeader() const
	{
		return Leader;
	}
	inline int32 GetSize() const
	{
		return Members.Num();
	}
	inline AActor* GetObjective() const
	{
		return Objective;
	}
	inline AUTGameObjective* GetGameObjective() const
	{
		return GameObjective;
	}

	virtual uint8 GetTeamNum() const override
	{
		return (Team != NULL) ? Team->TeamIndex : 255;
	}
	// not applicable
	virtual void SetTeamForSideSwap_Implementation(uint8 NewTeamNum) override
	{}

	virtual void BeginPlay() override
	{
		NavData = GetUTNavData(GetWorld());
		Super::BeginPlay();
	}

	virtual void Initialize(AUTTeamInfo* InTeam, FName InOrders)
	{
		Team = InTeam;
		Orders = InOrders;
	}

	virtual void SetObjective(AActor* InObjective)
	{
		if (InObjective != Objective)
		{
			Objective = InObjective;
			GameObjective = Cast<AUTGameObjective>(InObjective);
			for (AController* C : Members)
			{
				AUTBot* B = Cast<AUTBot>(C);
				if (B != NULL)
				{
					B->WhatToDoNext();
				}
			}
		}
	}

	virtual void AddMember(AController* C);
	virtual void RemoveMember(AController* C);
	virtual void SetLeader(AController* NewLeader);

	/** @return if enemy is important to track for as long as possible (e.g. threatening game objective) */
	virtual bool MustKeepEnemy(APawn* TheEnemy)
	{
		return false;
	}

	/** @return if allowed to use translocator */
	virtual bool AllowTranslocator(AUTBot* B)
	{
		return true;
	}

	/** called when bot lost track of enemy and wants a new one. Assigning one is optional.
	 * @return whether a new enemy was assigned
	 */
	virtual bool LostEnemy(AUTBot* B);

	/** @return modified rating for enemy, taking into account objectives */
	virtual float ModifyEnemyRating(float CurrentRating, const FBotEnemyInfo& EnemyInfo, AUTBot* B)
	{
		return CurrentRating;
	}

	/** modify the bot's attack aggressiveness, generally in response to its target's relevance to game objectives */
	virtual void ModifyAggression(AUTBot* B, float& Aggressiveness)
	{
		if (MustKeepEnemy(B->GetEnemy()))
		{
			Aggressiveness += 0.5f;
		}
	}

	/** return current orders for this bot
	 * generally just returns Orders, but for certain squads (e.g. freelance) this may change based on the bot's state (for example, when stacked switch to attacking)
	 */
	virtual FName GetCurrentOrders(AUTBot* B)
	{
		return Orders;
	}

	/** return true if the bot B has a better claim on the given pickup than the current claiming pawn (so this bot may consider taking it instead) */
	virtual bool HasBetterPickupClaim(AUTBot* B, const FPickupClaim& Claim);

	/** checks for any super pickups (powerups, strong armor, etc) that the AI should focus on above any game objectives right now (or AS the game objectives, such as in DM/TDM)
	 * also handles the possibility of teammate(s) already headed to a particular pickup so this bot shouldn't consider it
	 */
	virtual bool CheckSuperPickups(AUTBot* B, int32 MaxDist);

	/** called by bot during its decision logic to see if there's an action relating to the game's objectives it should take
	 * @return if an action was assigned
	 */
	virtual bool CheckSquadObjectives(AUTBot* B);

	/** called in bot fighting logic to ask for destination when bot wants to retreat
	 * should set MoveTarget to desired destination but don't set action
	 */
	virtual bool PickRetreatDestination(AUTBot* B);

	/** notifies AI of some game objective related event (e.g. flag taken)
	 * generally bots will get retasked if they were performing some action relating to the object that is no longer relevant
	 */
	virtual void NotifyObjectiveEvent(AActor* InObjective, AController* InstigatedBy, FName EventName);

	/** return whether the given bot should consider the squad objective as higher than normal priority and minimize unnecessary detours
	 * (e.g. in CTF when flag is out)
	 */
	virtual bool HasHighPriorityObjective(AUTBot* B)
	{
		return false;
	}
};