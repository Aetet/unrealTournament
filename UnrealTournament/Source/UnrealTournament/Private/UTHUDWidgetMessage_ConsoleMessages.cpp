// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTHUDWidgetMessage.h"
#include "UTHUDWidgetMessage_ConsoleMessages.h"

UUTHUDWidgetMessage_ConsoleMessages::UUTHUDWidgetMessage_ConsoleMessages(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ManagedMessageArea = FName(TEXT("ConsoleMessage"));
	Position = FVector2D(0.0f, 0.0f);			
	ScreenPosition = FVector2D(0.0f, 0.65f);
	Size = FVector2D(0.0f, 0.0f);			
	Origin = FVector2D(0.0f, 0.0f);				
	MessageColor = FLinearColor::Green;
	NumVisibleLines=4;
}

void UUTHUDWidgetMessage_ConsoleMessages::DrawMessages(float DeltaTime)
{
	Canvas->Reset();

	float Y = 0;
	int MessageIndex = NumVisibleLines < MessageQueue.Num() ? NumVisibleLines : MessageQueue.Num();
	while (MessageIndex >= 0)
	{
		if (MessageQueue[MessageIndex].MessageClass != NULL)	
		{
			DrawMessage(MessageIndex,0,Y);
			Y -= MessageQueue[MessageIndex].TextHeight;
		}
		MessageIndex--;
	}
}

void UUTHUDWidgetMessage_ConsoleMessages::DrawMessage(int32 QueueIndex, float X, float Y)
{
	MessageQueue[QueueIndex].bHasBeenRendered = true;
	DrawText(MessageQueue[QueueIndex].Text, X, Y, MessageQueue[QueueIndex].DisplayFont, FVector2D(2,2), FLinearColor::Black, 1.0f, 1.0f, MessageQueue[QueueIndex].DrawColor, ETextHorzPos::Left, ETextVertPos::Top);
}

void UUTHUDWidgetMessage_ConsoleMessages::LayoutMessage(int32 QueueIndex, TSubclassOf<class UUTLocalMessage> MessageClass, uint32 MessageIndex, FText LocalMessageText, int32 MessageCount, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject)
{
	Super::LayoutMessage(QueueIndex, MessageClass, MessageIndex, LocalMessageText, MessageCount, RelatedPlayerState_1, RelatedPlayerState_2, OptionalObject);

	AUTPlayerState* InstigatorPS = Cast<AUTPlayerState>(RelatedPlayerState_1);
	if (InstigatorPS != NULL && InstigatorPS->Team != NULL)
	{
		MessageQueue[QueueIndex].DrawColor = FMath::LerpStable(InstigatorPS->Team->TeamColor, FLinearColor::White, 0.1f);
	}
}