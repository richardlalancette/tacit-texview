// Settings.cpp
//
// Viewer settings stored as human-readable symbolic expressions.
//
// Copyright (c) 2019, 2020 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <System/tFile.h>
#include <System/tScript.h>
#include <Math/tFundamentals.h>
#include "Settings.h"
#include "TacitImage.h"
using namespace tMath;
#define ReadItem(name) case tHashCT(#name): name = e.Arg1(); break
#define	WriteItem(name) writer.Comp(#name, name)


void Settings::Reset()
{
	WindowW					= 1280;
	WindowH					= 720;
	WindowX					= 100;
	WindowY					= 100;

	ShowLog					= false;
	InfoOverlayShow			= false;
	ContentViewShow			= false;
	ThumbnailWidth			= 128.0f;
	SortKey					= 0;
	SortAscending			= true;
	OverlayCorner			= 3;
	Tile					= false;
	BackgroundStyle			= 1;
	BackgroundExtend		= false;
	ResampleFilter			= 2;
	ConfirmDeletes			= true;
	ConfirmFileOverwrites	= true;

	SlidehowFrameDuration	= 1.0/30.0;
	FileSaveType			= 0;
	FileSaveTargaRLE		= false;
	SaveAllSizeMode			= 0;
	MaxImageMemMB			= 1024;
	MaxCacheFiles			= 7000;
}


void Settings::Reset(int screenW, int screenH)
{
	Reset();
	WindowX					= (screenW - WindowW) >> 1;
	WindowY					= (screenH - WindowH) >> 1;
}


void Settings::Load(const tString& filename, int screenW, int screenH)
{
	if (!tSystem::tFileExists(filename))
	{
		Reset(screenW, screenH);
	}
	else
	{
		tScriptReader reader(filename);
		for (tExpr e = reader.First(); e.IsValid(); e = e.Next())
		{
			switch (e.Command().Hash())
			{
				ReadItem(WindowX);
				ReadItem(WindowY);
				ReadItem(WindowW);
				ReadItem(WindowH);
				ReadItem(ShowLog);
				ReadItem(InfoOverlayShow);
				ReadItem(ContentViewShow);
				ReadItem(ThumbnailWidth);
				ReadItem(SortKey);
				ReadItem(SortAscending);
				ReadItem(OverlayCorner);
				ReadItem(Tile);
				ReadItem(BackgroundStyle);
				ReadItem(BackgroundExtend);
				ReadItem(ResampleFilter);
				ReadItem(ConfirmDeletes);
				ReadItem(ConfirmFileOverwrites);
				ReadItem(SlidehowFrameDuration);
				ReadItem(FileSaveType);
				ReadItem(FileSaveTargaRLE);
				ReadItem(SaveAllSizeMode);
				ReadItem(MaxImageMemMB);
				ReadItem(MaxCacheFiles);
			}
		}
	}

	tiClamp(ResampleFilter, 0, 5);
	tiClamp(BackgroundStyle, 0, 4);
	tiClamp(WindowW, 640, screenW);
	tiClamp(WindowH, 360, screenH);
	tiClamp(WindowX, 0, screenW - WindowW);
	tiClamp(WindowY, 0, screenH - WindowH);
	tiClamp(OverlayCorner, 0, 3);
	tiClamp(FileSaveType, 0, 4);
	tiClamp(ThumbnailWidth, float(TacitImage::ThumbMinDispWidth), float(TacitImage::ThumbWidth));
	tiClamp(SortKey, 0, 3);
	tiClampMin(MaxImageMemMB, 256);
	tiClampMin(MaxCacheFiles, 200);
	tiClamp(SaveAllSizeMode, 0, 3);
}


bool Settings::Save(const tString& filename)
{
	tScriptWriter writer(filename);
	writer.Rem("Tacit Texture Viewer Configuration File");
	writer.CR();

	WriteItem(WindowX);
	WriteItem(WindowY);
	WriteItem(WindowW);
	WriteItem(WindowH);
	WriteItem(ShowLog);
	WriteItem(InfoOverlayShow);
	WriteItem(ContentViewShow);
	WriteItem(ThumbnailWidth);
	WriteItem(SortKey);
	WriteItem(SortAscending);
	WriteItem(OverlayCorner);
	WriteItem(Tile);
	WriteItem(BackgroundExtend);
	WriteItem(BackgroundStyle);
	WriteItem(ResampleFilter);
	WriteItem(ConfirmDeletes);
	WriteItem(ConfirmFileOverwrites);
	WriteItem(SlidehowFrameDuration);
	WriteItem(FileSaveType);
	WriteItem(FileSaveTargaRLE);
	WriteItem(SaveAllSizeMode);
	WriteItem(MaxImageMemMB);
	WriteItem(MaxCacheFiles);
	
	return true;
}
