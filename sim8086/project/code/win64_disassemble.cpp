#include <windows.h>

#include "win64_disassemble.h"

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

List_Entry *
find_reg
(List *list, u8 address)
{
	List_Entry *result = 0;
	
	for(u32 i = 0; i < list->count; i++)
	{
		if(address == list->start[i].reg_address)
		{
			result = &list->start[i];
			break;
		}
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
	Buffer program = win64_make_buffer(PAGE, PAGE_READWRITE);
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
	commandLine += 0x4c;
	String fileName = create_string(&buffer_strings, (char *)commandLine);
	buffer_append_u8(&buffer_strings, 0x00);
	fileName.len++;
	
	read_file_result file = Win64ReadEntireFile((char *)fileName.chars);
	u8 *EndOfFile = (u8 *)file.Contents + file.ContentsSize;
	(void)EndOfFile;
	
	u8 *ReadByte = (u8 *)file.Contents;
	while(ReadByte < EndOfFile)
	{
		
		create_string(&program, "mov ");
		u8 w = (u8)((*ReadByte & 0b01) << 3);
		ReadByte++;
		
		u8 dest = (u8)((*ReadByte & 0b111) | w);
		List_Entry *dest_reg = find_reg(&regs, dest);
		append_string(&program, dest_reg->name);
		create_string(&program, ", ");
		
		u8 src = (u8)(((*ReadByte & (0b111 << 3)) >> 3) | w);
		List_Entry *src_reg = find_reg(&regs, src);
		append_string(&program, src_reg->name);
		
		buffer_append_u8(&program, '\n');
		ReadByte++;
	}
	
	*(program.end - 1) = 0;
	
	String dsmFileName = create_string(&buffer_strings, "dsm_");
	append_string(&buffer_strings, fileName);
	dsmFileName.len += fileName.len;
	u8 *end = dsmFileName.chars + dsmFileName.len - 1;
	*end = '.';
	create_string(&buffer_strings, "asm");
	buffer_append_u8(&buffer_strings, 0);
	dsmFileName.len += 4;
	
	b32 writeSucceeded = Win64WriteEntireFile((char *)dsmFileName.chars, (u32)(program.end - program.memory), (void *)program.memory);
	(void)writeSucceeded;
	
	return(0);
}