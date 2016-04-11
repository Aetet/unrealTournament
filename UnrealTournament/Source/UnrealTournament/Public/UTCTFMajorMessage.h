// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTCTFMajorMessage.generated.h"

UCLASS()
class UNREALTOURNAMENT_API UUTCTFMajorMessage : public UUTCarriedObjectMessage
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Message)
		FText CaptureMessage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Message)
		FText HalftimeMessage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Message)
		FText OvertimeMessage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Message)
		FText LastLifeMessage;

	virtual bool InterruptAnnouncement_Implementation(int32 Switch, const UObject* OptionalObject, TSubclassOf<UUTLocalMessage> OtherMessageClass, int32 OtherSwitch, const UObject* OtherOptionalObject) const override;
	virtual bool CancelByAnnouncement_Implementation(int32 Switch, const UObject* OptionalObject, TSubclassOf<UUTLocalMessage> OtherMessageClass, int32 OtherSwitch, const UObject* OtherOptionalObject) const override;

	virtual FText GetText(int32 Switch = 0, bool bTargetsPlayerState1 = false, class APlayerState* RelatedPlayerState_1 = NULL, class APlayerState* RelatedPlayerState_2 = NULL, class UObject* OptionalObject = NULL) const override;

	virtual FLinearColor GetMessageColor_Implementation(int32 MessageIndex) const override;
	virtual FName GetTeamAnnouncement(int32 Switch, uint8 TeamIndex) const;
	virtual FName GetAnnouncementName_Implementation(int32 Switch, const UObject* OptionalObject) const override;
	virtual float GetAnnouncementSpacing_Implementation(int32 Switch, const UObject* OptionalObject) const override;
	virtual void PrecacheAnnouncements_Implementation(UUTAnnouncer* Announcer) const override;
	virtual bool UseMegaFont(int32 MessageIndex) const override;
	virtual float GetScaleInSize_Implementation(int32 MessageIndex) const override;
	virtual float GetAnnouncementPriority(int32 Switch) const override;
	virtual bool UseLargeFont(int32 MessageIndex) const override;
};



