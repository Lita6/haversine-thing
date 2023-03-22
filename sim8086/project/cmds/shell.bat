@echo off

set PATH=d:\programming\github\haversine-thing\sim8086\project\cmds;%PATH%

cd sim8086\project\code
call ..\cmds\4coder
call ..\cmds\asm_rdbg
call ..\cmds\dsm_rdbg
call ..\cmds\diff_rdbg
cls