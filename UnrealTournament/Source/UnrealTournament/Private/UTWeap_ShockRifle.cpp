// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTWeap_ShockRifle.h"
#include "UTProj_ShockBall.h"
#include "UTCanvasRenderTarget2D.h"

AUTWeap_ShockRifle::AUTWeap_ShockRifle(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	ClassicGroup = 4;
	BaseAISelectRating = 0.65f;
	BasePickupDesireability = 0.65f;
	ScreenMaterialID = 5;
	LastClientKillTime = -100000.0f;
}

void AUTWeap_ShockRifle::AttachToOwnerNative()
{
	Super::AttachToOwnerNative();

	if (GetNetMode() != NM_DedicatedServer && Mesh != NULL && ScreenMaterialID < Mesh->GetNumMaterials())
	{
		ScreenMI = Mesh->CreateAndSetMaterialInstanceDynamic(ScreenMaterialID);
		// FIXME: can't use, results in crash due to engine bug
		//ScreenTexture = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(UCanvasRenderTarget2D::StaticClass(), 64, 64);
		{
			ScreenTexture = ConstructObject<UUTCanvasRenderTarget2D>(UUTCanvasRenderTarget2D::StaticClass(), this);
			ScreenTexture->InitAutoFormat(64, 64);
		}
		ScreenTexture->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
		ScreenTexture->OnCanvasRenderTargetUpdate.AddDynamic(this, &AUTWeap_ShockRifle::UpdateScreenTexture);
		ScreenMI->SetTextureParameterValue(FName(TEXT("ScreenTexture")), ScreenTexture);
	}
}

void AUTWeap_ShockRifle::UpdateScreenTexture(UCanvas* C, int32 Width, int32 Height)
{
	if (GetWorld()->TimeSeconds - LastClientKillTime < 2.5f && ScreenKillNotifyTexture != NULL)
	{
		C->SetDrawColor(FColor::White);
		C->DrawTile(ScreenKillNotifyTexture, 0.0f, 0.0f, float(Width), float(Height), 0.0f, 0.0f, ScreenKillNotifyTexture->GetSizeX(), ScreenKillNotifyTexture->GetSizeY());
	}
	else
	{
		FFontRenderInfo RenderInfo;
		RenderInfo.bClipText = true;
		RenderInfo.GlowInfo.bEnableGlow = true;
		RenderInfo.GlowInfo.GlowColor = FLinearColor(-0.75f, -0.75f, -0.75f, 1.0f);
		RenderInfo.GlowInfo.GlowOuterRadius.X = 0.45f;
		RenderInfo.GlowInfo.GlowOuterRadius.Y = 0.475f;
		RenderInfo.GlowInfo.GlowInnerRadius.X = 0.475f;
		RenderInfo.GlowInfo.GlowInnerRadius.Y = 0.5f;

		FString AmmoText = FString::FromInt(Ammo);
		float XL, YL;
		C->TextSize(ScreenFont, AmmoText, XL, YL);
		FUTCanvasTextItem Item(FVector2D(Width / 2 - XL * 0.5f, Height / 2 - YL * 0.5f), FText::FromString(AmmoText), ScreenFont, (Ammo <= 5) ? FLinearColor::Red : FLinearColor::White);
		Item.FontRenderInfo = RenderInfo;
		C->DrawItem(Item);
	}
}

void AUTWeap_ShockRifle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ScreenTexture != NULL && Mesh->IsRegistered() && GetWorld()->TimeSeconds - Mesh->LastRenderTime < 1.0f)
	{
		ScreenTexture->UpdateResource();
	}
}

bool AUTWeap_ShockRifle::WaitingForCombo()
{
	if (ComboTarget != NULL && !ComboTarget->bPendingKillPending && !ComboTarget->bExploded)
	{
		return true;
	}
	else
	{
		ComboTarget = NULL;
		return false;
	}
}

void AUTWeap_ShockRifle::DoCombo()
{
	ComboTarget = NULL;
	if (UTOwner != NULL)
	{
		UTOwner->StartFire(0);
	}
}

bool AUTWeap_ShockRifle::IsPreparingAttack_Implementation()
{
	// if bot is allowed to do a moving combo then prioritize evasive action prior to combo time
	return !bMovingComboCheckResult && WaitingForCombo();
}

float AUTWeap_ShockRifle::SuggestAttackStyle_Implementation()
{
	return -0.4f;
}

float AUTWeap_ShockRifle::GetAISelectRating_Implementation()
{
	AUTBot* B = Cast<AUTBot>(UTOwner->Controller);
	if (B == NULL || B->GetEnemy() == NULL || Cast<APawn>(B->GetTarget()) == NULL)
	{
		return BaseAISelectRating;
	}
	else if (WaitingForCombo())
	{
		return 1.5f;
	}
	else if (!B->WeaponProficiencyCheck())
	{
		return BaseAISelectRating;
	}
	else
	{
		FVector EnemyLoc = B->GetEnemyLocation(B->GetEnemy(), true);
		if (B->IsStopped())
		{
			if (!B->LineOfSightTo(B->GetEnemy()) && (EnemyLoc - UTOwner->GetActorLocation()).Size() < 11000.0f)
			{
				return BaseAISelectRating + 0.5f;
			}
			else
			{
				return BaseAISelectRating + 0.3f;
			}
		}
		else if ((EnemyLoc - UTOwner->GetActorLocation()).Size() > 3500.0f)
		{
			return BaseAISelectRating + 0.1f;
		}
		else if (EnemyLoc.Z > UTOwner->GetActorLocation().Z + 325.0f)
		{
			return BaseAISelectRating + 0.15f;
		}
		else
		{
			return BaseAISelectRating;
		}
	}
}

bool AUTWeap_ShockRifle::ShouldAIDelayFiring_Implementation()
{
	if (!WaitingForCombo())
	{
		return false;
	}
	else if (bMovingComboCheckResult)
	{
		return true;
	}
	else
	{
		AUTBot* B = Cast<AUTBot>(UTOwner->Controller);
		if (B != NULL && !B->IsStopped())
		{
			// bot's too low skill to do combo since it started moving
			ComboTarget->ClearBotCombo();
			ComboTarget = NULL;
			return false;
		}
		else
		{
			return true;
		}
	}
}

bool AUTWeap_ShockRifle::CanAttack_Implementation(AActor* Target, const FVector& TargetLoc, bool bDirectOnly, bool bPreferCurrentMode, uint8& BestFireMode, FVector& OptimalTargetLoc)
{
	AUTBot* B = Cast<AUTBot>(UTOwner->Controller);
	if (B == NULL)
	{
		return Super::CanAttack_Implementation(Target, TargetLoc, bDirectOnly, bPreferCurrentMode, BestFireMode, OptimalTargetLoc);
	}
	else if (WaitingForCombo() && (Target == ComboTarget || Target == B->GetTarget()))
	{
		BestFireMode = 0;
		return true;
	}
	else if (Super::CanAttack_Implementation(Target, TargetLoc, bDirectOnly, bPreferCurrentMode, BestFireMode, OptimalTargetLoc))
	{
		if (bPreferCurrentMode)
		{
			return true;
		}
		else
		{
			if (Cast<APawn>(Target) == NULL)
			{
				BestFireMode = 0;
			}
			/*else if (!B.LineOfSightTo(B.Enemy))
			{
				if ((ComboTarget != None) && !ComboTarget.bDeleteMe && B.CanCombo())
				{
					bWaitForCombo = true;
					return 0;
				}
				ComboTarget = None;
				if (B.CanCombo() && B.ProficientWithWeapon())
				{
					bRegisterTarget = true;
					return 1;
				}
				return 0;
			}*/
			else
			{
				float EnemyDist = (TargetLoc - UTOwner->GetActorLocation()).Size();
				const AUTProjectile* DefAltProj = (ProjClass.IsValidIndex(1) && ProjClass[1] != NULL) ? ProjClass[1].GetDefaultObject() : NULL;
				const float AltSpeed = (DefAltProj != NULL && DefAltProj->ProjectileMovement != NULL) ? DefAltProj->ProjectileMovement->InitialSpeed : FLT_MAX;

				if (EnemyDist > 4.0f * AltSpeed)
				{
					bPlanningCombo = false;
					BestFireMode = 0;
				}
				else
				{
					ComboTarget = NULL;
					if (EnemyDist > 5500.0f && FMath::FRand() < 0.5f)
					{
						BestFireMode = 0;
					}
					else if (B->CanCombo() && B->WeaponProficiencyCheck())
					{
						bPlanningCombo = true;
						BestFireMode = 1;
					}
					// projectile is better in close due to its size
					else
					{
						AUTCharacter* EnemyChar = Cast<AUTCharacter>(Target);
						if (EnemyDist < 2200.0f && EnemyChar != NULL && EnemyChar->GetWeapon() != NULL && EnemyChar->GetWeapon()->GetClass() != GetClass() && B->WeaponProficiencyCheck())
						{
							BestFireMode = (FMath::FRand() < 0.3f) ? 0 : 1;
						}
						else
						{
							BestFireMode = (FMath::FRand() < 0.7f) ? 0 : 1;
						}
					}
				}
			}
			return true;
		}
	}
	else if (bDirectOnly)
	{
		return false;
	}
	else if ((bPreferCurrentMode && bPlanningCombo) || B->CanCombo())
	{
		// TODO: see if shock combo could hit enemy if it came around the corner
		BestFireMode = 1;
		return false;
	}
	else
	{
		return false;
	}
}

AUTProjectile* AUTWeap_ShockRifle::FireProjectile()
{
	AUTProjectile* Result = Super::FireProjectile();
	if (bPlanningCombo && UTOwner != NULL)
	{
		AUTProj_ShockBall* ShockBall = Cast<AUTProj_ShockBall>(Result);
		if (ShockBall != NULL)
		{
			ShockBall->StartBotComboMonitoring();
			ComboTarget = ShockBall;
			AUTBot* B = Cast<AUTBot>(UTOwner->Controller);
			if (B != NULL)
			{
				bMovingComboCheckResult = B->MovingComboCheck();
			}
			bPlanningCombo = false;
		}
	}
	return Result;
}