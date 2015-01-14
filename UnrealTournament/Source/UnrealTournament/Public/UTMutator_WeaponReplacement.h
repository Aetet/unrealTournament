// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTMutator.h"

#include "UTMutator_WeaponReplacement.generated.h"

USTRUCT(BlueprintType)
struct FReplacementInfo
{
	GENERATED_USTRUCT_BODY()
	/** class name of the weapon we want to get rid of */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Misc")
	FString OldClassPath;
	/** fully qualified path of the class to replace it with */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Misc")
	FString NewClassPath;
};

UCLASS(Blueprintable, Meta = (ChildCanTick), Config = Game)
class AUTMutator_WeaponReplacement : public AUTMutator
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintReadWrite, Config)
	TArray<FReplacementInfo> WeaponsToReplace;

	virtual void BeginPlay() override;
	void ModifyPlayer_Implementation(APawn* Other) override;
	bool CheckRelevance_Implementation(AActor* Other) override;
};