// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
//
// These are settings that are stored in a remote MCP managed profile.  A copy of them are also stored in the user folder on the local machine
// in case of MCP failure or downtime.

#pragma once

#include "GameFramework/InputSettings.h"
#include "UTPlayerInput.h"
#include "UTPlayerController.h"
#include "UTWeaponSkin.h"
#include "UTProfileSettings.generated.h"

static const uint32 VALID_PROFILESETTINGS_VERSION = 6;
static const uint32 EMOTE_TO_TAUNT_PROFILESETTINGS_VERSION = 6;
static const uint32 TAUNTFIXUP_PROFILESETTINGS_VERSION = 7;
static const uint32 SPECTATING_FIXUP_PROFILESETTINGS_VERSION = 8;
static const uint32 SLIDEFROMRUN_FIXUP_PROFILESETTINGS_VERSION = 8;
static const uint32 HEARTAUNTS_FIXUP_PROFILESETTINGS_VERSION = 10;
static const uint32 PAUSEKEY_FIXUP_PROFILESETTINGS_VERSION = 11;
static const uint32 HUDSETTINGS_FIXUP_PROFILESETTINGS_VERSION = 15;
static const uint32 ACTIVATEPOWERUP_FIXUP_PROFILESETTINGS_VERSION = 16;
static const uint32 BUY_MENU_AND_DROP_FLAG_BUTTON_FIXUP_PROFILE_SETTINGS_VERSION = 17;
static const uint32 SLIDE_FIXUP_PROFILE_SETTINGS_VERSION = 18;
static const uint32 WEAPON_WHEEL_FIXUP_PROFILESETTINGS_VERSION=19;
static const uint32 PUSH_TO_TALK_FIXUP_PROFILESETTINGS_VERSION=20;
static const uint32 CURRENT_PROFILESETTINGS_VERSION = 20;

static const uint32 CHALLENGE_FIXUP_VERSION = 12;


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

USTRUCT()
struct FStoredWeaponGroupInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString WeaponClassName;

	UPROPERTY()
	int32 Group;

	FStoredWeaponGroupInfo()
		: WeaponClassName(TEXT(""))
		, Group(0)
	{
	}

	FStoredWeaponGroupInfo(FString inWeaponClassName, int32 InGroup)
		: WeaponClassName(inWeaponClassName)
		, Group(InGroup)
	{
	}

};

UCLASS()
class UNREALTOURNAMENT_API UUTProfileSettings : public UObject
{
	GENERATED_UCLASS_BODY()

	friend class UUTProgressionStorage;

	void ClearWeaponPriorities();
	void SetWeaponPriority(FString WeaponClassName, float NewPriority);
	float GetWeaponPriority(FString WeaponClassName, float DefaultPriority);

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
	FString MatchmakingRegion;

	UPROPERTY()
	float QuickStatsAngle;

	UPROPERTY()
	float QuickStatsDistance;

	UPROPERTY()
	FName QuickStatsType;

	UPROPERTY()
	float QuickStatsBackgroundAlpha;	

	UPROPERTY()
	float QuickStatsForegroundAlpha;	

	UPROPERTY()
	bool bQuickStatsHidden;

	UPROPERTY()
	bool bQuickStatsBob;

	UPROPERTY()
	float QuickStatsScaleOverride;	


	// the below have been moved to UTProgressionStorage and are only here for backwards compatibility
private:
	UPROPERTY()
	TArray<FName> Achievements;

	UPROPERTY()
	int32 TotalChallengeStars;

	UPROPERTY()
	int32 SkullCount;

	// Linear list of token unique ids for serialization
	UPROPERTY()
	TArray<FName> FoundTokenUniqueIDs;

	TArray<FName> TempFoundTokenUniqueIDs;

	UPROPERTY()
	TMap<FName, float> BestTimes;
public:
	/** UNUSED - REMOVE */
	UPROPERTY()
	int32 LocalXP;

	UPROPERTY()
	FName Avatar;

	UPROPERTY()
	TArray<FUTChallengeResult> ChallengeResults;

	UPROPERTY()
	TArray<FUTDailyChallengeUnlock> UnlockedDailyChallenges;

	UPROPERTY()
	TArray<FStoredWeaponGroupInfo> WeaponGroups;

	// Yes. the WeaponGroups array holds all of this information.  We use a TMap for
	// quick lookup.
	TMap<FString, FStoredWeaponGroupInfo> WeaponGroupLookup;

	UPROPERTY()
	TArray<UUTWeaponSkin*> WeaponSkins;

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
	uint32 bInvertMouse;

	UPROPERTY()
	float MouseAcceleration;

	UPROPERTY()
	float MouseAccelerationPower;

	UPROPERTY()
	float MouseAccelerationMax;

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

	/** For backwards compatibility, maps to bCrouchTriggersSlide. */
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

	// If true, then the player will not show toasts in game.
	UPROPERTY()
	uint32 bSuppressToastsInGame : 1;

public:

	// This is the base HUD opacity level used by HUD Widgets RenderObjects
	UPROPERTY()
	float HUDWidgetOpacity;

	// HUD widgets that have borders will use this opacity value when rendering.
	UPROPERTY()
	float HUDWidgetBorderOpacity;

	// HUD widgets that have background slates will use this opacity value when rendering.
	UPROPERTY()
	float HUDWidgetSlateOpacity;

	// This is a special opacity value used by just the Weapon bar.  When the weapon bar isn't in use, this opacity value will be multipled in
	UPROPERTY()
	float HUDWidgetWeaponbarInactiveOpacity;

	// The weapon bar can get a secondary scale override using this value
	UPROPERTY()
	float HUDWidgetWeaponBarScaleOverride;

	UPROPERTY()
	float HUDWidgetWeaponBarInactiveIconOpacity;

	UPROPERTY()
	float HUDWidgetWeaponBarEmptyOpacity;

	/** Set true to force weapon bar to immediately update. */
	UPROPERTY()
	bool bHUDWeaponBarSettingChanged;

	// Allows the user to override the scaling factor for their hud.
	UPROPERTY()
	float HUDWidgetScaleOverride;

	// Allows the user to override the scaling factor for their hud.
	UPROPERTY()
	float HUDMessageScaleOverride;

	UPROPERTY()
	bool bUseWeaponColors;

	UPROPERTY()
	bool bDrawChatKillMsg;

	UPROPERTY()
	bool bDrawCenteredKillMsg;

	UPROPERTY()
	bool bDrawHUDKillIconMsg;

	UPROPERTY()
	bool bPlayKillSoundMsg;
	
	UPROPERTY()
	float HUDMinimapScale;

	//This is called in the constructor so do not make this virtual / BlueprintImplementable without reworking the constructor to remove it!
	UFUNCTION(BlueprintCallable, Category=Hud)
	void ResetHUD();

	UPROPERTY()
	bool bPushToTalk;


public:
	void UpdateCrosshairs(AUTHUD* HUD);

	// If true, we have forced and epic Employee to use the Epic Logo at least once before they changed it.
	UPROPERTY()
	uint32 bForcedToEpicAtLeastOnce : 1;

	void CopyTokens(TArray<FName>& Destination)
	{
		Destination = FoundTokenUniqueIDs;
	}

	// These slots are used by the weapon wheel menu.  They hold the classname of the weapon in this slot
	UPROPERTY()
	TArray<FString> WeaponWheelQuickSlots;
};
