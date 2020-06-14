 /* The smooth Class Library
  * Copyright (C) 1998-2020 Robert Kausch <robert.kausch@gmx.net>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of "The Artistic License, Version 2.0".
  *
  * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

#include <smooth.h>
#include <smooth/main.h>

#include "test.h"

using namespace smooth;

Int smooth::Main()
{
	return Test().Loop();
}

Test::Test()
{
	mainWnd			= new GUI::Window("smooth Test", Point(100, 100), Size(600, 500));
	mainWnd_titlebar	= new Titlebar();
	mainWnd_statusbar	= new Statusbar("Ready");

	mainWnd_divider1	= new Divider(240, OR_HORZ | OR_TOP);
	mainWnd_divider2	= new Divider(200, OR_VERT | OR_LEFT);

	mainWnd_client		= new Client();

	mainWnd_menubar		= new Menubar();

	menu_file		= new PopupMenu();
	menu_file->AddEntry("Exit")->onAction.Connect(&GUI::Window::Close, mainWnd);

	menu_dialogs		= new PopupMenu();
	menu_dialogs->AddEntry("File chooser...");
	menu_dialogs->AddEntry("Directory chooser...");
	menu_dialogs->AddEntry();
	menu_dialogs->AddEntry("Color selector...")->onAction.Connect(&Test::ColorDlg, this);

	menu_2ndlevel		= new PopupMenu();
	menu_2ndlevel->AddEntry("Click")->onAction.Connect(&Test::OnWidgetAction, this);

	checkbox_var		= True;
	optionbox_var		= 0;

	menu_menus		= new PopupMenu();
	menu_menus->AddEntry("2nd level popup menu", menu_2ndlevel);
	menu_menus->AddEntry();
	menu_menus->AddEntry("CheckBox menu entry", &checkbox_var);
	menu_menus->AddEntry();
	menu_menus->AddEntry("OptionBox menu entry", &optionbox_var, 0);
	menu_menus->AddEntry("OptionBox menu entry", &optionbox_var, 1);

	mainWnd_menubar->AddEntry("File", menu_file);
	mainWnd_menubar->AddEntry("Dialogs", menu_dialogs);
	mainWnd_menubar->AddEntry("Menus", menu_menus);

	text_arrows		= new Text("Arrows:", Point(10, 12));
	widget_arrows		= new Arrows(Point(80, 10), Size(24, 17), OR_HORZ);
	widget_arrows->onValueChange.Connect(&Test::OnArrowsValueChange, this);
	text_arrows_value	= new Text("0", Point(110, 12));

	text_activearea		= new Text("ActiveArea:", Point(10, 37));
	widget_activearea	= new ActiveArea(Color(255, 0, 0), Point(80, 35), Size(24, 17));
	widget_activearea->onAction.Connect(&Test::OnWidgetAction, this);

	text_button		= new Text("Button:", Point(10, 64));
	widget_button		= new Button("Click", Point(80, 60), Size(80, 22));
	widget_button->onAction.Connect(&Test::OnWidgetAction, this);

	text_hyperlink		= new Text("Hyperlink:", Point(10, 85));
	widget_hyperlink	= new Hyperlink("Click", "http://www.smooth-project.org/", Point(80, 85));

	text_scrollbar		= new Text("Scrollbar:", Point(10, 112));
	widget_scrollbar	= new Scrollbar(Point(80, 110), Size(120, 17), OR_HORZ);
	widget_scrollbar->onValueChange.Connect(&Test::OnScrollbarValueChange, this);
	text_scrollbar_value	= new Text("0", Point(210, 112));

	text_progressbar	= new Text("Progressbar:", Point(10, 138));
	widget_progressbar	= new Progressbar(Point(80, 135), Size(120, 19), OR_HORZ);

	text_slider		= new Text("Slider:", Point(10, 163));
	widget_slider		= new Slider(Point(80, 160), Size(120, 18), OR_HORZ);
	widget_slider->onValueChange.Connect(&Progressbar::SetValue, widget_progressbar);

	text_checkbox		= new Text("CheckBox:", Point(10, 187));
	widget_checkbox		= new CheckBox("Click", Point(80, 185), Size(60, 17), &checkbox_var);

	text_optionbox		= new Text("OptionBox:", Point(10, 212));
	widget_optionbox1	= new OptionBox("Click", Point(80, 210), Size(60, 17), &optionbox_var, 0);
	widget_optionbox2	= new OptionBox("Click", Point(150, 210), Size(60, 17), &optionbox_var, 1);

	text_editbox		= new Text("EditBox:", Point(250, 13));
	widget_editbox		= new EditBox("MyEditBox", Point(320, 10), Size(100, 19));

	text_combobox		= new Text("ComboBox:", Point(250, 38));
	widget_combobox		= new ComboBox(Point(320, 35), Size(100, 19));
	widget_combobox->AddEntry("Red");
	widget_combobox->AddEntry("Green");
	widget_combobox->AddEntry("Blue");
	widget_combobox->AddEntry("Yellow");

	Widget	*selected_widget = widget_editbox;

	button_show		= new Button("Show", Point(300, 180), Size(70, 22));
	button_show->onAction.Connect(&Widget::Show, selected_widget);

	button_hide		= new Button("Hide", Point(372, 180), Size(70, 22));
	button_hide->onAction.Connect(&Widget::Hide, selected_widget);

	button_activate		= new Button("Activate", Point(300, 204), Size(70, 22));
	button_activate->onAction.Connect(&Widget::Activate, selected_widget);

	button_deactivate	= new Button("Deactivate", Point(372, 204), Size(70, 22));
	button_deactivate->onAction.Connect(&Widget::Deactivate, selected_widget);

	Add(mainWnd);

	mainWnd->Add(mainWnd_titlebar);
	mainWnd->Add(mainWnd_statusbar);

	mainWnd->Add(mainWnd_divider1);
	mainWnd->Add(mainWnd_divider2);
	mainWnd->Add(mainWnd_client);

	mainWnd->Add(mainWnd_menubar);

	mainWnd->Add(text_arrows);
	mainWnd->Add(widget_arrows);
	mainWnd->Add(text_arrows_value);

	mainWnd->Add(text_activearea);
	mainWnd->Add(widget_activearea);

	mainWnd->Add(text_button);
	mainWnd->Add(widget_button);

	mainWnd->Add(text_hyperlink);
	mainWnd->Add(widget_hyperlink);

	mainWnd->Add(text_scrollbar);
	mainWnd->Add(widget_scrollbar);
	mainWnd->Add(text_scrollbar_value);

	mainWnd->Add(text_progressbar);
	mainWnd->Add(widget_progressbar);

	mainWnd->Add(text_slider);
	mainWnd->Add(widget_slider);

	mainWnd->Add(text_checkbox);
	mainWnd->Add(widget_checkbox);

	mainWnd->Add(text_optionbox);
	mainWnd->Add(widget_optionbox1);
	mainWnd->Add(widget_optionbox2);

	mainWnd->Add(text_editbox);
	mainWnd->Add(widget_editbox);

	mainWnd->Add(text_combobox);
	mainWnd->Add(widget_combobox);

	mainWnd->Add(button_show);
	mainWnd->Add(button_hide);
	mainWnd->Add(button_activate);
	mainWnd->Add(button_deactivate);

	mainWnd->SetMinimumSize(Size(470, 370));

	mainWnd->SetIcon(NIL);
}

Test::~Test()
{
	DeleteObject(text_arrows);
	DeleteObject(widget_arrows);
	DeleteObject(text_arrows_value);

	DeleteObject(text_activearea);
	DeleteObject(widget_activearea);

	DeleteObject(text_button);
	DeleteObject(widget_button);

	DeleteObject(text_hyperlink);
	DeleteObject(widget_hyperlink);

	DeleteObject(text_scrollbar);
	DeleteObject(widget_scrollbar);
	DeleteObject(text_scrollbar_value);

	DeleteObject(text_progressbar);
	DeleteObject(widget_progressbar);

	DeleteObject(text_slider);
	DeleteObject(widget_slider);

	DeleteObject(text_checkbox);
	DeleteObject(widget_checkbox);

	DeleteObject(text_optionbox);
	DeleteObject(widget_optionbox1);
	DeleteObject(widget_optionbox2);

	DeleteObject(text_editbox);
	DeleteObject(widget_editbox);

	DeleteObject(text_combobox);
	DeleteObject(widget_combobox);

	DeleteObject(button_show);
	DeleteObject(button_hide);
	DeleteObject(button_activate);
	DeleteObject(button_deactivate);

	DeleteObject(menu_file);
	DeleteObject(menu_dialogs);
	DeleteObject(menu_menus);
	DeleteObject(menu_2ndlevel);

	DeleteObject(mainWnd_menubar);

	DeleteObject(mainWnd_divider1);
	DeleteObject(mainWnd_divider2);
	DeleteObject(mainWnd_client);

	DeleteObject(mainWnd_statusbar);
	DeleteObject(mainWnd_titlebar);
	DeleteObject(mainWnd);
}

Void Test::ColorDlg()
{
	ColorSelection	 dialog;

	dialog.SetParentWindow(mainWnd);
	dialog.ShowDialog();
}

Void Test::OnArrowsValueChange(Int value)
{
	text_arrows_value->SetText(String::FromInt(value));

	Surface	*surface = mainWnd->GetDrawSurface();

	/* Boxes.
	 */
	surface->Box(Rect(Point(250, 150), Size(10, 10)), Color(255, 0, 0), Rect::Filled);
	surface->Box(Rect(Point(250, 170), Size(10, 10)), Color(255, 0, 0), Rect::Dotted);
	surface->Box(Rect(Point(250, 190), Size(10, 10)), Color(255, 0, 0), Rect::Outlined);

	/* Gradient.
	 */
	surface->Gradient(Rect(Point(300, 150), Size(100, 10)), Color(255, 0, 0), Color(0, 0, 255), OR_HORZ);

	/* Horizontal & vertical lines.
	 */
	surface->Line(Point(300, 170), Point(400, 170), Color(255, 0, 0));
	surface->Line(Point(400, 180), Point(300, 180), Color(255, 0, 0));
	surface->Line(Point(300, 190), Point(300, 200), Color(255, 0, 0));
	surface->Line(Point(290, 200), Point(290, 190), Color(255, 0, 0));

	/* Diagonal lines.
	 */
	surface->Line(Point(310, 190), Point(320, 200), Color(255, 0, 0));
	surface->Line(Point(330, 200), Point(340, 190), Color(255, 0, 0));
	surface->Line(Point(360, 200), Point(350, 190), Color(255, 0, 0));
	surface->Line(Point(380, 190), Point(370, 200), Color(255, 0, 0));

	/* Single pixel.
	 */
	surface->SetPixel(Point(399, 190), Color(255, 0, 0));
}

Void Test::OnScrollbarValueChange(Int value)
{
	text_scrollbar_value->SetText(String::FromInt(value));
}

Void Test::OnWidgetAction()
{
	QuickMessage("Some widget action occurred!", "Info", Message::Buttons::Ok, Message::Icon::Information);
}
