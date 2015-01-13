// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTWeap_LinkGun.h"
#include "UTProj_BioShot.h"
#include "UTProj_LinkPlasma.h"
#include "UTTeamGameMode.h"
#include "UnrealNetwork.h"

AUTWeap_LinkGun::AUTWeap_LinkGun(const FObjectInitializer& OI)
: Super(OI)
{
	if (FiringState.Num() > 0)
	{
		FireInterval[1] = 0.12f;
		InstantHitInfo.AddZeroed();
		InstantHitInfo[0].Damage = 9;
		InstantHitInfo[0].TraceRange = 2200.0f;
	}
	if (AmmoCost.Num() < 2)
	{
		AmmoCost.SetNum(2);
	}
	ClassicGroup = 5;
	AmmoCost[0] = 1;
	AmmoCost[1] = 1;

	Ammo = 70;
	MaxAmmo = 220;

	HUDIcon = MakeCanvasIcon(HUDIcon.Texture, 453.0f, 467.0, 147.0f, 41.0f);

	LinkVolume = 240;
	LinkBreakDelay = 0.5f;
	LinkFlexibility = 0.64f;
	LinkDistanceScaling = 1.5f;

	PerLinkDamageScalingPrimary = 1.f;
	PerLinkDamageScalingSecondary = 1.25f;
	PerLinkDistanceScalingSecondary = 0.2f;

	LinkedBio = NULL;
	bRecommendSuppressiveFire = true;
}

AUTProjectile* AUTWeap_LinkGun::FireProjectile()
{
	AUTProj_LinkPlasma* LinkProj = Cast<AUTProj_LinkPlasma>(Super::FireProjectile());
	if (LinkProj != NULL)
	{
		LinkProj->SetLinks(Links);
		LinkProj->DamageParams.BaseDamage += LinkProj->DamageParams.BaseDamage * PerLinkDamageScalingPrimary * Links;
	}
	
	return LinkProj;
}

void AUTWeap_LinkGun::FireInstantHit(bool bDealDamage, FHitResult* OutHit)
{
	const FVector SpawnLocation = GetFireStartLoc();

	FHitResult Hit;
	if (LinkTarget != nullptr && IsLinkValid())
	{
		Hit.Location = LinkTarget->GetActorLocation();
		Hit.Actor = LinkTarget;
		// don't need to bother with damage since linked
		if (Role == ROLE_Authority)
		{
			UTOwner->SetFlashLocation(Hit.Location, 10);
		}
		if (OutHit != NULL)
		{
			*OutHit = Hit;
		}
	}
	else
	{
		// TODO: Reset Link here since Link wasn't valid?
		LinkBreakTime = -1.f;
		LinkTarget = NULL;

		Super::FireInstantHit(bDealDamage, OutHit);
	}
}

bool AUTWeap_LinkGun::IsLinkValid_Implementation()
{
	const FVector Dir = GetAdjustedAim(GetFireStartLoc()).Vector();
	const FVector TargetDir = (LinkTarget->GetActorLocation() - GetUTOwner()->GetActorLocation()).GetSafeNormal();

	// do a trace to prevent link through objects
	FCollisionQueryParams TraceParams;
	FHitResult HitResult;
	if (GetWorld()->LineTraceSingle(HitResult, GetFireStartLoc(), LinkTarget->GetActorLocation(), ECC_Visibility, TraceParams))
	{
		if (HitResult.GetActor() != LinkTarget)
		{
			// blocked by other
			return false;
		}
	}

	// distance check
	float linkMaxDistance = (InstantHitInfo[GetCurrentFireMode()].TraceRange + (InstantHitInfo[GetCurrentFireMode()].TraceRange * PerLinkDistanceScalingSecondary * Links)) * LinkDistanceScaling;
	if (FVector::Dist(Dir, TargetDir) > linkMaxDistance)
	{
		// exceeded distance
		return false;
	}

	if (!(FVector::DotProduct(Dir, TargetDir) > LinkFlexibility))
	{
		// exceeded angle
		return false;
	}

	return true;
}


void AUTWeap_LinkGun::ConsumeAmmo(uint8 FireModeNum)
{
	LinkedConsumeAmmo(FireModeNum);

	Super::ConsumeAmmo(FireModeNum);
}

void AUTWeap_LinkGun::LinkedConsumeAmmo(int32 Mode)
{
	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		AUTCharacter* C = Cast<AUTCharacter>(*It);
		if (C != NULL)
		{
			AUTWeap_LinkGun* OtherLinkGun = Cast<AUTWeap_LinkGun>(C->GetWeapon());
			if (OtherLinkGun != NULL && OtherLinkGun->LinkedTo(this))
			{
				OtherLinkGun->ConsumeAmmo(Mode);
			}
		}
	}
}

bool AUTWeap_LinkGun::LinkedTo(AUTWeap_LinkGun* L)
{
	AUTCharacter* LinkCharacter = Cast<AUTCharacter>(LinkTarget);
	if (LinkCharacter != NULL && L == LinkCharacter->GetWeapon())
	{
		return true;
	}

	return false;
}

bool AUTWeap_LinkGun::IsLinkable(AActor* Other)
{
	APawn* Pawn = Cast<APawn>(Other);
	if (Pawn != nullptr) // && UTC.bProjTarget ~~~~ && UTC->GetActorEnableCollision()
	{
		// @! TODO : Allow/disallow Vehicles
		//if (Cast<AVehicle>(Pawn) != nullptr)
		//{
		//  return false;
		//}

		AUTCharacter* UTC = Cast<AUTCharacter>(Other);
		if (UTC != nullptr)
		{
			AUTWeap_LinkGun* OtherLinkGun = Cast<AUTWeap_LinkGun>(UTC->GetWeapon());
			if (OtherLinkGun != nullptr)
			{
				if (OtherLinkGun->LinkTarget == Cast<AActor>(GetUTOwner()))
				{
					return false;
				}

				// @! TODO: Loop and prevent circular chains

				AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
				return (GS != NULL && GS->OnSameTeam(UTC, UTOwner));
			}
		}
	}
	//@! TODO : Non-Pawns like Objectives

	return false;
}

void AUTWeap_LinkGun::SetLinkTo(AActor* Other)
{
	if (LinkTarget != nullptr)
	{
		RemoveLink(1 + Links, UTOwner);
	}

	LinkTarget = Other;

	if (LinkTarget != nullptr && Cast<AUTCharacter>(LinkTarget) != nullptr)
	{
		if (!AddLink(1 + Links, UTOwner))
		{
			bFeedbackDeath = true;
		}

		if (GetNetMode() != NM_DedicatedServer)
		{
			UUTGameplayStatics::UTPlaySound(GetWorld(), LinkEstablishedOtherSound, LinkTarget, SRT_None);
		}
	}
}

bool AUTWeap_LinkGun::AddLink(int32 Size, AUTCharacter* Starter)
{
	AUTWeap_LinkGun* OtherLinkGun;
	AUTCharacter* LinkCharacter = Cast<AUTCharacter>(LinkTarget);

	if (LinkCharacter != nullptr && !bFeedbackDeath)
	{
		if (LinkCharacter == Starter)
		{
			return false;
		}
		else
		{
			OtherLinkGun = Cast<AUTWeap_LinkGun>(LinkCharacter->GetWeapon());
			if (OtherLinkGun != nullptr)
			{
				if (OtherLinkGun->AddLink(Size, Starter))
				{
					OtherLinkGun->Links += Size;
				}
				else
				{
					return false;
				}
			}
		}
	}
	return true;
}

void AUTWeap_LinkGun::RemoveLink(int32 Size, AUTCharacter* Starter)
{
	AUTWeap_LinkGun* OtherLinkGun;
	AUTCharacter* LinkCharacter = Cast<AUTCharacter>(LinkTarget);

	if (LinkCharacter != NULL && !bFeedbackDeath)
	{
		if (LinkCharacter != Starter)
		{
			OtherLinkGun = Cast<AUTWeap_LinkGun>(LinkCharacter->GetWeapon());
			if (OtherLinkGun != NULL)
			{
				OtherLinkGun->RemoveLink(Size, Starter);
				OtherLinkGun->Links -= Size;
			}
		}
	}
}

void AUTWeap_LinkGun::ClearLinksTo()
{
	Links = 0;
}

void AUTWeap_LinkGun::ClearLinksFrom()
{
	LinkTarget = nullptr;
}

void AUTWeap_LinkGun::ClearLinks()
{
	ClearLinksFrom();
	ClearLinksTo();
}

void AUTWeap_LinkGun::PlayImpactEffects(const FVector& TargetLoc, uint8 FireMode, const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
	FVector ModifiedTargetLoc = TargetLoc;

	if (LinkedBio != NULL) 
	{
		if (LinkedBio->IsPendingKillPending() || FireMode != 1 || UTOwner == NULL || ((UTOwner->GetActorLocation() - LinkedBio->GetActorLocation()).Size() > LinkedBio->MaxLinkDistance + InstantHitInfo[1].TraceRange))
		{
			LinkedBio = NULL;
		}
		else
		{
			// verify line of sight
			FHitResult Hit;
			static FName NAME_BioLinkTrace(TEXT("BioLinkTrace"));
			bool bBlockingHit = GetWorld()->LineTraceSingle(Hit, SpawnLocation, LinkedBio->GetActorLocation(), COLLISION_TRACE_WEAPON, FCollisionQueryParams(NAME_BioLinkTrace, false, UTOwner));
			if ((bBlockingHit || (Hit.Actor != NULL)) && !Cast<AUTProj_BioShot>(Hit.Actor.Get()))
			{
				LinkedBio = NULL;
			}
		}
	}
	if (LinkedBio != NULL)
	{
		ModifiedTargetLoc = LinkedBio->GetActorLocation();
	}
	Super::PlayImpactEffects(ModifiedTargetLoc, FireMode, SpawnLocation, SpawnRotation);

	// color beam if linked
	if (MuzzleFlash.IsValidIndex(CurrentFireMode) && MuzzleFlash[CurrentFireMode] != NULL)
	{
		static FName NAME_TeamColor(TEXT("TeamColor"));
		bool bGotTeamColor = false;
		AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
		if (Cast<IUTTeamInterface>(LinkTarget) != NULL && GS != NULL)
		{
			uint8 TeamNum = Cast<IUTTeamInterface>(LinkTarget)->GetTeamNum();
			if (GS->Teams.IsValidIndex(TeamNum) && GS->Teams[TeamNum] != NULL)
			{
				MuzzleFlash[CurrentFireMode]->SetVectorParameter(NAME_TeamColor, FVector(GS->Teams[TeamNum]->TeamColor.R, GS->Teams[TeamNum]->TeamColor.G, GS->Teams[TeamNum]->TeamColor.B));
				bGotTeamColor = true;
			}
		}
		if (!bGotTeamColor)
		{
			MuzzleFlash[CurrentFireMode]->ClearParameter(NAME_TeamColor);
		}
	}
}

void AUTWeap_LinkGun::StopFire(uint8 FireModeNum)
{
	LinkedBio = NULL;
	Super::StopFire(FireModeNum);
}

// reset links
bool AUTWeap_LinkGun::PutDown()
{
	if (Super::PutDown())
	{
		ClearLinks();
		return true;
	}
	else
	{
		return false;
	}
}

void AUTWeap_LinkGun::ServerStopFire_Implementation(uint8 FireModeNum)
{
	LinkedBio = NULL;
	Super::ServerStopFire_Implementation(FireModeNum);
}

void AUTWeap_LinkGun::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AUTWeap_LinkGun, Links, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AUTWeap_LinkGun, LinkTarget, COND_OwnerOnly);
}

void AUTWeap_LinkGun::DebugSetLinkGunLinks(int32 newLinks)
{
	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Black, FString::Printf(TEXT("DebugSetLinkGunLinks, oldLinks: %i, newLinks: %i"), Links, newLinks));
	Links = newLinks;
}
