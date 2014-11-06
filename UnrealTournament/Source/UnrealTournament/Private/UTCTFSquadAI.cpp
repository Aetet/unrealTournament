// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTCTFSquadAI.h"
#include "UTCTFFlag.h"

AUTCTFSquadAI::AUTCTFSquadAI(const FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
}

void AUTCTFSquadAI::Initialize(AUTTeamInfo* InTeam, FName InOrders)
{
	Super::Initialize(InTeam, InOrders);

	for (TActorIterator<AUTGameObjective> It(GetWorld()); It; ++It)
	{
		if (It->GetTeamNum() != GetTeamNum())
		{
			EnemyBase = *It;
		}
		else
		{
			FriendlyBase = *It;
		}
	}
	// initial objective - if attacking, enemy base; if defending, my base
	if (Orders == NAME_Attack)
	{
		SetObjective(EnemyBase);
	}
	else if (Orders == NAME_Defend)
	{
		SetObjective(FriendlyBase);
	}
}

bool AUTCTFSquadAI::MustKeepEnemy(APawn* TheEnemy)
{
	// must keep enemy flag holder
	AUTCharacter* UTC = Cast<AUTCharacter>(TheEnemy);
	return (UTC != NULL && UTC->GetCarriedObject() != NULL && UTC->GetCarriedObject()->GetTeamNum() == GetTeamNum());
}

bool AUTCTFSquadAI::IsNearEnemyBase(const FVector& TestLoc)
{
	return EnemyBase != NULL && (TestLoc - EnemyBase->GetActorLocation()).Size() < 4500.0f;
}

float AUTCTFSquadAI::ModifyEnemyRating(float CurrentRating, const FBotEnemyInfo& EnemyInfo, AUTBot* B)
{
	if ( EnemyInfo.GetUTChar() != NULL && EnemyInfo.GetUTChar()->GetCarriedObject() != NULL && EnemyInfo.GetUTChar()->GetCarriedObject()->GetTeamNum() == GetTeamNum() &&
		 B->CanAttack(EnemyInfo.GetPawn(), EnemyInfo.LastKnownLoc, false) )
	{
		if ( (B->GetPawn()->GetActorLocation() - EnemyInfo.LastKnownLoc).Size() < 3500.0f || (B->GetUTChar() != NULL && B->GetUTChar()->GetWeapon() != NULL && B->GetUTChar()->GetWeapon()->bSniping) ||
			IsNearEnemyBase(EnemyInfo.LastKnownLoc) )
		{
			return CurrentRating + 6.0f;
		}
		else
		{
			return CurrentRating + 1.5f;
		}
	}
	else
	{
		return CurrentRating;
	}
}

bool AUTCTFSquadAI::SetFlagCarrierAction(AUTBot* B)
{
	if (FriendlyBase != NULL && FriendlyBase->GetCarriedObjectState() != CarriedObjectState::Home && (HideTarget.IsValid() || (B->GetPawn()->GetActorLocation() - FriendlyBase->GetActorLocation()).Size() < 3000.0f))
	{
		float EnemyStrength = B->RelativeStrength(B->GetEnemy());
		if (B->GetEnemy() != NULL && EnemyStrength < 0.0f)
		{
			// fight enemy
			return false;
		}
		else if (HideTarget.IsValid() && !NavData->HasReachedTarget(B->GetPawn(), *B->GetPawn()->GetNavAgentProperties(), HideTarget))
		{
			FSingleEndpointEval NodeEval(HideTarget.GetLocation(NULL));
			float Weight = 0.0f;
			if (NavData->FindBestPath(B->GetPawn(), *B->GetPawn()->GetNavAgentProperties(), NodeEval, B->GetNavAgentLocation(), Weight, true, B->RouteCache))
			{
				B->GoalString = "Hide";
				B->SetMoveTarget(B->RouteCache[0]);
				B->StartWaitForMove();
				return true;
			}
		}
		// new hide target
		// TODO: real logic
		HideTarget = FRouteCacheItem(FriendlyBase);
		FSingleEndpointEval NodeEval(HideTarget.GetLocation(NULL));
		float Weight = 0.0f;
		if (NavData->FindBestPath(B->GetPawn(), *B->GetPawn()->GetNavAgentProperties(), NodeEval, B->GetNavAgentLocation(), Weight, true, B->RouteCache))
		{
			B->GoalString = "Hide";
			B->SetMoveTarget(B->RouteCache[0]);
			B->StartWaitForMove();
			return true;
		}
		else
		{
			HideTarget.Clear();
			return false;
		}
	}
	else
	{
		// return to base
		// TODO: much more to do here
		bool bAllowDetours = (FriendlyBase != NULL && FriendlyBase->GetCarriedObjectState() != CarriedObjectState::Home) || (B->GetPawn()->GetActorLocation() - EnemyBase->GetActorLocation()).Size() < (B->GetPawn()->GetActorLocation() - FriendlyBase->GetActorLocation()).Size();
		return B->TryPathToward(FriendlyBase, bAllowDetours, "Return to base with enemy flag");
	}
}

bool AUTCTFSquadAI::CheckSquadObjectives(AUTBot* B)
{
	FName CurrentOrders = GetCurrentOrders(B);
	// TODO: will need to redirect to vehicle for VCTF
	if (B->GetUTChar() != NULL && B->GetUTChar()->GetCarriedObject() != NULL)
	{
		return SetFlagCarrierAction(B);
	}
	else if (CurrentOrders == NAME_Defend)
	{
		if (GameObjective != NULL && GameObjective->GetCarriedObject() != NULL && GameObjective->GetCarriedObjectState() != CarriedObjectState::Home)
		{
			bool bEnemyFlagOut = (EnemyBase == NULL || EnemyBase->GetCarriedObjectState() == CarriedObjectState::Home);
			AUTCharacter* EnemyCarrier = GameObjective->GetCarriedObject()->HoldingPawn;
			if (EnemyCarrier != NULL)
			{
				if (EnemyCarrier == B->GetEnemy())
				{
					B->GoalString = "Fight flag carrier";
					// fight enemy
					return false;
				}
				else
				{
					// TODO: FindPathToIntercept()? Maybe adjust hunting logic to not require hunting target == enemy and use that?
					return B->TryPathToward(EnemyCarrier, bEnemyFlagOut, "Hunt flag carrier");
				}
			}
			else
			{
				// TODO: model of where flag might be, search around for it
				return B->TryPathToward(GameObjective->GetCarriedObject(), bEnemyFlagOut, "Find dropped flag");
			}
		}
		else if (B->GetEnemy() != NULL)
		{
			B->GoalString = "Fight attacker";
			return false;
		}
		else if (B->FindInventoryGoal(0.0005))
		{
			B->GoalString = FString::Printf(TEXT("Get inventory %s"), *GetNameSafe(B->RouteCache.Last().Actor.Get()));
			B->SetMoveTarget(B->RouteCache[0]);
			B->StartWaitForMove();
			return true;
		}
		else if (Objective != NULL)
		{
			// TODO: find defense point
			return B->TryPathToward(Objective, true, "Defend objective");
		}
		else
		{
			return false;
		}
	}
	else if (CurrentOrders == NAME_Attack)
	{
		if (B->NeedsWeapon() && B->FindInventoryGoal(0.0f))
		{
			B->SetMoveTarget(B->RouteCache[0]);
			B->StartWaitForMove();
			return false;
		}
		else if (GameObjective != NULL && GameObjective->GetCarriedObjectState() != CarriedObjectState::Home)
		{
			if (GameObjective->GetCarriedObjectState() == CarriedObjectState::Held)
			{
				// defend flag carrier
				if (B->GetEnemy() != NULL && B->LineOfSightTo(GameObjective->GetCarriedObject()->HoldingPawn))
				{
					return false;
				}
				else
				{
					return B->TryPathToward(GameObjective->GetCarriedObject()->HoldingPawn, true, "Find friendly flag carrier");
				}
			}
			else
			{
				return B->TryPathToward(GameObjective->GetCarriedObject(), false, "Go pickup flag");
			}
		}
		else
		{
			// TODO: alternate path logic
			return B->TryPathToward(Objective, true, "Attack objective");
		}
	}
	else if (Cast<APlayerController>(Leader) != NULL && Leader->GetPawn() != NULL)
	{
		if (B->GetEnemy() != NULL && !B->LostContact(3.0f))
		{
			// fight!
			return false;
		}
		else
		{
			return B->TryPathToward(Leader->GetPawn(), true, "Find leader");
		}
	}
	else
	{
		// TODO: midfield - engage enemies, acquire powerups, attack when stacked
		return false;
	}
}

void AUTCTFSquadAI::NotifyObjectiveEvent(AActor* InObjective, AController* InstigatedBy, FName EventName)
{
	if (InstigatedBy != NULL && InObjective == Objective && GameObjective != NULL && GameObjective->GetCarriedObject() != NULL && GameObjective->GetCarriedObject()->Holder == InstigatedBy->PlayerState)
	{
		SetLeader(InstigatedBy);
	}

	Super::NotifyObjectiveEvent(InObjective, InstigatedBy, EventName);
}

bool AUTCTFSquadAI::HasHighPriorityObjective(AUTBot* B)
{
	// if our flag is out and enemy's is safe, everyone needs to try to rectify that in some way or another
	if (FriendlyBase != NULL && EnemyBase != NULL && FriendlyBase->GetCarriedObjectState() != CarriedObjectState::Home && EnemyBase->GetCarriedObjectState() == CarriedObjectState::Home)
	{
		return true;
	}
	else
	{
		// otherwise only high priority if the flag this squad cares about is out
		return (GameObjective != NULL && GameObjective->GetCarriedObjectState() != CarriedObjectState::Home && GameObjective->GetCarriedObjectHolder() != B->PlayerState);
	}
}