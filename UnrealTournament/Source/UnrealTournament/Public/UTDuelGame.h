// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTTeamDMGameMode.h"

#include "UTDuelGame.generated.h"

UCLASS()
class UNREALTOURNAMENT_API AUTDuelGame : public AUTTeamDMGameMode
{
	GENERATED_UCLASS_BODY()

	virtual bool CheckRelevance_Implementation(AActor* Other) override;
	virtual void InitGameState() override;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PlayEndOfMatchMessage() override;
	virtual void UpdateSkillRating() override;
	virtual bool ChangeTeam(AController* Player, uint8 NewTeam, bool bBroadcast) override;

	/** How long powerups last in Duel */
	UPROPERTY()
	float PowerupDuration;

#if !UE_SERVER
	virtual void CreateConfigWidgets(TSharedPtr<class SVerticalBox> MenuSpace, TArray< TSharedPtr<TAttributePropertyBase> >& ConfigProps) override;
	virtual TSharedRef<SWidget> CreateLobbyPanel(bool inIsHost, TWeakObjectPtr<class UUTLocalPlayer> inPlayerOwner, TWeakObjectPtr<AUTLobbyMatchInfo> inMatchInfo) const;
#endif

	virtual FString GetDefaultLobbyOptions() const;

protected:
	virtual void UpdateLobbyMatchStats();
};
