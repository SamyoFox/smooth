 /* The smooth Class Library
  * Copyright (C) 1998-2020 Robert Kausch <robert.kausch@gmx.net>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of "The Artistic License, Version 2.0".
  *
  * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

#include <smooth/gui/dialogs/tipodaydlg.h>
#include <smooth/gui/dialogs/icons/light.h>

#include <smooth/gui/application/application.h>

#include <smooth/gui/window/window.h>

#include <smooth/gui/widgets/basic/titlebar.h>
#include <smooth/gui/widgets/basic/divider.h>

#include <smooth/gui/widgets/basic/button.h>
#include <smooth/gui/widgets/basic/text.h>
#include <smooth/gui/widgets/basic/checkbox.h>
#include <smooth/gui/widgets/basic/image.h>

#include <smooth/gui/widgets/special/cursor.h>

#include <smooth/gui/widgets/layer.h>

#include <smooth/graphics/surface.h>
#include <smooth/graphics/imageloader/imageloader.h>

#include <smooth/system/screen.h>

#include <smooth/misc/string.h>
#include <smooth/misc/math.h>

#include <smooth/i18n/translator.h>

#include <smooth/foreach.h>

S::GUI::Dialogs::TipOfTheDay::TipOfTheDay(Bool *iShowTips)
{
	showTips = iShowTips;

	mode = TIP_ORDERED;
	offset = 0;

	dlgwnd		= new Window(I18n::Translator::defaultTranslator->TranslateString("Tip of the day"), Point(200, 200), Size(350, 300));

	titlebar	= new Titlebar(TB_CLOSEBUTTON);
	divbar		= new Divider(39, OR_HORZ | OR_BOTTOM);

	btn_ok		= new Button(I18n::Translator::defaultTranslator->TranslateString("OK"), Point(87, 29), Size());
	btn_ok->onAction.Connect(&TipOfTheDay::OnOK, this);
	btn_ok->SetOrientation(OR_LOWERRIGHT);

	btn_next	= new Button(I18n::Translator::defaultTranslator->TranslateString("Next tip"), Point(175, 29), Size());
	btn_next->onAction.Connect(&TipOfTheDay::OnNext, this);
	btn_next->SetOrientation(OR_LOWERRIGHT);
	btn_next->SetWidth(Math::Max(80, btn_next->GetUnscaledTextWidth() + 13));
	btn_next->SetX(btn_next->GetWidth() + 95);

	check_showtips	= new CheckBox(I18n::Translator::defaultTranslator->TranslateString("Show tips on startup"), Point(7, 27), Size(150, 0), showTips);
	check_showtips->SetOrientation(OR_LOWERLEFT);
	check_showtips->SetWidth(check_showtips->GetUnscaledTextWidth() + 21);

	Buffer<UnsignedByte> light(sizeof(Icons::Light));

	memcpy(light, Icons::Light, sizeof(Icons::Light));

	img_light	= new Image(ImageLoader::Load(light, IMAGE_FORMAT_PNG), Point(8, 3), Size(32, 32));

	txt_didyouknow	= new Text(I18n::Translator::defaultTranslator->TranslateString("Did you know..."), Point(11 + img_light->GetWidth(), 7));
	txt_didyouknow->SetFont(Font(Font::Default, 14, Font::Bold, 0, txt_didyouknow->GetFont().GetColor()));

	txt_tip		= new Cursor(Point(6, 4), Size(328, 182));
	txt_tip->SetFont(Font(Font::Default, Font::DefaultSize, Font::Normal, 0, Setup::TooltipTextColor));
	txt_tip->SetFlags(CF_MULTILINE);
	txt_tip->Deactivate();

	layer_inner	= new Layer();
	layer_inner->SetMetrics(Point(8, 39), Size(328, 182));
	layer_inner->SetBackgroundColor(Setup::TooltipColor);

	Add(dlgwnd);

	dlgwnd->Add(titlebar);
	dlgwnd->Add(divbar);

	dlgwnd->Add(btn_ok);
	dlgwnd->Add(btn_next);
	dlgwnd->Add(check_showtips);

	dlgwnd->Add(img_light);
	dlgwnd->Add(txt_didyouknow);

	dlgwnd->GetMainLayer()->Add(layer_inner);

	layer_inner->Add(txt_tip);

	dlgwnd->SetFlags(dlgwnd->GetFlags() | WF_MODAL);
	dlgwnd->onPaint.Connect(&TipOfTheDay::OnPaint, this);
}

S::GUI::Dialogs::TipOfTheDay::~TipOfTheDay()
{
	DeleteObject(dlgwnd);

	DeleteObject(titlebar);
	DeleteObject(divbar);

	DeleteObject(btn_ok);
	DeleteObject(btn_next);
	DeleteObject(check_showtips);

	DeleteObject(img_light);
	DeleteObject(txt_didyouknow);

	DeleteObject(layer_inner);

	DeleteObject(txt_tip);
}

const Error &S::GUI::Dialogs::TipOfTheDay::ShowDialog()
{
	if (caption != NIL) dlgwnd->SetText(caption);

	/* Compute and set dialog size.
	 */
	Rect	 workArea	= System::Screen::GetActiveScreenWorkArea();
	Float	 scaleFactor	= Surface().GetSurfaceDPI() / 96.0;

	Int	 additionalSize = Math::Max(0, check_showtips->GetWidth() + btn_next->GetWidth() - 234);
	Size	 size		= Size(328 + additionalSize, 182);

	foreach (const String &tip, tips) size = Size(Math::Max(size.cx, font.GetUnscaledTextSizeX(tip) + 12),
						      Math::Max(size.cy, font.GetUnscaledTextSizeY(tip) + 12));

	dlgwnd->SetSize(Size(24, 120) + size);
	dlgwnd->SetPosition(workArea.GetPosition() + Point((workArea.GetWidth()  - Math::Round(dlgwnd->GetWidth()  * scaleFactor)) / 2,
							   (workArea.GetHeight() - Math::Round(dlgwnd->GetHeight() * scaleFactor)) / 2));

	layer_inner->SetSize(size);
	txt_tip->SetSize(size);

	/* Set next tip and display dialog.
	 */
	OnNext();

	if (tips.Length() <= 1) btn_next->Deactivate();

	dlgwnd->Show();
	dlgwnd->WaitUntilClosed();

	return error;
}

S::Int S::GUI::Dialogs::TipOfTheDay::AddTip(const String &tip)
{
	tips.Add(tip);

	return Success();
}

S::Int S::GUI::Dialogs::TipOfTheDay::SetMode(Int nMode, Int nOffset, Bool showOnStartup)
{
	switch (nMode)
	{
		case TIP_ORDERED:
			mode	= nMode;
			offset	= nOffset;

			break;
		case TIP_RANDOM:
			mode	= nMode;
			offset	= 0;

			break;
		default:
			return Error();
	}

	*showTips = showOnStartup;

	return Success();
}

S::Int S::GUI::Dialogs::TipOfTheDay::GetOffset() const
{
	return offset;
}

S::Void S::GUI::Dialogs::TipOfTheDay::OnOK()
{
	dlgwnd->Close();
}

S::Void S::GUI::Dialogs::TipOfTheDay::OnNext()
{
	if (tips.Length() <= 1) return;

	String	 tip;

	switch (mode)
	{
		case TIP_ORDERED:
			tip = tips.GetNth(offset++);

			if (offset == tips.Length()) offset = 0;

			break;
		case TIP_RANDOM:
			Math::RandomSeed();

			do
			{
				tip = tips.GetNth(Math::Random() % tips.Length());
			}
			while (tip == txt_tip->GetText());

			break;
	}

	txt_tip->SetText(tip);

	OnPaint();
}

S::Void S::GUI::Dialogs::TipOfTheDay::OnPaint()
{
	dlgwnd->GetDrawSurface()->Box(Rect(layer_inner->GetRealPosition() - Point(1, 1), layer_inner->GetRealSize() + Size(2, 2)), 0, Rect::Outlined);
}
