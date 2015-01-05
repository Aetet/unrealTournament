// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTLobbyPC.h"
#include "UTGameState.h"
#include "UTLobbyGameState.h"
#include "UTLobbyPlayerState.h"
#include "../Private/Slate/SUWindowsLobby.h"
#include "UTBaseGameMode.h"
#include "UTServerBeaconLobbyHostListener.h"
#include "UTServerBeaconLobbyHostObject.h"
#include "UTLobbyGameMode.generated.h"

UCLASS(Config = Game)
class UNREALTOURNAMENT_API AUTLobbyGameMode : public AUTBaseGameMode
{
	GENERATED_UCLASS_BODY()

public:
	/** Cached reference to our game state for quick access. */
	UPROPERTY()
	AUTLobbyGameState* UTLobbyGameState;		

	UPROPERTY(GlobalConfig)
	FString LobbyPassword;

	UPROPERTY(GlobalConfig)
	int32 StartingInstancePort;

	UPROPERTY(GlobalConfig)
	int32 InstancePortStep;

	UPROPERTY()
	TSubclassOf<class UUTLocalMessage>  GameMessageClass;

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState();
	virtual void StartMatch();
	virtual void RestartPlayer(AController* aPlayer);
	virtual void ChangeName(AController* Other, const FString& S, bool bNameChange);

	virtual void PostLogin( APlayerController* NewPlayer );
	virtual void Logout(AController* Exiting);
	virtual bool PlayerCanRestart(APlayerController* Player);
	virtual TSubclassOf<AGameSession> GetGameSessionClass() const;
	virtual void OverridePlayerState(APlayerController* PC, APlayerState* OldPlayerState);

	virtual bool IsLobbyServer() { return true; }


#if !UE_SERVER

	/**
	 *	Returns the Menu to popup when the user requests a menu
	 **/
	virtual TSharedRef<SUWindowsDesktop> GetGameMenu(UUTLocalPlayer* PlayerOwner) const
	{
		return SNew(SUWindowsLobby).PlayerOwner(PlayerOwner);
	}

#endif
	virtual FName GetNextChatDestination(AUTPlayerState* PlayerState, FName CurrentChatDestination);

protected:

	// The actual instance query port to use.
	int32 InstanceQueryPort;

	/**
	 * Converts a string to a bool.  If the string is empty, it will return the default.
	 **/
	inline bool EvalBoolOptions(FString InOpt, bool Default)
	{
		if (!InOpt.IsEmpty())
		{
			if (FCString::Stricmp(*InOpt,TEXT("True") )==0 
				||	FCString::Stricmp(*InOpt,*GTrue.ToString())==0
				||	FCString::Stricmp(*InOpt,*GYes.ToString())==0)
			{
				return true;
			}
			else if(FCString::Stricmp(*InOpt,TEXT("False"))==0
				||	FCString::Stricmp(*InOpt,*GFalse.ToString())==0
				||	FCString::Stricmp(*InOpt,TEXT("No"))==0
				||	FCString::Stricmp(*InOpt,*GNo.ToString())==0)
			{
				return false;
			}
			else
			{
				return FCString::Atoi(*InOpt) != 0;
			}
		}
		else
		{
			return Default;
		}
	}

public:
	virtual void PreLogin(const FString& Options, const FString& Address, const TSharedPtr<class FUniqueNetId>& UniqueId, FString& ErrorMessage);

};



