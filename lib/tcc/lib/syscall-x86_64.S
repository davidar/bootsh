.global __syscall0
.type __syscall0,@function
__syscall0:
    movq %rdi,%rax
    syscall
    ret

.global __syscall1
.type __syscall1,@function
__syscall1:
    movq %rdi,%rax
    movq %rsi,%rdi
    syscall
    ret

.global __syscall2
.type __syscall2,@function
__syscall2:
    movq %rdi,%rax
    movq %rsi,%rdi
    movq %rdx,%rsi
    syscall
    ret

.global __syscall3
.type __syscall3,@function
__syscall3:
    movq %rdi,%rax
    movq %rsi,%rdi
    movq %rdx,%rsi
    movq %rcx,%rdx
    syscall
    ret

.global __syscall4
.type __syscall4,@function
__syscall4:
    movq %rdi,%rax
    movq %rsi,%rdi
    movq %rdx,%rsi
    movq %rcx,%rdx
    movq %r8,%r10
    syscall
    ret

.global __syscall5
.type __syscall5,@function
__syscall5:
    movq %rdi,%rax
    movq %rsi,%rdi
    movq %rdx,%rsi
    movq %rcx,%rdx
    movq %r8,%r10
    movq %r9,%r8
    syscall
    ret

.global __syscall6
.type __syscall6,@function
__syscall6:
    movq %rdi,%rax
    movq %rsi,%rdi
    movq %rdx,%rsi
    movq %rcx,%rdx
    movq %r8,%r10
    movq %r9,%r8
    movq 8(%rsp),%r9
    syscall
    ret
