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
		
	}
	
	r32 result = ((randomR32 * (max - min)) + min);
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
	
	s32 size_r32 = sizeof(r32);
	u32 buffer_size = (40000000 * (u32)size_r32);
	Buffer coordinates = win64_make_buffer(AlignSize(buffer_size, PAGE), PAGE_READWRITE);
	
	u32 count = 0;
	while(count < 40000000)
	{
		
		r32 minLong = -180.0;
		r32 maxLong = 180.0;
		r32 randLong = RangedRandomR32(minLong, maxLong);
		buffer_append_r32(&coordinates, randLong);
		count++;
		
		r32 minLat = -90.0;
		r32 maxLat = 90.0;
		r32 randLat = RangedRandomR32(minLat, maxLat);
		buffer_append_r32(&coordinates, randLat);
		count++;
		
	}
	
	return(0);
}