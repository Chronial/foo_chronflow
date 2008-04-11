#include "externHeaders.h"
#include "chronflow.h"
#include "Console.h"

#include <stdio.h>

HANDLE Console::screenBuffer = 0;

void Console::create(void)
{
	AllocConsole();
	screenBuffer = GetStdHandle(STD_OUTPUT_HANDLE);
}

void Console::print(const wchar_t* str)
{
	WriteConsole(screenBuffer, str, wcslen(str), NULL, NULL);
}

void Console::println(const wchar_t* str)
{
	print(str);
	print(L"\n");
}
void Console::printf(const wchar_t* format, ...)
{
	//swprintf_s(path,512, L"\"M:/Alben/%s\"", FindFileData.cFileName);
	va_list args;
	va_start(args, format);
	wchar_t out[1024];
	int len = vswprintf_s(out, 1024, format, args);
	WriteConsole(screenBuffer, out, len, NULL, NULL);
}
