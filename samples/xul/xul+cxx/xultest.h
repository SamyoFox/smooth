 /* The smooth Class Library
  * Copyright (C) 1998-2020 Robert Kausch <robert.kausch@gmx.net>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of "The Artistic License, Version 2.0".
  *
  * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

#ifndef	H_XULTEST
#define H_XULTEST

#include <smooth.h>

using namespace smooth;
using namespace smooth::GUI;
using namespace smooth::XML::XUL;

class XULTest : public Application
{
	private:
		Renderer	 xulRenderer;
	public:
				 XULTest();
				~XULTest();
};

#endif
