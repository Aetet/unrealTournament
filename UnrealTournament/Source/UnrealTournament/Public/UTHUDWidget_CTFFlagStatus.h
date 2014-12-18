// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTHUDWidget_CTFFlagStatus.generated.h"

UCLASS()
class UUTHUDWidget_CTFFlagStatus : public UUTHUDWidget
{
	GENERATED_UCLASS_BODY()

	// The text that will be displayed if you have the flag
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Message")
	FText YouHaveFlagText;

	// The text that will be displayed if the enemy has your flag
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Message")
	FText EnemyHasFlagText;

	// The text that will be displayed if both your flag is out and you have an enemy flag
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Message")
	FText BothFlagsText;

	// The font to display in
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Message")
	UFont* MessageFont;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Message")
	float AnimationAlpha;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	TArray<FHUDRenderObject_Texture> CircleSlate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	TArray<FHUDRenderObject_Texture> CircleBorder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	FHUDRenderObject_Texture FlagIconTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	FHUDRenderObject_Texture DroppedIconTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	FHUDRenderObject_Texture TakenIconTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	FHUDRenderObject_Texture FlagArrowTemplate;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	FHUDRenderObject_Text FlagStatusText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	TArray<FHUDRenderObject_Text> FlagHolderNames;

	virtual void Draw_Implementation(float DeltaTime);
	virtual void InitializeWidget(AUTHUD* Hud);

};