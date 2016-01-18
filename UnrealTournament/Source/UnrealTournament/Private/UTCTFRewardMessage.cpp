// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLocalMessage.h"
#include "UTCTFRewardMessage.h"
#include "GameFramework/LocalMessage.h"

UUTCTFRewardMessage::UUTCTFRewardMessage(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bIsUnique = true;
	bIsSpecial = true;
	Lifetime = 3.0f;
	MessageArea = FName(TEXT("DeathMessage"));
	bIsConsoleMessage = false;
	AssistMessage = NSLOCTEXT("CTFRewardMessage", "Assist", "Assist!");
	DeniedMessage = NSLOCTEXT("CTFRewardMessage", "Denied", "Denied!");
	BlueScoreMessage = NSLOCTEXT("CTFRewardMessage", "Blue Score", "BLUE Team Scores!");
	RedScoreMessage = NSLOCTEXT("CTFRewardMessage", "Red Score", "RED Team Scores!");
	HatTrickMessage = NSLOCTEXT("CTFRewardMessage", "HatTrick", "Hat Trick!");
	OtherHatTrickMessage = NSLOCTEXT("CTFRewardMessage", "OtherHatTrick", "{Player1Name} got a Hat Trick!");
	bIsStatusAnnouncement = false;
	bWantsBotReaction = true;
}

FLinearColor UUTCTFRewardMessage::GetMessageColor_Implementation(int32 MessageIndex) const
{
	if (MessageIndex == 3)
	{
		return FLinearColor::Red;
	}
	if (MessageIndex == 4)
	{
		return FLinearColor::Blue;
	}
	return FLinearColor::Yellow;
}

void UUTCTFRewardMessage::PrecacheAnnouncements_Implementation(UUTAnnouncer* Announcer) const
{
	for (int32 i = 0; i < 6; i++)
	{
		Announcer->PrecacheAnnouncement(GetAnnouncementName(i, NULL));
	}
}

float UUTCTFRewardMessage::GetScaleInSize_Implementation(int32 MessageIndex) const
{
	return 3.f;
}

float UUTCTFRewardMessage::GetAnnouncementDelay(int32 Switch)
{
	return ((Switch == 2) || (Switch == 5)) ? 1.5f : 0.f;
}

FName UUTCTFRewardMessage::GetAnnouncementName_Implementation(int32 Switch, const UObject* OptionalObject) const
{
	switch (Switch)
	{
	case 0: return TEXT("Denied"); break;
	case 1: return TEXT("LastSecondSave"); break;
	case 2: return TEXT("Assist"); break;
	case 5: return TEXT("HatTrick"); break;
	}
	return NAME_None;
}

bool UUTCTFRewardMessage::ShouldPlayAnnouncement(const FClientReceiveData& ClientData) const
{
	return IsLocalForAnnouncement(ClientData, true, true);
}

FText UUTCTFRewardMessage::GetText(int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const
{
	switch (Switch)
	{
	case 0: return DeniedMessage; break;
	case 2: return AssistMessage; break;
	case 3: return RedScoreMessage; break;
	case 4: return BlueScoreMessage; break;
	case 5: return (bTargetsPlayerState1 ? HatTrickMessage : OtherHatTrickMessage); break;
	}

	return FText::GetEmpty();
}


