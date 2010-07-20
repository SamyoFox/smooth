 /* The smooth Class Library
  * Copyright (C) 1998-2010 Robert Kausch <robert.kausch@gmx.net>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of "The Artistic License, Version 2.0".
  *
  * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

#include <smooth/i18n/charsetdetector_internal.h>

S::I18n::CharsetDetectorInternal::CharsetDetectorInternal() : nsUniversalDetector(NS_FILTER_ALL)
{
}

S::I18n::CharsetDetectorInternal::~CharsetDetectorInternal()
{
}

S::String S::I18n::CharsetDetectorInternal::DetectCharset(const char *data)
{
	/* Pass data to nsUniversalDetector and return the result.
	 */
	HandleData(data, strlen(data));
	DataEnd();

	return detectedCharset;
}

void S::I18n::CharsetDetectorInternal::Report(const char *charset)
{
	detectedCharset = charset;
}
