 /* The smooth Class Library
  * Copyright (C) 1998-2015 Robert Kausch <robert.kausch@gmx.net>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of "The Artistic License, Version 2.0".
  *
  * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

#include <smooth/io/instream.h>
#include <smooth/io/outstream.h>
#include <smooth/io/filter.h>

#include <smooth/io/drivers/driver_ansi.h>
#include <smooth/io/drivers/driver_posix.h>
#include <smooth/io/drivers/driver_memory.h>

#ifdef __WIN32__
#	include <smooth/io/drivers/driver_win32.h>
#endif

#include <smooth/files/file.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <memory.h>

S::IO::OutStream::OutStream(Int type, Driver *iDriver)
{
	inStream	= NIL;

	if (type != STREAM_DRIVER)		   { lastError = IO_ERROR_BADPARAM;	 return; }

	driver		= iDriver;

	if (driver->GetLastError() != IO_ERROR_OK) { lastError = driver->GetLastError(); return; }

	streamType	= STREAM_DRIVER;
	size		= driver->GetSize();
	currentFilePos	= driver->GetPos();
	closefile	= false;

	dataBuffer.Resize(packageSize);
}

S::IO::OutStream::OutStream(Int type, const String &fileName, Int mode)
{
	inStream	= NIL;

	if (type != STREAM_FILE)		   { lastError = IO_ERROR_BADPARAM;			return; }

#ifdef __WIN32__
	driver		= new DriverWin32(File(fileName), mode);
#else
	driver		= new DriverPOSIX(File(fileName), mode);
#endif

	if (driver->GetLastError() != IO_ERROR_OK) { lastError = driver->GetLastError(); delete driver; return; }

	streamType	= STREAM_DRIVER;
	size		= driver->GetSize();
	currentFilePos	= size;

	dataBuffer.Resize(packageSize);
}

S::IO::OutStream::OutStream(Int type, FILE *openFile)
{
	inStream	= NIL;

	if (type != STREAM_ANSI)		   { lastError = IO_ERROR_BADPARAM;			return; }

	driver		= new DriverANSI(openFile);

	if (driver->GetLastError() != IO_ERROR_OK) { lastError = driver->GetLastError(); delete driver; return; }

	streamType	= STREAM_DRIVER;
	size		= driver->GetSize();
	currentFilePos	= driver->GetPos();

	packageSize	= 1; // low package size, 'cause openFile could point to the console
	stdpacksize	= packageSize;
	origpacksize	= packageSize;

	dataBuffer.Resize(packageSize);
}

S::IO::OutStream::OutStream(Int type, Void *outBuffer, Long bufferSize)
{
	inStream	= NIL;

	if (type != STREAM_BUFFER)		   { lastError = IO_ERROR_BADPARAM;			return; }

	driver		= new DriverMemory(outBuffer, bufferSize);

	if (driver->GetLastError() != IO_ERROR_OK) { lastError = driver->GetLastError(); delete driver; return; }

	streamType	= STREAM_DRIVER;
	size		= driver->GetSize();

	packageSize	= 1;
	stdpacksize	= packageSize;
	origpacksize	= packageSize;

	dataBuffer.Resize(packageSize);
}

S::IO::OutStream::OutStream(Int type, InStream *in)
{
	inStream	= NIL;

	if (type != STREAM_STREAM)			      { lastError = IO_ERROR_BADPARAM;	 return; }
	if (in->streamType == STREAM_NONE || in->crosslinked) { lastError = IO_ERROR_OPNOTAVAIL; return; }

	streamType	= STREAM_STREAM;

	crosslinked		= true;
	inStream		= in;
	inStream->outStream	= this;
	inStream->crosslinked	= true;

	if (inStream->streamType == STREAM_DRIVER)
	{
		driver		= inStream->driver;

		streamType	= STREAM_DRIVER;
		size		= inStream->origsize;
		currentFilePos	= inStream->currentFilePos;
		closefile	= false;

		packageSize	= 1;
		stdpacksize	= packageSize;
		origpacksize	= packageSize;

		dataBuffer.Resize(packageSize);
	}
}

S::IO::OutStream::~OutStream()
{
	/* Close stream; ignore return value.
	 */
	Close();
}

S::Bool S::IO::OutStream::Flush()
{
	if (streamType == STREAM_NONE) { lastError = IO_ERROR_NOTOPEN; return false; }

	if (currentBufferPos <= 0) return true;

	if (bitstreamActive) CompleteBitstream();

	int	 ps		= packageSize;
	int	 oldcpos	= currentBufferPos;

	if (filters.Length() > 0)
	{
		if (filters.GetFirst()->GetPackageSize() > 0)
		{
			for (int i = 0; i < (packageSize - oldcpos); i++) OutputNumber(0, 1);
		}
	}

	if (currentBufferPos > 0)
	{
		packageSize = currentBufferPos;

		if (!WriteData())
		{
			packageSize = ps;

			{ lastError = IO_ERROR_UNKNOWN; return false; }
		}

		packageSize = ps;
	}

	return true;
}

S::Bool S::IO::OutStream::WriteData()
{
	if (streamType == STREAM_NONE) { lastError = IO_ERROR_NOTOPEN; return False; }

	if (currentBufferPos < packageSize) return True;

	if (filters.Length() > 0)
	{
		if (filters.GetFirst()->GetPackageSize() == -1)
		{
			dataBuffer.Resize(packageSize + defaultPackageSize);

			packageSize += defaultPackageSize;
			stdpacksize = packageSize;

			return True;
		}
	}

	if (streamType == STREAM_DRIVER)
	{
		Int	 encsize = 0;

		dataBuffer.Resize(packageSize);

		if (filters.Length() == 0) encsize = driver->WriteData(dataBuffer, packageSize);
		else			   encsize = filters.GetFirst()->WriteData(dataBuffer, packageSize);

		if (encsize == -1)
		{
			packageSize = 0;

			return False;
		}

		driver->Flush();

		if (size == currentFilePos) size -= (packageSize - encsize);

		currentFilePos	 -= (packageSize - encsize);
		currentBufferPos -= packageSize;
	}

	return True;
}

S::Bool S::IO::OutStream::OutputNumber(Int64 number, Int bytes)
{
	if (streamType == STREAM_NONE)	{ lastError = IO_ERROR_NOTOPEN; return false; }
	if (bytes > 8 || bytes < 0)	{ lastError = IO_ERROR_BADPARAM; return false; }

	if (bitstreamActive && !keepBits) CompleteBitstream();

	for (Int i = 0; i < bytes; i++)
	{
		if (currentBufferPos >= packageSize)
		{
			if (!WriteData()) { lastError = IO_ERROR_UNKNOWN; return false; }
		}

		dataBuffer[currentBufferPos] = IOGetByte(number, i);
		if (currentFilePos == size) size++;
		currentBufferPos++;
		currentFilePos++;
	}

	if (currentBufferPos >= packageSize)
	{
		if (!WriteData()) { lastError = IO_ERROR_UNKNOWN; return false; }
	}

	return true;
}

S::Bool S::IO::OutStream::OutputNumberRaw(Int64 number, Int bytes)
{
	if (streamType == STREAM_NONE)	{ lastError = IO_ERROR_NOTOPEN; return false; }
	if (bytes > 8 || bytes < 0)	{ lastError = IO_ERROR_BADPARAM; return false; }

	if (bitstreamActive && !keepBits) CompleteBitstream();

	for (int i = bytes - 1; i >= 0; i--)
	{
		if (currentBufferPos >= packageSize)
		{
			if (!WriteData()) { lastError = IO_ERROR_UNKNOWN; return false; }
		}

		dataBuffer[currentBufferPos] = IOGetByte(number, i);
		if (currentFilePos == size) size++;
		currentBufferPos++;
		currentFilePos++;
	}

	if (currentBufferPos >= packageSize)
	{
		if (!WriteData()) { lastError = IO_ERROR_UNKNOWN; return false; }
	}

	return true;
}

S::Bool S::IO::OutStream::OutputBits(Int64 number, Int bits)
{
	if (streamType == STREAM_NONE)	{ lastError = IO_ERROR_NOTOPEN; return false; }
	if (bits > 64 || bits < 0)	{ lastError = IO_ERROR_BADPARAM; return false; }

	if (!bitstreamActive) InitBitstream();

	for (int j = 0; j < bits; j++)
	{
		bitBuffer[bitLength] = IOGetBit(number, j);
		bitLength++;
	}

	while (bitLength >= 8)
	{
		unsigned char	 out = 0;

		for (int i = 0; i < 8; i++) out = out | (bitBuffer[i] << i);

		bitLength = bitLength - 8;

		for (int j = 0; j < bitLength; j++) bitBuffer[j] = bitBuffer[j + 8];

		dataBuffer[currentBufferPos] = out;

		if (currentFilePos == size) size++;

		currentBufferPos++;
		currentFilePos++;

		if (currentBufferPos >= packageSize)
		{
			if (!WriteData()) { lastError = IO_ERROR_UNKNOWN; return false; }
		}
	}

	return true;
}

S::Bool S::IO::OutStream::OutputString(const String &string)
{
	if (streamType == STREAM_NONE)		{ lastError = IO_ERROR_NOTOPEN; return false; }
	if (string == NIL)			{ lastError = IO_ERROR_BADPARAM; return false; }

	if (bitstreamActive && !keepBits) CompleteBitstream();

	/* Convert the string to char * here once to avoid
	 * having to convert it again and again later.
	 */
	const char	*value		= string;

	int		 bytesleft	= strlen(value);
	int		 databufferpos	= 0;

	while (bytesleft)
	{
		int	 amount = ((packageSize - currentBufferPos) < (bytesleft)) ? (packageSize - currentBufferPos) : (bytesleft);

		for (Int i = 0; i < amount; i++) ((char *) (((UnsignedByte *) dataBuffer) + currentBufferPos))[i] = value[databufferpos + i];

		bytesleft	 -= amount;
		databufferpos	 += amount;
		currentBufferPos += amount;
		currentFilePos	 += amount;

		if (size < currentFilePos) size = currentFilePos;

		if (!WriteData()) { lastError = IO_ERROR_UNKNOWN; return false; }
	}

	return true;
}

S::Bool S::IO::OutStream::OutputLine(const String &string)
{
	OutputString(string);

#if (defined __WIN32__ || defined MSDOS) && !defined __CYGWIN32__
	OutputNumber(13, 1);
#endif

	OutputNumber(10, 1);

	return true;
}

S::Bool S::IO::OutStream::OutputData(const Void *pointer, Int bytes)
{
	if (streamType == STREAM_NONE)		{ lastError = IO_ERROR_NOTOPEN; return false; }
	if (pointer == NULL)			{ lastError = IO_ERROR_BADPARAM; return false; }
	if (bytes < 0)				{ lastError = IO_ERROR_BADPARAM; return false; }

	if (bitstreamActive && !keepBits) CompleteBitstream();

	int	 bytesleft	= bytes;
	int	 databufferpos	= 0;

	while (bytesleft)
	{
		int	 amount = ((packageSize - currentBufferPos) < (bytesleft)) ? (packageSize - currentBufferPos) : (bytesleft);

		memcpy((void *) (((UnsignedByte *) dataBuffer) + currentBufferPos), (void *) ((unsigned char *) pointer + databufferpos), amount);

		bytesleft	 -= amount;
		databufferpos	 += amount;
		currentBufferPos += amount;
		currentFilePos	 += amount;

		if (size < currentFilePos) size = currentFilePos;

		if (!WriteData()) { lastError = IO_ERROR_UNKNOWN; return false; }
	}

	return true;
}

S::Bool S::IO::OutStream::InitBitstream()
{
	bitLength	= 0;
	bitstreamActive	= 1;

	return true;
}

S::Bool S::IO::OutStream::CompleteBitstream()
{
	if (bitLength > 0)
	{
		int	out = 0;

		for (int i = 0; i < 8; i++)
		{
			if (i < bitLength) out = out | (bitBuffer[i] << i);
		}

		keepBits = true;
		OutputNumber(out, 1);
		keepBits = false;
	}

	bitstreamActive = 0;

	return true;
}

S::Bool S::IO::OutStream::SetPackageSize(Int newPackageSize)
{
	if (streamType == STREAM_NONE)	{ lastError = IO_ERROR_NOTOPEN; return false; }
	if (!allowpackset)		{ lastError = IO_ERROR_OPNOTAVAIL; return false; }
	if (newPackageSize <= 0)	{ lastError = IO_ERROR_BADPARAM; return false; }

	Flush();

	dataBuffer.Resize(newPackageSize);

	packageSize = newPackageSize;
	stdpacksize = packageSize;

	return true;
}

S::Bool S::IO::OutStream::AddFilter(Filter *newFilter)
{
	if (streamType == STREAM_NONE)	{ lastError = IO_ERROR_NOTOPEN; return false; }

	newFilter->SetDriver(driver);

	if (newFilter->Activate() == false) { lastError = IO_ERROR_UNKNOWN; return false; }

	if (bitstreamActive && !keepBits) CompleteBitstream();

	Flush();

	filters.Add(newFilter);

	allowpackset = true;

	if (newFilter->GetPackageSize() > 0)
	{
		SetPackageSize(newFilter->GetPackageSize());	// package size must be eqv filter size

		allowpackset = false;
	}
	else if (newFilter->GetPackageSize() == -1)
	{
		SetPackageSize(defaultPackageSize);

		allowpackset = false;
	}

	return true;
}

S::Bool S::IO::OutStream::RemoveFilter(Filter *oldFilter)
{
	if (streamType == STREAM_NONE)	{ lastError = IO_ERROR_NOTOPEN; return false; }

	Int	 index = -1;

	for (Int i = 0; i < filters.Length(); i++) if (filters.GetNth(i) == oldFilter) index = filters.GetNthIndex(i);

	if (index == -1) { lastError = IO_ERROR_BADPARAM; return false; }

	if (bitstreamActive && !keepBits) CompleteBitstream();

	int	 oldcpos = currentBufferPos;

	if (oldFilter->GetPackageSize() > 0 && currentBufferPos != 0)
	{
		for (int i = 0; i < (packageSize - oldcpos); i++) OutputNumber(0, 1);
	}
	else
	{
		if (oldFilter->GetPackageSize() == -1) allowpackset = true;

		Flush();
	}

	allowpackset = true;

	oldFilter->Deactivate();

	filters.Remove(index);

	SetPackageSize(origpacksize);

	return true;
}

S::Bool S::IO::OutStream::Close()
{
	if (streamType == STREAM_NONE) { lastError = IO_ERROR_NOTOPEN; return false; }

	while (filters.Length() != 0) RemoveFilter(filters.GetLast());

	Flush();

	if (crosslinked)
	{
		if (closefile) inStream->closefile = true;

		inStream->crosslinked	= false;
		inStream->outStream	= NULL;

		closefile = false;
	}

	if (closefile) delete driver;

	streamType = STREAM_NONE;

	return true;
}

S::Bool S::IO::OutStream::Seek(Int64 position)
{
	if (streamType == STREAM_NONE)	{ lastError = IO_ERROR_NOTOPEN; return false; }

	Flush();

	driver->Seek(position);

	currentFilePos	 = position;
	currentBufferPos = 0;

	return true;
}

S::Bool S::IO::OutStream::RelSeek(Int64 offset)
{
	if (streamType == STREAM_NONE)	{ lastError = IO_ERROR_NOTOPEN; return false; }

	Flush();

	driver->Seek(currentFilePos + offset);

	currentFilePos	 += offset;
	currentBufferPos = 0;

	return true;
}
