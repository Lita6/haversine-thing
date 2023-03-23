#include <windows.h>

#include "win64_assemble.h"

global u32 PAGE;

struct List_Entry
{
	String name;
	u8 reg_address;
};

struct List
{
	Buffer *buffer;
	List_Entry *start;
	u32 count;
};

void
create_list_entry
(Buffer *strings, List *list, char *name, u8 reg_address)
{
	List_Entry *temp = (list->start + list->count);
	buffer_allocate(list->buffer, sizeof(String));
	temp->name = create_string(strings, name);
	temp->reg_address = reg_address;
	list->count++;
}

struct Instruction
{
	String operation;
	List_Entry operands[2];
};

void
find_reg_address
(List regs, List_Entry *operand)
{
	
	for(u32 i = 0; i < regs.count; i++)
	{
		if(operand->name == regs.start[i].name)
		{
			operand->reg_address = regs.start[i].reg_address;
			break;
		}
	}
	
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
		buffer_append_u8(dest, str.chars[i]);
	}
	
	return(result);
}

b32
IsEndOfLine
(u8 ch)
{
	b32 result = ((ch == '\r') || (ch == '\n')) ? TRUE : FALSE;
	return(result);
}

u8 *
findEndOfLine
(u8 *read, u8 *max)
{
	
	u8 *result = read;
	while((IsEndOfLine(*result) == FALSE) && (result < max))
	{
		result++;
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
	
	Buffer program = win64_make_buffer(PAGE, PAGE_READWRITE);
	Buffer buffer_strings = win64_make_buffer(PAGE, PAGE_READWRITE);
	Buffer buffer_reserved = win64_make_buffer(PAGE, PAGE_READWRITE);
	
	List regs = {};
	regs.start = (List_Entry *)buffer_reserved.end;
	regs.buffer = &buffer_reserved;
	create_list_entry(&buffer_strings, &regs, "ax", 0b1000);
	create_list_entry(&buffer_strings, &regs, "cx", 0b1001);
	create_list_entry(&buffer_strings, &regs, "dx", 0b1010);
	create_list_entry(&buffer_strings, &regs, "bx", 0b1011);
	create_list_entry(&buffer_strings, &regs, "sp", 0b1100);
	create_list_entry(&buffer_strings, &regs, "bp", 0b1101);
	create_list_entry(&buffer_strings, &regs, "si", 0b1110);
	create_list_entry(&buffer_strings, &regs, "di", 0b1111);
	create_list_entry(&buffer_strings, &regs, "al", 0b000);
	create_list_entry(&buffer_strings, &regs, "cl", 0b001);
	create_list_entry(&buffer_strings, &regs, "dl", 0b010);
	create_list_entry(&buffer_strings, &regs, "bl", 0b011);
	create_list_entry(&buffer_strings, &regs, "ah", 0b100);
	create_list_entry(&buffer_strings, &regs, "ch", 0b101);
	create_list_entry(&buffer_strings, &regs, "dh", 0b110);
	create_list_entry(&buffer_strings, &regs, "bh", 0b111);
	
	u8 *commandLine = (u8 *)GetCommandLineA();
	
	while((*commandLine != ' ') && (*commandLine != 0))
	{
		commandLine++;
	}
	Assert(*commandLine != 0);
	commandLine++;
	
	String fileName = create_string(&buffer_strings, (char *)commandLine);
	buffer_append_u8(&buffer_strings, 0x00);
	fileName.len++;
	
	read_file_result file = Win64ReadEntireFile((char *)fileName.chars);
	u8 *EndOfFile = (u8 *)file.Contents + file.ContentsSize;
	
	Instruction instr = {};
	u8 *ReadByte = (u8 *)file.Contents;
	while(ReadByte < EndOfFile)
	{
		
		if((*ReadByte == ';') || (*ReadByte == 'b'))
		{
			ReadByte = findEndOfLine(ReadByte, EndOfFile);
		}
		
		if(IsEndOfLine(*ReadByte))
		{
			ReadByte++;
			continue;
		}
		
		instr.operation.chars = ReadByte;
		while(*ReadByte != ' ')
		{
			instr.operation.len++;
			ReadByte++;
		}
		
		u32 current = 0;
		ReadByte++;
		while((*ReadByte != '\r') && (*ReadByte != '\n') && (ReadByte < EndOfFile))
		{
			
			if(*ReadByte == ',')
			{
				
				current++;
				ReadByte++;
			}
			else
			{
				
				if(instr.operands[current].name.chars == 0)
				{
					instr.operands[current].name.chars = ReadByte;
				}
				
				instr.operands[current].name.len++;
			}
			
			ReadByte++;
		}
		
		u8 op_code = 0x88;
		
		find_reg_address(regs, &instr.operands[0]);
		find_reg_address(regs, &instr.operands[1]);
		
		if((instr.operands[0].reg_address & 0b1000) > 1)
		{
			op_code |= 0b01;
		}
		
		u8 modrm = (u8)((0b11 << 6) | ((instr.operands[1].reg_address & 0b111) << 3) | (instr.operands[0].reg_address & 0b111));
		
		buffer_append_u8(&program, op_code);
		buffer_append_u8(&program, modrm);
		
		instr = {};
	}
	
	String outputName = create_string(&buffer_strings, "output\\");
	fileName.len = scan_string(fileName, '.');
	
	u32 i = scan_string(fileName, '\\');
	if(i != 0)
	{
		i++;
		fileName.chars += i;
		fileName.len -= i;
	}
	
	String temp = copy_string(&buffer_strings, fileName);
	outputName.len += temp.len;
	buffer_append_u8(&buffer_strings, 0);
	outputName.len++;
	
	b32 writeSucceeded = Win64WriteEntireFile((char *)outputName.chars, (u32)(program.end - program.memory), (void *)program.memory);
	(void)writeSucceeded;
	
	return(0);
}