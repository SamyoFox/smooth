 /* The smooth Class Library
  * Copyright (C) 1998-2010 Robert Kausch <robert.kausch@gmx.net>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of "The Artistic License, Version 2.0".
  *
  * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

#include <smooth/i18n/charsetdetector.h>
#include <smooth/i18n/charsetdetector_internal.h>

S::I18n::CharsetDetector::CharsetDetector()
{
}

S::I18n::CharsetDetector::~CharsetDetector()
{
}

S::String S::I18n::CharsetDetector::DetectCharset(const char *data)
{
	CharsetDetectorInternal	 internal;

	return internal.DetectCharset(data);
}
