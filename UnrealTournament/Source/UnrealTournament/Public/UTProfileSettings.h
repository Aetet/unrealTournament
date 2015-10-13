// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
//
// These are settings that are stored in a remote MCP managed profile.  A copy of them are also stored in the user folder on the local machine
// in case of MCP failure or downtime.

#pragma once

#include "GameFramework/InputSettings.h"
#include "UTPlayerInput.h"
#include "UTPlayerController.h"
#include "UTProfileSettings.generated.h"

static const uint32 VALID_PROFILESETTINGS_VERSION = 6;
static const uint32 EMOTE_TO_TAUNT_PROFILESETTINGS_VERSION = 6;
static const uint32 TAUNTFIXUP_PROFILESETTINGS_VERSION = 7;
static const uint32 SPECTATING_FIXUP_PROFILESETTINGS_VERSION = 8;
static const uint32 SLIDEFROMRUN_FIXUP_PROFILESETTINGS_VERSION = 8;
static const uint32 HEARTAUNTS_FIXUP_PROFILESETTINGS_VERSION = 10;
static const uint32 PAUSEKEY_FIXUP_PROFILESETTINGS_VERSION = 11;
static const uint32 CURRENT_PROFILESETTINGS_VERSION = 12;

namespace AchievementIDs
{
	extern const FName TutorialComplete;
	extern const FName ChallengeStars5;
	extern const FName ChallengeStars15;
	extern const FName ChallengeStars25;
	extern const FName ChallengeStars35;
	extern const FName ChallengeStars45;
	extern const FName PumpkinHead2015;
	extern const FName ChallengePumpkins5;
	extern const FName ChallengePumpkins10;
	extern const FName ChallengePumpkins15;
};

class UUTLocalPlayer;

USTRUCT()
struct FStoredWeaponPriority
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString WeaponClassName;

	UPROPERTY()
	float WeaponPriority;

	FStoredWeaponPriority()
		: WeaponClassName(TEXT(""))
		, WeaponPriority(0.0)
	{};

	FStoredWeaponPriority(FString inWeaponClassName, float inWeaponPriority)
		: WeaponClassName(inWeaponClassName)
		, WeaponPriority(inWeaponPriority)
	{};
};

UCLASS()
class UNREALTOURNAMENT_API UUTProfileSettings : public UObject
{
	GENERATED_UCLASS_BODY()

	void ClearWeaponPriorities();
	void SetWeaponPriority(FString WeaponClassName, float NewPriority);
	float GetWeaponPriority(FString WeaponClassName, float DefaultPriority);

	bool HasTokenBeenPickedUpBefore(FName TokenUniqueID);
	void TokenPickedUp(FName TokenUniqueID);
	void TokenRevoke(FName TokenUniqueID);
	void TokensCommit();
	void TokensReset();

	bool GetBestTime(FName TimingName, float& OutBestTime);
	void SetBestTime(FName TimingName, float InBestTime);
	
	// debug only
	void TokensClear();

	/**
	 *	Gather all of the settings so that this profile object can be saved.
	 **/
	void GatherAllSettings(UUTLocalPlayer* ProfilePlayer);

	void VersionFixup();

	/**
	 *	return the current player name.
	 **/
	FString GetPlayerName() { return PlayerName; };

	/**
	 *	Apply any settings stored in this profile object
	 **/
	void ApplyAllSettings(UUTLocalPlayer* ProfilePlayer);

	/**
	 *	Versioning
	 **/
	UPROPERTY()
	uint32 SettingsRevisionNum;

	UPROPERTY()
	FString HatPath;
	UPROPERTY()
	FString LeaderHatPath;
	UPROPERTY()
	int32 HatVariant;
	UPROPERTY()
	FString EyewearPath;
	UPROPERTY()
	int32 EyewearVariant;
	UPROPERTY()
	FString TauntPath;
	UPROPERTY()
	FString Taunt2Path;

	UPROPERTY()
	FString CharacterPath;

	UPROPERTY()
	FName CountryFlag;

	UPROPERTY()
	TArray<FCrosshairInfo> CrosshairInfos;

	UPROPERTY()
	bool bCustomWeaponCrosshairs;

	bool bNeedProfileWriteOnLevelChange;
	
	UPROPERTY()
	uint32 ReplayScreenshotResX;

	UPROPERTY()
	uint32 ReplayScreenshotResY;
	
	UPROPERTY()
	bool bReplayCustomPostProcess;

	UPROPERTY()
	float ReplayCustomBloomIntensity;
	
	UPROPERTY()
	float ReplayCustomDOFAmount;

	UPROPERTY()
	float ReplayCustomDOFDistance;

	UPROPERTY()
	float ReplayCustomDOFScale;

	UPROPERTY()
	float ReplayCustomDOFNearBlur;

	UPROPERTY()
	float ReplayCustomDOFFarBlur;

	UPROPERTY()
	float ReplayCustomMotionBlurAmount;

	UPROPERTY()
	float ReplayCustomMotionBlurMax;

	UPROPERTY()
	TArray<FName> Achievements;

	/** local XP, not synced with backend - granted for local play and untrusted servers */
	UPROPERTY()
	int32 LocalXP;

	UPROPERTY()
	FName Avatar;

	UPROPERTY()
	TArray<FUTChallengeResult> ChallengeResults;

	UPROPERTY()
	int32 TotalChallengeStars;

	UPROPERTY()
	int32 SkullCount;

protected:

	/**
	 *	Profiles settings go here.  Any standard UPROPERY is supported.
	 **/

	// What is the Player name associated with this profile
	UPROPERTY()
	FString PlayerName;

	// The UInputSettings object converted in to raw data for storage.
	UPROPERTY()
	TArray<uint8> RawInputSettings;

	UPROPERTY()
	TArray<struct FInputActionKeyMapping> ActionMappings;

	UPROPERTY()
	TArray<struct FInputAxisKeyMapping> AxisMappings;

	UPROPERTY()
	TArray<struct FInputAxisConfigEntry> AxisConfig;

	UPROPERTY()
	TArray<FCustomKeyBinding> CustomBinds;

	UPROPERTY()
	uint32 bEnableMouseSmoothing:1;

	UPROPERTY()
	uint32 bEnableFOVScaling:1;

	UPROPERTY()
	uint32 bInvertMouse;

	UPROPERTY()
	float MouseAcceleration;

	UPROPERTY()
	float MouseAccelerationPower;

	UPROPERTY()
	float MouseAccelerationMax;

	UPROPERTY()
	float FOVScale;

	UPROPERTY()
	float DoubleClickTime;

	UPROPERTY()
	float MouseSensitivity;

	UPROPERTY()
	float MaxDodgeClickTimeValue;

	UPROPERTY()
	float MaxDodgeTapTimeValue;
	
	UPROPERTY()
	uint32 bSingleTapWallDodge:1;

	UPROPERTY()
	uint32 bSingleTapAfterJump : 1;

	UPROPERTY()
	uint32 bAllowSlideFromRun : 1;

	UPROPERTY()
	uint32 bHearsTaunts : 1;

	UPROPERTY()
	FKey ConsoleKey;

	UPROPERTY()
	uint32 bAutoWeaponSwitch:1;

	UPROPERTY()
	float WeaponBob;

	UPROPERTY()
	float ViewBob;

	UPROPERTY()
	TEnumAsByte<EWeaponHand> WeaponHand;

	UPROPERTY()
	FLinearColor FFAPlayerColor;

	/** Holds a list of weapon class names (as string) and weapon switch priorities. - NOTE: this will only show priorities of those weapon the player has "seen" and are stored as "WeaponName:####" */
	UPROPERTY()
	TArray<FStoredWeaponPriority> WeaponPriorities;

	UPROPERTY()
	float PlayerFOV;

	// Linear list of token unique ids for serialization
	UPROPERTY()
	TArray<FName> FoundTokenUniqueIDs;
	
	TArray<FName> TempFoundTokenUniqueIDs;

	UPROPERTY()
	TMap<FName, float> BestTimes;

	// If true, then the player will not show toasts in game.
	UPROPERTY()
	uint32 bSuppressToastsInGame : 1;
};