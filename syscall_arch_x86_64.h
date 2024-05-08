#define __SYSCALL_LL_E(x) (x)
#define __SYSCALL_LL_O(x) (x)

static __inline long __syscall0(long n);
asm (
	".type __syscall0, @function;"
	"__syscall0:;"
	"movq %rdi, %rax;"
	"syscall;"
	"ret"
);

static __inline long __syscall1(long n, long a1);
asm (
	".type __syscall1, @function;"
	"__syscall1:;"
	"movq %rdi, %rax;"
	"movq %rsi, %rdi;"
	"syscall;"
	"ret"
);

static __inline long __syscall2(long n, long a1, long a2);
asm (
	".type __syscall2, @function;"
	"__syscall2:;"
	"movq %rdi, %rax;"
	"movq %rsi, %rdi;"
	"movq %rdx, %rsi;"
	"syscall;"
	"ret"
);

static __inline long __syscall3(long n, long a1, long a2, long a3);
asm (
	".type __syscall3, @function;"
	"__syscall3:;"
	"movq %rdi, %rax;"
	"movq %rsi, %rdi;"
	"movq %rdx, %rsi;"
	"movq %rcx, %rdx;"
	"syscall;"
	"ret"
);

static __inline long __syscall4(long n, long a1, long a2, long a3, long a4);
asm (
	".type __syscall4, @function;"
	"__syscall4:;"
	"movq %rdi, %rax;"
	"movq %rsi, %rdi;"
	"movq %rdx, %rsi;"
	"movq %rcx, %rdx;"
	"movq %r8, %r10;"
	"syscall;"
	"ret"
);

static __inline long __syscall5(long n, long a1, long a2, long a3, long a4, long a5);
asm (
	".type __syscall5, @function;"
	"__syscall5:;"
	"movq %rdi, %rax;"
	"movq %rsi, %rdi;"
	"movq %rdx, %rsi;"
	"movq %rcx, %rdx;"
	"movq %r8, %r10;"
	"movq %r9, %r8;"
	"syscall;"
	"ret"
);

static __inline long __syscall6(long n, long a1, long a2, long a3, long a4, long a5, long a6);
asm (
	".type __syscall6, @function;"
	"__syscall6:;"
	"movq %rdi, %rax;"
	"movq %rsi, %rdi;"
	"movq %rdx, %rsi;"
	"movq %rcx, %rdx;"
	"movq %r8, %r10;"
	"movq %r9, %r8;"
	"movq 8(%rsp), %r9;"
	"syscall;"
	"ret"
);

#define VDSO_USEFUL
#define VDSO_CGT_SYM "__vdso_clock_gettime"
#define VDSO_CGT_VER "LINUX_2.6"
#define VDSO_GETCPU_SYM "__vdso_getcpu"
#define VDSO_GETCPU_VER "LINUX_2.6"

#define IPC_64 0
