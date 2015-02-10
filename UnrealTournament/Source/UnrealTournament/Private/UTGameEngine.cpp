// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTGameEngine.h"
#include "UTAnalytics.h"
#include "AssetRegistryModule.h"
#if !UE_SERVER
#include "SlateBasics.h"
#include "MoviePlayer.h"
#include "Private/Slate/SUWindowsStyle.h"
#endif

UUTGameEngine::UUTGameEngine(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bFirstRun = true;
	bAllowClientNetProfile = false;
	ReadEULACaption = NSLOCTEXT("UTGameEngine", "ReadEULACaption", "READ ME FIRST");
	ReadEULAText = NSLOCTEXT("UTGameEngine", "ReadEULAText", "EULA TEXT");
	GameNetworkVersion = 3008026;

	SmoothedDeltaTime = 0.01f;

	HitchTimeThreshold = 0.05f;
	HitchScaleThreshold = 2.f;
	HitchSmoothingRate = 0.5f;
	NormalSmoothingRate = 0.1f;
	MaximumSmoothedTime = 0.04f;

	ServerMaxPredictionPing = 160.f;
}


void UUTGameEngine::Init(IEngineLoop* InEngineLoop)
{
	// @TODO FIXMESTEVE temp hack for network compatibility code
	GEngineNetVersion = GameNetworkVersion;
	UE_LOG(UT, Warning, TEXT("************************************Set Net Version %d"), GEngineNetVersion);

	if (bFirstRun)
	{
		if (FPlatformMisc::MessageBoxExt(EAppMsgType::YesNo, *ReadEULAText.ToString(), *ReadEULACaption.ToString()) != EAppReturnType::Yes)
		{
			FPlatformMisc::RequestExit(false);
			return;
		}

		bFirstRun = false;
		SaveConfig();
		GConfig->Flush(false);
	}

	LoadDownloadedAssetRegistries();

	FUTAnalytics::Initialize();
	Super::Init(InEngineLoop);

	// HACK: UGameUserSettings::ApplyNonResolutionSettings() isn't virtual so we need to force our settings to be applied...
	GetGameUserSettings()->ApplySettings(true);

	UE_LOG(UT, Log, TEXT("Running %d processors (%d logical cores)"), FPlatformMisc::NumberOfCores(), FPlatformMisc::NumberOfCoresIncludingHyperthreads());
	if (FPlatformMisc::NumberOfCoresIncludingHyperthreads() < ParallelRendererProcessorRequirement)
	{
		UE_LOG(UT, Log, TEXT("Enabling r.RHICmdBypass due to not having %d logical cores"), ParallelRendererProcessorRequirement);
		IConsoleVariable* BypassVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RHICmdBypass"));
		BypassVar->Set(1);
	}
}

void UUTGameEngine::PreExit()
{
	Super::PreExit();
	FUTAnalytics::Shutdown();
}

// @TODO FIXMESTEVE - we want open to be relative like it used to be
bool UUTGameEngine::HandleOpenCommand(const TCHAR* Cmd, FOutputDevice& Ar, UWorld *InWorld)
{
	return HandleTravelCommand(Cmd, Ar, InWorld);
}

bool UUTGameEngine::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Out)
{
	if (FParse::Command(&Cmd, TEXT("START")))
	{
		FWorldContext &WorldContext = GetWorldContextFromWorldChecked(InWorld);
		FURL TestURL(&WorldContext.LastURL, Cmd, TRAVEL_Absolute);
		// make sure the file exists if we are opening a local file
		if (TestURL.IsLocalInternal() && !MakeSureMapNameIsValid(TestURL.Map))
		{
			Out.Logf(TEXT("ERROR: The map '%s' does not exist."), *TestURL.Map);
			return true;
		}
		else
		{
			SetClientTravel(InWorld, Cmd, TRAVEL_Absolute);
			return true;
		}
	}
	else if (FParse::Command(&Cmd, TEXT("GAMEVER")) || FParse::Command(&Cmd, TEXT("GAMEVERSION")))
	{
		FString VersionString = FString::Printf(TEXT("GameVersion Date: %s Time: %s"),
			TEXT(__DATE__), TEXT(__TIME__));

		Out.Logf(*VersionString);
		FPlatformMisc::ClipboardCopy(*VersionString);
		return true;
	}
	else
	{
		return Super::Exec(InWorld, Cmd, Out);
	}

}

void UUTGameEngine::Tick(float DeltaSeconds, bool bIdleMode)
{
	// HACK: make sure our default URL options are in all travel URLs since FURL code to do this was removed
	for (int32 WorldIdx = 0; WorldIdx < WorldList.Num(); ++WorldIdx)
	{
		FWorldContext& Context = WorldList[WorldIdx];
		if (!Context.TravelURL.IsEmpty())
		{
			FURL DefaultURL;
			DefaultURL.LoadURLConfig(TEXT("DefaultPlayer"), GGameIni);
			FURL NewURL(&DefaultURL, *Context.TravelURL, TRAVEL_Absolute);
			for (int32 i = 0; i < DefaultURL.Op.Num(); i++)
			{
				FString OpKey;
				DefaultURL.Op[i].Split(TEXT("="), &OpKey, NULL);
				if (!NewURL.HasOption(*OpKey))
				{
					new(NewURL.Op) FString(DefaultURL.Op[i]);
				}
			}
			UUTLocalPlayer* UTLocalPlayer = Cast<UUTLocalPlayer>(GetLocalPlayerFromControllerId(GWorld,0));
			if (UTLocalPlayer)
			{
				if (NewURL.HasOption(TEXT("Rank")))
				{
					NewURL.RemoveOption(TEXT("Rank"));
				}

				NewURL.AddOption(*FString::Printf(TEXT("Rank=%i"),UTLocalPlayer->GetBaseELORank()));
			}

			Context.TravelURL = NewURL.ToString();
		}
	}
	
	Super::Tick(DeltaSeconds, bIdleMode);
}

EBrowseReturnVal::Type UUTGameEngine::Browse( FWorldContext& WorldContext, FURL URL, FString& Error )
{
#if !UE_SERVER && !UE_EDITOR
	if (URL.Valid && URL.HasOption(TEXT("downloadfiles")))
	{
		WorldContext.TravelURL = TEXT("");

		// Need to download files, load default map
		if (WorldContext.PendingNetGame)
		{
			CancelPending(WorldContext);
		}
		// Handle failure URL.
		UE_LOG(LogNet, Log, TEXT("%s"), TEXT("Returning to Entry"));
		if (WorldContext.World() != NULL)
		{
			ResetLoaders(WorldContext.World()->GetOuter());
		}

		const UGameMapsSettings* GameMapsSettings = GetDefault<UGameMapsSettings>();
		bool LoadSuccess = LoadMap(WorldContext, FURL(&URL, *(GameMapsSettings->GetGameDefaultMap() + GameMapsSettings->LocalMapOptions), TRAVEL_Partial), NULL, Error);
		if (LoadSuccess == false)
		{
			UE_LOG(LogNet, Fatal, TEXT("Failed to load default map (%s). Error: (%s)"), *(GameMapsSettings->GetGameDefaultMap() + GameMapsSettings->LocalMapOptions), *Error);
		}

		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

		// now remove "downloadfiles" options from LastURL so it doesn't get copied on to future URLs
		WorldContext.LastURL.RemoveOption(TEXT("downloadfiles"));
		
		BroadcastTravelFailure(WorldContext.World(), ETravelFailure::PackageMissing, TEXT("DownloadFiles"));

		return (LoadSuccess ? EBrowseReturnVal::Success : EBrowseReturnVal::Failure);
	}
#endif

	return Super::Browse(WorldContext, URL, Error);
}

static TAutoConsoleVariable<int32> CVarUnsteadyFPS(
	TEXT("ut.UnsteadyFPS"), 0,
	TEXT("Causes FPS to bounce around randomly in the 85-120 range."));

static TAutoConsoleVariable<int32> CVarSmoothFrameRate(
	TEXT("ut.SmoothFrameRate"), 1,
	TEXT("Enable frame rate smoothing."));

float UUTGameEngine::GetMaxTickRate(float DeltaTime, bool bAllowFrameRateSmoothing) const
{
	float MaxTickRate = 0;

	// Don't smooth here if we're a dedicated server
	if (IsRunningDedicatedServer())
	{
		UWorld* World = NULL;

		for (int32 WorldIndex = 0; WorldIndex < WorldList.Num(); ++WorldIndex)
		{
			if (WorldList[WorldIndex].WorldType == EWorldType::Game)
			{
				World = WorldList[WorldIndex].World();
				break;
			}
		}

		if (World)
		{
			UNetDriver* NetDriver = World->GetNetDriver();
			// In network games, limit framerate to not saturate bandwidth.
			if (NetDriver && (NetDriver->GetNetMode() == NM_DedicatedServer || (NetDriver->GetNetMode() == NM_ListenServer && NetDriver->bClampListenServerTickRate)))
			{
				// We're a dedicated server, use the LAN or Net tick rate.
				MaxTickRate = FMath::Clamp(NetDriver->NetServerMaxTickRate, 10, 120);
			}
		}
		return MaxTickRate;
	}

	if (CVarUnsteadyFPS.GetValueOnGameThread())
	{
		float RandDelta = 0.f;
		// random variation in frame time because of effects, etc.
		if (FMath::FRand() < 0.5f)
		{
			RandDelta = (1.f+FMath::FRand()) * DeltaTime;
		}
		// occasional large hitches
		if (FMath::FRand() < 0.002f)
		{
			RandDelta += 0.1f;
		}

		DeltaTime += RandDelta;
		MaxTickRate = 1.f / DeltaTime;
		//UE_LOG(UT, Warning, TEXT("FORCING UNSTEADY FRAME RATE desired delta %f adding %f maxtickrate %f"),  DeltaTime, RandDelta, MaxTickRate);
	}

	if (bSmoothFrameRate && bAllowFrameRateSmoothing && CVarSmoothFrameRate.GetValueOnGameThread())
	{
		MaxTickRate = 1.f / SmoothedDeltaTime;
	}

	// Hard cap at frame rate cap
	if (FrameRateCap > 0)
	{
		if (MaxTickRate > 0)
		{
			MaxTickRate = FMath::Min(FrameRateCap, MaxTickRate);
		}
		else
		{
			MaxTickRate = FrameRateCap;
		}
	}
	
	return MaxTickRate;
}

void UUTGameEngine::UpdateRunningAverageDeltaTime(float DeltaTime, bool bAllowFrameRateSmoothing)
{
	if (DeltaTime > SmoothedDeltaTime)
	{
		// can't slow down drop
		SmoothedDeltaTime = DeltaTime;
		// return 0.f; // @TODO FIXMESTEVE - not doing this so unsteady FPS is valid
	}
	else if (SmoothedDeltaTime > FMath::Max(HitchTimeThreshold, HitchScaleThreshold * DeltaTime))
	{
		// fast return from hitch - smoothing them makes game crappy a long time
		HitchSmoothingRate = FMath::Clamp(HitchSmoothingRate, 0.f, 1.f);
		SmoothedDeltaTime = (1.f - HitchSmoothingRate)*SmoothedDeltaTime + HitchSmoothingRate*DeltaTime;
	}
	else
	{
		// simply smooth the trajectory back up to limit mouse input variations because of difference in sampling frame time and this frame time
		NormalSmoothingRate = FMath::Clamp(NormalSmoothingRate, 0.f, 1.f);

		SmoothedDeltaTime = (1.f - NormalSmoothingRate)*SmoothedDeltaTime + NormalSmoothingRate*DeltaTime;
	}

	// Make sure that we don't try to smooth below a certain minimum
	SmoothedDeltaTime = FMath::Clamp(SmoothedDeltaTime, 0.001f, MaximumSmoothedTime);

	//UE_LOG(UT, Warning, TEXT("SMOOTHED TO %f"), SmoothedDeltaTime);
}

void UUTGameEngine::LoadDownloadedAssetRegistries()
{
	// Plugin manager should handle this instead of us, but we're not using plugin-based dlc just yet
	if (FPlatformProperties::RequiresCookedData())
	{
		// Helper class to find all pak files.
		class FPakFileSearchVisitor : public IPlatformFile::FDirectoryVisitor
		{
			TArray<FString>& FoundFiles;
		public:
			FPakFileSearchVisitor(TArray<FString>& InFoundFiles)
				: FoundFiles(InFoundFiles)
			{}
			virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory)
			{
				if (bIsDirectory == false)
				{
					FString Filename(FilenameOrDirectory);
					if (Filename.MatchesWildcard(TEXT("*.pak")))
					{
						FoundFiles.Add(Filename);
					}
				}
				return true;
			}
		};

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

		// Search for pak files that were downloaded through redirects
		TArray<FString>	FoundPaks;
		FPakFileSearchVisitor PakVisitor(FoundPaks);
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		FoundPaks.Empty();
		PlatformFile.IterateDirectoryRecursively(*FPaths::Combine(*FPaths::GameSavedDir(), TEXT("Paks"), TEXT("Downloads")), PakVisitor);
		for (const auto& PakPath : FoundPaks)
		{
			FString PakFilename = FPaths::GetBaseFilename(PakPath);

			TArray<uint8> Data;
			if (FFileHelper::LoadFileToArray(Data, *PakPath))
			{
				uint32 CRC32 = FCrc::MemCrc32(Data.GetData(), Data.Num());
				DownloadedContentCRCs.Add(PakFilename, CRC32);
			}
		}

		FoundPaks.Empty();
		PlatformFile.IterateDirectoryRecursively(*FPaths::Combine(*FPaths::GameSavedDir(), TEXT("Paks"), TEXT("MyContent")), PakVisitor);
		for (const auto& PakPath : FoundPaks)
		{
			FString PakFilename = FPaths::GetBaseFilename(PakPath);
			
			TArray<uint8> Data;
			if (FFileHelper::LoadFileToArray(Data, *PakPath))
			{
				uint32 CRC32 = FCrc::MemCrc32(Data.GetData(), Data.Num());
				MyContentCRCs.Add(PakFilename, CRC32);
			}

			FArrayReader SerializedAssetData;
			int32 DashPosition = PakFilename.Find(TEXT("-"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
			if (DashPosition != -1)
			{
				PakFilename = PakFilename.Left(DashPosition);
				if (!PakFilename.Equals(TEXT("UnrealTournament"), ESearchCase::IgnoreCase))
				{
					FString AssetRegistryName = PakFilename + TEXT("-AssetRegistry.bin");
					if (FFileHelper::LoadFileToArray(SerializedAssetData, *(FPaths::GameDir() / AssetRegistryName)))
					{
						// serialize the data with the memory reader (will convert FStrings to FNames, etc)
						AssetRegistryModule.Get().Serialize(SerializedAssetData);
					}
					else
					{
						UE_LOG(UT, Warning, TEXT("%s could not be found"), *AssetRegistryName);
					}
				}
			}
		}
	}
}

void UUTGameEngine::SetupLoadingScreen()
{
#if !UE_SERVER
	if (IsMoviePlayerEnabled())
	{
		FLoadingScreenAttributes LoadingScreen;
		LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
		// TODO: probably need to do something to handle aspect ratio
		//LoadingScreen.WidgetLoadingScreen = SNew(SImage).Image(&LoadingScreenImage);
		LoadingScreen.WidgetLoadingScreen = SNew(SImage).Image(SUWindowsStyle::Get().GetBrush("LoadingScreen"));
		GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
		FCoreUObjectDelegates::PreLoadMap.Broadcast();
	}
#endif
}