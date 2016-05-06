// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "UTSecurityCameraComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UNREALTOURNAMENT_API UUTSecurityCameraComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UUTSecurityCameraComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SecurityCam")
		float DetectionRadius;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SecurityCam")
		bool bCameraEnabled;

	UPROPERTY(BlueprintReadOnly, Category = "SecurityCam")
		class AUTCharacter* DetectedFlagCarrier;

	UPROPERTY(BlueprintReadOnly, Category = "SecurityCam")
		class AUTCarriedObject* DetectedFlag;


	// How long a enemy ping is valid
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SecurityCam")
		float CameraPingedDuration;

	/** Event called when a flag carrier is detected that wasn't previously tracked. */
	UFUNCTION(BlueprintImplementableEvent, Category = "SecurityCam")
		void OnFlagCarrierDetected(class AUTCharacter* FlagCarrier);

	/** Event called when a flag carrier that was detected is no longer tracked. */
	UFUNCTION(BlueprintImplementableEvent, Category = "SecurityCam")
		void OnFlagCarrierDetectionLost(class AUTCharacter* FlagCarrier);
};


