// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTServerBeaconHost.h"
#include "UTServerBeaconClient.h"

AUTServerBeaconHost::AUTServerBeaconHost(const FObjectInitializer& ObjectInitializer) :
Super(ObjectInitializer)
{
	BeaconTypeName = TEXT("UTServerBeacon");
}

bool AUTServerBeaconHost::Init()
{
	UE_LOG(LogBeacon, Verbose, TEXT("Init"));
	return true;
}

void AUTServerBeaconHost::ClientConnected(AOnlineBeaconClient* NewClientActor, UNetConnection* ClientConnection)
{
	UE_LOG(LogBeacon, Verbose, TEXT("ClientConnected %s from (%s)"),
		NewClientActor ? *NewClientActor->GetName() : TEXT("NULL"),
		NewClientActor ? *NewClientActor->GetNetConnection()->LowLevelDescribe() : TEXT("NULL"));

	/*
	AUTServerBeaconClient* BeaconClient = Cast<AUTServerBeaconClient>(NewClientActor);
	if (BeaconClient != NULL)
	{
		BeaconClient->ClientPing();
	}
	*/
}

AOnlineBeaconClient* AUTServerBeaconHost::SpawnBeaconActor(UNetConnection* ClientConnection)
{
	FActorSpawnParameters SpawnInfo;
	AUTServerBeaconClient* BeaconActor = GetWorld()->SpawnActor<AUTServerBeaconClient>(AUTServerBeaconClient::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);
	if (BeaconActor)
	{
		BeaconActor->SetBeaconOwner(this);
	}

	return BeaconActor;
}
