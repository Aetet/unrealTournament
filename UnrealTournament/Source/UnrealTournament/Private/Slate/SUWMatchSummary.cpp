// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "../Public/UnrealTournament.h"
#include "SUWMatchSummary.h"
#include "SUWindowsStyle.h"
#include "SUWWeaponConfigDialog.h"
#include "SNumericEntryBox.h"
#include "../Public/UTCanvasRenderTarget2D.h"
#include "EngineModule.h"
#include "SlateMaterialBrush.h"
#include "../Public/UTPlayerCameraManager.h"
#include "UTCharacterContent.h"
#include "UTWeap_ShockRifle.h"
#include "UTWeaponAttachment.h"
#include "UTHUD.h"
#include "Engine/UserInterfaceSettings.h"
#include "UTGameEngine.h"
#include "Animation/SkeletalMeshActor.h"
#include "UTCTFGameState.h"
#include "UTCTFGameMode.h"
#include "UTPlayerState.h"
#include "Private/Slate/Widgets/SUTTabWidget.h"
#include "Panels/SUInGameHomePanel.h"
#include "UTConsole.h"
#include "UTHUDWidgetMessage.h"
#include "UTHUDWidgetMessage_GameMessages.h"
#include "Widgets/SUTXPBar.h"

#if !UE_SERVER
#include "Runtime/AppFramework/Public/Widgets/Colors/SColorPicker.h"
#endif

#include "AssetData.h"

static const float PLAYER_SPACING = 180.0f;
static const float MIN_TEAM_SPACING = 120.f;
static const float TEAM_CAMERA_OFFSET = 500.0f;
static const float TEAM_CAMERA_ZOFFSET = 50.0f;
static const float ALL_CAMERA_OFFSET = 400.0f;
static const float ALL_CAMERA_ANGLE = -5.0f;
static const float TEAMANGLE = 12.0f;

#if !UE_SERVER

#include "SScaleBox.h"
#include "Widgets/SDragImage.h"

void FTeamCamera::InitCam(class SUWMatchSummary* MatchWidget)
{
	CamFlags |= CF_ShowSwitcher | CF_ShowPlayerNames | CF_Team;
	MatchWidget->ViewedTeamNum = TeamNum;
	MatchWidget->GetTeamCamTransforms(TeamNum, CamStart, CamEnd);
	MatchWidget->ShowTeam(MatchWidget->ViewedTeamNum);
	MatchWidget->TeamCamAlpha = 0.5f;
}

void FCharacterCamera::InitCam(class SUWMatchSummary* MatchWidget)
{
	CamFlags |= CF_ShowSwitcher | CF_ShowInfoWidget | CF_Player;
	if (Character.IsValid())
	{
		MatchWidget->ViewedChar = Character;
		MatchWidget->ViewedTeamNum = Character->GetTeamNum() != 255 ? Character->GetTeamNum() : 0;
		MatchWidget->ShowCharacter(Character.Get());
		MatchWidget->FriendStatus = NAME_None;
		MatchWidget->BuildInfoPanel();

		FRotator Dir = Character->GetActorRotation();
		FVector Location = Character->GetActorLocation() + (Dir.Vector() * 300.0f);
		Location += Dir.Quaternion().GetAxisY() * -60.0f + FVector(0.0f, 0.0f, 45.0f);
		Dir.Yaw += 180.0f;

		CameraTransform.SetLocation(Location);
		CameraTransform.SetRotation(Dir.Quaternion());
	}
}

void FAllCamera::InitCam(class SUWMatchSummary* MatchWidget)
{
	CamFlags |= CF_All;
	float CameraOffset = MatchWidget->GetAllCameraOffset();
	CameraTransform.SetLocation(FVector(CameraOffset, 0.0f, -1.f * CameraOffset * FMath::Sin(ALL_CAMERA_ANGLE * PI / 180.f)));
	CameraTransform.SetRotation(FRotator(ALL_CAMERA_ANGLE, 180.0f, 0.0f).Quaternion());
	MatchWidget->ShowAllCharacters();
}

void SUWMatchSummary::Construct(const FArguments& InArgs)
{
	FVector2D ViewportSize;
	InArgs._PlayerOwner->ViewportClient->GetViewportSize(ViewportSize);

	PlayerOwner = InArgs._PlayerOwner;
	GameState = InArgs._GameState;

	StatsWidth = 0.0f;
	LastChatCount = 0;

	ShotStartTime = 0.0f;
	CurrentShot = 0;

	ViewedTeamNum = 0;
	CameraTransform.SetLocation(FVector(5000.0f, 0.0f, 35.0f));
	CameraTransform.SetRotation(FRotator(0.0f, 180.0f, 0.0f).Quaternion());
	//DesiredCameraTransform = CameraTransform;
	TeamCamAlpha = 0.5f;
	bAutoScrollTeam = true;
	AutoScrollTeamDirection = 1.0f;

	// allocate a preview scene for rendering
	PlayerPreviewWorld = UWorld::CreateWorld(EWorldType::Game, true); // NOTE: Custom depth does not work with EWorldType::Preview
	PlayerPreviewWorld->bHack_Force_UsesGameHiddenFlags_True = true;
	PlayerPreviewWorld->bShouldSimulatePhysics = true;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(PlayerPreviewWorld);
	PlayerPreviewWorld->InitializeActorsForPlay(FURL(), true);
	ViewState.Allocate();
	{
		UClass* EnvironmentClass = LoadObject<UClass>(nullptr, TEXT("/Game/RestrictedAssets/UI/PlayerPreviewEnvironment.PlayerPreviewEnvironment_C"));
		PreviewEnvironment = PlayerPreviewWorld->SpawnActor<AActor>(EnvironmentClass, FVector(500.f, 50.f, 0.f), FRotator(0, 0, 0));
	}

	//Spawn a gamestate and add the taccom overlay so we can outline the character on mouse over
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.ObjectFlags |= RF_Transient;
	AUTGameState* NewGS = PlayerPreviewWorld->SpawnActor<AUTGameState>(GameState->GetClass(), SpawnInfo);
	if (NewGS != nullptr)
	{
		TSubclassOf<class AUTCharacter> DefaultPawnClass = Cast<UClass>(StaticLoadObject(UClass::StaticClass(), NULL, *GetDefault<AUTGameMode>()->PlayerPawnObject.ToStringReference().AssetLongPathname, NULL, LOAD_NoWarn));
		if (DefaultPawnClass != nullptr)
		{
			NewGS->AddOverlayMaterial(DefaultPawnClass.GetDefaultObject()->TacComOverlayMaterial);
		}
	}
	
	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(NULL, TEXT("/Game/RestrictedAssets/UI/PlayerPreviewProxy.PlayerPreviewProxy"));
	if (BaseMat != NULL)
	{
		PlayerPreviewTexture = Cast<UUTCanvasRenderTarget2D>(UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(GetPlayerOwner().Get(), UUTCanvasRenderTarget2D::StaticClass(), ViewportSize.X, ViewportSize.Y));
		PlayerPreviewTexture->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
		PlayerPreviewTexture->OnNonUObjectRenderTargetUpdate.BindSP(this, &SUWMatchSummary::UpdatePlayerRender);
		PlayerPreviewMID = UMaterialInstanceDynamic::Create(BaseMat, PlayerPreviewWorld);
		PlayerPreviewMID->SetTextureParameterValue(FName(TEXT("TheTexture")), PlayerPreviewTexture);
		PlayerPreviewBrush = new FSlateMaterialBrush(*PlayerPreviewMID, ViewportSize);
	}
	else
	{
		PlayerPreviewTexture = NULL;
		PlayerPreviewMID = NULL;
		PlayerPreviewBrush = new FSlateMaterialBrush(*UMaterial::GetDefaultMaterial(MD_Surface), ViewportSize);
	}

	PlayerPreviewTexture->TargetGamma = GEngine->GetDisplayGamma();
	PlayerPreviewTexture->InitCustomFormat(ViewportSize.X, ViewportSize.Y, PF_B8G8R8A8, false);
	PlayerPreviewTexture->UpdateResourceImmediate();

	FVector2D ResolutionScale(ViewportSize.X / 1280.0f, ViewportSize.Y / 720.0f);

	UUTGameUserSettings* Settings = Cast<UUTGameUserSettings>(GEngine->GetGameUserSettings());

	ChildSlot
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[

		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SScaleBox)
				.Stretch(EStretch::ScaleToFill)
				[
					SNew(SDragImage)
					.Image(PlayerPreviewBrush)
					.OnDrag(this, &SUWMatchSummary::DragPlayerPreview)
					.OnZoom(this, &SUWMatchSummary::ZoomPlayerPreview)
					.OnMove(this, &SUWMatchSummary::OnMouseMovePlayerPreview)
					.OnMousePressed(this, &SUWMatchSummary::OnMouseDownPlayerPreview)
				]
			]
			+ SOverlay::Slot()
			.VAlign(VAlign_Bottom)
			.HAlign(HAlign_Left)
			[
				SNew(SBox)
				.HeightOverride(140.0f)
				.WidthOverride(600.0f)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SBorder)
						.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.95f))
						.BorderImage(SUWindowsStyle::Get().GetBrush("UT.Background.Dark"))
						.Padding(0)
					]
					+ SOverlay::Slot()
					[
						SAssignNew(ChatScroller, SScrollBox)
						+ SScrollBox::Slot()
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.VAlign(VAlign_Bottom)
							.AutoHeight()
							[
								SAssignNew(ChatDisplay, SRichTextBlock)
								.TextStyle(SUWindowsStyle::Get(), "UWindows.Chat.Text.Global")
								.Justification(ETextJustify::Left)
								.DecoratorStyleSet(&SUWindowsStyle::Get())
								.AutoWrapText(true)
							]
						]
					]
				]
			]
			+ SOverlay::Slot()
			.VAlign(VAlign_Fill)
			.HAlign(HAlign_Fill)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Bottom)
				.HAlign(HAlign_Center)
				.Padding(0.0f, 0.0f, 0.0f, 160.0f)
				[
					SNew(SBorder)
					.BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f))
					.BorderImage(SUWindowsStyle::Get().GetBrush("UT.Background.Dark"))
					.Padding(0)
					.Visibility(this, &SUWMatchSummary::GetSwitcherVisibility)
					.Content()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.Padding(20.0f, 0.0f, 0.0f, 0.0f)
						.AutoWidth()
						[
							SNew(SButton)
							.HAlign(HAlign_Left)
							.ButtonStyle(SUWindowsStyle::Get(), "UT.BottomMenu.Button")
							.ContentPadding(FMargin(20.0f, 5.0f, 20.0f, 5.0f))
							.Text(NSLOCTEXT("SUWMatchSummary", "PreviousPlayer", "<-"))
							.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
							.OnClicked(this, &SUWMatchSummary::OnSwitcherNext)
							.Visibility(this, &SUWMatchSummary::GetSwitcherButtonVisibility)
						]
						+ SHorizontalBox::Slot()
						.HAlign(HAlign_Fill)
						.FillWidth(1.0f)
						[
							SNew(SBox)
							.VAlign(VAlign_Center)
							.HAlign(HAlign_Center)
							[
								SNew(STextBlock)
								.Text(this, &SUWMatchSummary::GetSwitcherText)
								.ColorAndOpacity(this, &SUWMatchSummary::GetSwitcherColor)
								.TextStyle(SUWindowsStyle::Get(), "UT.Dialog.TitleTextStyle")
							]
						]
						+ SHorizontalBox::Slot()
						.Padding(0.0f, 0.0f, 20.0f, 0.0f)
						.AutoWidth()
						[
							SNew(SButton)
							.HAlign(HAlign_Right)
							.ButtonStyle(SUWindowsStyle::Get(), "UT.BottomMenu.Button")
							.ContentPadding(FMargin(20.0f, 5.0f, 20.0f, 5.0f))
							.Text(NSLOCTEXT("SUWMatchSummary", "NextPlayer", "->"))
							.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
							.OnClicked(this, &SUWMatchSummary::OnSwitcherPrevious)
							.Visibility(this, &SUWMatchSummary::GetSwitcherButtonVisibility)
						]
					]
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Fill)
				.HAlign(HAlign_Right)
				.Padding(0.0f)
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(TAttribute<FOptionalSize>::Create(TAttribute<FOptionalSize>::FGetter::CreateSP(this, &SUWMatchSummary::GetStatsWidth)))
					.Content()
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							SNew(SBorder)
						]
						+ SOverlay::Slot()
						[
							SNew(SImage)
							.Image(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
							.ColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f))
						]
						+ SOverlay::Slot()
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.VAlign(VAlign_Fill)
							[
								SAssignNew(InfoPanel, SOverlay)
							]
							+ SVerticalBox::Slot()
							.Padding(FMargin(5.0f, 5.0f, 5.0f, 5.0f))
							.AutoHeight()
							[
								SAssignNew(XPOverlay, SOverlay)
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(SBox)
								.MaxDesiredHeight(2.0f)
								.WidthOverride(1000.0f)
								.Content()
								[
									SNew(SImage)
									.Image(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
									.ColorAndOpacity(FLinearColor(0.4f, 0.4f, 0.4f, 0.8f))
								]
							]
							+ SVerticalBox::Slot()
							.Padding(FMargin(5.0f, 5.0f, 5.0f, 5.0f))
							.AutoHeight()
							[
								SAssignNew(FriendPanel, SHorizontalBox)
							]
						]
					]
				]
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SAssignNew(ChatPanel, SUInGameHomePanel, GetPlayerOwner())
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					SNew(SImage)
					.Image(SUWindowsStyle::Get().GetBrush("UT.ChatBar.Fill"))
				]
				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.HAlign(HAlign_Right)
					.ButtonStyle(SUWindowsStyle::Get(), "UT.BottomMenu.Button")
					.ContentPadding(FMargin(20.0f, 5.0f, 20.0f, 5.0f))
					.Text(NSLOCTEXT("SUWMatchSummary", "Close", "Close"))
					.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
					.OnClicked(this, &SUWMatchSummary::OnClose)
				]
			]
		]
	];
	RecreateAllPlayers();

	//Set the camera state based on the game state
	if (GameState.IsValid())
	{
		if (!GameState->HasMatchStarted())
		{
			SetupIntroCam();
		}
		//View the winning team at the end of game
		else if(GameState->GetMatchState() == MatchState::WaitingPostMatch)
		{
			SetupMatchCam();
		}
		else if (GameState->GetMatchState() == MatchState::MatchEnteringHalftime
			|| GameState->GetMatchState() == MatchState::MatchIsAtHalftime)
		{
			//Reset the scoreboard page and timers
			UUTScoreboard* Scoreboard = GetScoreboard();
			if (Scoreboard != nullptr)
			{
				Scoreboard->SetPage(0);
				Scoreboard->SetScoringPlaysTimer(true);
			}
			ViewAll();
		}
		else
		{
			int32 TeamToView = 0;
			if (GetPlayerOwner().IsValid() && Cast<AUTPlayerController>(GetPlayerOwner()->PlayerController) != nullptr)
			{
				TeamToView = Cast<AUTPlayerController>(GetPlayerOwner()->PlayerController)->GetTeamNum();
			}
			ViewTeam(TeamToView);
		}
	}

	// Turn on Screen Space Reflection max quality
	auto SSRQualityCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.SSR.Quality"));
	OldSSRQuality = SSRQualityCVar->GetInt();
	SSRQualityCVar->Set(4, ECVF_SetByCode);
}

SUWMatchSummary::~SUWMatchSummary()
{
	// Reset Screen Space Reflection max quality, wish there was a cleaner way to reset the flags
	auto SSRQualityCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.SSR.Quality"));
	EConsoleVariableFlags Flags = SSRQualityCVar->GetFlags();
	Flags = (EConsoleVariableFlags)(((uint32)Flags & ~ECVF_SetByMask) | ECVF_SetByScalability);
	SSRQualityCVar->Set(OldSSRQuality, ECVF_SetByCode);
	SSRQualityCVar->SetFlags(Flags);

	if (!GExitPurge)
	{
		if (PlayerPreviewTexture != NULL)
		{
			PlayerPreviewTexture->OnNonUObjectRenderTargetUpdate.Unbind();
			PlayerPreviewTexture = NULL;
		}
		FlushRenderingCommands();
		if (PlayerPreviewBrush != NULL)
		{
			// FIXME: Slate will corrupt memory if this is deleted. Must be referencing it somewhere that doesn't get cleaned up...
			//		for now, we'll take the minor memory leak (the texture still gets GC'ed so it's not too bad)
			//delete PlayerPreviewBrush;
			PlayerPreviewBrush->SetResourceObject(NULL);
			PlayerPreviewBrush = NULL;
		}
		if (PlayerPreviewWorld != NULL)
		{
			PlayerPreviewWorld->DestroyWorld(true);
			GEngine->DestroyWorldContext(PlayerPreviewWorld);
			PlayerPreviewWorld = NULL;
		}
	}
	ViewState.Destroy();
}

void SUWMatchSummary::OnTabButtonSelectionChanged(const FText& NewText)
{
	static const FText Score = NSLOCTEXT("AUTGameMode", "Score", "Score");
	static const FText Weapons = NSLOCTEXT("AUTGameMode", "Weapons", "Weapons");
	static const FText Rewards = NSLOCTEXT("AUTGameMode", "Rewards", "Rewards");
	static const FText Movement = NSLOCTEXT("AUTGameMode", "Movement", "Movement");

	AUTPlayerController* UTPC = Cast<AUTPlayerController>(GetPlayerOwner()->PlayerController);
	AUTPlayerState* PS = ViewedChar.IsValid() ? Cast<AUTPlayerState>(ViewedChar->PlayerState) : nullptr;
	if (UTPC != nullptr && PS != nullptr && !PS->IsPendingKill() && PS->IsValidLowLevel())
	{
		if (NewText.EqualTo(Score))
		{
			UTPC->ServerSetViewedScorePS(PS, 0);
		}
		else if (NewText.EqualTo(Weapons))
		{
			UTPC->ServerSetViewedScorePS(PS, 1);
		}
		else if (NewText.EqualTo(Rewards))
		{
			UTPC->ServerSetViewedScorePS(PS, 2);
		}
		else if (NewText.EqualTo(Movement))
		{
			UTPC->ServerSetViewedScorePS(PS, 3);
		}
		else
		{
			UTPC->ServerSetViewedScorePS(nullptr, 0);
		}
	}
}

void SUWMatchSummary::BuildInfoPanel()
{
	StatList.Empty();
	InfoPanel->ClearChildren();

	InfoPanel->AddSlot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SAssignNew(TabWidget, SUTTabWidget)
			.OnTabButtonSelectionChanged(this, &SUWMatchSummary::OnTabButtonSelectionChanged)
			.TabTextStyle(SUWindowsStyle::Get(), "UT.MatchSummary.TabButton.TextStyle")
		];

	//Build all of the player stats
	AUTPlayerState* UTPS = ViewedChar.IsValid() ? Cast<AUTPlayerState>(ViewedChar->PlayerState) : nullptr;
	if (UTPS != nullptr && !UTPS->IsPendingKill() && UTPS->IsValidLowLevel())
	{
		AUTGameMode* DefaultGameMode = GameState.IsValid() && GameState->GameModeClass ? Cast<AUTGameMode>(GameState->GameModeClass->GetDefaultObject()) : NULL;
		if (DefaultGameMode != nullptr)
		{
			//Build the highlights
			if (GameState->HasMatchStarted())
			{
				TSharedPtr<SVerticalBox> VBox;
				TabWidget->AddTab(NSLOCTEXT("AUTGameMode", "Highlights", "Highlights"),
					SNew(SOverlay)
					+ SOverlay::Slot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Fill)
					[
						SAssignNew(VBox, SVerticalBox)
					]);

				TArray<FText> Highlights = GameState->GetPlayerHighlights(UTPS);

				//Cap at 5 highlights
				if (Highlights.Num() > 5)
				{
					Highlights.SetNum(5);
				}

				for (int32 i = 0; i < Highlights.Num(); i++)
				{
					VBox->AddSlot()
						.Padding(100, 20)
						.AutoHeight()
						[
							SNew(SBorder)
							.BorderImage(SUWindowsStyle::Get().GetBrush("UT.MatchSummary.Highlight.Border"))
							.Padding(2)
							.Content()
							[
								SNew(SBox)
								.MinDesiredHeight(100.0f)
								.Content()
								[
									SNew(SOverlay)
									+ SOverlay::Slot()
									[
										SNew(SImage)
										.Image(SUWindowsStyle::Get().GetBrush("UT.MatchSummary.Highlight.BG"))
									]
									+ SOverlay::Slot()
										.VAlign(VAlign_Center)
										.HAlign(HAlign_Fill)
										[
											SNew(SRichTextBlock)
											.Text(Highlights[i])
											.TextStyle(SUWindowsStyle::Get(), "UT.MatchSummary.HighlightText.Normal")
											.Justification(ETextJustify::Center)
											.DecoratorStyleSet(&SUWindowsStyle::Get())
											.AutoWrapText(false)
										]
								]
							]
						];
				}
			}

			UTPS->BuildPlayerInfo(TabWidget, StatList);

			//Build the player stats only if the match has started
			if (GameState->HasMatchStarted())
			{
				DefaultGameMode->BuildPlayerInfo(UTPS, TabWidget, StatList);
			}

			TabWidget->SelectTab(0);
		}
	}
}

void SUWMatchSummary::UpdateChatText()
{
	if (PlayerOwner->ChatArchive.Num() != LastChatCount)
	{
		LastChatCount = PlayerOwner->ChatArchive.Num();

		FString RichText = TEXT("");
		for (int32 i = 0; i<PlayerOwner->ChatArchive.Num(); i++)
		{
			TSharedPtr<FStoredChatMessage> Msg = PlayerOwner->ChatArchive[i];
			if (i>0) RichText += TEXT("\n");
			if (Msg->Type != ChatDestinations::MOTD)
			{
				FString Style;
				FString PlayerName = Msg->Sender + TEXT(": ");

				if (Msg->Type == ChatDestinations::Friends)
				{
					Style = TEXT("UWindows.Chat.Text.Friends");
				}
				else if (Msg->Type == ChatDestinations::System || Msg->Type == ChatDestinations::MOTD)
				{
					Style = TEXT("UWindows.Chat.Text.Admin");
					PlayerName.Empty(); //Don't show player name for system messages
				}
				else if (Msg->Type == ChatDestinations::Lobby)
				{
					Style = TEXT("UWindows.Chat.Text.Lobby");
				}
				else if (Msg->Type == ChatDestinations::Match)
				{
					Style = TEXT("UWindows.Chat.Text.Match");
				}
				else if (Msg->Type == ChatDestinations::Local)
				{
					Style = TEXT("UWindows.Chat.Text.Local");
				}
				else if (Msg->Type == ChatDestinations::Team)
				{
					if (Msg->Color.R > Msg->Color.B)
					{
						Style = TEXT("UWindows.Chat.Text.Team.Red");
					}
					else
					{
						Style = TEXT("UWindows.Chat.Text.Team.Blue");
					}
				}
				else
				{
					Style = TEXT("UWindows.Chat.Text.Global");
				}

				RichText += FString::Printf(TEXT("<%s>%s%s</>"), *Style, *PlayerName, *Msg->Message);
			}
			else
			{
				RichText += Msg->Message;
			}
		}

		ChatDisplay->SetText(FText::FromString(RichText));
		ChatScroller->SetScrollOffset(290999);
	}
}

void SUWMatchSummary::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(PlayerPreviewTexture);
	Collector.AddReferencedObject(PlayerPreviewMID);
	Collector.AddReferencedObject(PlayerPreviewWorld);
}

void SUWMatchSummary::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (GameState.IsValid() && (GameState->GetMatchState() == MatchState::WaitingToStart))
	{
		// recreate players if something has changed
		bool bPlayersAreValid = true;
		int32 TotalPlayers = 0;

		// @TODO FIXMESTEVE - this could be reported to match summary on valid change, rather than checking every tick
		for (int32 iTeam = 0; iTeam < TeamPreviewMeshs.Num(); iTeam++)
		{
			TArray<AUTCharacter*> &TeamCharacters = TeamPreviewMeshs[iTeam];
			for (int32 iPlayer = 0; iPlayer < TeamCharacters.Num(); iPlayer++)
			{
				AUTPlayerState* PS = (TeamCharacters[iPlayer] && TeamCharacters[iPlayer]->PlayerState && !TeamCharacters[iPlayer]->PlayerState->IsPendingKillPending()) ? Cast<AUTPlayerState>(TeamCharacters[iPlayer]->PlayerState) : NULL;
				if (!PS || (GameState->bTeamGame && (!PS->Team || (PS->Team->TeamIndex != iTeam))))
				{
					bPlayersAreValid = false;
					break;
				}
				TotalPlayers++;
			}
		}
		if (TotalPlayers != GameState->PlayerArray.Num())
		{
			bPlayersAreValid = false;
		}
		if (!bPlayersAreValid)
		{
			RecreateAllPlayers();
			if (HasCamFlag(CF_All))
			{
				ViewAll();
			}
		}
	}
	if (PlayerPreviewWorld != nullptr)
	{
		PlayerPreviewWorld->Tick(LEVELTICK_All, InDeltaTime);
	}

	if ( PlayerPreviewTexture != nullptr )
	{
		PlayerPreviewTexture->UpdateResource();
	}

	UpdateChatText();
	BuildFriendPanel();

	if (CameraShots.IsValidIndex(CurrentShot) && GameState.IsValid())
	{
		float ElapsedTime = GameState->GetWorld()->RealTimeSeconds - ShotStartTime;
		if (CameraShots[CurrentShot]->TickCamera(this, ElapsedTime, InDeltaTime, CameraTransform))
		{
			SetCamShot(CurrentShot+1);
		}
	}

	//Create the xp widget the first time the info widget is open
	AUTPlayerState* PS = (PlayerOwner.IsValid() && PlayerOwner->PlayerController != nullptr) ? Cast<AUTPlayerState>(PlayerOwner->PlayerController->PlayerState) : nullptr;
	if (!XPBar.IsValid() && HasCamFlag(CF_ShowInfoWidget) && GameState.IsValid() && GameState->GetMatchState() == MatchState::WaitingPostMatch && (PS != nullptr && PS->CanAwardOnlineXP()))
	{
		XPOverlay->AddSlot()
		[
			SAssignNew(XPBar, SUTXPBar).PlayerOwner(PlayerOwner)
		];
	}
}

void SUWMatchSummary::SetCamShot(int32 ShotIndex)
{
	if (CameraShots.IsValidIndex(ShotIndex) && GameState.IsValid())
	{
		CurrentShot = ShotIndex;
		ShotStartTime = GameState->GetWorld()->RealTimeSeconds;
		CameraShots[CurrentShot]->InitCam(this);

		//Make sure all the characters are rotated in their proper team rotation
		for (int32 iTeam = 0; iTeam < TeamPreviewMeshs.Num(); iTeam++)
		{
			for (int32 iCharacter = 0; iCharacter < TeamPreviewMeshs[iTeam].Num(); iCharacter++)
			{
				if (TeamAnchors.IsValidIndex(iTeam))
				{
					TeamPreviewMeshs[iTeam][iCharacter]->SetActorRotation(TeamAnchors[iTeam]->GetActorRotation());
				}
			}
		}
	}
}

void SUWMatchSummary::SetupIntroCam()
{
	CameraShots.Empty();

	//Start with viewing team
	TSharedPtr<FAllCamera> AllCam = MakeShareable(new FAllCamera);
	AllCam->CamFlags |= CF_Intro;
	AllCam->Time = 0.1f;
	CameraShots.Add(AllCam);

	//View teams
	int32 NumViewTeams = GameState->Teams.Num();
	if (NumViewTeams == 0)
	{
		NumViewTeams = 1;
	}
	//6 seconds for the team camera pan works well with the current song
	float TimePerTeam = 6.0f / NumViewTeams;

	//Add camera pan for each team
	for (int32 i = 0; i < NumViewTeams; i++)
	{
		TSharedPtr<FTeamCamera> TeamCam = MakeShareable(new FTeamCamera(i));
		TeamCam->Time = TimePerTeam;
		TeamCam->CamFlags |= CF_ShowPlayerNames | CF_Intro;
		CameraShots.Add(TeamCam);
	}
	SetCamShot(0);

	//Play the intro music
	AUTPlayerController* UTPC = Cast<AUTPlayerController>(GetPlayerOwner()->PlayerController);
	USoundBase* Music = LoadObject<USoundBase>(NULL, TEXT("/Game/RestrictedAssets/Audio/Music/FragCenterIntro.FragCenterIntro"), NULL, LOAD_NoWarn | LOAD_Quiet);
	if (UTPC != nullptr && Music != nullptr)
	{
		UTPC->ClientPlaySound(Music);
	}
}

void SUWMatchSummary::GetTeamCamTransforms(int32 TeamNum, FTransform& Start, FTransform& End)
{
	if (TeamPreviewMeshs.IsValidIndex(TeamNum))
	{
		TArray<AUTCharacter*> &TeamCharacters = TeamPreviewMeshs[TeamNum];
		if (TeamCharacters.Num() > 0)
		{
			AUTCharacter* StartChar = TeamCharacters[TeamCharacters.Num() - 1];
			FRotator Dir = StartChar->GetActorRotation();
			FVector Location = StartChar->GetActorLocation() + (Dir.Vector() * TEAM_CAMERA_OFFSET) + FVector(0.0f, 0.0f, TEAM_CAMERA_ZOFFSET);

			Dir.Yaw += 180.0f;
			Start.SetLocation(Location);
			Start.SetRotation(Dir.Quaternion());
		}
		if (TeamCharacters.Num() > 0)
		{
			AUTCharacter* EndChar = TeamCharacters[0];
			FRotator Dir = EndChar->GetActorRotation();
			FVector Location = EndChar->GetActorLocation() + (Dir.Vector() * TEAM_CAMERA_OFFSET) + FVector(0.0f, 0.0f, TEAM_CAMERA_ZOFFSET);

			Dir.Yaw += 180.0f;
			End.SetLocation(Location);
			End.SetRotation(Dir.Quaternion());
		}
	}
}

void SUWMatchSummary::SetupMatchCam()
{
	int32 TeamToView = 0;
	if (GameState.IsValid() && GameState->GetMatchState() == MatchState::WaitingPostMatch && GameState->WinningTeam != nullptr)
	{
		TeamToView = GameState->WinningTeam->GetTeamNum();
	}
	else if (GetPlayerOwner().IsValid() && Cast<AUTPlayerController>(GetPlayerOwner()->PlayerController) != nullptr)
	{
		TeamToView = Cast<AUTPlayerController>(GetPlayerOwner()->PlayerController)->GetTeamNum();
	}
	if (TeamToView == 255)
	{
		TeamToView = 0;
	}

	CameraShots.Empty();
	TSharedPtr<FTeamCamera> TeamCam = MakeShareable(new FTeamCamera(TeamToView));
	TeamCam->Time = 0.5f;
	CameraShots.Add(TeamCam);

	if (TeamPreviewMeshs.IsValidIndex(TeamToView) && GameState.IsValid())
	{
		float DisplayTime = 3.f;
		AUTGameMode* DefaultGame = GameState->GameModeClass->GetDefaultObject<AUTGameMode>();
		if (DefaultGame)
		{
			DisplayTime = DefaultGame->WinnerSummaryDisplayTime;
		}

		TArray<AUTCharacter*> &TeamCharacters = TeamPreviewMeshs[TeamToView];
		int32 NumWinnersToShow = FMath::Min(int32(GameState->NumWinnersToShow), TeamCharacters.Num());

		// determine match highlight scores to use for sorting
		for (int32 i = 0; i < TeamCharacters.Num(); i++)
		{
			AUTPlayerState* PS = TeamCharacters[i] ? Cast<AUTPlayerState>(TeamCharacters[i]->PlayerState) : NULL;
			if (PS)
			{
				PS->MatchHighlightScore = GameState->MatchHighlightScore(PS);
			}
		}

		// sort winners
		bool(*SortFunc)(const AUTCharacter&, const AUTCharacter&);
		SortFunc = [](const AUTCharacter& A, const AUTCharacter& B)
		{
			AUTPlayerState* PSA = Cast<AUTPlayerState>(A.PlayerState);
			AUTPlayerState* PSB = Cast<AUTPlayerState>(B.PlayerState);
			return !PSB || (PSA && (PSA->MatchHighlightScore > PSB->MatchHighlightScore));
		};
		TeamCharacters.Sort(SortFunc);

		// add winner shots
		for (int32 i = 0; i < NumWinnersToShow; i++)
		{
			TSharedPtr<FCharacterCamera> PlayerCam = MakeShareable(new FCharacterCamera(TeamCharacters[i]));
			PlayerCam->Time = DisplayTime;
			CameraShots.Add(PlayerCam);
		}
	}

	// finally go to local player view
	if (GetPlayerOwner().IsValid() && Cast<AUTPlayerController>(GetPlayerOwner()->PlayerController) != nullptr)
	{
		AUTPlayerState* LocalPS = Cast<AUTPlayerState>(GetPlayerOwner()->PlayerController->PlayerState);
		AUTCharacter* LocalChar = NULL;
		if (LocalPS)
		{
			int32 LocalTeam = LocalPS->GetTeamNum();
			if (LocalTeam == 255)
			{
				LocalTeam = 0;
			}
			if (LocalTeam < TeamPreviewMeshs.Num())
			{
				TArray<AUTCharacter*> &TeamCharacters = TeamPreviewMeshs[LocalTeam];
				for (int32 iPlayer = 0; iPlayer < TeamCharacters.Num(); iPlayer++)
				{
					APlayerState* PS = (TeamCharacters[iPlayer] && TeamCharacters[iPlayer]->PlayerState && !TeamCharacters[iPlayer]->PlayerState->IsPendingKillPending()) ? TeamCharacters[iPlayer]->PlayerState : NULL;
					if (PS == LocalPS)
					{
						LocalChar = TeamCharacters[iPlayer];
						break;
					}
				}
			}
		}
		if (LocalChar)
		{
			TSharedPtr<FCharacterCamera> PlayerCam = MakeShareable(new FCharacterCamera(LocalChar));
			PlayerCam->Time = 99.0f;
			CameraShots.Add(PlayerCam);
		}
	}
	SetCamShot(0);
}

void SUWMatchSummary::RecreateAllPlayers()
{
	//Destroy all the characters and their weapons
	for (auto Weapon : PreviewWeapons)
	{
		Weapon->Destroy();
	}
	PreviewWeapons.Empty();

	for (auto Char : PlayerPreviewMeshs)
	{
		Char->Destroy();
	}
	PlayerPreviewMeshs.Empty();
	TeamPreviewMeshs.Empty();
	for (auto Flag : PreviewFlags)
	{
		Flag->Destroy();
	}
	PreviewFlags.Empty();
	for (auto Anchor : TeamAnchors)
	{
		Anchor->Destroy();
	}
	TeamAnchors.Empty();

	//Gather All of the playerstates
	TArray<TArray<class AUTPlayerState*> > TeamPlayerStates;
	if (GameState.IsValid())
	{
		for (TActorIterator<AUTPlayerState> It(GameState->GetWorld()); It; ++It)
		{
			AUTPlayerState* PS = *It;

			if (!PS->bOnlySpectator)
			{
				int32 TeamNum = PS->GetTeamNum() == 255 ? 0 : PS->GetTeamNum();
				if (!TeamPlayerStates.IsValidIndex(TeamNum))
				{
					TeamPlayerStates.SetNum(TeamNum + 1);
				}
				TeamPlayerStates[TeamNum].Add(PS);
			}
		}
	}

	for (int32 iTeam = 0; iTeam < TeamPlayerStates.Num(); iTeam++)
	{
		FVector PlayerLocation(0.0f, 0.0f, 0.0f);
		FVector TeamCenter = FVector(0.0f,(TeamPlayerStates[iTeam].Num() - 1) * PLAYER_SPACING * 0.5f, 0.0f);

		//Create an actor that all team actors will attach to for easy team manipulation
		AActor* TeamAnchor = PlayerPreviewWorld->SpawnActor<AActor>(ATargetPoint::StaticClass(), TeamCenter, FRotator::ZeroRotator);
		if (TeamAnchor != nullptr)
		{
			TeamAnchors.Add(TeamAnchor);

			//Spawn all of the characters for this team
			for (int32 iPlayer = 0; iPlayer < TeamPlayerStates[iTeam].Num(); iPlayer++)
			{
				AUTPlayerState* PS = TeamPlayerStates[iTeam][iPlayer];
				// slightly oppose rotation imposed by teamplayerstate
				FRotator PlayerRotation = FRotator(0.f); //(iTeam == 0) ? FRotator(0.f, 0.5f * (90.f - TEAMANGLE), 0.0f) : FRotator(0, 0.5f * (TEAMANGLE - 90.f), 0.0f);
				AUTCharacter* NewCharacter = RecreatePlayerPreview(PS, PlayerLocation, PlayerRotation);
				NewCharacter->AttachRootComponentToActor(TeamAnchor, NAME_None, EAttachLocation::KeepWorldPosition);

				//Add the character to the team list
				if (!TeamPreviewMeshs.IsValidIndex(iTeam))
				{
					TeamPreviewMeshs.SetNum(iTeam + 1);
				}
				TeamPreviewMeshs[iTeam].Add(NewCharacter);
				PlayerLocation.Y += PLAYER_SPACING;
			}
		}
	}

	//Move the teams into position
	if (TeamAnchors.Num() == 1)
	{
		TeamAnchors[0]->SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	}
	else if (TeamAnchors.Num() == 2)
	{
		//Angle the teams a bit.
		float YDistPerPlayer = 0.5f * PLAYER_SPACING * FMath::Sin(TEAMANGLE * PI / 180.f);
		float Dist = MIN_TEAM_SPACING + (TeamPlayerStates[0].Num() - 1) * YDistPerPlayer;
		TeamAnchors[0]->SetActorLocationAndRotation(FVector(0.f, Dist, 0.f), FRotator(0, TEAMANGLE - 90.f, 0.0f));

		Dist = MIN_TEAM_SPACING + (TeamPlayerStates[1].Num() - 1) * YDistPerPlayer;
		TeamAnchors[1]->SetActorLocationAndRotation(FVector(0.f, -1.f*Dist, 0.f), FRotator(0.f, 90.f - TEAMANGLE, 0.0f));

		if (GameState.IsValid() && Cast<AUTCTFGameState>(GameState.Get()) != nullptr)
		{
			for (int32 iTeam = 0; iTeam < 2; iTeam++)
			{
				//Spawn a flag if its a ctf game TODO: Let the gamemode pick the mesh
				FVector FlagLocation = TeamAnchors[iTeam]->GetActorLocation() - 120.f * TeamAnchors[iTeam]->GetActorRotation().Vector();
				FlagLocation.Z = -20.f;
				FRotator FlagRotation = (iTeam == 0) ? FRotator(0.0f, 90.0f, 0.0f) : FRotator(0.0f, -90.0f, 0.0f);
				USkeletalMesh* const FlagMesh = Cast<USkeletalMesh>(StaticLoadObject(USkeletalMesh::StaticClass(), NULL, TEXT("SkeletalMesh'/Game/RestrictedAssets/Proto/UT3_Pickups/Flag/S_CTF_Flag_IronGuard.S_CTF_Flag_IronGuard'"), NULL, LOAD_None, NULL));
				ASkeletalMeshActor* NewFlag = PlayerPreviewWorld->SpawnActor<ASkeletalMeshActor>(ASkeletalMeshActor::StaticClass(), FlagLocation, FlagRotation);
				if (NewFlag != nullptr && FlagMesh != nullptr)
				{
					UMaterialInstanceConstant* FlagMat = nullptr;
					if (iTeam == 0)
					{
						FlagMat = Cast<UMaterialInstanceConstant>(StaticLoadObject(UMaterialInstanceConstant::StaticClass(), NULL, TEXT("MaterialInstanceConstant'/Game/RestrictedAssets/Proto/UT3_Pickups/Flag/MI_CTF_RedFlag.MI_CTF_RedFlag'"), NULL, LOAD_None, NULL));
					}
					else
					{
						FlagMat = Cast<UMaterialInstanceConstant>(StaticLoadObject(UMaterialInstanceConstant::StaticClass(), NULL, TEXT("MaterialInstanceConstant'/Game/RestrictedAssets/Proto/UT3_Pickups/Flag/MI_CTF_BlueFlag.MI_CTF_BlueFlag'"), NULL, LOAD_None, NULL));
					}

					NewFlag->GetSkeletalMeshComponent()->SetSkeletalMesh(FlagMesh);
					NewFlag->GetSkeletalMeshComponent()->SetMaterial(1, FlagMat);
					NewFlag->SetActorScale3D(FVector(1.8f));
					NewFlag->AttachRootComponentToActor(TeamAnchors[iTeam], NAME_None, EAttachLocation::KeepWorldPosition);
					PreviewFlags.Add(NewFlag);
				}
			}
		}
	}
}

AUTCharacter* SUWMatchSummary::RecreatePlayerPreview(AUTPlayerState* NewPS, FVector Location, FRotator Rotation)
{
	AUTWeaponAttachment* PreviewWeapon = nullptr;

	FActorSpawnParameters SpawnParams;
	SpawnParams.bNoCollisionFail = true;
	TSubclassOf<class APawn> DefaultPawnClass = Cast<UClass>(StaticLoadObject(UClass::StaticClass(), NULL, *GetDefault<AUTGameMode>()->PlayerPawnObject.ToStringReference().AssetLongPathname, NULL, LOAD_NoWarn));

	AUTCharacter* PlayerPreviewMesh = PlayerPreviewWorld->SpawnActor<AUTCharacter>(DefaultPawnClass, Location, Rotation, SpawnParams);
	
	if (PlayerPreviewMesh)
	{
		PlayerPreviewMesh->PlayerState = NewPS; //PS needed for team colors
		PlayerPreviewMesh->Health = 100; //Set to 100 so the TacCom Overlay doesn't show damage
		PlayerPreviewMesh->DeactivateSpawnProtection();

		if (NewPS->GetSelectedCharacter().GetDefaultObject()->bIsFemale)
		{
			PlayerPreviewAnimBlueprint = LoadObject<UClass>(nullptr, TEXT("/Game/RestrictedAssets/UI/ABP_Female_PlayerPreview.ABP_Female_PlayerPreview_C"));
		}
		else
		{
			PlayerPreviewAnimBlueprint = LoadObject<UClass>(nullptr, TEXT("/Game/RestrictedAssets/UI/ABP_PlayerPreview.ABP_PlayerPreview_C"));
		}

		PlayerPreviewMesh->GetMesh()->SetAnimInstanceClass(PlayerPreviewAnimBlueprint);

		PlayerPreviewMesh->ApplyCharacterData(NewPS->GetSelectedCharacter());
		PlayerPreviewMesh->NotifyTeamChanged();

		PlayerPreviewMesh->SetHatClass(NewPS->HatClass);
		PlayerPreviewMesh->SetHatVariant(NewPS->HatVariant);
		PlayerPreviewMesh->SetEyewearClass(NewPS->EyewearClass);
		PlayerPreviewMesh->SetEyewearVariant(NewPS->EyewearVariant);

		if (!PreviewWeapon)
		{
			UClass* PreviewAttachmentType = NewPS->FavoriteWeapon ? NewPS->FavoriteWeapon->GetDefaultObject<AUTWeapon>()->AttachmentType : NULL;
			if (!PreviewAttachmentType)
			{
				PreviewAttachmentType = LoadClass<AUTWeaponAttachment>(NULL, TEXT("/Game/RestrictedAssets/Weapons/ShockRifle/ShockAttachment.ShockAttachment_C"), NULL, LOAD_None, NULL);
			}
			if (PreviewAttachmentType != NULL)
			{
				PreviewWeapon = PlayerPreviewWorld->SpawnActor<AUTWeaponAttachment>(PreviewAttachmentType, FVector(0, 0, 0), FRotator(0, 0, 0));
			}
		}
		if (PreviewWeapon)
		{
			PreviewWeapon->Instigator = PlayerPreviewMesh;
			PreviewWeapon->BeginPlay();
			PreviewWeapon->AttachToOwner();
			PreviewWeapons.Add(PreviewWeapon);
		}

		PlayerPreviewMeshs.Add(PlayerPreviewMesh);
		return PlayerPreviewMesh;
	}
	return nullptr;
}

void SUWMatchSummary::UpdatePlayerRender(UCanvas* C, int32 Width, int32 Height)
{
	FEngineShowFlags ShowFlags(ESFIM_Game);
	//ShowFlags.SetLighting(false); // FIXME: create some proxy light and use lit mode
	ShowFlags.SetMotionBlur(false);
	ShowFlags.SetGrain(false);
	//ShowFlags.SetPostProcessing(false);
	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(PlayerPreviewTexture->GameThread_GetRenderTargetResource(), PlayerPreviewWorld->Scene, ShowFlags).SetRealtimeUpdate(true));

	//	EngineShowFlagOverride(ESFIM_Game, VMI_Lit, ViewFamily.EngineShowFlags, NAME_None, false);
	const float PreviewFOV = 45;
	const float AspectRatio = Width / (float)Height;

	FSceneViewInitOptions PlayerPreviewInitOptions;
	PlayerPreviewInitOptions.SetViewRectangle(FIntRect(0, 0, C->SizeX, C->SizeY));
	PlayerPreviewInitOptions.ViewOrigin = CameraTransform.GetLocation();
	PlayerPreviewInitOptions.ViewRotationMatrix = FInverseRotationMatrix(CameraTransform.Rotator());
	PlayerPreviewInitOptions.ViewRotationMatrix = PlayerPreviewInitOptions.ViewRotationMatrix * FMatrix(
		FPlane(0, 0, 1, 0),
		FPlane(1, 0, 0, 0),
		FPlane(0, 1, 0, 0),
		FPlane(0, 0, 0, 1));
	PlayerPreviewInitOptions.ProjectionMatrix =
		FReversedZPerspectiveMatrix(
		FMath::Max(0.001f, PreviewFOV) * (float)PI / 360.0f,
		AspectRatio,
		1.0f,
		GNearClippingPlane);
	PlayerPreviewInitOptions.ViewFamily = &ViewFamily;
	PlayerPreviewInitOptions.SceneViewStateInterface = ViewState.GetReference();
	PlayerPreviewInitOptions.BackgroundColor = FLinearColor::Black;
	PlayerPreviewInitOptions.WorldToMetersScale = GetPlayerOwner()->GetWorld()->GetWorldSettings()->WorldToMeters;
	PlayerPreviewInitOptions.CursorPos = FIntPoint(-1, -1);

	ViewFamily.bUseSeparateRenderTarget = true;

	FSceneView* View = new FSceneView(PlayerPreviewInitOptions); // note: renderer gets ownership
	View->ViewLocation = FVector::ZeroVector;
	View->ViewRotation = FRotator::ZeroRotator;
	FPostProcessSettings PPSettings = GetDefault<AUTPlayerCameraManager>()->DefaultPPSettings;

	ViewFamily.Views.Add(View);

	View->StartFinalPostprocessSettings(CameraTransform.GetLocation());

	//View->OverridePostProcessSettings(PPSettings, 1.0f);

	View->EndFinalPostprocessSettings(PlayerPreviewInitOptions);

	// workaround for hacky renderer code that uses GFrameNumber to decide whether to resize render targets
	--GFrameNumber;
	GetRendererModule().BeginRenderingViewFamily(C->Canvas, &ViewFamily);

	// Force the preview mesh and weapons to put the highest mips into memory if visible. This assumes each char has a weapon
	FConvexVolume Frustum;
	GetViewFrustumBounds(Frustum, PlayerPreviewInitOptions.ComputeViewProjectionMatrix(), true);
	for (auto Weapon : PreviewWeapons)
	{
		AUTCharacter* Holder = Cast<AUTCharacter>(Weapon->Instigator);
		if (Holder != nullptr)
		{
			if (!Holder->bHidden)
			{
				FVector Origin, BoxExtent;
				Holder->GetActorBounds(true, Origin, BoxExtent);

				if (Frustum.IntersectBox(Origin, BoxExtent))
				{
					Holder->PrestreamTextures(1, true);
					Weapon->PrestreamTextures(1, true);
					continue;
				}
			}
			Holder->PrestreamTextures(0, false);
			Weapon->PrestreamTextures(0, false);
		}
	}

	//Check if the mouse is over a player and apply taccom effect
	if (HasCamFlag(CF_CanInteract) && !HasCamFlag(CF_Player))
	{
		FVector Start, Direction;
		View->DeprojectFVector2D(MousePos, Start, Direction);

		FHitResult Hit;
		PlayerPreviewWorld->LineTraceSingleByChannel(Hit, Start, Start + Direction * 50000.0f, COLLISION_TRACE_WEAPON, FCollisionQueryParams(FName(TEXT("CharacterMouseOver"))));
		if (Hit.bBlockingHit && Cast<AUTCharacter>(Hit.GetActor()) != nullptr)
		{
			AUTCharacter* HitChar = Cast<AUTCharacter>(Hit.GetActor());
			if (HitChar != HighlightedChar && HighlightedChar != nullptr)
			{
				HighlightedChar->UpdateTacComMesh(false);
			}

			if (HitChar != nullptr)
			{
				HitChar->UpdateTacComMesh(true);
				HighlightedChar = HitChar;
			}
		}
		else if (HighlightedChar != nullptr)
		{
			HighlightedChar->UpdateTacComMesh(false);
			HighlightedChar = nullptr;
		}
	}

	//Draw the player names above their heads
	if (HasCamFlag(CF_ShowPlayerNames))
	{
		//Helper for making sure player names don't overlap
		//TODO: do this better. Smooth the spacing of names when they overlap
		struct FPlayerName
		{
			FPlayerName(FString InPlayerName, FVector InLocation3D, FVector2D InLocation, FVector2D InSize, UFont* InFont)
				: PlayerName(InPlayerName), Location3D(InLocation3D), Location(InLocation), Size(InSize), DrawFont(InFont) {}
			FString PlayerName;
			FVector Location3D;
			FVector2D Location;
			FVector2D Size;
			UFont* DrawFont;
		};

		UFont* SmallFont = HasCamFlag(CF_Team) ? AUTHUD::StaticClass()->GetDefaultObject<AUTHUD>()->SmallFont : AUTHUD::StaticClass()->GetDefaultObject<AUTHUD>()->TinyFont;
		UFont* SelectFont = AUTHUD::StaticClass()->GetDefaultObject<AUTHUD>()->MediumFont;
		AUTCharacter* SelectedChar = ViewedChar.IsValid() ? ViewedChar.Get() : (HighlightedChar.IsValid() ? HighlightedChar.Get() : NULL);

		//Gather all of the player names
		TArray<FPlayerName> PlayerNames;
		for (AUTCharacter* UTC : PlayerPreviewMeshs)
		{
			if (UTC->PlayerState != nullptr && FVector::DotProduct(CameraTransform.Rotator().Vector(), (UTC->GetActorLocation() - CameraTransform.GetLocation())) > 0.0f)
			{
				FVector ActorLocation = UTC->GetActorLocation() + FVector(0.0f, 0.0f, 140.0f);
				FVector2D ScreenLoc;
				View->WorldToPixel(ActorLocation, ScreenLoc);

				float XL = 0.f, YL = 0.f;
				UFont* Font = (UTC == SelectedChar) ? SelectFont : SmallFont;
				C->TextSize(Font, UTC->PlayerState->PlayerName, XL, YL);

				//center the text
				ScreenLoc.X -= XL * 0.5f;
				ScreenLoc.Y -= YL * 0.75f;

				PlayerNames.Add(FPlayerName(UTC->PlayerState->PlayerName, ActorLocation, ScreenLoc, FVector2D(XL, YL), Font));
			}
		}

		//Sort closest to farthest
		FVector CamLoc = CameraTransform.GetLocation();
		PlayerNames.Sort([CamLoc](const FPlayerName& A, const FPlayerName& B) -> bool
		{
			return  FVector::DistSquared(CamLoc, A.Location3D) > FVector::DistSquared(CamLoc, B.Location3D);
		});

		for (int32 i = 0; i < PlayerNames.Num(); i++)
		{
			//Draw the Player name
			FFontRenderInfo FontInfo;
			FontInfo.bEnableShadow = true;
			FontInfo.bClipText = true;
			C->DrawColor = FLinearColor::White;
			C->DrawText(PlayerNames[i].DrawFont, FText::FromString(PlayerNames[i].PlayerName), PlayerNames[i].Location.X, PlayerNames[i].Location.Y, 1.0f, 1.0f, FontInfo);

			//Move the remaining names out of the way
			for (int32 j = i + 1; j < PlayerNames.Num(); j++)
			{
				FPlayerName& A = PlayerNames[i];
				FPlayerName& B = PlayerNames[j];

				//if the names intersect, move it up along the Y axis
				if (A.Location.X < B.Location.X + B.Size.X && A.Location.X + A.Size.X > B.Location.X
					&& A.Location.Y < B.Location.Y + B.Size.Y && A.Location.Y + A.Size.Y > B.Location.Y)
				{
					B.Location.Y = A.Location.Y - (B.Size.Y * 0.6f);
				}
			}
		}
	}

	//Draw any needed hud canvas stuff
	AUTPlayerController* UTPC = GetPlayerOwner().IsValid() ? Cast<AUTPlayerController>(GetPlayerOwner()->PlayerController) : nullptr;
	if (UTPC != nullptr && UTPC->MyUTHUD != nullptr)
	{
		TArray<UUTHUDWidget*> DrawWidgets;

		//Draw the scoreboard if its not the intro
		if (HasCamFlag(CF_ShowScoreboard))
		{
			UUTScoreboard* Scoreboard = UTPC->MyUTHUD->GetScoreboard();
			if (Scoreboard != nullptr)
			{
				DrawWidgets.Add(Scoreboard);
			}
		}

		UUTHUDWidget* GameMessagesWidget = UTPC->MyUTHUD->FindHudWidgetByClass(UUTHUDWidgetMessage_GameMessages::StaticClass(), true);
		if (GameMessagesWidget != nullptr)
		{
			DrawWidgets.Add(GameMessagesWidget);
		}

		//Draw all the widgets
		for (auto Widget : DrawWidgets)
		{
			Widget->PreDraw(UTPC->GetWorld()->DeltaTimeSeconds, UTPC->MyUTHUD, C, FVector2D(C->SizeX * 0.5f, C->SizeY * 0.5f));
			Widget->Draw(UTPC->GetWorld()->DeltaTimeSeconds);
			Widget->PostDraw(UTPC->GetWorld()->GetTimeSeconds());
		}
	}
}

FReply SUWMatchSummary::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	//Send the key to the console since it doesn't have focus with the dialog open
	UUTConsole* UTConsole = (GetPlayerOwner() != nullptr && GetPlayerOwner()->ViewportClient != nullptr) ? Cast<UUTConsole>(GetPlayerOwner()->ViewportClient->ViewportConsole) : nullptr;
	if (UTConsole != nullptr)
	{
		if (UTConsole->InputKey(0, InKeyEvent.GetKey(), EInputEvent::IE_Pressed))
		{
			return FReply::Handled();
		}
	}

	//Pass taunt buttons along to the player controller
	UUTPlayerInput* UTInput = Cast<UUTPlayerInput>(GetPlayerOwner()->PlayerController->PlayerInput);
	AUTPlayerController* UTPC = Cast<AUTPlayerController>(GetPlayerOwner()->PlayerController);

	if (UTInput != nullptr && UTPC != nullptr)
	{
		for (auto& Bind : UTInput->CustomBinds)
		{
			if (Bind.Command == TEXT("PlayTaunt") && InKeyEvent.GetKey().GetFName() == Bind.KeyName)
			{
				UTPC->PlayTaunt();
				return FReply::Handled();
			}
			if (Bind.Command == TEXT("PlayTaunt2") && InKeyEvent.GetKey().GetFName() == Bind.KeyName)
			{
				UTPC->PlayTaunt2();
				return FReply::Handled();
			}
		}

		//Pass the key event to PlayerInput 
		if (UTInput->InputKey(InKeyEvent.GetKey(), EInputEvent::IE_Pressed, 1.0f, false))
		{
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

FReply SUWMatchSummary::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	//Send the key to the console since it doesn't have focus with the dialog open
	UUTConsole* UTConsole = (GetPlayerOwner() != nullptr && GetPlayerOwner()->ViewportClient != nullptr) ? Cast<UUTConsole>(GetPlayerOwner()->ViewportClient->ViewportConsole) : nullptr;
	if (UTConsole != nullptr)
	{
		if (UTConsole->InputKey(0, InKeyEvent.GetKey(), EInputEvent::IE_Released))
		{
			return FReply::Handled();
		}
	}

	//Pass the key event to PlayerInput 
	UUTPlayerInput* UTInput = Cast<UUTPlayerInput>(GetPlayerOwner()->PlayerController->PlayerInput);
	if (UTInput != nullptr)
	{
		if (UTInput->InputKey(InKeyEvent.GetKey(), EInputEvent::IE_Released, 1.0f, false))
		{
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

FReply SUWMatchSummary::OnKeyChar(const FGeometry& InGeometry, const FCharacterEvent& InCharacterEvent)
{
	//Send the character to the console since it doesn't have focus with the dialog open
	UUTConsole* UTConsole = (GetPlayerOwner() != nullptr && GetPlayerOwner()->ViewportClient != nullptr) ? Cast<UUTConsole>(GetPlayerOwner()->ViewportClient->ViewportConsole) : nullptr;
	if (UTConsole != nullptr)
	{
		FString CharacterString;
		CharacterString += InCharacterEvent.GetCharacter();
		if (UTConsole->InputChar(InCharacterEvent.GetUserIndex(), CharacterString))
		{
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

void SUWMatchSummary::PlayTauntByClass(AUTPlayerState* PS, TSubclassOf<AUTTaunt> TauntToPlay, float EmoteSpeed)
{
	AUTCharacter* UTC = FindCharacter(PS);
	if (UTC != nullptr)
	{
		UTC->PlayTauntByClass(TauntToPlay, EmoteSpeed);
	}
}

void SUWMatchSummary::SetEmoteSpeed(AUTPlayerState* PS, float EmoteSpeed)
{
	AUTCharacter* UTC = FindCharacter(PS);
	if (UTC != nullptr)
	{
		UTC->SetEmoteSpeed(EmoteSpeed);
	}
}

AUTCharacter* SUWMatchSummary::FindCharacter(class AUTPlayerState* PS)
{
	if (PS != nullptr)
	{
		for (AUTCharacter* UTC : PlayerPreviewMeshs)
		{
			if (UTC->PlayerState != nullptr && UTC->PlayerState == PS)
			{
				return UTC;
			}
		}
	}
	return nullptr;
}

void SUWMatchSummary::ViewCharacter(AUTCharacter* NewChar)
{
	if (NewChar != nullptr)
	{
		NewChar->UpdateTacComMesh(false);
	}

	TSharedPtr<FCharacterCamera> PlayerCam = MakeShareable(new FCharacterCamera(NewChar));
	PlayerCam->CamFlags |= CF_CanInteract;
	CameraShots.Empty();
	CameraShots.Add(PlayerCam);
	SetCamShot(0);

	FriendStatus = NAME_None;
	BuildInfoPanel();
}

void SUWMatchSummary::SelectPlayerState(AUTPlayerState* PS)
{
	AUTCharacter* UTC = FindCharacter(PS);
	if (UTC != nullptr)
	{
		ViewCharacter(UTC);
	}
}

void SUWMatchSummary::ViewTeam(int32 NewTeam)
{
	if (TeamAnchors.Num() == 0)
	{
		return;
	}

	ViewedTeamNum = NewTeam;
	if (!TeamAnchors.IsValidIndex(ViewedTeamNum))
	{
		ViewedTeamNum = 0;
	}

	//use TeamCamAlpha to blend between start and end cams
	struct FTeamCameraPan : FTeamCamera
	{
		FTeamCameraPan(int32 InTeamNum) : FTeamCamera(InTeamNum) {}
		virtual bool TickCamera(class SUWMatchSummary* MatchWidget, float ElapsedTime, float DeltaTime, FTransform& InOutCamera) override
		{
			CameraTransform.Blend(CamStart, CamEnd, FMath::Clamp(MatchWidget->TeamCamAlpha, 0.0f, 1.0f));
			FMatchCamera::TickCamera(MatchWidget, ElapsedTime, DeltaTime, InOutCamera);
			return false;
		}
	};

	TSharedPtr<FTeamCameraPan> TeamCam = MakeShareable(new FTeamCameraPan(NewTeam));
	TeamCam->CamFlags |= CF_CanInteract;
	CameraShots.Empty();
	CameraShots.Add(TeamCam);
	SetCamShot(0);

	bAutoScrollTeam = true;
	BuildInfoPanel();
}

void SUWMatchSummary::ShowTeam(int32 TeamNum)
{
	// hide everyone else, show this team
	if (TeamPreviewMeshs.Num() > 1)
	{
		for (int32 i = 0; i< TeamPreviewMeshs.Num(); i++)
		{
			TArray<AUTCharacter*> &TeamCharacters = TeamPreviewMeshs[i];
			bool bViewedTeam = (i == ViewedTeamNum);
			for (int32 j = 0; j < TeamCharacters.Num(); j++)
			{
				TeamCharacters[j]->HideCharacter(!bViewedTeam);
			}
		}
		for (auto Weapon : PreviewWeapons)
		{
			AUTCharacter* Holder = Cast<AUTCharacter>(Weapon->Instigator);
			AUTPlayerState* PS = Holder ? Cast<AUTPlayerState>(Holder->PlayerState) : NULL;
			bool bSameTeamWeapon = (PS && PS->Team && (PS->Team->TeamIndex == TeamNum)) || TeamNum < 0;
			Weapon->SetActorHiddenInGame(!bSameTeamWeapon);
		}
	}
}

void SUWMatchSummary::ShowCharacter(AUTCharacter* UTC)
{
	// hide everyone else, show this player
	if (TeamPreviewMeshs.Num() > 1)
	{
		for (int32 i = 0; i< TeamPreviewMeshs.Num(); i++)
		{
			TArray<AUTCharacter*> &TeamCharacters = TeamPreviewMeshs[i];
			for (int32 j = 0; j < TeamCharacters.Num(); j++)
			{
				TeamCharacters[j]->HideCharacter(TeamCharacters[j] != UTC);
			}
		}
		for (auto Weapon : PreviewWeapons)
		{
			AUTCharacter* Holder = Cast<AUTCharacter>(Weapon->Instigator);
			Weapon->SetActorHiddenInGame(Holder != UTC);
		}
	}
}

void SUWMatchSummary::ShowAllCharacters()
{
	if (TeamPreviewMeshs.Num() > 1)
	{
		for (int32 i = 0; i< TeamPreviewMeshs.Num(); i++)
		{
			TArray<AUTCharacter*> &TeamCharacters = TeamPreviewMeshs[i];
			for (int32 j = 0; j < TeamCharacters.Num(); j++)
			{
				TeamCharacters[j]->HideCharacter(false);
			}
		}
		for (auto Weapon : PreviewWeapons)
		{
			AUTCharacter* Holder = Cast<AUTCharacter>(Weapon->Instigator);
			Weapon->SetActorHiddenInGame(false);
		}
	}
}

float SUWMatchSummary::GetAllCameraOffset()
{
	int32 MaxSize = 1;
	for (int32 iTeam = 0; iTeam < TeamPreviewMeshs.Num(); iTeam++)
	{
		MaxSize = FMath::Max(MaxSize, TeamPreviewMeshs[iTeam].Num());
	}
	MaxSize += 1.f;
	if (TeamPreviewMeshs.Num() < 2)
	{
		// players are across rather than angled  @FIXMESTEVE use actual FOV if can change in this view
		return ALL_CAMERA_OFFSET + (MaxSize * PLAYER_SPACING) / FMath::Tan(45.f * PI / 180.f);
	}
	float BaseCameraOffset = ALL_CAMERA_OFFSET + 0.5f * MaxSize * PLAYER_SPACING * FMath::Sin(TEAMANGLE * PI / 180.f);
	float Width = 2.f*MIN_TEAM_SPACING + 0.5f * MaxSize * PLAYER_SPACING * FMath::Cos(TEAMANGLE * PI / 180.f);
	return BaseCameraOffset + Width / FMath::Tan(45.f * PI / 180.f);
}

void SUWMatchSummary::ViewAll()
{
	TSharedPtr<FAllCamera> AllCam = MakeShareable(new FAllCamera);
	AllCam->CamFlags = CF_CanInteract | CF_ShowScoreboard;
	CameraShots.Empty();
	CameraShots.Add(AllCam);
	SetCamShot(0);
}

UUTScoreboard* SUWMatchSummary::GetScoreboard()
{
	AUTPlayerController* UTPC = GetPlayerOwner().IsValid() ? Cast<AUTPlayerController>(GetPlayerOwner()->PlayerController) : nullptr;
	if (UTPC != nullptr && UTPC->MyUTHUD != nullptr)
	{
		UUTScoreboard* Scoreboard = UTPC->MyUTHUD->GetScoreboard();
		if (Scoreboard != nullptr)
		{
			//Make sure the hud owner is set before calling any Scoreboard functions
			Scoreboard->UTHUDOwner = UTPC->MyUTHUD;
			return Scoreboard;
		}
	}
	return nullptr;
}

void SUWMatchSummary::OnMouseDownPlayerPreview(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	MousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	//Try to Click the scoreboard first
	if (HasCamFlag(CF_ShowScoreboard) && HasCamFlag(CF_CanInteract))
	{
		UUTScoreboard* Scoreboard = GetScoreboard();
		if (Scoreboard != nullptr && Scoreboard->AttemptSelection(MousePos))
		{
			Scoreboard->SelectionClick();
			return;
		}
	}

	//Click a character
	if (HasCamFlag(CF_CanInteract) && !HasCamFlag(CF_Player) && HighlightedChar.IsValid())
	{
		ViewCharacter(HighlightedChar.Get());
	}
}

bool SUWMatchSummary::HasCamFlag(ECamFlags CamFlag) const
{
	TSharedPtr<FMatchCamera> Shot = GetCurrentShot();
	if (Shot.IsValid())
	{
		return (Shot->CamFlags & (uint32)CamFlag) > 0;
	}
	return false;
}

void SUWMatchSummary::OnMouseMovePlayerPreview(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	MousePos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	UUTScoreboard* Scoreboard = GetScoreboard();
	if (Scoreboard != nullptr)
	{
		if (HasCamFlag(CF_CanInteract) && HasCamFlag(CF_ShowScoreboard))
		{
			Scoreboard->BecomeInteractive();
			Scoreboard->TrackMouseMovement(MousePos);
		}
		else
		{
			Scoreboard->BecomeNonInteractive();
		}
	}
}

void SUWMatchSummary::DragPlayerPreview(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (HasCamFlag(CF_CanInteract))
	{
		if (HasCamFlag(CF_Team))
		{
			//Scroll the team left/right
			if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && TeamPreviewMeshs.IsValidIndex(ViewedTeamNum) && TeamPreviewMeshs[ViewedTeamNum].Num() > 0)
			{
				float Dist = FVector::Dist(TeamPreviewMeshs[ViewedTeamNum][0]->GetActorLocation(), TeamPreviewMeshs[ViewedTeamNum][TeamPreviewMeshs[ViewedTeamNum].Num() - 1]->GetActorLocation());

				if (Dist > 0.0f)
				{
					TeamCamAlpha -= MouseEvent.GetCursorDelta().X / Dist;
					TeamCamAlpha = FMath::Clamp(TeamCamAlpha, 0.0f, 1.0f);
				}
				bAutoScrollTeam = false;
			}
		}
		else if (HasCamFlag(CF_Player) && ViewedChar.IsValid())
		{
			//Rotate the character
			if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
			{
				ViewedChar->SetActorRotation(ViewedChar->GetActorRotation() + FRotator(0, 0.2f * -MouseEvent.GetCursorDelta().X, 0.0f));
			}
			//Pan up and down
			else if (MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
			{
				if (CameraShots.Num() > 0)
				{
					FVector Location = CameraShots[0]->CameraTransform.GetLocation();
					Location.Z = FMath::Clamp(Location.Z + MouseEvent.GetCursorDelta().Y, -25.0f, 70.0f);
					CameraShots[0]->CameraTransform.SetLocation(Location);
				}
			}
		}
	}
}

void SUWMatchSummary::ZoomPlayerPreview(float WheelDelta)
{
	if (HasCamFlag(CF_CanInteract))
	{
		if (HasCamFlag(CF_Player) && WheelDelta < 0.0f)
		{
			ViewTeam(ViewedTeamNum);
		}
		else if (HasCamFlag(CF_Team) && WheelDelta < 0.0f)
		{
			ViewAll();
		}
	}

	//Send the scroll wheel to player input so they can change speed of the taunt
	UUTPlayerInput* UTInput = Cast<UUTPlayerInput>(GetPlayerOwner()->PlayerController->PlayerInput);
	if (UTInput != nullptr)
	{
		FKey ScrollKey = WheelDelta > 0.0f ? EKeys::MouseScrollUp : EKeys::MouseScrollDown;
		UTInput->InputKey(ScrollKey, EInputEvent::IE_Pressed, 1.0f, false);
	}
}

FReply SUWMatchSummary::OnSwitcherNext()
{
	if (HasCamFlag(CF_Team))
	{
		int32 Index = ViewedTeamNum + 1;
		ViewTeam(TeamAnchors.IsValidIndex(Index) ? Index : 0);
	}
	else if (HasCamFlag(CF_Player) && ViewedChar.IsValid())
	{
		int32 Index = PlayerPreviewMeshs.Find(ViewedChar.Get());
		if (Index == INDEX_NONE)
		{
			Index = 0;
		}

		Index++;
		if (PlayerPreviewMeshs.IsValidIndex(Index))
		{
			ViewCharacter(PlayerPreviewMeshs[Index]);
		}
		else
		{
			ViewCharacter(PlayerPreviewMeshs[0]);
		}
	}
	return FReply::Handled();
}

FReply SUWMatchSummary::OnSwitcherPrevious()
{
	if (HasCamFlag(CF_Team))
	{
		int32 Index = ViewedTeamNum - 1;
		ViewTeam(TeamAnchors.IsValidIndex(Index) ? Index : TeamAnchors.Num() - 1);
	}
	else if (HasCamFlag(CF_Player) && ViewedChar.IsValid())
	{
		int32 Index = PlayerPreviewMeshs.Find(ViewedChar.Get());
		if (Index == INDEX_NONE)
		{
			Index = 0;
		}

		Index--;
		if (PlayerPreviewMeshs.IsValidIndex(Index))
		{
			ViewCharacter(PlayerPreviewMeshs[Index]);
		}
		else
		{
			ViewCharacter(PlayerPreviewMeshs[PlayerPreviewMeshs.Num() - 1]);
		}
	}
	return FReply::Handled();
}

FText SUWMatchSummary::GetSwitcherText() const
{
	if (HasCamFlag(CF_Player) && ViewedChar.IsValid() && ViewedChar->PlayerState != nullptr)
	{
		return FText::FromString(ViewedChar->PlayerState->PlayerName);
	}
	if (HasCamFlag(CF_Team) && GameState.IsValid() && GameState->Teams.IsValidIndex(ViewedTeamNum))
	{
		return FText::Format(NSLOCTEXT("SUWMatchSummary", "Team", "{0} Team"), GameState->Teams[ViewedTeamNum]->TeamName);
	}
	return FText::GetEmpty();
}

FSlateColor SUWMatchSummary::GetSwitcherColor() const
{
	if (HasCamFlag(CF_Team) && GameState.IsValid() && GameState->Teams.IsValidIndex(ViewedTeamNum))
	{
		return FMath::LerpStable(GameState->Teams[ViewedTeamNum]->TeamColor, FLinearColor::White, 0.3f);
	}
	return FLinearColor::White;
}

EVisibility SUWMatchSummary::GetSwitcherVisibility() const
{
	if (HasCamFlag(CF_ShowSwitcher) && !GetSwitcherText().IsEmpty())
	{
		return EVisibility::Visible;
	}
	return EVisibility::Hidden;
}

EVisibility SUWMatchSummary::GetSwitcherButtonVisibility() const
{
	return (HasCamFlag(CF_CanInteract) && !HasCamFlag(CF_All)) ? EVisibility::Visible : EVisibility::Hidden;
}

FOptionalSize SUWMatchSummary::GetStatsWidth() const
{
	float WantedWidth = HasCamFlag(CF_ShowInfoWidget) ? 1050.f : 0.0f;
	StatsWidth = GameState.IsValid() && GameState->GetWorld() ? FMath::FInterpTo(StatsWidth, WantedWidth, GameState->GetWorld()->DeltaTimeSeconds, 10.0f) : 10.f;
	return FOptionalSize(StatsWidth);
}

FReply SUWMatchSummary::OnClose()
{
	if (GameState.IsValid() && ((GameState->GetMatchState() == MatchState::WaitingToStart) || (GameState->GetMatchState() == MatchState::WaitingPostMatch)))
	{
		ViewAll();
	}
	else
	{
		GetPlayerOwner()->CloseMatchSummary();
	}
	return FReply::Handled();
}

FReply SUWMatchSummary::OnSendFriendRequest()
{
	if (ViewedChar.IsValid() && ViewedChar->PlayerState != nullptr)
	{
		if (FriendStatus != FFriendsStatus::FriendRequestPending && FriendStatus != FFriendsStatus::IsBot)
		{
			GetPlayerOwner()->RequestFriendship(ViewedChar->PlayerState->UniqueId.GetUniqueNetId());

			FriendPanel->ClearChildren();
			FriendPanel->AddSlot()
				.Padding(10.0, 0.0, 0.0, 0.0)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("SUWPlayerInfoDialog", "FriendRequestPending", "You have sent a friend request..."))
					.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
				];

			FriendStatus = FFriendsStatus::FriendRequestPending;
		}
	}
	return FReply::Handled();
}

FText SUWMatchSummary::GetFunnyText()
{
	return NSLOCTEXT("SUWPlayerInfoDialog", "FunnyDefault", "Viewing self.");
}

void SUWMatchSummary::BuildFriendPanel()
{
	if (ViewedChar.IsValid() && ViewedChar->PlayerState != nullptr)
	{
		FName NewFriendStatus;
		if (GetPlayerOwner()->PlayerController->PlayerState == ViewedChar->PlayerState)
		{
			NewFriendStatus = FFriendsStatus::IsYou;
		}
		else if (ViewedChar->PlayerState->bIsABot)
		{
			NewFriendStatus = FFriendsStatus::IsBot;
		}
		else
		{
			NewFriendStatus = GetPlayerOwner()->IsAFriend(ViewedChar->PlayerState->UniqueId) ? FFriendsStatus::Friend : FFriendsStatus::NotAFriend;
		}

		bool bRequiresRefresh = false;
		if (FriendStatus == FFriendsStatus::FriendRequestPending)
		{
			if (NewFriendStatus == FFriendsStatus::Friend)
			{
				FriendStatus = NewFriendStatus;
				bRequiresRefresh = true;
			}
		}
		else
		{
			bRequiresRefresh = FriendStatus != NewFriendStatus;
			FriendStatus = NewFriendStatus;
		}

		if (bRequiresRefresh)
		{
			FriendPanel->ClearChildren();
			if (FriendStatus == FFriendsStatus::IsYou)
			{
				FText FunnyText = GetFunnyText();
				FriendPanel->AddSlot()
					.Padding(10.0, 0.0, 0.0, 0.0)
					[
						SNew(STextBlock)
						.Text(FunnyText)
						.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
					];
			}
			else if (FriendStatus == FFriendsStatus::IsBot)
			{
				FriendPanel->AddSlot()
					.Padding(10.0, 0.0, 0.0, 0.0)
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("SUWPlayerInfoDialog", "IsABot", "AI (C) Liandri Corp."))
						.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
					];
			}
			else if (FriendStatus == FFriendsStatus::Friend)
			{
				FriendPanel->AddSlot()
					.Padding(10.0, 0.0, 0.0, 0.0)
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("SUWPlayerInfoDialog", "IsAFriend", "Is your friend"))
						.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
					];
			}
			else if (FriendStatus == FFriendsStatus::FriendRequestPending)
			{
			}
			else
			{
				FriendPanel->AddSlot()
					.AutoWidth()
					.Padding(10.0, 0.0, 0.0, 0.0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.ButtonStyle(SUWindowsStyle::Get(), "UT.BottomMenu.Button")
						.ContentPadding(FMargin(5.0f, 5.0f, 5.0f, 5.0f))
						.Text(NSLOCTEXT("SUWPlayerInfoDialog", "SendFriendRequest", "Send Friend Request"))
						.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
						.OnClicked(this, &SUWMatchSummary::OnSendFriendRequest)
					];
			}
		}
	}
}

#endif