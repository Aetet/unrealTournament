// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "WebBrowserPrivatePCH.h"
#include "WebBrowserWindow.h"
#include "SlateCore.h"
#include "SlateBasics.h"

#if WITH_CEF3
FWebBrowserWindow::FWebBrowserWindow(FIntPoint InViewportSize)
	: UpdatableTexture(nullptr)
	, ViewportSize(InViewportSize)
	, bIsClosing(false)
	, bHasBeenPainted(false)
{
	TextureData.Reserve(ViewportSize.X * ViewportSize.Y * 4);
	TextureData.SetNumZeroed(ViewportSize.X * ViewportSize.Y * 4);

	if (FSlateApplication::Get().GetRenderer().IsValid())
	{
		UpdatableTexture = FSlateApplication::Get().GetRenderer()->CreateUpdatableTexture(ViewportSize.X, ViewportSize.Y);
	}
}

FWebBrowserWindow::~FWebBrowserWindow()
{
	CloseBrowser();

	if (FSlateApplication::IsInitialized() && FSlateApplication::Get().GetRenderer().IsValid() && UpdatableTexture != nullptr)
	{
		FSlateApplication::Get().GetRenderer()->ReleaseUpdatableTexture(UpdatableTexture);
	}
	UpdatableTexture = nullptr;
}

void FWebBrowserWindow::SetViewportSize(FVector2D WindowSize)
{
	// Magic number for texture size, can't access GetMax2DTextureDimension easily
	FIntPoint ClampedWindowSize = WindowSize.ClampAxes(1, 2048).IntPoint();

	// Ignore sizes that can't be seen as it forces CEF to re-render whole image
	if (WindowSize >= FVector2D::UnitVector && ViewportSize != ClampedWindowSize)
	{
		FIntPoint OldViewportSize = MoveTemp(ViewportSize);
		TArray<uint8> OldTextureData = MoveTemp(TextureData);
		ViewportSize = ClampedWindowSize;
		TextureData.SetNumZeroed(ViewportSize.X * ViewportSize.Y * 4);

		// copy row by row to avoid texture distortion
		const int32 WriteWidth = FMath::Min(OldViewportSize.X, ViewportSize.X) * 4;
		const int32 WriteHeight = FMath::Min(OldViewportSize.Y, ViewportSize.Y);
		for (int32 RowIndex = 0; RowIndex < WriteHeight; ++RowIndex)
		{
			FMemory::Memcpy(TextureData.GetData() + ViewportSize.X * RowIndex * 4, OldTextureData.GetData() + OldViewportSize.X * RowIndex * 4, WriteWidth);
		}

		if (UpdatableTexture != nullptr)
		{
			UpdatableTexture->ResizeTexture(ViewportSize.X, ViewportSize.Y);
			UpdatableTexture->UpdateTexture(TextureData);
		}
		if (IsValid())
		{
			InternalCefBrowser->GetHost()->WasResized();
		}
	}
}

FSlateShaderResource* FWebBrowserWindow::GetTexture()
{
	if (UpdatableTexture != nullptr)
	{
		return UpdatableTexture->GetSlateResource();
	}
	return nullptr;
}

bool FWebBrowserWindow::IsValid() const
{
	return InternalCefBrowser.get() != nullptr;
}

bool FWebBrowserWindow::HasBeenPainted() const
{
	return bHasBeenPainted;
}

bool FWebBrowserWindow::IsClosing() const
{
	return bIsClosing;
}

FString FWebBrowserWindow::GetTitle() const
{
	return Title;
}

void FWebBrowserWindow::OnKeyDown(const FKeyEvent& InKeyEvent)
{
	if (IsValid())
	{
		CefKeyEvent KeyEvent;
#if PLATFORM_MAC
		KeyEvent.native_key_code = InKeyEvent.GetKeyCode();
		KeyEvent.character = InKeyEvent.GetCharacter();
#else
		KeyEvent.windows_key_code = InKeyEvent.GetKeyCode();
#endif
		// TODO: Figure out whether this is a system key if we come across problems
		/*KeyEvent.is_system_key = message == WM_SYSCHAR ||
			message == WM_SYSKEYDOWN ||
			message == WM_SYSKEYUP;*/

		KeyEvent.type = KEYEVENT_RAWKEYDOWN;
		KeyEvent.modifiers = GetCefKeyboardModifiers(InKeyEvent);

		InternalCefBrowser->GetHost()->SendKeyEvent(KeyEvent);
	}
}

void FWebBrowserWindow::OnKeyUp(const FKeyEvent& InKeyEvent)
{
	if (IsValid())
	{
		CefKeyEvent KeyEvent;
#if PLATFORM_MAC
		KeyEvent.native_key_code = InKeyEvent.GetKeyCode();
		KeyEvent.character = InKeyEvent.GetCharacter();
#else
		KeyEvent.windows_key_code = InKeyEvent.GetKeyCode();
#endif
		// TODO: Figure out whether this is a system key if we come across problems
		/*KeyEvent.is_system_key = message == WM_SYSCHAR ||
			message == WM_SYSKEYDOWN ||
			message == WM_SYSKEYUP;*/

		KeyEvent.type = KEYEVENT_KEYUP;
		KeyEvent.modifiers = GetCefKeyboardModifiers(InKeyEvent);

		InternalCefBrowser->GetHost()->SendKeyEvent(KeyEvent);
	}
}

void FWebBrowserWindow::OnKeyChar(const FCharacterEvent& InCharacterEvent)
{
	if (IsValid())
	{
		CefKeyEvent KeyEvent;
#if PLATFORM_MAC
		KeyEvent.character = InCharacterEvent.GetCharacter();
#else
		KeyEvent.windows_key_code = InCharacterEvent.GetCharacter();
#endif
		// TODO: Figure out whether this is a system key if we come across problems
		/*KeyEvent.is_system_key = message == WM_SYSCHAR ||
			message == WM_SYSKEYDOWN ||
			message == WM_SYSKEYUP;*/

		KeyEvent.type = KEYEVENT_CHAR;
		KeyEvent.modifiers = GetCefInputModifiers(InCharacterEvent);

		InternalCefBrowser->GetHost()->SendKeyEvent(KeyEvent);
	}
}

void FWebBrowserWindow::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (IsValid())
	{
		FKey Button = MouseEvent.GetEffectingButton();
		CefBrowserHost::MouseButtonType Type =
			(Button == EKeys::LeftMouseButton ? MBT_LEFT : (
			Button == EKeys::RightMouseButton ? MBT_RIGHT : MBT_MIDDLE));

		CefMouseEvent Event;
		FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		Event.x = LocalPos.X;
		Event.y = LocalPos.Y;
		Event.modifiers = GetCefMouseModifiers(MouseEvent);

		InternalCefBrowser->GetHost()->SendMouseClickEvent(Event, Type, false,1);
	}
}

void FWebBrowserWindow::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (IsValid())
	{
		FKey Button = MouseEvent.GetEffectingButton();
		CefBrowserHost::MouseButtonType Type =
			(Button == EKeys::LeftMouseButton ? MBT_LEFT : (
			Button == EKeys::RightMouseButton ? MBT_RIGHT : MBT_MIDDLE));

		CefMouseEvent Event;
		FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		Event.x = LocalPos.X;
		Event.y = LocalPos.Y;
		Event.modifiers = GetCefMouseModifiers(MouseEvent);

		InternalCefBrowser->GetHost()->SendMouseClickEvent(Event, Type, true, 1);
	}
}

void FWebBrowserWindow::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (IsValid())
	{
		FKey Button = MouseEvent.GetEffectingButton();
		CefBrowserHost::MouseButtonType Type =
			(Button == EKeys::LeftMouseButton ? MBT_LEFT : (
			Button == EKeys::RightMouseButton ? MBT_RIGHT : MBT_MIDDLE));

		CefMouseEvent Event;
		FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		Event.x = LocalPos.X;
		Event.y = LocalPos.Y;
		Event.modifiers = GetCefMouseModifiers(MouseEvent);

		InternalCefBrowser->GetHost()->SendMouseClickEvent(Event, Type, false, 2);
	}
}

void FWebBrowserWindow::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (IsValid())
	{
		CefMouseEvent Event;
		FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		Event.x = LocalPos.X;
		Event.y = LocalPos.Y;
		Event.modifiers = GetCefMouseModifiers(MouseEvent);

		InternalCefBrowser->GetHost()->SendMouseMoveEvent(Event, false);
	}
}

void FWebBrowserWindow::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if(IsValid())
	{
		// The original delta is reduced so this should bring it back to what CEF expects
		const float SpinFactor = 120.0f;
		const float TrueDelta = MouseEvent.GetWheelDelta() * SpinFactor;
		CefMouseEvent Event;
		FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		Event.x = LocalPos.X;
		Event.y = LocalPos.Y;
		Event.modifiers = GetCefMouseModifiers(MouseEvent);
		
		InternalCefBrowser->GetHost()->SendMouseWheelEvent(Event,
															MouseEvent.IsShiftDown() ? TrueDelta : 0,
															!MouseEvent.IsShiftDown() ? TrueDelta : 0);
	}
}

void FWebBrowserWindow::OnFocus(bool SetFocus)
{
	if (IsValid())
	{
		InternalCefBrowser->GetHost()->SendFocusEvent(SetFocus);
	}
}

void FWebBrowserWindow::OnCaptureLost()
{
	if (IsValid())
	{
		InternalCefBrowser->GetHost()->SendCaptureLostEvent();
	}
}

bool FWebBrowserWindow::CanGoBack() const
{
	if (IsValid())
	{
		return InternalCefBrowser->CanGoBack();
	}
	return false;
}

void FWebBrowserWindow::GoBack()
{
	if (IsValid())
	{
		InternalCefBrowser->GoBack();
	}
}

bool FWebBrowserWindow::CanGoForward() const
{
	if (IsValid())
	{
		return InternalCefBrowser->CanGoForward();
	}
	return false;
}

void FWebBrowserWindow::GoForward()
{
	if (IsValid())
	{
		InternalCefBrowser->GoForward();
	}
}

bool FWebBrowserWindow::IsLoading() const
{
	if (IsValid())
	{
		return InternalCefBrowser->IsLoading();
	}
	return false;
}

void FWebBrowserWindow::Reload()
{
	if (IsValid())
	{
		InternalCefBrowser->Reload();
	}
}

void FWebBrowserWindow::StopLoad()
{
	if (IsValid())
	{
		InternalCefBrowser->StopLoad();
	}
}

void FWebBrowserWindow::SetHandler(CefRefPtr<FWebBrowserHandler> InHandler)
{
	if (InHandler.get())
	{
		Handler = InHandler;
		Handler->SetBrowserWindow(SharedThis(this));
	}
}

void FWebBrowserWindow::CloseBrowser()
{
	bIsClosing = true;
	if (IsValid())
	{
		InternalCefBrowser->GetHost()->CloseBrowser(false);
		InternalCefBrowser = nullptr;
		Handler = nullptr;
	}
}

void FWebBrowserWindow::BindCefBrowser(CefRefPtr<CefBrowser> Browser)
{
	InternalCefBrowser = Browser;
}

void FWebBrowserWindow::SetTitle(const CefString& InTitle)
{
	Title = InTitle.ToWString().c_str();
	OnTitleChangedDelegate.Broadcast(Title);
}

bool FWebBrowserWindow::GetViewRect(CefRect& Rect)
{
	Rect.x = 0;
	Rect.y = 0;
	Rect.width = ViewportSize.X;
	Rect.height = ViewportSize.Y;

	return true;
}

void FWebBrowserWindow::OnPaint(CefRenderHandler::PaintElementType Type, const CefRenderHandler::RectList& DirtyRects, const void* Buffer, int Width, int Height)
{
	const int32 BufferSize = Width*Height*4;
	if (BufferSize == TextureData.Num())
	{
		FMemory::Memcpy(TextureData.GetData(), Buffer, BufferSize);
	}
	else
	{
		// copy row by row to avoid texture distortion
		const int32 WriteWidth = FMath::Min(Width, ViewportSize.X) * 4;
		const int32 WriteHeight = FMath::Min(Height, ViewportSize.Y);
		for (int32 RowIndex = 0; RowIndex < WriteHeight; ++RowIndex)
		{
			FMemory::Memcpy(TextureData.GetData() + ViewportSize.X * RowIndex * 4, static_cast<const uint8*>(Buffer) + Width * RowIndex * 4, WriteWidth);
		}
	}

	if (UpdatableTexture != nullptr)
	{
		UpdatableTexture->UpdateTexture(TextureData);
	}

	bHasBeenPainted = true;
}

void FWebBrowserWindow::OnCursorChange(CefCursorHandle Cursor)
{
	// TODO: Figure out Unreal cursor type from this,
	// may need to reload unreal cursors to compare handles
	//::SetCursor( Cursor );
}

int32 FWebBrowserWindow::GetCefKeyboardModifiers(const FKeyEvent& KeyEvent)
{
	int32 Modifiers = GetCefInputModifiers(KeyEvent);

	const FKey Key = KeyEvent.GetKey();
	if (Key == EKeys::LeftAlt ||
		Key == EKeys::LeftCommand ||
		Key == EKeys::LeftControl ||
		Key == EKeys::LeftShift)
	{
		Modifiers |= EVENTFLAG_IS_LEFT;
	}
	if (Key == EKeys::RightAlt ||
		Key == EKeys::RightCommand ||
		Key == EKeys::RightControl ||
		Key == EKeys::RightShift)
	{
		Modifiers |= EVENTFLAG_IS_RIGHT;
	}
	if (Key == EKeys::NumPadZero ||
		Key == EKeys::NumPadOne ||
		Key == EKeys::NumPadTwo ||
		Key == EKeys::NumPadThree ||
		Key == EKeys::NumPadFour ||
		Key == EKeys::NumPadFive ||
		Key == EKeys::NumPadSix ||
		Key == EKeys::NumPadSeven ||
		Key == EKeys::NumPadEight ||
		Key == EKeys::NumPadNine)
	{
		Modifiers |= EVENTFLAG_IS_KEY_PAD;
	}

	return Modifiers;
}

int32 FWebBrowserWindow::GetCefMouseModifiers(const FPointerEvent& InMouseEvent)
{
	int32 Modifiers = GetCefInputModifiers(InMouseEvent);

	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		Modifiers |= EVENTFLAG_LEFT_MOUSE_BUTTON;
	}
	if (InMouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton))
	{
		Modifiers |= EVENTFLAG_MIDDLE_MOUSE_BUTTON;
	}
	if (InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
	{
		Modifiers |= EVENTFLAG_RIGHT_MOUSE_BUTTON;
	}

	return Modifiers;
}

int32 FWebBrowserWindow::GetCefInputModifiers(const FInputEvent& InputEvent)
{
	int32 Modifiers = 0;

	if (InputEvent.IsShiftDown())
	{
		Modifiers |= EVENTFLAG_SHIFT_DOWN;
	}
	if (InputEvent.IsControlDown())
	{
		Modifiers |= EVENTFLAG_CONTROL_DOWN;
	}
	if (InputEvent.IsAltDown())
	{
		Modifiers |= EVENTFLAG_ALT_DOWN;
	}
	if (InputEvent.IsCommandDown())
	{
		Modifiers |= EVENTFLAG_COMMAND_DOWN;
	}
	if (InputEvent.AreCapsLocked())
	{
		Modifiers |= EVENTFLAG_CAPS_LOCK_ON;
	}
	// TODO: Add function for this if necessary
	/*if (InputEvent.AreNumsLocked())
	{
		Modifiers |= EVENTFLAG_NUM_LOCK_ON;
	}*/

	return Modifiers;
}

#endif
