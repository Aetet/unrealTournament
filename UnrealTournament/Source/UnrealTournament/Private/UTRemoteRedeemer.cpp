// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UnrealNetwork.h"
#include "UTProjectileMovementComponent.h"
#include "UTImpactEffect.h"
#include "UTRemoteRedeemer.h"
#include "UTLastSecondMessage.h"

AUTRemoteRedeemer::AUTRemoteRedeemer(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	// Use a sphere as a simple collision representation
	CollisionComp = PCIP.CreateOptionalDefaultSubobject<USphereComponent>(this, TEXT("SphereComp"));
	if (CollisionComp != NULL)
	{
		CollisionComp->InitSphereRadius(0.0f);
		CollisionComp->BodyInstance.SetCollisionProfileName("ProjectileShootable");			// Collision profiles are defined in DefaultEngine.ini
		CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AUTRemoteRedeemer::OnOverlapBegin);
		CollisionComp->bTraceComplexOnMove = true;
		CollisionComp->bGenerateOverlapEvents = false;
		RootComponent = CollisionComp;
	}

	// Use a ProjectileMovementComponent to govern this projectile's movement
	ProjectileMovement = PCIP.CreateDefaultSubobject<UUTProjectileMovementComponent>(this, TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 2000.f;
	ProjectileMovement->MaxSpeed = 2000.f;
	ProjectileMovement->ProjectileGravityScale = 0;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->OnProjectileStop.AddDynamic(this, &AUTRemoteRedeemer::OnStop);

	SetReplicates(true);
	bNetTemporary = false;
	bReplicateInstigator = true;

	AccelRate = 4000.0;
	RedeemerMouseSensitivity = 700.0f;
	AccelerationBlend = 5.0f;
	MaximumRoll = 20.0f;
	RollMultiplier = 0.01f;
	RollSmoothingMultiplier = 5.0f;
	MaxPitch = 75.0f;
	MinPitch = -55.0f;

	ExplosionTimings[0] = 0.5;
	ExplosionTimings[1] = 0.2;
	ExplosionTimings[2] = 0.2;
	ExplosionTimings[3] = 0.2;
	ExplosionTimings[4] = 0.2;

	ExplosionRadii[0] = 0.125f;
	ExplosionRadii[1] = 0.3f;
	ExplosionRadii[2] = 0.475f;
	ExplosionRadii[3] = 0.65f;
	ExplosionRadii[4] = 0.825f;
	ExplosionRadii[5] = 1.0f;

	CollisionFreeRadius = 1200;
}

FVector AUTRemoteRedeemer::GetVelocity() const
{
	return ProjectileMovement->Velocity;
}

void AUTRemoteRedeemer::PostNetReceiveVelocity(const FVector& NewVelocity)
{
	ProjectileMovement->Velocity = NewVelocity;
}

bool AUTRemoteRedeemer::TryToDrive(APawn* NewDriver)
{
	return DriverEnter(NewDriver);
}

bool AUTRemoteRedeemer::DriverEnter(APawn* NewDriver)
{
	if (Role != ROLE_Authority)
	{
		return false;
	}

	if (Driver != nullptr)
	{
		DriverLeave(true);
	}

	if (NewDriver != nullptr)
	{
		Driver = NewDriver;
		AController* C = NewDriver->Controller;
		if (C)
		{
			C->UnPossess();
			NewDriver->SetOwner(this);
			C->Possess(this);
			AUTCharacter *UTChar = Cast<AUTCharacter>(NewDriver);
			if (UTChar)
			{
				UTChar->StartDriving(this);
			}
			DamageInstigator = C;
		}
	}

	return true;
}

bool AUTRemoteRedeemer::DriverLeave(bool bForceLeave)
{
	BlowUp();

	AController* C = Controller;
	if (Driver && C)
	{
		C->UnPossess();
		Driver->SetOwner(C);
		AUTCharacter *UTChar = Cast<AUTCharacter>(Driver);
		if (UTChar)
		{
			UTChar->StopDriving(this);
		}
		C->Possess(Driver);
	}

	Driver = nullptr;

	return true;
}

void AUTRemoteRedeemer::OnStop(const FHitResult& Hit)
{
	BlowUp();
}

void AUTRemoteRedeemer::OnOverlapBegin(AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Role != ROLE_Authority)
	{
		return;
	}

	if (Driver == OtherActor)
	{
		return;
	}

	const IUTTeamInterface* TeamInterface = InterfaceCast<IUTTeamInterface>(OtherActor);
	if (TeamInterface)
	{
		if (TeamInterface->GetTeamNum() == GetTeamNum())
		{
			return;
		}
	}

	BlowUp();
}

void AUTRemoteRedeemer::Destroyed()
{
	if (Driver != nullptr)
	{
		DriverLeave(true);
	}
}

void AUTRemoteRedeemer::BlowUp()
{
	if (!bExploded)
	{
		bExploded = true;

		GetWorldTimerManager().ClearTimer(this, &AUTRemoteRedeemer::BlowUp);
		DriverLeave(true);

		ProjectileMovement->SetActive(false);

		TArray<USceneComponent*> Components;
		GetComponents<USceneComponent>(Components);
		for (int32 i = 0; i < Components.Num(); i++)
		{
			Components[i]->SetHiddenInGame(true);
		}

		PlayExplosionEffects();

		ExplodeStage1();
	}
}

void AUTRemoteRedeemer::OnRep_PlayExplosionEffects()
{
	PlayExplosionEffects();
}

void AUTRemoteRedeemer::PlayExplosionEffects()
{
	bPlayExplosionEffects = true;
	
	// stop any looping audio
	TArray<USceneComponent*> Components;
	GetComponents<USceneComponent>(Components);
	for (int32 i = 0; i < Components.Num(); i++)
	{
		UAudioComponent* Audio = Cast<UAudioComponent>(Components[i]);
		if (Audio != NULL)
		{
			// only stop looping (ambient) sounds - note that the just played explosion sound may be encountered here
			if (Audio->Sound != NULL && Audio->Sound->GetDuration() >= INDEFINITELY_LOOPING_DURATION)
			{
				Audio->Stop();
			}
		}
	}

	if (ExplosionEffects != NULL)
	{
		ExplosionEffects.GetDefaultObject()->SpawnEffect(GetWorld(), FTransform(GetActorRotation(), GetActorLocation()), nullptr, this, DamageInstigator);
	}
}

uint8 AUTRemoteRedeemer::GetTeamNum() const
{
	const IUTTeamInterface* TeamInterface = InterfaceCast<IUTTeamInterface>(Controller);
	if (TeamInterface != NULL)
	{
		return TeamInterface->GetTeamNum();
	}

	return 255;
}

void AUTRemoteRedeemer::FaceRotation(FRotator NewControlRotation, float DeltaTime)
{
	if (Controller && Controller->IsLocalPlayerController())
	{
		APlayerController *PC = Cast<APlayerController>(Controller);

		FRotator Rotation = GetActorRotation();
		FVector X, Y, Z;
		FRotationMatrix R(Rotation);
		R.GetScaledAxes(X, Y, Z);

		// Bleed off Yaw and Pitch acceleration while addition additional from input
		YawAccel = (1.0f - 2.0f * DeltaTime) * YawAccel + DeltaTime * PC->RotationInput.Yaw * RedeemerMouseSensitivity;
		PitchAccel = (1.0f - 2.0f * DeltaTime) * PitchAccel + DeltaTime * PC->RotationInput.Pitch * RedeemerMouseSensitivity;
		
		if (Rotation.Pitch > MaxPitch)
		{
			PitchAccel = FMath::Min(0.0f, PitchAccel);
		}
		else if (Rotation.Pitch < MinPitch)
		{
			PitchAccel = FMath::Max(0.0f, PitchAccel);
		}

		ProjectileMovement->Acceleration = ProjectileMovement->Velocity + AccelerationBlend * (YawAccel * Y + PitchAccel * Z);
		if (ProjectileMovement->Acceleration.IsNearlyZero())
		{
			ProjectileMovement->Acceleration = ProjectileMovement->Velocity;
		}
		
		ProjectileMovement->Acceleration = ProjectileMovement->Acceleration.SafeNormal() * AccelRate;
	}
}

void AUTRemoteRedeemer::GetActorEyesViewPoint(FVector& out_Location, FRotator& out_Rotation) const
{
	out_Location = GetActorLocation();
	out_Rotation = GetActorRotation();
}

void AUTRemoteRedeemer::PawnStartFire(uint8 FireModeNum)
{
	ServerBlowUp();
	ProjectileMovement->SetActive(false);
}

bool AUTRemoteRedeemer::ServerBlowUp_Validate()
{
	return true;
}

void AUTRemoteRedeemer::ServerBlowUp_Implementation()
{
	BlowUp();
}

void AUTRemoteRedeemer::ExplodeStage(float RangeMultiplier)
{
	AUTProj_Redeemer *DefaultRedeemer = RedeemerProjectileClass->GetDefaultObject<AUTProj_Redeemer>();
	if (DefaultRedeemer)
	{
		FRadialDamageParams AdjustedDamageParams = DefaultRedeemer->DamageParams;
		if (AdjustedDamageParams.OuterRadius > 0.0f)
		{
			TArray<AActor*> IgnoreActors;
			FVector ExplosionCenter = GetActorLocation() + FVector(0, 0, 400);

			UUTGameplayStatics::UTHurtRadius(this, AdjustedDamageParams.BaseDamage, AdjustedDamageParams.MinimumDamage, DefaultRedeemer->Momentum, ExplosionCenter, RangeMultiplier * AdjustedDamageParams.InnerRadius, RangeMultiplier * AdjustedDamageParams.OuterRadius, AdjustedDamageParams.DamageFalloff,
				DefaultRedeemer->MyDamageType, IgnoreActors, this, DamageInstigator, nullptr, nullptr, CollisionFreeRadius);
		}
	}
	else
	{
		UE_LOG(UT, Warning, TEXT("UTRemoteRedeemer does not have a proper reference to UTProj_Redeemer"));
	}
}

void AUTRemoteRedeemer::ExplodeStage1()
{
	ExplodeStage(ExplosionRadii[0]);
	GetWorldTimerManager().SetTimer(this, &AUTRemoteRedeemer::ExplodeStage2, ExplosionTimings[0]);
}
void AUTRemoteRedeemer::ExplodeStage2()
{
	ExplodeStage(ExplosionRadii[1]);
	GetWorldTimerManager().SetTimer(this, &AUTRemoteRedeemer::ExplodeStage3, ExplosionTimings[1]);
}
void AUTRemoteRedeemer::ExplodeStage3()
{
	ExplodeStage(ExplosionRadii[2]);
	GetWorldTimerManager().SetTimer(this, &AUTRemoteRedeemer::ExplodeStage4, ExplosionTimings[2]);
}
void AUTRemoteRedeemer::ExplodeStage4()
{
	ExplodeStage(ExplosionRadii[3]);
	GetWorldTimerManager().SetTimer(this, &AUTRemoteRedeemer::ExplodeStage5, ExplosionTimings[3]);
}
void AUTRemoteRedeemer::ExplodeStage5()
{
	ExplodeStage(ExplosionRadii[4]);
	GetWorldTimerManager().SetTimer(this, &AUTRemoteRedeemer::ExplodeStage6, ExplosionTimings[4]);
}
void AUTRemoteRedeemer::ExplodeStage6()
{
	ExplodeStage(ExplosionRadii[5]);
	ShutDown();
}

void AUTRemoteRedeemer::ShutDown()
{
	// Post explosion clean up here
	ProjectileMovement->SetActive(false);

	SetLifeSpan(2.0f);
}

void AUTRemoteRedeemer::ForceReplication_Implementation()
{
}

bool AUTRemoteRedeemer::IsRelevancyOwnerFor(AActor* ReplicatedActor, AActor* ActorOwner, AActor* ConnectionActor)
{
	if (ReplicatedActor == ActorOwner)
	{
		return true;
	}

	if (Driver == ReplicatedActor->GetOwner())
	{
		return true;
	}

	return Super::IsRelevancyOwnerFor(ReplicatedActor, ActorOwner, ConnectionActor);
}

void AUTRemoteRedeemer::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AUTRemoteRedeemer, bPlayExplosionEffects, COND_None);
}

void AUTRemoteRedeemer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Role != ROLE_SimulatedProxy)
	{
		FRotator Rotation = GetActorRotation();
		FVector X, Y, Z;
		FRotationMatrix R(Rotation);
		R.GetScaledAxes(X, Y, Z);

		// Roll the camera when there's yaw acceleration
		FRotator RolledRotation = ProjectileMovement->Velocity.Rotation();
		float YawMag = ProjectileMovement->Acceleration | Y;
		if (YawMag > 0)
		{
			RolledRotation.Roll = FMath::Min(MaximumRoll, RollMultiplier * YawMag);
		}
		else
		{
			RolledRotation.Roll = FMath::Max(-MaximumRoll, RollMultiplier * YawMag);
		}

		float SmoothRoll = FMath::Min(1.0f, RollSmoothingMultiplier * DeltaSeconds);
		RolledRotation.Roll = RolledRotation.Roll * SmoothRoll + Rotation.Roll * (1.0f - SmoothRoll);
		SetActorRotation(RolledRotation);
	}
}

void AUTRemoteRedeemer::RedeemerDenied(AController* InstigatedBy)
{
	APlayerState* InstigatorPS = GetController() ? GetController()->PlayerState : NULL;
	APlayerState* InstigatedbyPS = InstigatedBy ? InstigatedBy->PlayerState : NULL;
	if (Cast<AUTPlayerController>(InstigatedBy))
	{
		Cast<AUTPlayerController>(InstigatedBy)->ClientReceiveLocalizedMessage(UUTLastSecondMessage::StaticClass(), 0, InstigatedbyPS, InstigatorPS, NULL);
	}
	if (Cast<AUTPlayerController>(GetController()))
	{
		Cast<AUTPlayerController>(GetController())->ClientReceiveLocalizedMessage(UUTLastSecondMessage::StaticClass(), 0, InstigatedbyPS, InstigatorPS, NULL);
	}
}

float AUTRemoteRedeemer::TakeDamage(float Damage, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!ShouldTakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser))
	{
		return 0.f;
	}
	else
	{
		int32 ResultDamage = Damage;
		FVector ResultMomentum(0.f);
		AUTGameMode* Game = GetWorld()->GetAuthGameMode<AUTGameMode>();
		if (Game != NULL)
		{
			// we need to pull the hit info out of FDamageEvent because ModifyDamage() goes through blueprints and that doesn't correctly handle polymorphic structs
			FHitResult HitInfo;
			FVector UnusedDir;
			DamageEvent.GetBestHitInfo(this, DamageCauser, HitInfo, UnusedDir);

			Game->ModifyDamage(ResultDamage, ResultMomentum, this, EventInstigator, HitInfo, DamageCauser, DamageEvent.DamageTypeClass);
		}

		if (ResultDamage > 0)
		{
			if (EventInstigator != NULL && EventInstigator != Controller)
			{
				LastHitBy = EventInstigator;
			}

			if (ResultDamage > 0)
			{
				// this is partially copied from AActor::TakeDamage() (just the calls to the various delegates and K2 notifications)
				const UDamageType* const DamageTypeCDO = DamageEvent.DamageTypeClass ? DamageEvent.DamageTypeClass->GetDefaultObject<UDamageType>() : GetDefault<UDamageType>();

				float ActualDamage = float(ResultDamage); // engine hooks want float
				// generic damage notifications sent for any damage
				ReceiveAnyDamage(ActualDamage, DamageTypeCDO, EventInstigator, DamageCauser);
				OnTakeAnyDamage.Broadcast(ActualDamage, DamageTypeCDO, EventInstigator, DamageCauser);
				if (EventInstigator != NULL)
				{
					EventInstigator->InstigatedAnyDamage(ActualDamage, DamageTypeCDO, this, DamageCauser);
				}
				BlowUp();
			}
		}
		return float(ResultDamage);
	}
}