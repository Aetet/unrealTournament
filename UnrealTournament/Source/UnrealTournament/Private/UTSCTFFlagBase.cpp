// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTSCTFFlag.h"
#include "UTSCTFGameState.h"
#include "UTSCTFFlagBase.h"
#include "Net/UnrealNetwork.h"

AUTSCTFFlagBase::AUTSCTFFlagBase(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> GMesh(TEXT("SkeletalMesh'/Game/RestrictedAssets/Proto/UT3_Pickups/Flag/S_Gauntlet_Flag_Mesh.S_Gauntlet_Flag_Mesh'"));

	GhostMesh = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("GhostMesh"));
	GhostMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GhostMesh->SetSkeletalMesh(GMesh.Object);
	GhostMesh->AttachParent = RootComponent;

	bGhostMeshVisibile = true;
	GhostMesh->SetVisibility(bGhostMeshVisibile);
}

void AUTSCTFFlagBase::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AUTSCTFFlagBase, bGhostMeshVisibile);		
}

void AUTSCTFFlagBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Force the state of the ghost mesh
	OnRep_bGhostMeshVisibile();	
}


void AUTSCTFFlagBase::OnRep_bGhostMeshVisibile()
{
	GhostMesh->SetVisibility(bGhostMeshVisibile);
}


void AUTSCTFFlagBase::PostRenderFor(APlayerController* PC, UCanvas* Canvas, FVector CameraPosition, FVector CameraDir)
{
	if (bGhostMeshVisibile && PC->GetPawn())
	{
		float Scale = Canvas->ClipY / 1080.0f;
		FVector2D Size = FVector2D(14,47) * Scale;

		AUTHUD* HUD = Cast<AUTHUD>(PC->MyHUD);
		FVector FlagLoc = GetActorLocation() + FVector(0.0f,0.0f,128.0f);
		FVector ScreenPosition = Canvas->Project(FlagLoc);

		FVector LookVec;
		FRotator LookDir;
		PC->GetPawn()->GetActorEyesViewPoint(LookVec,LookDir);

		if (HUD && FVector::DotProduct(LookDir.Vector().GetSafeNormal(), (FlagLoc - LookVec)) > 0.0f && 
			ScreenPosition.X > 0 && ScreenPosition.X < Canvas->ClipX && 
			ScreenPosition.Y > 0 && ScreenPosition.Y < Canvas->ClipY)
		{
			Canvas->SetDrawColor(255,255,255,255);
			Canvas->DrawTile(HUD->HUDAtlas, ScreenPosition.X - (Size.X * 0.5f), ScreenPosition.Y - Size.Y, Size.X, Size.Y,1009,0,14,47);
		}
	}
}

void AUTSCTFFlagBase::OnOverlapBegin(AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bScoreBase)
	{
		AUTCharacter* Character = Cast<AUTCharacter>(OtherActor);
		if (Character != NULL)
		{
			AUTSCTFFlag* Flag = Cast<AUTSCTFFlag>(Character->GetCarriedObject());
			if ( Flag != NULL && Flag->GetTeamNum() == GetTeamNum() )
			{
				AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
				if (GS == NULL || (GS->IsMatchInProgress() && !GS->IsMatchIntermission()))
				{
					Flag->Score(FName(TEXT("FlagCapture")), Flag->HoldingPawn, Flag->Holder);
					Flag->PlayCaptureEffect();

					for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
					{
						AUTPlayerController* PC = Cast<AUTPlayerController>(*Iterator);
						if (PC)
						{
							if (GetTeamNum() == PC->GetTeamNum() && FlagScoreRewardSound)
							{
								PC->ClientPlaySound(FlagScoreRewardSound, 2.f);
							}
						}
					}
				}
			}
		}
	}
}

void AUTSCTFFlagBase::CreateCarriedObject()
{
	return;
}

void AUTSCTFFlagBase::Activate()
{
	CreateFlag();
}

void AUTSCTFFlagBase::Deactivate()
{
	if (bScoreBase)
	{
		bGhostMeshVisibile = false;
		if (Role == ROLE_Authority) OnRep_bGhostMeshVisibile();
	}

	MyFlag = nullptr;
	if (CarriedObject != NULL)
	{
		AUTSCTFGameState* GameState = GetWorld()->GetGameState<AUTSCTFGameState>();
		if (GameState && GameState->Flag == Cast<AUTSCTFFlag>(CarriedObject))
		{
			GameState->Flag = nullptr;
		}
		CarriedObject->Destroy();
	}
}

void AUTSCTFFlagBase::Reset()
{
	RoundReset();
}

void AUTSCTFFlagBase::RoundReset()
{
	Deactivate();
}


void AUTSCTFFlagBase::CreateFlag()
{
	FActorSpawnParameters Params;
	Params.Owner = this;

	if (bScoreBase)
	{
		bGhostMeshVisibile = true;
		if (Role == ROLE_Authority) OnRep_bGhostMeshVisibile();
	}
	else
	{
		bGhostMeshVisibile = false;
		if (Role == ROLE_Authority) OnRep_bGhostMeshVisibile();

		CarriedObject = GetWorld()->SpawnActor<AUTCarriedObject>(CarriedObjectClass, GetActorLocation() + FVector(0, 0, 96), GetActorRotation(), Params);
		AUTSCTFGameState* GameState = GetWorld()->GetGameState<AUTSCTFGameState>();
		if (GameState && Cast<AUTSCTFFlag>(CarriedObject))
		{
			GameState->Flag = Cast<AUTSCTFFlag>(CarriedObject);
		}
	}

	if (CarriedObject != NULL)
	{
		CarriedObject->Init(this);
	}
	else
	{
		UE_LOG(UT, Warning, TEXT("%s Could not create an object of type %s"), *GetNameSafe(this), *GetNameSafe(CarriedObjectClass));
	}

	MyFlag = Cast<AUTCTFFlag>(CarriedObject);
	if (MyFlag && MyFlag->GetMesh())
	{
		MyFlag->GetMesh()->ClothBlendWeight = MyFlag->ClothBlendHome;
		MyFlag->bShouldPingFlag = true;
	}
}

FText AUTSCTFFlagBase::GetHUDStatusMessage(AUTHUD* HUD)
{
	AUTSCTFGameState* GameState = GetWorld()->GetGameState<AUTSCTFGameState>();

	if (GameState && GameState->HasMatchStarted())
	{
		if (bScoreBase )
		{
			if (GameState->Flag && GameState->Flag->ObjectState == CarriedObjectState::Held && GameState->Flag->GetTeamNum() == GetTeamNum())
			{
				// This is a destination -- display the help tip..
				if (GetTeamNum() == HUD->UTPlayerOwner->GetTeamNum())
				{
					return NSLOCTEXT("AUTSCTFFlagBase","CaptureMsg","Capture Here");
				}
				else
				{
					return NSLOCTEXT("AUTSCTFFlagBase","DefendMsg","Defend Here");
				}
			}
		}
		else
		{
			if (MyFlag == nullptr)
			{
				return FText::AsNumber(GameState->FlagSpawnTimer);
			}
		}
	}
	return FText::GetEmpty();
}


