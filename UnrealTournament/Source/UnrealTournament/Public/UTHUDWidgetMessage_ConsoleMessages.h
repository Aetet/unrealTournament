// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once 
#include "UTHUDWidgetMessage_ConsoleMessages.generated.h"

UCLASS()
class UUTHUDWidgetMessage_ConsoleMessages : public UUTHUDWidgetMessage 
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Widgets")
	int32 NumVisibleLines;

	virtual bool ShouldDraw_Implementation(bool bShowScores) override
	{
		return true;
	}

protected:
	virtual void DrawMessages(float DeltaTime);
	virtual void DrawMessage(int32 QueueIndex, float X, float Y);
	virtual void LayoutMessage(int32 QueueIndex, TSubclassOf<class UUTLocalMessage> MessageClass, uint32 MessageIndex, FText LocalMessageText, int32 MessageCount, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) override;
	virtual float GetDrawScaleOverride() override;
};
