 /* The smooth Class Library
  * Copyright (C) 1998-2020 Robert Kausch <robert.kausch@gmx.net>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of "The Artistic License, Version 2.0".
  *
  * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

#include <smooth/gui/widgets/special/cursor.h>
#include <smooth/gui/widgets/basic/editbox.h>
#include <smooth/gui/widgets/multi/menu/popupmenu.h>
#include <smooth/gui/window/toolwindow.h>
#include <smooth/gui/clipboard/clipboard.h>
#include <smooth/graphics/surface.h>
#include <smooth/input/keyboard.h>
#include <smooth/input/pointer.h>
#include <smooth/i18n/translator.h>
#include <smooth/misc/binary.h>
#include <smooth/misc/math.h>
#include <smooth/system/system.h>
#include <smooth/system/timer.h>

#include <fribidi/fribidi.h>

static const S::Int	 CF_NOINPUT = 1 << 30;

const S::Short	 S::GUI::Cursor::classID = S::Object::RequestClassID();

S::Signal2<S::Void, S::GUI::Cursor *, const S::GUI::Point &>	 S::GUI::Cursor::internalSetCursor;
S::Signal1<S::Void, S::GUI::Cursor *>				 S::GUI::Cursor::internalRemoveCursor;

S::GUI::Cursor::Cursor(const Point &iPos, const Size &iSize) : Widget(iPos, iSize)
{
	type		= classID;
	markStart	= -1;
	markEnd		= -1;
	maxSize		= 32768;
	promptPos	= 0;
	promptVisible	= False;
	timer		= NIL;
	marking		= False;
	visibleOffset	= 0;
	scrollPos	= 0;
	maxScrollPos	= 0;
	contextMenu	= NIL;
	historyPos	= 0;
	tabSize		= 4;

	imeAdvance	= 0;
	imeCursor	= False;

	SetTabstopCapable(True);

	onGetFocus.Connect(&Cursor::OnGetFocus, this);
	onGetFocusByKeyboard.Connect(&Cursor::OnGetFocusByKeyboard, this);
	onLoseFocus.Connect(&Cursor::OnLoseFocus, this);

	getContextMenu.Connect(&Cursor::GetContextMenu, this);

	onInput.SetParentObject(this);
	onEnter.SetParentObject(this);
	onScroll.SetParentObject(this);

	AddHistoryEntry();
}

S::GUI::Cursor::~Cursor()
{
	if (contextMenu != NIL) DeleteObject(contextMenu);

	if (timer != NIL) DeleteObject(timer);
}

S::Int S::GUI::Cursor::Paint(Int message)
{
	if (!IsRegistered()) return Error();
	if (!IsVisible())    return Success();

	switch (message)
	{
		case SP_PAINT:
			DrawWidget();

			break;
		case SP_MOUSEIN:
			Input::Pointer::SetCursor(container->GetContainerWindow(), Input::Pointer::CursorTextEdit);

			break;
		case SP_MOUSEOUT:
			Input::Pointer::SetCursor(container->GetContainerWindow(), Input::Pointer::CursorArrow);

			break;
	}

	return Success();
}

S::Int S::GUI::Cursor::Process(Int message, Int wParam, Int lParam)
{
	if (!IsRegistered()) return Error();
	if (!IsVisible())    return Success();

	Window	*window	= container->GetContainerWindow();

	if (window == NIL) return Success();

	switch (message)
	{
		case SM_LBUTTONDOWN:
			if (mouseOver)
			{
				Rect	 frame = Rect(GetRealPosition(), GetRealSize());

				String	 wText = text;
				Int	 wPromptPos = 0;

				if (Binary::IsFlagSet(GetFlags(), CF_MULTILINE))
				{
					Int	 line	   = (window->GetMousePosition().y - frame.top) / (font.GetScaledTextSizeY() + 3);
					Int	 lineCount = 0;
					Int	 length	   = text.Length();

					for (Int i = 0; i <= length; i++)
					{
						wText[i - wPromptPos] = text[i];

						if (text[i] == '\n' || text[i] == 0)
						{
							wText[i - wPromptPos] = 0;

							if (lineCount - scrollPos == line || text[i] == 0) break;

							wPromptPos = i + 1;
							lineCount++;
						}
					}
				}

				Int	 newPromptPos = GetLogicalCursorPositionFromDisplay(wText, window->GetMousePosition().x - frame.left) + wPromptPos;

				marking = True;

				if (Input::Keyboard::GetKeyState(Input::Keyboard::KeyShift))
				{
					if (markStart == -1) markStart = GetCursorPos();
					if (markStart == -1) markStart = newPromptPos;

					MarkText(markStart, newPromptPos);
				}
				else
				{
					MarkText(newPromptPos, newPromptPos);
				}

				SetCursorPos(newPromptPos);

				if (!focussed)
				{
					focussed = True;

					onGetFocus.Emit();
				}
				else
				{
					onClickInFocus.Emit();
				}
			}
			else
			{
				if (focussed)
				{
					focussed = False;

					onLoseFocus.Emit();
				}
			}

			break;
		case SM_LBUTTONUP:
			if (focussed && markStart != -1)
			{
				if (markStart == markEnd) { markStart = -1; markEnd = -1; }

				marking = False;
			}

			break;
		case SM_LBUTTONDBLCLK:
			if (focussed)
			{
				/* Compute word start and end.
				 */
				Int	 length	   = text.Length();

				Int	 wordStart = 0;
				Int	 wordEnd   = length;

				for (Int i = promptPos - 2; i >= 0; i--)
				{
					if ( (						   text[i + 1] == '\n') ||
					    ((text[i    ] == ' ' || text[i    ] == '\t' || text[i    ] == '\n') &&
					     (text[i + 1] != ' ' && text[i + 1] != '\t' && text[i + 1] != '\n'))) { wordStart = i + 1; break; }
				}

				for (Int i = promptPos - 1; i < length; i++)
				{
					if ( (						   text[i + 1] == '\n') ||
					    ((text[i    ] == ' ' || text[i    ] == '\t' || text[i    ] == '\n') &&
					     (text[i + 1] != ' ' && text[i + 1] != '\t' && text[i + 1] != '\n'))) { wordEnd = i + 1; break; }
				}

				/* Set mark region.
				 */
				if (Input::Keyboard::GetKeyState(Input::Keyboard::KeyShift))
				{
					if (markStart == -1) markStart = promptPos;

					if (promptPos <= markStart)
					{
						MarkText(markStart, wordStart);

						SetCursorPos(wordStart);
					}
					else
					{
						MarkText(markStart, wordEnd);

						SetCursorPos(wordEnd);
					}
				}
				else
				{
					MarkText(wordStart, wordEnd);

					SetCursorPos(wordEnd);
				}
			}

			break;
		case SM_RBUTTONDOWN:
		case SM_RBUTTONDBLCLK:
			if (mouseOver)
			{
				/* Activate this widget before opening the context menu.
				 */
				if (!focussed)
				{
					window->Process(SM_LBUTTONDOWN, 0, 0);
					window->Process(SM_LBUTTONUP, 0, 0);
				}

				/* Enable the context menu even for inactive widgets.
				 */
				if (!IsActive())
				{
					OpenContextMenu();

					/* Force mouseOut event.
					 */
					window->Process(SM_MOUSEMOVE, 0, 0);
				}
			}

			break;
		case SM_MBUTTONDOWN:
			if (focussed && IsActive() && Clipboard(container->GetContainerWindow()).GetSelectionText() != NIL)
			{
				Bool	 selected = (markStart != markEnd && markStart >= 0 && markEnd >= 0);

				flags |= CF_NOINPUT;

				DeleteSelectedText();

				flags ^= CF_NOINPUT;

				String	 insertText = Clipboard(container->GetContainerWindow()).GetSelectionText();

				if (insertText.Length() > 0 && (insertText.Length() + text.Length()) <= maxSize)
				{
					if (Binary::IsFlagSet(container->GetFlags(), EDB_NUMERIC) && (insertText.ToInt() == 0 && insertText[0] != '0')) break;

					if (selected) RemoveHistoryEntry();

					InsertText(insertText);
				}

				onInput.Emit(text);
			}

			break;
		case SM_MOUSEMOVE:
			{
				Rect	 frame = Rect(GetRealPosition(), GetRealSize());

				if (!mouseOver && window->IsMouseOn(frame))
				{
					mouseOver = True;

					Paint(SP_MOUSEIN);
				}
				else if (mouseOver && !window->IsMouseOn(frame))
				{
					mouseOver = False;

					Paint(SP_MOUSEOUT);
				}

				if (focussed && markStart != -1 && marking)
				{
					String	 wText = text;
					Int	 wPromptPos = 0;

					if (Binary::IsFlagSet(GetFlags(), CF_MULTILINE))
					{
						Int	 line	   = (window->GetMousePosition().y - frame.top) / (font.GetScaledTextSizeY() + 3);
						Int	 lineCount = 0;
						Int	 length	   = text.Length();

						if (window->GetMousePosition().y - frame.top < 0) line--;

						for (Int i = 0; i <= length; i++)
						{
							wText[i - wPromptPos] = text[i];

							if (text[i] == '\n' || text[i] == 0)
							{
								wText[i - wPromptPos] = 0;

								if (lineCount - scrollPos == line || text[i] == 0) break;

								wPromptPos = i + 1;
								lineCount++;
							}
						}

						if (lineCount - scrollPos != line) wPromptPos = -1;
					}

					if (wPromptPos >= 0)
					{
						Int	 newMarkEnd = GetLogicalCursorPositionFromDisplay(wText, window->GetMousePosition().x - frame.left) + wPromptPos;

						MarkText(markStart, newMarkEnd);

						SetCursorPos(newMarkEnd);
					}
				}
			}

			break;
		case SM_KEYDOWN:
			if (OnSpecialKey(wParam)) return MessageProcessed;

			break;
		case SM_CHAR:
			OnInput(wParam, lParam);

			break;
	}

	return Widget::Process(message, wParam, lParam);
}

S::Int S::GUI::Cursor::DrawWidget()
{
	Int	 nMaxScrollPos = Math::Max(0, (Int) Math::Ceil((Float) (font.GetUnscaledTextSizeY(text) - GetHeight()) / (font.GetUnscaledTextSizeY() + 3)));

	if (nMaxScrollPos != maxScrollPos)
	{
		maxScrollPos = nMaxScrollPos;

		if (maxScrollPos == 0) scrollPos = 0;

		onScroll.Emit(scrollPos, maxScrollPos);
	}

	Int	 textLength = text.Length();

	if (lineIndices.Length() == 0)
	{
		lineIndices.Add(0);

		for (Int i = 0; i <= textLength; i++)
		{
			if (text[i] != '\n' && text[i] != 0) continue;

			lineIndices.Add(i + 1);
		}
	}

	Surface	*surface = GetDrawSurface();
	Point	 realPos = GetRealPosition();
	Rect	 frame	 = Rect(realPos, GetRealSize());

	surface->StartPaint(GetVisibleArea());

	surface->Box(frame, GetBackgroundColor(), Rect::Filled);

	Int	 lineNumber = scrollPos;

	for (Int lineStart = lineIndices.GetNth(lineNumber); lineStart <= textLength; lineStart = lineIndices.GetNth(++lineNumber))
	{
		if ((lineNumber - scrollPos) * (font.GetUnscaledTextSizeY() + 3) >= frame.GetHeight()) break;

		Int	 lineLength = lineIndices.GetNth(lineNumber + 1) - lineStart - 1;
		String	 line	    = text.SubString(lineStart, lineLength);

		if (!Binary::IsFlagSet(container->GetFlags(), EDB_ASTERISK)) surface->SetText(ConvertTabs(line, tabSize),      frame + Point(-visibleOffset, (lineNumber - scrollPos) * (font.GetScaledTextSizeY() + 3)) + Point(0, 1) * surface->GetSurfaceDPI() / 96.0 + Size(visibleOffset, -2), font);
		else							     surface->SetText(String().FillN('*', lineLength), frame + Point(-visibleOffset, (lineNumber - scrollPos) * (font.GetScaledTextSizeY() + 3)) + Point(0, 1) * surface->GetSurfaceDPI() / 96.0 + Size(visibleOffset, -2), font);

		if (markStart != markEnd && markStart >= 0 && markEnd >= 0)
		{
			Int	 lineMarkStart = Math::Max(0, Math::Min(markStart, markEnd) - lineStart);
			Int	 lineMarkEnd   = Math::Min(lineLength, Math::Max(markStart, markEnd) - lineStart);

			if (lineMarkStart < lineLength && lineMarkEnd > 0)
			{
				Array<Int>	 markRegionStarts;
				Array<Int>	 markRegionEnds;

				if (Setup::useIconv && ContainsRTLCharacters(line))
				{
					FriBidiStrIndex	 length = lineLength;

					/* Get visual positions.
					 */
					FriBidiStrIndex	*positions = new FriBidiStrIndex [length + 1];
					FriBidiParType	 type = (IsRightToLeft() ? FRIBIDI_PAR_RTL : FRIBIDI_PAR_LTR);

					fribidi_log2vis((FriBidiChar *) line.ConvertTo("UCS-4LE"), length, &type, NIL, positions, NIL, NIL);

					/* Find marked regions in visual string.
					 */
					for (Int i = lineMarkStart; i < lineMarkEnd; i++)
					{
						Bool	 done = False;

						for (Int j = 0; j < markRegionStarts.Length(); j++)
						{
							if	(markRegionStarts.GetNth(j) == positions[i] + 1) { markRegionStarts.SetNth(j, markRegionStarts.GetNth(j) - 1); done = True; break; }
							else if (markRegionEnds.GetNth(j)   == positions[i]    ) { markRegionEnds.SetNth(j, markRegionEnds.GetNth(j) + 1);     done = True; break; }
						}

						if (!done)
						{
							markRegionStarts.Add(positions[i]);
							markRegionEnds.Add(positions[i] + 1);
						}
					}

					delete [] positions;

					/* Consolidate adjacent regions into one.
					 */
					for (Int i = 0; i < markRegionStarts.Length() - 1; i++)
					{
						for (Int j = i + 1; j < markRegionStarts.Length(); j++)
						{
							if (markRegionStarts.GetNth(i) < markRegionStarts.GetNth(j) &&
							    markRegionEnds.GetNth(i) >= markRegionStarts.GetNth(j))
							{
								markRegionEnds.SetNth(i, markRegionEnds.GetNth(j));

								markRegionStarts.RemoveNth(j);
								markRegionEnds.RemoveNth(j);

								j--;
							}
							else if (markRegionStarts.GetNth(j) < markRegionStarts.GetNth(i) &&
								 markRegionEnds.GetNth(j) >= markRegionStarts.GetNth(i))
							{
								markRegionStarts.SetNth(i, markRegionStarts.GetNth(j));

								markRegionStarts.RemoveNth(j);
								markRegionEnds.RemoveNth(j);

								j--;
							}
						}
					}
				}
				else
				{
					markRegionStarts.Add(lineMarkStart);
					markRegionEnds.Add(lineMarkEnd);
				}

				/* Draw marked regions.
				 */
				for (Int i = 0; i < markRegionStarts.Length(); i++)
				{
					Int	 markRegionStart = GetDisplayCursorPositionFromVisual(line, markRegionStarts.GetNth(i));
					Int	 markRegionEnd	 = GetDisplayCursorPositionFromVisual(line, markRegionEnds.GetNth(i));

					Rect	 markRect = Rect(realPos + Point(Math::Min(markRegionStart, markRegionEnd), (lineNumber - scrollPos) * (font.GetScaledTextSizeY() + 3)) + Point(0, 1) * surface->GetSurfaceDPI() / 96.0 - Point(0, 1), Size(Math::Abs(markRegionEnd - markRegionStart), font.GetScaledTextSizeY() + 3));
					Font	 nFont = font;

					nFont.SetColor(Setup::HighlightTextColor);

					surface->StartPaint(markRect);

					surface->Box(markRect, Setup::HighlightColor, Rect::Filled);

					if (!Binary::IsFlagSet(container->GetFlags(), EDB_ASTERISK)) surface->SetText(ConvertTabs(line, tabSize),      frame + Point(-visibleOffset, (lineNumber - scrollPos) * (font.GetScaledTextSizeY() + 3)) + Point(0, 1) * surface->GetSurfaceDPI() / 96.0 + Size(visibleOffset, -2), nFont);
					else							     surface->SetText(String().FillN('*', lineLength), frame + Point(-visibleOffset, (lineNumber - scrollPos) * (font.GetScaledTextSizeY() + 3)) + Point(0, 1) * surface->GetSurfaceDPI() / 96.0 + Size(visibleOffset, -2), nFont);

					surface->EndPaint();
				}
			}
		}
	}

	surface->EndPaint();

	return Success();
}

S::Void S::GUI::Cursor::ShowCursor(Bool visible)
{
	/* Cancel if cursor is displayed by the IME.
	 */
	if (imeCursor) return;

	/* Cancel if cursor is already in the desired state.
	 */
	if (promptVisible == visible) return;

	Surface	*surface = GetDrawSurface();
	Point	 point	 = GetRealPosition();

	Int	 line = 0;

	if (Binary::IsFlagSet(GetFlags(), CF_MULTILINE))
	{
		for (Int j = promptPos - 1; j >= 0; j--)
		{
			if (text[j] == '\n') line++;
		}
	}

	point.x += GetDisplayCursorPositionFromLogical(promptPos) + imeAdvance;
	point.y += (font.GetScaledTextSizeY() + 3) * (line - scrollPos);

	if (!(Binary::IsFlagSet(GetFlags(), CF_MULTILINE) && (line - scrollPos < 0 || (font.GetUnscaledTextSizeY() + 3) * (line - scrollPos + 1) > GetHeight())))
	{
		if (IsVisible()) surface->Box(Rect(point, Size(1, font.GetScaledTextSizeY() + 3)), 0, Rect::Inverted);

		promptVisible = visible;
	}
}

S::Int S::GUI::Cursor::SetText(const String &newText)
{
	if (text == newText) return Success();

	Surface	*surface = GetDrawSurface();
	Rect	 frame	 = Rect(GetRealPosition(), GetRealSize());

	Bool	 visible = IsVisible();

	if (visible) surface->StartPaint(frame);

	ShowCursor(False);

	MarkText(-1, -1);

	lineIndices.RemoveAll();

	if (timer != NIL)
	{
		DeleteObject(timer);

		timer = NIL;
	}

	promptPos	= newText.Length();
	visibleOffset	= 0;
	scrollPos	= 0;

	ClearHistory();

	Widget::SetText(newText);

	if (IsActive()) AddHistoryEntry();

	if (visible) surface->EndPaint();

	return Success();
}

S::Void S::GUI::Cursor::MarkText(Int newMarkStart, Int newMarkEnd)
{
	if (newMarkStart == markStart && newMarkEnd == markEnd) return;

	ShowCursor(False);

	markStart	= newMarkStart;
	markEnd		= newMarkEnd;

	Paint(SP_PAINT);

	/* Copy selected text to selection clipboard.
	 */
	if (Math::Abs(markEnd - markStart) > 0) Clipboard(container->GetContainerWindow()).SetSelectionText(text.SubString(Math::Min(markStart, markEnd), Math::Abs(markEnd - markStart)));
}

S::Int S::GUI::Cursor::MarkAll()
{
	MarkText(0, text.Length());

	SetCursorPos(text.Length());

	return Success();
}

S::Void S::GUI::Cursor::InsertText(const String &insertText)
{
	ShowCursor(False);

	Int	 textLength   = text.Length();
	Int	 insertLength = insertText.Length();

	/* Build resulting text value.
	 */
	String	 newText;

	for (Int i = 0;		i <  promptPos;		       i++) newText[i] = text[i];
	for (Int i = promptPos; i <  promptPos + insertLength; i++) newText[i] = insertText[i - promptPos];
	for (Int i = promptPos; i <=		 textLength;   i++) newText[i + insertLength] = text[i];

	/* Draw widget with new content.
	 */
	Bool	 prevVisible = IsVisible();

	visible = False;

	lineIndices.RemoveAll();

	Widget::SetText(newText);

	visible = prevVisible;

	/* Update screen.
	 */
	Surface	*surface = GetDrawSurface();

	surface->StartPaint(Rect(container->GetRealPosition(), container->GetRealSize()));

	container->Paint(SP_PAINT);

	surface->EndPaint();

	/* Update cursor position.
	 */
	SetCursorPos(promptPos + insertLength);

	AddHistoryEntry();
}

S::Void S::GUI::Cursor::CopyToClipboard()
{
	if (markStart != markEnd)
	{
		Clipboard(container->GetContainerWindow()).SetClipboardText(text.SubString(Math::Min(markStart, markEnd), Math::Abs(markEnd - markStart)));
	}
}

S::Void S::GUI::Cursor::InsertFromClipboard()
{
	Bool	 selected = (markStart != markEnd && markStart >= 0 && markEnd >= 0);

	flags |= CF_NOINPUT;

	DeleteSelectedText();

	flags ^= CF_NOINPUT;

	String	 insertText = Clipboard(container->GetContainerWindow()).GetClipboardText();

	if (insertText.Length() > 0 && (insertText.Length() + text.Length()) <= maxSize)
	{
		if (Binary::IsFlagSet(container->GetFlags(), EDB_NUMERIC) && (insertText.ToInt() == 0 && insertText[0] != '0')) return;

		if (selected) RemoveHistoryEntry();

		InsertText(insertText);
	}

	onInput.Emit(text);
}

S::Void S::GUI::Cursor::DeleteSelectedText()
{
	if (markStart == markEnd || markStart < 0 || markEnd < 0) return;

	ShowCursor(False);

	Int	 bMarkStart	= Math::Min(markStart, markEnd);
	Int	 bMarkEnd	= Math::Max(markStart, markEnd);

	Bool	 prevVisible = IsVisible();

	visible = False;

	MarkText(-1, -1);

	String	 newText;

	for (Int i = 0; i < bMarkStart; i++)		newText[i] = text[i];
	for (Int j = bMarkEnd; j <= text.Length(); j++)	newText[j - (bMarkEnd - bMarkStart)] = text[j];

	lineIndices.RemoveAll();

	Widget::SetText(newText);

	visible = prevVisible;

	Surface	*surface = GetDrawSurface();

	surface->StartPaint(Rect(container->GetRealPosition(), container->GetRealSize()));

	container->Paint(SP_PAINT);

	surface->EndPaint();

	SetCursorPos(bMarkStart);

	AddHistoryEntry();

	if(!Binary::IsFlagSet(GetFlags(), CF_NOINPUT)) onInput.Emit(newText);
}

S::Int S::GUI::Cursor::Scroll(Int nScrollPos)
{
	ShowCursor(False);

	scrollPos = Math::Max(0, Math::Min(nScrollPos, maxScrollPos));

	Paint(SP_PAINT);

	onScroll.Emit(scrollPos, maxScrollPos);

	return Success();
}

S::Void S::GUI::Cursor::OnTimer()
{
	ShowCursor(!promptVisible);
}

S::Void S::GUI::Cursor::OnGetFocus()
{
	SetCursorPos(promptPos);
}

S::Void S::GUI::Cursor::OnGetFocusByKeyboard()
{
	MarkAll();
}

S::Void S::GUI::Cursor::OnLoseFocus()
{
	ShowCursor(False);

	MarkText(-1, -1);

	if (timer != NIL)
	{
		DeleteObject(timer);

		timer = NIL;
	}

	/* Notify listeners of removed cursor.
	 */
	internalRemoveCursor.Emit(this);
}

S::Bool S::GUI::Cursor::OnSpecialKey(Int keyCode)
{
	/* Called when a special key such as
	 * return or an arrow key is hit.
	 */
	if (!focussed) return False;

#if defined __APPLE__
	Input::Keyboard::Key	 commandKey = Input::Keyboard::KeyCommand;
#elif defined __HAIKU__
	Input::Keyboard::Key	 commandKey = Input::Keyboard::KeyAlt;
#else
	Input::Keyboard::Key	 commandKey = Input::Keyboard::KeyControl;
#endif

	/* Make sure no other modifiers are pressed along with command key.
	 */
	if (Input::Keyboard::GetKeyState(commandKey) && !(Input::Keyboard::GetKeyState(Input::Keyboard::KeyCommand) ^
							  Input::Keyboard::GetKeyState(Input::Keyboard::KeyControl) ^
							  Input::Keyboard::GetKeyState(Input::Keyboard::KeyAlt))) return False;

	/* Evaluate key code.
	 */
	Int	 newPos	 = 0;
	Int	 linePos = 0;

	Int	 i	 = 0;

	switch (keyCode)
	{
		case Input::Keyboard::KeyLeft:
		case Input::Keyboard::KeyRight:
			if (!Input::Keyboard::GetKeyState(Input::Keyboard::KeyShift)) MarkText(-1, -1);

			/* Calculate new cursor pos.
			 */
			if (Input::Keyboard::GetKeyState(commandKey))
			{
				Bool	 isLeft = ((keyCode == Input::Keyboard::KeyLeft  && !IsRightToLeft()) || (keyCode == Input::Keyboard::KeyRight && IsRightToLeft()));

				if (isLeft)
				{
					newPos = 0;

					for (Int i = promptPos - 2; i >= 0; i--)
					{
						if ( (						   text[i + 1] == '\n') ||
						    ((text[i    ] == ' ' || text[i    ] == '\t' || text[i    ] == '\n') &&
						     (text[i + 1] != ' ' && text[i + 1] != '\t' && text[i + 1] != '\n'))) { newPos = i + 1; break; }
					}
				}
				else
				{
					newPos = text.Length();

					for (Int i = promptPos; i < text.Length(); i++)
					{
						if ( (						   text[i + 1] == '\n') ||
						    ((text[i    ] == ' ' || text[i    ] == '\t' || text[i    ] == '\n') &&
						     (text[i + 1] != ' ' && text[i + 1] != '\t' && text[i + 1] != '\n'))) { newPos = i + 1; break; }
					}
				}
			}
			else
			{
				Bool	 isLeft = ((keyCode == Input::Keyboard::KeyLeft  && !IsRightToLeft()) || (keyCode == Input::Keyboard::KeyRight && IsRightToLeft()));

				if (isLeft) { if (promptPos == 0)	      break; newPos = promptPos - 1; }
				else	    { if (promptPos >= text.Length()) break; newPos = promptPos + 1; }
			}

			/* Set mark region and cursor.
			 */
			if (Input::Keyboard::GetKeyState(Input::Keyboard::KeyShift))
			{
				if (markStart == -1) markStart = promptPos;

				MarkText(markStart, newPos);
			}

			SetCursorPos(newPos);

			break;
		case Input::Keyboard::KeyHome:
		case Input::Keyboard::KeyEnd:
			if (!Input::Keyboard::GetKeyState(Input::Keyboard::KeyShift)) MarkText(-1, -1);

			/* Calculate new cursor pos.
			 */
			if (Input::Keyboard::GetKeyState(commandKey))
			{
				if	(keyCode == Input::Keyboard::KeyHome)	{ if (promptPos == 0)		  break; newPos = 0;		 }
				else if (keyCode == Input::Keyboard::KeyEnd)	{ if (promptPos >= text.Length()) break; newPos = text.Length(); }
			}
			else
			{
				if (keyCode == Input::Keyboard::KeyHome)
				{
					newPos = 0;

					for (Int i = promptPos - 1; i >= 0; i--)
					{
						if (text[i] == '\n') { newPos = i + 1; break; }
					}
				}
				else
				{
					newPos = text.Length();

					for (Int i = promptPos; i < text.Length(); i++)
					{
						if (text[i] == '\n') { newPos = i; break; }
					}
				}
			}

			/* Set mark region and cursor.
			 */
			if (Input::Keyboard::GetKeyState(Input::Keyboard::KeyShift))
			{
				if (markStart == -1) markStart = promptPos;

				MarkText(markStart, newPos);
			}

			SetCursorPos(newPos);

			break;
		case Input::Keyboard::KeyUp:
		case Input::Keyboard::KeyDown:
			if (!Binary::IsFlagSet(GetFlags(), CF_MULTILINE)) break;

			if (!Input::Keyboard::GetKeyState(Input::Keyboard::KeyShift)) MarkText(-1, -1);

			/* Calculate new cursor pos.
			 */
			linePos	= promptPos;
			newPos	= promptPos;

			for (i = promptPos - 1; i >= 0; i--)
			{
				if (text[i] == '\n') { linePos -= (i + 1); break; }
			}

			if (keyCode == Input::Keyboard::KeyUp)
			{
				if (i == 0) newPos = 0;

				for (Int j = i - 1; j >= 0; j--)
				{
					if (text[j] == '\n' || j == 0) { newPos = Math::Min(i, j + linePos + (j == 0 ? 0 : 1)); break; }
				}
			}
			else
			{
				for (Int j = promptPos; j < text.Length(); j++)
				{
					if (text[j] == '\n')
					{
						for (Int k = j + 1; k <= j + 1 + linePos; k++)
						{
							newPos = k;

							if (text[k] == '\n' || text[k] == 0) break;
						}

						break;
					}
				}
			}

			/* Set mark region and cursor.
			 */
			if (Input::Keyboard::GetKeyState(Input::Keyboard::KeyShift))
			{
				if (markStart == -1) markStart = promptPos;

				MarkText(markStart, newPos);
			}

			SetCursorPos(newPos);

			break;
		case Input::Keyboard::KeyReturn:
			if (!IsActive()) break;

			if (Binary::IsFlagSet(GetFlags(), CF_MULTILINE))
			{
				Bool	 selected = (markStart != markEnd && markStart >= 0 && markEnd >= 0);

				flags |= CF_NOINPUT;

				DeleteSelectedText();

				flags ^= CF_NOINPUT;

				if (selected) RemoveHistoryEntry();

				String	 insertText;

				insertText[0] = '\n';

				InsertText(insertText);

				onInput.Emit(text);
			}
			else
			{
				focussed = False;

				onLoseFocus.Emit();

				onEnter.Emit(text);
			}

			break;
		case Input::Keyboard::KeyEscape:
			if (!IsActive()) break;

			focussed = False;

			onLoseFocus.Emit();

			return True;
		case Input::Keyboard::KeyInsert:
#ifndef __APPLE__
			if (		   Input::Keyboard::GetKeyState(commandKey) && !Input::Keyboard::GetKeyState(Input::Keyboard::KeyShift)) CopyToClipboard();
			if (IsActive() && !Input::Keyboard::GetKeyState(commandKey) &&  Input::Keyboard::GetKeyState(Input::Keyboard::KeyShift)) InsertFromClipboard();
#endif
			break;
		case Input::Keyboard::KeyBack:
			if (!IsActive() || Input::Keyboard::GetKeyState(commandKey) || Input::Keyboard::GetKeyState(Input::Keyboard::KeyShift)) break;

			if (markStart != markEnd) { DeleteSelectedText(); break; }

			if (promptPos == 0) break;

			markStart = promptPos - 1;
			markEnd	  = promptPos;

			DeleteSelectedText();

			break;
		case Input::Keyboard::KeyDelete:
			if (!IsActive() || Input::Keyboard::GetKeyState(commandKey)) break;
#ifdef __APPLE__
			if (Input::Keyboard::GetKeyState(Input::Keyboard::KeyShift)) break;
#endif
			if (markStart != markEnd)
			{
				if (Input::Keyboard::GetKeyState(Input::Keyboard::KeyShift)) CopyToClipboard();

				DeleteSelectedText();

				break;
			}

			if (Input::Keyboard::GetKeyState(Input::Keyboard::KeyShift)) break;

			if (promptPos == text.Length()) break;

			markStart = promptPos;
			markEnd	  = promptPos + 1;

			DeleteSelectedText();

			break;
		case Input::Keyboard::KeyA:
			if (		  Input::Keyboard::GetKeyState(commandKey))   MarkAll();

			break;
		case Input::Keyboard::KeyC:
			if (		  Input::Keyboard::GetKeyState(commandKey))   CopyToClipboard();

			break;
		case Input::Keyboard::KeyX:
			if (IsActive() && Input::Keyboard::GetKeyState(commandKey)) { CopyToClipboard(); DeleteSelectedText(); }

			break;
		case Input::Keyboard::KeyV:
			if (IsActive() && Input::Keyboard::GetKeyState(commandKey))   InsertFromClipboard();

			break;
		case Input::Keyboard::KeyZ:

			if (IsActive() && Input::Keyboard::GetKeyState(commandKey))   Undo();

			break;
		case Input::Keyboard::KeyY:
			if (IsActive() && Input::Keyboard::GetKeyState(commandKey))   Redo();

			break;
	}

	return False;
}

S::Void S::GUI::Cursor::OnInput(Int character, Int flags)
{
	/* Called when a character is entered.
	 */
	if (!focussed)					      return;
	if (text.Length() == maxSize && markStart == markEnd) return;

	if (character >= 0x20 && character != 0x7F && IsActive())
	{
		Bool	 selected = (markStart != markEnd && markStart >= 0 && markEnd >= 0);

		this->flags |= CF_NOINPUT;

		DeleteSelectedText();

		this->flags ^= CF_NOINPUT;

		if (!Binary::IsFlagSet(container->GetFlags(), EDB_NUMERIC) || (character >= '0' && character <= '9') || character == '-' || character == '.')
		{
			String	 insertText;

			insertText[0] = character;

			if (selected) RemoveHistoryEntry();

			InsertText(insertText);
		}

		onInput.Emit(text);
	}
}

S::Void S::GUI::Cursor::OnCut()
{
	CopyToClipboard();
	DeleteSelectedText();
}

S::Void S::GUI::Cursor::OnInsert()
{
	InsertFromClipboard();
}

S::Void S::GUI::Cursor::AddHistoryEntry()
{
	while (history.Length() > historyPos)
	{
		history.RemoveNth(history.Length() - 1);
		historyPrompt.RemoveNth(historyPrompt.Length() - 1);
	}

	history.Add(text);
	historyPrompt.Add(promptPos);
	historyPos++;
}

S::Void S::GUI::Cursor::RemoveHistoryEntry()
{
	history.RemoveNth(history.Length() - 1);
	historyPrompt.RemoveNth(historyPrompt.Length() - 1);
	historyPos--;
}

S::Void S::GUI::Cursor::ClearHistory()
{
	history.RemoveAll();
	historyPrompt.RemoveAll();
	historyPos = 0;
}

S::Void S::GUI::Cursor::Undo()
{
	if (historyPos <= 1) return;

	lineIndices.RemoveAll();

	Surface	*surface = GetDrawSurface();

	surface->StartPaint(GetVisibleArea());

	Int	 index	 = --historyPos - 1;

	Widget::SetText(history.GetNth(index));
	SetCursorPos(historyPrompt.GetNth(index));

	surface->EndPaint();

	onInput.Emit(text);
}

S::Void S::GUI::Cursor::Redo()
{
	if (historyPos >= history.Length()) return;

	lineIndices.RemoveAll();

	Surface	*surface = GetDrawSurface();

	surface->StartPaint(GetVisibleArea());

	Int	 index	 = historyPos++;

	Widget::SetText(history.GetNth(index));
	SetCursorPos(historyPrompt.GetNth(index));

	surface->EndPaint();

	onInput.Emit(text);
}

S::GUI::PopupMenu *S::GUI::Cursor::GetContextMenu()
{
	if (!IsActive() && text == NIL) return NIL;

	if (contextMenu == NIL) contextMenu = new PopupMenu();

	contextMenu->RemoveAllEntries();

	MenuEntry	*entryUndo    = contextMenu->AddEntry(I18n::Translator::defaultTranslator->TranslateString("Undo"));

	contextMenu->AddEntry();

	MenuEntry	*entryCut     = contextMenu->AddEntry(I18n::Translator::defaultTranslator->TranslateString("Cut"));
	MenuEntry	*entryCopy    = contextMenu->AddEntry(I18n::Translator::defaultTranslator->TranslateString("Copy"));
	MenuEntry	*entryInsert  = contextMenu->AddEntry(I18n::Translator::defaultTranslator->TranslateString("Paste"));
	MenuEntry	*entryDelete  = contextMenu->AddEntry(I18n::Translator::defaultTranslator->TranslateString("Clear"));

	contextMenu->AddEntry();

	MenuEntry	*entryMarkAll = contextMenu->AddEntry(I18n::Translator::defaultTranslator->TranslateString("Select all"));

	entryUndo->onAction.Connect(&Cursor::Undo, this);

	entryCut->onAction.Connect(&Cursor::OnCut, this);
	entryCopy->onAction.Connect(&Cursor::CopyToClipboard, this);
	entryInsert->onAction.Connect(&Cursor::OnInsert, this);
	entryDelete->onAction.Connect(&Cursor::DeleteSelectedText, this);

	entryMarkAll->onAction.Connect(&Cursor::MarkAll, this);

	/* Disable modification if inactive.
	 */
	if (!IsActive())
	{
		entryUndo->Deactivate();
		entryCut->Deactivate();
		entryInsert->Deactivate();
		entryDelete->Deactivate();
	}

	/* Disable copy and delete if nothing is selected.
	 */
	if (markStart == markEnd)
	{
		entryCut->Deactivate();
		entryCopy->Deactivate();
		entryDelete->Deactivate();
	}

	/* Disable "Select all" if everything is selected already.
	 */
	if ((markStart == 0 && markEnd == text.Length()) || text == NIL)
	{
		entryMarkAll->Deactivate();
	}

	/* Disable undo if no history is available.
	 */
	if (historyPos == 1)
	{
		entryUndo->Deactivate();
	}

	return contextMenu;
}

S::Int S::GUI::Cursor::SetCursorPos(Int newPos)
{
	focussed = True;

	ShowCursor(False);

	imeAdvance = 0;
	imeCursor  = False;

	Rect	 frame	 = Rect(GetRealPosition(), GetRealSize());
	Point	 p1	 = GetRealPosition();

	promptPos = newPos;

	/* Compute line number.
	 */
	Int	 line = 0;

	if (Binary::IsFlagSet(GetFlags(), CF_MULTILINE))
	{
		for (Int j = promptPos - 1; j >= 0; j--)
		{
			if (text[j] == '\n') line++;
		}
	}

	/* Compute cursor position on screen.
	 */
	p1.x += GetDisplayCursorPositionFromLogical(promptPos);

	while (p1.x > frame.right || p1.x < frame.left)
	{
		if	(p1.x > frame.right) { visibleOffset +=  5; p1.x -=  5; }
		else if (p1.x < frame.left)  { visibleOffset -= 20; p1.x += 20; }
	}

	if (visibleOffset < 0) { p1.x -= visibleOffset; visibleOffset = 0; }

	p1.y += (font.GetScaledTextSizeY() + 3) * (line - scrollPos);

	if (Binary::IsFlagSet(GetFlags(), CF_MULTILINE) && (line - scrollPos < 0 || (font.GetUnscaledTextSizeY() + 3) * (line - scrollPos + 1) > GetHeight()))
	{
		while (line - scrollPos < 0)							 scrollPos--;
		while ((font.GetUnscaledTextSizeY() + 3) * (line - scrollPos + 1) > GetHeight()) scrollPos++;
	}

	if (maxScrollPos > 0) onScroll.Emit(scrollPos, maxScrollPos);

	/* Notify listeners of new cursor position.
	 */
	if (IsActive()) internalSetCursor.Emit(this, p1 + Point(1, 2));

	/* Start blinking cursor timer.
	 */
	if (timer != NIL) DeleteObject(timer);

	timer = new System::Timer();

	timer->onInterval.Connect(&Cursor::OnTimer, this);
	timer->Start(500);

	/* Redraw and show cursor.
	 */
	Paint(SP_PAINT);

	ShowCursor(True);

	return Success();
}

S::Void S::GUI::Cursor::SetMaxSize(Int newMaxSize)
{
	if (newMaxSize <= 0) maxSize = 32768;
	else		     maxSize = newMaxSize;
}

S::Void S::GUI::Cursor::SetIMEAdvance(Int newIMEAdvance)
{
	ShowCursor(False);

	imeAdvance = newIMEAdvance;
}

S::Void S::GUI::Cursor::SetIMECursor(Bool newIMECursor)
{
	ShowCursor(False);

	imeCursor = newIMECursor;
}

/* Returns the display cursor position
 * for a given logical cursor position.
 */
S::Int S::GUI::Cursor::GetDisplayCursorPositionFromLogical(Int promptPos) const
{
	FriBidiStrIndex	 length = text.Length();

	if (length == 0) return 0;

	String	 wText = text;
	Int	 wPromptPos = promptPos;

	if (Binary::IsFlagSet(GetFlags(), CF_MULTILINE))
	{
		Int	 lineStart  = promptPos;
		Int	 lineLength = 0;

		for (Int i = promptPos - 1; i >= 0;	 i--) { if (text[i] == '\n')		     break; lineStart--;  }
		for (Int i = lineStart;	    i <  length; i++) { if (text[i] == '\n' || text[i] == 0) break; lineLength++; }

		wText = text.SubString(lineStart, lineLength);
		wPromptPos = promptPos - lineStart;
	}

	return GetDisplayCursorPositionFromLogical(wText, wPromptPos);
}

/* Returns the display cursor position
 * for a given logical cursor position.
 */
S::Int S::GUI::Cursor::GetDisplayCursorPositionFromLogical(const String &line, Int promptPos) const
{
	return GetDisplayCursorPositionFromVisual(line, GetVisualCursorPositionFromLogical(line, promptPos));
}

/* Returns the display cursor position
 * for a given visual cursor position.
 */
S::Int S::GUI::Cursor::GetDisplayCursorPositionFromVisual(const String &line, Int promptPos) const
{
	FriBidiStrIndex	 length = line.Length();

	if (length == 0) return 0;

	Int		 position = 0;

	if (!Binary::IsFlagSet(container->GetFlags(), EDB_ASTERISK))
	{
		String	 string = line;

		if (Setup::useIconv && ContainsRTLCharacters(line))
		{
			FriBidiChar	*visual = new FriBidiChar [length + 1];
			FriBidiParType	 type = (IsRightToLeft() ? FRIBIDI_PAR_RTL : FRIBIDI_PAR_LTR);

			fribidi_log2vis((FriBidiChar *) string.ConvertTo("UCS-4LE"), length, &type, visual, NIL, NIL, NIL);

			visual[length] = 0;

			string.ImportFrom("UCS-4LE", (char *) visual);

			delete [] visual;
		}

		/* Convert tabs.
		 */
		Int	 offset = 0;

		for (Int i = 0; i < length; i++)
		{
			if (string[i] != '\t') continue;

			Int	 spaces = tabSize - i % tabSize;

			string  = string.Head(i).Append(String().FillN(' ', spaces)).Append(string.Tail(length - i - 1));
			length += spaces - 1;

			if (i < promptPos + offset) offset += spaces - 1;
		}

		if (!IsRightToLeft()) position += font.GetScaledTextSizeX(string.Head(	       promptPos + offset)) - visibleOffset;
		else		      position += font.GetScaledTextSizeX(string.Tail(length - promptPos - offset)) - visibleOffset;
	}
	else
	{
		position += font.GetScaledTextSizeX(String().FillN('*', promptPos)) - visibleOffset;
	}

	return position;
}

/* Return the logical cursor position
 * for a given display cursor position.
 */
S::Int S::GUI::Cursor::GetLogicalCursorPositionFromDisplay(const String &line, Int displayPos) const
{
	FriBidiStrIndex	 length = line.Length();

	if (length == 0) return 0;

	/* Find best position using binary search.
	 */
	Int	 bestPos   = 0;
	Int	 bestValue = 2147483647;

	Int	 startPos  = 0;
	Int	 endPos	   = length;

	while (endPos >= startPos)
	{
		/* Select element to compare and perform comparison.
		 */
		Int	 m   = (startPos + endPos) / 2;
		Int	 pos = GetDisplayCursorPositionFromVisual(line, m);

		if ((!IsRightToLeft() && pos > displayPos) ||
		    ( IsRightToLeft() && pos < displayPos)) endPos   = m - 1;
		else					    startPos = m + 1;

		if (Math::Abs(pos - displayPos) < bestValue)
		{
			bestPos	  = m;
			bestValue = Math::Abs(pos - displayPos);
		}
	}

	return GetLogicalCursorPositionFromVisual(line, bestPos);
}

/* Returns the cursor position in the visual string
 * for a given logical cursor position.
 */
S::Int S::GUI::Cursor::GetVisualCursorPositionFromLogical(const String &line, Int n) const
{
	FriBidiStrIndex	 length = line.Length();

	if (length == 0) return 0;

	Int		 position = n;

	/* Check if the input string contains RTL characters.
	 */
	if (Setup::useIconv && ContainsRTLCharacters(line))
	{
		/* Get BiDi types for input string.
		 */
		FriBidiCharType	*types = new FriBidiCharType [length + 1];

		fribidi_get_bidi_types((FriBidiChar *) line.ConvertTo("UCS-4LE"), length, types);

		/* Get visual cursor position.
		 */
		FriBidiStrIndex	*positions = new FriBidiStrIndex [length + 1];
		FriBidiParType	 type = (IsRightToLeft() ? FRIBIDI_PAR_RTL : FRIBIDI_PAR_LTR);

		fribidi_log2vis((FriBidiChar *) line.ConvertTo("UCS-4LE"), length, &type, NIL, positions, NIL, NIL);

		if (n == length)
		{
			if	( FRIBIDI_IS_RTL(types[n - 1])) position = positions[n - 1];
			else					position = positions[n - 1] + 1;
		}
		else if (n == 0)
		{
			if	( FRIBIDI_IS_RTL(types[n    ]))	position = positions[n    ] + 1;
			else					position = positions[n    ];
		}
		else
		{
			if	( FRIBIDI_IS_RTL(types[n    ]))	position = positions[n    ] + 1;
			else if ( FRIBIDI_IS_RTL(types[n - 1])) position = positions[n - 1];
			else if ( FRIBIDI_IS_SPACE(types[n]) &&
				 !FRIBIDI_IS_RTL(types[n - 1])) position = positions[n - 1] + 1;
			else					position = positions[n    ];
		}

		delete [] types;
		delete [] positions;
	}

	return position;
}

/* Returns the cursor position in the logical string
 * for a given visual cursor position.
 */
S::Int S::GUI::Cursor::GetLogicalCursorPositionFromVisual(const String &line, Int n) const
{
	FriBidiStrIndex	 length = line.Length();

	if (length == 0) return 0;

	Int		 position = n;

	/* Check if the input string contains RTL characters.
	 */
	if (Setup::useIconv && ContainsRTLCharacters(line))
	{
		/* Get BiDi types for input string.
		 */
		FriBidiCharType	*types = new FriBidiCharType [length + 1];

		fribidi_get_bidi_types((FriBidiChar *) line.ConvertTo("UCS-4LE"), length, types);

		/* Get visual cursor position.
		 */
		FriBidiStrIndex	*positions = new FriBidiStrIndex [length + 1];
		FriBidiParType	 type = (IsRightToLeft() ? FRIBIDI_PAR_RTL : FRIBIDI_PAR_LTR);

		fribidi_log2vis((FriBidiChar *) line.ConvertTo("UCS-4LE"), length, &type, NIL, NIL, positions, NIL);

		if (n == length)
		{
			if	( FRIBIDI_IS_RTL(types[n - 1])) position = positions[n - 1];
			else					position = positions[n - 1] + 1;
		}
		else if (n == 0)
		{
			if	( FRIBIDI_IS_RTL(types[n    ]))	position = positions[n    ] + 1;
			else					position = positions[n    ];
		}
		else
		{
			if	( FRIBIDI_IS_RTL(types[n    ]))	position = positions[n    ] + 1;
			else if ( FRIBIDI_IS_RTL(types[n - 1])) position = positions[n - 1];
			else if ( FRIBIDI_IS_SPACE(types[n]) &&
				 !FRIBIDI_IS_RTL(types[n - 1])) position = positions[n - 1] + 1;
			else					position = positions[n    ];
		}

		delete [] types;
		delete [] positions;
	}

	return position;
}

/* Convert tabs in a string.
 */
S::String S::GUI::Cursor::ConvertTabs(const String &line, Int tabSize)
{
	String	 string	      = line;
	Int	 stringLength = string.Length();

	for (Int i = 0; i < stringLength; i++)
	{
		if (string[i] != '\t') continue;

		Int	 spaces = tabSize - i % tabSize;

		string	      = string.Head(i).Append(String().FillN(' ', spaces)).Append(string.Tail(stringLength - i - 1));
		stringLength += spaces - 1;
	}

	return string;
}

/* Check if the string contains right-to-left characters.
 */
S::Bool S::GUI::Cursor::ContainsRTLCharacters(const String &line)
{
	FriBidiStrIndex	 length = line.Length();

	if (length == 0) return False;

	Bool		 rtlCharacters = False;

	if (Setup::useIconv)
	{
		/* Get BiDi types for input string.
		 */
		FriBidiCharType	*types = new FriBidiCharType [length + 1];

		fribidi_get_bidi_types((FriBidiChar *) line.ConvertTo("UCS-4LE"), length, types);

		/* Check if the input string contains RTL characters.
		 */
		for (Int i = 0; i < length; i++)
		{
			if (FRIBIDI_IS_RTL(types[i])) { rtlCharacters = True; break; }
		}

		delete [] types;
	}
	else
	{
		for (Int i = 0; i < length; i++)
		{
			if (line[i] >= 0x0590 && line[i] <= 0x08FF) { rtlCharacters = True; break; }
		}
	}

	return rtlCharacters;
}
