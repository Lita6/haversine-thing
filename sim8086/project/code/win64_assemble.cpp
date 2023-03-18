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
	create_list_entry(&buffer_strings, &regs, "ax", 0b000);
	create_list_entry(&buffer_strings, &regs, "cx", 0b001);
	create_list_entry(&buffer_strings, &regs, "dx", 0b010);
	create_list_entry(&buffer_strings, &regs, "bx", 0b011);
	
	u8 *commandLine = (u8 *)GetCommandLineA();
	commandLine += 0x49;
	String fileName = create_string(&buffer_strings, (char *)commandLine);
	buffer_append_u8(&buffer_strings, 0x00);
	fileName.len++;
	
	read_file_result file = Win64ReadEntireFile((char *)fileName.chars);
	u8 *EndOfFile = (u8 *)file.Contents + file.ContentsSize;
	
	
	Instruction instr = {};
	
	u8 *ReadByte = (u8 *)file.Contents;
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
	
	u8 op_code = 0x89;
	
	find_reg_address(regs, &instr.operands[0]);
	find_reg_address(regs, &instr.operands[1]);
	
	u8 modrm = (u8)((0b11 << 6) | (instr.operands[1].reg_address << 3) | instr.operands[0].reg_address);
	
	buffer_append_u8(&program, op_code);
	buffer_append_u8(&program, modrm);
	
	u32 loc = scan_string(fileName, '.');
	fileName.chars[loc] = 0;
	
	b32 writeSucceeded = Win64WriteEntireFile((char *)fileName.chars, (u32)(program.end - program.memory), (void *)program.memory);
	(void)writeSucceeded;
	
	return(0);
}