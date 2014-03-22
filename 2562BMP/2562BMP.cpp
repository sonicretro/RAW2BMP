// 2562BMP.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <cstdint>
#include <iostream>
#include <fstream>
#include <Shlwapi.h>
#include <WinGDI.h>
using namespace std;

const wchar_t *const rawext = L".256"; // because 256ext is not a valid identifier
const wchar_t *const bmpext = L".bmp";

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
	if (_wcsicmp(ext, rawext) == 0)
	{
		ifstream input(fn, ios_base::binary);
		RGBQUAD pal[256];
		for (int i = 0; i < 256; i++)
		{
			pal[i].rgbReserved = 255;
			pal[i].rgbRed = input.get();
			pal[i].rgbGreen = input.get();
			pal[i].rgbBlue = input.get();
		}
		uint16_t width;
		input.read((char *)&width, sizeof(uint16_t));
		uint16_t height;
		input.read((char *)&height, sizeof(uint16_t));
		int size = width * height;
		char *raw = new char[size];
		input.read(raw, size);
		input.close();
		if (argc > 2)
			fn = argv[2];
		else
			wcscpy(ext, bmpext);
		ofstream output(fn, ios_base::binary | ios_base::trunc);
		BITMAPFILEHEADER bmpfile;
		memcpy(&bmpfile.bfType, "BM", 2);
		bmpfile.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (sizeof(RGBQUAD) * 256) + size;
		bmpfile.bfReserved1 = 0;
		bmpfile.bfReserved2 = 0;
		bmpfile.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (sizeof(RGBQUAD) * 256);
		output.write((char *)&bmpfile, sizeof(BITMAPFILEHEADER));
		BITMAPINFOHEADER bmpinfo;
		bmpinfo.biSize = sizeof(BITMAPINFOHEADER);
		bmpinfo.biWidth = width;
		bmpinfo.biHeight = -height;
		bmpinfo.biPlanes = 1;
		bmpinfo.biBitCount = 8;
		bmpinfo.biCompression = BI_RGB;
		bmpinfo.biSizeImage = size;
		bmpinfo.biXPelsPerMeter = 3780;
		bmpinfo.biYPelsPerMeter = 3780;
		bmpinfo.biClrUsed = 256;
		bmpinfo.biClrImportant = 0;
		output.write((char *)&bmpinfo, sizeof(BITMAPINFOHEADER));
		output.write((char *)pal, sizeof(RGBQUAD) * 256);
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
		if (bmpinfo.biBitCount != 8)
		{
			wcout << L"Unsupported pixel format." << endl;
			return UnsupportedPixelFormat;
		}
		if (bmpinfo.biCompression != BI_RGB)
		{
			wcout << L"Unsupported compression format." << endl;
			return UnsupportedCompressionFormat;
		}
		if (argc > 2)
			fn = argv[2];
		else
			wcscpy(ext, rawext);
		ofstream output(fn, ios_base::binary | ios_base::trunc);
		RGBQUAD *pal = new RGBQUAD[bmpinfo.biClrUsed];
		input.read((char *)pal, sizeof(RGBQUAD) * bmpinfo.biClrUsed);
		for (unsigned int i = 0; i < min(bmpinfo.biClrUsed, 256); i++)
		{
			output.put(pal[i].rgbRed);
			output.put(pal[i].rgbGreen);
			output.put(pal[i].rgbBlue);
		}
		delete[] pal;
		uint16_t tmp = (uint16_t)bmpinfo.biWidth;
		output.write((char *)&tmp, sizeof(uint16_t));
		tmp = (uint16_t)bmpinfo.biHeight;
		output.write((char *)&tmp, sizeof(uint16_t));
		int size = abs(bmpinfo.biHeight) * bmpinfo.biWidth;
		char *raw = new char[size];
		int padding = bmpinfo.biWidth % 4;
		if (padding != 0)
			padding = 4 - padding;
		for (int y = 0; y < abs(bmpinfo.biHeight); y++)
		{
			char *row;
			if (bmpinfo.biHeight < 0)
				row = &raw[y * bmpinfo.biWidth];
			else
				row = &raw[(y - bmpinfo.biHeight - 1) * bmpinfo.biWidth];
			input.read(raw, bmpinfo.biWidth);
			if (padding != 0)
				input.seekg(padding, ios_base::cur);
		}
		output.write(raw, size);
		output.close();
		return Success;
	}
	wcout << L"Unknown file type." << endl;
	return UnknownFileType;
}