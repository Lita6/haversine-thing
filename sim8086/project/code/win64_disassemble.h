#ifndef WIN64_DISASSEMBLE_H
#define WIN64_DISASSEMBLE_H

/*
*
* NOTE: TYPES
*
*/

#include <stdint.h>
#include <stddef.h>

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

#define TRUE 1
#define FALSE 0

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
	u8 *End;
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
					Result.End = (u8 *)Result.Contents + FileSize32;
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

b32
operator==
(String a, String b)
{
	b32 result = TRUE;
	
	if(a.len != b.len)
	{
		result = FALSE;
		
	}
	else
	{
		for(u64 i = 0; i < a.len; i++)
		{
			
			if(a.chars[i] != b.chars[i])
			{
				result = FALSE;
				break;
				
			}
		}
	}
	
	return(result);
}

void
append_string
(Buffer *buffer, String str)
{
	for(u32 i = 0; i < str.len; i++)
	{
		buffer_append_u8(buffer, str.chars[i]);
	}
}

String
U8ToString
(Buffer *strings, u8 num)
{
	
	String result ={};
	
#define MAX 3
	
	u8 digits[MAX] = {};
	u8 mod = 10;
	u8 read = num;
	for(u8 i = 0; i < MAX; i++)
	{
		digits[i] = (u8)(read % mod);
		read -= digits[i];
		
		if(read == 0)
		{
			result.len = i;
			break;
		}
		
		read /= mod;
	}
	
	result.chars = strings->end;
	for(s32 i = (s32)result.len; i >= 0; i--)
	{
		buffer_append_u8(strings, (u8)(digits[i] + '0'));
	}
	
#undef MAX
	result.len++;
	return(result);
}

String
U16ToString
(Buffer *strings, u16 num)
{
	
	String result ={};
	
#define MAX 5
	
	u8 digits[MAX] = {};
	u16 mod = 10;
	u16 read = num;
	for(u8 i = 0; i < MAX; i++)
	{
		digits[i] = (u8)(read % mod);
		read -= (u16)digits[i];
		
		if(read == 0)
		{
			result.len = i;
			break;
		}
		
		read /= mod;
	}
	
	result.chars = strings->end;
	for(s32 i = (s32)result.len; i >= 0; i--)
	{
		buffer_append_u8(strings, (u8)(digits[i] + '0'));
	}
	
#undef MAX
	result.len++;
	return(result);
}

#endif //WIN64_DISASSEMBLE_H
