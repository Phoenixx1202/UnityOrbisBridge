.intel_syntax noprefix
.text

.global SYSCALL

SYSCALL:
    mov rax, 0
    mov r10, rcx
    syscall
    jb err
    ret

err:
    push rax
    call __error
    pop rcx
    mov [rax], ecx
    mov rax, -1
    mov rdx, -1
    ret
