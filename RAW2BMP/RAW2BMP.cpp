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

const int filesizes[] = {
	21504,
	23040,
	49152,
	196608,
	638976,
	921600,
	368640
};

const SIZE bmpsizes[] = {
	{ 32, 224 },
	{ 96, 80 },
	{ 128, 128 },
	{ 256, 256 },
	{ 1664, 128 },
	{ 640, 480 },
	{ 320, 384 }
};

map<int, SIZE> sizes;

template <typename T, size_t N>
inline size_t LengthOfArray( const T(&)[ N ] )
{
	return N;
}

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

int wmain(int argc, wchar_t *argv[])
{
	for (size_t i = 0; i < LengthOfArray(filesizes); i++)
		sizes[filesizes[i]] = bmpsizes[i];
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
		if (bmpinfo.biBitCount != 24 && bmpinfo.biBitCount != 32)
		{
			wcout << L"Unsupported pixel format." << endl;
			return UnsupportedPixelFormat;
		}
		if (bmpinfo.biCompression != BI_RGB)
		{
			wcout << L"Unsupported compression format." << endl;
			return UnsupportedCompressionFormat;
		}
		input.seekg(bmpinfo.biClrUsed * sizeof(RGBQUAD), ios_base::cur);
		if (argc > 2)
			fn = argv[2];
		else if (bmpinfo.biWidth == 320 && abs(bmpinfo.biHeight) == 384)
			wcscpy(ext, plyext);
		else
			wcscpy(ext, rawext);
		int rowsize = bmpinfo.biWidth * 3;
		int size = abs(bmpinfo.biHeight) * rowsize;
		char *raw = new char[size];
		int padding = 0;
		switch (bmpinfo.biBitCount)
		{
		case 24:
			padding = rowsize % 4;
			if (padding != 0)
				padding = 4 - padding;
			for (int y = 0; y < abs(bmpinfo.biHeight); y++)
			{
				char *row;
				if (bmpinfo.biHeight < 0)
					row = &raw[y * rowsize];
				else
					row = &raw[(y - bmpinfo.biHeight - 1) * rowsize];
				for (int x = 0; x < bmpinfo.biWidth; x++)
				{
					row[2] = input.get();
					row[1] = input.get();
					row[0] = input.get();
					row += 3;
				}
				if (padding != 0)
					input.seekg(padding, ios_base::cur);
			}
			break;
		case 32:
			for (int y = 0; y < abs(bmpinfo.biHeight); y++)
			{
				char *row;
				if (bmpinfo.biHeight < 0)
					row = &raw[y * rowsize];
				else
					row = &raw[(y - bmpinfo.biHeight - 1) * rowsize];
				for (int x = 0; x < bmpinfo.biWidth; x++)
				{
					row[2] = input.get();
					row[1] = input.get();
					row[0] = input.get();
					input.get();
					row += 3;
				}
			}
			break;
		}
		ofstream output(fn, ios_base::binary | ios_base::trunc);
		output.write(raw, size);
		output.close();
		return Success;
	}
	wcout << L"Unknown file type." << endl;
	return UnknownFileType;
}