// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTImpactEffect.h"
#include "UTWorldSettings.h"
#include "Particles/ParticleSystemComponent.h"

AUTImpactEffect::AUTImpactEffect(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bCheckInView = true;
	bForceForLocalPlayer = true;
	bRandomizeDecalRoll = true;
	AlwaysSpawnDistance = 500.0f;
	CullDistance = 20000.0f;
}

bool AUTImpactEffect::SpawnEffect_Implementation(UWorld* World, const FTransform& InTransform, UPrimitiveComponent* HitComp, AActor* SpawnedBy, AController* InstigatedBy, ESoundReplicationType SoundReplication) const
{
	if (World == NULL)
	{
		return false;
	}
	else
	{
		UUTGameplayStatics::UTPlaySound(World, Audio, SpawnedBy, SoundReplication, false, InTransform.GetLocation());

		if (World->GetNetMode() == NM_DedicatedServer)
		{
			return false;
		}
		else
		{
			bool bSpawn = true;
			if (bCheckInView)
			{
				AUTWorldSettings* WS = Cast<AUTWorldSettings>(World->GetWorldSettings());
				bSpawn = (WS == NULL || WS->EffectIsRelevant(SpawnedBy, InTransform.GetLocation(), SpawnedBy != NULL, bForceForLocalPlayer && InstigatedBy != NULL && InstigatedBy->IsLocalPlayerController(), CullDistance, AlwaysSpawnDistance, false));
			}
			if (bSpawn)
			{
				// create components
				TArray<USceneComponent*> NativeCompList;
				GetComponents<USceneComponent>(NativeCompList);
				TArray<USCS_Node*> ConstructionNodes;
				{
					TArray<const UBlueprintGeneratedClass*> ParentBPClassStack;
					UBlueprintGeneratedClass::GetGeneratedClassesHierarchy(GetClass(), ParentBPClassStack);
					for (int32 i = ParentBPClassStack.Num() - 1; i >= 0; i--)
					{
						const UBlueprintGeneratedClass* CurrentBPGClass = ParentBPClassStack[i];
						if (CurrentBPGClass->SimpleConstructionScript)
						{
							ConstructionNodes += CurrentBPGClass->SimpleConstructionScript->GetAllNodes();
						}
					}
				}
				CreateEffectComponents(World, InTransform, HitComp, SpawnedBy, InstigatedBy, bAttachToHitComp ? HitComp : NULL, NAME_None, NativeCompList, ConstructionNodes);
				return true;
			}
			else
			{
				return false;
			}
		}
	}
}

void AUTImpactEffect::CallSpawnEffect(UObject* WorldContextObject, const AUTImpactEffect* Effect, const FTransform& InTransform, UPrimitiveComponent* HitComp, AActor* SpawnedBy, AController* InstigatedBy, ESoundReplicationType SoundReplication)
{
	if (WorldContextObject == NULL)
	{
		UE_LOG(UT, Warning, TEXT("SpawnEffect(): No world context"));
	}
	else if (Effect == NULL)
	{
		UE_LOG(UT, Warning, TEXT("SpawnEffect(): No effect specified"));
	}
	else
	{
		Effect->SpawnEffect(WorldContextObject->GetWorld(), InTransform, HitComp, SpawnedBy, InstigatedBy, SoundReplication);
	}
}

bool AUTImpactEffect::ShouldCreateComponent_Implementation(const USceneComponent* TestComp, FName CompTemplateName, const FTransform& BaseTransform, UPrimitiveComponent* HitComp, AActor* SpawnedBy, AController* InstigatedBy) const
{
	if (HitComp != NULL && TestComp->IsA(UDecalComponent::StaticClass()) && (!HitComp->bReceivesDecals || !HitComp->ShouldRender()) && !HitComp->IsA(UBrushComponent::StaticClass())) // special case to attach to blocking volumes to project on world geo behind it
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool AUTImpactEffect::ComponentCreated_Implementation(USceneComponent* NewComp, UPrimitiveComponent* HitComp, AActor* SpawnedBy, AController* InstigatedBy) const
{
	UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(NewComp);
	if (Prim != NULL)
	{
		if (WorldTimeParam != NAME_None)
		{
			for (int32 i = Prim->GetNumMaterials() - 1; i >= 0; i--)
			{
				if (Prim->GetMaterial(i) != NULL)
				{
					UMaterialInstanceDynamic* MID = Prim->CreateAndSetMaterialInstanceDynamic(i);
					MID->SetScalarParameterValue(WorldTimeParam, Prim->GetWorld()->TimeSeconds);
				}
			}
		}
		if (Prim->IsA(UParticleSystemComponent::StaticClass()))
		{
			((UParticleSystemComponent*)Prim)->bAutoDestroy = true;
			((UParticleSystemComponent*)Prim)->SecondsBeforeInactive = 0.0f;
		}
		else if (Prim->IsA(UAudioComponent::StaticClass()))
		{
			((UAudioComponent*)Prim)->bAutoDestroy = true;
		}
	}
	else
	{
		UDecalComponent* Decal = Cast<UDecalComponent>(NewComp);
		if (Decal != NULL)
		{
			Decal->RelativeScale3D.X = 1.0f; // this prevents stretching around corners
			if (bRandomizeDecalRoll)
			{
				Decal->RelativeRotation.Roll += 360.0f * FMath::FRand();
			}
			if (HitComp != NULL && HitComp->Mobility == EComponentMobility::Movable)
			{
				Decal->bAbsoluteScale = true;
				Decal->AttachTo(HitComp, NAME_None, EAttachLocation::KeepWorldPosition);
			}
			Decal->UpdateComponentToWorld();
		}
	}

	// unused, see header comment
	return true;
}

void AUTImpactEffect::CreateEffectComponents(UWorld* World, const FTransform& BaseTransform, UPrimitiveComponent* HitComp, AActor* SpawnedBy, AController* InstigatedBy, USceneComponent* CurrentAttachment, FName TemplateName, const TArray<USceneComponent*>& NativeCompList, const TArray<USCS_Node*>& BPNodes) const
{
	AUTWorldSettings* WS = Cast<AUTWorldSettings>(World->GetWorldSettings());
	for (int32 i = 0; i < NativeCompList.Num(); i++)
	{
		if (NativeCompList[i]->AttachParent == CurrentAttachment && ShouldCreateComponent(NativeCompList[i], NativeCompList[i]->GetFName(), BaseTransform, HitComp, SpawnedBy, InstigatedBy))
		{
			USceneComponent* NewComp = ConstructObject<USceneComponent>(NativeCompList[i]->GetClass(), World, NAME_None, RF_NoFlags, NativeCompList[i]);
			NewComp->AttachParent = NULL;
			NewComp->AttachChildren.Empty();
			UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(NewComp);
			if (Prim != NULL)
			{
				Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
			NewComp->RegisterComponentWithWorld(World);
			if (CurrentAttachment != NULL)
			{
				NewComp->AttachTo(CurrentAttachment, NewComp->AttachSocketName);
			}
			if (CurrentAttachment == NULL || CurrentAttachment == HitComp)
			{
				NewComp->SetWorldTransform(FTransform(NewComp->RelativeRotation, NewComp->RelativeLocation, NewComp->RelativeScale3D) * BaseTransform);
			}
			ComponentCreated(NewComp, HitComp, SpawnedBy, InstigatedBy);
			if (WS != NULL)
			{
				WS->AddImpactEffect(NewComp);
			}
			// recurse
			CreateEffectComponents(World, BaseTransform, HitComp, SpawnedBy, InstigatedBy, NewComp, NativeCompList[i]->GetFName(), NativeCompList, BPNodes);
		}
	}
	for (int32 i = 0; i < BPNodes.Num(); i++)
	{
		if (Cast<USceneComponent>(BPNodes[i]->ComponentTemplate) != NULL && BPNodes[i]->ParentComponentOrVariableName == TemplateName && ShouldCreateComponent((USceneComponent*)BPNodes[i]->ComponentTemplate, TemplateName, BaseTransform, HitComp, SpawnedBy, InstigatedBy))
		{
			USceneComponent* NewComp = ConstructObject<USceneComponent>(BPNodes[i]->ComponentTemplate->GetClass(), World, NAME_None, RF_NoFlags, BPNodes[i]->ComponentTemplate);
			NewComp->AttachParent = NULL;
			NewComp->AttachChildren.Empty();
			UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(NewComp);
			if (Prim != NULL)
			{
				Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
			NewComp->RegisterComponentWithWorld(World);
			if (CurrentAttachment != NULL)
			{
				NewComp->AttachTo(CurrentAttachment, BPNodes[i]->AttachToName);
			}
			if (CurrentAttachment == NULL || CurrentAttachment == HitComp)
			{
				NewComp->SetWorldTransform(FTransform(NewComp->RelativeRotation, NewComp->RelativeLocation, NewComp->RelativeScale3D) * BaseTransform);
			}
			ComponentCreated(NewComp, HitComp, SpawnedBy, InstigatedBy);
			if (WS != NULL)
			{
				WS->AddImpactEffect(NewComp);
			}
			// recurse
			CreateEffectComponents(World, BaseTransform, HitComp, SpawnedBy, InstigatedBy, NewComp, BPNodes[i]->VariableName, NativeCompList, BPNodes);
		}
	}
}
