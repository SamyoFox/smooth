 /* The smooth Class Library
  * Copyright (C) 1998-2006 Robert Kausch <robert.kausch@gmx.net>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of "The Artistic License, Version 2.0".
  *
  * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

#ifndef _H_OBJSMOOTH_SIGNAL_
#define _H_OBJSMOOTH_SIGNAL_

#include "../../basic/object.h"

namespace smooth
{
	class Signal
	{
		protected:
			Object	*parent;

			Void ProtectParent() const
			{
				if (parent != NIL) parent->EnterProtectedRegion();
			}

			Void UnprotectParent() const
			{
				if (parent != NIL) parent->LeaveProtectedRegion();
			}
		public:
			Signal()
			{
				parent = NIL;
			}

			Int SetParentObject(Object *newParent)
			{
				parent = newParent;

				return Success();
			}
	};
};

#endif
