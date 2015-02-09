// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTCarriedObject.h"
#include "UTCTFFlag.h"
#include "UTCTFGameMessage.h"
#include "UTCTFGameState.h"
#include "UTCTFGameMode.h"
#include "UTLastSecondMessage.h"

AUTCTFFlag::AUTCTFFlag(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> FlagMesh (TEXT("SkeletalMesh'/Game/RestrictedAssets/Proto/UT3_Pickups/Flag/S_CTF_Flag_IronGuard.S_CTF_Flag_IronGuard'"));

	Mesh = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("CTFFlag"));
	GetMesh()->SetSkeletalMesh(FlagMesh.Object);
	GetMesh()->AlwaysLoadOnClient = true;
	GetMesh()->AlwaysLoadOnServer = true;
	GetMesh()->AttachParent = RootComponent;
	GetMesh()->SetAbsolute(false, false, true);

	MovementComponent->ProjectileGravityScale=1.3f;
	MessageClass = UUTCTFGameMessage::StaticClass();
	bAlwaysRelevant = true;
}

void AUTCTFFlag::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// backwards compatibility; force values on existing instances
	GetMesh()->SetAbsolute(false, false, true);
	GetMesh()->SetWorldRotation(FRotator(0.0f, 0.f, 0.f));

	if (Role == ROLE_Authority)
	{
		FTimerHandle TempHandle;
		GetWorldTimerManager().SetTimer(TempHandle, this, &AUTCTFFlag::DefaultTimer, 1.0f, true);
	}
}

void AUTCTFFlag::DefaultTimer()
{
	if (Holder != NULL)
	{
		// Look to see if the holder's flag is out (thus our holder is at minimum preventing the other team from scoring)
		uint8 HolderTeam = Holder->GetTeamNum();
		AUTCTFGameState* CTFGameState = GetWorld()->GetGameState<AUTCTFGameState>();
		if (CTFGameState != NULL && CTFGameState->FlagBases.IsValidIndex(HolderTeam) && CTFGameState->FlagBases[HolderTeam] != NULL && CTFGameState->FlagBases[HolderTeam]->GetCarriedObjectHolder() != NULL)
		{
			AUTCTFGameMode* GM = Cast<AUTCTFGameMode>(GetWorld()->GetAuthGameMode());
			if (GM != NULL)
			{
				GM->ScoreHolder(Holder);
			}
		}
	}
}

bool AUTCTFFlag::CanBePickedUpBy(AUTCharacter* Character)
{
	if (Character != NULL)
	{
		AUTCTFFlag* CarriedFlag = Cast<AUTCTFFlag>(Character->GetCarriedObject());
		if (CarriedFlag != NULL && CarriedFlag != this && ObjectState == CarriedObjectState::Home)
		{
			if (CarriedFlag->GetTeamNum() != GetTeamNum())
			{
				CarriedFlag->Score(FName(TEXT("FlagCapture")), CarriedFlag->HoldingPawn, CarriedFlag->Holder);
				CarriedFlag->GetMesh()->SetRelativeScale3D(FVector(1.5f,1.5f,1.5f));
				CarriedFlag->GetMesh()->SetWorldScale3D(FVector(1.5f,1.5f,1.5f));
				return false;
			}
		}
	}

	return Super::CanBePickedUpBy(Character);
}

void AUTCTFFlag::DetachFrom(USkeletalMeshComponent* AttachToMesh)
{
	Super::DetachFrom(AttachToMesh);
	if (AttachToMesh != NULL && Mesh != NULL)
	{
		GetMesh()->SetAbsolute(false, false, true);
		GetMesh()->SetRelativeScale3D(FVector(1.0f,1.0f,1.0f));
		GetMesh()->SetWorldScale3D(FVector(1.0f,1.0f,1.0f));
	}
}

void AUTCTFFlag::OnObjectStateChanged()
{
	Super::OnObjectStateChanged();

	if (Role == ROLE_Authority)
	{
		if (ObjectState == CarriedObjectState::Dropped)
		{
			GetWorldTimerManager().SetTimer(SendHomeWithNotifyHandle, this, &AUTCTFFlag::SendHomeWithNotify, AutoReturnTime, false);
		}
		else
		{
			GetWorldTimerManager().ClearTimer(SendHomeWithNotifyHandle);
		}
	}
}

void AUTCTFFlag::SendHomeWithNotify()
{
	SendGameMessage(1, NULL, NULL);
	SendHome();
}

void AUTCTFFlag::SendHome()
{
	GetMesh()->SetRelativeScale3D(FVector(1.5f,1.5f,1.5f));
	GetMesh()->SetWorldScale3D(FVector(1.5f,1.5f,1.5f));
	Super::SendHome();
}

void AUTCTFFlag::Drop(AController* Killer)
{
	bool bDelayDroppedMessage = false;
	AUTPlayerState* KillerState = Killer ? Cast<AUTPlayerState>(Killer->PlayerState) : NULL;
	if (KillerState && KillerState->Team && (KillerState != Holder))
	{
		// see if this is a last second save
		AUTCTFGameState* GameState = GetWorld()->GetGameState<AUTCTFGameState>();
		if (GameState)
		{
			AUTCTFFlagBase* KillerBase = GameState->FlagBases[1-GetTeamNum()];
			AActor* Considered = LastHoldingPawn;
			if (!Considered)
			{
				Considered = this;
			}

			if (KillerBase && (KillerBase->GetFlagState() == CarriedObjectState::Home) && KillerBase->ActorIsNearMe(Considered))
			{
				AUTGameMode* GM = GetWorld()->GetAuthGameMode<AUTGameMode>();
				if (GM)
				{
					bDelayDroppedMessage = true;
					GM->BroadcastLocalized(this, UUTLastSecondMessage::StaticClass(), 0, Killer->PlayerState, Holder, NULL);
				}
			}
		}
	}

	FlagDropTime = GetWorld()->GetTimeSeconds();
	if (bDelayDroppedMessage)
	{
		FTimerHandle TempHandle;
		GetWorldTimerManager().SetTimer(TempHandle, this, &AUTCTFFlag::DelayedDropMessage, 0.8f, false);
	}
	else
	{
		SendGameMessage(3, Holder, NULL);
	}
	NoLongerHeld();

	// Toss is out
	TossObject(LastHoldingPawn);

	if (HomeBase != NULL)
	{
		HomeBase->ObjectWasDropped(LastHoldingPawn);
	}
	ChangeState(CarriedObjectState::Dropped);
}

void AUTCTFFlag::DelayedDropMessage()
{
	if ((LastGameMessageTime < FlagDropTime) && (ObjectState == CarriedObjectState::Dropped))
	{
		SendGameMessage(3, Holder, NULL);
	}
}