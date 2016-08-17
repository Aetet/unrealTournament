// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTProj_StingerShard.h"
#include "UnrealNetwork.h"
#include "UTImpactEffect.h"
#include "UTLift.h"
#include "UTProjectileMovementComponent.h"
#include "PhysicsEngine/BodySetup.h"

AUTProj_StingerShard::AUTProj_StingerShard(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	ShardMesh = ObjectInitializer.CreateOptionalDefaultSubobject<UStaticMeshComponent>(this, FName(TEXT("Mesh")));
	if (ShardMesh != NULL)
	{
		ShardMesh->SetCollisionProfileName(FName(TEXT("NoCollision")));
		ShardMesh->AttachParent = RootComponent;
	}
	if (PawnOverlapSphere != NULL)
	{
		PawnOverlapSphere->SetRelativeLocation(FVector(-10.f, 0.f, 0.f));
	}
	InitialLifeSpan = 3.f;
	ImpactedShardDamage = 12;
	ImpactedShardMomentum = 50000.f;
	bLowPriorityLight = true;
	bNetTemporary = true;
}

void AUTProj_StingerShard::Destroyed()
{
	// detonate shards embedded in walls when they time out
	if (!ImpactNormal.IsZero())
	{
		Explode(GetActorLocation(), ImpactNormal, CollisionComp);
	}
	Super::Destroyed();
}

void AUTProj_StingerShard::ProcessHit_Implementation(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FVector& HitLocation, const FVector& HitNormal)
{
	// FIXME: temporary? workaround for synchronization issues on clients where it explodes on client but not on server
	if (GetNetMode() == NM_Client && (Cast<APawn>(OtherActor) != NULL || Cast<AUTProjectile>(OtherActor) != NULL) && !OtherActor->bTearOff)
	{
		return;
	}

	APawn* HitPawn = Cast<APawn>(OtherActor);
	if (AttachedPawns.Contains(HitPawn))
	{
		return;
	}
	else if (HitPawn != NULL || Cast<AUTProjectile>(OtherActor) != NULL || (OtherActor && OtherActor->bCanBeDamaged))
	{
		if (HitPawn != NULL)
		{
			if (ProjectileMovement->Velocity.IsZero())
			{
				// only provide momentum along plane
				FVector MomentumDir = OtherActor->GetActorLocation() - GetActorLocation();
				MomentumDir = (MomentumDir - (ImpactNormal | MomentumDir) * ImpactNormal).GetSafeNormal();
				MomentumDir.Z = 1.f;
				ProjectileMovement->Velocity = MomentumDir.GetSafeNormal();

				// clamp player z vel
				AUTCharacter* HitChar = Cast<AUTCharacter>(HitPawn);
				if (HitChar)
				{
					UUTCharacterMovement* UTMove = Cast<UUTCharacterMovement>(HitChar->GetCharacterMovement());
					if (UTMove)
					{
						float CurrentVelZ = UTMove->Velocity.Z;
						if (CurrentVelZ > -1.5f * UTMove->JumpZVelocity)
						{
							CurrentVelZ = FMath::Min(0.f, 3.f * (CurrentVelZ + UTMove->JumpZVelocity));
						}
						UTMove->Velocity.Z = CurrentVelZ;
						UTMove->ClearPendingImpulse();
						UTMove->NeedsClientAdjustment();
					}
				}

				Explode(GetActorLocation(), ImpactNormal, OtherComp);

				FUTPointDamageEvent Event;
				float AdjustedMomentum = ImpactedShardMomentum;
				Event.Damage = ImpactedShardDamage;
				Event.DamageTypeClass = MyDamageType;
				Event.HitInfo = FHitResult(HitPawn, OtherComp, HitLocation, HitNormal);
				Event.ShotDirection = ProjectileMovement->Velocity;
				Event.Momentum = Event.ShotDirection * AdjustedMomentum;
				HitPawn->TakeDamage(Event.Damage, Event, InstigatorController, this);
			}
			else
			{
				AUTCharacter* HitChar = Cast<AUTCharacter>(HitPawn);
				if (HitChar != NULL && HitChar->IsDead() && HitChar->IsRagdoll())
				{
					AttachToRagdoll(HitChar, HitLocation);
				}
				else
				{
					Super::ProcessHit_Implementation(OtherActor, OtherComp, HitLocation, HitNormal);
				}
			}
		}
		else
		{
			Super::ProcessHit_Implementation(OtherActor, OtherComp, HitLocation, HitNormal);
		}
	}
	else if (!ShouldIgnoreHit(OtherActor, OtherComp))
	{
		//Stop the projectile and give it collision
		ProjectileMovement->ProjectileGravityScale = 0.0f;
		ProjectileMovement->Velocity = FVector::ZeroVector;
		ShardMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		bCanHitInstigator = true;
		SetLifeSpan(0.9f);
		if (BounceSound != NULL)
		{
			UUTGameplayStatics::UTPlaySound(GetWorld(), BounceSound, this, SRT_IfSourceNotReplicated, false, FVector::ZeroVector, NULL, NULL, true, SAT_WeaponFoley);
		}

		SetActorLocation(HitLocation + HitNormal);
		ImpactNormal = HitNormal;

		AUTLift* Lift = Cast<AUTLift>(OtherActor);
		if (Lift != NULL && Lift->GetEncroachComponent())
		{
			AttachRootComponentTo(Lift->GetEncroachComponent(), NAME_None, EAttachLocation::KeepWorldPosition);
		}

		// turn off in-flight sound and particle system
		TArray<UAudioComponent*> AudioComponents;
		GetComponents<UAudioComponent>(AudioComponents);
		for (int32 i = 0; i < AudioComponents.Num(); i++)
		{
			if (AudioComponents[i] && AudioComponents[i]->Sound != NULL && AudioComponents[i]->Sound->GetDuration() >= INDEFINITELY_LOOPING_DURATION)
			{
				AudioComponents[i]->Stop();
			}
		}
		TArray<UParticleSystemComponent*> ParticleComponents;
		GetComponents<UParticleSystemComponent>(ParticleComponents);
		for (int32 i = 0; i < ParticleComponents.Num(); i++)
		{
			if (ParticleComponents[i])
			{
				UParticleSystem* SavedTemplate = ParticleComponents[i]->Template;
				ParticleComponents[i]->DeactivateSystem();
				ParticleComponents[i]->KillParticlesForced();
				// FIXME: KillParticlesForced() doesn't kill particles immediately for GPU particles, but the below does...
				ParticleComponents[i]->SetTemplate(NULL);
				ParticleComponents[i]->SetTemplate(SavedTemplate);
			}
		}
	}
}

void AUTProj_StingerShard::AttachToRagdoll(AUTCharacter* HitChar, const FVector& HitLocation)
{
	// create a physics constraint to the ragdoll to drag it and ultimately pin it
	const FBodyInstance* ClosestRagdollPart = NULL;
	float BestDist = FLT_MAX;
	for (const FBodyInstance* TestBody : HitChar->GetMesh()->Bodies)
	{
		if (TestBody->BodySetup.IsValid())
		{
			float TestDist = (TestBody->GetUnrealWorldTransform().GetLocation() - HitLocation).SizeSquared();
			if (TestDist < BestDist)
			{
				ClosestRagdollPart = TestBody;
				BestDist = TestDist;
			}
		}
	}
	if (ClosestRagdollPart != NULL)
	{
		if (HitChar->RagdollConstraint != NULL)
		{
			HitChar->RagdollConstraint->BreakConstraint();
			HitChar->RagdollConstraint->DestroyComponent();
			HitChar->RagdollConstraint = NULL;
		}
		UPhysicsConstraintComponent* NewConstraint = NewObject<UPhysicsConstraintComponent>(this);
		NewConstraint->OnComponentCreated();
		NewConstraint->SetWorldLocation(HitLocation); // note: important! won't work right if not in the proper location
		NewConstraint->RegisterComponent();
		NewConstraint->ConstraintInstance.bDisableCollision = true;
		NewConstraint->SetConstrainedComponents(HitChar->GetMesh(), ClosestRagdollPart->BodySetup.Get()->BoneName, Cast<UPrimitiveComponent>(ProjectileMovement->UpdatedComponent), NAME_None);
		NewConstraint->ConstraintInstance.ProjectionLinearTolerance = 0.05f;
		//NewConstraint->ConstraintInstance.EnableProjection();
		HitChar->RagdollConstraint = NewConstraint;
		ProjectileMovement->Velocity *= 0.3f;
	}
	AttachedPawns.Add(HitChar);
	SetTimerUFunc(this, FName(TEXT("DetachRagdollsInFlight")), 0.5f);
}

void AUTProj_StingerShard::DetachRagdollsInFlight()
{
	if (ProjectileMovement != NULL && !ProjectileMovement->Velocity.IsZero() && ProjectileMovement->UpdatedComponent != NULL)
	{
		for (AUTCharacter* HitChar : AttachedPawns)
		{
			if (HitChar != NULL && !HitChar->IsPendingKillPending() && HitChar->RagdollConstraint != NULL && HitChar->RagdollConstraint->GetOuter() == this)
			{
				HitChar->RagdollConstraint->DestroyComponent();
				HitChar->RagdollConstraint = NULL;
			}
		}
		AttachedPawns.Empty();
	}
}

void AUTProj_StingerShard::Explode_Implementation(const FVector& HitLocation, const FVector& HitNormal, UPrimitiveComponent* HitComp)
{
	// if the impacted actor is a corpse (or became so as a result of our direct damage) then don't explode and keep going
	APawn* HitPawn = Cast<APawn>(ImpactedActor);
	if (HitPawn == NULL || ProjectileMovement->Velocity.IsZero())
	{
		Super::Explode_Implementation(HitLocation, HitNormal, HitComp);
	}
	else if (!AttachedPawns.Contains(HitPawn))
	{
		// note: dedicated servers don't create ragdolls, but they also turn off corpse collision immediately, so in effect the client's "pass through" behavior here actually matches the server, however unintuitive
		AUTCharacter* UTC = Cast<AUTCharacter>(HitPawn);
		if (UTC == NULL || !UTC->IsDead() || !UTC->IsRagdoll())
		{
			Super::Explode_Implementation(HitLocation, HitNormal, HitComp);
		}
		else
		{
			AttachToRagdoll(UTC, HitLocation);
		}
	}
}

float AUTProj_StingerShard::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser)
{
	Explode(GetActorLocation(), ImpactNormal, CollisionComp);
	return DamageAmount;
}
