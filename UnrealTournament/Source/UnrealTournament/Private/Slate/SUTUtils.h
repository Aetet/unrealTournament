// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateBasics.h"
#include "Base/SUTDialogBase.h"

class SUTUtils
{
public:
	static TSharedRef<SToolTip> CreateTooltip(const TAttribute<FText>& Text);
};