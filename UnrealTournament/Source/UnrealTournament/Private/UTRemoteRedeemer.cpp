// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UnrealNetwork.h"
#include "UTProjectileMovementComponent.h"
#include "UTImpactEffect.h"
#include "UTRemoteRedeemer.h"
#include "UTCTFRewardMessage.h"
#include "UTHUD.h"
#include "StatNames.h"
#include "UTWorldSettings.h"
#include "UTProj_WeaponScreen.h"
#include "UTRedeemerLaunchAnnounce.h"

AUTRemoteRedeemer::AUTRemoteRedeemer(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	// Use a sphere as a simple collision representation
	CollisionComp = ObjectInitializer.CreateOptionalDefaultSubobject<USphereComponent>(this, TEXT("SphereComp"));
	if (CollisionComp != NULL)
	{
		CollisionComp->InitSphereRadius(0.0f);
		CollisionComp->BodyInstance.SetCollisionProfileName("ProjectileShootable");			// Collision profiles are defined in DefaultEngine.ini
		CollisionComp->OnComponentBeginOverlap.AddDynamic(this, &AUTRemoteRedeemer::OnOverlapBegin);
		CollisionComp->bTraceComplexOnMove = true;
		CollisionComp->bGenerateOverlapEvents = false;
		RootComponent = CollisionComp;
	}

	CapsuleComp = ObjectInitializer.CreateOptionalDefaultSubobject<UCapsuleComponent>(this, TEXT("CapsuleComp"));
	if (CapsuleComp != NULL)
	{
		CapsuleComp->BodyInstance.SetCollisionProfileName("ProjectileShootable");			// Collision profiles are defined in DefaultEngine.ini
		CapsuleComp->OnComponentBeginOverlap.AddDynamic(this, &AUTRemoteRedeemer::OnOverlapBegin);
		CapsuleComp->bTraceComplexOnMove = true;
		CapsuleComp->InitCapsuleSize(16.f, 70.0f);
		CapsuleComp->SetRelativeRotation(FRotator(90.f, 90.f, 90.f));
		CapsuleComp->AttachParent = RootComponent;
	}

	// Use a ProjectileMovementComponent to govern this projectile's movement
	ProjectileMovement = ObjectInitializer.CreateDefaultSubobject<UUTProjectileMovementComponent>(this, TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 2000.f;
	ProjectileMovement->MaxSpeed = 2000.f;
	ProjectileMovement->ProjectileGravityScale = 0;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->OnProjectileStop.AddDynamic(this, &AUTRemoteRedeemer::OnStop);

	SetReplicates(true);
	bNetTemporary = false;

	AccelRate = 4000.f;
	RedeemerMouseSensitivity = 700.0f;
	AccelerationBlend = 5.0f;
	MaximumRoll = 25.0f;
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

	CollisionFreeRadius = 1000.f;
	StatsHitCredit = 0.f;
	HitsStatsName = NAME_RedeemerHits;
	ProjHealth = 35;
	LockCount = 0;
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
				UTChar->PlayerState = PlayerState;
			}
			DamageInstigator = C;
		}
	}

	return true;
}

bool AUTRemoteRedeemer::DriverLeave(bool bForceLeave)
{
	if (!bShotDown)
	{
		BlowUp();
	}

	AController* C = Controller;
	if (Driver && C)
	{
		C->UnPossess();
		Driver->SetOwner(C);
		AUTCharacter *UTChar = Cast<AUTCharacter>(Driver);
		if (UTChar)
		{
			UTChar->StopDriving(this);
			if (UTChar->GetWeapon())
			{
				UTChar->GetWeapon()->AddAmmo(0);
			}
		}
		C->Possess(Driver);
	}
	if (Driver && PlayerState)
	{
		for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
		{
			AUTPlayerController* UTPC = Cast<AUTPlayerController>(It->PlayerController);
			if (UTPC && UTPC->LastSpectatedPlayerState == PlayerState)
			{
				UTPC->ViewPawn(Driver);
			}
		}
	}

	Driver = nullptr;
	return true;
}

void AUTRemoteRedeemer::OnStop(const FHitResult& Hit)
{
	if (Role == ROLE_Authority)
	{
		BlowUp();
	}
}

void AUTRemoteRedeemer::OnOverlapBegin(AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Role == ROLE_Authority && Driver != OtherActor && !Cast<APhysicsVolume>(OtherActor))
	{
		AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
		if (GS == NULL || !GS->OnSameTeam(this, OtherActor))
		{
			if (Cast<AUTProjectile>(OtherActor))
			{
				if (Cast<AUTProj_WeaponScreen>(OtherActor))
				{
					// rather than customtimedilation, just cutting velocity for now
					ProjectileMovement->Velocity = FVector::ZeroVector;
				}
				else
				{
					OnShotDown();
					RedeemerDenied(OtherActor->GetInstigatorController());
				}
			}
			else
			{
				BlowUp();
			}
		}
	}
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
		AUTGameMode* GM = GetWorld()->GetAuthGameMode<AUTGameMode>();
		if (GM)
		{
			GM->BroadcastLocalized(this, UUTRedeemerLaunchAnnounce::StaticClass(), 3);
		}
		bExploded = true;
		bTearOff = true;

		if (OverlayMI != NULL)
		{
			AUTWorldSettings* WS = Cast<AUTWorldSettings>(GetWorld()->GetWorldSettings());
			if (WS != NULL)
			{
				WS->AddTimedMaterialParameter(OverlayMI, FName(TEXT("Static")), OverlayStaticCurve);
			}
		}

		ProjectileMovement->SetActive(false);
		CapsuleComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

void AUTRemoteRedeemer::ExplodeTimed()
{
	if (!bExploded && Role == ROLE_Authority)
	{
		BlowUp();
	}
}

void AUTRemoteRedeemer::OnShotDown()
{
	if (!bExploded)
	{
		bShotDown = true;

		if (Role == ROLE_Authority)
		{
			bTearOff = true;
			DriverLeave(true);
		}

		// fall to ground, explode after a delay
		ProjectileMovement->ProjectileGravityScale = 1.0f;
		ProjectileMovement->MaxSpeed += 2000.0f; // make room for gravity
		ProjectileMovement->bShouldBounce = true;
		ProjectileMovement->Bounciness = 0.25f;
		SetTimerUFunc(this, FName(TEXT("ExplodeTimed")), 2.0f, false);

		if (GetNetMode() != NM_DedicatedServer)
		{
			PlayShotDownEffects();
		}
	}
}

void AUTRemoteRedeemer::PlayShotDownEffects()
{
	// stop any looping audio and particles
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
		else
		{
			UParticleSystemComponent* PSC = Cast<UParticleSystemComponent>(Components[i]);
			if (PSC != NULL && IsLoopingParticleSystem(PSC->Template))
			{
				PSC->DeactivateSystem();
			}
		}
	}
}

void AUTRemoteRedeemer::PlayExplosionEffects()
{
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

	AUTProj_Redeemer* DefaultRedeemer = (RedeemerProjectileClass != NULL) ? RedeemerProjectileClass->GetDefaultObject<AUTProj_Redeemer>() : NULL;
	if (DefaultRedeemer != NULL)
	{
		if (DefaultRedeemer->ExplosionBP != NULL)
		{
			GetWorld()->SpawnActor<AActor>(DefaultRedeemer->ExplosionBP, FTransform(GetActorRotation(), GetActorLocation()));
		}
		else if (DefaultRedeemer->ExplosionEffects != NULL)
		{
			DefaultRedeemer->ExplosionEffects.GetDefaultObject()->SpawnEffect(GetWorld(), FTransform(GetActorRotation(), GetActorLocation()), nullptr, this, DamageInstigator);
		}
	}
}

uint8 AUTRemoteRedeemer::GetTeamNum() const
{
	const IUTTeamInterface* TeamInterface = Cast<IUTTeamInterface>(PlayerState);
	if (TeamInterface != NULL)
	{
		return TeamInterface->GetTeamNum();
	}

	return 255;
}

void AUTRemoteRedeemer::FaceRotation(FRotator NewControlRotation, float DeltaTime)
{
	AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
	if (GS && (GS->IsMatchIntermission() || GS->HasMatchEnded()))
	{
		ProjectileMovement->Acceleration = FVector::ZeroVector;
		ProjectileMovement->Velocity = FVector::ZeroVector;
	}
	else if (Controller && Controller->IsLocalPlayerController())
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
		
		ProjectileMovement->Acceleration = ProjectileMovement->Acceleration.GetSafeNormal() * AccelRate;
/*
		if (Controller->GetPawn() == this)
		{
			NewControlRotation.Roll = Rotation.Roll;
			Controller->SetControlRotation(NewControlRotation);
		}
*/	}
}

void AUTRemoteRedeemer::GetActorEyesViewPoint(FVector& out_Location, FRotator& out_Rotation) const
{
	out_Location = GetActorLocation();
	out_Rotation = GetActorRotation();
}

void AUTRemoteRedeemer::PawnStartFire(uint8 FireModeNum)
{
	if (FireModeNum == 0)
	{
		ServerBlowUp();
		ProjectileMovement->SetActive(false);
	}
}

bool AUTRemoteRedeemer::ServerBlowUp_Validate()
{
	return true;
}

void AUTRemoteRedeemer::ServerBlowUp_Implementation()
{
	BlowUp();
}

void AUTRemoteRedeemer::OnRep_PlayerState()
{
	for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
	{
		AUTPlayerController* UTPC = Cast<AUTPlayerController>(It->PlayerController);
		if (UTPC && UTPC->LastSpectatedPlayerState == PlayerState)
		{
			UTPC->ViewPawn(this);
		}
	}
}

void AUTRemoteRedeemer::ExplodeStage(float RangeMultiplier)
{
	AUTProj_Redeemer* DefaultRedeemer = (RedeemerProjectileClass != NULL) ? RedeemerProjectileClass->GetDefaultObject<AUTProj_Redeemer>() : NULL;
	if (DefaultRedeemer != NULL)
	{
		FRadialDamageParams AdjustedDamageParams = DefaultRedeemer->DamageParams;
		if (AdjustedDamageParams.OuterRadius > 0.0f)
		{
			TArray<AActor*> IgnoreActors;
			FVector ExplosionCenter = GetActorLocation();

			StatsHitCredit = 0.f;
			UUTGameplayStatics::UTHurtRadius(this, AdjustedDamageParams.BaseDamage, AdjustedDamageParams.MinimumDamage, DefaultRedeemer->Momentum, ExplosionCenter, RangeMultiplier * AdjustedDamageParams.InnerRadius, RangeMultiplier * AdjustedDamageParams.OuterRadius, AdjustedDamageParams.DamageFalloff,
				DefaultRedeemer->MyDamageType, IgnoreActors, this, DamageInstigator, nullptr, nullptr, CollisionFreeRadius);
			if ((Role == ROLE_Authority) && (HitsStatsName != NAME_None))
			{
				AUTPlayerState* PS = DamageInstigator ? Cast<AUTPlayerState>(DamageInstigator->PlayerState) : NULL;
				if (PS)
				{
					PS->ModifyStatsValue(HitsStatsName, StatsHitCredit / AdjustedDamageParams.BaseDamage);
				}
			}
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
	FTimerHandle TempHandle;
	GetWorldTimerManager().SetTimer(TempHandle, this, &AUTRemoteRedeemer::ExplodeStage2, ExplosionTimings[0]);
}
void AUTRemoteRedeemer::ExplodeStage2()
{
	ExplodeStage(ExplosionRadii[1]);
	FTimerHandle TempHandle;
	GetWorldTimerManager().SetTimer(TempHandle, this, &AUTRemoteRedeemer::ExplodeStage3, ExplosionTimings[1]);
}
void AUTRemoteRedeemer::ExplodeStage3()
{
	if (Role == ROLE_Authority)
	{
		DriverLeave(true);
	}
	ExplodeStage(ExplosionRadii[2]);
	FTimerHandle TempHandle;
	GetWorldTimerManager().SetTimer(TempHandle, this, &AUTRemoteRedeemer::ExplodeStage4, ExplosionTimings[2]);
}
void AUTRemoteRedeemer::ExplodeStage4()
{
	ExplodeStage(ExplosionRadii[3]);
	FTimerHandle TempHandle;
	GetWorldTimerManager().SetTimer(TempHandle, this, &AUTRemoteRedeemer::ExplodeStage5, ExplosionTimings[3]);
}
void AUTRemoteRedeemer::ExplodeStage5()
{
	ExplodeStage(ExplosionRadii[4]);
	FTimerHandle TempHandle;
	GetWorldTimerManager().SetTimer(TempHandle, this, &AUTRemoteRedeemer::ExplodeStage6, ExplosionTimings[4]);
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

bool AUTRemoteRedeemer::IsRelevancyOwnerFor(const AActor* ReplicatedActor, const AActor* ActorOwner, const AActor* ConnectionActor) const
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

	DOREPLIFETIME_CONDITION(AUTRemoteRedeemer, bShotDown, COND_None);
}

void AUTRemoteRedeemer::TornOff()
{
	if (bShotDown)
	{
		OnShotDown();
	}
	else
	{
		BlowUp();
	}
}

void AUTRemoteRedeemer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Role != ROLE_SimulatedProxy)
	{
		AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
		if (GS && (GS->IsMatchIntermission() || GS->HasMatchEnded()))
		{
			ProjectileMovement->Acceleration = FVector::ZeroVector;
			ProjectileMovement->Velocity = FVector::ZeroVector;
		}
		else
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

	// check outline
	// this is done in Tick() so that it handles edge cases like viewer changing teams
	if (GetNetMode() != NM_DedicatedServer)
	{
		AUTGameState* GS = Cast<AUTGameState>(GetWorld()->GetGameState());
		bool bShowOutline = false;
		if (GS != nullptr && !bExploded)
		{
			for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
			{
				if (It->PlayerController != nullptr && GS->OnSameTeam(It->PlayerController, this))
				{
					// note: does not handle splitscreen
					bShowOutline = true;
					break;
				}
			}
		}
		if (bShowOutline)
		{
			if (CustomDepthMesh == nullptr)
			{
				TInlineComponentArray<UMeshComponent*> Meshes(this);
				if (Meshes.Num() > 0)
				{
					CustomDepthMesh = CreateCustomDepthOutlineMesh(Meshes[0], this);
					CustomDepthMesh->CustomDepthStencilValue = GetTeamNum() + 1;
					CustomDepthMesh->RegisterComponent();
				}
			}
		}
		else if (CustomDepthMesh != nullptr)
		{
			CustomDepthMesh->DestroyComponent();
			CustomDepthMesh = nullptr;
		}
	}
}

void AUTRemoteRedeemer::RedeemerDenied(AController* InstigatedBy)
{
	AUTGameMode* GM = GetWorld()->GetAuthGameMode<AUTGameMode>();
	if (GM)
	{
		APlayerState* InstigatorPS = GetController() ? GetController()->PlayerState : NULL;;
		APlayerState* InstigatedbyPS = InstigatedBy ? InstigatedBy->PlayerState : NULL;
		GM->BroadcastLocalized(this, UUTCTFRewardMessage::StaticClass(), 0, InstigatedbyPS, InstigatorPS, NULL);
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
				ProjHealth -= ActualDamage;
				if (ProjHealth <= 0)
				{
					// small explosion when damaged
					OnShotDown();
					RedeemerDenied(EventInstigator);
				}
			}
		}
		return float(ResultDamage);
	}
}

void AUTRemoteRedeemer::PostRender(AUTHUD* HUD, UCanvas* C)
{
	if (RedeemerDisplayOne != NULL)
	{
		FCanvasTileItem XHairItem(0.5f*FVector2D(C->ClipX - 0.8f*C->ClipY, 0.f), RedeemerDisplayOne->Resource, 0.8f*FVector2D(C->ClipY, C->ClipY), FLinearColor::Red);
		XHairItem.UV0 = FVector2D(0.0f, 0.0f);
		XHairItem.UV1 = FVector2D(1.0f, 1.0f);
		XHairItem.BlendMode = SE_BLEND_Translucent;
		XHairItem.Rotation.Yaw = -1.f*GetActorRotation().Roll;
		XHairItem.Position.Y = XHairItem.Position.Y + 0.1f * C->ClipY;
		XHairItem.PivotPoint = FVector2D(0.5f, 0.5f);
		C->DrawItem(XHairItem);
	}
	int32 NewLockCount = 0;
	if (TargetIndicator != NULL)
	{
		AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
		for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
		{
			AUTCharacter* EnemyChar = Cast<AUTCharacter>(*It);
			if (EnemyChar != NULL && !EnemyChar->IsDead() && !EnemyChar->IsInvisible() && (EnemyChar->DrivenVehicle != this) && !EnemyChar->IsFeigningDeath() && (EnemyChar->GetMesh()->LastRenderTime > GetWorld()->TimeSeconds - 0.15f) && (GS == NULL || !GS->OnSameTeam(EnemyChar, this)))
			{
				FVector CircleLoc = EnemyChar->GetActorLocation();
				static FName NAME_RedeemerHUD(TEXT("RedeemerHUD"));
				if (!GetWorld()->LineTraceTestByChannel(GetActorLocation(), CircleLoc, COLLISION_TRACE_WEAPONNOCHARACTER, FCollisionQueryParams(NAME_RedeemerHUD, true, this)))
				{
					float CircleRadius = 1.2f * EnemyChar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
					FVector Perpendicular = (CircleLoc - GetActorLocation()).GetSafeNormal() ^ FVector(0.0f, 0.0f, 1.0f);
					FVector PointA = C->Project(CircleLoc + Perpendicular * CircleRadius);
					FVector PointB = C->Project(CircleLoc - Perpendicular * CircleRadius);
					FVector2D UpperLeft(FMath::Min<float>(PointA.X, PointB.X), FMath::Min<float>(PointA.Y, PointB.Y));
					FVector2D BottomRight(FMath::Max<float>(PointA.X, PointB.X), FMath::Max<float>(PointA.Y, PointB.Y));
					float MidY = (UpperLeft.Y + BottomRight.Y) * 0.5f;

					// skip drawing if too off-center
					if ((FMath::Abs(MidY - 0.5f*C->SizeY) < 0.5f*C->SizeY) && (FMath::Abs(0.5f*(UpperLeft.X + BottomRight.X) - 0.5f*C->SizeX) < 0.5f*C->SizeX))
					{
						NewLockCount++;

						// square-ify
						float SizeY = FMath::Max<float>(MidY - UpperLeft.Y, (BottomRight.X - UpperLeft.X) * 0.5f);
						UpperLeft.Y = MidY - SizeY;
						BottomRight.Y = MidY + SizeY;
						FLinearColor TargetColor = EnemyChar->bIsWearingHelmet ? FLinearColor(1.0f, 1.0f, 0.0f, 1.0f) : FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);
						FCanvasTileItem HeadCircleItem(UpperLeft, TargetIndicator->Resource, BottomRight - UpperLeft, TargetColor);
						HeadCircleItem.BlendMode = SE_BLEND_Translucent;
						C->DrawItem(HeadCircleItem);

						FFormatNamedArguments Args;
						static const FNumberFormattingOptions RespawnTimeFormat = FNumberFormattingOptions()
							.SetMinimumFractionalDigits(2)
							.SetMaximumFractionalDigits(2);
						Args.Add("Dist", FText::AsNumber( 0.01f * (GetActorLocation() - EnemyChar->GetActorLocation()).Size(), &RespawnTimeFormat));
						FText DistanceMessage = FText::Format(NSLOCTEXT("UUTHUDWidget_Spectator", "TargetDist", "{Dist}m"), Args);
						FFontRenderInfo TextRenderInfo;
						TextRenderInfo.bEnableShadow = true;
						C->SetDrawColor(FLinearColor::Red.ToFColor(false));
						C->DrawText(HUD->TinyFont, DistanceMessage, BottomRight.X, UpperLeft.Y, 1.f, 1.f, TextRenderInfo);
					}
				}
			}
		}
	}
	if ((NewLockCount > LockCount) && HUD && HUD->UTPlayerOwner)
	{
		HUD->UTPlayerOwner->UTClientPlaySound(LockAcquiredSound);
	}

	LockCount = NewLockCount;
}