#include <windows.h>

#include "win64_diff.h"

global u32 PAGE;

u8 *
find_end
(u8 *start)
{
	u8 *result = start;
	while((*result != ' ') || (*result != 0))
	{
		result++;
	}
	
	return(result);
}

String
copy_string
(Buffer *dest, String str)
{
	String result = {};
	result.chars = dest->end;
	result.len = str.len;
	
	for(u32 i = 0; i < result.len; i++)
	{
		result.chars[i] = str.chars[i];
	}
	
	return(result);
}

int __stdcall
WinMainCRTStartup
(void)
{
	
	SYSTEM_INFO SysInfo = {};
	GetSystemInfo(&SysInfo);
	Assert(SysInfo.dwPageSize != 0);
	PAGE = SysInfo.dwPageSize;
	
	Buffer buffer_strings = win64_make_buffer(PAGE, PAGE_READWRITE);
	
	u8 *commandLine = (u8 *)GetCommandLineA();
	
	u8 *end = find_end(commandLine);
	String toCopy = {};
	toCopy.chars = commandLine;
	toCopy.len = (u32)(end - commandLine);
	
	String fileOneName = copy_string(&buffer_strings, toCopy);
	buffer_append_u8(&buffer_strings, 0x00);
	fileOneName.len++;
	
	toCopy = {};
	toCopy.chars = end + 1;
	end = find_end(toCopy.chars);
	toCopy.len = (u32)(end - toCopy.chars);
	
	String fileTwoName = copy_string(&buffer_strings, toCopy);
	buffer_append_u8(&buffer_strings, 0x00);
	fileTwoName.len++;
	
	read_file_result fileOne = Win64ReadEntireFile((char *)fileOneName.chars);
	(void)fileOne;
	read_file_result fileTwo = Win64ReadEntireFile((char *)fileTwoName.chars);
	(void)fileTwo;
	
	return(0);
}