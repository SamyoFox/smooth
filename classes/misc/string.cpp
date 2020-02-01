 /* The smooth Class Library
  * Copyright (C) 1998-2020 Robert Kausch <robert.kausch@gmx.net>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of "The Artistic License, Version 2.0".
  *
  * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

#include <smooth/misc/string.h>
#include <smooth/misc/number.h>
#include <smooth/misc/math.h>
#include <smooth/misc/encoding/base64.h>
#include <smooth/misc/hash/crc32.h>
#include <smooth/misc/hash/crc64.h>
#include <smooth/system/cpu.h>
#include <smooth/templates/buffer.h>
#include <smooth/templates/threadlocal.h>
#include <smooth/init.h>

#include <iconv.h>
#include <wchar.h>
#include <memory.h>
#include <stdlib.h>

#ifdef __WIN32__
#	include <windows.h>
#endif

namespace smooth
{
	typedef Array<char *, Void *>		 BufferArray;

	static multithread (BufferArray *)	 allocatedBuffers = NIL;

	static multithread (char *)		 inputFormat	  = NIL;
	static multithread (char *)		 outputFormat	  = NIL;

	Int					 ConvertString(const char *, Int, const char *, char *, Int, const char *);
};

S::Int	 addStringInitTmp = S::AddInitFunction(&S::String::Initialize);
S::Int	 addStringFreeTmp = S::AddFreeFunction(&S::String::Free);

S::String::String(const int nil)
{
}

S::String::String(const char *iString)
{
	*this = iString;
}

S::String::String(const wchar_t *iString)
{
	*this = iString;
}

S::String::String(const String &iString)
{
	*this = iString;
}

S::String::~String()
{
}

S::Int S::String::Initialize()
{
	return Success();
}

S::Int S::String::Free()
{
	/* Delete all temporarily allocated string buffers.
	 */
	DeleteTemporaryBuffers(True);

	return Success();
}

const char *S::String::GetDefaultEncoding()
{
#ifdef __WIN32__
	static char	 encoding[16] = { };

	if (strlen(encoding) > 0) return encoding;

	/* Find system codepage.
	 */
	Int	 codePage = GetACP();

	if (codePage != CP_UTF8)
	{
		char	 buffer[12];

		strncpy(encoding, "CP", 3);
		strncat(encoding, itoa(codePage, buffer, 10), 12);
	}
	else
	{
		strncpy(encoding, "UTF-8", 6);
	}

	return encoding;
#else
	return "UTF-8";
#endif
}

S::Void S::String::AddTemporaryBuffer(char *buffer)
{
	/* Free old buffers and add new.
	 */
	if (allocatedBuffers == NIL) allocatedBuffers = new BufferArray();

	if (allocatedBuffers->Length() >= 8) DeleteTemporaryBuffers();

	allocatedBuffers->Add(buffer);
}

S::Void S::String::DeleteTemporaryBuffers(Bool all)
{
	/* Free all but the most recent 8 buffers.
	 */
	if (allocatedBuffers != NIL)
	{
		Int	 nOfEntries = allocatedBuffers->Length();

		for (Int i = 0; i < nOfEntries - (all ? 0 : 8); i++)
		{
			delete [] allocatedBuffers->GetFirst();

			allocatedBuffers->RemoveNth(0);
		}

		if (all)
		{
			delete allocatedBuffers;

			allocatedBuffers = NIL;
		}
	}

	/* When purging all, also free I/O format strings.
	 */
	if (all)
	{
		if (inputFormat	 != NIL) delete [] inputFormat;
		if (outputFormat != NIL) delete [] outputFormat;

		inputFormat  = NIL;
		outputFormat = NIL;
	}
}

S::Void S::String::Clean()
{
	wString.Resize(0);
}

S::UnsignedInt32 S::String::ComputeCRC32() const
{
	if (wString.Size() == 0) return 0;

	return Hash::CRC32::Compute((UnsignedByte *) (wchar_t *) wString, Length() * sizeof(wchar_t));
}

S::UnsignedInt64 S::String::ComputeCRC64() const
{
	if (wString.Size() == 0) return 0;

	return Hash::CRC64::Compute((UnsignedByte *) (wchar_t *) wString, Length() * sizeof(wchar_t));
}

S::String S::String::EncodeBase64() const
{
	if (wString.Size() == 0) return NIL;

	const char		*utf8string = ConvertTo("UTF-8");
	int			 length	    = strlen(utf8string);

	Buffer<UnsignedByte>	 buffer(length);

	strncpy((char *) (UnsignedByte *) buffer, utf8string, length);

	return Encoding::Base64(buffer).Encode();
}

S::String S::String::DecodeBase64() const
{
	if (wString.Size() == 0) return NIL;

	Buffer<UnsignedByte>	 buffer;
	String			 string;

	Encoding::Base64(buffer).Decode(*this);

	string.ImportFrom("UTF-8", (char *) (UnsignedByte *) buffer);

	return string;
}

S::Bool S::String::IsANSI(const String &string)
{
	return !IsUnicode(string);
}

S::Bool S::String::IsUnicode(const String &string)
{
	const wchar_t	*chars	= string;
	Int		 length = string.Length();

	for (Int i = 0; i < length; i++)
	{
		if ( chars[i] <	 0x20 || chars[i] > 0xFF ||
		    (chars[i] >= 0x80 && chars[i] < 0xA0)) return True;
	}

	return False;
}

const char *S::String::GetInputFormat()
{
	if (inputFormat == NIL) return GetDefaultEncoding();

	return inputFormat;
}

const char *S::String::SetInputFormat(const char *iFormat)
{
	if (iFormat == NIL) return SetInputFormat(GetDefaultEncoding());

	/* Return immediately if requested format equals current.
	 */
	if (inputFormat != NIL && strcmp(iFormat, inputFormat) == 0) return inputFormat;

	/* Backup input format and set new one.
	 */
	char	*previousInputFormat = inputFormat;

	inputFormat = NIL;

	if (strcmp(iFormat, GetDefaultEncoding()) != 0)
	{
		int	 length = strlen(iFormat) + 1;

		inputFormat = new char [length];

		strncpy(inputFormat, iFormat, length);
	}

	/* Add previous format to temporary buffers list.
	 */
	if (previousInputFormat != NIL)
	{
		AddTemporaryBuffer(previousInputFormat);

		return previousInputFormat;
	}

	return GetDefaultEncoding();
}

const char *S::String::GetOutputFormat()
{
	if (outputFormat == NIL) return GetDefaultEncoding();

	return outputFormat;
}

const char *S::String::SetOutputFormat(const char *oFormat)
{
	if (oFormat == NIL) return SetOutputFormat(GetDefaultEncoding());

	/* Return immediately if requested format equals current.
	 */
	if (outputFormat != NIL && strcmp(oFormat, outputFormat) == 0) return outputFormat;

	/* Backup output format and set new one.
	 */
	char	*previousOutputFormat = outputFormat;

	outputFormat = NIL;

	if (strcmp(oFormat, GetDefaultEncoding()) != 0)
	{
		int	 length = strlen(oFormat) + 1;

		outputFormat = new char [length];

		strncpy(outputFormat, oFormat, length);
	}

	/* Add previous format to temporary buffers list.
	 */
	if (previousOutputFormat != NIL)
	{
		AddTemporaryBuffer(previousOutputFormat);

		return previousOutputFormat;
	}

	return GetDefaultEncoding();
}

const char *S::String::GetInternalFormat()
{
	static char	*internalFormat = NIL;

	if (internalFormat == NIL)
	{
		System::Endianness	 endianness = System::CPU().GetEndianness();

		if	(sizeof(wchar_t) == 2) internalFormat = (char *) (endianness == System::EndianLittle ? "UTF-16LE" : "UTF-16BE");
		else if (sizeof(wchar_t) == 4) internalFormat = (char *) (endianness == System::EndianLittle ? "UTF-32LE" : "UTF-32BE");
	}

	return internalFormat;
}

S::Int S::String::ImportFrom(const char *format, const char *str)
{
	Clean();

	if (str == NIL) return Success();

	Int	 width = 1;

	if	(strncmp(format, "UCS-4", 5)		== 0) width = 4;
	else if	(strncmp(format, "UCS-2", 5)		== 0) width = 2;
	else if	(strncmp(format, "UTF-32", 6)		== 0) width = 4;
	else if	(strncmp(format, "UTF-16", 6)		== 0) width = 2;
	else if	(strncmp(format, "CSUCS4", 6)		== 0) width = 4;
	else if	(strncmp(format, "ISO-10646-UCS-4", 15)	== 0) width = 4;
	else if	(strncmp(format, "ISO-10646-UCS-2", 15)	== 0) width = 2;
	else if	(strncmp(format, "UNICODELITTLE", 13)	== 0) width = 2;
	else if	(strncmp(format, "UNICODEBIG", 10)	== 0) width = 2;
	else if	(strncmp(format, "CSUNICODE", 9)	== 0) width = 2;
	else if	(strncmp(format, "UNICODE-1-1", 11)	== 0) width = 2;
	else if	(strncmp(format, "CSUNICODE11", 11)	== 0) width = 2;

	Int	 length = -1;

	if	(width == 1) while (true) { if (((char  *) str)[++length] == 0) { length *= 1; break; } }
	else if (width == 2) while (true) { if (((short *) str)[++length] == 0) { length *= 2; break; } }
	else if (width == 4) while (true) { if (((int   *) str)[++length] == 0) { length *= 4; break; } }

	Int	 size = ConvertString(str, length, format, NIL, 0, GetInternalFormat());

	if	(size < 0 && strcmp(format, "ISO-8859-1") != 0) return ImportFrom("ISO-8859-1", str);
	else if (size < 0)					return Error();

	size = size / sizeof(wchar_t) + 1;

	wString.Resize(size);

	ConvertString(str, length, format, (char *) (wchar_t *) wString, size * sizeof(wchar_t), GetInternalFormat());

	wString[size - 1] = 0;

	return Success();
}

char *S::String::ConvertTo(const char *encoding) const
{
	Int	 size = Length() + 1;

	if (size == 1) return NIL;

	Int	 bufferSize = ConvertString((char *) (wchar_t *) wString, size * sizeof(wchar_t), GetInternalFormat(), NIL, 0, encoding);

	if (bufferSize == -1) bufferSize = ConvertString((char *) (wchar_t *) wString, size * sizeof(wchar_t), GetInternalFormat(), NIL, 0, "ISO-8859-1");

	char	*buffer = NIL;

	if (bufferSize <= 0)
	{
		Int	 length = Length();

		buffer = new char [length + 1];

		ConvertString((char *) (wchar_t *) wString, size * sizeof(wchar_t), GetInternalFormat(), buffer, length + 1, encoding);

		for (Int i = -bufferSize; i < length; i++) buffer[i] = '?';

		buffer[length] = 0;
	}
	else
	{
		buffer = new char [bufferSize + 1];

		ConvertString((char *) (wchar_t *) wString, size * sizeof(wchar_t), GetInternalFormat(), buffer, bufferSize + 1, encoding);
	}

	AddTemporaryBuffer(buffer);

	return buffer;
}

wchar_t &S::String::operator [](int n)
{
	static wchar_t	 dummy = 0;

	if (n < 0) return dummy;

	if (n + 1 >= wString.Size())
	{
		Int	 length = Length();

		/* Allocate more memory than actually
		 * needed to speed up string operations.
		 */
		wString.Resize(n + Math::Max(10, Math::Min(n / 10, 100000)));

		wmemset(wString + length, 0, wString.Size() - length);
	}

	return wString[n];
}

wchar_t S::String::operator []( int n) const
{
	if (n < 0 || n + 1 >= wString.Size()) return 0;

	return wString[n];
}

S::String::operator char *() const
{
	return ConvertTo(GetOutputFormat());
}

S::String::operator wchar_t *() const
{
	return wString;
}

S::String &S::String::operator =(const int nil)
{
	Clean();

	return *this;
}

S::String &S::String::operator =(const char *newString)
{
	Clean();

	if (newString != NIL) ImportFrom(GetInputFormat(), newString);

	return *this;
}

S::String &S::String::operator =(const wchar_t *newString)
{
	Clean();

	if (newString != NIL)
	{
		Int	 size = wcslen(newString) + 1;

		wString.Resize(size);

		wcsncpy(wString, newString, size);
	}

	return *this;
}

S::String &S::String::operator =(const String &newString)
{
	if (&newString == this) return *this;

	Clean();

	if (newString.wString.Size() > 0)
	{
		Int	 size = newString.wString.Size();

		wString.Resize(size);

		wcsncpy(wString, newString.wString, size);
	}

	return *this;
}

S::Bool S::String::operator ==(const int nil) const
{
	if (wString.Size() == 0) return True;
	if (wString[0] == 0)	 return True;
	else			 return False;
}

S::Bool S::String::operator ==(const char *str) const
{
	if (wString.Size() == 0 && str == NIL) return True;
	if (wString.Size() == 0 && str != NIL) if (str[0] == 0) return True;
	if (wString.Size() != 0 && str == NIL) if (wString[0] == 0) return True;
	if (wString.Size() == 0 || str == NIL) return False;

	if (!Compare(str)) return True;
	else		   return False;
}

S::Bool S::String::operator ==(const wchar_t *str) const
{
	if (wString.Size() == 0 && str == NIL) return True;
	if (wString.Size() == 0 && str != NIL) if (str[0] == 0) return True;
	if (wString.Size() != 0 && str == NIL) if (wString[0] == 0) return True;
	if (wString.Size() == 0 || str == NIL) return False;

	if (!Compare(str)) return True;
	else		   return False;
}

S::Bool S::String::operator ==(const String &str) const
{
	if (wString.Size() == 0 && str.wString.Size() == 0) return True;
	if (wString.Size() == 0 && str.wString.Size() != 0) if (str.wString[0] == 0) return True;
	if (wString.Size() != 0 && str.wString.Size() == 0) if (wString[0] == 0) return True;
	if (wString.Size() == 0 || str.wString.Size() == 0) return False;

	if (!Compare(str)) return True;
	else		   return False;
}

S::Bool S::String::operator !=(const int nil) const
{
	if (wString.Size() == 0) return False;
	if (wString[0] == 0)	 return False;
	else			 return True;
}

S::Bool S::String::operator !=(const char *str) const
{
	if (wString.Size() == 0 && str == NIL)	return False;
	if (wString.Size() == 0 && str != NIL)	if (str[0] == 0) return False;
	if (wString.Size() != 0 && str == NIL)	if (wString[0] == 0) return False;
	if (wString.Size() == 0 || str == NIL)	return True;

	if (Compare(str) != 0) return True;
	else		       return False;
}

S::Bool S::String::operator !=(const wchar_t *str) const
{
	if (wString.Size() == 0 && str == NIL) return False;
	if (wString.Size() == 0 && str != NIL) if (str[0] == 0) return False;
	if (wString.Size() != 0 && str == NIL) if (wString[0] == 0) return False;
	if (wString.Size() == 0 || str == NIL) return True;

	if (Compare(str) != 0) return True;
	else		       return False;
}

S::Bool S::String::operator !=(const String &str) const
{
	if (wString.Size() == 0 && str.wString.Size() == 0) return False;
	if (wString.Size() == 0 && str.wString.Size() != 0) if (str.wString[0] == 0) return False;
	if (wString.Size() != 0 && str.wString.Size() == 0) if (wString[0] == 0) return False;
	if (wString.Size() == 0 || str.wString.Size() == 0) return True;

	if (Compare(str) != 0) return True;
	else		       return False;
}

S::Int S::String::Length() const
{
	if (wString.Size() == 0) return 0;

	return wcslen(wString);
}

S::Int S::String::Find(const String &str) const
{
	Int	 len1 = Length();
	Int	 len2 = str.Length();

	for (Int i = 0; i <= len1 - len2; i++)
	{
		if (wcsncmp(wString + i, str.wString, len2) == 0) return i;
	}

	return -1;
}

S::Int S::String::FindLast(const String &str) const
{
	Int	 len1 = Length();
	Int	 len2 = str.Length();

	for (Int i = len1 - len2; i >= 0; i--)
	{
		if (wcsncmp(wString + i, str.wString, len2) == 0) return i;
	}

	return -1;
}

S::String &S::String::Append(const String &str)
{
	Int	 len1 = Length();
	Int	 len2 = str.Length();

	wString.Resize(len1 + len2 + 1);

	wcsncpy(wString + len1, str.wString, len2);

	wString[len1 + len2] = 0;

	return *this;
}

S::String S::String::Append(const String &str) const
{
	return String(*this).Append(str);
}

S::String &S::String::Replace(const String &str1, const String &str2)
{
	if (str1 == NIL) return *this;

	Int	 len1 = Length();
	Int	 len2 = str1.Length();
	Int	 len3 = str2.Length();

	for (Int i = 0; i <= len1 - len2; i++)
	{
		if (wcsncmp(wString + i, str1.wString, len2) == 0)
		{
			if (len2 != len3)
			{
				if (len3 > len2) wString.Resize(len1 + 1 + (len3 - len2));

				wmemmove(wString + i + len3, wString + i + len2, len1 - i - len2 + 1);

				len1 += (len3 - len2);
			}

			wcsncpy(wString + i, str2.wString, len3);

			i += len3 - 1;
		}
	}

	return *this;
}

S::String S::String::Replace(const String &str1, const String &str2) const
{
	return String(*this).Replace(str1, str2);
}

S::String &S::String::Copy(const String &str)
{
	*this = str;

	return *this;
}

S::String &S::String::CopyN(const String &str, const Int n)
{
	Clean();

	wString.Resize(n + 1);

	wcsncpy(wString, str.wString, n);

	wString[n] = 0;

	return *this;
}

S::Bool S::String::Contains(const String &str) const
{
	if (Find(str) >= 0) return True;
	else		    return False;
}

S::Int S::String::Compare(const String &str) const
{
	Int	 len1 = Length();
	Int	 len2 = str.Length();

	if	(len1 != len2)				   return len1 - len2;
	else if (wcsncmp(wString, str.wString, len1) != 0) return 1;

	return 0;
}

S::Int S::String::CompareN(const String &str, Int n) const
{
	if (Length() < n) return 1;

	if (wcsncmp(wString, str.wString, n) != 0) return 1;

	return 0;
}

S::Bool S::String::StartsWith(const String &str) const
{
	Int	 len1 = Length();
	Int	 len2 = str.Length();

	if (len1 >= len2)
	{
		if (wcsncmp(wString, str.wString, len2) != 0) return False;

		return True;
	}

	return False;
}

S::Bool S::String::EndsWith(const String &str) const
{
	Int	 len1 = Length();
	Int	 len2 = str.Length();

	if (len1 >= len2)
	{
		if (wcsncmp(wString + len1 - len2, str.wString, len2) != 0) return False;

		return True;
	}

	return False;
}

S::String S::String::SubString(Int start, Int number) const
{
	if (number <= 0 || start < 0 || start >= Length()) return NIL;

	String	 subString;

	subString.wString.Resize(number + 1);

	wcsncpy(subString.wString, wString + start, number);

	subString.wString[number] = 0;

	return subString;
}

S::String S::String::Head(Int n) const
{
	return SubString(0, n);
}

S::String S::String::Tail(Int n) const
{
	return SubString(Length() - n, n);
}

S::String S::String::Trim() const
{
	Int	 length	= Length();
	Int	 triml	= 0;
	Int	 trimr	= 0;

	for (Int i = 0; i < length; i++)
	{
		if ((*this)[i] == ' '  ||
		    (*this)[i] == '\t' ||
		    (*this)[i] == '\n' ||
		    (*this)[i] == '\r' ||
		    (*this)[i] == '\0' ||
		    (*this)[i] == '\x0B') triml++;
		else			  break;
	}

	for (Int i = length - 1; i >= 0; i--)
	{
		if ((*this)[i] == ' '  ||
		    (*this)[i] == '\t' ||
		    (*this)[i] == '\n' ||
		    (*this)[i] == '\r' ||
		    (*this)[i] == '\0' ||
		    (*this)[i] == '\x0B') trimr++;
		else			  break;
	}

	return SubString(triml, length - triml - trimr);
}

S::String &S::String::Fill(const Int value)
{
	wmemset(wString, value, Length());

	return *this;
}

S::String &S::String::FillN(const Int value, const Int count)
{
	Clean();

	if (count >= 0)
	{
		wString.Resize(count + 1);

		wmemset(wString, value, count);

		wString[count] = 0;
	}

	return *this;
}

S::Int64 S::String::ToInt() const
{
	return Number::FromIntString(*this);
}

S::Float S::String::ToFloat() const
{
	return Number::FromFloatString(*this);
}

S::String S::String::FromInt(const Int64 value)
{
	return Number(value).ToIntString();
}

S::String S::String::FromFloat(Float value)
{
	return Number(value).ToFloatString();
}

S::Array<S::String> S::String::Explode(const String &delimiter) const
{
	/* Split string and add entries to array.
	 */
	Array<String>	 array;
	Int		 length	= Length();

	for (Int i = 0; i < length; )
	{
		Int	 index = SubString(i, length - i).Find(delimiter);
		String	 part;

		if (index == -1) part = SubString(i, length - i);
		else		 part = SubString(i, index);

		array.Add(part);

		i += part.Length() + delimiter.Length();
	}

	return array;
}

S::String S::String::Implode(const Array<String> &array, const String &delimiter)
{
	String	 string;

	for (Int i = 0; i < array.Length(); i++)
	{
		if (i > 0) string.Append(delimiter);

		string.Append(array.GetNth(i));
	}

	return string;
}

S::Int S::ConvertString(const char *inBuffer, Int inBytes, const char *inEncoding, char *outBuffer, Int outBytes, const char *outEncoding)
{
	/* Copy string if both encodings are the same.
	 */
	if (strcmp(inEncoding, outEncoding) == 0)
	{
		Int	 size = inBytes;

		if (outBuffer != NIL)
		{
			if (size < outBytes && size > 0) wcsncpy((wchar_t *) outBuffer, (wchar_t *) inBuffer, size / sizeof(wchar_t));

			if (size >= outBytes) size = 0;
		}

		return size;
	}

	/* Switch bytes for conversion between little and big endian.
	 */
	if ((strcmp(inEncoding, "UTF-16LE") == 0 && strcmp(outEncoding, "UTF-16BE") == 0) || (strcmp(inEncoding, "UTF-16BE") == 0 && strcmp(outEncoding, "UTF-16LE") == 0))
	{
		Int	 size = inBytes;

		if (outBuffer != NIL)
		{
			if (size < outBytes && size > 0) for (UnsignedInt i = 0; i < size / sizeof(wchar_t); i++) ((wchar_t *) outBuffer)[i] = ((((wchar_t *) inBuffer)[i] & 255) << 8) | (((wchar_t *) inBuffer)[i] >> 8);

			if (size >= outBytes) size = 0;
		}

		return size;
	}

	/* Convert using iconv/libiconv.
	 */
	if (Setup::useIconv)
	{
		/* Open and configure iconv.
		 */
		iconv_t	 cd = iconv_open(outEncoding, inEncoding);

		if (cd != (iconv_t) -1)
		{
#if _LIBICONV_VERSION >= 0x0108
			int	 on = 1;

			iconvctl(cd, ICONV_SET_TRANSLITERATE, &on);
#endif

			/* Assign output buffer if not provided.
			 */
			if (outBuffer == NIL)
			{
				static multithread (Buffer<char> *)	 buffer = NIL;

				if (buffer == NIL) buffer = new Buffer<char>();

				if (buffer->Size() > 4096 && inBytes * 8 < 256) buffer->Free();

				buffer->Resize(inBytes * 8);

				outBytes  = buffer->Size();
				outBuffer = *buffer;
			}

			/* Perform actual conversion.
			 */
			size_t		 inBytesLeft  = inBytes;
			size_t		 outBytesLeft = outBytes;
			char	       **outPointer   = &outBuffer;

#if defined __NetBSD__ || defined __sun
			const char     **inPointer    = &inBuffer;
#else
			char	       **inPointer    = const_cast<char **>(&inBuffer);
#endif

			while (inBytesLeft)
			{
				if (iconv(cd, inPointer, &inBytesLeft, outPointer, &outBytesLeft) == (size_t) -1) break;
			}

			iconv_close(cd);

			Int	 size = !outBytesLeft ? 0 : outBytes - outBytesLeft;

			if (inBytesLeft) size *= -1;

			return size;
		}
	}

#ifdef __WIN32__
	/* Convert to UTF-16LE on Windows.
	 */
	if (strcmp(outEncoding, "UTF-16LE") == 0)
	{
		Int	 codePage = CP_ACP;

		if	(strcmp(inEncoding, "UTF-8")	   == 0) codePage = CP_UTF8;
		else if (strcmp(inEncoding, "UTF-7")	   == 0) codePage = CP_UTF7;
		else if (strcmp(inEncoding, "ISO-8859-1")  == 0) codePage = 28591;
		else if (strcmp(inEncoding, "ISO-8859-2")  == 0) codePage = 28592;
		else if (strcmp(inEncoding, "ISO-8859-3")  == 0) codePage = 28593;
		else if (strcmp(inEncoding, "ISO-8859-4")  == 0) codePage = 28594;
		else if (strcmp(inEncoding, "ISO-8859-5")  == 0) codePage = 28595;
		else if (strcmp(inEncoding, "ISO-8859-6")  == 0) codePage = 28596;
		else if (strcmp(inEncoding, "ISO-8859-7")  == 0) codePage = 28597;
		else if (strcmp(inEncoding, "ISO-8859-8")  == 0) codePage = 28598;
		else if (strcmp(inEncoding, "ISO-8859-9")  == 0) codePage = 28599;
		else if (strcmp(inEncoding, "ISO-8859-15") == 0) codePage = 28605;
		else if (strcmp(inEncoding, "KOI8-R")	   == 0) codePage = 20866;
		else if (strcmp(inEncoding, "KOI8-U")	   == 0) codePage = 21866;
		else if (strcmp(inEncoding, "SHIFT-JIS")   == 0) codePage = 932;
		else if (strcmp(inEncoding, "GBK")	   == 0) codePage = 936;
		else if (strcmp(inEncoding, "BIG-5")	   == 0) codePage = 950;
		else if (strcmp(inEncoding, "ISO-2022-JP") == 0) codePage = 50220;
		else if (strcmp(inEncoding, "ISO-2022-KR") == 0) codePage = 50225;
		else if (strcmp(inEncoding, "ISO-2022-CN") == 0) codePage = 50227;

		if (inEncoding[0] == 'C' && inEncoding[1] == 'P') codePage = atoi(inEncoding + 2);

		Int	 size = MultiByteToWideChar(codePage, 0, inBuffer, -1, NIL, 0) * sizeof(wchar_t);

		/* Codepage not installed? Let's try CP_ACP!
		 */
		if (size == 0 && GetLastError() == ERROR_INVALID_PARAMETER) size = MultiByteToWideChar(codePage = CP_ACP, 0, inBuffer, -1, NIL, 0) * sizeof(wchar_t);

		if (outBuffer != NIL)
		{
			if (size < outBytes && size > 0) MultiByteToWideChar(codePage, 0, inBuffer, -1, (wchar_t *) outBuffer, size / sizeof(wchar_t));

			if (size >= outBytes) size = 0;
		}

		return size;
	}

	/* Convert from UTF-16LE on Windows.
	 */
	if (strcmp(inEncoding, "UTF-16LE") == 0)
	{
		Int	 codePage = CP_ACP;

		if	(strcmp(outEncoding, "UTF-8")	    == 0) codePage = CP_UTF8;
		else if (strcmp(outEncoding, "UTF-7")	    == 0) codePage = CP_UTF7;
		else if (strcmp(outEncoding, "ISO-8859-1")  == 0) codePage = 28591;
		else if (strcmp(outEncoding, "ISO-8859-2")  == 0) codePage = 28592;
		else if (strcmp(outEncoding, "ISO-8859-3")  == 0) codePage = 28593;
		else if (strcmp(outEncoding, "ISO-8859-4")  == 0) codePage = 28594;
		else if (strcmp(outEncoding, "ISO-8859-5")  == 0) codePage = 28595;
		else if (strcmp(outEncoding, "ISO-8859-6")  == 0) codePage = 28596;
		else if (strcmp(outEncoding, "ISO-8859-7")  == 0) codePage = 28597;
		else if (strcmp(outEncoding, "ISO-8859-8")  == 0) codePage = 28598;
		else if (strcmp(outEncoding, "ISO-8859-9")  == 0) codePage = 28599;
		else if (strcmp(outEncoding, "ISO-8859-15") == 0) codePage = 28605;
		else if (strcmp(outEncoding, "KOI8-R")	    == 0) codePage = 20866;
		else if (strcmp(outEncoding, "KOI8-U")	    == 0) codePage = 21866;
		else if (strcmp(outEncoding, "SHIFT-JIS")   == 0) codePage = 932;
		else if (strcmp(outEncoding, "GBK")	    == 0) codePage = 936;
		else if (strcmp(outEncoding, "BIG-5")	    == 0) codePage = 950;
		else if (strcmp(outEncoding, "ISO-2022-JP") == 0) codePage = 50220;
		else if (strcmp(outEncoding, "ISO-2022-KR") == 0) codePage = 50225;
		else if (strcmp(outEncoding, "ISO-2022-CN") == 0) codePage = 50227;

		if (outEncoding[0] == 'C' && outEncoding[1] == 'P') codePage = atoi(outEncoding + 2);

		Int	 size = WideCharToMultiByte(codePage, 0, (wchar_t *) inBuffer, -1, NIL, 0, NIL, NIL);

		/* Codepage not installed? Let's try CP_ACP!
		 */
		if (size == 0 && GetLastError() == ERROR_INVALID_PARAMETER) size = WideCharToMultiByte(codePage = CP_ACP, 0, (wchar_t *) inBuffer, -1, NIL, 0, NIL, NIL);

		if (outBuffer != NIL)
		{
			if (size < outBytes && size > 0) WideCharToMultiByte(codePage, 0, (wchar_t *) inBuffer, -1, outBuffer, size, NIL, NIL);

			if (size >= outBytes) size = 0;
		}

		return size;
	}
#endif

	return -1;
}
