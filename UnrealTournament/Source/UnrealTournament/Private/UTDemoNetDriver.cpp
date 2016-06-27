#include "UnrealTournament.h"
#include "UTDemoNetDriver.h"
#include "UTGameInstance.h"
#include "UTGameEngine.h"

void UUTDemoNetDriver::WriteGameSpecificDemoHeader(TArray<FString>& GameSpecificData)
{
	AUTBaseGameMode* Game = GetWorld()->GetAuthGameMode<AUTBaseGameMode>();
	if (Game != NULL)
	{
		TArray<FPackageRedirectReference> AllRedirects;
		Game->GatherRequiredRedirects(AllRedirects);
		for (const FPackageRedirectReference& Redirect : AllRedirects)
		{
			GameSpecificData.Add(FString::Printf(TEXT("%s\n%s\n%s"), *Redirect.PackageName, *Redirect.ToString(), *Redirect.PackageChecksum));
		}
	}
}

bool UUTDemoNetDriver::ShouldSaveCheckpoint()
{
#if UE_SERVER
	return Super::ShouldSaveCheckpoint();
#endif

	const double TimeElapsed = DemoCurrentTime - LastCheckpointTime;

	if (TimeElapsed > 500.0)
	{
		return true;
	}

	if (TimeElapsed > 60)
	{
		// If playercontroller has no pawn, probably dead and ok to checkpoint
		APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (PC)
		{
			if (PC->GetPawn() == nullptr)
			{
				return true;
			}
		}
		else
		{
			return Super::ShouldSaveCheckpoint();
		}
	}

	return false;
}
bool UUTDemoNetDriver::ProcessGameSpecificDemoHeader(const TArray<FString>& GameSpecificData, FString& Error)
{
	UUTGameInstance* GI = Cast<UUTGameInstance>(GetWorld()->GetGameInstance());
	if (GI == NULL || GameSpecificData.Num() == 0 || GI->GetLastTriedDemo() == DemoFilename)
	{ 
		return true;
	}
	else
	{
		bool bDownloading = false;
		UUTGameEngine* Engine = Cast<UUTGameEngine>(GEngine);
		for (const FString& Item : GameSpecificData)
		{
			TArray<FString> Pieces;
			Item.ParseIntoArray(Pieces, TEXT("\n"), false);
			if (Pieces.Num() == 3)
			{
				// 0: pak name, 1: URL, 2: checksum
				bDownloading = GI->StartRedirectDownload(Pieces[0], Pieces[1], Pieces[2]) || bDownloading;
			}
		}
		if (bDownloading)
		{
			GI->SetLastTriedDemo(DemoFilename);
			Error = TEXT("Waiting for redirects to download files");
			return false;
		}
		else
		{
			return true;
		}
	}
}