.386
.model flat, stdcall
option casemap :none

extern _except_handler:near

_except_handler_safeseh proto
.SAFESEH _except_handler_safeseh

.code
_except_handler_safeseh proc
	jmp _except_handler
_except_handler_safeseh endp
end