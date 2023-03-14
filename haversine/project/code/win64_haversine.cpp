#include <windows.h>

#include "win64_haversine.h"

global u32 PAGE;
global u64 SEED;

u64
RandomU64
()
{
	u64 newbit = (SEED ^ (SEED >> 3) ^ (SEED >> 5) ^ (SEED >> 8) ^ (SEED >> 13)) & 1;
	SEED = (SEED >> 1) | (newbit << 63);
	
	return(SEED);
}

u32
AlignSize
(u32 size, u32 align)
{
	u32 result = 0;
	
	u32 mod = size % align;
	if(mod != 0)
	{
		result = size + (align - mod);
	}
	else
	{
		result = size;
	}
	
	return(result);
}

r32
RangedRandomR32
(r32 min, r32 max)
{
	
	r32 randomR32 = 0.0;
	u32 rand = 0;
	for(u8 i = 0; i < 10; i++)
	{
		
		rand = (u32)((RandomU64() & (u64)0xf000000000000000) >> 60);
		randomR32 /= 10.0;
		randomR32 += ((r32)rand / (r32)16.0);
		
		if(randomR32 >= 1.0)
		{
			randomR32 -= 1.0;
		}
		
	}
	
	r32 result = ((randomR32 * (max - min)) + min);
	Assert(result <= max);
	return(result);
}

Buffer
Generate40Mil
()
{
	
	Buffer result = win64_make_buffer(160002048, PAGE_READWRITE);
	Assert(result.memory != 0);
	
	u32 count = 0;
	while(count < 40000000)
	{
		
		buffer_append_r32(&result, RangedRandomR32(-180.0, 180.0));
		count++;
		
		buffer_append_r32(&result, RangedRandomR32(-90.0, 90.0));
		count++;
		
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
	
	SEED = (u64)0x0123456789ABCDEF;
	
#if 0
	Buffer coordinates = Generate40Mil();
	
	b32 coordWriteSucceeded = Win64WriteEntireFile("d:\\programming\\github\\haversine-thing\\haversine\\build\\fortymil.array", coordinates.size, coordinates.memory);
	Assert(coordWriteSucceeded == TRUE);
#endif
	
	read_file_result pairs = Win64ReadEntireFile("d:\\programming\\github\\haversine-thing\\haversine\\build\\fortymil.array");
	Assert(pairs.Contents != 0);
	
	Buffer jsonFile = win64_make_buffer((183106 * PAGE), PAGE_READWRITE);
	String temp = create_string(&jsonFile, "{\"pairs\":[");
	
	r32 current = 0.0;
	r32 *num = (r32 *)pairs.Contents;
	u32 count = 0;
	while(count < 40000000)
	{
		
		u32 whichNum = count % 4;
		if(whichNum == 0)
		{
			temp = create_string(&jsonFile, "\n\t{\"x0\":");
			
		}
		else if(whichNum == 1)
		{
			temp = create_string(&jsonFile, ", \"y0\":");
		}
		else if(whichNum == 2)
		{
			temp = create_string(&jsonFile, ", \"x1\":");
			
		}
		else if(whichNum == 3)
		{
			temp = create_string(&jsonFile, ", \"y1\":");
		}
		
		u8 chars[12] = {};
		u8 c = 0;
		current = *num;
		
		if(current < 0.0)
		{
			chars[c++] = '-';
			current *= -1.0;
		}
		
		s32 nonDecimal = (s32)current;
		if(nonDecimal == 0)
		{
			chars[c++] = '0';
		}
		else
		{
			current -= (r32)nonDecimal;
			
			s32 mod = 100;
			while(mod != 0)
			{
				
				s32 digit = nonDecimal / mod;
				if(digit != 0)
				{
					chars[c++] = (u8)(digit + '0');
					nonDecimal -= (digit * mod);
				}
				
				mod /= 10;
			}
		}
		
		chars[c++] = '.';
		
		for(u32 i = 0; i < 6; i++)
		{
			current *= 10;
			s32 digit = (s32)current;
			current -= (r32)digit;
			chars[c++] = (u8)(digit + '0');
		}
		
		temp = create_string(&jsonFile, (char *)chars);
		
		if(whichNum == 3)
		{
			temp = create_string(&jsonFile, "},");
		}
		
		num++;
		count++;
	}
	
	temp.chars[(temp.len-1)] = '\n';
	temp = create_string(&jsonFile, "\t]\n}");
	
	b32 jsonWriteSucceeded = Win64WriteEntireFile("d:\\programming\\github\\haversine-thing\\haversine\\build\\tenMil.json", (u32)((jsonFile.end - jsonFile.memory) + 1), jsonFile.memory);
	Assert(jsonWriteSucceeded == TRUE);
	
	return(0);
}