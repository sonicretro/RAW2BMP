// RAW2BMP.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <cstdint>
#include <iostream>
#include <fstream>
#include <map>
#include <Shlwapi.h>
#include <WinGDI.h>
using namespace std;

const wchar_t *const rawext = L".raw";
const wchar_t *const plyext = L".ply";
const wchar_t *const bmpext = L".bmp";

map<int, SIZE> sizes = {
	{ 21504, { 32, 224 } },
	{ 23040, { 96, 80 } },
	{ 49152, { 128, 128 } },
	{ 196608, { 256, 256 } },
	{ 638976, { 1664, 128 } },
	{ 921600, { 640, 480 } },
	{ 368640, { 320, 384 } }
};

enum errcode
{
	Success,
	FileNotFound,
	UnknownFileType,
	UnknownFileSize,
	InvalidBitmapFile,
	UnsupportedBitmapVersion,
	UnsupportedPixelFormat,
	UnsupportedCompressionFormat
};

struct rgb { uint8_t red, green, blue; };

int wmain(int argc, wchar_t *argv[])
{
	wchar_t *fn;
	if (argc > 1)
		fn = argv[1];
	else
	{
		wcout << L"File: ";
		fn = new wchar_t[MAX_PATH];
		wcin.getline(fn, MAX_PATH);
	}
	if (!PathFileExists(fn))
	{
		wcout << L"File not found." << endl;
		return FileNotFound;
	}
	wchar_t *ext = PathFindExtension(fn);
	if (_wcsicmp(ext, rawext) == 0 || _wcsicmp(ext, plyext) == 0)
	{
		ifstream input(fn, ios_base::binary | ios_base::ate);
		int size = (int)input.tellg();
		auto iter = sizes.find(size);
		if (iter == sizes.end())
		{
			wcout << L"Unknown file size." << endl;
			return UnknownFileSize;
		}
		int width = iter->second.cx;
		int height = iter->second.cy;
		char *raw = new char[size];
		input.seekg(0, ios_base::beg);
		input.read(raw, size);
		input.close();
		for (int i = 0; i < size; i += 3)
			swap(raw[i], raw[i + 2]);
		if (argc > 2)
			fn = argv[2];
		else
			wcscpy(ext, bmpext);
		ofstream output(fn, ios_base::binary | ios_base::trunc);
		BITMAPFILEHEADER bmpfile;
		memcpy(&bmpfile.bfType, "BM", 2);
		bmpfile.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + size;
		bmpfile.bfReserved1 = 0;
		bmpfile.bfReserved2 = 0;
		bmpfile.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		output.write((char *)&bmpfile, sizeof(BITMAPFILEHEADER));
		BITMAPINFOHEADER bmpinfo;
		bmpinfo.biSize = sizeof(BITMAPINFOHEADER);
		bmpinfo.biWidth = width;
		bmpinfo.biHeight = -height;
		bmpinfo.biPlanes = 1;
		bmpinfo.biBitCount = 24;
		bmpinfo.biCompression = BI_RGB;
		bmpinfo.biSizeImage = size;
		bmpinfo.biXPelsPerMeter = 3780;
		bmpinfo.biYPelsPerMeter = 3780;
		bmpinfo.biClrUsed = 0;
		bmpinfo.biClrImportant = 0;
		output.write((char *)&bmpinfo, sizeof(BITMAPINFOHEADER));
		output.write(raw, size);
		output.close();
		return Success;
	} 
	else if (_wcsicmp(ext, bmpext) == 0)
	{
		ifstream input(fn, ios_base::binary);
		char magic[3];
		input.read(magic, 2);
		magic[2] = 0;
		if (strcmp(magic, "BM") != 0)
		{
			wcout << L"Invalid bitmap file." << endl;
			return InvalidBitmapFile;
		}
		input.seekg(sizeof(BITMAPFILEHEADER), ios_base::beg);
		BITMAPINFOHEADER bmpinfo;
		input.read((char *)&bmpinfo, sizeof(BITMAPINFOHEADER));
		if (bmpinfo.biSize != sizeof(BITMAPINFOHEADER))
		{
			wcout << L"Unsupported bitmap version." << endl;
			return UnsupportedBitmapVersion;
		}
		if (bmpinfo.biCompression != BI_RGB)
		{
			wcout << L"Unsupported compression format." << endl;
			return UnsupportedCompressionFormat;
		}
		int rowsize = bmpinfo.biWidth * (int)ceil(bmpinfo.biBitCount / 8.0);
		int size = abs(bmpinfo.biHeight) * bmpinfo.biWidth;
		rgb *raw = new rgb[size];
		int padding = rowsize % 4;
		if (padding != 0)
			padding = 4 - padding;
		RGBQUAD *palette = nullptr;
		switch (bmpinfo.biBitCount)
		{
		case 1:
			palette = new RGBQUAD[bmpinfo.biClrUsed];
			input.read((char *)palette, bmpinfo.biClrUsed * sizeof(RGBQUAD));
			for (int y = 0; y < abs(bmpinfo.biHeight); y++)
			{
				rgb *row;
				if (bmpinfo.biHeight < 0)
					row = &raw[y * bmpinfo.biWidth];
				else
					row = &raw[(bmpinfo.biHeight - y - 1) * bmpinfo.biWidth];
				for (int x = 0; x < bmpinfo.biWidth; x += 8)
				{
					uint8_t byte = input.get();
					RGBQUAD pix = palette[byte >> 7];
					row->red = pix.rgbRed;
					row->green = pix.rgbGreen;
					row->blue = pix.rgbBlue;
					row++;
					if (x + 1 == bmpinfo.biWidth)
						continue;
					pix = palette[(byte >> 6) & 1];
					row->red = pix.rgbRed;
					row->green = pix.rgbGreen;
					row->blue = pix.rgbBlue;
					row++;
					if (x + 2 == bmpinfo.biWidth)
						continue;
					pix = palette[(byte >> 5) & 1];
					row->red = pix.rgbRed;
					row->green = pix.rgbGreen;
					row->blue = pix.rgbBlue;
					row++;
					if (x + 3 == bmpinfo.biWidth)
						continue;
					pix = palette[(byte >> 4) & 1];
					row->red = pix.rgbRed;
					row->green = pix.rgbGreen;
					row->blue = pix.rgbBlue;
					row++;
					if (x + 4 == bmpinfo.biWidth)
						continue;
					pix = palette[(byte >> 3) & 1];
					row->red = pix.rgbRed;
					row->green = pix.rgbGreen;
					row->blue = pix.rgbBlue;
					row++;
					if (x + 5 == bmpinfo.biWidth)
						continue;
					pix = palette[(byte >> 2) & 1];
					row->red = pix.rgbRed;
					row->green = pix.rgbGreen;
					row->blue = pix.rgbBlue;
					row++;
					if (x + 6 == bmpinfo.biWidth)
						continue;
					pix = palette[(byte >> 1) & 1];
					row->red = pix.rgbRed;
					row->green = pix.rgbGreen;
					row->blue = pix.rgbBlue;
					row++;
					if (x + 7 == bmpinfo.biWidth)
						continue;
					pix = palette[byte & 1];
					row->red = pix.rgbRed;
					row->green = pix.rgbGreen;
					row->blue = pix.rgbBlue;
					row++;
				}
				if (padding != 0)
					input.seekg(padding, ios_base::cur);
			}
			break;
		case 4:
			palette = new RGBQUAD[bmpinfo.biClrUsed];
			input.read((char *)palette, bmpinfo.biClrUsed * sizeof(RGBQUAD));
			for (int y = 0; y < abs(bmpinfo.biHeight); y++)
			{
				rgb *row;
				if (bmpinfo.biHeight < 0)
					row = &raw[y * bmpinfo.biWidth];
				else
					row = &raw[(bmpinfo.biHeight - y - 1) * bmpinfo.biWidth];
				for (int x = 0; x < bmpinfo.biWidth; x += 2)
				{
					uint8_t byte = input.get();
					RGBQUAD pix = palette[byte >> 4];
					row->red = pix.rgbRed;
					row->green = pix.rgbGreen;
					row->blue = pix.rgbBlue;
					row++;
					if (x + 1 == bmpinfo.biWidth)
						continue;
					pix = palette[byte & 0xF];
					row->red = pix.rgbRed;
					row->green = pix.rgbGreen;
					row->blue = pix.rgbBlue;
					row++;
				}
				if (padding != 0)
					input.seekg(padding, ios_base::cur);
			}
			break;
		case 8:
			palette = new RGBQUAD[bmpinfo.biClrUsed];
			input.read((char *)palette, bmpinfo.biClrUsed * sizeof(RGBQUAD));
			for (int y = 0; y < abs(bmpinfo.biHeight); y++)
			{
				rgb *row;
				if (bmpinfo.biHeight < 0)
					row = &raw[y * bmpinfo.biWidth];
				else
					row = &raw[(bmpinfo.biHeight - y - 1) * bmpinfo.biWidth];
				for (int x = 0; x < bmpinfo.biWidth; x++)
				{
					RGBQUAD pix = palette[input.get()];
					row->red = pix.rgbRed;
					row->green = pix.rgbGreen;
					row->blue = pix.rgbBlue;
					row++;
				}
				if (padding != 0)
					input.seekg(padding, ios_base::cur);
			}
			break;
		case 16:
			input.seekg(bmpinfo.biClrUsed * sizeof(RGBQUAD), ios_base::cur);
			for (int y = 0; y < abs(bmpinfo.biHeight); y++)
			{
				rgb *row;
				if (bmpinfo.biHeight < 0)
					row = &raw[y * bmpinfo.biWidth];
				else
					row = &raw[(bmpinfo.biHeight - y - 1) * bmpinfo.biWidth];
				for (int x = 0; x < bmpinfo.biWidth; x++)
				{
					uint16_t pix;
					input.read((char *)&pix, sizeof(uint16_t));
					row->red = (pix >> 10) & 0x1F;
					row->red = (row->red << 3) | (row->red >> 2);
					row->green = (pix >> 5) & 0x1F;
					row->green = (row->green << 3) | (row->green >> 2);
					row->blue = pix & 0x1F;
					row->blue = (row->blue << 3) | (row->blue >> 2);
					row++;
				}
				if (padding != 0)
					input.seekg(padding, ios_base::cur);
			}
			break;
		case 24:
			input.seekg(bmpinfo.biClrUsed * sizeof(RGBQUAD), ios_base::cur);
			for (int y = 0; y < abs(bmpinfo.biHeight); y++)
			{
				rgb *row;
				if (bmpinfo.biHeight < 0)
					row = &raw[y * bmpinfo.biWidth];
				else
					row = &raw[(bmpinfo.biHeight - y - 1) * bmpinfo.biWidth];
				for (int x = 0; x < bmpinfo.biWidth; x++)
				{
					row->blue = input.get();
					row->green = input.get();
					row->red = input.get();
					row++;
				}
				if (padding != 0)
					input.seekg(padding, ios_base::cur);
			}
			break;
		case 32:
			input.seekg(bmpinfo.biClrUsed * sizeof(RGBQUAD), ios_base::cur);
			for (int y = 0; y < abs(bmpinfo.biHeight); y++)
			{
				rgb *row;
				if (bmpinfo.biHeight < 0)
					row = &raw[y * bmpinfo.biWidth];
				else
					row = &raw[(bmpinfo.biHeight - y - 1) * bmpinfo.biWidth];
				for (int x = 0; x < bmpinfo.biWidth; x++)
				{
					uint32_t pix;
					input.read((char *)&pix, sizeof(uint32_t));
					row->red = (pix >> 16) & 0xFF;
					row->green = (pix >> 8) & 0xFF;
					row->blue = pix & 0xFF;
					row++;
				}
			}
			break;
		default:
			wcout << L"Unsupported pixel format." << endl;
			return UnsupportedPixelFormat;
		}
		if (argc > 2)
			fn = argv[2];
		else if (bmpinfo.biWidth == 320 && abs(bmpinfo.biHeight) == 384)
			wcscpy(ext, plyext);
		else
			wcscpy(ext, rawext);
		ofstream output(fn, ios_base::binary | ios_base::trunc);
		output.write((char *)raw, size * sizeof(rgb));
		output.close();
		return Success;
	}
	wcout << L"Unknown file type." << endl;
	return UnknownFileType;
}