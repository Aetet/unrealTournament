// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTPlayerCameraManager.h"
#include "UTCTFFlagBase.h"
#include "UTViewPlaceholder.h"

AUTPlayerCameraManager::AUTPlayerCameraManager(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FreeCamOffset = FVector(-256, 0, 90);
	EndGameFreeCamOffset = FVector(-256, 0, 45);
	EndGameFreeCamDistance = 55.0f;
	FlagBaseFreeCamOffset = FVector(0, 0, 90);
	bUseClientSideCameraUpdates = false;

	DefaultPPSettings.SetBaseValues();
	DefaultPPSettings.bOverride_AmbientCubemapIntensity = true;
	DefaultPPSettings.bOverride_BloomIntensity = true;
	DefaultPPSettings.bOverride_BloomDirtMaskIntensity = true;
	DefaultPPSettings.bOverride_AutoExposureMinBrightness = true;
	DefaultPPSettings.bOverride_AutoExposureMaxBrightness = true;
	DefaultPPSettings.bOverride_LensFlareIntensity = true;
	DefaultPPSettings.bOverride_MotionBlurAmount = true;
	DefaultPPSettings.bOverride_AntiAliasingMethod = true;
	DefaultPPSettings.bOverride_ScreenSpaceReflectionIntensity = true;
	DefaultPPSettings.BloomIntensity = 0.20f;
	DefaultPPSettings.BloomDirtMaskIntensity = 0.0f;
	DefaultPPSettings.AutoExposureMinBrightness = 1.0f;
	DefaultPPSettings.AutoExposureMaxBrightness = 1.0f;
	DefaultPPSettings.VignetteIntensity = 0.20f;
	DefaultPPSettings.MotionBlurAmount = 0.0f;
	DefaultPPSettings.AntiAliasingMethod = AAM_FXAA;
	DefaultPPSettings.ScreenSpaceReflectionIntensity = 0.0f;

	StylizedPPSettings.AddZeroed();
	StylizedPPSettings[0].SetBaseValues();
	StylizedPPSettings[0].bOverride_FilmWhitePoint = true;
	StylizedPPSettings[0].bOverride_SceneFringeIntensity = true;
	StylizedPPSettings[0].bOverride_AmbientCubemapTint = true;
	StylizedPPSettings[0].bOverride_AmbientCubemapIntensity = true;
	StylizedPPSettings[0].bOverride_BloomIntensity = true;
	StylizedPPSettings[0].bOverride_BloomThreshold = true;
	StylizedPPSettings[0].bOverride_BloomDirtMaskIntensity = true;
	StylizedPPSettings[0].bOverride_BloomDirtMask = true;
	StylizedPPSettings[0].bOverride_AutoExposureLowPercent = true;
	StylizedPPSettings[0].bOverride_AutoExposureHighPercent = true;
	StylizedPPSettings[0].bOverride_AutoExposureMinBrightness = true;
	StylizedPPSettings[0].bOverride_AutoExposureMaxBrightness = true;
	StylizedPPSettings[0].bOverride_AutoExposureSpeedUp = true;
	StylizedPPSettings[0].bOverride_AutoExposureSpeedDown = true;
	StylizedPPSettings[0].bOverride_AutoExposureBias = true;
	StylizedPPSettings[0].bOverride_LensFlareIntensity = true;
	StylizedPPSettings[0].bOverride_VignetteIntensity = true;
	StylizedPPSettings[0].bOverride_AmbientOcclusionIntensity = true;
	StylizedPPSettings[0].bOverride_AmbientOcclusionStaticFraction = true;
	StylizedPPSettings[0].bOverride_AmbientOcclusionRadius = true;
	StylizedPPSettings[0].bOverride_AmbientOcclusionRadiusInWS = true;
	StylizedPPSettings[0].bOverride_AmbientOcclusionQuality = true;
	StylizedPPSettings[0].bOverride_DepthOfFieldFocalDistance = true;
	StylizedPPSettings[0].bOverride_DepthOfFieldScale = true;
	StylizedPPSettings[0].bOverride_DepthOfFieldMaxBokehSize = true;
	StylizedPPSettings[0].bOverride_DepthOfFieldNearBlurSize = true;
	StylizedPPSettings[0].bOverride_DepthOfFieldFarBlurSize = true;
	StylizedPPSettings[0].bOverride_MotionBlurAmount = true;
	StylizedPPSettings[0].bOverride_MotionBlurMax = true;
	StylizedPPSettings[0].bOverride_AntiAliasingMethod = true;
	StylizedPPSettings[0].bOverride_ScreenSpaceReflectionIntensity = true;
	StylizedPPSettings[0].bOverride_ScreenSpaceReflectionQuality = true;
	StylizedPPSettings[0].bOverride_ScreenSpaceReflectionMaxRoughness = true;
	StylizedPPSettings[0].BloomIntensity = 0.100000;
	StylizedPPSettings[0].AmbientCubemapTint = FLinearColor(0.440000, 0.368895, 0.312400, 1.000000);
	StylizedPPSettings[0].AmbientCubemapIntensity = 0.000000;
	StylizedPPSettings[0].AutoExposureMinBrightness = 1.000000;
	StylizedPPSettings[0].AutoExposureMaxBrightness = 1.000000;
	StylizedPPSettings[0].AutoExposureSpeedUp = 1.000000;
	StylizedPPSettings[0].AutoExposureBias = 3.000000;
	StylizedPPSettings[0].LensFlareIntensity = 0.000000;
	StylizedPPSettings[0].VignetteIntensity = 0.300000;
	StylizedPPSettings[0].GrainIntensity = 0.247788;
	StylizedPPSettings[0].AmbientOcclusionIntensity = 0.000000;
	StylizedPPSettings[0].AmbientOcclusionRadius = 100.000000;
	StylizedPPSettings[0].AmbientOcclusionRadiusInWS = true;
	StylizedPPSettings[0].AmbientOcclusionQuality = 100.000000;
	StylizedPPSettings[0].DepthOfFieldScale = 0.159292;
	StylizedPPSettings[0].DepthOfFieldMaxBokehSize = 24.911505;
	StylizedPPSettings[0].DepthOfFieldNearBlurSize = 17.840708;
	StylizedPPSettings[0].DepthOfFieldFarBlurSize = 30.292036;
	StylizedPPSettings[0].MotionBlurAmount = 0.000000;
	StylizedPPSettings[0].MotionBlurMax = 0.000000;

	LastThirdPersonCameraLoc = FVector(0);
	ThirdPersonCameraSmoothingSpeed = 6.0f;
}

FName AUTPlayerCameraManager::GetCameraStyleWithOverrides() const
{
	static const FName NAME_FreeCam = FName(TEXT("FreeCam"));

	AUTCharacter* UTCharacter = Cast<AUTCharacter>(GetViewTarget());
	AUTViewPlaceholder* UTPlaceholder = Cast<AUTViewPlaceholder>(GetViewTarget());

	if (UTPlaceholder != nullptr)
	{
		return NAME_FreeCam;
	}

	// force third person if target is dead, ragdoll or emoting
	if (UTCharacter != NULL && (UTCharacter->IsDead() || UTCharacter->IsRagdoll() || UTCharacter->EmoteCount > 0))
	{
		return NAME_FreeCam;
	}
	else
	{
		AUTGameState* GameState = GetWorld()->GetGameState<AUTGameState>();
		return (GameState != NULL) ? GameState->OverrideCameraStyle(PCOwner, CameraStyle) : CameraStyle;
	}
}

void AUTPlayerCameraManager::UpdateCamera(float DeltaTime)
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		CameraStyle = NAME_Default;

		LastThirdPersonCameraLoc = FVector(0);
		ViewTarget.CheckViewTarget(PCOwner);
		// our camera is now viewing there
		FMinimalViewInfo NewPOV;
		NewPOV.FOV = DefaultFOV;
		NewPOV.OrthoWidth = DefaultOrthoWidth;
		NewPOV.bConstrainAspectRatio = false;
		NewPOV.ProjectionMode = bIsOrthographic ? ECameraProjectionMode::Orthographic : ECameraProjectionMode::Perspective;
		NewPOV.PostProcessBlendWeight = 1.0f;

		const bool bK2Camera = BlueprintUpdateCamera(ViewTarget.Target, NewPOV.Location, NewPOV.Rotation,NewPOV.FOV);
		if (!bK2Camera)
		{
			ViewTarget.Target->CalcCamera(DeltaTime, NewPOV);
		}

		// Cache results
		FillCameraCache(NewPOV);
	}
	else
	{
		Super::UpdateCamera(DeltaTime);
	}
}

void AUTPlayerCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	static const FName NAME_FreeCam = FName(TEXT("FreeCam"));
	static const FName NAME_GameOver = FName(TEXT("GameOver"));	

	FName SavedCameraStyle = CameraStyle;
	CameraStyle = GetCameraStyleWithOverrides();
	AUTCharacter* UTCharacter = Cast<AUTCharacter>(OutVT.Target);
	AUTCTFFlagBase* UTFlagBase = Cast<AUTCTFFlagBase>(OutVT.Target);
	AUTViewPlaceholder* UTPlaceholder = Cast<AUTViewPlaceholder>(OutVT.Target);

	// smooth third person camera all the time
	if (CameraStyle == NAME_FreeCam)
	{
		OutVT.POV.FOV = DefaultFOV;
		OutVT.POV.OrthoWidth = DefaultOrthoWidth;
		OutVT.POV.bConstrainAspectRatio = false;
		OutVT.POV.ProjectionMode = bIsOrthographic ? ECameraProjectionMode::Orthographic : ECameraProjectionMode::Perspective;
		OutVT.POV.PostProcessBlendWeight = 1.0f;

		FVector DesiredLoc = OutVT.Target->GetActorLocation();

		// we must use the capsule location here as the ragdoll's root component can be rubbbing a wall
		if (UTCharacter != nullptr && UTCharacter->IsRagdoll() && UTCharacter->GetCapsuleComponent() != nullptr)
		{
			DesiredLoc = UTCharacter->GetCapsuleComponent()->GetComponentLocation();
		}

		if (UTFlagBase != nullptr)
		{
			DesiredLoc += FlagBaseFreeCamOffset;
		}

		FRotator Rotator = PCOwner->GetControlRotation();

		FVector Loc = DesiredLoc;
		if (LastThirdPersonCameraLoc != FVector(0))
		{
			Loc = FMath::VInterpTo(LastThirdPersonCameraLoc, DesiredLoc, DeltaTime, ThirdPersonCameraSmoothingSpeed);
		}
		LastThirdPersonCameraLoc = Loc;

		float CameraDistance = FreeCamDistance;
		FVector CameraOffset = FreeCamOffset;
		AUTPlayerController* UTPC = Cast<AUTPlayerController>(PCOwner);
		if (UTPC != nullptr && UTPC->GetStateName() == NAME_GameOver)
		{
			CameraDistance = EndGameFreeCamDistance;
			CameraOffset = EndGameFreeCamOffset;
		}

		FVector Pos = Loc + FRotationMatrix(Rotator).TransformVector(CameraOffset) - Rotator.Vector() * CameraDistance;
		FCollisionQueryParams BoxParams(NAME_FreeCam, false, this);
		BoxParams.AddIgnoredActor(OutVT.Target);
		
		// When viewing a placeholder actor, just don't collide with any pawns
		if (UTPlaceholder != nullptr)
		{
			for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
			{
				BoxParams.AddIgnoredActor(*It);
			}
		}

		FHitResult Result;

		GetWorld()->SweepSingle(Result, Loc, Pos, FQuat::Identity, ECC_Camera, FCollisionShape::MakeBox(FVector(12.f)), BoxParams);
		OutVT.POV.Location = !Result.bBlockingHit ? Pos : Result.Location;
		OutVT.POV.Rotation = Rotator;

		// Synchronize the actor with the view target results
		SetActorLocationAndRotation(OutVT.POV.Location, OutVT.POV.Rotation, false);
	}
	else
	{
		LastThirdPersonCameraLoc = FVector(0);

		Super::UpdateViewTarget(OutVT, DeltaTime);
	}

	CameraStyle = SavedCameraStyle;
}

void AUTPlayerCameraManager::ApplyCameraModifiers(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	Super::ApplyCameraModifiers(DeltaTime, InOutPOV);

	// if no PP volumes, force our default PP in at the beginning
	if (GetWorld()->PostProcessVolumes.Num() == 0)
	{
		PostProcessBlendCache.Insert(DefaultPPSettings, 0);
		PostProcessBlendCacheWeights.Insert(1.0f, 0);
	}

	AUTPlayerController* UTPCOwner = Cast<AUTPlayerController>(PCOwner);
	if (UTPCOwner && UTPCOwner->StylizedPPIndex != INDEX_NONE)
	{
		PostProcessBlendCache.Insert(StylizedPPSettings[UTPCOwner->StylizedPPIndex], 0);
		PostProcessBlendCacheWeights.Insert(1.0f, 0);
	}
}