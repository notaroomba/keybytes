// Coroutine switch for the simulator (Linux x86_64, System V).
// void ctx_switch(void **save_sp, void *to_sp)
	.text
	.globl ctx_switch
ctx_switch:
	push %rbp
	push %rbx
	push %r12
	push %r13
	push %r14
	push %r15
	mov %rsp, (%rdi)
	mov %rsi, %rsp
	pop %r15
	pop %r14
	pop %r13
	pop %r12
	pop %rbx
	pop %rbp
	ret

// First switch into a new coroutine lands here with rbx = its node.
	.globl ctx_boot
ctx_boot:
	mov %rbx, %rdi
	call node_entry
	ud2

	.section .note.GNU-stack,"",@progbits
