 /* The smooth Class Library
  * Copyright (C) 1998-2004 Robert Kausch <robert.kausch@gmx.net>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of "The Artistic License, Version 2.0".
  *
  * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

#ifndef _H_OBJSMOOTH_ARGS_
#define _H_OBJSMOOTH_ARGS_

#include "main.h"
#include "misc/args.h"

using namespace smooth;

Int smooth::Main()
{
	ArgumentsParser	 parser(szCmdLine);
	Array<String>	&args = parser.GetArguments();

	return Main(args);
}

#endif
