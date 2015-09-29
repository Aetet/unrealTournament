// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTReplicatedGameRuleset.h"
#include "UTReplicatedMapInfo.h"
#include "Net/UnrealNetwork.h"

#if !UE_SERVER
#include "Slate/SUWindowsStyle.h"
#endif

AUTReplicatedGameRuleset::AUTReplicatedGameRuleset(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bAlwaysRelevant = true;
	bReplicateMovement = false;
	bNetLoadOnClient = false;
}

void AUTReplicatedGameRuleset::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AUTReplicatedGameRuleset, UniqueTag);
	DOREPLIFETIME(AUTReplicatedGameRuleset, Categories);
	DOREPLIFETIME(AUTReplicatedGameRuleset, Title);
	DOREPLIFETIME(AUTReplicatedGameRuleset, Tooltip);
	DOREPLIFETIME(AUTReplicatedGameRuleset, Description);
	DOREPLIFETIME(AUTReplicatedGameRuleset, MaxMapsInList);
	DOREPLIFETIME(AUTReplicatedGameRuleset, MapList);
	DOREPLIFETIME(AUTReplicatedGameRuleset, MinPlayersToStart);
	DOREPLIFETIME(AUTReplicatedGameRuleset, MaxPlayers);
	DOREPLIFETIME(AUTReplicatedGameRuleset, OptimalPlayers);
	DOREPLIFETIME(AUTReplicatedGameRuleset, DisplayTexture);
	DOREPLIFETIME(AUTReplicatedGameRuleset, bCustomRuleset);
	DOREPLIFETIME(AUTReplicatedGameRuleset, GameMode);
	DOREPLIFETIME(AUTReplicatedGameRuleset, bTeamGame);
}


int32 AUTReplicatedGameRuleset::AddMapAssetToMapList(const FAssetData& Asset)
{

	AUTGameState* GameState = GetWorld()->GetGameState<AUTGameState>();
	if (GameState)
	{
		AUTReplicatedMapInfo* MapInfo = GameState->CreateMapInfo(Asset);
		return MapList.Add(MapInfo);
	}

	return INDEX_NONE;
}


void AUTReplicatedGameRuleset::SetRules(UUTGameRuleset* NewRules, const TArray<FAssetData>& MapAssets)
{
	UniqueTag			= NewRules->UniqueTag;
	Categories			= NewRules->Categories;
	Title				= NewRules->Title;
	Tooltip				= NewRules->Tooltip;
	Description			= Fixup(NewRules->Description);
	MinPlayersToStart	= NewRules->MinPlayersToStart;
	MaxPlayers			= NewRules->MaxPlayers;
	bTeamGame			= NewRules->bTeamGame;
	DefaultMap			= NewRules->DefaultMap;
	QuickPlayMaps		= NewRules->QuickPlayMaps;

	MaxMapsInList = NewRules->MaxMapsInList;

	// First add the Epic maps.
	if (!NewRules->EpicMaps.IsEmpty())
	{
		TArray<FString> EpicMapList;
		NewRules->EpicMaps.ParseIntoArray(EpicMapList,TEXT(","), true);
		for (int32 i = 0 ; i < EpicMapList.Num(); i++)
		{
			FString MapName = EpicMapList[i];
			if ( !MapName.IsEmpty() )
			{
				if ( FPackageName::IsShortPackageName(MapName) )
				{
					FPackageName::SearchForPackageOnDisk(MapName, &MapName); 
				}

				// Look for the map in the asset registry...

				for (const FAssetData& Asset : MapAssets)
				{
					FString AssetPackageName = Asset.PackageName.ToString();
					if ( MapName.Equals(AssetPackageName, ESearchCase::IgnoreCase) )
					{
						// Found the asset data for this map.  Build the FMapListInfo.
						AddMapAssetToMapList(Asset);
						break;
					}
				}
			}
		}
	}

	// Now add the custom maps..
	for (int32 i = 0; i < NewRules->CustomMapList.Num(); i++)
	{
		if (MaxMapsInList > 0 && MapList.Num() >= MaxMapsInList) break;

		FString MapPackageName = NewRules->CustomMapList[i];
		if ( FPackageName::IsShortPackageName(MapPackageName) )
		{
			if (!FPackageName::SearchForPackageOnDisk(MapPackageName, &MapPackageName))
			{
				UE_LOG(UT,Log,TEXT("Failed to find package for shortname '%s'"), *MapPackageName);
			}
		}

		// Look for the map in the asset registry...
		for (const FAssetData& Asset : MapAssets)
		{
			FString AssetPackageName = Asset.PackageName.ToString();
			if ( MapPackageName.Equals(AssetPackageName, ESearchCase::IgnoreCase) )
			{
				// Found the asset data for this map.  Build the FMapListInfo.
				int32 Idx = AddMapAssetToMapList(Asset);

				AUTBaseGameMode* DefaultBaseGameMode = GetWorld()->GetAuthGameMode<AUTBaseGameMode>();
				if (DefaultBaseGameMode)
				{
					// Look to see if there are redirects for this map
					
					FPackageRedirectReference Redirect;
					if ( DefaultBaseGameMode->FindRedirect(NewRules->CustomMapList[i], Redirect) )
					{
						MapList[Idx]->Redirect = Redirect;
					}
				}
				break;
			}
		}
	}

	RequiredPackages = NewRules->RequiredPackages;
	DisplayTexture = NewRules->DisplayTexture;
	GameMode = NewRules->GameMode;
	GameOptions = NewRules->GameOptions;
	BuildSlateBadge();

}

FString AUTReplicatedGameRuleset::Fixup(FString OldText)
{
	FString Final = OldText.Replace(TEXT("\\n"), TEXT("\n"), ESearchCase::IgnoreCase);
	Final = Final.Replace(TEXT("\\n"), TEXT("\n"), ESearchCase::IgnoreCase);

	return Final;
}


void AUTReplicatedGameRuleset::BuildSlateBadge()
{
#if !UE_SERVER
	BadgeTexture = LoadObject<UTexture2D>(nullptr, *DisplayTexture, nullptr, LOAD_None, nullptr);
	SlateBadge = new FSlateDynamicImageBrush(BadgeTexture, FVector2D(256.0f, 256.0f), NAME_None);
#endif
}

#if !UE_SERVER
const FSlateBrush* AUTReplicatedGameRuleset::GetSlateBadge() const
{
	if (SlateBadge != nullptr) 
	{
		return SlateBadge;
	}
	else
	{
		return SUWindowsStyle::Get().GetBrush("UWindows.Lobby.MatchBadge");	
	}
}
#endif

void AUTReplicatedGameRuleset::GotTag()
{
}

AUTGameMode* AUTReplicatedGameRuleset::GetDefaultGameModeObject()
{
	if (!GameMode.IsEmpty())
	{
		FString LongGameModeClassname = AGameMode::StaticGetFullGameClassName(GameMode);
		UClass* GModeClass = LoadClass<AUTGameMode>(NULL, *LongGameModeClassname, NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
		if (GModeClass)
		{
			AUTGameMode* DefaultGameModeObject = GModeClass->GetDefaultObject<AUTGameMode>();
			return DefaultGameModeObject;
		}
	
	}
	else
	{
		UE_LOG(UT, Warning, TEXT("%s Empty GameModeClass for Ruleset %s"), *GetName(), *Title);
	}

	return NULL;
}