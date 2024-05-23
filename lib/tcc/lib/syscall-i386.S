.global __syscall0
.type __syscall0,@function
__syscall0:
    movl 4(%esp),%eax
    int $0x80
    ret

.global __syscall1
.type __syscall1,@function
__syscall1:
    movl 4(%esp),%eax
    movl 8(%esp),%edx
    xchg %ebx,%edx
    int $0x80
    xchg %ebx,%edx
    ret

.global __syscall2
.type __syscall2,@function
__syscall2:
    movl 4(%esp),%eax
    movl 8(%esp),%edx
    movl 12(%esp),%ecx
    xchg %ebx,%edx
    int $0x80
    xchg %ebx,%edx
    ret

.global __syscall3
.type __syscall3,@function
__syscall3:
    pushl %ebx
    movl 8(%esp),%eax
    movl 12(%esp),%ebx
    movl 16(%esp),%ecx
    movl 20(%esp),%edx
    int $0x80
    popl %ebx
    ret

.global __syscall4
.type __syscall4,@function
__syscall4:
    pushl %esi
    pushl %ebx
    movl 12(%esp),%eax
    movl 16(%esp),%ebx
    movl 20(%esp),%ecx
    movl 24(%esp),%edx
    movl 28(%esp),%esi
    int $0x80
    popl %ebx
    popl %esi
    ret

.global __syscall5
.type __syscall5,@function
__syscall5:
    pushl %edi
    pushl %esi
    pushl %ebx
    movl 16(%esp),%eax
    movl 20(%esp),%ebx
    movl 24(%esp),%ecx
    movl 28(%esp),%edx
    movl 32(%esp),%esi
    movl 36(%esp),%edi
    int $0x80
    popl %ebx
    popl %esi
    popl %edi
    ret

.global __syscall6
.type __syscall6,@function
__syscall6:
    pushl %ebp
    pushl %edi
    pushl %esi
    pushl %ebx
    movl 20(%esp),%eax
    movl 24(%esp),%ebx
    movl 28(%esp),%ecx
    movl 32(%esp),%edx
    movl 36(%esp),%esi
    movl 40(%esp),%edi
    movl 44(%esp),%ebp
    int $0x80
    popl %ebx
    popl %esi
    popl %edi
    popl %ebp
    ret
