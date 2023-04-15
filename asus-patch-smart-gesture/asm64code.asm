
OPTION DOTNAME
option casemap :none

EXTERN MessageBoxA: PROC
EXTERN SwitchTPStatusNoBroadcastFake: PROC

PUBLIC asm_hookSwitchTPStatusNoBroadcast

EXTERN g_hookSwitchTPStatusNoBroadcastRetAddr : QWORD

_DATA SEGMENT

hello_msg db "Hello world", 0
info_msg db "Info", 0

_DATA ENDS

_TEXT SEGMENT

asm_hookSwitchTPStatusNoBroadcast PROC

	push g_hookSwitchTPStatusNoBroadcastRetAddr
	ret
asm_hookSwitchTPStatusNoBroadcast ENDP

asm_SwitchTPStatusNoBroadcast PROC
	;mov     ecx, 1
	;call    SwitchTPStatusNoBroadcastFunc
asm_SwitchTPStatusNoBroadcast ENDP

_TEXT ENDS

END
