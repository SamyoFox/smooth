 /* The smooth Class Library
  * Copyright (C) 1998-2004 Robert Kausch <robert.kausch@gmx.net>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of "The Artistic License, Version 2.0".
  *
  * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

#ifndef _H_OBJSMOOTH_BACKEND_
#define _H_OBJSMOOTH_BACKEND_

namespace smooth
{
	namespace Backends
	{
		class Backend;
	};
};

#include "../definitions.h"

namespace smooth
{
	namespace Backends
	{
		class Backend
		{
			private:
				static Array<Backend *>		*backends;
				static Array<Backend *(*)()>	*backend_creators;
			public:
				static Int			 AddBackend(Backend *(*)());

				static Int			 InitBackends();
				static Int			 DeinitBackends();

				virtual Int			 Init();
				virtual Int			 Deinit();
		};
	};
};

#endif
