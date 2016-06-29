// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTPlayerController.h"

#include "UTDemoRecSpectator.generated.h"

UCLASS()
class UNREALTOURNAMENT_API AUTDemoRecSpectator : public AUTPlayerController
{
	GENERATED_UCLASS_BODY()

	virtual void PlayerTick(float DeltaTime) override;

	virtual void ReceivedPlayer() override;

	virtual void ViewPlayerState(APlayerState* PS) override;
	virtual void ViewSelf(FViewTargetTransitionParams TransitionParams = FViewTargetTransitionParams()) override;
	virtual void ViewPawn(APawn* PawnToView) override;
	virtual void ServerViewProjectileShim() override;

	virtual void ViewAPlayer(int32 dir) override;
	virtual APlayerState* GetNextViewablePlayer(int32 dir) override;

	virtual bool CallRemoteFunction(UFunction* Function, void* Parameters, FOutParmRec* OutParms, FFrame* Stack) override;

	virtual void ClientTravelInternal_Implementation(const FString& URL, ETravelType TravelType, bool bSeamless, FGuid MapPackageGuid) override;

	virtual void ClientToggleScoreboard_Implementation(bool bShow) override;
	virtual void ShowEndGameScoreboard() override;

	virtual void ClientGameEnded_Implementation(AActor* EndGameFocus, bool bIsWinner) override;

	virtual void InitPlayerState() override;
	virtual void CleanupPlayerState() override
	{
		// don't do AddInactivePlayer() stuff for demo spectator
		AController::CleanupPlayerState();
	}

	virtual void BeginPlay() override;
	virtual void OnNetCleanup(class UNetConnection* Connection) override;

	UFUNCTION(Exec)
	void ToggleReplayWindow();

	virtual void ShowMenu(const FString& Parameters) override;
	virtual void HideMenu() override;


	virtual void SmoothTargetViewRotation(APawn* TargetPawn, float DeltaSeconds) override;

	virtual void ViewFlag(uint8 Index) override;

	UFUNCTION(Client, UnReliable)
	virtual void DemoNotifyCausedHit(APawn* InstigatorPawn, AUTCharacter* HitPawn, uint8 AppliedDamage, FVector Momentum, const FDamageEvent& DamageEvent);

	UPROPERTY()
	APlayerState* QueuedPlayerStateToView;
};