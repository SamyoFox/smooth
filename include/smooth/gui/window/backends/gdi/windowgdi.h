 /* The smooth Class Library
  * Copyright (C) 1998-2020 Robert Kausch <robert.kausch@gmx.net>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of "The Artistic License, Version 2.0".
  *
  * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

#ifndef H_OBJSMOOTH_WINDOWGDI
#define H_OBJSMOOTH_WINDOWGDI

#include <shlobj.h>
#include <imm.h>

namespace smooth
{
	namespace System
	{
		class Timer;
	};

	namespace GUI
	{
		class Cursor;

		class WindowGDI;
	};
};

#include "../windowbackend.h"
#include "../../../../graphics/forms/rect.h"
#include "../../../../input/keyboard.h"

namespace smooth
{
	namespace GUI
	{
		const Short	 WINDOW_GDI	= 1;

		class WindowGDI : public WindowBackend
		{
			private:
#if defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
				static LRESULT CALLBACK			 WindowProc(HWND, UINT, WPARAM, LPARAM) __attribute__((force_align_arg_pointer));
#else
				static LRESULT CALLBACK			 WindowProc(HWND, UINT, WPARAM, LPARAM);
#endif
				static Array<WindowGDI *, Void *>	 windowBackends;

				static WindowGDI			*GetWindowBackend(HWND);

				static Cursor				*activeCursor;

				Int					 ProcessSystemMessages(UINT, WPARAM, LPARAM);

				WindowGDI				*FindLeaderWindow();
			protected:
				Int					 id;

				HWND					 hwnd;

				WNDCLASSEX				*wndclass;
				String					 className;

				HICON					 sysIcon;
				Bool					 destroyIcon;

				HDROP					 hDrop;
				IDropTarget				*dropTarget;
				Bool					 enableDropFiles;

				ITaskbarList3				*taskbar;

				Rect					 restoredRect;
				Int					 restoredWndStyle;

				Size					 frameSize;
				Float					 fontSize;

				Size					 minSize;
				Size					 maxSize;

				Bool					 minimized;
				Bool					 maximized;

				Int					 flags;

				Input::Keyboard::Key			 ConvertKey(Int);
			public:
				static Int				 Initialize();
				static Int				 Free();

									 WindowGDI(Void * = NIL);
									~WindowGDI();

				Void					*GetSystemWindow() const;

				Int					 Open(const String &, const Point &, const Size &, Int);
				Int					 Close();

				Int					 RequestClose();

				Int					 SetTitle(const String &);

				Int					 SetIcon(const Bitmap &);
				Int					 SetIconDirect(Void *);

				Int					 SetProgressIndicator(Window::ProgressIndicatorState, Float);

				Int					 EnableDropFiles(Bool);
				const Array<String>			&GetDroppedFiles() const;

				Int					 SetMinimumSize(const Size &);
				Int					 SetMaximumSize(const Size &);

				Int					 Show();
				Int					 Hide();

				Rect					 GetRestoredWindowRect() const	{ return restoredRect; }

				Int					 SetMetrics(const Point &, const Size &);

				Int					 Minimize();

				Int					 Maximize();
				Int					 Restore();

				Int					 Raise();
			slots:
				static Void				 SetCursor(Cursor *, const Point &);
				static Void				 RemoveCursor(Cursor *);
		};
	};
};

#endif
