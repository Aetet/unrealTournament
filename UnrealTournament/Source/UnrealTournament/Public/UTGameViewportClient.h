// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "UTGameViewportClient.generated.h"

// Used to hold a list of items for redirect download.
USTRUCT()
struct FPendingRedirect
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString FileURL;

	UPROPERTY()
	TEnumAsByte<ERedirectStatus::Type> Status;

	FHttpRequestPtr HttpRequest;

	FPendingRedirect()
		: FileURL(TEXT(""))
	{
	}
	
	FPendingRedirect(FString inFileURL)
	{
		FileURL = inFileURL;
		Status = ERedirectStatus::Pending;
	}
};



UCLASS()
class UNREALTOURNAMENT_API UUTGameViewportClient : public UGameViewportClient
{
	GENERATED_UCLASS_BODY()

	virtual void AddViewportWidgetContent(TSharedRef<class SWidget> ViewportContent, const int32 ZOrder = 0) override;
	virtual void RemoveViewportWidgetContent(TSharedRef<class SWidget> ViewportContent) override;

	/**Only use these NoAspect versions if you really need to*/
	virtual void AddViewportWidgetContent_NoAspect(TSharedRef<class SWidget> ViewportContent, const int32 ZOrder = 0);
	virtual void RemoveViewportWidgetContent_NoAspect(TSharedRef<class SWidget> ViewportContent);

	virtual void PeekTravelFailureMessages(UWorld* World, enum ETravelFailure::Type FailureType, const FString& ErrorString) override;
	virtual void PeekNetworkFailureMessages(UWorld *World, UNetDriver *NetDriver, enum ENetworkFailure::Type FailureType, const FString& ErrorString) override;

	virtual void Draw(FViewport* InViewport, FCanvas* SceneCanvas) override;
	virtual void FinalizeViews(FSceneViewFamily* ViewFamily, const TMap<ULocalPlayer*, FSceneView*>& PlayerViewMap) override;
	virtual void UpdateActiveSplitscreenType() override;
	virtual void PostRender(UCanvas* Canvas) override;
	virtual ULocalPlayer* SetupInitialLocalPlayer(FString& OutError) override;
	virtual void Tick(float DeltaSeconds) override;

	/** panini project the given location using the player's view
	 * PaniniParamsMat will be used to grab parameters for the projection if available, otherwise reasonable default values are used
	 */
	FVector PaniniProjectLocation(const FSceneView* SceneView, const FVector& WorldLoc, UMaterialInterface* PaniniParamsMat = NULL) const;
	/** calls PaniniProjectLocation() with a SceneView representing the player's view (slower, don't use if SceneView is already available) */
	FVector PaniniProjectLocationForPlayer(ULocalPlayer* Player, const FVector& WorldLoc, UMaterialInterface* PaniniParamsMat = NULL) const;

protected:

	TWeakPtr<class SUTGameLayerManager> LayerManagerPtr;

	TSharedPtr<class SUWDialog> ReconnectDialog;
	TSharedPtr<class SUWRedirectDialog> RedirectDialog;

	// Holds the IP/Port of the last connect so we can try to reconnect
	FURL LastAttemptedURL;
	float ReconnectAfterDownloadingMapDelay;
	float VerifyFilesToDownloadAndReconnectDelay;

	virtual void RankDialogResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID);
	virtual void NetworkFailureDialogResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID);
	virtual void LoginFailureDialogResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID);
	virtual void ConnectPasswordResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID);
	virtual void RedirectResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID);
	virtual void CloudRedirectResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID);

	virtual void VerifyFilesToDownloadAndReconnect();
	virtual void ReconnectAfterDownloadingContent();

	FContentDownloadComplete ContentDownloadComplete;

	// holds a list of redirects to download.
	TArray<FPendingRedirect> PendingDownloads;

	// if there are pending redirects, this function is called each tick.  NOTE: it will only be called if there are pending directs.
	virtual void UpdateRedirects(float DeltaTime);

	void HttpRequestProgress(FHttpRequestPtr HttpRequest, int32 NumBytesSent, int32 NumBytesRecv);
	void HttpRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);

public:

	// Returns TRUE if a given direct reference exists on this machine
	bool CheckIfRedirectExists(const FPackageRedirectReference& Redirect);

	// Returns TRUE if there is currently a download in progress.
	virtual bool IsDownloadInProgress();

	// Adds a redirect to the download queue and downloads it.
	virtual void DownloadRedirect(FPackageRedirectReference Redirect);

	// Cancel a given redirect.  If this download is currently in progress it is ended and the next in the queue started.  
	virtual void CancelRedirect(FPackageRedirectReference Redirect);

	// Cancel all downloads.
	virtual void CancelAllRedirectDownloads();

	/**
	 *	Gives a call back when the download has completed
	 **/
	virtual FDelegateHandle RegisterContentDownloadCompleteDelegate(const FContentDownloadComplete::FDelegate& NewDelegate);

	/**
	 *	Removes the  call back to an object looking to know when a player's status changed.
	 **/
	virtual void RemoveContentDownloadCompleteDelegate(FDelegateHandle DelegateHandle);

	virtual bool HideCursorDuringCapture() override;

	// Will be called when a client connects to a server.
	void ClientConnectedToServer();

protected:
	TMap<FString, FString> SavedPasswords;
	FString LastPasswordAttemptHost;

};

