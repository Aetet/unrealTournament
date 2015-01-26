// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTServerBeaconClient.h"

AUTServerBeaconClient::AUTServerBeaconClient(const class FObjectInitializer& ObjectInitializer) :
Super(ObjectInitializer)
{
	PingStartTime = -1;
}

void AUTServerBeaconClient::SetBeaconNetDriverName(FString InBeaconName)
{
	BeaconNetDriverName = FName(*InBeaconName);
	NetDriverName = BeaconNetDriverName;
}


void AUTServerBeaconClient::OnConnected()
{
	Super::OnConnected();

	UE_LOG(UT,Log, TEXT("---> PING"));

	// Tell the server that we want to ping
	PingStartTime = GetWorld()->RealTimeSeconds;
	ServerPing();
}

void AUTServerBeaconClient::OnFailure()
{
	UE_LOG(UT, Log, TEXT("UTServer beacon connection failure, handling connection timeout."));
	OnServerRequestFailure.ExecuteIfBound(this);
	Super::OnFailure();
	PingStartTime = -2;
}

bool AUTServerBeaconClient::ServerPing_Validate()
{
	return true;
}

void AUTServerBeaconClient::ServerPing_Implementation()
{
	UE_LOG(UT,Log, TEXT("<--- PONG"));
	// Respond to the client
	ClientPong();
}

void AUTServerBeaconClient::ClientPong_Implementation()
{
	Ping = (GetWorld()->RealTimeSeconds - PingStartTime) * 1000.0f;

	UE_LOG(UT,Log, TEXT("---> Requesting Info %f"), Ping);

	// Ask for additional server info
	ServerRequestInfo();
}

bool AUTServerBeaconClient::ServerRequestInfo_Validate() { return true; }
void AUTServerBeaconClient::ServerRequestInfo_Implementation()
{
	FServerBeaconInfo ServerInfo;
	
 	AUTGameState* GameState = GetWorld()->GetGameState<AUTGameState>();
	if (GameState)
	{
		ServerInfo.ServerPlayers = TEXT("");

		// Add the Players section3
		for (int32 i=0; i < GameState->PlayerArray.Num(); i++)
		{
			FString PlayerName =  GameState->PlayerArray[i]->PlayerName;
			FString PlayerScore = FString::Printf(TEXT("%i"), int32(GameState->PlayerArray[i]->Score));
			FString UniqueID = GameState->PlayerArray[i]->UniqueId.IsValid() ? GameState->PlayerArray[i]->UniqueId->ToString() : TEXT("none");
			ServerInfo.ServerPlayers += FString::Printf(TEXT("%s\t%s\t%s\t"), *PlayerName, *PlayerScore, *UniqueID);
		}

		ServerInfo.MOTD = GameState->ServerMOTD;
	}

	AUTGameMode* GameMode = Cast<AUTGameMode>(GetWorld()->GetAuthGameMode());
	if (GameMode)
	{
		ServerInfo.ServerRules = TEXT("");
		GameMode->BuildServerResponseRules(ServerInfo.ServerRules);	
	}

	int32 NumInstances = GameMode->GetInstanceData(InstanceHostNames, InstanceDescriptions);
	UE_LOG(UT,Log, TEXT("<--- Sending Info %i"), NumInstances);

	ClientRecieveInfo(ServerInfo, NumInstances);
}

void AUTServerBeaconClient::ClientRecieveInfo_Implementation(const FServerBeaconInfo ServerInfo, int32 NumInstances)
{
	HostServerInfo = ServerInfo;
	InstanceCount = NumInstances;

	if (NumInstances > 0)
	{
		UE_LOG(UT,Log, TEXT("---> Received Info [%i] Requesting Instance Data"), NumInstances);
		ServerRequestInstances(-1);
	}
	else
	{
		UE_LOG(UT,Log, TEXT("---> Received Info [%i] DONE!!!!"), NumInstances);
		OnServerRequestResults.ExecuteIfBound(this, HostServerInfo);
	}
}

bool AUTServerBeaconClient::ServerRequestInstances_Validate(int32 LastInstanceIndex) { return true; }
void AUTServerBeaconClient::ServerRequestInstances_Implementation(int32 LastInstanceIndex)
{
	LastInstanceIndex++;
	
	if (LastInstanceIndex < InstanceHostNames.Num() && LastInstanceIndex < InstanceDescriptions.Num() )
	{
		UE_LOG(UT,Log, TEXT("<--- Sending Instance [%i]"), LastInstanceIndex);
		ClientRecieveInstance_Implementation(LastInstanceIndex, InstanceHostNames.Num(), InstanceHostNames[LastInstanceIndex], InstanceDescriptions[LastInstanceIndex]);
	}
	else
	{
		UE_LOG(UT,Log, TEXT("<--- Out of Instances [%i] %i"), LastInstanceIndex, InstanceHostNames.Num());
		ClientRecievedAllInstance(InstanceHostNames.Num());
	}
}

void AUTServerBeaconClient::ClientRecieveInstance_Implementation(uint32 InstanceCount, uint32 TotalInstances, const FString& InstanceHostName, const FString& InstanceDescription)
{
	UE_LOG(UT,Log, TEXT("---> Recieved Instance [%i] %s"), InstanceCount, *InstanceHostName);
	if (InstanceCount >= 0 && InstanceCount < TotalInstances)
	{
		InstanceHostNames.Add(InstanceHostName);
		InstanceDescriptions.Add(InstanceDescription);
	}

	ServerRequestInstances(InstanceCount);
}

void AUTServerBeaconClient::ClientRecievedAllInstance_Implementation(uint32 FinalCount)
{
	if (InstanceHostNames.Num() != InstanceDescriptions.Num() || InstanceHostNames.Num() != FinalCount)
	{
		UE_LOG(UT,Log,TEXT("ERROR: Instance Names/Descriptions doesn't meet the final size requirement: %i/%i vs %i"), InstanceHostNames.Num(), InstanceDescriptions.Num(), FinalCount);
	}

	UE_LOG(UT,Log, TEXT("---> Got them All DONE!!!!  [%i vs %i]"), InstanceHostNames.Num(), FinalCount );

	OnServerRequestResults.ExecuteIfBound(this, HostServerInfo);		
}
