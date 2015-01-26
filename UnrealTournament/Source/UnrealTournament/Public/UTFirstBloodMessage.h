// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTAnnouncer.h"

#include "UTFirstBloodMessage.generated.h"

UCLASS(CustomConstructor)
class UUTFirstBloodMessage : public UUTLocalMessage
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	TArray<FName> AnnouncementNames;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText FirstBloodLocalText;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Message)
	FText FirstBloodRemoteText;

	UUTFirstBloodMessage(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{
		MessageArea = FName(TEXT("DeathMessage"));

		FirstBloodLocalText = NSLOCTEXT("UTFirstBloodMessage", "FirstBloodLocalText", "First Blood!");
		FirstBloodRemoteText = NSLOCTEXT("UTFirstBloodMessage", "FirstBloodRemoteText", "{Player1Name} drew First Blood!");

		AnnouncementNames.Add(TEXT("FirstBlood"));

		Importance = 0.9f;
		bIsSpecial = true;
		bIsUnique = true;
		bIsConsoleMessage = false;
		Lifetime = 4.0f;
		AnnouncementDelay = 0.5f;
	}

	virtual FLinearColor GetMessageColor(int32 MessageIndex) const
	{
		return FLinearColor::Red;
	}

	virtual void ClientReceive(const FClientReceiveData& ClientData) const override
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(ClientData.LocalPC);
		if (PC != NULL && PC->RewardAnnouncer != NULL && ClientData.RelatedPlayerState_1 == PC->PlayerState)
		{
			PC->RewardAnnouncer->PlayAnnouncement(GetClass(), ClientData.MessageIndex, ClientData.OptionalObject);
		}

		Super::ClientReceive(ClientData);
	}
	virtual FName GetAnnouncementName_Implementation(int32 Switch, const UObject* OptionalObject) const override
	{
		return AnnouncementNames.Num() > 0 ? AnnouncementNames[FMath::Clamp<int32>(Switch, 0, AnnouncementNames.Num() - 1)] : NAME_None;
	}
	virtual FText GetText(int32 Switch = 0, bool bTargetsPlayerState1 = false, class APlayerState* RelatedPlayerState_1 = NULL, class APlayerState* RelatedPlayerState_2 = NULL, class UObject* OptionalObject = NULL) const override
	{
		if (bTargetsPlayerState1)
		{
			return FirstBloodLocalText;
		}
			
		return FirstBloodRemoteText;
	}
};