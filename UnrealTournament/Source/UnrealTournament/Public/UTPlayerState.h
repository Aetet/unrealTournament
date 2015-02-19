// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTTeamInterface.h"
#include "Stat.h"
#include "Online.h"
#include "OnlineSubsystemTypes.h"
#include "UTDamageType.h"
#include "UTHat.h"
#include "UTEyewear.h"
#include "UTPlayerState.generated.h"

USTRUCT(BlueprintType)
struct FWeaponSpree
{
	GENERATED_USTRUCT_BODY()

	FWeaponSpree() : SpreeSoundName(NAME_None), Kills(0) {};

	FWeaponSpree(FName InSpreeSoundName) : SpreeSoundName(InSpreeSoundName), Kills(0) {};

	UPROPERTY()
	FName SpreeSoundName;

	UPROPERTY()
	int32 Kills;
};

UCLASS()
class UNREALTOURNAMENT_API AUTPlayerState : public APlayerState, public IUTTeamInterface
{
	GENERATED_UCLASS_BODY()

private:

	/** Instance of a stat manager that tracks gameplay/reward stats for this Player Controller */
	UPROPERTY()
	class UStatManager *StatManager;

protected:
	/** selected character (if NULL left at default) */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = NotifyTeamChanged, Category = PlayerState)
	TSubclassOf<class AUTCharacterContent> SelectedCharacter;
public:
	UFUNCTION(BlueprintCallable, Category = Character)
	virtual void SetCharacter(const FString& CharacterPath);
	
	UFUNCTION(BlueprintCallable, Reliable, Server, WithValidation, Category = Character)
	void ServerSetCharacter(const FString& CharacterPath);
	inline TSubclassOf<class AUTCharacterContent> GetSelectedCharacter() const
	{
		return SelectedCharacter;
	}

	virtual void UpdatePing(float InPing) override;

	/** player's team if we're playing a team game */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = NotifyTeamChanged, Category = PlayerState)
	class AUTTeamInfo* Team;

	/** Whether this player is waiting to enter match */
	UPROPERTY(BlueprintReadOnly, replicated, Category = PlayerState)
	uint32 bWaitingPlayer:1;

	/** Whether this player has confirmed ready to play */
	UPROPERTY(BlueprintReadWrite, replicated, Category = PlayerState)
	uint32 bReadyToPlay:1;

	/** Whether this player has a pending switch team request (waiting for swap partner) */
	UPROPERTY(BlueprintReadWrite, replicated, Category = PlayerState)
	uint32 bPendingTeamSwitch : 1;

	UPROPERTY(BlueprintReadWrite, Category = PlayerState)
	float LastKillTime;
	/** current multikill level (1 = double, 2 = multi, etc)
	 * note that the value isn't reset until a new kill comes in
	 */
	UPROPERTY(BlueprintReadWrite, Category = PlayerState)
	int32 MultiKillLevel;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = PlayerState)
	int32 Spree;

	/** Kills by this player.  Not replicated but calculated client-side */
	UPROPERTY(BlueprintReadWrite, replicated, Category = PlayerState)
	int32 Kills;

	/** Can't respawn once out of lives */
	UPROPERTY(BlueprintReadWrite, replicated, Category = PlayerState)
	uint32 bOutOfLives:1;

	/** How many times associated player has died */
	UPROPERTY(BlueprintReadOnly, replicated, ReplicatedUsing = OnDeathsReceived, Category = PlayerState)
	int32 Deaths;

	/** How many times has the player captured the flag */
	UPROPERTY(BlueprintReadWrite, replicated, Category = PlayerState)
	uint32 FlagCaptures;

	/** How many times has the player returned the flag */
	UPROPERTY(BlueprintReadWrite, replicated, Category = PlayerState)
	uint32 FlagReturns;

	/** How many times has the player captured the flag */
	UPROPERTY(BlueprintReadWrite, replicated, Category = PlayerState)
	uint32 Assists;

	UPROPERTY(BlueprintReadOnly, replicated, Category = PlayerState)
	AUTPlayerState* LastKillerPlayerState;

	UPROPERTY(BlueprintReadOnly, Category = PlayerState)
	bool bIsRconAdmin;

	UPROPERTY(BlueprintReadOnly, replicated, Category = PlayerState)
	bool bHasHighScore;

	// Player Stats 

	/** This is the unique ID for stats generation*/
	FString StatsID;
	
	// How long until this player can respawn.  It's not directly replicated to the clients instead it's set
	// locally via OnDeathsReceived.  It will be set to the value of "GameState.RespawnWaitTime"

	UPROPERTY(BlueprintReadWrite, Category = PlayerState)
	float RespawnTime;

	UPROPERTY(BlueprintReadWrite, Category = PlayerState)
	float ForceRespawnTime;

	UPROPERTY(replicated)
	bool bChosePrimaryRespawnChoice;

	UPROPERTY(replicated)
	class APlayerStart* RespawnChoiceA;

	UPROPERTY(replicated)
	class APlayerStart* RespawnChoiceB;

	/** The currently held object */
	UPROPERTY(BlueprintReadOnly, replicated, ReplicatedUsing = OnCarriedObjectChanged, Category = PlayerState)
	class AUTCarriedObject* CarriedObject;

	UPROPERTY(BlueprintReadOnly, Category = PlayerState)
	TArray<FWeaponSpree> WeaponSprees;

	UPROPERTY(BlueprintReadOnly, replicated, ReplicatedUsing = OnWeaponSpreeDamage, Category = PlayerState)
	TSubclassOf<UUTDamageType> WeaponSpreeDamage;

	UFUNCTION()
	void OnWeaponSpreeDamage();

	virtual void AnnounceWeaponSpree(int32 SpreeIndex, TSubclassOf<class UUTDamageType> UTDamage);

	UFUNCTION()
	void OnDeathsReceived();

	/** Team has changed, announce, tell pawn, etc. */
	UFUNCTION()
	virtual void HandleTeamChanged(AController* Controller);

	UFUNCTION(BlueprintNativeEvent)
	void NotifyTeamChanged();

	UFUNCTION()
	void OnCarriedObjectChanged();

	UFUNCTION()
	virtual void SetCarriedObject(AUTCarriedObject* NewCarriedObject);

	UFUNCTION()
	virtual void ClearCarriedObject(AUTCarriedObject* OldCarriedObject);


	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = PlayerState)
	virtual void SetWaitingPlayer(bool B);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = PlayerState)
	virtual void IncrementKills(TSubclassOf<UDamageType> DamageType, bool bEnemyKill);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = PlayerState)
	virtual void IncrementDeaths(TSubclassOf<UDamageType> DamageType, AUTPlayerState* KillerPlayerState);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = PlayerState)
	virtual void AdjustScore(int32 ScoreAdjustment);

	virtual void Tick(float DeltaTime) override;

	inline bool IsFemale()
	{
		return false; // TODO
	}

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerRequestChangeTeam(uint8 NewTeamIndex);

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerReceiveStatsID(const FString& NewStatsID);

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerRecieveCountryFlag(uint32 NewCountryFlag);

	UPROPERTY()
	TSubclassOf<AUTHat> HatClass;

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerReceiveHatClass(const FString& NewHatClass);

	UPROPERTY()
	TSubclassOf<AUTEyewear> EyewearClass;

	UFUNCTION(Server, Reliable, WithValidation)
	virtual void ServerReceiveEyewearClass(const FString& NewEyewearClass);

	virtual void CopyProperties(APlayerState* PlayerState) override;
	virtual void OverrideWith(APlayerState* PlayerState) override;

	// Returns the team number of the team that owns this object
	UFUNCTION()
	virtual uint8 GetTeamNum() const;
	// not applicable
	virtual void SetTeamForSideSwap_Implementation(uint8 NewTeamNum) override
	{}

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	virtual	bool ModifyStat(FName StatName, int32 Amount, EStatMod::Type ModType);

	// Where should any chat go.  NOTE: some commands like Say and TeamSay ignore this value
	UPROPERTY(replicated)
	FName ChatDestination;

	UFUNCTION(server, reliable, withvalidation)
	virtual void ServerNextChatDestination();

	UPROPERTY(Replicated, BlueprintReadOnly, Category = PlayerState)
	uint32 CountryFlag;

	virtual void ValidateEntitlements();

	void WriteStatsToCloud();
	virtual void AddMatchToStats(const FString& GameType, const TArray<class AUTTeamInfo*>* Teams, const TArray<APlayerState*>* ActivePlayerStates, const TArray<APlayerState*>* InactivePlayerStates);

	virtual int32 GetSkillRating(FName SkillStatName);
	virtual void UpdateTeamSkillRating(FName SkillStatName, bool bWonMatch);
	virtual void UpdateIndividualSkillRating(FName SkillStatName);

private:
	bool bWroteStatsToCloud;
	int32 DuelSkillRatingThisMatch;
	int32 TDMSkillRatingThisMatch;
	IOnlineIdentityPtr OnlineIdentityInterface;
	IOnlineUserCloudPtr OnlineUserCloudInterface;
	FOnReadUserFileCompleteDelegate OnReadUserFileCompleteDelegate;
	FOnWriteUserFileCompleteDelegate OnWriteUserFileCompleteDelegate;
	FString GetStatsFilename();
	void ReadStatsFromCloud();
	virtual void OnReadUserFileComplete(bool bWasSuccessful, const FUniqueNetId& InUserId, const FString& FileName);
	virtual void OnWriteUserFileComplete(bool bWasSuccessful, const FUniqueNetId& InUserId, const FString& FileName);

public:
	// Average ELO rank for this player.
	UPROPERTY(Replicated)
	int32 AverageRank;

	// Find the local player and see if we are his friend.
	void OnRep_UniqueId();

	// Calculated client-side by the local player when 
	bool bIsFriend;

};



