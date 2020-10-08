 /* The smooth Class Library
  * Copyright (C) 1998-2020 Robert Kausch <robert.kausch@gmx.net>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of "The Artistic License, Version 2.0".
  *
  * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

#include <smooth/gui/widgets/multi/menu/micromenu.h>
#include <smooth/gui/widgets/multi/menu/popupmenu.h>
#include <smooth/gui/widgets/hotspot/simplebutton.h>
#include <smooth/gui/window/window.h>
#include <smooth/graphics/surface.h>
#include <smooth/misc/math.h>
#include <smooth/system/screen.h>

const S::Short	 S::GUI::MicroMenu::classID = S::Object::RequestClassID();

S::GUI::MicroMenu::MicroMenu(const Point &iPos, const Size &iSize, Int sType)
{
	type	= classID;
	subtype	= sType;

	SetMetrics(iPos, iSize);

	if (GetWidth() == 0) SetWidth(subtype == OR_VERT ? 13 : 7);
	if (GetHeight() == 0) SetHeight(subtype == OR_VERT ? 7 : 13);

	onOpenPopupMenu.SetParentObject(this);
	onClosePopupMenu.SetParentObject(this);

	hotspot	= new HotspotSimpleButton(Point(), GetSize());

	hotspot->onLeftButtonDown.Connect(&MicroMenu::OpenPopupMenu, this);

	Add(hotspot);

	popup = new PopupMenu();
}

S::GUI::MicroMenu::~MicroMenu()
{
	DeleteObject(popup);

	DeleteObject(hotspot);
}

S::GUI::MenuEntry *S::GUI::MicroMenu::AddEntryInternal(const String &text, const Bitmap &bitmap, PopupMenu *popupMenu, Bool *bVar, Int *iVar, Int iCode)
{
	if	(bVar != NIL) return popup->AddEntry(text, bVar);
	else if (iVar != NIL) return popup->AddEntry(text, iVar, iCode);

	return popup->AddEntry(text, bitmap, popupMenu);
}

S::Int S::GUI::MicroMenu::Remove(Widget *widget)
{
	if (widget->GetObjectType() == Entry::classID) return popup->Remove(widget);

	return Menu::Remove(widget);
}

S::Int S::GUI::MicroMenu::RemoveAllEntries()
{
	return popup->RemoveAllEntries();
}

S::GUI::Rect S::GUI::MicroMenu::GetVisibleArea() const
{
	/* MicroMenus are usually placed outside of their containers,
	 * so let's ignore the containers visible area.
	 */
	return Rect(GetRealPosition(), GetRealSize());
}

S::Int S::GUI::MicroMenu::Paint(Int message)
{
	if (!IsRegistered()) return Error();
	if (!IsVisible())    return Success();

	Surface	*surface = GetDrawSurface();
	Rect	 frame	 = Rect(GetRealPosition(), GetRealSize());

	switch (message)
	{
		case SP_PAINT:
			surface->StartPaint(frame);

			if (IsBackgroundColorSet()) surface->Box(frame, GetBackgroundColor(), Rect::Filled);

			surface->Frame(frame, FRAME_UP);

			if (subtype == OR_HORZ)
			{
				Int	 size = Math::Round(3 * surface->GetSurfaceDPI() / 96.0);

				for (Int i = 0; i < size; i++)
				{
					Point	 lineStart = Point(frame.left + (frame.GetWidth() - size) / 2 + i, frame.top + frame.GetHeight() / 2 - size + 1 + i);
					Point	 lineEnd   = lineStart + Point(0, 2 * (size - i) - 1);

					if (IsActive())	surface->Line(lineStart, lineEnd, Setup::TextColor);
					else		surface->Line(lineStart, lineEnd, Setup::InactiveTextColor);
				}
			}
			else if (subtype == OR_VERT)
			{
				Int	 size = Math::Round(3 * surface->GetSurfaceDPI() / 96.0);

				for (Int i = 0; i < size; i++)
				{
					Point	 lineStart = Point(frame.left + frame.GetWidth() / 2 - size + 1 + (IsRightToLeft() ? 1 : 0) + i, frame.top + (frame.GetHeight() - size) / 2 + i);
					Point	 lineEnd   = lineStart + Point(2 * (size - i) - 1, 0);

					if (IsActive())	surface->Line(lineStart, lineEnd, Setup::TextColor);
					else		surface->Line(lineStart, lineEnd, Setup::InactiveTextColor);
				}
			}

			surface->EndPaint();

			break;
		case SP_MOUSEOUT:
			if (hotspot->IsActive()) surface->Frame(frame, FRAME_UP);

			break;
	}

	return Success();
}

S::Void S::GUI::MicroMenu::OpenPopupMenu()
{
	if (popup == NIL) return;

	Widget	*window	     = container->GetContainerWindow();
	Surface	*surface     = GetDrawSurface();

	if (window == NIL) return;

	hotspot->Deactivate();

	Rect	 monitor     = System::Screen::GetActiveScreenWorkArea();
	Float	 scaleFactor = surface->GetSurfaceDPI() / 96.0;

	popup->CalculateSize();

	Point	 realPos     = GetRealPosition();
	Size	 realSize    = GetRealSize();

	Point	 popupPos    = realPos + Point(subtype == OR_HORZ ? realSize.cx : 0, subtype == OR_VERT ? realSize.cy : 0);
	Size	 popupSize   = popup->GetSize();

	if (!IsRightToLeft()) { if (window->GetX() + popupPos.x			       + Math::Round(popupSize.cx * scaleFactor) >= monitor.right) popupPos.x = realPos.x - Math::Round(popupSize.cx * scaleFactor) + (subtype == OR_VERT ? realSize.cx : 0); }
	else		      { if (window->GetX() + (window->GetWidth() - popupPos.x) - Math::Round(popupSize.cx * scaleFactor) <  monitor.left)  popupPos.x = realPos.x - Math::Round(popupSize.cx * scaleFactor) + (subtype == OR_VERT ? realSize.cx : 0); }

	if (window->GetY() + popupPos.y + Math::Round(popupSize.cy * scaleFactor) >= monitor.bottom) popupPos.y = realPos.y - Math::Round(popupSize.cy * scaleFactor) + (subtype == OR_HORZ ? GetHeight() : 0);

	popup->SetPosition(popupPos);
	popup->internalRequestClose.Connect(&MicroMenu::ClosePopupMenu, this);

	onOpenPopupMenu.Emit();

	Add(popup);
}

S::Void S::GUI::MicroMenu::ClosePopupMenu()
{
	if (popup == NIL || popup->GetContainer() != this) return;

	Remove(popup);

	onClosePopupMenu.Emit();

	popup->internalRequestClose.Disconnect(&MicroMenu::ClosePopupMenu, this);

	hotspot->Activate();
}
