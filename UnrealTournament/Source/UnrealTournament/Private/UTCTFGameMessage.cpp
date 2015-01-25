// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTCarriedObjectMessage.h"
#include "UTCTFGameMessage.h"
#include "UTAnnouncer.h"

UUTCTFGameMessage::UUTCTFGameMessage(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	MessageArea = FName(TEXT("GameMessages"));
	ReturnMessage = NSLOCTEXT("CTFGameMessage","ReturnMessage","{Player1Name} returned the {OptionalTeam} Flag!");
	ReturnedMessage = NSLOCTEXT("CTFGameMessage","ReturnedMessage","The {OptionalTeam} Flag was returned!");
	CaptureMessage = NSLOCTEXT("CTFGameMessage","CaptureMessage","{Player1Name} captured the flag and scored for {OptionalTeam}!");
	DroppedMessage = NSLOCTEXT("CTFGameMessage","DroppedMessage","{Player1Name} dropped the {OptionalTeam} Flag!");
	HasMessage = NSLOCTEXT("CTFGameMessage","HasMessage","{Player1Name} took the {OptionalTeam} Flag!");
	KilledMessage = NSLOCTEXT("CTFGameMessage","KilledMessage","{Player1Name} killed the {OptionalTeam} flag carrier!");
	HasAdvantageMessage = NSLOCTEXT("CTFGameMessage", "HasAdvantage", "{OptionalTeam} Team has Advantage");
	LosingAdvantageMessage = NSLOCTEXT("CTFGameMessage", "LostAdvantage", "{OptionalTeam} Team is losing advantage");

	bIsStatusAnnouncement = true;
	bIsPartiallyUnique = true;

}

FText UUTCTFGameMessage::GetText(int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const
{
	switch(Switch)
	{
		case 0 : return ReturnMessage; break;
		case 1 : return ReturnedMessage; break;
		case 2 : return CaptureMessage; break;
		case 3 : return DroppedMessage; break;
		case 4 : return HasMessage; break;
		case 5 : return KilledMessage; break;
		case 6 : return HasAdvantageMessage; break;
		case 7 : return LosingAdvantageMessage; break;
		case 8 : return CaptureMessage; break;
		case 9 : return CaptureMessage; break;
	}

	return FText::GetEmpty();
}


bool UUTCTFGameMessage::InterruptAnnouncement_Implementation(int32 Switch, const UObject* OptionalObject, TSubclassOf<UUTLocalMessage> OtherMessageClass, int32 OtherSwitch, const UObject* OtherOptionalObject) const
{
	if (Cast<UUTLocalMessage>(OtherMessageClass->GetDefaultObject())->bOptionalSpoken)
	{
		return true;
	}
	if (GetClass() == OtherMessageClass)
	{
		if ((OtherSwitch == 2) || (OtherSwitch == 8) || (OtherSwitch == 9))
		{
			// never interrupt scoring announcements
			return false;
		}
		if (OptionalObject == OtherOptionalObject)
		{
			// interrupt announcement about same object
			return true;
		}
	}
	return false;
}

void UUTCTFGameMessage::PrecacheAnnouncements_Implementation(UUTAnnouncer* Announcer) const
{
	for (int32 i = 0; i <= 10; i++)
	{
		for (uint8 j = 0; j < 2; j++)
		{
			Announcer->PrecacheAnnouncement(GetTeamAnnouncement(i, j));
		}
	}
}

FName UUTCTFGameMessage::GetTeamAnnouncement(int32 Switch, uint8 TeamNum) const
{
	switch (Switch)
	{
		case 0: return TeamNum == 0 ? TEXT("RedFlagReturned") : TEXT("BlueFlagReturned"); break;
		case 1: return TeamNum == 0 ? TEXT("RedFlagReturned") : TEXT("BlueFlagReturned"); break;
		case 2: return TeamNum == 0 ? TEXT("RedTeamScores") : TEXT("BlueTeamScores"); break;
		case 3: return TeamNum == 0 ? TEXT("RedFlagDropped") : TEXT("BlueFlagDropped"); break;
		case 4: return TeamNum == 0 ? TEXT("RedFlagTaken") : TEXT("BlueFlagTaken"); break;
		case 6: return TeamNum == 0 ? TEXT("LosingAdvantage") : TEXT("LosingAdvantage"); break;
		case 7: return TeamNum == 0 ? TEXT("RedTeamAdvantage") : TEXT("BlueTeamAdvantage"); break;
		case 8: return TeamNum == 0 ? TEXT("RedIncreasesLead") : TEXT("BlueIncreasesLead"); break;
		case 9: return TeamNum == 0 ? TEXT("RedDominating") : TEXT("BlueDominating"); break;
		case 10: return TeamNum == 0 ? TEXT("RedDominating") : TEXT("BlueDominating"); break;
	}
	return NAME_None;
}

FName UUTCTFGameMessage::GetAnnouncementName_Implementation(int32 Switch, const UObject* OptionalObject) const
{
	const AUTTeamInfo* TeamInfo = Cast<AUTTeamInfo>(OptionalObject);
	uint8 TeamNum = (TeamInfo != NULL) ? TeamInfo->GetTeamNum() : 0;

	return GetTeamAnnouncement(Switch, TeamNum);
}
