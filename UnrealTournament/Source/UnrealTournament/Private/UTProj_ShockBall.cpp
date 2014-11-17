// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTProj_ShockBall.h"
#include "UTWeap_ShockRifle.h"
#include "UnrealNetwork.h"

AUTProj_ShockBall::AUTProj_ShockBall(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ComboDamageParams = FRadialDamageParams(215.0f, 550.0f);
	ComboAmmoCost = 3;
	bComboExplosion = false;
	ComboMomentum = 330000.0f;
	bIsEnergyProjectile = true;
	PrimaryActorTick.bCanEverTick = true;
}

void AUTProj_ShockBall::InitFakeProjectile(AUTPlayerController* OwningPlayer)
{
	Super::InitFakeProjectile(OwningPlayer);
	TArray<USphereComponent*> Components;
	GetComponents<USphereComponent>(Components);
	for (int32 i = 0; i < Components.Num(); i++)
	{
		if (Components[i] != CollisionComp)
		{
			Components[i]->SetCollisionResponseToAllChannels(ECR_Ignore);
		}
	}
}

void AUTProj_ShockBall::ReceiveAnyDamage(float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, class AActor* DamageCauser)
{
	if (bFakeClientProjectile)
	{
		if (MasterProjectile && !MasterProjectile->IsPendingKillPending())
		{
			MasterProjectile->ReceiveAnyDamage(Damage, DamageType, InstigatedBy, DamageCauser);
		}
		return;
	}
	if (ComboTriggerType != NULL && DamageType != NULL && DamageType->IsA(ComboTriggerType))
	{
		AUTPlayerController* UTPC = Cast<AUTPlayerController>(InstigatedBy);
		bool bUsingClientSideHits = UTPC && (UTPC->MaxPredictionPing > 0.f);

		if ((Role == ROLE_Authority) && !bUsingClientSideHits)
		{
			PerformCombo(InstigatedBy, DamageCauser);
		}
		else if ((Role != ROLE_Authority) && bUsingClientSideHits)
		{
			UTPC->ServerNotifyProjectileHit(this, GetActorLocation(), DamageCauser, GetWorld()->GetTimeSeconds());
		}
	}
}

void AUTProj_ShockBall::NotifyClientSideHit(AUTPlayerController* InstigatedBy, FVector HitLocation, AActor* DamageCauser)
{
	// @TODO FIXMESTEVE - do I limit how far I move combo, so fair to all?
	TArray<USphereComponent*> Components;
	GetComponents<USphereComponent>(Components);
	for (int32 i = 0; i < Components.Num(); i++)
	{
		if (Components[i] != CollisionComp)
		{
			Components[i]->SetCollisionResponseToAllChannels(ECR_Ignore);
		}
	}
	SetActorLocation(HitLocation);
	PerformCombo(InstigatedBy, DamageCauser);
}

void AUTProj_ShockBall::PerformCombo(class AController* InstigatedBy, class AActor* DamageCauser)
{
	//Consume extra ammo for the combo
	AUTWeapon* Weapon = Cast<AUTWeapon>(DamageCauser);
	if (Weapon != NULL)
	{
		Weapon->AddAmmo(-ComboAmmoCost);
	}

	//The player who combos gets the credit
	InstigatorController = InstigatedBy;

	// Replicate combo and execute locally
	bComboExplosion = true;
	OnRep_ComboExplosion();
	Explode(GetActorLocation(), FVector(0.0f, 0.0f, 1.0f));
}

void AUTProj_ShockBall::OnRep_ComboExplosion()
{
	//Swap combo damage and effects
	DamageParams = ComboDamageParams;
	ExplosionEffects = ComboExplosionEffects;
	MyDamageType = ComboDamageType;
	Momentum = ComboMomentum;
}

void AUTProj_ShockBall::OnComboExplode_Implementation(){}

void AUTProj_ShockBall::Explode_Implementation(const FVector& HitLocation, const FVector& HitNormal, UPrimitiveComponent* HitComp)
{
	if (!bExploded)
	{
		if (MyDamageType == ComboDamageType)
		{
			// special case for combo - get rid of fake projectile
			if (MyFakeProjectile != NULL)
			{
				// to make sure effects play consistently we need to copy over the LastRenderTime
				float FakeLastRenderTime = MyFakeProjectile->GetLastRenderTime();
				TArray<UPrimitiveComponent*> Components;
				GetComponents<UPrimitiveComponent>(Components);
				for (UPrimitiveComponent* Comp : Components)
				{
					Comp->LastRenderTime = FMath::Max<float>(Comp->LastRenderTime, FakeLastRenderTime);
				}

				MyFakeProjectile->Destroy();
				MyFakeProjectile = NULL;
			}
		}
		Super::Explode_Implementation(HitLocation, HitNormal, HitComp);
		if (bComboExplosion)
		{
			OnComboExplode();
		}
		// if bot is low skill, delay clearing bot monitoring so that it will occasionally fire for the combo slightly too late - a realistic player mistake
		AUTBot* B = Cast<AUTBot>(InstigatorController);
		if (bPendingKillPending || B == NULL || B->WeaponProficiencyCheck())
		{
			ClearBotCombo();
		}
		else
		{
			GetWorldTimerManager().SetTimer(this, &AUTProj_ShockBall::ClearBotCombo, 0.2f, false);
		}
	}
}

void AUTProj_ShockBall::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	ClearBotCombo();
}

void AUTProj_ShockBall::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AUTProj_ShockBall, bComboExplosion, COND_None);
}

void AUTProj_ShockBall::StartBotComboMonitoring()
{
	bMonitorBotCombo = true;
	PrimaryActorTick.SetTickFunctionEnable(true);
}

void AUTProj_ShockBall::ClearBotCombo()
{
	AUTBot* B = Cast<AUTBot>(InstigatorController);
	if (B != NULL)
	{
		if (B->GetTarget() == this)
		{
			B->SetTarget(NULL);
		}
		if (B->GetFocusActor() == this)
		{
			B->SetFocus(B->GetTarget());
		}
	}
	bMonitorBotCombo = false;
}

void AUTProj_ShockBall::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bMonitorBotCombo)
	{
		AUTBot* B = Cast<AUTBot>(InstigatorController);
		if (B != NULL)
		{
			AUTWeap_ShockRifle* Rifle = (B->GetUTChar() != NULL) ? Cast<AUTWeap_ShockRifle>(B->GetUTChar()->GetWeapon()) : NULL;
			if (Rifle != NULL && !Rifle->IsFiring())
			{
				switch (B->ShouldTriggerCombo(GetActorLocation(), GetVelocity(), ComboDamageParams))
				{
					case BMS_Abort:
						if (Rifle->ComboTarget == this)
						{
							Rifle->ComboTarget = NULL;
						}
						// if high skill, still monitor just in case
						bMonitorBotCombo = B->WeaponProficiencyCheck() && B->LineOfSightTo(this);
						break;
					case BMS_PrepareActivation:
						if (B->GetTarget() != this)
						{
							B->SetTarget(this);
							B->SetFocus(this);
						}
						break;
					case BMS_Activate:
						if (Rifle->ComboTarget == this)
						{
							Rifle->ComboTarget = NULL;
						}
						if (B->GetFocusActor() == this && !B->NeedToTurn(GetTargetLocation()))
						{
							Rifle->DoCombo();
						}
						else if (B->GetTarget() != this)
						{
							B->SetTarget(this);
							B->SetFocus(this);
						}
						break;
					default:
						break;
				}
			}
		}
	}
}