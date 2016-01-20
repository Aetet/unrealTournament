// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTGameMode.h"

#define SETTING_GAMENAME FName(TEXT("GAMENAME"))
#define SETTING_SERVERNAME FName(TEXT("UT_SERVERNAME"))
#define SETTING_SERVERVERSION FName(TEXT("UT_SERVERVERSION"))
#define SETTING_SERVERMOTD FName(TEXT("UT_SERVERMOTD"))
#define SETTING_PLAYERSONLINE FName(TEXT("UT_PLAYERONLINE"))
#define SETTING_SPECTATORSONLINE FName(TEXT("UT_SPECTATORSONLINE"))
#define SETTING_SERVERFLAGS FName(TEXT("UT_SERVERFLAGS"))
#define SETTING_SERVERINSTANCEGUID FName(TEXT("UT_SERVERINSTANCEGUID"))
#define SETTING_GAMEINSTANCE FName(TEXT("UT_GAMEINSTANCE"))
#define SETTING_NUMMATCHES FName(TEXT("UT_NUMMATCHES"))
#define SETTING_MINELO FName(TEXT("UT_MINELO"))
#define SETTING_MAXELO FName(TEXT("UT_MAXELO"))
#define SETTING_TRAININGGROUND FName(TEXT("UT_TRAININGGROUND"))
#define SETTING_TRUSTLEVEL FName(TEXT("UT_SERVERTRUSTLEVEL"))
#define SETTING_UTMAXPLAYERS FName(TEXT("UT_MAXPLAYERS"))
#define SETTING_UTMAXSPECTATORS FName(TEXT("UT_MAXSPECTATORS"))

// Requires a password to join
#define SERVERFLAG_RequiresPassword 0x00000001

// This server is restricted for some reason.  
#define SERVERFLAG_Restricted 0x00000002

class UNREALTOURNAMENT_API FUTOnlineGameSettingsBase : public FOnlineSessionSettings
{
public:
	FUTOnlineGameSettingsBase(bool bIsLanGame = false, bool bIsPresense = false, bool bPrivate = false, int32 MaxNumberPlayers = 32);
	virtual ~FUTOnlineGameSettingsBase(){}

	virtual void ApplyGameSettings(AUTBaseGameMode* CurrentGame);
};