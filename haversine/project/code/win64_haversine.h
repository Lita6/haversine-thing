/* date = December 22nd 2022 5:14 pm */

#ifndef WIN64_HAVERSINE_H
#define WIN64_HAVERSINE_H

/*
*
* NOTE: TYPES
*
*/

#include <stdint.h>
#include <stddef.h>
#include <intrin.h>

#define global static

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define MAX_U8 0xFF
#define MAX_U16 0XFFFF
#define MAX_U32 0xFFFFFFFF
#define MAX_U64 0XFFFFFFFFFFFFFFFF

typedef s8 b8;
typedef s16 b16;
typedef s32 b32;

#define Pi32 3.141159265359f

#define TRUE 1
#define FALSE 0

extern "C"
{
	// NOTE: This is needed for the linker to use floats
	int _fltused;
}

typedef float r32;
typedef double r64;

#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}

u32
SafeTruncateS64toU32(s64 Value)
{
	Assert(Value >= 0);
	Assert(Value <= MAX_U32);
	u32 Result = (u32)Value;
	return(Result);
}

struct read_file_result
{
	u32 ContentsSize;
	void *Contents;
};

void 
Win64FreeMemory
(void *Memory)
{
	if(Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

read_file_result
Win64ReadEntireFile
(char *FileName)
{
	read_file_result Result = {};
	
	HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(FileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize;
		if(GetFileSizeEx(FileHandle, &FileSize))
		{
			u32 FileSize32 = SafeTruncateS64toU32((s64)FileSize.QuadPart);
			Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if(Result.Contents)
			{
				DWORD BytesRead;
				if(ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) && (FileSize32 == BytesRead))
				{
					Result.ContentsSize = FileSize32;
				}
				else
				{
					Win64FreeMemory(Result.Contents);
					Result.Contents = 0;
				}
			}
			else
			{
				Assert(!"Failed to allocate memory for file read.\n");
			}
		}
		else
		{
			Assert(!"Failed to get file size after opening.\n");
		}
		
		CloseHandle(FileHandle);
	}
	else
	{
		Assert(!"Failed to open file.\n");
	}
	
	return(Result);
}

b32
Win64WriteEntireFile(char *FileName, u32 MemorySize, void *Memory)
{
	b32 Result = false;
	
	HANDLE FileHandle = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if(FileHandle != INVALID_HANDLE_VALUE)
	{
		DWORD BytesWritten;
		if(WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
		{
			Result = (BytesWritten == MemorySize);
		}
		else
		{
			Assert(!"Failed to write file.\n");
		}
		
		CloseHandle(FileHandle);
	}
	else
	{
		Assert(!"Failed to create file to write.\n");
	}
	
	return(Result);
}

struct Buffer
{
	u8 *memory;
	u8 *end;
	u32 size;
};

u8 *
buffer_allocate
(Buffer *buffer, u32 amount)
{
	
	Assert((buffer->end + amount) <= (buffer->memory + buffer->size));
	
	u8 *Result = buffer->end;
	buffer->end += amount;
	
	return(Result);
}

#define define_buffer_append(Type) \
inline void \
buffer_append_##Type \
(Buffer *buffer, Type value) \
{ \
Assert((buffer->end + sizeof(Type)) <= (buffer->memory + buffer->size)); \
*(Type *)buffer->end = value; \
buffer->end += sizeof(Type); \
}

define_buffer_append(s8)
define_buffer_append(s16)
define_buffer_append(s32)
define_buffer_append(s64)

define_buffer_append(u8)
define_buffer_append(u16)
define_buffer_append(u32)
define_buffer_append(u64)

define_buffer_append(r32)
define_buffer_append(r64)
#undef define_buffer_append

Buffer
win64_make_buffer
(u32 size, u32 permission)
{
	Buffer buffer = {};
	buffer.memory = (u8 *)(VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, permission));
	Assert(buffer.memory);
	buffer.end = buffer.memory;
	buffer.size = size;
	
	return(buffer);
}

struct String
{
	u8 *chars;
	u32 len;
};

String
create_string
(Buffer *buffer, char *str)
{
	
	String result = {};
	result.chars = buffer->end;
	
	u8 *index = (u8 *)str;
	while(*index)
	{
		buffer_append_u8(buffer, *index++);
		result.len++;
	}
	
	return(result);
}

u32
GetStringLength
(u8 *chars)
{
	u32 result = 0;
	
	while(*chars++ != 0)
	{
		result++;
	}
	
	return(result);
}

u32
scan_string
(String string, u8 ch)
{
	
	u32 result = 0;
	for(u32 i = 0; i < string.len; i++)
	{
		if(string.chars[i] == ch)
		{
			result = i;
			break;
		}
	}
	
	return(result);
}

b32
IsMemZero
(u8 *memory, u32 size)
{
	b32 result = TRUE;
	for(u32 i = 0; i < size; i++)
	{
		if(memory[i] != 0)
		{
			result = FALSE;
			break;
		}
	}
	
	return(result);
}

r32
DegreesToRadians
(r32 deg)
{
	r32 result = deg * Pi32 / 180.0f;
	return(result);
}

r32
sqrt32
(r32 a)
{
	r32 result = 0;
	
	__m128 float_reg = _mm_load_ss(&a);
	float_reg = _mm_sqrt_ss(float_reg);
	result = _mm_cvtss_f32(float_reg);
	
	return(result);
}

r32
pow
(r32 a, r32 b)
{
	r32 result = a;
	
	for(u32 i = 0; i < ((u32)b-1); i++)
	{
		result *= a;
	}
	
	return(result);
}

r32
sin32
(r32 rad)
{
	
	r32 result = rad - (pow(rad, 3.0f) / 6.0f) + (pow(rad, 5.0f) / 120.0f) - (pow(rad, 7.0f) / 5040.0f);
	return(result);
}

r32
cos32
(r32 rad)
{
	
	r32 result = 1 - (pow(rad, 2.0f) / 2.0f) + (pow(rad, 4.0f) / 24.0f) - (pow(rad, 6.0f) / 720.0f);
	return(result);
}

r32
asin32
(r32 z)
{
	r32 result = z + (0.5f * (pow(z, 3.0f) / 3.0f)) + (0.375f * (pow(z, 5.0f) / 5.0f)) + (0.3125f * (pow(z, 7.0f) / 7.0f));
	return(result);
}

#endif //WIN64_HAVERSINE_H
