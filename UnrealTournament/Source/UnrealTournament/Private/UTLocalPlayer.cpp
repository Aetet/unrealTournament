// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLocalPlayer.h"
#include "UTCharacter.h"
#include "Online.h"
#include "OnlineSubsystemTypes.h"
#include "UTMenuGameMode.h"
#include "UTProfileSettings.h"
#include "UTGameViewportClient.h"
#include "Slate/SUWindowsDesktop.h"
#include "Slate/SUWindowsMainMenu.h"
#include "Slate/Panels/SUWServerBrowser.h"
#include "Slate/Panels/SUWReplayBrowser.h"
#include "Slate/Panels/SUWStatsViewer.h"
#include "Slate/Panels/SUWCreditsPanel.h"
#include "Slate/SUWMessageBox.h"
#include "Slate/SUWindowsStyle.h"
#include "Slate/SUTStyle.h"
#include "Slate/SUWDialog.h"
#include "Slate/SUWToast.h"
#include "Slate/SUWInputBox.h"
#include "Slate/SUWLoginDialog.h"
#include "Slate/SUWPlayerSettingsDialog.h"
#include "Slate/SUWPlayerInfoDialog.h"
#include "Slate/SUWHUDSettingsDialog.h"
#include "Slate/SUTQuickMatch.h"
#include "Slate/SUTJoinInstance.h"
#include "Slate/SUWFriendsPopup.h"
#include "Slate/SUWRedirectDialog.h"
#include "Slate/SUWVideoCompressionDialog.h"
#include "Slate/SUTLoadoutMenu.h"
#include "Slate/SUTBuyMenu.h"
#include "Slate/SUWMapVoteDialog.h"
#include "Slate/SUTReplayWindow.h"
#include "Slate/SUTReplayMenu.h"
#include "UTAnalytics.h"
#include "FriendsAndChat.h"
#include "Runtime/Analytics/Analytics/Public/Analytics.h"
#include "Runtime/Analytics/Analytics/Public/Interfaces/IAnalyticsProvider.h"
#include "Base64.h"
#include "UTGameEngine.h"
#include "Engine/DemoNetDriver.h"
#include "UTConsole.h"
#include "Runtime/Core/Public/Features/IModularFeatures.h"
#include "UTVideoRecordingFeature.h"
#include "Slate/SUWYoutubeUpload.h"
#include "Slate/SUWYoutubeConsent.h"
#include "Slate/SUWMatchSummary.h"
#include "UTLobbyGameState.h"
#include "StatNames.h"
#include "UTChallengeManager.h"

UUTLocalPlayer::UUTLocalPlayer(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bInitialSignInAttempt = true;
	LastProfileCloudWriteTime = 0;
	ProfileCloudWriteCooldownTime = 15;
	bShowSocialNotification = false;
	ServerPingBlockSize = 30;
	bSuppressToastsInGame = false;
	DownloadStatusText = FText::GetEmpty();
	QuickMatchLimitTime = -60.0;
}

UUTLocalPlayer::~UUTLocalPlayer()
{
	// Terminate the dedicated server if we started one
	if (DedicatedServerProcessHandle.IsValid() && FPlatformProcess::IsProcRunning(DedicatedServerProcessHandle))
	{
		FPlatformProcess::TerminateProc(DedicatedServerProcessHandle);
	}
}

void UUTLocalPlayer::InitializeOnlineSubsystem()
{
	OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem) 
	{
		OnlineIdentityInterface = OnlineSubsystem->GetIdentityInterface();
		OnlineUserCloudInterface = OnlineSubsystem->GetUserCloudInterface();
		OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();
		OnlinePresenceInterface = OnlineSubsystem->GetPresenceInterface();
		OnlineFriendsInterface = OnlineSubsystem->GetFriendsInterface();
	}

	if (OnlineIdentityInterface.IsValid())
	{
		OnLoginCompleteDelegate = OnlineIdentityInterface->AddOnLoginCompleteDelegate_Handle(GetControllerId(), FOnLoginCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnLoginComplete));
		OnLoginStatusChangedDelegate = OnlineIdentityInterface->AddOnLoginStatusChangedDelegate_Handle(GetControllerId(), FOnLoginStatusChangedDelegate::CreateUObject(this, &UUTLocalPlayer::OnLoginStatusChanged));
		OnLogoutCompleteDelegate = OnlineIdentityInterface->AddOnLogoutCompleteDelegate_Handle(GetControllerId(), FOnLogoutCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnLogoutComplete));
	}

	if (OnlineUserCloudInterface.IsValid())
	{
		OnReadUserFileCompleteDelegate = OnlineUserCloudInterface->AddOnReadUserFileCompleteDelegate_Handle(FOnReadUserFileCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnReadUserFileComplete));
		OnWriteUserFileCompleteDelegate = OnlineUserCloudInterface->AddOnWriteUserFileCompleteDelegate_Handle(FOnWriteUserFileCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnWriteUserFileComplete));
		OnDeleteUserFileCompleteDelegate = OnlineUserCloudInterface->AddOnDeleteUserFileCompleteDelegate_Handle(FOnDeleteUserFileCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnDeleteUserFileComplete));
		OnEnumerateUserFilesCompleteDelegate = OnlineUserCloudInterface->AddOnEnumerateUserFilesCompleteDelegate_Handle(FOnEnumerateUserFilesCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnEnumerateUserFilesComplete));
	}

	if (OnlineSessionInterface.IsValid())
	{
		OnJoinSessionCompleteDelegate = OnlineSessionInterface->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnJoinSessionComplete));
	}
	
}

void UUTLocalPlayer::CleanUpOnlineSubSystyem()
{
	if (OnlineSubsystem)
	{
		if (OnlineIdentityInterface.IsValid())
		{
			OnlineIdentityInterface->ClearOnLoginCompleteDelegate_Handle(GetControllerId(), OnLoginCompleteDelegate);
			OnlineIdentityInterface->ClearOnLoginStatusChangedDelegate_Handle(GetControllerId(), OnLoginStatusChangedDelegate);
			OnlineIdentityInterface->ClearOnLogoutCompleteDelegate_Handle(GetControllerId(), OnLogoutCompleteDelegate);
		}

		if (OnlineUserCloudInterface.IsValid())
		{
			OnlineUserCloudInterface->ClearOnReadUserFileCompleteDelegate_Handle(OnReadUserFileCompleteDelegate);
			OnlineUserCloudInterface->ClearOnWriteUserFileCompleteDelegate_Handle(OnWriteUserFileCompleteDelegate);
			OnlineUserCloudInterface->ClearOnDeleteUserFileCompleteDelegate_Handle(OnDeleteUserFileCompleteDelegate);
			OnlineUserCloudInterface->ClearOnEnumerateUserFilesCompleteDelegate_Handle(OnEnumerateUserFilesCompleteDelegate);
		}

		if (OnlineSessionInterface.IsValid())
		{
			OnlineSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);
		}
	}
}

bool UUTLocalPlayer::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	// disallow certain commands in shipping builds
#if (UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (FParse::Command(&Cmd, TEXT("SHOW")))
	{
		return true;
	}
#endif
	return Super::Exec(InWorld, Cmd, Ar);
}

bool UUTLocalPlayer::IsAFriend(FUniqueNetIdRepl PlayerId)
{
	return (PlayerId.IsValid() && OnlineFriendsInterface.IsValid() && OnlineFriendsInterface->IsFriend(0, *PlayerId, EFriendsLists::ToString(EFriendsLists::InGamePlayers)));
}

FString UUTLocalPlayer::GetNickname() const
{
	return PlayerNickname;
}

FText UUTLocalPlayer::GetAccountDisplayName() const
{
	if (OnlineIdentityInterface.IsValid() && PlayerController && PlayerController->PlayerState)
	{

		TSharedPtr<FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
		if (UserId.IsValid())
		{
			TSharedPtr<FUserOnlineAccount> UserAccount = OnlineIdentityInterface->GetUserAccount(*UserId);
			if (UserAccount.IsValid())
			{
				return FText::FromString(UserAccount->GetDisplayName());
			}
		}
	}

	return FText::GetEmpty();
}

FString UUTLocalPlayer::GetAccountName() const
{
	if (OnlineIdentityInterface.IsValid() && PlayerController && PlayerController->PlayerState)
	{
		TSharedPtr<FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
		if (UserId.IsValid())
		{
			TSharedPtr<FUserOnlineAccount> UserAccount = OnlineIdentityInterface->GetUserAccount(*UserId);
			if (UserAccount.IsValid())
			{
				FString Result;
				UserAccount->GetUserAccountData(TEXT("email"), Result);
				return Result;
			}
		}
	}

	return TEXT("");
}


FText UUTLocalPlayer::GetAccountSummary() const
{
	if (OnlineIdentityInterface.IsValid() && PlayerController && PlayerController->PlayerState)
	{

		TSharedPtr<FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
		if (UserId.IsValid())
		{
			TSharedPtr<FUserOnlineAccount> UserAccount = OnlineIdentityInterface->GetUserAccount(*UserId);
			if (UserAccount.IsValid())
			{
				return FText::Format(NSLOCTEXT("UTLocalPlayer","AccountSummaryFormat","{0} # of Friends: {1}  # Online: {2}"), FText::FromString(UserAccount->GetDisplayName()), FText::AsNumber(0),FText::AsNumber(0));
			}
		}
	}

	return FText::GetEmpty();
}



void UUTLocalPlayer::PlayerAdded(class UGameViewportClient* InViewportClient, int32 InControllerID)
{
#if !UE_SERVER
	SUWindowsStyle::Initialize();
	SUTStyle::Initialize();
#endif

	Super::PlayerAdded(InViewportClient, InControllerID);

	if (FUTAnalytics::IsAvailable())
	{
		FString OSMajor;
		FString OSMinor;

		FPlatformMisc::GetOSVersions(OSMajor, OSMinor);

		TArray<FAnalyticsEventAttribute> ParamArray;
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("OSMajor"), OSMajor));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("OSMinor"), OSMinor));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("CPUVendor"), FPlatformMisc::GetCPUVendor()));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("CPUBrand"), FPlatformMisc::GetCPUBrand()));
		FUTAnalytics::GetProvider().RecordEvent( TEXT("SystemInfo"), ParamArray );
	}

	if (!InViewportClient->GetWorld()->IsPlayInEditor())
	{
		if (!HasAnyFlags(RF_ClassDefaultObject))
		{
			// Initialize the Online Subsystem for this player
			InitializeOnlineSubsystem();

			if (OnlineIdentityInterface.IsValid())
			{
				// Attempt to Auto-Login to MCP
				if (!OnlineIdentityInterface->AutoLogin(GetControllerId()))
				{
					bInitialSignInAttempt = false;
				}
			}
		}
	}
}

bool UUTLocalPlayer::IsMenuGame()
{
	if (bNoMidGameMenu) return true;

	if (GetWorld()->GetNetMode() == NM_Standalone)
	{
		AUTMenuGameMode* GM = Cast<AUTMenuGameMode>(GetWorld()->GetAuthGameMode());
		return GM != NULL;
	}

	return false;
}


void UUTLocalPlayer::ShowMenu(const FString& Parameters)
{
#if !UE_SERVER
	
	if (bRecordingReplay)
	{
		static const FName VideoRecordingFeatureName("VideoRecording");
		if (IModularFeatures::Get().IsModularFeatureAvailable(VideoRecordingFeatureName))
		{
			UTVideoRecordingFeature* VideoRecorder = &IModularFeatures::Get().GetModularFeature<UTVideoRecordingFeature>(VideoRecordingFeatureName);
			if (VideoRecorder)
			{
				VideoRecorder->CancelRecording();
			}
		}
	}

	// Create the slate widget if it doesn't exist
	if (!DesktopSlateWidget.IsValid())
	{
		if ( IsMenuGame() )
		{
			SAssignNew(DesktopSlateWidget, SUWindowsMainMenu).PlayerOwner(this);
		}
		else if (IsReplay())
		{
			SAssignNew(DesktopSlateWidget, SUTReplayMenu).PlayerOwner(this);
		}
		else
		{

			AGameState* GameState = GetWorld()->GetGameState<AGameState>();
			if (GameState != nullptr && GameState->GameModeClass != nullptr)
			{
				AUTBaseGameMode* UTGameMode = GameState->GameModeClass->GetDefaultObject<AUTBaseGameMode>();
				if (UTGameMode != nullptr)
				{
					DesktopSlateWidget = UTGameMode->GetGameMenu(this);
				}
			}

		}
		if (DesktopSlateWidget.IsValid())
		{
			GEngine->GameViewport->AddViewportWidgetContent( SNew(SWeakWidget).PossiblyNullContent(DesktopSlateWidget.ToSharedRef()));
		}
	}

	// Make it visible.
	if (DesktopSlateWidget.IsValid())
	{
		// Widget is already valid, just make it visible.
		DesktopSlateWidget->SetVisibility(EVisibility::Visible);
		DesktopSlateWidget->OnMenuOpened(Parameters);

		if (PlayerController)
		{
			if (!IsMenuGame())
			{
				PlayerController->SetPause(true);
			}
		}
	}
#endif
}
void UUTLocalPlayer::HideMenu()
{
#if !UE_SERVER

	if (ContentLoadingMessage.IsValid())
	{
		UE_LOG(UT,Log,TEXT("Can't close menus during loading"));
		return; // Don't allow someone to close the menu while we are loading....
	}

	if (DesktopSlateWidget.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(DesktopSlateWidget.ToSharedRef());
		DesktopSlateWidget->OnMenuClosed();
		DesktopSlateWidget.Reset();
		if (PlayerController)
		{
			PlayerController->SetPause(false);
		}
	}
	else
	{
		UE_LOG(UT,Log,TEXT("Call to HideMenu() when without a menu being opened."));
	}
	CloseConnectingDialog();
#endif
}

void UUTLocalPlayer::OpenTutorialMenu()
{
#if !UE_SERVER
	if (IsMenuGame() && DesktopSlateWidget.IsValid())
	{
		StaticCastSharedPtr<SUWindowsMainMenu>(DesktopSlateWidget)->OpenTutorialMenu();
	}
#endif
}

void UUTLocalPlayer::MessageBox(FText MessageTitle, FText MessageText)
{
#if !UE_SERVER
	ShowMessage(MessageTitle, MessageText, UTDIALOG_BUTTON_OK, NULL);
#endif
}

#if !UE_SERVER
TSharedPtr<class SUWDialog>  UUTLocalPlayer::ShowMessage(FText MessageTitle, FText MessageText, uint16 Buttons, const FDialogResultDelegate& Callback, FVector2D DialogSize)
{
	TSharedPtr<class SUWDialog> NewDialog;
	if (DialogSize.IsNearlyZero())
	{
		SAssignNew(NewDialog, SUWMessageBox)
			.PlayerOwner(this)
			.DialogTitle(MessageTitle)
			.MessageText(MessageText)
			.ButtonMask(Buttons)
			.OnDialogResult(Callback);
	}
	else
	{
		SAssignNew(NewDialog, SUWMessageBox)
			.PlayerOwner(this)
			.bDialogSizeIsRelative(true)
			.DialogSize(DialogSize)
			.DialogTitle(MessageTitle)
			.MessageText(MessageText)
			.ButtonMask(Buttons)
			.OnDialogResult(Callback);
	}

	OpenDialog( NewDialog.ToSharedRef() );
	return NewDialog;
}

TSharedPtr<class SUWDialog> UUTLocalPlayer::ShowSupressableConfirmation(FText MessageTitle, FText MessageText, FVector2D DialogSize, bool &InOutShouldSuppress, const FDialogResultDelegate& Callback)
{
	auto OnGetSuppressibleState = [&InOutShouldSuppress]() { return InOutShouldSuppress ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; };

	auto OnSetSuppressibleState = [&InOutShouldSuppress](ECheckBoxState CheckBoxState) 
	{ 
		InOutShouldSuppress = CheckBoxState == ECheckBoxState::Checked;
	};

	TSharedPtr<class SUWDialog> NewDialog;
	if (DialogSize.IsNearlyZero())
	{
		SAssignNew(NewDialog, SUWMessageBox)
			.PlayerOwner(this)
			.DialogTitle(MessageTitle)
			.MessageText(MessageText)
			.ButtonMask(UTDIALOG_BUTTON_OK)
			.OnDialogResult(Callback)
			.IsSuppressible(true)
			.SuppressibleCheckBoxState_Lambda( OnGetSuppressibleState )
			.OnSuppressibleCheckStateChanged_Lambda( OnSetSuppressibleState );
	}
	else
	{
		SAssignNew(NewDialog, SUWMessageBox)
			.PlayerOwner(this)
			.bDialogSizeIsRelative(true)
			.DialogSize(DialogSize)
			.DialogTitle(MessageTitle)
			.MessageText(MessageText)
			.ButtonMask(UTDIALOG_BUTTON_OK)
			.IsSuppressible(true)
			.SuppressibleCheckBoxState_Lambda(OnGetSuppressibleState)
			.OnSuppressibleCheckStateChanged_Lambda(OnSetSuppressibleState)
			.OnDialogResult(Callback);
	}

	OpenDialog(NewDialog.ToSharedRef());
	return NewDialog;
}

void UUTLocalPlayer::OpenDialog(TSharedRef<SUWDialog> Dialog, int32 ZOrder)
{
	GEngine->GameViewport->AddViewportWidgetContent(Dialog, ZOrder);
	Dialog->OnDialogOpened();
	OpenDialogs.Add(Dialog);
}

void UUTLocalPlayer::CloseDialog(TSharedRef<SUWDialog> Dialog)
{
	OpenDialogs.Remove(Dialog);
	Dialog->OnDialogClosed();
	GEngine->GameViewport->RemoveViewportWidgetContent(Dialog);
}

TSharedPtr<class SUWServerBrowser> UUTLocalPlayer::GetServerBrowser()
{
	if (!ServerBrowserWidget.IsValid())
	{
		SAssignNew(ServerBrowserWidget, SUWServerBrowser, this);
	}

	return ServerBrowserWidget;
}

TSharedPtr<class SUWReplayBrowser> UUTLocalPlayer::GetReplayBrowser()
{
	if (!ReplayBrowserWidget.IsValid())
	{
		SAssignNew(ReplayBrowserWidget, SUWReplayBrowser, this);
	}

	return ReplayBrowserWidget;
}

TSharedPtr<class SUWStatsViewer> UUTLocalPlayer::GetStatsViewer()
{
	if (!StatsViewerWidget.IsValid())
	{
		SAssignNew(StatsViewerWidget, SUWStatsViewer, this);
	}

	return StatsViewerWidget;
}

TSharedPtr<class SUWCreditsPanel> UUTLocalPlayer::GetCreditsPanel()
{
	if (!CreditsPanelWidget.IsValid())
	{
		SAssignNew(CreditsPanelWidget, SUWCreditsPanel, this);
	}

	return CreditsPanelWidget;
}

bool UUTLocalPlayer::AreMenusOpen()
{
	return DesktopSlateWidget.IsValid()
		|| MatchSummaryWindow.IsValid()
		|| LoadoutMenu.IsValid()
		|| OpenDialogs.Num() > 0;
	//Add any widget thats not in the menu here
	//TODO: Should look through each active widget and determine the needed input mode EIM_UIOnly > EIM_GameAndUI > EIM_GameOnly
}

#endif

void UUTLocalPlayer::ShowHUDSettings()
{
#if !UE_SERVER
	if (!HUDSettings.IsValid())
	{
		SAssignNew(HUDSettings, SUWHUDSettingsDialog)
			.PlayerOwner(this);

		OpenDialog( HUDSettings.ToSharedRef() );

		if (PlayerController)
		{
			if (!IsMenuGame())
			{
				PlayerController->SetPause(true);
			}
		}
	}
#endif
}

void UUTLocalPlayer::HideHUDSettings()
{
#if !UE_SERVER

	if (HUDSettings.IsValid())
	{
		CloseDialog(HUDSettings.ToSharedRef());
		HUDSettings.Reset();

		if (!IsMenuGame())
		{
			if (PlayerController)
			{
				PlayerController->SetPause(false);
			}
		}
	}
#endif
}

bool UUTLocalPlayer::IsLoggedIn() const
{ 
	return OnlineIdentityInterface.IsValid() && OnlineIdentityInterface->GetLoginStatus(GetControllerId());
}


void UUTLocalPlayer::LoginOnline(FString EpicID, FString Auth, bool bIsRememberToken, bool bSilentlyFail)
{
	if ( !OnlineIdentityInterface.IsValid() ) return;

	FString Override;
	if ( FParse::Value(FCommandLine::Get(),TEXT("-id="),Override))
	{
		EpicID = Override;
	}

	if ( FParse::Value(FCommandLine::Get(),TEXT("-pass="),Override))
	{
		Auth=Override;
		bIsRememberToken=false;
	}

	if (EpicID == TEXT(""))
	{
		EpicID = LastEpicIDLogin;
	}

	// Save this for later.
	PendingLoginUserName = EpicID;
	bSilentLoginFail = bSilentlyFail;

	if (EpicID == TEXT("") || Auth == TEXT(""))
	{
		GetAuth();
		return;
	}

	FOnlineAccountCredentials AccountCreds(TEXT("epic"), EpicID, Auth);
	if (bIsRememberToken)
	{
		AccountCreds.Type = TEXT("refresh");
	}

	// Begin the Login Process...
	if (!OnlineIdentityInterface->Login(GetControllerId(), AccountCreds))
	{
#if !UE_SERVER
		// We should never fail here unless something has gone horribly wrong
		if (bSilentLoginFail)
		{
			UE_LOG(UT, Warning, TEXT("Could not connect to the online subsystem. Please check your connection and try again."));
		}
		else
		{
			ShowMessage(NSLOCTEXT("MCPMessages", "OnlineError", "Online Error"), NSLOCTEXT("MCPMessages", "UnknownLoginFailuire", "Could not connect to the online subsystem.  Please check your connection and try again."), UTDIALOG_BUTTON_OK, NULL);
		}
		return;
#endif
	}
}

void UUTLocalPlayer::Logout()
{
	if (IsLoggedIn() && OnlineIdentityInterface.IsValid())
	{
		// Begin the Login Process....
		if (!OnlineIdentityInterface->Logout(GetControllerId()))
		{
#if !UE_SERVER
			// We should never fail here unless something has gone horribly wrong
			ShowMessage(NSLOCTEXT("MCPMessages","OnlineError","Online Error"), NSLOCTEXT("MCPMessages","UnknownLogoutFailuire","Could not log out from the online subsystem.  Please check your connection and try again."), UTDIALOG_BUTTON_OK, NULL);
			return;
#endif
		}
	}
}



FString UUTLocalPlayer::GetOnlinePlayerNickname()
{
	return IsLoggedIn() ? OnlineIdentityInterface->GetPlayerNickname(0) : TEXT("None");
}

void UUTLocalPlayer::OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UniqueID, const FString& ErrorMessage)
{
	if (bWasSuccessful)
	{
		// Save the creds for the next auto-login

		TSharedPtr<FUserOnlineAccount> Account = OnlineIdentityInterface->GetUserAccount(UniqueID);
		if (Account.IsValid())
		{
			FString RememberMeToken;
			FString Token;
			Account->GetAuthAttribute(TEXT("refresh_token"), RememberMeToken);

			if ( Account->GetAuthAttribute(TEXT("ut:developer"), Token) )			CommunityRole = EUnrealRoles::Developer;
			else if ( Account->GetAuthAttribute(TEXT("ut:contributor"), Token) )	CommunityRole = EUnrealRoles::Contributor;
			else if ( Account->GetAuthAttribute(TEXT("ut:concepter"), Token) )		CommunityRole = EUnrealRoles::Concepter;
			else if ( Account->GetAuthAttribute(TEXT("ut:prototyper"), Token) )		CommunityRole = EUnrealRoles::Prototyper;
			else if ( Account->GetAuthAttribute(TEXT("ut:marketplace"), Token) )	CommunityRole = EUnrealRoles::Marketplace;
			else if ( Account->GetAuthAttribute(TEXT("ut:ambassador"), Token) )		CommunityRole = EUnrealRoles::Ambassador;
			else 
			{
				CommunityRole = EUnrealRoles::Gamer;
			}
			
			LastEpicIDLogin = PendingLoginUserName;
			LastEpicRememberMeToken = RememberMeToken;
			SaveConfig();
		}

		PendingLoginUserName = TEXT("");

		LoadProfileSettings();
		FText WelcomeToast = FText::Format(NSLOCTEXT("MCP","MCPWelcomeBack","Welcome back {0}"), FText::FromString(*GetOnlinePlayerNickname()));
		ShowToast(WelcomeToast);

		// Init the Friends And Chat system
		IFriendsAndChatModule::Get().GetFriendsAndChatManager()->Login();
		IFriendsAndChatModule::Get().GetFriendsAndChatManager()->SetAnalyticsProvider(FUTAnalytics::GetProviderPtr());

		if (!IFriendsAndChatModule::Get().GetFriendsAndChatManager()->OnFriendsJoinGame().IsBoundToObject(this))
		{
			IFriendsAndChatModule::Get().GetFriendsAndChatManager()->OnFriendsJoinGame().AddUObject(this, &UUTLocalPlayer::HandleFriendsJoinGame);
		}
		if (!IFriendsAndChatModule::Get().GetFriendsAndChatManager()->AllowFriendsJoinGame().IsBoundToObject(this))
		{
			IFriendsAndChatModule::Get().GetFriendsAndChatManager()->AllowFriendsJoinGame().BindUObject(this, &UUTLocalPlayer::AllowFriendsJoinGame);
		}
		if (!IFriendsAndChatModule::Get().GetFriendsAndChatManager()->OnFriendsNotification().IsBoundToObject(this))
		{
			IFriendsAndChatModule::Get().GetFriendsAndChatManager()->OnFriendsNotification().AddUObject(this, &UUTLocalPlayer::HandleFriendsNotificationAvail);
		}
		if (!IFriendsAndChatModule::Get().GetFriendsAndChatManager()->OnFriendsActionNotification().IsBoundToObject(this))
		{
			IFriendsAndChatModule::Get().GetFriendsAndChatManager()->OnFriendsActionNotification().AddUObject(this, &UUTLocalPlayer::HandleFriendsActionNotification);
		}

		// on successful auto login, attempt to join an accepted friend game invite
		if (bInitialSignInAttempt)
		{
			FString SessionId;
			FString FriendId;
			if (FParse::Value(FCommandLine::Get(), TEXT("invitesession="), SessionId) && !SessionId.IsEmpty() &&
				FParse::Value(FCommandLine::Get(), TEXT("invitefrom="), FriendId) && !FriendId.IsEmpty())
			{
				JoinFriendSession(FUniqueNetIdString(FriendId), FUniqueNetIdString(SessionId));
			}
		}

		AUTPlayerController* UTPC = Cast<AUTPlayerController>(PlayerController);
		if (UTPC)
		{
			if (OnlineIdentityInterface.IsValid() && OnlineIdentityInterface->GetLoginStatus(GetControllerId()))
			{
				TSharedPtr<FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
				if (UserId.IsValid())
				{
					UTPC->ServerReceiveStatsID(UserId->ToString());
					UTPC->PlayerState->SetUniqueId(UserId);
				}
			}
		}
	}

	// We have enough credentials to auto-login.  So try it, but silently fail if we cant.
	else if (bInitialSignInAttempt)
	{
		if (LastEpicIDLogin != TEXT("") && LastEpicRememberMeToken != TEXT(""))
		{
			bInitialSignInAttempt = false;
			LoginOnline(LastEpicIDLogin, LastEpicRememberMeToken, true, true);
		}
	}

	// Otherwise if this is the first attempt, then silently fair
	else if (!bSilentLoginFail)
	{
		// Broadcast the failure to the UI.
		PlayerOnlineStatusChanged.Broadcast(this, ELoginStatus::NotLoggedIn, UniqueID);
		GetAuth(ErrorMessage);
	}
}

void UUTLocalPlayer::GetAuth(FString ErrorMessage)
{
#if !UE_SERVER
	if (GetWorld()->IsPlayInEditor())
	{
		return;
	}

	if (LoginDialog.IsValid())
	{
		return;
	}

	bool bError = ErrorMessage != TEXT("");

	SAssignNew(LoginDialog, SUWLoginDialog)
		.OnDialogResult(FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::AuthDialogClosed))
		.UserIDText(PendingLoginUserName)
		.ErrorText(bError ? FText::FromString(ErrorMessage) : FText::GetEmpty())
		.PlayerOwner(this);

	GEngine->GameViewport->AddViewportWidgetContent(LoginDialog.ToSharedRef(), 160);
	LoginDialog->SetInitialFocus();

#endif
}

void UUTLocalPlayer::OnLoginStatusChanged(int32 LocalUserNum, ELoginStatus::Type PreviousLoginStatus, ELoginStatus::Type LoginStatus, const FUniqueNetId& UniqueID)
{
	UE_LOG(UT,Verbose,TEXT("***[LoginStatusChanged]*** - User %i - %i"), LocalUserNum, int32(LoginStatus));

	// If we have logged out, or started using the local profile, then clear the online profile.
	if (LoginStatus == ELoginStatus::NotLoggedIn || LoginStatus == ELoginStatus::UsingLocalProfile)
	{
		CurrentProfileSettings = NULL;
		FUTAnalytics::LoginStatusChanged(FString());

		if (bPendingLoginCreds)
		{
			bPendingLoginCreds = false;
			LoginOnline(PendingLoginName, PendingLoginPassword);
			PendingLoginPassword = TEXT("");
		}


		// If we are connected to a server, then exit back to the main menu.
		if (GetWorld()->GetNetMode() == NM_Client)
		{
			ReturnToMainMenu();		
		}

	}
	else if (LoginStatus == ELoginStatus::LoggedIn)
	{
		ReadELOFromCloud();
		UpdatePresence(LastPresenceUpdate, bLastAllowInvites,bLastAllowInvites,bLastAllowInvites,false);
		ReadCloudFileListing();
		// query entitlements for UI
		IOnlineEntitlementsPtr EntitlementsInterface = OnlineSubsystem->GetEntitlementsInterface();
		if (EntitlementsInterface.IsValid())
		{
			EntitlementsInterface->QueryEntitlements(UniqueID, TEXT("ut"));
		}
		FUTAnalytics::LoginStatusChanged(UniqueID.ToString());

		// If we have a pending session, then join it.
		JoinPendingSession();
	}


	PlayerOnlineStatusChanged.Broadcast(this, LoginStatus, UniqueID);
}

void UUTLocalPlayer::ReadCloudFileListing()
{
	if (OnlineUserCloudInterface.IsValid() && OnlineIdentityInterface.IsValid())
	{
		TSharedPtr<FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
		if (UserId.IsValid())
		{
			OnlineUserCloudInterface->EnumerateUserFiles(*UserId.Get());
		}
	}
}

void UUTLocalPlayer::OnEnumerateUserFilesComplete(bool bWasSuccessful, const FUniqueNetId& InUserId)
{
	UE_LOG(UT, Verbose, TEXT("OnEnumerateUserFilesComplete %d"), bWasSuccessful ? 1 : 0);
	UUTGameEngine* UTEngine = Cast<UUTGameEngine>(GEngine);
	if (UTEngine)
	{
		UTEngine->CloudContentChecksums.Empty();

		if (OnlineUserCloudInterface.IsValid() && OnlineIdentityInterface.IsValid())
		{
			TArray<FCloudFileHeader> UserFiles;
			OnlineUserCloudInterface->GetUserFileList(InUserId, UserFiles);
			for (int32 i = 0; i < UserFiles.Num(); i++)
			{
				TArray<uint8> DecodedHash;
				FBase64::Decode(UserFiles[i].Hash, DecodedHash);
				FString Hash = BytesToHex(DecodedHash.GetData(), DecodedHash.Num());
				UE_LOG(UT, Verbose, TEXT("%s %s"), *UserFiles[i].FileName, *Hash);
				UTEngine->CloudContentChecksums.Add(FPaths::GetBaseFilename(UserFiles[i].FileName), Hash);
			}
		}		
	}
}

void UUTLocalPlayer::OnLogoutComplete(int32 LocalUserNum, bool bWasSuccessful)
{
	UE_LOG(UT,Verbose,TEXT("***[Logout Complete]*** - User %i"), LocalUserNum);
	// TO-DO: Add a Toast system for displaying stuff like this

	GetWorld()->GetTimerManager().ClearTimer(ProfileWriteTimerHandle);

	// If we have pending login creds then try to log right back in.
	if (bPendingLoginCreds)
	{
		bPendingLoginCreds = false;
		LoginOnline(PendingLoginName, PendingLoginPassword);
		PendingLoginPassword = TEXT("");
	}
	else
	{
		ShowToast(NSLOCTEXT("UTLocalPlayer","LoggedOutMsg","You have logged out!"));
	}

}

#if !UE_SERVER

void UUTLocalPlayer::CloseAuth()
{
	GEngine->GameViewport->RemoveViewportWidgetContent(LoginDialog.ToSharedRef());
	LoginDialog.Reset();
}

void UUTLocalPlayer::AuthDialogClosed(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	
	if (ButtonID != UTDIALOG_BUTTON_CANCEL)
	{
		if (LoginDialog.IsValid())
		{
			// Look to see if we are already logged in.
			if ( IsLoggedIn() )
			{
				bPendingLoginCreds = true;
				PendingLoginName = LoginDialog->GetEpicID();
				PendingLoginPassword = LoginDialog->GetPassword();

				CloseAuth();


				// If we are in an active session, warn that this will cause you to go back to the main menu.
				TSharedPtr<FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(0);
				if (UserId.IsValid() && OnlineSessionInterface->IsPlayerInSession(GameSessionName, *UserId))
				{
					ShowMessage(NSLOCTEXT("UTLocalPlayer", "SwitchLoginsTitle", "Change Users..."), NSLOCTEXT("UTLocalPlayer", "SwitchLoginsMsg", "Switching users will cause you to return to the main menu and leave any game you are currently in.  Are you sure you wish to do this?"), UTDIALOG_BUTTON_YES + UTDIALOG_BUTTON_NO, FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::OnSwitchUserResult),FVector2D(0.25,0.25));					
				}
				else
				{
					Logout();
				}
				return;
			}

			else
			{
				FString UserName = LoginDialog->GetEpicID();
				FString Password = LoginDialog->GetPassword();
				CloseAuth();
				LoginOnline(UserName, Password,false);
			}
		}
	}
	else
	{
		if (LoginDialog.IsValid())
		{
			CloseAuth();
		}
		PendingLoginUserName = TEXT("");
	}
}

void UUTLocalPlayer::OnSwitchUserResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_YES)
	{
		// If we are in an active session, then we have to force a return to the main menu.  If we are not in an active session (ie: setting at the main menu)
		// we can just logout/login..
		TSharedPtr<FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(0);
		if (UserId.IsValid() && OnlineSessionInterface->IsPlayerInSession(GameSessionName, *UserId))
		{
			// kill the current menu....
			HideMenu();
			ReturnToMainMenu();	
		}
		else
		{
			Logout();
		}
	}
	else
	{
		bPendingLoginCreds = false;
		PendingLoginPassword = TEXT("");
	}
}

#endif

FDelegateHandle UUTLocalPlayer::RegisterPlayerOnlineStatusChangedDelegate(const FPlayerOnlineStatusChanged::FDelegate& NewDelegate)
{
	return PlayerOnlineStatusChanged.Add(NewDelegate);
}

void UUTLocalPlayer::RemovePlayerOnlineStatusChangedDelegate(FDelegateHandle DelegateHandle)
{
	PlayerOnlineStatusChanged.Remove(DelegateHandle);
}

FDelegateHandle UUTLocalPlayer::RegisterChatArchiveChangedDelegate(const FChatArchiveChanged::FDelegate& NewDelegate)
{
	return ChatArchiveChanged.Add(NewDelegate);
}

void UUTLocalPlayer::RemoveChatArchiveChangedDelegate(FDelegateHandle DelegateHandle)
{
	ChatArchiveChanged.Remove(DelegateHandle);
}


void UUTLocalPlayer::ShowToast(FText ToastText)
{
#if !UE_SERVER

	if (GetWorld()->GetNetMode() == ENetMode::NM_Client && bSuppressToastsInGame) return;

	// Build the Toast to Show...

	TSharedPtr<SUWToast> Toast;
	SAssignNew(Toast, SUWToast)
		.PlayerOwner(this)
		.ToastText(ToastText);

	if (Toast.IsValid())
	{
		ToastList.Add(Toast);

		// Auto show if it's the first toast..
		if (ToastList.Num() == 1)
		{
			AddToastToViewport(ToastList[0]);
		}
	}
#endif
}

#if !UE_SERVER
void UUTLocalPlayer::AddToastToViewport(TSharedPtr<SUWToast> ToastToDisplay)
{
	GEngine->GameViewport->AddViewportWidgetContent( SNew(SWeakWidget).PossiblyNullContent(ToastToDisplay.ToSharedRef()),10000);
}

void UUTLocalPlayer::ToastCompleted()
{
	GEngine->GameViewport->RemoveViewportWidgetContent(ToastList[0].ToSharedRef());
	ToastList.RemoveAt(0,1);

	if (ToastList.Num() > 0)
	{
		AddToastToViewport(ToastList[0]);
	}
}

#endif

FString UUTLocalPlayer::GetProfileFilename()
{
	if (IsLoggedIn())
	{
		return TEXT("user_profile_1");
	}

	return TEXT("local_user_profile");
}

/*
 *	If the player is currently logged in, trigger a load of their profile settings from the MCP.  
 */
void UUTLocalPlayer::LoadProfileSettings()
{
	if (GetWorld()->IsPlayInEditor())
	{
		return;
	}

	if (IsLoggedIn())
	{
		TSharedPtr<FUniqueNetId> UserID = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
		if (UserID.IsValid())
		{
			if (OnlineUserCloudInterface.IsValid())
			{
				OnlineUserCloudInterface->ReadUserFile(*UserID, GetProfileFilename());
			}
		
			ReadProfileItems();
		}
	}
}

void UUTLocalPlayer::ReadProfileItems()
{
#if !UE_BUILD_SHIPPING
	// this code ends up invoking the asset registry (slow in dev builds) so command line option to skip
	if (FParse::Param(FCommandLine::Get(), TEXT("noitems")))
	{
		return;
	}
#endif
	TSharedPtr<FUniqueNetId> UserID = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
	if (UserID.IsValid() && (LastItemReadTime == 0.0 || FPlatformTime::Seconds() > LastItemReadTime + 60.0))
	{
		FHttpRequestCompleteDelegate Delegate;
		Delegate.BindUObject(this, &UUTLocalPlayer::OnReadProfileItemsComplete);
		ReadBackendStats(Delegate, UserID->ToString());
		LastItemReadTime = FPlatformTime::Seconds();
	}
}

void UUTLocalPlayer::ClearProfileSettings()
{
#if !UE_SERVER
	if (IsLoggedIn())
	{
		ShowMessage(NSLOCTEXT("UUTLocalPlayer","ClearCloudWarnTitle","!!! WARNING !!!"), NSLOCTEXT("UUTLocalPlayer","ClearCloudWarnMessage","You are about to clear all of your settings in the cloud as well as clear your active game and input ini files locally.  The game will then exit and wait for a restart!\n\nAre you sure you want to do this??"), UTDIALOG_BUTTON_YES + UTDIALOG_BUTTON_NO, FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::ClearProfileWarnResults));
	}
#endif
}

void UUTLocalPlayer::ClearProfileWarnResults(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (IsLoggedIn() && ButtonID == UTDIALOG_BUTTON_YES)
	{
		TSharedPtr<FUniqueNetId> UserID = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
		if (OnlineUserCloudInterface.IsValid() && UserID.IsValid())
		{
			OnlineUserCloudInterface->DeleteUserFile(*UserID, GetProfileFilename(), true, true);
		}
	}
}

void UUTLocalPlayer::OnDeleteUserFileComplete(bool bWasSuccessful, const FUniqueNetId& InUserId, const FString& FileName)
{
#if !UE_SERVER
	// We successfully cleared the cloud, rewrite everything
	if (bWasSuccessful && FileName == GetProfileFilename())
	{
		FString PlaformName = FPlatformProperties::PlatformName();
		FString Filename = FString::Printf(TEXT("%s%s/Input.ini"), *FPaths::GeneratedConfigDir(), *PlaformName);
		if (FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*Filename))
		{
			UE_LOG(UT,Log,TEXT("Failed to delete Input.ini"));
		}

		Filename = FString::Printf(TEXT("%s%s/Game.ini"), *FPaths::GeneratedConfigDir(), *PlaformName);
		if (FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*Filename))
		{
			UE_LOG(UT,Log,TEXT("Failed to delete Game.ini"));
		}

		Filename = FString::Printf(TEXT("%s%s/GameUserSettings.ini"), *FPaths::GeneratedConfigDir(), *PlaformName);
		if (FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*Filename))
		{
			UE_LOG(UT,Log,TEXT("Failed to delete GameUserSettings.ini"));
		}


		FPlatformMisc::RequestExit( 0 );
	}
#endif
}


void UUTLocalPlayer::OnReadUserFileComplete(bool bWasSuccessful, const FUniqueNetId& InUserId, const FString& FileName)
{
	if (FileName == GetProfileFilename())
	{
		// We were attempting to read the profile.. see if it was successful.	

		if (bWasSuccessful && OnlineUserCloudInterface.IsValid())	
		{
			// Create the current profile.
			if (CurrentProfileSettings == NULL)
			{
				CurrentProfileSettings = NewObject<UUTProfileSettings>(GetTransientPackage(),UUTProfileSettings::StaticClass());
			}

			TArray<uint8> FileContents;
			OnlineUserCloudInterface->GetFileContents(InUserId, FileName, FileContents);
			
			// Serialize the object
			FMemoryReader MemoryReader(FileContents, true);
			FObjectAndNameAsStringProxyArchive Ar(MemoryReader, false);
			CurrentProfileSettings->Serialize(Ar);
			CurrentProfileSettings->VersionFixup();

			FString CmdLineSwitch = TEXT("");
			bool bClearProfile = FParse::Param(FCommandLine::Get(), TEXT("ClearProfile"));

			// Set the ranks/etc so the player card is right.
			AUTBasePlayerController* UTBasePlayer = Cast<AUTBasePlayerController>(PlayerController);
			if (UTBasePlayer) UTBasePlayer->ServerReceiveRank(GetBaseELORank(), GetRankDuel(), GetRankCTF(), GetRankTDM(), GetRankDM(), GetTotalChallengeStars());

			// Check to make sure the profile settings are valid and that we aren't forcing them
			// to be cleared.  If all is OK, then apply these settings.
			if (CurrentProfileSettings->SettingsRevisionNum >= VALID_PROFILESETTINGS_VERSION && !bClearProfile)
			{
				CurrentProfileSettings->ApplyAllSettings(this);
				return;
			}
			else
			{
				CurrentProfileSettings->ClearWeaponPriorities();
			}

		}
		else if (CurrentProfileSettings == NULL) // Create a new profile settings object
		{
			CurrentProfileSettings = NewObject<UUTProfileSettings>(GetTransientPackage(),UUTProfileSettings::StaticClass());

			// Set some profile defaults, should be a function call if this gets any larger
			CurrentProfileSettings->TauntPath = GetDefaultURLOption(TEXT("Taunt"));
			CurrentProfileSettings->Taunt2Path = GetDefaultURLOption(TEXT("Taunt2"));
		}

		PlayerNickname = GetAccountDisplayName().ToString();
		SaveConfig();
		SaveProfileSettings();

#if !UE_SERVER
		FText WelcomeMessage = FText::Format(NSLOCTEXT("UTLocalPlayer","Welcome","Your player name is set to '{0}'. Would you like to change it?"), GetAccountDisplayName());
		ShowMessage(NSLOCTEXT("UTLocalPlayer", "WelcomeTitle", "Welcome to Unreal Tournament"), WelcomeMessage, UTDIALOG_BUTTON_YES + UTDIALOG_BUTTON_NO, FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::WelcomeDialogResult),FVector2D(0.35,0.25));
		// We couldn't load our profile or it was invalid or we choose to clear it so save it out.
#endif

	}
	else if (FileName == GetStatsFilename())
	{
		if (bWasSuccessful)
		{
			UpdateBaseELOFromCloudData();
		}
	}
}

void UUTLocalPlayer::OnReadProfileItemsComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	if (bSucceeded)
	{
		ParseProfileItemJson(HttpResponse->GetContentAsString(), ProfileItems, OnlineXP);
	}
}

bool UUTLocalPlayer::OwnsItemFor(const FString& Path, int32 VariantId) const
{
	for (const FProfileItemEntry& Entry : ProfileItems)
	{
		if (Entry.Item != NULL && Entry.Item->Grants(Path))
		{
			return true;
		}
	}
	return false;
}

#if !UE_SERVER
void UUTLocalPlayer::WelcomeDialogResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_YES)
	{
		OpenDialog(SNew(SUWPlayerSettingsDialog).PlayerOwner(this));			
	}
}

#endif

void UUTLocalPlayer::SaveProfileSettings()
{
	if ( CurrentProfileSettings != NULL && IsLoggedIn() )
	{
		CurrentProfileSettings->GatherAllSettings(this);
		CurrentProfileSettings->SettingsRevisionNum = CURRENT_PROFILESETTINGS_VERSION;

		CurrentProfileSettings->bNeedProfileWriteOnLevelChange = false;

		// Build a blob of the profile contents
		TArray<uint8> FileContents;
		FMemoryWriter MemoryWriter(FileContents, true);
		FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, false);
		CurrentProfileSettings->Serialize(Ar);

		if (FApp::GetCurrentTime() - LastProfileCloudWriteTime < ProfileCloudWriteCooldownTime)
		{
			GetWorld()->GetTimerManager().SetTimer(ProfileWriteTimerHandle, this, &UUTLocalPlayer::SaveProfileSettings, ProfileCloudWriteCooldownTime - (FApp::GetCurrentTime() - LastProfileCloudWriteTime), false);
		}
		else
		{
			// Save the blob to the cloud
			TSharedPtr<FUniqueNetId> UserID = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
			if (OnlineUserCloudInterface.IsValid() && UserID.IsValid())
			{
				OnlineUserCloudInterface->WriteUserFile(*UserID, GetProfileFilename(), FileContents);
			}
		}
	}
}

void UUTLocalPlayer::OnWriteUserFileComplete(bool bWasSuccessful, const FUniqueNetId& InUserId, const FString& FileName)
{
	// Make sure this was our filename
	if (FileName == GetProfileFilename())
	{
		LastProfileCloudWriteTime = FApp::GetCurrentTime();

		if (bWasSuccessful)
		{
			FText Saved = NSLOCTEXT("MCP", "ProfileSaved", "Profile Saved");
			ShowToast(Saved);
		}
		else
		{
	#if !UE_SERVER
			// Should give a warning here if it fails.
			ShowMessage(NSLOCTEXT("MCPMessages", "ProfileSaveErrorTitle", "An error has occured"), NSLOCTEXT("MCPMessages", "ProfileSaveErrorText", "UT could not save your profile with the MCP.  Your settings may be lost."), UTDIALOG_BUTTON_OK, NULL);
	#endif
		}
	}
}

void UUTLocalPlayer::SetNickname(FString NewName)
{
	PlayerNickname = NewName;
	SaveConfig();

	
	if (PlayerController) 
	{
		PlayerController->ServerChangeName(NewName);
	}
}

void UUTLocalPlayer::SaveChat(FName Type, FString Sender, FString Message, FLinearColor Color, bool bMyChat)
{
	TSharedPtr<FStoredChatMessage> ArchiveMessage = FStoredChatMessage::Make(Type, Sender, Message, Color, int32(GetWorld()->GetRealTimeSeconds()), bMyChat );
	ChatArchive.Add( ArchiveMessage );
	ChatArchiveChanged.Broadcast(this, ArchiveMessage );
}

FName UUTLocalPlayer::TeamStyleRef(FName InName)
{
	if (PlayerController)
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerController);
		if (PC && PC->GetTeamNum() == 0)
		{
			return FName( *(TEXT("Red.") + InName.ToString()));
		}
	}

	return FName( *(TEXT("Blue.") + InName.ToString()));
}

void UUTLocalPlayer::ReadELOFromCloud()
{
	TSharedPtr<FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
	if (OnlineUserCloudInterface.IsValid() && UserId.IsValid())
	{
		OnlineUserCloudInterface->ReadUserFile(*UserId, GetStatsFilename());
	}
}

void UUTLocalPlayer::UpdateBaseELOFromCloudData()
{
	TArray<uint8> FileContents;
	TSharedPtr<FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
	if (UserId.IsValid() && OnlineUserCloudInterface.IsValid() && OnlineUserCloudInterface->GetFileContents(*UserId, GetStatsFilename(), FileContents))
	{
		if (FileContents.Num() <= 0)
		{
			UE_LOG(LogGameStats, Warning, TEXT("Stats json content is empty"));
			return;
		}

		if (FileContents.GetData()[FileContents.Num() - 1] != 0)
		{
			UE_LOG(LogGameStats, Warning, TEXT("Failed to get proper stats json"));
			return;
		}

		FString JsonString = ANSI_TO_TCHAR((char*)FileContents.GetData());

		UE_LOG(LogGameStats,VeryVerbose,TEXT("Stats JSON: %s"),*JsonString);

		TSharedPtr<FJsonObject> StatsJson;
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);
		if (FJsonSerializer::Deserialize(JsonReader, StatsJson) && StatsJson.IsValid())
		{
			FString JsonStatsID;
			if (StatsJson->TryGetStringField(TEXT("StatsID"), JsonStatsID) && JsonStatsID == UserId->ToString())
			{
				StatsJson->TryGetNumberField(NAME_SkillRating.ToString(), DUEL_ELO);
				StatsJson->TryGetNumberField(NAME_TDMSkillRating.ToString(), TDM_ELO);
				StatsJson->TryGetNumberField(NAME_DMSkillRating.ToString(), FFA_ELO);
				StatsJson->TryGetNumberField(NAME_CTFSkillRating.ToString(), CTF_ELO);
				StatsJson->TryGetNumberField(NAME_MatchesPlayed.ToString(), MatchesPlayed);
				StatsJson->TryGetNumberField(NAME_SkillRatingSamples.ToString(), DuelMatchesPlayed);
				StatsJson->TryGetNumberField(NAME_TDMSkillRatingSamples.ToString(), TDMMatchesPlayed);
				StatsJson->TryGetNumberField(NAME_DMSkillRatingSamples.ToString(), FFAMatchesPlayed);
				StatsJson->TryGetNumberField(NAME_CTFSkillRatingSamples.ToString(), CTFMatchesPlayed);
			}
		}
	}

	// Sanitize the elo rankings
	const int32 StartingELO = 1500;
	if (DUEL_ELO <= 0)
	{
		DUEL_ELO = StartingELO;
	}
	if (TDM_ELO <= 0)
	{
		TDM_ELO = StartingELO;
	}
	if (FFA_ELO <= 0)
	{
		FFA_ELO = StartingELO;
	}
	if (CTF_ELO <= 0)
	{
		CTF_ELO = StartingELO;
	}

	// 3000 should be fairly difficult to achieve
	// Have some possible bugged profiles with overlarge ELOs
	const int32 MaximumELO = 3000;
	if (DUEL_ELO > MaximumELO)
	{
		DUEL_ELO = MaximumELO;
	}
	if (TDM_ELO > MaximumELO)
	{
		TDM_ELO = MaximumELO;
	}
	if (FFA_ELO > MaximumELO)
	{
		FFA_ELO = MaximumELO;
	}
	if (CTF_ELO > MaximumELO)
	{
		CTF_ELO = MaximumELO;
	}

	if (MatchesPlayed <= 0)		MatchesPlayed = 0;
	if (DuelMatchesPlayed <= 0) DuelMatchesPlayed = 0;
	if (TDMMatchesPlayed <= 0)	TDMMatchesPlayed = 0;
	if (FFAMatchesPlayed <= 0)	FFAMatchesPlayed = 0;
	if (CTFMatchesPlayed <= 0)	CTFMatchesPlayed = 0;

}

int32 UUTLocalPlayer::GetBaseELORank()
{
	float TotalRating = 0.f;
	float RatingCount = 0.f;
	float CurrentRating = 1.f;
	int32 BestRating = 0;
	int32 MatchCount = DuelMatchesPlayed + TDMMatchesPlayed + FFAMatchesPlayed + CTFMatchesPlayed;
	const int32 MatchThreshold = 40;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	int32 ForcedELORank;
	if (FParse::Value(FCommandLine::Get(), TEXT("ForceELORank="), ForcedELORank))
	{
		return ForcedELORank;
	}
#endif

	UE_LOG(LogGameStats, Verbose, TEXT("GetBaseELORank Duels:%d, TDM:%d, FFA %d, CTF %d"), DuelMatchesPlayed, TDMMatchesPlayed, FFAMatchesPlayed, CTFMatchesPlayed);

	if (DUEL_ELO > 0)
	{
		TotalRating += DUEL_ELO;
		RatingCount += 1.f;
		CurrentRating = TotalRating / RatingCount;
		if (DuelMatchesPlayed > MatchThreshold)
		{
			BestRating = FMath::Max(BestRating, DUEL_ELO);
			UE_LOG(LogGameStats, Verbose, TEXT("GetBaseELORank Duel ELO %d is candidate for best"), DUEL_ELO);
		}
	}

	if (TDM_ELO > 0)
	{
		TotalRating += TDM_ELO;
		RatingCount += 1.f;
		CurrentRating = TotalRating / RatingCount;
		if (TDMMatchesPlayed > MatchThreshold)
		{
			BestRating = FMath::Max(BestRating, TDM_ELO);
			UE_LOG(LogGameStats, Verbose, TEXT("GetBaseELORank TDM ELO %d is candidate for best"), TDM_ELO);
		}
	}

	// FFA Elo is the least accurate, weighted lower @TODO FIXMESTEVE show badge based on current game type
	// max rating of 2400 based on FFA 
	if ((FFA_ELO > CurrentRating) && ((CurrentRating < 2400) || (DuelMatchesPlayed + TDMMatchesPlayed + CTFMatchesPlayed < MatchThreshold)))
	{
		UE_LOG(LogGameStats, Verbose, TEXT("GetBaseELORank applying FFA ELO %d to average rank, max 2400"), FFA_ELO);
		TotalRating += 0.5f * FMath::Min(FFA_ELO, 2400);
		RatingCount += 0.5f;
		CurrentRating = TotalRating / RatingCount;
	}
	else
	{
		UE_LOG(LogGameStats, Verbose, TEXT("GetBaseELORank not factoring in FFA ELO %d to average rank"), FFA_ELO);
	}

	if (CTF_ELO > 0 && ((CTF_ELO > CurrentRating) || (DuelMatchesPlayed + TDMMatchesPlayed < MatchThreshold)))
	{
		TotalRating += CTF_ELO;
		RatingCount += 1.f;
		CurrentRating = TotalRating / RatingCount;
		if (CTFMatchesPlayed > MatchThreshold)
		{
			BestRating = FMath::Max(BestRating, CTF_ELO);
			UE_LOG(LogGameStats, Verbose, TEXT("GetBaseELORank CTF ELO %d is candidate for best"), CTF_ELO);
		}
	}
	else
	{
		UE_LOG(LogGameStats, Verbose, TEXT("GetBaseELORank not factoring in CTF ELO %d to average rank"), CTF_ELO);
	}

	UE_LOG(LogGameStats, Verbose, TEXT("GetBaseELORank Best:%d WeightedAverage:%f MatchThrottled:%f"), BestRating, CurrentRating, 400.f + 50.f * MatchCount);

	// Limit displayed Elo to 400 + 50 * number of matches played
	return FMath::Min(FMath::Max(float(BestRating), CurrentRating), 400.f + 50.f * MatchCount);
}

void UUTLocalPlayer::GetBadgeFromELO(int32 EloRating, int32& BadgeLevel, int32& SubLevel)
{
	// Bronze levels up to 1750, start at 400, go up every 140
	if (EloRating  < 1660)
	{
		BadgeLevel = 0;
		SubLevel = FMath::Clamp((float(EloRating) - 250.f) / 140.f, 0.f, 8.f);
	}
	else if (EloRating < 2200)
	{
		BadgeLevel = 1;
		SubLevel = FMath::Clamp((float(EloRating) - 1660.f) / 60.f, 0.f, 8.f);
	}
	else
	{
		BadgeLevel = 2;
		SubLevel = FMath::Clamp((float(EloRating) - 2200.f) / 50.f, 0.f, 8.f);
	}
}


bool UUTLocalPlayer::IsConsideredABeginnner()
{
	float BaseELO = GetBaseELORank();

	return (BaseELO < 1400) && (DuelMatchesPlayed + TDMMatchesPlayed + FFAMatchesPlayed + CTFMatchesPlayed) < 20;
}


int32 UUTLocalPlayer::GetHatVariant() const
{
	return (CurrentProfileSettings != NULL) ? CurrentProfileSettings->HatVariant : FCString::Atoi(*GetDefaultURLOption(TEXT("HatVar")));
}

void UUTLocalPlayer::SetHatVariant(int32 NewVariant)
{
	if (CurrentProfileSettings != NULL)
	{
		CurrentProfileSettings->HatVariant = NewVariant;
	}
	SetDefaultURLOption(TEXT("HatVar"), FString::FromInt(NewVariant));
	if (PlayerController != NULL)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerController->PlayerState);
		if (PS != NULL)
		{
			PS->ServerReceiveHatVariant(NewVariant);
		}
	}
}

int32 UUTLocalPlayer::GetEyewearVariant() const
{
	return (CurrentProfileSettings != NULL) ? CurrentProfileSettings->EyewearVariant : FCString::Atoi(*GetDefaultURLOption(TEXT("EyewearVar")));

}
void UUTLocalPlayer::SetEyewearVariant(int32 NewVariant)
{
	if (CurrentProfileSettings != NULL)
	{
		CurrentProfileSettings->EyewearVariant = NewVariant;
	}
	SetDefaultURLOption(TEXT("EyewearVar"), FString::FromInt(NewVariant));
	if (PlayerController != NULL)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerController->PlayerState);
		if (PS != NULL)
		{
			PS->ServerReceiveEyewearVariant(NewVariant);
		}
	}
}

FString UUTLocalPlayer::GetHatPath() const
{
	return (CurrentProfileSettings != NULL) ? CurrentProfileSettings->HatPath : GetDefaultURLOption(TEXT("Hat"));
}
void UUTLocalPlayer::SetHatPath(const FString& NewHatPath)
{
	if (CurrentProfileSettings != NULL)
	{
		CurrentProfileSettings->HatPath = NewHatPath;
	}
	SetDefaultURLOption(TEXT("Hat"), NewHatPath);
	if (PlayerController != NULL)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerController->PlayerState);
		if (PS != NULL)
		{
			PS->ServerReceiveHatClass(NewHatPath);
		}
	}
}
FString UUTLocalPlayer::GetEyewearPath() const
{
	return (CurrentProfileSettings != NULL) ? CurrentProfileSettings->EyewearPath : GetDefaultURLOption(TEXT("Eyewear"));
}
void UUTLocalPlayer::SetEyewearPath(const FString& NewEyewearPath)
{
	if (CurrentProfileSettings != NULL)
	{
		CurrentProfileSettings->EyewearPath = NewEyewearPath;
	}
	SetDefaultURLOption(TEXT("Eyewear"), NewEyewearPath);
	if (PlayerController != NULL)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerController->PlayerState);
		if (PS != NULL)
		{
			PS->ServerReceiveEyewearClass(NewEyewearPath);
		}
	}
}
FString UUTLocalPlayer::GetCharacterPath() const
{
	return (CurrentProfileSettings != NULL) ? CurrentProfileSettings->CharacterPath : GetDefaultURLOption(TEXT("Character"));
}
void UUTLocalPlayer::SetCharacterPath(const FString& NewCharacterPath)
{
	if (CurrentProfileSettings != NULL)
	{
		CurrentProfileSettings->CharacterPath = NewCharacterPath;
	}
	SetDefaultURLOption(TEXT("Character"), NewCharacterPath);
	if (PlayerController != NULL)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerController->PlayerState);
		if (PS != NULL)
		{
			PS->ServerSetCharacter(NewCharacterPath);
		}
	}
}
FString UUTLocalPlayer::GetTauntPath() const
{
	return (CurrentProfileSettings != NULL) ? CurrentProfileSettings->TauntPath : GetDefaultURLOption(TEXT("Taunt"));
}
void UUTLocalPlayer::SetTauntPath(const FString& NewTauntPath)
{
	if (CurrentProfileSettings != NULL)
	{
		CurrentProfileSettings->TauntPath = NewTauntPath;
	}
	SetDefaultURLOption(TEXT("Taunt"), NewTauntPath);
	if (PlayerController != NULL)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerController->PlayerState);
		if (PS != NULL)
		{
			PS->ServerReceiveTauntClass(NewTauntPath);
		}
	}
}


FString UUTLocalPlayer::GetTaunt2Path() const
{
	return (CurrentProfileSettings != NULL) ? CurrentProfileSettings->Taunt2Path : GetDefaultURLOption(TEXT("Taunt2"));
}
void UUTLocalPlayer::SetTaunt2Path(const FString& NewTauntPath)
{
	if (CurrentProfileSettings != NULL)
	{
		CurrentProfileSettings->Taunt2Path = NewTauntPath;
	}
	SetDefaultURLOption(TEXT("Taunt2"), NewTauntPath);
	if (PlayerController != NULL)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerController->PlayerState);
		if (PS != NULL)
		{
			PS->ServerReceiveTaunt2Class(NewTauntPath);
		}
	}
}

FString UUTLocalPlayer::GetDefaultURLOption(const TCHAR* Key) const
{
	FURL DefaultURL;
	DefaultURL.LoadURLConfig(TEXT("DefaultPlayer"), GGameIni);
	FString Op = DefaultURL.GetOption(Key, TEXT(""));
	FString Result;
	Op.Split(TEXT("="), NULL, &Result);
	return Result;
}
void UUTLocalPlayer::SetDefaultURLOption(const FString& Key, const FString& Value)
{
	FURL DefaultURL;
	DefaultURL.LoadURLConfig(TEXT("DefaultPlayer"), GGameIni);
	DefaultURL.AddOption(*FString::Printf(TEXT("%s=%s"), *Key, *Value));
	DefaultURL.SaveURLConfig(TEXT("DefaultPlayer"), *Key, GGameIni);
}

#if !UE_SERVER
void UUTLocalPlayer::ShowContentLoadingMessage()
{
	if (!ContentLoadingMessage.IsValid())
	{
		SAssignNew(ContentLoadingMessage, SOverlay)
		+SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.HAlign(HAlign_Center)
				.AutoHeight()
				[
					SNew(SBox)
					.WidthOverride(700)
					.HeightOverride(64)
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.VAlign(VAlign_Fill)
							.HAlign(HAlign_Fill)
							[
								SNew(SImage)
								.Image(SUWindowsStyle::Get().GetBrush("UWindows.Standard.Dialog.Background"))
							]
						]
						+SOverlay::Slot()
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.VAlign(VAlign_Center)
							.HAlign(HAlign_Center)
							[
								SNew(STextBlock)
								.Text(NSLOCTEXT("MenuMessages","InitMenu","Initializing Menus"))
								.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.TextStyle")
							]
						]
					]
				]
			]
		];
	}

	if (ContentLoadingMessage.IsValid())
	{
		GEngine->GameViewport->AddViewportWidgetContent(ContentLoadingMessage.ToSharedRef(), 255);
	}

}

void UUTLocalPlayer::HideContentLoadingMessage()
{
	if (ContentLoadingMessage.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(ContentLoadingMessage.ToSharedRef());			
		ContentLoadingMessage.Reset();
	}
}

TSharedPtr<SUWFriendsPopup> UUTLocalPlayer::GetFriendsPopup()
{
	if (!FriendsMenu.IsValid())
	{
		SAssignNew(FriendsMenu, SUWFriendsPopup)
			.PlayerOwner(this);
	}

	return FriendsMenu;
}


#endif

void UUTLocalPlayer::ReturnToMainMenu()
{
	HideMenu();

	// Under certain situations (when we fail to load a replay immediately after starting watching it), 
	//	the replay menu will show up at the last second, and nothing will close it.
	// This is to make absolutely sure the replay menu doesn't persist into the main menu
	CloseReplayWindow();

	if ( GetWorld() != nullptr )
	{
		Exec( GetWorld(), TEXT( "disconnect" ), *GLog );
	}
	else
	{
		// If we don't have a world, we likely failed to load one during replays
		// Recover by loading default map
		FURL DefaultURL;
		DefaultURL.LoadURLConfig( TEXT( "DefaultPlayer" ), GGameIni );

		FURL URL( &DefaultURL, TEXT( "" ), TRAVEL_Partial );

		FString Error;

		GEngine->Browse( *GetGameInstance()->GetWorldContext(), URL, Error );
	}
}

bool UUTLocalPlayer::JoinSession(const FOnlineSessionSearchResult& SearchResult, bool bSpectate, FName QuickMatchType, bool bFindMatch, int32 DesiredTeam,  FString MatchId)
{
	UE_LOG(UT,Log, TEXT("##########################"));
	UE_LOG(UT,Log, TEXT("Joining a New Session"));
	UE_LOG(UT,Log, TEXT("##########################"));

	QuickMatchJoinType = QuickMatchType;

	bWantsToConnectAsSpectator = bSpectate;
	bWantsToFindMatch = bFindMatch;

	ConnectDesiredTeam = DesiredTeam;

	PendingJoinMatchId =  MatchId;


	FUniqueNetIdRepl UniqueId = OnlineIdentityInterface->GetUniquePlayerId(0);

	if (!UniqueId.IsValid())
	{
		return false;
	}
	else
	{
		if (OnlineSessionInterface->IsPlayerInSession(GameSessionName, *UniqueId))
		{
			UE_LOG(UT, Log, TEXT("--- Already in a Session -- Deferring while I clean it up"));
			bPendingSession = true;
			PendingSession = SearchResult;
			LeaveSession();
		}
		else
		{
			SearchResult.Session.SessionSettings.Get(SETTING_TRUSTLEVEL, CurrentSessionTrustLevel);
			OnlineSessionInterface->JoinSession(0, GameSessionName, SearchResult);
		}

		return true;
	}
}

void UUTLocalPlayer::JoinPendingSession()
{
	if (bPendingSession)
	{
		bPendingSession = false;
		PendingSession.Session.SessionSettings.Get(SETTING_TRUSTLEVEL, CurrentSessionTrustLevel);
		OnlineSessionInterface->JoinSession(0, GameSessionName, PendingSession);
	}
}

void UUTLocalPlayer::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	bPendingSession = false;

	UE_LOG(UT,Log, TEXT("----------- [OnJoinSessionComplete %i"), (Result == EOnJoinSessionCompleteResult::Success));

	// If we are trying to be crammed in to an existing session, we can just exit.
	if (bAttemptingForceJoin)
	{
		bAttemptingForceJoin = false;
		return;
	}

	ChatArchive.Empty();

	// If we successed, nothing else needs to be done.
	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		FString ConnectionString;
		if ( OnlineSessionInterface->GetResolvedConnectString(SessionName, ConnectionString) )
		{
			if (QuickMatchJoinType != NAME_None)
			{
				if (QuickMatchJoinType == QuickMatchTypes::Deathmatch)
				{
					ConnectionString += TEXT("?QuickStart=DM");
				}
				else if (QuickMatchJoinType == QuickMatchTypes::CaptureTheFlag)
				{
					ConnectionString += TEXT("?QuickStart=CTF");
				}
			}
			QuickMatchJoinType = NAME_None;

			if (PendingFriendInviteFriendId != TEXT(""))
			{
				ConnectionString += FString::Printf(TEXT("?Friend=%s"), *PendingFriendInviteFriendId);
				PendingFriendInviteFriendId = TEXT("");
			}

			if (bWantsToFindMatch)
			{
				ConnectionString += TEXT("?RTM=1");
			}

			ConnectionString += FString::Printf(TEXT("?SpectatorOnly=%i"), bWantsToConnectAsSpectator ? 1 : 0);

			if (ConnectDesiredTeam >= 0)
			{
				ConnectionString += FString::Printf(TEXT("?Team=%i"), ConnectDesiredTeam);
			}

			if (!PendingJoinMatchId.IsEmpty())
			{
				ConnectionString += FString::Printf(TEXT("?MatchId=%s"), * PendingJoinMatchId);
				PendingJoinMatchId.Empty();
			}

			FWorldContext &Context = GEngine->GetWorldContextFromWorldChecked(GetWorld());
			Context.LastURL.RemoveOption(TEXT("QuickStart"));
			
			PlayerController->ClientTravel(ConnectionString, ETravelType::TRAVEL_Partial,false);

			bWantsToFindMatch = false;
			bWantsToConnectAsSpectator = false;
			return;

		}
	}

	// Any failures, return to the main menu.
	bWantsToConnectAsSpectator = false;
	bWantsToFindMatch = false;
	QuickMatchJoinType = NAME_None;

	if (Result == EOnJoinSessionCompleteResult::AlreadyInSession)
	{
		MessageBox(NSLOCTEXT("MCPMessages", "OnlineError", "Online Error"), NSLOCTEXT("MCPMessages", "AlreadyInSession", "You are already in a session and can't join another."));
	}


	if (Result == EOnJoinSessionCompleteResult::SessionIsFull)
	{
		MessageBox(NSLOCTEXT("MCPMessages", "OnlineError", "Online Error"), NSLOCTEXT("MCPMessages", "SessionFull", "The session you are attempting to join is full."));
	}

	CloseConnectingDialog();
#if !UE_SERVER
	if (GetWorld()->bIsDefaultLevel && !DesktopSlateWidget.IsValid())
	{
		ReturnToMainMenu();
	}
#endif
}

void UUTLocalPlayer::LeaveSession()
{
	if (OnlineIdentityInterface.IsValid())
	{
		TSharedPtr<FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(0);
		if (UserId.IsValid() && OnlineSessionInterface.IsValid() && OnlineSessionInterface->IsPlayerInSession(GameSessionName, *UserId))
		{
			OnEndSessionCompleteDelegate = OnlineSessionInterface->AddOnEndSessionCompleteDelegate_Handle(FOnEndSessionCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnEndSessionComplete));
			OnlineSessionInterface->EndSession(GameSessionName);
		}
		else
		{
			if (bPendingLoginCreds)
			{
				Logout();
			}
		}
	}
}

void UUTLocalPlayer::OnEndSessionComplete(FName SessionName, bool bWasSuccessful)
{
	OnlineSessionInterface->ClearOnEndSessionCompleteDelegate_Handle(OnEndSessionCompleteDelegate);
	OnDestroySessionCompleteDelegate = OnlineSessionInterface->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnDestroySessionComplete));
	OnlineSessionInterface->DestroySession(GameSessionName);
}

void UUTLocalPlayer::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(UT,Log, TEXT("----------- [OnDestroySessionComplete %i"), bPendingSession);
	
	OnlineSessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate);

	if (bPendingLoginCreds)
	{
		Logout();
	}
	else
	{
		JoinPendingSession();
	}

}

void UUTLocalPlayer::UpdatePresence(FString NewPresenceString, bool bAllowInvites, bool bAllowJoinInProgress, bool bAllowJoinViaPresence, bool bAllowJoinViaPresenceFriendsOnly)
{
	if (OnlineIdentityInterface.IsValid() && OnlineSessionInterface.IsValid() && OnlinePresenceInterface.IsValid())
	{
		TSharedPtr<FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
		if (UserId.IsValid())
		{
			FOnlineSessionSettings* GameSettings = OnlineSessionInterface->GetSessionSettings(TEXT("Game"));
			if (GameSettings != NULL)
			{
				GameSettings->bAllowInvites = true;
				GameSettings->bAllowJoinInProgress = true;
				GameSettings->bAllowJoinViaPresence = true;
				GameSettings->bAllowJoinViaPresenceFriendsOnly = false;
				OnlineSessionInterface->UpdateSession(TEXT("Game"), *GameSettings, false);
			}

			TSharedPtr<FOnlineUserPresence> CurrentPresence;
			OnlinePresenceInterface->GetCachedPresence(*UserId, CurrentPresence);
			if (CurrentPresence.IsValid())
			{
				CurrentPresence->Status.StatusStr = NewPresenceString;
				OnlinePresenceInterface->SetPresence(*UserId, CurrentPresence->Status, IOnlinePresence::FOnPresenceTaskCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnPresenceUpdated));
			}
			else
			{
				FOnlineUserPresenceStatus NewStatus;
				NewStatus.State = EOnlinePresenceState::Online;
				NewStatus.StatusStr = NewPresenceString;
				OnlinePresenceInterface->SetPresence(*UserId, NewStatus, IOnlinePresence::FOnPresenceTaskCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnPresenceUpdated));
			}
		}
		else
		{
			LastPresenceUpdate = NewPresenceString;
			bLastAllowInvites = bAllowInvites;
		}
	}
}

bool UUTLocalPlayer::IsPlayerShowingSocialNotification() const
{
	return bShowSocialNotification;
}

void UUTLocalPlayer::OnPresenceUpdated(const FUniqueNetId& UserId, const bool bWasSuccessful)
{
	UE_LOG(UT,Verbose,TEXT("OnPresenceUpdated %s"), (bWasSuccessful ? TEXT("Successful") : TEXT("Failed")));
}

void UUTLocalPlayer::OnPresenceReceived(const FUniqueNetId& UserId, const TSharedRef<FOnlineUserPresence>& Presence)
{
	UE_LOG(UT,Verbose,TEXT("Presence Received %s %i %i"), *UserId.ToString(), Presence->bIsJoinable);
}

void UUTLocalPlayer::HandleFriendsJoinGame(const FUniqueNetId& FriendId, const FUniqueNetId& SessionId)
{
	JoinFriendSession(FriendId, SessionId);
}

bool UUTLocalPlayer::AllowFriendsJoinGame()
{
	// determine when to disable "join game" option in friends/chat UI
	return true;
}

void UUTLocalPlayer::HandleFriendsNotificationAvail(bool bAvailable)
{
	bShowSocialNotification = bAvailable;
}

void UUTLocalPlayer::HandleFriendsActionNotification(TSharedRef<FFriendsAndChatMessage> FriendsAndChatMessage)
{
	if (FriendsAndChatMessage->GetMessageType() == EFriendsRequestType::GameInvite ||
		FriendsAndChatMessage->GetMessageType() == EFriendsRequestType::FriendAccepted || 
		FriendsAndChatMessage->GetMessageType() == EFriendsRequestType::FriendInvite)
	{
		ShowToast(FText::FromString(FriendsAndChatMessage->GetMessage()));
	}
}

void UUTLocalPlayer::JoinFriendSession(const FUniqueNetId& FriendId, const FUniqueNetId& SessionId)
{
	UE_LOG(UT, Log, TEXT("##########################"));
	UE_LOG(UT, Log, TEXT("Joining a Friend Session"));
	UE_LOG(UT, Log, TEXT("##########################"));

	//@todo samz - use FindSessionById instead of FindFriendSession with a pending SessionId
	PendingFriendInviteSessionId = SessionId.ToString();
	PendingFriendInviteFriendId = FriendId.ToString();
	OnFindFriendSessionCompleteDelegate = OnlineSessionInterface->AddOnFindFriendSessionCompleteDelegate_Handle(0, FOnFindFriendSessionCompleteDelegate::CreateUObject(this, &UUTLocalPlayer::OnFindFriendSessionComplete));
	OnlineSessionInterface->FindFriendSession(0, FriendId);
}

void UUTLocalPlayer::OnFindFriendSessionComplete(int32 LocalUserNum, bool bWasSuccessful, const FOnlineSessionSearchResult& SearchResult)
{
	OnlineSessionInterface->ClearOnFindFriendSessionCompleteDelegate_Handle(LocalUserNum, OnFindFriendSessionCompleteDelegate);
	if (bWasSuccessful)
	{
		if (SearchResult.Session.SessionInfo.IsValid())
		{
			JoinSession(SearchResult, false);
		}
		else
		{
			PendingFriendInviteFriendId = TEXT("");
			MessageBox(NSLOCTEXT("MCPMessages", "OnlineError", "Online Error"), NSLOCTEXT("MCPMessages", "InvalidFriendSession", "Friend no longer in session."));
		}
	}
	else
	{
		PendingFriendInviteFriendId = TEXT("");
		MessageBox(NSLOCTEXT("MCPMessages", "OnlineError", "Online Error"), NSLOCTEXT("MCPMessages", "NoFriendSession", "Couldn't find friend session to join."));
	}
	PendingFriendInviteSessionId = FString();
}

FName UUTLocalPlayer::GetCountryFlag()
{
	if (CurrentProfileSettings)
	{
		return CurrentProfileSettings->CountryFlag;
	}
	if (PlayerController)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerController->PlayerState);
		if (PS)
		{
			return PS->CountryFlag;
		}
	}
	return NAME_None;
}

void UUTLocalPlayer::SetCountryFlag(FName NewFlag, bool bSave)
{
	if (CurrentProfileSettings)
	{
		CurrentProfileSettings->CountryFlag = NewFlag;
		if (bSave)
		{
			SaveProfileSettings();
		}
	}

	AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerController);
	if (PC != NULL)
	{
		PC->ServerReceiveCountryFlag(NewFlag);
	}
}

FName UUTLocalPlayer::GetAvatar()
{
	if (CurrentProfileSettings)
	{
		return CurrentProfileSettings->Avatar;
	}

	if (PlayerController)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerController->PlayerState);
		if (PS) return PS->Avatar;
	}
	return NAME_None;
}

void UUTLocalPlayer::SetAvatar(FName NewAvatar, bool bSave)
{
	if (CurrentProfileSettings)
	{
		CurrentProfileSettings->Avatar = NewAvatar;
		if (bSave)
		{
			SaveProfileSettings();
		}
	}

	AUTBasePlayerController * BasePC = Cast<AUTBasePlayerController>(PlayerController);
	if (BasePC != NULL)
	{
		BasePC->ServerSetAvatar(NewAvatar);
	}
}


#if !UE_SERVER

void UUTLocalPlayer::StartQuickMatch(FString QuickMatchType)
{
	if (IsLoggedIn() && OnlineSessionInterface.IsValid())
	{
		if (QuickMatchDialog.IsValid())
		{
			return;
		}

		if (GetWorld()->GetTimeSeconds() < QuickMatchLimitTime)
		{
			MessageBox(NSLOCTEXT("Generic","CantStartQuickMatchTitle","Please Wait"), NSLOCTEXT("Generic","CantStartQuickMatchText","You need to wait for at least 1 minute before you can attempt to quickmatch again."));
			return;
		}

		if ( ServerBrowserWidget.IsValid() && ServerBrowserWidget->IsRefreshing())
		{
			MessageBox(NSLOCTEXT("Generic","RequestInProgressTitle","Busy"), NSLOCTEXT("Generic","RequestInProgressText","A server list request is already in progress.  Please wait for it to finish before attempting to quickmatch."));
			return;
		}

		if (OnlineSessionInterface.IsValid())
		{
			OnlineSessionInterface->CancelFindSessions();				
		}

		SAssignNew(QuickMatchDialog, SUTQuickMatch)
			.PlayerOwner(this)
			.QuickMatchType(QuickMatchType);

		if (QuickMatchDialog.IsValid())
		{
			GEngine->GameViewport->AddViewportWidgetContent(QuickMatchDialog.ToSharedRef(), 150);
			QuickMatchDialog->TellSlateIWantKeyboardFocus();
		}
	}
	else
	{
		MessageBox(NSLOCTEXT("Generic","LoginNeededTitle","Login Needed"), NSLOCTEXT("Generic","LoginNeededMessage","You need to login before you can do that."));
	}
}
void UUTLocalPlayer::CloseQuickMatch()
{
	if (QuickMatchDialog.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(QuickMatchDialog.ToSharedRef());
		QuickMatchDialog.Reset();
	}
}

#endif

void UUTLocalPlayer::ShowConnectingDialog()
{
#if !UE_SERVER
	if (!ConnectingDialog.IsValid())
	{
		FDialogResultDelegate Callback;
		Callback.BindUObject(this, &UUTLocalPlayer::ConnectingDialogCancel);

		TSharedPtr<SUWDialog> NewDialog; // important to make sure the ref count stays until OpenDialog()
		SAssignNew(NewDialog, SUWMessageBox)
			.PlayerOwner(this)
			.DialogTitle(NSLOCTEXT("UT", "ConnectingTitle", "Connecting..."))
			.MessageText(NSLOCTEXT("UT", "ConnectingText", "Connecting to server, please wait..."))
			.ButtonMask(UTDIALOG_BUTTON_CANCEL)
			.OnDialogResult(Callback);

		ConnectingDialog = NewDialog;
		OpenDialog(NewDialog.ToSharedRef());
	}
#endif
}
void UUTLocalPlayer::CloseConnectingDialog()
{
#if !UE_SERVER
	if (ConnectingDialog.IsValid())
	{
		CloseDialog(ConnectingDialog.Pin().ToSharedRef());
	}
#endif
}
void UUTLocalPlayer::ConnectingDialogCancel(TSharedPtr<SCompoundWidget> Dialog, uint16 ButtonID)
{
#if !UE_SERVER
	GEngine->Exec(GetWorld(), TEXT("Cancel"));
#endif
}

bool UUTLocalPlayer::IsInSession()
{ 
	TSharedPtr<FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(0);
	return (UserId.IsValid() && OnlineSessionInterface.IsValid() && OnlineSessionInterface->IsPlayerInSession(GameSessionName,*UserId));
}

void UUTLocalPlayer::ShowPlayerInfo(TWeakObjectPtr<AUTPlayerState> Target)
{
#if !UE_SERVER
	if (MatchSummaryWindow.IsValid() && Target.IsValid())
	{
		MatchSummaryWindow->SelectPlayerState(Target.Get());
	}
	else
	{
		if (DesktopSlateWidget.IsValid() && !IsMenuGame() && Cast<AUTLobbyGameState>(GetWorld()->GameState) == nullptr)
		{
			HideMenu();
		}
		OpenDialog(SNew(SUWPlayerInfoDialog).PlayerOwner(this).TargetPlayerState(Target));
	}
#endif
}

int32 UUTLocalPlayer::GetFriendsList(TArray< FUTFriend >& OutFriendsList)
{
	OutFriendsList.Empty();

	TArray< TSharedPtr< IFriendItem > > FriendsList;
	int32 RetVal = IFriendsAndChatModule::Get().GetFriendsAndChatManager()->GetFilteredFriendsList(FriendsList);
	for (auto Friend : FriendsList)
	{
		OutFriendsList.Add(FUTFriend(Friend->GetUniqueID()->ToString(), Friend->GetName()));
	}

	return RetVal;
}

int32 UUTLocalPlayer::GetRecentPlayersList(TArray< FUTFriend >& OutRecentPlayersList)
{
	OutRecentPlayersList.Empty();

	TArray< TSharedPtr< IFriendItem > > RecentPlayersList;
	int32 RetVal = IFriendsAndChatModule::Get().GetFriendsAndChatManager()->GetRecentPlayersList(RecentPlayersList);
	for (auto RecentPlayer : RecentPlayersList)
	{
		OutRecentPlayersList.Add(FUTFriend(RecentPlayer->GetUniqueID()->ToString(), RecentPlayer->GetName()));
	}

	return RetVal;
}


void UUTLocalPlayer::OnTauntPlayed(AUTPlayerState* PS, TSubclassOf<AUTTaunt> TauntToPlay, float EmoteSpeed)
{
#if !UE_SERVER
	if (MatchSummaryWindow.IsValid())
	{
		MatchSummaryWindow->PlayTauntByClass(PS, TauntToPlay, EmoteSpeed);
	}
#endif
}

void UUTLocalPlayer::OnEmoteSpeedChanged(AUTPlayerState* PS, float EmoteSpeed)
{
#if !UE_SERVER
	if (MatchSummaryWindow.IsValid())
	{
		MatchSummaryWindow->SetEmoteSpeed(PS, EmoteSpeed);
	}
#endif
}

void UUTLocalPlayer::RequestFriendship(TSharedPtr<FUniqueNetId> FriendID)
{
	if (OnlineFriendsInterface.IsValid() && FriendID.IsValid())
	{
		OnlineFriendsInterface->SendInvite(0, *FriendID.Get(), EFriendsLists::ToString(EFriendsLists::Default));
	}
}

void UUTLocalPlayer::UpdateRedirect(const FString& FileURL, int32 NumBytes, float Progress, int32 NumFilesLeft)
{
	DownloadStatusText = FText::Format(NSLOCTEXT("UTLocalPlayer","DownloadStatusFormat","Downloading {0} Files: {1} ({2} / {3}) ...."), FText::AsNumber(NumFilesLeft), FText::FromString(FileURL), FText::AsNumber(NumBytes), FText::AsPercent(Progress));
	UE_LOG(UT,Verbose,TEXT("Redirect: %s %i [%f%%]"), *FileURL, NumBytes, Progress);
}

bool UUTLocalPlayer::ContentExists(const FPackageRedirectReference& Redirect)
{
	FString Path = FPaths::Combine(*FPaths::GameSavedDir(), TEXT("Paks"), TEXT("DownloadedPaks"), *Redirect.PackageName) + TEXT(".pak");
	UUTGameEngine* UTEngine = Cast<UUTGameEngine>(GEngine);
	if (UTEngine)
	{
		if (UTEngine->LocalContentChecksums.Contains(Redirect.PackageName))
		{
			if (UTEngine->LocalContentChecksums[Redirect.PackageName] == Redirect.PackageChecksum)
			{
				return true;
			}
			else
			{
				// Local content has a non-matching md5, not sure if we should try to unmount/delete it
				return false;
			}
		}

		if (UTEngine->MountedDownloadedContentChecksums.Contains(Redirect.PackageName))
		{
			if (UTEngine->MountedDownloadedContentChecksums[Redirect.PackageName] == Redirect.PackageChecksum)
			{
				// We've already mounted the content needed and the checksum matches
				return true;
			}
			else
			{
				// Unmount the pak
				if (FCoreDelegates::OnUnmountPak.IsBound())
				{
					FCoreDelegates::OnUnmountPak.Execute(Path);
				}

				// Remove the CRC entry
				UTEngine->MountedDownloadedContentChecksums.Remove(Redirect.PackageName);
				UTEngine->DownloadedContentChecksums.Remove(Redirect.PackageName);

				// Delete the original file
				FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*Path);
			}
		}

		if (UTEngine->DownloadedContentChecksums.Contains(Redirect.PackageName))
		{
			if (UTEngine->DownloadedContentChecksums[Redirect.PackageName] == Redirect.PackageChecksum)
			{
				// Mount the pak
				if (FCoreDelegates::OnMountPak.IsBound())
				{
					FCoreDelegates::OnMountPak.Execute(Path, 0);
					UTEngine->MountedDownloadedContentChecksums.Add(Redirect.PackageName, Redirect.PackageChecksum);
				}

				return true;
			}
			else
			{
				UTEngine->DownloadedContentChecksums.Remove(Redirect.PackageName);

				// Delete the original file
				FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*Path);
			}
		}

		return false;
	}

	return true;

}

void UUTLocalPlayer::AccquireContent(TArray<FPackageRedirectReference>& Redirects)
{
	UUTGameViewportClient* UTGameViewport = Cast<UUTGameViewportClient>(ViewportClient);
	if (UTGameViewport)
	{
		for (int32 i = 0; i < Redirects.Num(); i++)
		{
			if (!Redirects[i].PackageName.IsEmpty() && !ContentExists(Redirects[i]))
			{
				UTGameViewport->DownloadRedirect(Redirects[i]);
			}
		}
	}
}

FText UUTLocalPlayer::GetDownloadStatusText()
{
	return IsDownloadInProgress() ? DownloadStatusText : FText::GetEmpty();
}

bool UUTLocalPlayer::IsDownloadInProgress()
{
	UUTGameViewportClient* UTGameViewport = Cast<UUTGameViewportClient>(ViewportClient);
	return UTGameViewport ? UTGameViewport->IsDownloadInProgress() : false;
}

void UUTLocalPlayer::CancelDownload()
{
	UUTGameViewportClient* UTGameViewport = Cast<UUTGameViewportClient>(ViewportClient);
	if (UTGameViewport && UTGameViewport->IsDownloadInProgress())
	{
		UTGameViewport->CancelAllRedirectDownloads();		
	}
}

void UUTLocalPlayer::HandleNetworkFailureMessage(enum ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	AUTBasePlayerController* BasePlayerController = Cast<AUTBasePlayerController>(PlayerController);
	if (BasePlayerController)
	{
		BasePlayerController->HandleNetworkFailureMessage(FailureType, ErrorString);
	}
}
void UUTLocalPlayer::OpenLoadout(bool bBuyMenu)
{
#if !UE_SERVER
	// Create the slate widget if it doesn't exist
	if (!LoadoutMenu.IsValid())
	{
		if (bBuyMenu)
		{
			SAssignNew(LoadoutMenu, SUTBuyMenu).PlayerOwner(this);
		}
		else
		{
			SAssignNew(LoadoutMenu, SUTLoadoutMenu).PlayerOwner(this);
		}

		if (LoadoutMenu.IsValid())
		{
			GEngine->GameViewport->AddViewportWidgetContent( SNew(SWeakWidget).PossiblyNullContent(LoadoutMenu.ToSharedRef()),60);
		}

		// Make it visible.
		if (LoadoutMenu.IsValid())
		{
			// Widget is already valid, just make it visible.
			LoadoutMenu->SetVisibility(EVisibility::Visible);
			LoadoutMenu->OnMenuOpened(TEXT(""));
		}
	}
#endif
}
void UUTLocalPlayer::CloseLoadout()
{
#if !UE_SERVER
	if (LoadoutMenu.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(LoadoutMenu.ToSharedRef());
		LoadoutMenu->OnMenuClosed();
		LoadoutMenu.Reset();
		if (PlayerController)
		{
			PlayerController->SetPause(false);
		}
	}
#endif
}

void UUTLocalPlayer::OpenMapVote(AUTGameState* GameState)
{
#if !UE_SERVER
	if (!MapVoteMenu.IsValid())
	{

		if (GameState == NULL)
		{
			GameState = GetWorld()->GetGameState<AUTGameState>();
			if (GameState == NULL) return;
		}

		SAssignNew(MapVoteMenu,SUWMapVoteDialog).PlayerOwner(this).GameState(GameState);
		OpenDialog( MapVoteMenu.ToSharedRef(), 200 );
	}
#endif
}
void UUTLocalPlayer::CloseMapVote()
{
#if !UE_SERVER
	if (MapVoteMenu.IsValid())
	{
		CloseDialog(MapVoteMenu.ToSharedRef());		
		MapVoteMenu.Reset();
	}
#endif
}

void UUTLocalPlayer::OpenMatchSummary(AUTGameState* GameState)
{
#if !UE_SERVER
	if (MatchSummaryWindow.IsValid())
	{
		CloseMatchSummary();
	}
	SAssignNew(MatchSummaryWindow, SUWMatchSummary).PlayerOwner(this).GameState(GameState);

	//Disable world rendering since this is a fullscreen widget with a render target
	GEngine->GameViewport->bDisableWorldRendering = true;

	UUTGameViewportClient* UTGVC = Cast<UUTGameViewportClient>(GEngine->GameViewport);
	if (MatchSummaryWindow.IsValid() && UTGVC != nullptr)
	{
		UTGVC->AddViewportWidgetContent(MatchSummaryWindow.ToSharedRef(), -1);
		FSlateApplication::Get().SetKeyboardFocus(MatchSummaryWindow.ToSharedRef(), EKeyboardFocusCause::Keyboard);
	}
#endif
}
void UUTLocalPlayer::CloseMatchSummary()
{
#if !UE_SERVER
	UUTGameViewportClient* UTGVC = Cast<UUTGameViewportClient>(GEngine->GameViewport);
	if (MatchSummaryWindow.IsValid() && UTGVC != nullptr)
	{
		UTGVC->RemoveViewportWidgetContent(MatchSummaryWindow.ToSharedRef());
		MatchSummaryWindow.Reset();

		//Enable rendering again
		GEngine->GameViewport->bDisableWorldRendering = false;

		//Since we use SUInGameHomePanel for the time being for chat, we need to clear bForceScores
		AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerController);
		if (PC && PC->MyUTHUD)
		{
			PC->MyUTHUD->bForceScores = false;
		}
	}
#endif
}

void UUTLocalPlayer::OpenReplayWindow()
{
#if !UE_SERVER
	UDemoNetDriver* DemoDriver = GetWorld()->DemoNetDriver;
	if (DemoDriver)
	{
		// If the demo net driver changed, reopen the window, we're in a different replay.
		if (ReplayWindow.IsValid() && ReplayWindow->GetDemoNetDriver() != DemoDriver)
		{
			CloseReplayWindow();
		}

		if (!ReplayWindow.IsValid())
		{
			SAssignNew(ReplayWindow, SUTReplayWindow)
				.PlayerOwner(this)
				.DemoNetDriver(DemoDriver);

			UUTGameViewportClient* UTGVC = Cast<UUTGameViewportClient>(GEngine->GameViewport);
			if (ReplayWindow.IsValid() && UTGVC != nullptr)
			{
				UTGVC->AddViewportWidgetContent_NoAspect(ReplayWindow.ToSharedRef(), -1);
				ReplayWindow->SetVisibility(EVisibility::SelfHitTestInvisible);
			}
		}
	}
#endif
}

void UUTLocalPlayer::CloseReplayWindow()
{
#if !UE_SERVER
	UUTGameViewportClient* UTGVC = Cast<UUTGameViewportClient>(GEngine->GameViewport);
	if (ReplayWindow.IsValid() && UTGVC != nullptr)
	{
		UTGVC->RemoveViewportWidgetContent_NoAspect(ReplayWindow.ToSharedRef());
		ReplayWindow.Reset();
	}
#endif
}

void UUTLocalPlayer::ToggleReplayWindow()
{
#if !UE_SERVER
	if (IsReplay())
	{
		if (!ReplayWindow.IsValid())
		{
			OpenReplayWindow();
		}
		else
		{
			CloseReplayWindow();
		}
	}
#endif
}

bool UUTLocalPlayer::IsReplay()
{
	return (GetWorld()->DemoNetDriver != nullptr);
}

#if !UE_SERVER

void UUTLocalPlayer::RecordReplay(float RecordTime)
{
	if (!bRecordingReplay)
	{
		CloseReplayWindow();

		bRecordingReplay = true;

		static const FName VideoRecordingFeatureName("VideoRecording");
		if (IModularFeatures::Get().IsModularFeatureAvailable(VideoRecordingFeatureName))
		{
			UTVideoRecordingFeature* VideoRecorder = &IModularFeatures::Get().GetModularFeature<UTVideoRecordingFeature>(VideoRecordingFeatureName);
			if (VideoRecorder)
			{
				VideoRecorder->OnRecordingComplete().AddUObject(this, &UUTLocalPlayer::RecordingReplayComplete);
				VideoRecorder->StartRecording(RecordTime);
			}
		}
	}
}

void UUTLocalPlayer::RecordingReplayComplete()
{
	bRecordingReplay = false;

	static const FName VideoRecordingFeatureName("VideoRecording");
	if (IModularFeatures::Get().IsModularFeatureAvailable(VideoRecordingFeatureName))
	{
		UTVideoRecordingFeature* VideoRecorder = &IModularFeatures::Get().GetModularFeature<UTVideoRecordingFeature>(VideoRecordingFeatureName);
		if (VideoRecorder)
		{
			VideoRecorder->OnRecordingComplete().RemoveAll(this);
		}
	}

	// Pause the replay streamer
	AWorldSettings* const WorldSettings = GetWorld()->GetWorldSettings();
	if (WorldSettings->Pauser == nullptr)
	{
		WorldSettings->Pauser = (PlayerController != nullptr) ? PlayerController->PlayerState : nullptr;
	}

	// Show a dialog asking player if they want to compress
	ShowMessage(NSLOCTEXT("VideoMessages", "CompressNowTitle", "Compress now?"),
				NSLOCTEXT("VideoMessages", "CompressNow", "Your video recorded successfully.\nWould you like to compress the video now? It may take several minutes."),
				UTDIALOG_BUTTON_YES | UTDIALOG_BUTTON_NO, FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::ShouldVideoCompressDialogResult));
}

void UUTLocalPlayer::ShouldVideoCompressDialogResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_YES)
	{
		// Pick a proper filename for the video
		FString BasePath = FPaths::ScreenShotDir();
		if (IFileManager::Get().MakeDirectory(*FPaths::ScreenShotDir(), true))
		{
			RecordedReplayFilename = BasePath / TEXT("anim.webm");
			static int32 WebMIndex = 0;
			const int32 MaxTestWebMIndex = 65536;
			for (int32 TestWebMIndex = WebMIndex + 1; TestWebMIndex < MaxTestWebMIndex; ++TestWebMIndex)
			{
				const FString TestFileName = BasePath / FString::Printf(TEXT("UTReplay%05i.webm"), TestWebMIndex);
				if (IFileManager::Get().FileSize(*TestFileName) < 0)
				{
					WebMIndex = TestWebMIndex;
					RecordedReplayFilename = TestFileName;
					break;
				}
			}

			static const FName VideoRecordingFeatureName("VideoRecording");
			if (IModularFeatures::Get().IsModularFeatureAvailable(VideoRecordingFeatureName))
			{
				UTVideoRecordingFeature* VideoRecorder = &IModularFeatures::Get().GetModularFeature<UTVideoRecordingFeature>(VideoRecordingFeatureName);
				if (VideoRecorder)
				{
					// Open a dialog that shows a nice progress bar of the compression
					OpenDialog(SNew(SUWVideoCompressionDialog)
								.OnDialogResult(FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::VideoCompressDialogResult))
								.DialogTitle(NSLOCTEXT("VideoMessages", "Compressing", "Compressing"))
								.PlayerOwner(this)
								.VideoRecorder(VideoRecorder)
								.VideoFilename(RecordedReplayFilename)
								);
				}
			}
		}
	}
	else
	{
		OpenReplayWindow();
	}
}

void UUTLocalPlayer::VideoCompressDialogResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_OK)
	{
		OpenDialog(SNew(SUWInputBox)
			.OnDialogResult(FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::ShouldVideoUploadDialogResult))
			.PlayerOwner(this)
			.DialogSize(FVector2D(700, 400))
			.bDialogSizeIsRelative(false)
			.DefaultInput(TEXT("UT Automated Upload"))
			.DialogTitle(NSLOCTEXT("VideoMessages", "UploadNowTitle", "Upload to YouTube?"))
			.MessageText(NSLOCTEXT("VideoMessages", "UploadNow", "Your video compressed successfully.\nWould you like to upload the video to YouTube now?\n\nPlease enter a video title in the text box."))
			.ButtonMask(UTDIALOG_BUTTON_YES | UTDIALOG_BUTTON_NO)
			);
	}
	else
	{
		OpenReplayWindow();
	}
}

void UUTLocalPlayer::ShouldVideoUploadDialogResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_YES)
	{
		TSharedPtr<SUWInputBox> Box = StaticCastSharedPtr<SUWInputBox>(Widget);
		if (Box.IsValid())
		{
			RecordedReplayTitle = Box->GetInputText();
		}

		if (YoutubeAccessToken.IsEmpty())
		{
			GetYoutubeConsentForUpload();
		}
		else if (!YoutubeRefreshToken.IsEmpty())
		{
			// Show a dialog here to stop the user for doing anything
			YoutubeDialog = ShowMessage(NSLOCTEXT("VideoMessages", "YoutubeTokenTitle", "AccessingYoutube"),
				NSLOCTEXT("VideoMessages", "YoutubeToken", "Contacting YouTube..."), 0);

			FHttpRequestPtr YoutubeTokenRefreshRequest = FHttpModule::Get().CreateRequest();
			YoutubeTokenRefreshRequest->SetURL(TEXT("https://accounts.google.com/o/oauth2/token"));
			YoutubeTokenRefreshRequest->OnProcessRequestComplete().BindUObject(this, &UUTLocalPlayer::YoutubeTokenRefreshComplete);
			YoutubeTokenRefreshRequest->SetVerb(TEXT("POST"));
			YoutubeTokenRefreshRequest->SetHeader(TEXT("Content-Type"), TEXT("application/x-www-form-urlencoded"));

			// ClientID and ClientSecret UT Youtube app on PLK google account
			FString ClientID = TEXT("465724645978-10npjjgfbb03p4ko12ku1vq1ioshts24.apps.googleusercontent.com");
			FString ClientSecret = TEXT("kNKauX2DKUq_5cks86R8rD5E");
			FString TokenRequest = TEXT("client_id=") + ClientID + TEXT("&client_secret=") + ClientSecret + 
				                   TEXT("&refresh_token=") + YoutubeRefreshToken + TEXT("&grant_type=refresh_token");

			YoutubeTokenRefreshRequest->SetContentAsString(TokenRequest);
			YoutubeTokenRefreshRequest->ProcessRequest();
		}
	}
	else
	{
		OpenReplayWindow();
	}
}

void UUTLocalPlayer::YoutubeConsentResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_OK)
	{
		// Show a dialog here to stop the user for doing anything
		YoutubeDialog = ShowMessage(NSLOCTEXT("VideoMessages", "YoutubeTokenTitle", "AccessingYoutube"),
			NSLOCTEXT("VideoMessages", "YoutubeToken", "Contacting YouTube..."), 0);

		FHttpRequestPtr YoutubeTokenRequest = FHttpModule::Get().CreateRequest();
		YoutubeTokenRequest->SetURL(TEXT("https://accounts.google.com/o/oauth2/token"));
		YoutubeTokenRequest->OnProcessRequestComplete().BindUObject(this, &UUTLocalPlayer::YoutubeTokenRequestComplete);
		YoutubeTokenRequest->SetVerb(TEXT("POST"));
		YoutubeTokenRequest->SetHeader(TEXT("Content-Type"), TEXT("application/x-www-form-urlencoded"));

		// ClientID and ClientSecret UT Youtube app on PLK google account
		FString ClientID = TEXT("465724645978-10npjjgfbb03p4ko12ku1vq1ioshts24.apps.googleusercontent.com");
		FString ClientSecret = TEXT("kNKauX2DKUq_5cks86R8rD5E");
		FString TokenRequest = TEXT("code=") + YoutubeConsentDialog->UniqueCode + TEXT("&client_id=") + ClientID
			+ TEXT("&client_secret=") + ClientSecret + TEXT("&redirect_uri=urn:ietf:wg:oauth:2.0:oob&grant_type=authorization_code");

		YoutubeTokenRequest->SetContentAsString(TokenRequest);
		YoutubeTokenRequest->ProcessRequest();
	}
	else
	{
		UE_LOG(UT, Warning, TEXT("Failed to get Youtube consent"));
	}
}

void UUTLocalPlayer::YoutubeTokenRequestComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	if (YoutubeDialog.IsValid())
	{
		CloseDialog(YoutubeDialog.ToSharedRef());
	}

	if (HttpResponse->GetResponseCode() == 200)
	{
		TSharedPtr<FJsonObject> YoutubeTokenJson;
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(HttpResponse->GetContentAsString());
		if (FJsonSerializer::Deserialize(JsonReader, YoutubeTokenJson) && YoutubeTokenJson.IsValid())
		{
			YoutubeTokenJson->TryGetStringField(TEXT("access_token"), YoutubeAccessToken);
			YoutubeTokenJson->TryGetStringField(TEXT("refresh_token"), YoutubeRefreshToken);

			UE_LOG(UT, Log, TEXT("YoutubeTokenRequestComplete %s %s"), *YoutubeAccessToken, *YoutubeRefreshToken);

			SaveConfig();

			UploadVideoToYoutube();
		}
	}
	else
	{
		UE_LOG(UT, Warning, TEXT("Failed to get token from Youtube\n%s"), *HttpResponse->GetContentAsString());
	}
}

void UUTLocalPlayer::YoutubeTokenRefreshComplete(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	if (YoutubeDialog.IsValid())
	{
		CloseDialog(YoutubeDialog.ToSharedRef());
	}

	if (HttpResponse->GetResponseCode() == 200)
	{
		UE_LOG(UT, Log, TEXT("YouTube Token refresh succeeded"));

		TSharedPtr<FJsonObject> YoutubeTokenJson;
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(HttpResponse->GetContentAsString());
		if (FJsonSerializer::Deserialize(JsonReader, YoutubeTokenJson) && YoutubeTokenJson.IsValid())
		{
			YoutubeTokenJson->TryGetStringField(TEXT("access_token"), YoutubeAccessToken);
			SaveConfig();

			UploadVideoToYoutube();
		}
	}
	else
	{
		UE_LOG(UT, Log, TEXT("YouTube Token might've expired, doing full consent\n%s"), *HttpResponse->GetContentAsString());

		// Refresh token might have been expired
		YoutubeAccessToken.Empty();
		YoutubeRefreshToken.Empty();

		GetYoutubeConsentForUpload();
	}
}

void UUTLocalPlayer::GetYoutubeConsentForUpload()
{
	// Get youtube consent
	OpenDialog(
		SAssignNew(YoutubeConsentDialog, SUWYoutubeConsent)
		.PlayerOwner(this)
		.DialogSize(FVector2D(0.8f, 0.8f))
		.DialogPosition(FVector2D(0.5f, 0.5f))
		.DialogTitle(NSLOCTEXT("UUTLocalPlayer", "YoutubeConsent", "Allow UT to post to YouTube?"))
		.ButtonMask(UTDIALOG_BUTTON_CANCEL)
		.OnDialogResult(FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::YoutubeConsentResult))
		);
}

void UUTLocalPlayer::UploadVideoToYoutube()
{
	// Get youtube consent
	OpenDialog(
		SNew(SUWYoutubeUpload)
		.PlayerOwner(this)
		.ButtonMask(UTDIALOG_BUTTON_CANCEL)
		.VideoFilename(RecordedReplayFilename)
		.AccessToken(YoutubeAccessToken)
		.VideoTitle(RecordedReplayTitle)
		.DialogTitle(NSLOCTEXT("UUTLocalPlayer", "YoutubeUpload", "Uploading To Youtube"))
		.OnDialogResult(FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::YoutubeUploadResult))
		);
}

void UUTLocalPlayer::YoutubeUploadResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	if (ButtonID == UTDIALOG_BUTTON_OK)
	{
		ShowMessage(NSLOCTEXT("UUTLocalPlayer", "YoutubeUploadCompleteTitle", "Upload To Youtube Complete"),
					NSLOCTEXT("UUTLocalPlayer", "YoutubeUploadComplete", "Your upload to Youtube completed successfully. It will be available in a few minutes."),
					UTDIALOG_BUTTON_OK, FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::YoutubeUploadCompleteResult));
	}
	else
	{
		SUWYoutubeUpload* UploadDialog = (SUWYoutubeUpload*)Widget.Get();
		TSharedRef< TJsonReader<> > JsonReader = TJsonReaderFactory<>::Create(UploadDialog->UploadFailMessage);
		TSharedPtr< FJsonObject > JsonObject;
		FJsonSerializer::Deserialize(JsonReader, JsonObject);
		const TSharedPtr<FJsonObject>* ErrorObject;
		bool bNeedsYoutubeSignup = false;
		if (JsonObject->TryGetObjectField(TEXT("error"), ErrorObject))
		{
			const TArray<TSharedPtr<FJsonValue>>* ErrorArray;
			if ((*ErrorObject)->TryGetArrayField(TEXT("errors"), ErrorArray))
			{
				for (int32 Idx = 0; Idx < ErrorArray->Num(); Idx++)
				{
					FString ErrorReason;
					if ((*ErrorArray)[Idx]->AsObject()->TryGetStringField(TEXT("reason"), ErrorReason))
					{
						if (ErrorReason == TEXT("youtubeSignupRequired"))
						{
							bNeedsYoutubeSignup = true;
						}
					}
				}
			}
		}

		if (bNeedsYoutubeSignup)
		{
			ShowMessage(NSLOCTEXT("UUTLocalPlayer", "YoutubeUploadNeedSignupTitle", "Upload To Youtube Failed"),
						NSLOCTEXT("UUTLocalPlayer", "YoutubeUploadNeedSignup", "Your account does not currently have a YouTube channel.\nPlease create one and try again."),
						UTDIALOG_BUTTON_OK, FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::YoutubeUploadCompleteResult));
		}
		else
		{		
			ShowMessage(NSLOCTEXT("UUTLocalPlayer", "YoutubeUploadCompleteFailedTitle", "Upload To Youtube Failed"),
						NSLOCTEXT("UUTLocalPlayer", "YoutubeUploadCompleteFailed", "Your upload to Youtube did not complete successfully."),
						UTDIALOG_BUTTON_OK, FDialogResultDelegate::CreateUObject(this, &UUTLocalPlayer::YoutubeUploadCompleteResult));
		}
	}
}

void UUTLocalPlayer::YoutubeUploadCompleteResult(TSharedPtr<SCompoundWidget> Widget, uint16 ButtonID)
{
	OpenReplayWindow();
}

#endif

void UUTLocalPlayer::VerifyGameSession(const FString& ServerSessionId)
{
	if (IsReplay())
	{
		return;
	}

	if (OnlineSessionInterface.IsValid())
	{
		// Get our current Session Id.
		FNamedOnlineSession* Session = OnlineSessionInterface->GetNamedSession(FName(TEXT("Game")));
		if (Session == NULL || !Session->SessionInfo.IsValid() || Session->SessionInfo->GetSessionId().ToString() != ServerSessionId)
		{
			TSharedPtr<FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(GetControllerId());
			if (UserId.IsValid())
			{
				TSharedPtr<FUniqueNetId> ServerId = MakeShareable(new FUniqueNetIdString(ServerSessionId));		
				TSharedPtr<FUniqueNetId> EmptyId = MakeShareable(new FUniqueNetIdString(""));				
				FOnSingleSessionResultCompleteDelegate CompletionDelegate;
				CompletionDelegate.BindUObject(this, &UUTLocalPlayer::OnFindSessionByIdComplete);
				OnlineSessionInterface->FindSessionById(*UserId, *ServerId, *EmptyId, CompletionDelegate);
			}
		}
	}
}

void UUTLocalPlayer::OnFindSessionByIdComplete(int32 LocalUserNum, bool bWasSucessful, const FOnlineSessionSearchResult& SearchResult)
{
	if (bWasSucessful)
	{
		bAttemptingForceJoin = true;
		OnlineSessionInterface->JoinSession(0, GameSessionName, SearchResult);
	}
}

void UUTLocalPlayer::CloseAllUI(bool bExceptDialogs)
{
	ChatArchive.Empty();

	if (GetWorld()->WorldType == EWorldType::Game)
	{
		GEngine->GameViewport->RemoveAllViewportWidgets();
	}
#if !UE_SERVER
	if (bExceptDialogs)
	{
		if (GetWorld()->WorldType == EWorldType::Game)
		{
			// restore dialogs to the viewport
			for (TSharedPtr<SUWDialog> Dialog : OpenDialogs)
			{
				if (Dialog.IsValid())
				{
					GEngine->GameViewport->AddViewportWidgetContent(Dialog.ToSharedRef(), 255);
				}
			}
		}
	}
	else
	{
		OpenDialogs.Empty();
	}
	DesktopSlateWidget.Reset();
	ServerBrowserWidget.Reset();
	ReplayBrowserWidget.Reset();
	StatsViewerWidget.Reset();
	CreditsPanelWidget.Reset();
	QuickMatchDialog.Reset();
	LoginDialog.Reset();
	HUDSettings.Reset();
	ContentLoadingMessage.Reset();
	FriendsMenu.Reset();
	RedirectDialog.Reset();
	LoadoutMenu.Reset();
	MapVoteMenu.Reset();
	ReplayWindow.Reset();
	YoutubeDialog.Reset();
	YoutubeConsentDialog.Reset();
	CloseMatchSummary();
#endif
}

void UUTLocalPlayer::AttemptJoinInstance(TSharedPtr<FServerData> ServerData, FString InstanceId, bool bSpectate)
{
#if !UE_SERVER

	SAssignNew(JoinInstanceDialog, SUTJoinInstance)
		.PlayerOwner(this)
		.ServerData(ServerData)
		.InstanceId(InstanceId)
		.bSpectator(bSpectate);

	if (JoinInstanceDialog.IsValid())
	{
		GEngine->GameViewport->AddViewportWidgetContent(JoinInstanceDialog.ToSharedRef(), 151);
		JoinInstanceDialog->TellSlateIWantKeyboardFocus();
	}
#endif
}
void UUTLocalPlayer::CloseJoinInstanceDialog()
{
#if !UE_SERVER
	if (JoinInstanceDialog.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(JoinInstanceDialog.ToSharedRef());
		JoinInstanceDialog.Reset();
	}
#endif

}

int32 UUTLocalPlayer::GetTotalChallengeStars()
{
	return (CurrentProfileSettings ? CurrentProfileSettings->TotalChallengeStars : 0);
}

int32 UUTLocalPlayer::GetChallengeStars(FName ChallengeTag)
{
	if (CurrentProfileSettings)
	{
		for (int32 i = 0 ; i < CurrentProfileSettings->ChallengeResults.Num(); i++)
		{
			if (CurrentProfileSettings->ChallengeResults[i].Tag == ChallengeTag)
			{
				return CurrentProfileSettings->ChallengeResults[i].Stars;
			}
		}
	}

	return 0;
}

FString UUTLocalPlayer::GetChallengeDate(FName ChallengeTag)
{
	if (CurrentProfileSettings)
	{
		for (int32 i = 0 ; i < CurrentProfileSettings->ChallengeResults.Num(); i++)
		{
			if (CurrentProfileSettings->ChallengeResults[i].Tag == ChallengeTag)
			{
				FDateTime LastUpdate = CurrentProfileSettings->ChallengeResults[i].LastUpdate;
				return LastUpdate.ToString(TEXT("%m.%d.%y @ %h:%M:%S%a"));
			}
		}
	}
	return TEXT("Never");
}

void UUTLocalPlayer::ChallengeCompleted(FName ChallengeTag, int32 Stars)
{
	if (CurrentProfileSettings && Stars > 0)
	{
		bool bFound = false;
		for (int32 i = 0 ; i < CurrentProfileSettings->ChallengeResults.Num(); i++)
		{
			if (CurrentProfileSettings->ChallengeResults[i].Tag == ChallengeTag)
			{
				if (CurrentProfileSettings->ChallengeResults[i].Stars < Stars)
				{
					CurrentProfileSettings->ChallengeResults[i].Update(Stars);
				}

				bFound = true;
				break;
			}
		}

		if (!bFound)
		{
			CurrentProfileSettings->ChallengeResults.Add(FUTChallengeResult(ChallengeTag,Stars));
		}

		int32 TotalStars = 0;
		for (int32 i = 0 ; i < CurrentProfileSettings->ChallengeResults.Num(); i++)
		{
			TotalStars += CurrentProfileSettings->ChallengeResults[i].Stars;
		}

		CurrentProfileSettings->TotalChallengeStars = TotalStars;
		SaveProfileSettings();

		FText ChallengeToast = FText::Format(NSLOCTEXT("Challenge", "GainedStars", "Challenge Completed!  You earned {0} stars."), FText::AsNumber(Stars));
		ShowToast(ChallengeToast);
		if (TotalStars / 5 != (TotalStars - Stars) / 5)
		{
			FText OldTeammate = FText::FromName(UUTChallengeManager::StaticClass()->GetDefaultObject<UUTChallengeManager>()->PlayerTeamRoster.Roster[(TotalStars - Stars) / 5]);
			FText NewTeammate = FText::FromName(UUTChallengeManager::StaticClass()->GetDefaultObject<UUTChallengeManager>()->PlayerTeamRoster.Roster[4 + (TotalStars - Stars) / 5]);
			FText RosterToast = FText::Format(NSLOCTEXT("Challenge", "RosterUpgrade", "Roster Upgrade!  {0} replaces {1}."), OldTeammate, NewTeammate);
			ShowToast(RosterToast);
		}
	}
}
