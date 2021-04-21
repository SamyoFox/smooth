 /* The smooth Class Library
  * Copyright (C) 1998-2021 Robert Kausch <robert.kausch@gmx.net>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of "The Artistic License, Version 2.0".
  *
  * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

#include <smooth/gui/widgets/multi/list/list.h>
#include <smooth/foreach.h>

const S::Short	 S::GUI::List::classID = S::Object::RequestClassID();

S::GUI::List::List()
{
	type = classID;

	onSelectEntry.SetParentObject(this);
	onMarkEntry.SetParentObject(this);
}

S::GUI::List::~List()
{
}

S::GUI::ListEntry *S::GUI::List::AddEntry(const String &text, Bool marked)
{
	ListEntry	*newEntry = new ListEntry(text);

	newEntry->SetMark(marked);

	if (Add(newEntry) == Success())
	{
		createdEntry.Set(newEntry->GetHandle(), True);

		return newEntry;
	}

	DeleteObject(newEntry);

	return NIL;
}

S::GUI::ListEntry *S::GUI::List::AddSeparator()
{
	ListEntrySeparator	*newEntry = new ListEntrySeparator();

	if (Add(newEntry) == Success())
	{
		createdEntry.Set(newEntry->GetHandle(), True);

		return newEntry;
	}

	DeleteObject(newEntry);

	return NIL;
}

S::Int S::GUI::List::Add(Widget *widget)
{
	if (widget == NIL) return Error();

	if (!widget->IsRegistered())
	{
		if (widget->GetObjectType() == ListEntry::classID) widget->Hide();

		return Container::Add(widget);
	}

	return Error();
}

S::Int S::GUI::List::SelectEntry(const ListEntry *entryToSelect)
{
	foreach (Entry *entry, elementOrder)
	{
		if (entry != entryToSelect) continue;

		((ListEntry *) entry)->Select();

		return Success();
	}

	return Error();
}

S::GUI::ListEntry *S::GUI::List::GetSelectedEntry() const
{
	foreach (Entry *entry, elementOrder)
	{
		if (((ListEntry *) entry)->IsSelected()) return (ListEntry *) entry;
	}

	return NIL;
}

S::Int S::GUI::List::SelectNthEntry(Int n)
{
	if (n < 0 || n >= Length()) return Error();

	ListEntry	*entry = GetNthEntry(n);

	if (!entry->IsSelected()) entry->Select();

	return Success();
}

S::Int S::GUI::List::GetSelectedEntryNumber() const
{
	for (Int i = 0; i < elementOrder.Length(); i++)
	{
		ListEntry	*entry = (ListEntry *) elementOrder.GetNth(i);

		if (entry->IsSelected()) return i;
	}

	return -1;
}

S::Int S::GUI::List::GetEntryNumber(const String &entryText) const
{
	for (Int i = 0; i < elementOrder.Length(); i++)
	{
		ListEntry	*entry = (ListEntry *) elementOrder.GetNth(i);

		if (entry->GetText() == entryText) return i;
	}

	return -1;
}

S::GUI::ListEntry *S::GUI::List::GetEntry(const String &entryText) const
{
	return GetNthEntry(GetEntryNumber(entryText));
}

S::Int S::GUI::List::SelectEntry(const String &entryText)
{
	return SelectNthEntry(GetEntryNumber(entryText));
}
