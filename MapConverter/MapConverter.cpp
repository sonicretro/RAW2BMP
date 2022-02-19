// MapConverter.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "stdafx.h"
#include <vector>
#include <cstdio>

using namespace Gdiplus;
using std::vector;

char map[65536];
uint8_t ply[368640];
BYTE mappix[201326592];
CLSID encoder;
void MakeMapImage(const wchar_t* plyfile, const WCHAR* outfile)
{
	FILE* plyhandle = _wfopen(plyfile, L"rb");
	fread(ply, 960, 384, plyhandle);
	fclose(plyhandle);
	int srcind = 0;
	int dstrow = 0;
	do
	{
		int dstcol = dstrow;
		do
		{
			div_t dr = div(map[srcind++], 10);
			int plysrc = dr.quot * 30720 + dr.rem * 96;
			int plydst = dstcol;
			for (int y = 0; y < 32; y++)
			{
				for (int x = 0; x < 96; x += 3)
				{
					mappix[plydst + x + 2] = ply[plysrc + x];
					mappix[plydst + x + 1] = ply[plysrc + x + 1];
					mappix[plydst + x] = ply[plysrc + x + 2];
				}
				plysrc += 960;
				plydst += 24576;
			}
			dstcol += 96;
		} while (dstcol < dstrow + 24576);
		dstrow += 786432;
	} while (srcind < 65536);
	Bitmap mapbmp(8192, 8192, 24576, PixelFormat24bppRGB, mappix);
	mapbmp.Save(outfile, &encoder);
}

struct tile
{
	uint8_t* reg;
	uint8_t* snow;

	tile(const uint8_t* regsrc, const uint8_t* snowsrc, int regstride, int snowstride, int width, int height)
	{
		int line = width * 4;
		reg = new uint8_t[line * height];
		uint8_t* dst = reg;
		for (int y = 0; y < height; y++)
		{
			memcpy(dst, regsrc, line);
			dst += line;
			regsrc += regstride;
		}
		if (snowsrc)
		{
			snow = new uint8_t[line * height];
			dst = snow;
			for (int y = 0; y < height; y++)
			{
				memcpy(dst, snowsrc, line);
				dst += line;
				snowsrc += snowstride;
			}
		}
	}

	~tile()
	{
		if (reg != nullptr)
			delete[] reg;
		if (snow != nullptr)
			delete[] snow;
	}
};

int wmain(int argc, wchar_t* argv[])
{
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	UINT numEncoders;
	UINT encodersSize;
	GetImageEncodersSize(&numEncoders, &encodersSize);
	ImageCodecInfo *codecs = (ImageCodecInfo*)malloc(encodersSize);
	GetImageEncoders(numEncoders, encodersSize, codecs);
	for (size_t i = 0; i < numEncoders; i++)
		if (codecs[i].FormatID == ImageFormatPNG)
		{
			encoder = codecs[i].Clsid;
			break;
		}
	free(codecs);
	wchar_t* file1;
	if (argc > 1)
		file1 = argv[1];
	else
	{
		file1 = new wchar_t[MAX_PATH];
		wprintf(L"File: ");
		_getws_s(file1, MAX_PATH);
	}
	if (_wcsicmp(PathFindExtensionW(file1), L".map") == 0)
	{
		wchar_t* mapfile = file1;
		FILE* maphandle = _wfopen(mapfile, L"rb");
		fread(map, 256, 256, maphandle);
		fclose(maphandle);
		wchar_t outfile[MAX_PATH];
		wcscpy(outfile, mapfile);
		wcscpy(PathFindExtensionW(outfile), L"_map.png");
		wchar_t* plyfile;
		if (argc > 2)
			plyfile = argv[2];
		else
		{
			plyfile = _wcsdup(mapfile);
			memcpy(PathFindExtensionW(plyfile), L".ply", 4 * sizeof(wchar_t));
		}
		MakeMapImage(plyfile, outfile);
		wcscpy(PathFindExtensionW(outfile), L"_s.png");
		wchar_t plyfile2[MAX_PATH];
		wcscpy(plyfile2, plyfile);
		PathRemoveFileSpecW(plyfile2);
		PathAppendW(plyfile2, L"snow");
		PathAppendW(plyfile2, PathFindFileNameW(plyfile));
		wcscpy(PathFindExtensionW(plyfile2), L"_s");
		PathAddExtensionW(plyfile2, PathFindExtensionW(plyfile));
		if (PathFileExistsW(plyfile2))
			MakeMapImage(plyfile2, outfile);
	}
	else
	{
		Bitmap* bmp = new Bitmap(file1);
		Rect rect(0, 0, bmp->GetWidth(), bmp->GetHeight());
		BitmapData bmpd;
		bmp->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bmpd);
		Bitmap* bmp2 = nullptr;
		wchar_t file2[MAX_PATH];
		wcscpy(file2, file1);
		wcscpy(PathFindExtensionW(file2), L"_s");
		PathAddExtensionW(file2, PathFindExtensionW(file1));
		BitmapData bmpd2;
		if (PathFileExistsW(file2))
		{
			bmp2 = new Bitmap(file2);
			rect = Rect(0, 0, bmp2->GetWidth(), bmp2->GetHeight());
			bmp2->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bmpd2);
		}
		int twidth = bmp->GetWidth() / 256;
		int theight = bmp->GetHeight() / 256;
		size_t tsz = twidth * theight * 4;
		vector<tile*> tiles;
		uint8_t* regdata = static_cast<uint8_t*>(bmpd.Scan0);
		uint8_t* snowdata = nullptr;
		if (bmp2)
			snowdata = static_cast<uint8_t*>(bmpd2.Scan0);
		for (int y = 0; y < 256; ++y)
		{
			const uint8_t* regline = regdata;
			const uint8_t* snowline = snowdata;
			for (int x = 0; x < 256; ++x)
			{
				tile* t = new tile(regline, snowline, bmpd.Stride, bmpd2.Stride, twidth, theight);
				char ti = -1;
				for (size_t i = 0; i < tiles.size(); ++i)
				{
					if (bmp2 != nullptr && memcmp(t->snow, tiles[i]->snow, tsz))
						continue;
					if (!memcmp(t->reg, tiles[i]->reg, tsz))
					{
						ti = static_cast<char>(i);
						break;
					}
				}
				if (ti == -1)
				{
					map[y * 256 + x] = static_cast<char>(tiles.size());
					tiles.push_back(t);
				}
				else
				{
					map[y * 256 + x] = ti;
					delete t;
				}
				regline += twidth * 4;
				if (snowline)
					snowline += twidth * 4;
			}
			regdata += bmpd.Stride * theight;
			if (snowdata)
				snowdata += bmpd2.Stride * theight;
		}
		if (tiles.size() > 120)
			printf("Too many unique tiles in source image(s)! Must be lower than 120 (was %d).\n", tiles.size());
		wcscpy(file2, file1);
		wchar_t* fnend = wcsstr(file2, L"_map");
		if (fnend == nullptr)
			fnend = PathFindExtensionW(file2);
		wcscpy(fnend, L".map");
		FILE* fh = _wfopen(file2, L"wb");
		fwrite(map, 256, 256, fh);
		fclose(fh);
		bmp->UnlockBits(&bmpd);
		delete bmp;
		rect.Width = twidth * 10;
		rect.Height = theight * 10;
		bmp = new Bitmap(rect.Width, rect.Height, PixelFormat32bppARGB);
		bmp->LockBits(&rect, ImageLockModeWrite, PixelFormat32bppARGB, &bmpd);
		regdata = static_cast<uint8_t*>(bmpd.Scan0);
		if (bmp2)
		{
			bmp2->UnlockBits(&bmpd2);
			delete bmp2;
			bmp2 = new Bitmap(rect.Width, rect.Height, PixelFormat32bppARGB);
			bmp2->LockBits(&rect, ImageLockModeWrite, PixelFormat32bppARGB, &bmpd2);
			snowdata = static_cast<uint8_t*>(bmpd2.Scan0);
		}
		int tnum = 0;
		for (int row = 0; row < 12; ++row)
		{
			uint8_t* regline = regdata;
			uint8_t* snowline = snowdata;
			for (int col = 0; col < 10; ++col)
			{
				uint8_t* regsrc = tiles[tnum]->reg;
				uint8_t* snowsrc = tiles[tnum]->snow;
				uint8_t* regtile = regline;
				uint8_t* snowtile = snowline;
				for (int y = 0; y < theight; ++y)
				{
					memcpy(regtile, regsrc, twidth * 4);
					regsrc += twidth * 4;
					regtile += bmpd.Stride;
					if (snowtile)
					{
						memcpy(snowtile, snowsrc, twidth * 4);
						snowsrc += twidth * 4;
						snowtile += bmpd2.Stride;
					}
				}
				delete tiles[tnum];
				if (++tnum == tiles.size())
					break;
				regline += twidth * 4;
				if (snowline)
					snowline += twidth * 4;
			}
			if (tnum == tiles.size())
				break;
			regdata += bmpd.Stride * theight;
			if (snowdata)
				snowdata += bmpd2.Stride * theight;
		}
		bmp->UnlockBits(&bmpd);
		if (bmp2)
			bmp2->UnlockBits(&bmpd2);
		wcscpy(fnend, L".png");
		bmp->Save(file2, &encoder);
		Bitmap* plybmp = new Bitmap(320, 384, PixelFormat32bppARGB);
		Rect plyrect{ 0, 0, 320, 384 };
		Graphics *gfx = new Graphics(plybmp);
		gfx->SetCompositingMode(CompositingModeSourceCopy);
		if (bmp->GetWidth() == 320 && bmp->GetHeight() == 384)
			gfx->SetInterpolationMode(InterpolationModeNearestNeighbor);
		else
			gfx->SetInterpolationMode(InterpolationModeHighQualityBicubic);
		gfx->DrawImage(bmp, plyrect);
		delete gfx;
		delete bmp;
		plybmp->LockBits(&plyrect, ImageLockModeRead, PixelFormat32bppARGB, &bmpd);
		const uint8_t* srcdata = static_cast<const uint8_t*>(bmpd.Scan0);
		uint8_t* dst = ply;
		for (size_t y = 0; y < bmpd.Height; ++y)
		{
			const uint8_t* srcline = srcdata;
			for (size_t x = 0; x < bmpd.Width; ++x)
			{
				if (srcline[3] <= 128)
				{
					*dst++ = 0;
					*dst++ = 0xF8;
					*dst++ = 0;
				}
				else
				{
					*dst++ = srcline[2];
					*dst++ = srcline[1];
					*dst++ = srcline[0];
				}
				srcline += 4;
			}
			srcdata += bmpd.Stride;
		}
		plybmp->UnlockBits(&bmpd);
		delete plybmp;
		wcscpy(fnend, L".ply");
		fh = _wfopen(file2, L"wb");
		fwrite(ply, 960, 384, fh);
		fclose(fh);
		if (bmp2 != nullptr)
		{
			wcscpy(fnend, L"_s.png");
			bmp2->Save(file2, &encoder);
			plybmp = new Bitmap(320, 384, PixelFormat32bppARGB);
			gfx = new Graphics(plybmp);
			gfx->SetCompositingMode(CompositingModeSourceCopy);
			if (bmp2->GetWidth() == 320 && bmp2->GetHeight() == 384)
				gfx->SetInterpolationMode(InterpolationModeNearestNeighbor);
			else
				gfx->SetInterpolationMode(InterpolationModeHighQualityBicubic);
			gfx->DrawImage(bmp2, plyrect);
			delete gfx;
			delete bmp2;
			plybmp->LockBits(&plyrect, ImageLockModeRead, PixelFormat32bppARGB, &bmpd);
			srcdata = static_cast<const uint8_t*>(bmpd.Scan0);
			dst = ply;
			for (size_t y = 0; y < bmpd.Height; ++y)
			{
				const uint8_t* srcline = srcdata;
				for (size_t x = 0; x < bmpd.Width; ++x)
				{
					if (srcline[3] <= 128)
					{
						*dst++ = 0;
						*dst++ = 0xF8;
						*dst++ = 0;
					}
					else
					{
						*dst++ = srcline[2];
						*dst++ = srcline[1];
						*dst++ = srcline[0];
					}
					srcline += 4;
				}
				srcdata += bmpd.Stride;
			}
			plybmp->UnlockBits(&bmpd);
			delete plybmp;
			wcscpy(fnend, L"_s.ply");
			fh = _wfopen(file2, L"wb");
			fwrite(ply, 960, 384, fh);
			fclose(fh);
		}
	}
	GdiplusShutdown(gdiplusToken);
}