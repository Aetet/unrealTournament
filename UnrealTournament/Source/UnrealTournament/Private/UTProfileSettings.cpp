// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTProfileSettings.h"
#include "UTPlayerInput.h"
#include "GameFramework/InputSettings.h"

const FName AchievementIDs::TutorialComplete(TEXT("TutorialComplete"));
const FName AchievementIDs::ChallengeStars5(TEXT("ChallengeStars5"));
const FName AchievementIDs::ChallengeStars15(TEXT("ChallengeStars15"));
const FName AchievementIDs::ChallengeStars25(TEXT("ChallengeStars25"));
const FName AchievementIDs::ChallengeStars35(TEXT("ChallengeStars35"));
const FName AchievementIDs::ChallengeStars45(TEXT("ChallengeStars45"));
const FName AchievementIDs::PumpkinHead2015Level1(TEXT("PumpkinHead2015Level1"));
const FName AchievementIDs::PumpkinHead2015Level2(TEXT("PumpkinHead2015Level2"));
const FName AchievementIDs::PumpkinHead2015Level3(TEXT("PumpkinHead2015Level3"));
const FName AchievementIDs::ChallengePumpkins5(TEXT("ChallengePumpkins5"));
const FName AchievementIDs::ChallengePumpkins10(TEXT("ChallengePumpkins10"));
const FName AchievementIDs::ChallengePumpkins15(TEXT("ChallengePumpkins15"));
const FName AchievementIDs::FacePumpkins(TEXT("FacePumpkins"));

UUTProfileSettings::UUTProfileSettings(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	PlayerName = TEXT("Player");
	bSuppressToastsInGame = false;

	bNeedProfileWriteOnLevelChange = false;

	ReplayCustomBloomIntensity = 0.2f;
	ReplayCustomDOFScale = 1.0f;

	Avatar = FName("UT.Avatar.0");
}

void UUTProfileSettings::ClearWeaponPriorities()
{
	WeaponPriorities.Empty();
}

void UUTProfileSettings::VersionFixup()
{
	if (SettingsRevisionNum <= EMOTE_TO_TAUNT_PROFILESETTINGS_VERSION)
	{
		for (auto Iter = ActionMappings.CreateIterator(); Iter; ++Iter)
		{
			if (Iter->ActionName == FName(TEXT("PlayEmote1")))
			{
				Iter->ActionName = FName(TEXT("PlayTaunt"));
			}
			else if (Iter->ActionName == FName(TEXT("PlayEmote2")))
			{
				Iter->ActionName = FName(TEXT("PlayTaunt2"));
			}
			else if (Iter->ActionName == FName(TEXT("PlayEmote3")))
			{
				ActionMappings.RemoveAt(Iter.GetIndex());
			}
		}
	}
	if (SettingsRevisionNum <= TAUNTFIXUP_PROFILESETTINGS_VERSION)
	{
		FInputActionKeyMapping Taunt2;
		Taunt2.ActionName = FName(TEXT("PlayTaunt2"));
		Taunt2.Key = EKeys::K;
		ActionMappings.AddUnique(Taunt2);
	}
	if (SettingsRevisionNum <= SLIDEFROMRUN_FIXUP_PROFILESETTINGS_VERSION)
	{
		bAllowSlideFromRun = true;
	}
	if (SettingsRevisionNum <= HEARTAUNTS_FIXUP_PROFILESETTINGS_VERSION)
	{
		bHearsTaunts = true;
	}
	if (SettingsRevisionNum <= PAUSEKEY_FIXUP_PROFILESETTINGS_VERSION)
	{
		new(CustomBinds) FCustomKeyBinding(EKeys::Pause.GetFName(), IE_Pressed, TEXT("Pause"));
	}
}

void UUTProfileSettings::SetWeaponPriority(FString WeaponClassName, float NewPriority)
{
	for (int32 i=0;i<WeaponPriorities.Num(); i++)
	{
		if (WeaponPriorities[i].WeaponClassName == WeaponClassName)
		{
			if (WeaponPriorities[i].WeaponPriority != NewPriority)
			{
				WeaponPriorities[i].WeaponPriority = NewPriority;
			}
			break;
		}
	}

	WeaponPriorities.Add(FStoredWeaponPriority(WeaponClassName, NewPriority));
}

float UUTProfileSettings::GetWeaponPriority(FString WeaponClassName, float DefaultPriority)
{
	for (int32 i=0;i<WeaponPriorities.Num(); i++)
	{
		if (WeaponPriorities[i].WeaponClassName == WeaponClassName)
		{
			return WeaponPriorities[i].WeaponPriority;
		}
	}

	return DefaultPriority;
}


void UUTProfileSettings::GatherAllSettings(UUTLocalPlayer* ProfilePlayer)
{
	bSuppressToastsInGame = ProfilePlayer->bSuppressToastsInGame;
	PlayerName = ProfilePlayer->GetNickname();
	
	// Get all settings from the Player Controller
	AUTPlayerController* PC = Cast<AUTPlayerController>(ProfilePlayer->PlayerController);
	if (PC == NULL)
	{
		PC = AUTPlayerController::StaticClass()->GetDefaultObject<AUTPlayerController>();
	}

	if (PC != NULL)
	{
		MaxDodgeClickTimeValue = PC->MaxDodgeClickTime;
		MaxDodgeTapTimeValue = PC->MaxDodgeTapTime;
		bSingleTapWallDodge = PC->bSingleTapWallDodge;
		bSingleTapAfterJump = PC->bSingleTapAfterJump;
		bAutoWeaponSwitch = PC->bAutoWeaponSwitch;
		bAllowSlideFromRun = PC->bAllowSlideFromRun;
		bHearsTaunts = PC->bHearsTaunts;
		WeaponBob = PC->WeaponBobGlobalScaling;
		ViewBob = PC->EyeOffsetGlobalScaling;
		WeaponHand = PC->GetPreferredWeaponHand();
		FFAPlayerColor = PC->FFAPlayerColor;

		PlayerFOV = PC->ConfigDefaultFOV;

		// Get any settings from UTPlayerInput
		UUTPlayerInput* UTPlayerInput = Cast<UUTPlayerInput>(PC->PlayerInput);
		if (UTPlayerInput == NULL)
		{
			UTPlayerInput = UUTPlayerInput::StaticClass()->GetDefaultObject<UUTPlayerInput>();
		}
		if (UTPlayerInput)
		{
			MouseAccelerationPower = UTPlayerInput->AccelerationPower;
			MouseAcceleration = UTPlayerInput->Acceleration;
			MouseAccelerationMax = UTPlayerInput->AccelerationMax;

			CustomBinds.Empty();
			for (int32 i = 0; i < UTPlayerInput->CustomBinds.Num(); i++)
			{
				CustomBinds.Add(UTPlayerInput->CustomBinds[i]);
			}
			//MouseSensitivity = UTPlayerInput->GetMouseSensitivity();
		}
	}

	// Grab the various settings from the InputSettings object.

	UInputSettings* DefaultInputSettingsObject = UInputSettings::StaticClass()->GetDefaultObject<UInputSettings>();
	if (DefaultInputSettingsObject)
	{
		ActionMappings.Empty();
		for (int32 i = 0; i < DefaultInputSettingsObject->ActionMappings.Num(); i++)
		{
			ActionMappings.Add(DefaultInputSettingsObject->ActionMappings[i]);
		}

		AxisMappings.Empty();
		for (int32 i = 0; i < DefaultInputSettingsObject->AxisMappings.Num(); i++)
		{
			AxisMappings.Add(DefaultInputSettingsObject->AxisMappings[i]);
		}

		AxisConfig.Empty();
		for (int32 i = 0; i < DefaultInputSettingsObject->AxisConfig.Num(); i++)
		{
			AxisConfig.Add(DefaultInputSettingsObject->AxisConfig[i]);
			if (AxisConfig[i].AxisKeyName == EKeys::MouseY && AxisConfig[i].AxisProperties.bInvert)
			{
				bInvertMouse = true;
			}
		}

		bEnableMouseSmoothing = DefaultInputSettingsObject->bEnableMouseSmoothing;
		bEnableFOVScaling = DefaultInputSettingsObject->bEnableFOVScaling;
		FOVScale = DefaultInputSettingsObject->FOVScale;
		DoubleClickTime = DefaultInputSettingsObject->DoubleClickTime;

		if (DefaultInputSettingsObject->ConsoleKeys.Num() > 0)
		{
			ConsoleKey = DefaultInputSettingsObject->ConsoleKeys[0];
		}
	}
}
void UUTProfileSettings::ApplyAllSettings(UUTLocalPlayer* ProfilePlayer)
{
	ProfilePlayer->bSuppressToastsInGame = bSuppressToastsInGame;
	ProfilePlayer->SetNickname(PlayerName);
	ProfilePlayer->SaveConfig();

	// Get all settings from the Player Controller
	AUTPlayerController* PC = Cast<AUTPlayerController>(ProfilePlayer->PlayerController);
	if (PC == NULL)
	{
		PC = AUTPlayerController::StaticClass()->GetDefaultObject<AUTPlayerController>();
	}

	if (PC != NULL)
	{
		PC->MaxDodgeClickTime = MaxDodgeClickTimeValue;
		PC->MaxDodgeTapTime = MaxDodgeTapTimeValue;
		PC->bSingleTapWallDodge = bSingleTapWallDodge;
		PC->bSingleTapAfterJump = bSingleTapAfterJump;
		PC->bAutoWeaponSwitch = bAutoWeaponSwitch;
		PC->bAllowSlideFromRun = bAllowSlideFromRun;
		PC->bHearsTaunts = bHearsTaunts;
		PC->WeaponBobGlobalScaling = WeaponBob;
		PC->EyeOffsetGlobalScaling = ViewBob;
		PC->SetWeaponHand(WeaponHand);
		PC->FFAPlayerColor = FFAPlayerColor;
		PC->ConfigDefaultFOV = PlayerFOV;

		PC->SaveConfig();
		// Get any settings from UTPlayerInput
		UUTPlayerInput* UTPlayerInput = Cast<UUTPlayerInput>(PC->PlayerInput);
		if (UTPlayerInput == NULL)
		{
			UTPlayerInput = UUTPlayerInput::StaticClass()->GetDefaultObject<UUTPlayerInput>();

			UTPlayerInput->AccelerationPower = MouseAccelerationPower;
			UTPlayerInput->Acceleration = MouseAcceleration;
			UTPlayerInput->AccelerationMax = MouseAccelerationMax;
		}
		if (UTPlayerInput && CustomBinds.Num() > 0)
		{
			UTPlayerInput->CustomBinds.Empty();
			for (int32 i = 0; i < CustomBinds.Num(); i++)
			{
				UTPlayerInput->CustomBinds.Add(CustomBinds[i]);
			}
			//UTPlayerInput->SetMouseSensitivity(MouseSensitivity);
			UTPlayerInput->SaveConfig();
		}

		AUTCharacter* C = Cast<AUTCharacter>(PC->GetPawn());
		if (C != NULL)
		{
			C->NotifyTeamChanged();
		}
	}

	// Save all settings to the UInputSettings object
	UInputSettings* DefaultInputSettingsObject = UInputSettings::StaticClass()->GetDefaultObject<UInputSettings>();
	if (DefaultInputSettingsObject)
	{
		if (ActionMappings.Num() > 0)
		{
			DefaultInputSettingsObject->ActionMappings.Empty();
			for (int32 i = 0; i < ActionMappings.Num(); i++)
			{
				DefaultInputSettingsObject->ActionMappings.Add(ActionMappings[i]);
			}
		}

		if (AxisConfig.Num() > 0)
		{
			DefaultInputSettingsObject->AxisConfig.Empty();
			for (int32 i = 0; i < AxisConfig.Num(); i++)
			{
				DefaultInputSettingsObject->AxisConfig.Add(AxisConfig[i]);
				if (bInvertMouse && AxisConfig[i].AxisKeyName == EKeys::MouseY)
				{
					AxisConfig[i].AxisProperties.bInvert = true;
				}
			}
		}

		if (AxisMappings.Num() > 0)
		{
			DefaultInputSettingsObject->AxisMappings.Empty();
			for (int32 i = 0; i < AxisMappings.Num(); i++)
			{
				DefaultInputSettingsObject->AxisMappings.Add(AxisMappings[i]);
			}
		}

		DefaultInputSettingsObject->bEnableMouseSmoothing = bEnableMouseSmoothing;
		DefaultInputSettingsObject->bEnableFOVScaling = bEnableFOVScaling;
		DefaultInputSettingsObject->FOVScale = FOVScale;
		DefaultInputSettingsObject->DoubleClickTime = DoubleClickTime;
		DefaultInputSettingsObject->ConsoleKeys.Empty();
		DefaultInputSettingsObject->ConsoleKeys.Add(ConsoleKey);
		DefaultInputSettingsObject->SaveConfig();
	}

	if (ProfilePlayer->PlayerController != NULL && ProfilePlayer->PlayerController->PlayerInput != NULL)
	{
		// make sure default object mirrors live object
		ProfilePlayer->PlayerController->PlayerInput->GetClass()->GetDefaultObject()->ReloadConfig();

		UUTPlayerInput* UTPlayerInput = Cast<UUTPlayerInput>(ProfilePlayer->PlayerController->PlayerInput);
		if (UTPlayerInput != NULL)
		{
			UTPlayerInput->UTForceRebuildingKeyMaps(true);
		}
		else
		{
			ProfilePlayer->PlayerController->PlayerInput->ForceRebuildingKeyMaps(true);
		}
	}
	UUTPlayerInput::StaticClass()->GetDefaultObject<UUTPlayerInput>()->ForceRebuildingKeyMaps(true);

	//Don't overwrite default crosshair info if nothing has been saved to the profile yet
	if (CrosshairInfos.Num() > 0)
	{
		//Copy crosshair settings to every hud class
		for (TObjectIterator<AUTHUD> It(EObjectFlags::RF_NoFlags, true); It; ++It)
		{
			AUTHUD* Hud = *It;
			if (Hud != nullptr)
			{
				Hud->LoadedCrosshairs.Empty();
				Hud->CrosshairInfos = CrosshairInfos;
				Hud->bCustomWeaponCrosshairs = bCustomWeaponCrosshairs;
				Hud->SaveConfig();
			}
		}
	}

	ProfilePlayer->SetCharacterPath(CharacterPath);
	ProfilePlayer->SetHatPath(HatPath);
	ProfilePlayer->SetLeaderHatPath(LeaderHatPath);
	ProfilePlayer->SetEyewearPath(EyewearPath);
	ProfilePlayer->SetTauntPath(TauntPath);
	ProfilePlayer->SetTaunt2Path(Taunt2Path);
	ProfilePlayer->SetHatVariant(HatVariant);
	ProfilePlayer->SetEyewearVariant(EyewearVariant);

	TokensCommit();
}

bool UUTProfileSettings::HasTokenBeenPickedUpBefore(FName TokenUniqueID)
{
	return FoundTokenUniqueIDs.Contains(TokenUniqueID);
}

void UUTProfileSettings::TokenPickedUp(FName TokenUniqueID)
{
	TempFoundTokenUniqueIDs.AddUnique(TokenUniqueID);
}

void UUTProfileSettings::TokenRevoke(FName TokenUniqueID)
{
	TempFoundTokenUniqueIDs.Remove(TokenUniqueID);
}

void UUTProfileSettings::TokensCommit()
{
	for (FName ID : TempFoundTokenUniqueIDs)
	{
		FoundTokenUniqueIDs.AddUnique(ID);
		bNeedProfileWriteOnLevelChange = true;
	}

	// see if all achievement tokens have been picked up
	if (!Achievements.Contains(AchievementIDs::TutorialComplete))
	{
		bool bCompletedTutorial = true;
		static TArray<FName, TInlineAllocator<60>> TutorialTokens = []()
		{
			TArray<FName, TInlineAllocator<60>> List;
			FNumberFormattingOptions Options;
			Options.MinimumIntegralDigits = 3;
			for (int32 i = 0; i < 15; i++)
			{
				List.Add(FName(*FString::Printf(TEXT("movementtraining_token_%s"), *FText::AsNumber(i, &Options).ToString())));
			}
			for (int32 i = 0; i < 15; i++)
			{
				List.Add(FName(*FString::Printf(TEXT("weapontraining_token_%s"), *FText::AsNumber(i, &Options).ToString())));
			}
			for (int32 i = 0; i < 10; i++)
			{
				List.Add(FName(*FString::Printf(TEXT("pickuptraining_token_%s"), *FText::AsNumber(i, &Options).ToString())));
			}
			for (int32 i = 0; i < 5; i++)
			{
				List.Add(FName(*FString::Printf(TEXT("tuba_token_%s"), *FText::AsNumber(i, &Options).ToString())));
			}
			for (int32 i = 0; i < 5; i++)
			{
				List.Add(FName(*FString::Printf(TEXT("outpost23_token_%s"), *FText::AsNumber(i, &Options).ToString())));
			}
			for (int32 i = 0; i < 5; i++)
			{
				List.Add(FName(*FString::Printf(TEXT("face_token_%s"), *FText::AsNumber(i, &Options).ToString())));
			}
			for (int32 i = 0; i < 5; i++)
			{
				List.Add(FName(*FString::Printf(TEXT("asdf_token_%s"), *FText::AsNumber(i, &Options).ToString())));
			}
			return List;
		}();
		for (FName TestToken : TutorialTokens)
		{
			if (!FoundTokenUniqueIDs.Contains(TestToken))
			{
				bCompletedTutorial = false;
				break;
			}
		}
		if (bCompletedTutorial)
		{
			Achievements.Add(AchievementIDs::TutorialComplete);
			// TODO: toast
		}
	}

	TempFoundTokenUniqueIDs.Empty();
}

void UUTProfileSettings::TokensReset()
{
	TempFoundTokenUniqueIDs.Empty();
}

void UUTProfileSettings::TokensClear()
{
	TempFoundTokenUniqueIDs.Empty();
	FoundTokenUniqueIDs.Empty();
}

bool UUTProfileSettings::GetBestTime(FName TimingName, float& OutBestTime)
{
	OutBestTime = 0;

	float* BestTime = BestTimes.Find(TimingName);
	if (BestTime)
	{
		OutBestTime = *BestTime;
		return true;
	}

	return false;
}

void UUTProfileSettings::SetBestTime(FName TimingName, float InBestTime)
{
	BestTimes.Add(TimingName, InBestTime);
	bNeedProfileWriteOnLevelChange = true;

	// hacky halloween reward implementation
	if (TimingName == AchievementIDs::FacePumpkins && InBestTime >= 6666.0f)
	{
		for (TObjectIterator<UUTLocalPlayer> It; It; ++It)
		{
			if (It->GetProfileSettings() == this)
			{
				It->AwardAchievement(AchievementIDs::FacePumpkins);
			}
		}
	}
}