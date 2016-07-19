// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTCarriedObjectMessage.h"
#include "UTCTFGameMessage.h"
#include "UTAnnouncer.h"

UUTCTFGameMessage::UUTCTFGameMessage(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	MessageArea = FName(TEXT("Announcements"));
	MessageSlot = FName(TEXT("GameMessages"));
	ReturnMessage = NSLOCTEXT("CTFGameMessage","ReturnMessage","{Player1Name} returned the {OptionalTeam} Flag!");
	ReturnedMessage = NSLOCTEXT("CTFGameMessage","ReturnedMessage","The {OptionalTeam} Flag was returned!");
	CaptureMessage = NSLOCTEXT("CTFGameMessage","CaptureMessage","{Player1Name} scores for {OptionalTeam}!");
	DroppedMessage = NSLOCTEXT("CTFGameMessage","DroppedMessage","{Player1Name} dropped the {OptionalTeam} Flag!");
	HasMessage = NSLOCTEXT("CTFGameMessage","HasMessage","{Player1Name} took the {OptionalTeam} Flag!");
	KilledMessage = NSLOCTEXT("CTFGameMessage","KilledMessage","{Player1Name} killed the {OptionalTeam} flag carrier!");
	HasAdvantageMessage = NSLOCTEXT("CTFGameMessage", "HasAdvantage", "{OptionalTeam} Team has Advantage");
	LosingAdvantageMessage = NSLOCTEXT("CTFGameMessage", "LostAdvantage", "{OptionalTeam} Team is losing advantage");
	HalftimeMessage = NSLOCTEXT("CTFGameMessage", "Halftime", "");
	OvertimeMessage = NSLOCTEXT("CTFGameMessage", "Overtime", "OVERTIME!");
	NoReturnMessage = NSLOCTEXT("CTFGameMessage", "NoPickupFlag", "You can't pick up this flag.");
	LastLifeMessage = NSLOCTEXT("CTFGameMessage", "LastLife", "This is your last life");
	DefaultPowerupMessage = NSLOCTEXT("CTFGameMessage", "DefaultPowerupMessage", "{Player1Name} used their powerup!");

	bIsStatusAnnouncement = true;
	bIsPartiallyUnique = true;
}

FLinearColor UUTCTFGameMessage::GetMessageColor_Implementation(int32 MessageIndex) const
{
	return FLinearColor::White;
}

FText UUTCTFGameMessage::GetText(int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const
{
	switch(Switch)
	{
		case 0 : return ReturnMessage; break;
		case 1 : return ReturnedMessage; break;
		case 2 : return CaptureMessage; break;
		case 3 : return RelatedPlayerState_1 ? DroppedMessage : FText::GetEmpty(); break;
		case 4 : return HasMessage; break;
		case 5 : return KilledMessage; break;
		case 6 : return HasAdvantageMessage; break;
		case 7 : return LosingAdvantageMessage; break;
		case 8 : return CaptureMessage; break;
		case 9 : return CaptureMessage; break;
		case 11: return HalftimeMessage; break;
		case 12: return OvertimeMessage; break;
		case 13: return NoReturnMessage; break;
		case 20: return LastLifeMessage; break;
	}
	return FText::GetEmpty();
}

int32 UUTCTFGameMessage::GetFontSizeIndex(int32 MessageIndex) const
{
	if ((MessageIndex == 11) || (MessageIndex == 12))
	{
		return 3;
	}
	if ((MessageIndex == 2) || (MessageIndex == 2) || (MessageIndex > 5))
	{
		return 2;
	}
	return 1;
}

float UUTCTFGameMessage::GetAnnouncementPriority(int32 Switch) const
{
	return ((Switch == 2) || (Switch == 8) || (Switch == 9) || (Switch == 10)) ? 1.f : 0.5f;
}

float UUTCTFGameMessage::GetScaleInSize_Implementation(int32 MessageIndex) const
{
	return ((MessageIndex == 11) || (MessageIndex == 12)) ? 3.f : 1.f;
}

bool UUTCTFGameMessage::InterruptAnnouncement_Implementation(int32 Switch, const UObject* OptionalObject, TSubclassOf<UUTLocalMessage> OtherMessageClass, int32 OtherSwitch, const UObject* OtherOptionalObject) const
{
	if (OtherMessageClass->GetDefaultObject<UUTLocalMessage>()->bOptionalSpoken)
	{
		return true;
	}
	if (GetClass() == OtherMessageClass)
	{
		if ((OtherSwitch == Switch) || (OtherSwitch == 2) || (OtherSwitch == 8) || (OtherSwitch == 9) || (OtherSwitch == 10) || (OtherSwitch == 11) || (OtherSwitch == 12))
		{
			// never interrupt scoring announcements or same announcement
			return false;
		}
		if (OptionalObject && (OptionalObject == OtherOptionalObject))
		{
			// interrupt announcement about same object
			return true;
		}
	}
	if (GetAnnouncementPriority(Switch) > OtherMessageClass->GetDefaultObject<UUTLocalMessage>()->GetAnnouncementPriority(OtherSwitch))
	{
		return true;
	}
	return false;
}

float UUTCTFGameMessage::GetAnnouncementSpacing_Implementation(int32 Switch, const UObject* OptionalObject) const
{
	return 0.1f;
}

bool UUTCTFGameMessage::CancelByAnnouncement_Implementation(int32 Switch, const UObject* OptionalObject, TSubclassOf<UUTLocalMessage> OtherMessageClass, int32 OtherSwitch, const UObject* OtherOptionalObject) const
{
	if ((Switch == 2) || (Switch == 8) || (Switch == 9) || (Switch == 10))
	{
		// never cancel scoring announcement
		return false;
	}
	if ((OtherSwitch == 2) || (OtherSwitch == 8) || (OtherSwitch == 9) || (OtherSwitch == 10))
	{
		// always cancel if before scoring announcement
		return true;
	}
	return ((OtherSwitch == Switch) && ((OtherSwitch == 11) || (OtherSwitch == 12)));
}

void UUTCTFGameMessage::PrecacheAnnouncements_Implementation(UUTAnnouncer* Announcer) const
{
	for (int32 i = 0; i < 11; i++)
	{
		for (uint8 j = 0; j < 2; j++)
		{
			Announcer->PrecacheAnnouncement(GetTeamAnnouncement(i, j));
		}
	}
	Announcer->PrecacheAnnouncement(GetTeamAnnouncement(11, 0));
	Announcer->PrecacheAnnouncement(GetTeamAnnouncement(12, 0));
	Announcer->PrecacheAnnouncement(TEXT("YouHaveTheFlag_02"));
}

FName UUTCTFGameMessage::GetTeamAnnouncement(int32 Switch, uint8 TeamNum, const UObject* OptionalObject, const class APlayerState* RelatedPlayerState_1, const class APlayerState* RelatedPlayerState_2) const
{
	switch (Switch)
	{
		case 0: return TeamNum == 0 ? TEXT("RedFlagReturned") : TEXT("BlueFlagReturned"); break;
		case 1: return TeamNum == 0 ? TEXT("RedFlagReturned") : TEXT("BlueFlagReturned"); break;
		case 2: return TeamNum == 0 ? TEXT("RedTeamScores") : TEXT("BlueTeamScores"); break;
		case 3: return TeamNum == 0 ? TEXT("RedFlagDropped") : TEXT("BlueFlagDropped"); break;
		case 4:
			if (RelatedPlayerState_1 && Cast<AUTPlayerController>(RelatedPlayerState_1->GetOwner()))
			{
				return TEXT("YouHaveTheFlag_02"); break;
			}
			return TeamNum == 0 ? TEXT("RedFlagTaken") : TEXT("BlueFlagTaken"); break;
		case 6: return TeamNum == 0 ? TEXT("RedTeamAdvantage") : TEXT("BlueTeamAdvantage"); break;
		case 7: return TeamNum == 0 ? TEXT("LosingAdvantage") : TEXT("LosingAdvantage"); break;
		case 8: return TeamNum == 0 ? TEXT("RedIncreasesLead") : TEXT("BlueIncreasesLead"); break;
		case 9: return TeamNum == 0 ? TEXT("RedDominating") : TEXT("BlueDominating"); break;
		case 10: return TeamNum == 0 ? TEXT("RedDominating") : TEXT("BlueDominating"); break;
		case 11: return TEXT("HalfTime"); break;
		case 12: return TEXT("OverTime"); break;
		//case 14: return TEXT("Attack"); break;
		//case 15: return TEXT("Defend"); break;
	}
	return NAME_None;
}

FName UUTCTFGameMessage::GetAnnouncementName_Implementation(int32 Switch, const UObject* OptionalObject, const class APlayerState* RelatedPlayerState_1, const class APlayerState* RelatedPlayerState_2) const
{
	const AUTTeamInfo* TeamInfo = Cast<AUTTeamInfo>(OptionalObject);
	uint8 TeamNum = (TeamInfo != NULL) ? TeamInfo->GetTeamNum() : 0;
	return GetTeamAnnouncement(Switch, TeamNum, OptionalObject, RelatedPlayerState_1, RelatedPlayerState_2);
}

