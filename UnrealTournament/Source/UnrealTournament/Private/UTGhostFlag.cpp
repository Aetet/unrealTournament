// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTGhostFlag.h"
#include "Net/UnrealNetwork.h"
#include "UTFlagReturnTrail.h"

AUTGhostFlag::AUTGhostFlag(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	SetReplicates(true);
	NetPriority = 3.0;
	bReplicateMovement = true;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	UCapsuleComponent* Root = ObjectInitializer.CreateDefaultSubobject<UCapsuleComponent>(this, TEXT("Capsule"));
	Root->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Root->bShouldUpdatePhysicsVolume = false;
	Root->Mobility = EComponentMobility::Movable;
	RootComponent = Root;

	TimerEffect = ObjectInitializer.CreateDefaultSubobject<UParticleSystemComponent>(this, TEXT("TimerEffect"));
	if (TimerEffect != NULL)
	{
		TimerEffect->SetHiddenInGame(true);
		TimerEffect->AttachParent = RootComponent;
		TimerEffect->LDMaxDrawDistance = 1024.0f;
		TimerEffect->RelativeLocation.Z = 40.0f;
		TimerEffect->Mobility = EComponentMobility::Movable;
		TimerEffect->SetCastShadow(false);
	}
}

void AUTGhostFlag::Destroyed()
{
	Super::Destroyed();
	if (Trail)
	{
		Trail->Destroy();
	}
}

void AUTGhostFlag::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AUTGhostFlag, MyCarriedObject);
}

void AUTGhostFlag::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (GetNetMode() != NM_DedicatedServer && MyCarriedObject != nullptr && MyCarriedObject->ObjectState == CarriedObjectState::Dropped)
	{
		float TimerPosition = MyCarriedObject->AutoReturnTime > 0 ? (1.0f - MyCarriedObject->FlagReturnTime / MyCarriedObject->AutoReturnTime) : 0.0;
		TimerEffect->SetHiddenInGame(false);
		TimerEffect->SetFloatParameter(NAME_Progress, TimerPosition);			
		TimerEffect->SetFloatParameter(NAME_RespawnTime, 60);
	}
	else
	{
		TimerEffect->SetHiddenInGame(true);
	}
}

void AUTGhostFlag::OnSetCarriedObject()
{
	if (MyCarriedObject && (GetNetMode() != NM_DedicatedServer))
	{
		if (Trail)
		{
			Trail->Destroy();
		}
		FActorSpawnParameters Params;
		Params.Owner = this;
		Trail = GetWorld()->SpawnActor<AUTFlagReturnTrail>(AUTFlagReturnTrail::StaticClass(), MyCarriedObject->GetActorLocation(), MyCarriedObject->GetActorRotation(), Params);
		Trail->EndPoint = GetActorLocation();
		Trail->SetTeamIndex(TeamIndex);
		Trail->CustomTimeDilation = 0.1f;
	}
}

void AUTGhostFlag::SetCarriedObject(AUTCarriedObject* NewCarriedObject)
{
	MyCarriedObject = NewCarriedObject;
	TeamIndex = (MyCarriedObject && MyCarriedObject->Team) ? MyCarriedObject->Team->TeamIndex : 0;
	OnSetCarriedObject();
}

void AUTGhostFlag::MoveTo(const FVector& NewLocation)
{
	SetActorLocation(NewLocation);
	OnSetCarriedObject(); 
}

void AUTGhostFlag::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	OnSetCarriedObject();
}

