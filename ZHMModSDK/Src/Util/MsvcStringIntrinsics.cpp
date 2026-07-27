// Provide intrinsics for cross-compiling on Linux.
#if defined(__clang__) && defined(_M_X64)

__asm__(
    ".intel_syntax noprefix\n"
    ".text\n"

    ".globl __movsb\n"
    "__movsb:\n"               // (RCX = dst, RDX = src, R8 = count)
    "    push rdi\n"
    "    push rsi\n"
    "    mov rdi, rcx\n"
    "    mov rsi, rdx\n"
    "    mov rcx, r8\n"
    "    rep movsb\n"
    "    pop rsi\n"
    "    pop rdi\n"
    "    ret\n"

    ".globl __stosb\n"
    "__stosb:\n"               // (RCX = dst, DL = value, R8 = count)
    "    push rdi\n"
    "    mov rdi, rcx\n"
    "    mov al, dl\n"
    "    mov rcx, r8\n"
    "    rep stosb\n"
    "    pop rdi\n"
    "    ret\n"

    ".att_syntax prefix\n"
);

#endif
