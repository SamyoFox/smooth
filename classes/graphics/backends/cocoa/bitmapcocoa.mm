 /* The smooth Class Library
  * Copyright (C) 1998-2020 Robert Kausch <robert.kausch@gmx.net>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of "The Artistic License, Version 2.0".
  *
  * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
  * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
  * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

#include <smooth/graphics/backends/cocoa/bitmapcocoa.h>
#include <smooth/graphics/bitmap.h>

S::GUI::BitmapBackend *CreateBitmapCocoa_pV(S::Void *iBitmap)
{
	return new S::GUI::BitmapCocoa(iBitmap);
}

S::GUI::BitmapBackend *CreateBitmapCocoa_crSI(const S::GUI::Size &iSize, S::Int iDepth)
{
	return new S::GUI::BitmapCocoa(iSize, iDepth);
}

S::GUI::BitmapBackend *CreateBitmapCocoa_cI(const int null)
{
	return new S::GUI::BitmapCocoa(null);
}

S::GUI::BitmapBackend *CreateBitmapCocoa_crB(const S::GUI::BitmapBackend &iBitmap)
{
	return new S::GUI::BitmapCocoa((const S::GUI::BitmapCocoa &) iBitmap);
}

S::Int	 bitmapCocoaTmp_pV	= S::GUI::BitmapBackend::SetBackend(&CreateBitmapCocoa_pV);
S::Int	 bitmapCocoaTmp_crSI	= S::GUI::BitmapBackend::SetBackend(&CreateBitmapCocoa_crSI);
S::Int	 bitmapCocoaTmp_cI	= S::GUI::BitmapBackend::SetBackend(&CreateBitmapCocoa_cI);
S::Int	 bitmapCocoaTmp_crB	= S::GUI::BitmapBackend::SetBackend(&CreateBitmapCocoa_crB);

S::GUI::BitmapCocoa::BitmapCocoa(Void *iBitmap)
{
	Initialize();

	SetSystemBitmap(iBitmap);
}

S::GUI::BitmapCocoa::BitmapCocoa(const Size &iSize, Int iDepth)
{
	Initialize();

	CreateBitmap(iSize, iDepth);
}

S::GUI::BitmapCocoa::BitmapCocoa(const int null)
{
	Initialize();

	SetSystemBitmap(NIL);
}

S::GUI::BitmapCocoa::BitmapCocoa(const BitmapCocoa &iBitmap)
{
	Initialize();

	SetSystemBitmap((Void *) iBitmap.bitmap);

	depth = iBitmap.depth;
}

S::GUI::BitmapCocoa::~BitmapCocoa()
{
	DeleteBitmap();
}

S::Void S::GUI::BitmapCocoa::Initialize()
{
	type	= BITMAP_COCOA;
	bitmap	= nil;
	image   = nil;
}

S::Bool S::GUI::BitmapCocoa::CreateBitmap(const Size &nSize, Int nDepth)
{
	DeleteBitmap();

	if (nDepth == -1)		  nDepth = [[NSScreen deepestScreen] depth];
	if (nDepth != 24 && nDepth != 32) nDepth = 24;

	bitmap	= [[NSBitmapImageRep alloc] initWithBitmapDataPlanes: nil
							  pixelsWide: nSize.cx
							  pixelsHigh: nSize.cy
						       bitsPerSample: 8
						     samplesPerPixel: nDepth == 32 ? 4 : 3
							    hasAlpha: nDepth == 32
							    isPlanar: NO
						      colorSpaceName: NSCalibratedRGBColorSpace
							bitmapFormat: 0
							 bytesPerRow: 4 * nSize.cx
							bitsPerPixel: 32];

	if (bitmap == nil) return False;

	bytes	= [bitmap bitmapData];

	size	= nSize;
	depth	= nDepth;
	bpp	= 32;
	align	= 4;

	return True;
}

S::Bool S::GUI::BitmapCocoa::DeleteBitmap()
{
	if (bitmap != nil)
	{
		if (image != nil)
		{
			[image release];

			image = nil;
		}

		[bitmap release];

		bitmap	= nil;

		size	= Size(0, 0);
		depth	= 0;

		bytes	= NIL;
		bpp	= 0;
		align	= 0;
	}

	return True;
}

S::GUI::Bitmap S::GUI::BitmapCocoa::Scale(const Size &newSize) const
{
	Bitmap			 result(newSize, depth);
	NSImage			*image	 = (NSImage *) result.GetSystemBitmap();

	NSAutoreleasePool	*pool	 = [[NSAutoreleasePool alloc] init];
	NSGraphicsContext	*context = [NSGraphicsContext graphicsContextWithBitmapImageRep: (NSBitmapImageRep *) [[image representations] objectAtIndex: 0]];

	[NSGraphicsContext saveGraphicsState];
	[NSGraphicsContext setCurrentContext: context];

	[bitmap drawInRect: NSMakeRect(0, 0, newSize.cx, newSize.cy)];
	[image recache];

	[NSGraphicsContext restoreGraphicsState];

	[pool release];

	return result;
}

S::Bool S::GUI::BitmapCocoa::SetSystemBitmap(Void *nBitmap)
{
	if (nBitmap == GetSystemBitmap()) return True;

	if (nBitmap == NIL)
	{
		DeleteBitmap();
	}
	else
	{
		CreateBitmap(Size([(NSBitmapImageRep *) nBitmap pixelsWide], [(NSBitmapImageRep *) nBitmap pixelsHigh]), 8 * [(NSBitmapImageRep *) nBitmap samplesPerPixel]);

		/* Copy source bitmap to destination.
		 */
		NSAutoreleasePool	*pool	 = [[NSAutoreleasePool alloc] init];
		NSGraphicsContext	*context = [NSGraphicsContext graphicsContextWithBitmapImageRep: bitmap];

		[NSGraphicsContext saveGraphicsState];
		[NSGraphicsContext setCurrentContext: context];

		[(NSBitmapImageRep *) nBitmap drawInRect: NSMakeRect(0, 0, size.cx, size.cy)];

		[NSGraphicsContext restoreGraphicsState];

		[pool release];
	}

	return True;
}

S::Void *S::GUI::BitmapCocoa::GetSystemBitmap() const
{
	if (image == nil && bitmap != nil)
	{
		image = [[NSImage alloc] initWithSize: NSMakeSize(size.cx, size.cy)];

		[image addRepresentation: bitmap];
		[image setFlipped: YES];
	}

	return (Void *) image;
}

S::Bool S::GUI::BitmapCocoa::SetPixel(const Point &point, const Color &iColor)
{
	if (bytes == NIL)			      return False;
	if (point.y >= size.cy || point.x >= size.cx) return False;

	Color		 color	= iColor.ConvertTo(Color::RGBA);

	UnsignedByte	*data	= ((UnsignedByte *) bytes);
	Int		 offset = point.y * (size.cx * 4) + point.x * 4;

	switch (depth)
	{
		case 24:
			data[offset + 2] = (color >> 16) & 255;
			data[offset + 1] = (color >>  8) & 255;
			data[offset + 0] =  color	 & 255;

			return True;
		case 32:
			data[offset + 2] = (color >> 16) & 255;
			data[offset + 1] = (color >>  8) & 255;
			data[offset + 0] =  color	 & 255;
			data[offset + 3] = (color >> 24) & 255;

			return True;
	}

	return False;
}

S::GUI::Color S::GUI::BitmapCocoa::GetPixel(const Point &point) const
{
	if (bytes == NIL)			      return 0;
	if (point.y >= size.cy || point.x >= size.cx) return 0;

	UnsignedByte	*data	= ((UnsignedByte *) bytes);
	Int		 offset = point.y * (size.cx * 4) + point.x * 4;

	switch (depth)
	{
		case 24:
			return Color(			      data[offset + 2] << 16 | data[offset + 1] << 8 | data[offset + 0], Color::RGB);
		case 32:
			return Color(data[offset + 3] << 24 | data[offset + 2] << 16 | data[offset + 1] << 8 | data[offset + 0], Color::RGBA);
	}

	return 0;
}

S::GUI::BitmapBackend &S::GUI::BitmapCocoa::operator =(const BitmapBackend &newBitmap)
{
	if (&newBitmap == this) return *this;

	SetSystemBitmap((Void *) ((BitmapCocoa &) newBitmap).bitmap);

	return *this;
}

S::Bool S::GUI::BitmapCocoa::operator ==(const int null) const
{
	if (bitmap == NIL)	return True;
	else			return False;
}

S::Bool S::GUI::BitmapCocoa::operator !=(const int null) const
{
	if (bitmap == NIL)	return False;
	else			return True;
}
