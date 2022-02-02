 /* The smooth Class Library
  * Copyright (C) 1998-2022 Robert Kausch <robert.kausch@gmx.net>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of "The Artistic License, Version 2.0".
  *
  * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

/* Define to get 64 bit stat call.
 */
#ifndef __WIN32__
#	define _FILE_OFFSET_BITS 64
#endif

#include <smooth/files/directory.h>

#ifdef __WIN32__
#	include <smooth/backends/win32/backendwin32.h>

#	include <windows.h>
#else
#	include <glob.h>
#	include <unistd.h>
#	include <stdio.h>
#	include <limits.h>
#	include <sys/stat.h>

#	ifndef PATH_MAX
#		define PATH_MAX 32768
#	endif

#	ifndef GLOB_ONLYDIR
#		define GLOB_ONLYDIR 0
#	endif
#endif

char	*S::Directory::directoryDelimiter = NIL;

S::Directory::Directory()
{
}

S::Directory::Directory(const String &iDirName, const String &iDirPath)
{
	const String	&delimiter = Directory::GetDirectoryDelimiter();

	dirName = iDirName;
	dirPath = iDirPath;

	dirName.Replace("/",  delimiter);
	dirName.Replace("\\", delimiter);

	dirPath.Replace("/",  delimiter);
	dirPath.Replace("\\", delimiter);

	if (dirName != NIL && dirPath == NIL)
	{
#ifdef __WIN32__
		if (dirName.StartsWith(delimiter) && !dirName.StartsWith("\\\\")) dirName = String(Directory::GetActiveDirectory()).Head(2).Append(dirName);
#endif

#ifdef __WIN32__
		if (dirName[1] == ':' || dirName.StartsWith("\\\\"))
#else
		if (dirName.StartsWith(delimiter) || dirName.StartsWith("~"))
#endif
		{
			dirPath = dirName;
			dirName = NIL;
		}
		else
		{
			dirPath = String(Directory::GetActiveDirectory()).Append(delimiter).Append(dirName);
			dirName = NIL;
		}
	}

	if (dirName == NIL)
	{
		if (dirPath.EndsWith(delimiter)) dirPath[dirPath.Length() - 1] = 0;

		Int	 lastBS = dirPath.FindLast(delimiter);

		if (lastBS >= 0)
		{
			dirName = dirPath.Tail(dirPath.Length() - lastBS - 1);
			dirPath[lastBS] = 0;
		}
	}

	/* Replace ./ elements.
	 */
	if (!dirPath.EndsWith(delimiter)) dirPath.Append(delimiter);

	dirPath.Replace(String(delimiter).Append(".").Append(delimiter), delimiter);

	if (dirPath.StartsWith(String(".").Append(delimiter))) dirPath = String(Directory::GetActiveDirectory()).Append(dirPath.Tail(dirPath.Length() - 2));

	/* Replace ../ elements.
	 */
	while (dirPath.Contains(String(delimiter).Append("..").Append(delimiter)))
	{
		Int	 upPos	= dirPath.Find(String(delimiter).Append("..").Append(delimiter));
		Int	 prePos	= dirPath.Head(upPos).FindLast(delimiter);

		dirPath.Replace(dirPath.SubString(prePos, upPos - prePos + 3), String());
	}

	if (dirPath.EndsWith(delimiter)) dirPath[dirPath.Length() - 1] = 0;
}

S::Directory::Directory(const Directory &iDirectory)
{
	*this = iDirectory;
}

S::Directory::~Directory()
{
}

S::Directory &S::Directory::operator =(const Directory &nDirectory)
{
	if (&nDirectory == this) return *this;

	dirName = nDirectory.dirName;
	dirPath = nDirectory.dirPath;

	return *this;
}

S::Directory::operator S::String() const
{
	return String(dirPath).Append(dirName == NIL ? String() : String(Directory::GetDirectoryDelimiter())).Append(dirName);
}

const S::String &S::Directory::GetDirectoryName() const
{
	return dirName;
}

const S::String &S::Directory::GetDirectoryPath() const
{
	return dirPath;
}

const S::Array<S::File> &S::Directory::GetFiles() const
{
	return GetFilesByPattern("*");
}

const S::Array<S::Directory> &S::Directory::GetDirectories() const
{
	return GetDirectoriesByPattern("*");
}

const S::Array<S::File> &S::Directory::GetFilesByPattern(const String &pattern) const
{
	files.RemoveAll();

#ifdef __WIN32__
	WIN32_FIND_DATA	 findData;
	HANDLE		 handle = FindFirstFile(MakeExtendedPath(*this).Append("\\").Append(pattern), &findData);

	Bool	 success = (handle != INVALID_HANDLE_VALUE);

	while (success)
	{
		if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) files.Add(File(findData.cFileName, *this));

		success = FindNextFile(handle, &findData);
	}

	FindClose(handle);
#else
	String	 path	  = String(*this).Replace("*", "\\*").Replace("?", "\\?").
					  Replace("[", "\\[").Replace("]", "\\]");
	glob_t	 fileData = { 0 };

	if (glob(path.Append("/").Append(pattern).ConvertTo("UTF-8"), GLOB_MARK, NIL, &fileData) == 0)
	{
		String::InputFormat	 inputFormat("UTF-8");

		for (size_t i = 0; i < fileData.gl_pathc; i++)
		{
			if (!String(fileData.gl_pathv[i]).EndsWith("/")) files.Add(File(fileData.gl_pathv[i]));
		}

		globfree(&fileData);
	}
#endif

	return files;
}

const S::Array<S::Directory> &S::Directory::GetDirectoriesByPattern(const String &pattern) const
{
	directories.RemoveAll();

#ifdef __WIN32__
	WIN32_FIND_DATA	 findData;
	HANDLE		 handle = FindFirstFile(MakeExtendedPath(*this).Append("\\").Append(pattern), &findData);

	Bool	 success = (handle != INVALID_HANDLE_VALUE);

	while (success)
	{
		if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && String(findData.cFileName) != "." && String(findData.cFileName) != "..") directories.Add(Directory(findData.cFileName, *this));

		success = FindNextFile(handle, &findData);
	}

	FindClose(handle);
#else
	String	 path	  = String(*this).Replace("*", "\\*").Replace("?", "\\?").
					  Replace("[", "\\[").Replace("]", "\\]");
	glob_t	 fileData = { 0 };

	if (glob(path.Append("/").Append(pattern).ConvertTo("UTF-8"), GLOB_MARK | GLOB_ONLYDIR, NIL, &fileData) == 0)
	{
		String::InputFormat	 inputFormat("UTF-8");

		for (size_t i = 0; i < fileData.gl_pathc; i++)
		{
			if (String(fileData.gl_pathv[i]).EndsWith("/")) directories.Add(Directory(fileData.gl_pathv[i]));
		}

		globfree(&fileData);
	}
#endif

	return directories;
}

S::DateTime S::Directory::GetCreateTime() const
{
	DateTime	 dateTime;

	if (!Exists()) return dateTime;

#ifdef __WIN32__
	WIN32_FIND_DATA	 findData;
	HANDLE		 handle = FindFirstFile(MakeExtendedPath(*this), &findData);

	FindClose(handle);

	SYSTEMTIME	 time;

	FileTimeToSystemTime(&findData.ftCreationTime, &time);

	dateTime.SetYMD(time.wYear, time.wMonth, time.wDay);
	dateTime.SetHMS(time.wHour, time.wMinute, time.wSecond);
#endif

	return dateTime;
}

S::Bool S::Directory::Exists() const
{
#ifdef __WIN32__
	/* Check if root directory of a drive
	 */
	if (dirPath[dirPath.Length() - 1] == ':' && dirName == NIL)
	{
		DWORD	 drives = GetLogicalDrives();

		if ((drives >> (dirPath.ToUpper()[0] - 'A')) & 1) return True;
		else						  return False;
	}

	WIN32_FIND_DATA	 findData;
	HANDLE		 handle = FindFirstFile(MakeExtendedPath(*this), &findData);

	if (handle == INVALID_HANDLE_VALUE) return False;

	FindClose(handle);

	if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) return False;
#else
	struct stat	 info;

	if (stat(String(*this).ConvertTo("UTF-8"), &info) != 0) return False;

	if (!S_ISDIR(info.st_mode)) return False;
#endif

	return True;
}

S::Int S::Directory::Create()
{
	if (Exists()) return Success();

	Bool	 result	   = False;
	String	 directory = *this;

	for (Int i = 1; i <= directory.Length(); i++)
	{
		if (directory[i] == '\\' || directory[i] == '/' || directory[i] == 0)
		{
			String	 path = directory.Head(i);

#ifdef __WIN32__
			result = CreateDirectory(MakeExtendedPath(path), NIL);
#else
			if (mkdir(path.ConvertTo("UTF-8"), 0777) == 0) result = True;
			else					       result = False;
#endif
		}
	}

	if (result == False) return Error();
	else		     return Success();
}

S::Int S::Directory::Copy(const Directory &destination)
{
	return Error();
}

S::Int S::Directory::Move(const Directory &destination)
{
	if (!Exists()) return Error();

#ifdef __WIN32__
	Bool	 result = MoveFile(MakeExtendedPath(*this), MakeExtendedPath(destination));
#else
	Bool	 result = (rename(String(*this).ConvertTo("UTF-8"), String(destination).ConvertTo("UTF-8")) == 0);
#endif

	if (result == False) return Error();
	else		     return Success();
}

S::Int S::Directory::Delete()
{
#ifdef __WIN32__
	Bool	 result = RemoveDirectory(MakeExtendedPath(*this));
#else
	Bool	 result = (rmdir(String(*this).ConvertTo("UTF-8")) == 0);
#endif

	if (result == False) return Error();
	else		     return Success();
}

S::Int S::Directory::Empty()
{
#ifdef __WIN32__
	WIN32_FIND_DATA	 findData;
	HANDLE		 handle = FindFirstFile(MakeExtendedPath(*this).Append("\\*"), &findData);

	if (handle == INVALID_HANDLE_VALUE) return Error();

	do
	{
		if (String(findData.cFileName) == "." || String(findData.cFileName) == "..") continue;

		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			Directory	 dir(findData.cFileName, *this);

			dir.Empty();
			dir.Delete();
		}
		else
		{
			File(findData.cFileName, *this).Delete();
		}
	}
	while (FindNextFile(handle, &findData));

	FindClose(handle);
#endif

	return Success();
}

const char *S::Directory::GetDirectoryDelimiter()
{
	if (directoryDelimiter == NIL)
	{
#ifdef __WIN32__
		directoryDelimiter = (char *) "\\";
#else
		directoryDelimiter = (char *) "/";
#endif
	}

	return directoryDelimiter;
}

const char *S::Directory::GetUnicodePathPrefix(const String &path)
{
#ifdef __WIN32__
	static const char	*unicodePathPrefix = "\\\\?\\";

	if (path.StartsWith("\\\\")) return "";

	return unicodePathPrefix;
#endif

	return "";
}

S::String S::Directory::MakeExtendedPath(const String &path)
{
#ifdef __WIN32__
	static const String	 extendedPathPrefix = "\\\\?\\";
	static const String	 uncPathPrefix	    = "\\\\?\\UNC\\";

	if (!path.StartsWith(extendedPathPrefix))
	{
		if (path.StartsWith("\\\\")) return String(uncPathPrefix).Append(path.Tail(path.Length() - 2));

		return String(extendedPathPrefix).Append(path);
	}
#endif

	return path;
}

S::String S::Directory::StripExtendedPathPrefix(const String &path)
{
#ifdef __WIN32__
	static const String	 extendedPathPrefix = "\\\\?\\";
	static const String	 uncPathPrefix	    = "\\\\?\\UNC\\";

	if (path.StartsWith(extendedPathPrefix))
	{
		if (path.StartsWith(uncPathPrefix)) return String("\\\\").Append(path.Tail(path.Length() - uncPathPrefix.Length()));

		return path.Tail(path.Length() - extendedPathPrefix.Length());
	}
#endif

	return path;
}

S::Directory S::Directory::GetActiveDirectory()
{
#ifdef __WIN32__
	Buffer<wchar_t>	 buffer(32768 + 1);

	GetCurrentDirectory(buffer.Size(), buffer);

	String		 dir = (wchar_t *) buffer;
#else
	Buffer<char>	 buffer(PATH_MAX + 1);
	String		 dir;

	if (getcwd(buffer, buffer.Size()) != NIL) dir = buffer;
#endif

	return Directory(NIL, dir);
}

S::Int S::Directory::SetActiveDirectory(const Directory &directory)
{
#ifdef __WIN32__
	Bool	 result = SetCurrentDirectory(MakeExtendedPath(directory).Append("\\"));
#else
	Bool	 result = (chdir(String(directory).ConvertTo("UTF-8")) == 0);
#endif

	if (result == False) return Error();
	else		     return Success();
}
