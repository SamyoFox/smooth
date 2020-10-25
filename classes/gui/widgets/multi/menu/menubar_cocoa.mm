 /* The smooth Class Library
  * Copyright (C) 1998-2020 Robert Kausch <robert.kausch@gmx.net>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of "The Artistic License, Version 2.0".
  *
  * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

#include <Cocoa/Cocoa.h>

#include <smooth/gui/widgets/multi/menu/menubar_cocoa.h>
#include <smooth/gui/widgets/multi/menu/popupmenu.h>
#include <smooth/gui/widgets/basic/checkbox.h>
#include <smooth/gui/widgets/basic/optionbox.h>
#include <smooth/gui/widgets/special/shortcut.h>
#include <smooth/gui/window/window.h>
#include <smooth/input/keyboard.h>
#include <smooth/foreach.h>

using namespace smooth;
using namespace smooth::GUI;

const S::Short	 S::GUI::MenubarCocoa::classID = S::Object::RequestClassID();

class CocoaMenuEntryDelegate;

/* CocoaMenuEntry implements basic mapping from
 * smooth menu entries to Cocoa menu items.
 */
@interface CocoaMenuEntry : NSMenuItem
{
	@private
		const MenuEntry		*menuEntry;
		CocoaMenuEntryDelegate	*menuEntryDelegate;
}

	- (id)		initWithMenuEntry:	(const MenuEntry *) iMenuEntry
			  ignoreLastEntry:	(BOOL)		    ignoreLastEntry;
	- (void)	dispose;

	- (void)	checkValue;

	- (void)	onAction;
@end

/* CocoaMenu implements basic mapping
 * from smooth menus to Cocoa menus.
 */
@interface CocoaMenu : NSMenu
{
	@private
		const Menu			*menu;
	@protected
		Array<NSMenuItem *, Void *>	 menuItems;
}

	- (id)		initWithMenu:	(const Menu *)	 iMenu
			       title:	(const String &) iTitle
		     ignoreLastEntry:	(BOOL)		 ignoreLastEntry;
	- (void)	dispose;
@end

/* CocoaMenubar implements specific behaviour only needed
 * for the menubar such as filling the application menu.
 */
@interface CocoaMenubar : CocoaMenu
{
	@private
		const Menubar	*menubar;

		CocoaMenuEntry	*aboutItem;
		CocoaMenuEntry	*quitItem;
}

	- (id)		initWithMenubar:	(const Menubar *) iMenubar;
	- (void)	dispose;
@end

/* CocoaMenuEntry implements basic mapping from
 * smooth menu entries to Cocoa menu items.
 */
class CocoaMenuEntryDelegate
{
	private:
		CocoaMenuEntry	*menuEntry;
	public:
				 CocoaMenuEntryDelegate(CocoaMenuEntry *iMenuEntry) : menuEntry(iMenuEntry)	{ CheckBox::internalCheckValues.Connect(&CocoaMenuEntryDelegate::OnCheckValues, this);    OptionBox::internalCheckValues.Connect(&CocoaMenuEntryDelegate::OnCheckValues, this);	   }
				~CocoaMenuEntryDelegate()							{ CheckBox::internalCheckValues.Disconnect(&CocoaMenuEntryDelegate::OnCheckValues, this); OptionBox::internalCheckValues.Disconnect(&CocoaMenuEntryDelegate::OnCheckValues, this); }
	slots:
		Void		 OnCheckValues()								{ [menuEntry checkValue]; }
};

@implementation CocoaMenuEntry
	- (id) initWithMenuEntry: (const MenuEntry *) iMenuEntry
		 ignoreLastEntry: (BOOL)	      ignoreLastEntry
	{
		[super initWithTitle: [NSString stringWithUTF8String: iMenuEntry->GetText().ConvertTo("UTF-8")]
			      action: @selector(onAction)
		       keyEquivalent: @""];

		menuEntry	  = iMenuEntry;
		menuEntryDelegate = new CocoaMenuEntryDelegate(self);

		if (menuEntry->GetPopupMenu() != NIL)
		{
			NSMenu	*submenu = [[CocoaMenu alloc] initWithMenu: menuEntry->GetPopupMenu()
								     title: menuEntry->GetText()
							   ignoreLastEntry: ignoreLastEntry];

			[self setSubmenu: submenu];
			[self setAction: nil];
		}

		if (menuEntry->GetShortcut() != NIL)
		{
			const Shortcut	*shortcut = menuEntry->GetShortcut();
			Int		 key	  = shortcut->GetKey();
			Int		 flags	  = shortcut->GetFlags();

			String		 keyString;

			if	(key >= Input::Keyboard::Key0  && key <= Input::Keyboard::Key9)	  keyString[0] = '0'    + key - Input::Keyboard::Key0;
			else if (key >= Input::Keyboard::KeyA  && key <= Input::Keyboard::KeyZ)	  keyString[0] = 'a'    + key - Input::Keyboard::KeyA;
			else if (key >= Input::Keyboard::KeyF1 && key <= Input::Keyboard::KeyF24) keyString[0] = 0xF704 + key - Input::Keyboard::KeyF1;

			if (keyString != NIL)
			{
				[self setKeyEquivalent: [NSString stringWithUTF8String: keyString.ConvertTo("UTF-8")]];

				[self setKeyEquivalentModifierMask: (flags & SC_CTRL  ? NSControlKeyMask   : 0) |
								    (flags & SC_CMD   ? NSCommandKeyMask   : 0) |
								    (flags & SC_ALT   ? NSAlternateKeyMask : 0) |
								    (flags & SC_SHIFT ? NSShiftKeyMask	   : 0)];
			}
		}

		if (!menuEntry->IsActive()) [self setEnabled: NO];

		[self checkValue];

		[self setTarget: self];

		return self;
	}

	- (void) dispose
	{
		delete menuEntryDelegate;

		if ([self hasSubmenu])
		{
			if ([[self submenu] respondsToSelector: @selector(dispose)]) [[self submenu] performSelector: @selector(dispose)];

			[[self submenu] release];

			[self setSubmenu: nil];
		}
	}

	- (void) checkValue
	{
		if (menuEntry->GetCheckVariable() != NIL)
		{
			[self setState: *menuEntry->GetCheckVariable()];
		}
		else if (menuEntry->GetOptionVariable() != NIL)
		{
			if (*menuEntry->GetOptionVariable() == menuEntry->GetOptionCode()) [self setState: NSOnState];
			else								   [self setState: NSOffState];
		}
	}

	- (void) onAction
	{
		if (menuEntry->GetCheckVariable() != NIL)
		{
			*menuEntry->GetCheckVariable() = !*menuEntry->GetCheckVariable();

			[self setState: *menuEntry->GetCheckVariable()];

			CheckBox::internalCheckValues.Emit();
		}
		else if (menuEntry->GetOptionVariable() != NIL)
		{
			*menuEntry->GetOptionVariable() = menuEntry->GetOptionCode();

			OptionBox::internalCheckValues.Emit();
		}

		menuEntry->onAction.Emit();
	}
@end

/* CocoaMenu implements basic mapping
 * from smooth menus to Cocoa menus.
 */
@implementation CocoaMenu
	- (id) initWithMenu: (const Menu *)   iMenu
		      title: (const String &) iTitle
	    ignoreLastEntry: (BOOL)	      ignoreLastEntry
	{
		[super initWithTitle: [NSString stringWithUTF8String: iTitle.ConvertTo("UTF-8")]];
		[super setAutoenablesItems: NO];

		menu = iMenu;

		for (Int i = 0; i < menu->Length(); i++)
		{
			const MenuEntry	*entry = menu->GetNthEntry(i);

			if (ignoreLastEntry && ( i == menu->Length() - 1 ||
						(i == menu->Length() - 2 && entry->GetText() == NIL))) break;

			/* Add menu item.
			 */
			NSMenuItem	*item = nil;

			if (entry->GetText() != NIL) item = [[CocoaMenuEntry alloc] initWithMenuEntry: entry
										      ignoreLastEntry: NO];
			else			     item = [[NSMenuItem separatorItem] retain];

			menuItems.Add(item);

			[self addItem: item];
		}

		return self;
	}

	- (void) dispose
	{
		foreach (NSMenuItem *item, menuItems)
		{
			[self removeItem: item];

			if ([item respondsToSelector: @selector(dispose)]) [item performSelector: @selector(dispose)];

			[item release];
		}
	}
@end

/* CocoaMenubar implements specific behaviour only needed
 * for the menubar such as filling the application menu.
 */
@implementation CocoaMenubar
	- (id) initWithMenubar: (const Menubar *) iMenubar
	{
		[super initWithTitle: @"MainMenu"];
		[super setAutoenablesItems: NO];

		menubar = iMenubar;

		/* Build application menu.
		 */
		NSMenu		*appMenu = [[NSMenu alloc] initWithTitle: @"Apple"];

		aboutItem = nil;
		quitItem  = nil;

		/* Add About... entry.
		 */
		if (menubar->Length() >= 1 && menubar->GetNthEntry(menubar->Length() - 1)->GetOrientation() == OR_RIGHT && menubar->GetNthEntry(menubar->Length() - 1)->GetPopupMenu() != NIL)
		{
			const Menu	*lastMenu = menubar->GetNthEntry(menubar->Length() - 1)->GetPopupMenu();

			aboutItem = [[CocoaMenuEntry alloc] initWithMenuEntry: lastMenu->GetNthEntry(lastMenu->Length() - 1)
							      ignoreLastEntry: NO];

			[appMenu addItem: aboutItem];
			[appMenu addItem: [NSMenuItem separatorItem]];
		}

		/* Add Hide... entries.
		 */
		NSMenuItem	*hideItem = [appMenu addItemWithTitle: NSLocalizedString(@"Hide", nil)
							       action: @selector(hide:)
							keyEquivalent: @"h"];
		[hideItem setTarget: NSApp];

		NSMenuItem	*hideOthersItem = [appMenu addItemWithTitle: NSLocalizedString(@"Hide Others", nil)
								     action: @selector(hideOtherApplications:)
							      keyEquivalent: @"h"];

		[hideOthersItem setKeyEquivalentModifierMask: NSCommandKeyMask | NSAlternateKeyMask];
		[hideOthersItem setTarget: NSApp];

		/* Add Quit entry.
		 */
		if (menubar->Length() >= 1 && menubar->GetNthEntry(0)->GetPopupMenu() != NIL)
		{
			const Menu	*firstMenu = menubar->GetNthEntry(0)->GetPopupMenu();

			quitItem = [[CocoaMenuEntry alloc] initWithMenuEntry: firstMenu->GetNthEntry(firstMenu->Length() - 1)
							     ignoreLastEntry: NO];

			[appMenu addItem: [NSMenuItem separatorItem]];
			[appMenu addItem: quitItem];
		}

		NSMenuItem	*appMenuItem = [[NSMenuItem alloc] initWithTitle: @"Apple"
									  action: nil
								   keyEquivalent: @""];

		[appMenuItem setSubmenu: appMenu];

		menuItems.Add(appMenuItem);

		[self addItem: appMenuItem];

		[NSApp performSelector: @selector(setAppleMenu:)
			    withObject: appMenu];

		/* Add other menus.
		 */
		for (Int i = 0; i < menubar->Length(); i++)
		{
			const MenuEntry	*entry = menubar->GetNthEntry(i);

			if (entry->GetText() == NIL) continue;

			/* Add menu item.
			 */
			NSMenuItem	*item = [[CocoaMenuEntry alloc] initWithMenuEntry: entry
									  ignoreLastEntry: (i == 0		       && quitItem  != nil) ||
											   (i == menubar->Length() - 1 && aboutItem != nil)];

			menuItems.Add(item);

			[self addItem: item];
		}

		[NSApp setMainMenu: self];

		return self;
	}

	- (void) dispose
	{
		[super dispose];

		[aboutItem dispose];
		[aboutItem release];

		[quitItem dispose];
		[quitItem release];
	}
@end

S::GUI::MenubarCocoa::MenubarCocoa(Menubar *iMenubar) : Widget(Point(), Size())
{
	type		= classID;
	orientation	= OR_FREE;

	menubar		= iMenubar;
	menubarCocoa	= NIL;

	CheckBox::internalCheckValues.Connect(&MenubarCocoa::UpdateCocoaMenubar, this);
	OptionBox::internalCheckValues.Connect(&MenubarCocoa::UpdateCocoaMenubar, this);
}

S::GUI::MenubarCocoa::~MenubarCocoa()
{
	CheckBox::internalCheckValues.Disconnect(&MenubarCocoa::UpdateCocoaMenubar, this);
	OptionBox::internalCheckValues.Disconnect(&MenubarCocoa::UpdateCocoaMenubar, this);

	if (menubarCocoa != NIL)
	{
		[(CocoaMenubar *) menubarCocoa dispose];
		[(CocoaMenubar *) menubarCocoa release];
	}
}

S::Int S::GUI::MenubarCocoa::Paint(Int message)
{
	if (!IsRegistered()) return Error();
	if (!IsVisible())    return Success();

	Window	*window = GetContainerWindow();

	if (GetSize() == Size()) SetSize(window->GetSize());
	if (GetSize() == window->GetSize()) UpdateCocoaMenubar();

	SetVisibleDirect(False);
	SetSize(window->GetSize());
	SetVisibleDirect(True);

	return Success();
}

S::Void S::GUI::MenubarCocoa::UpdateCocoaMenubar()
{
	if (menubarCocoa != NIL)
	{
		[(CocoaMenubar *) menubarCocoa dispose];
		[(CocoaMenubar *) menubarCocoa release];
	}

	menubarCocoa = [[CocoaMenubar alloc] initWithMenubar: menubar];
}
