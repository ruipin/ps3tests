#include <stdint.h>
#include <cell/sysmodule.h>
#include <cell/atomic.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <sys/memory.h>
#include <sys/event.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/process.h>
#include <sys/synchronization.h>
#include <sys/prx.h>
#include <sys/sys_time.h>

/*
 * Short-forms for various literal types
 */
typedef uintptr_t uptr;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef intptr_t sptr;
typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;


/*
 * Helper macros
 */
#define ERROR_EXIT(x) do { \
		if (u64 err = (x) != CELL_OK) { \
			printf("Failure! err=0x%lx\n", err); \
			exit(-1); \
		} \
	} while(0)

#define INFO(msg, ...) printf("%llu: " msg "\n", (get_time()-_start_time)/1000, ##__VA_ARGS__); 

#define SYSCALL(n, name, ...) \
	static __attribute__((noinline)) error_code name(__VA_ARGS__) { \
		register uint64_t p1 __asm__ ("3");       \
		register uint64_t p2 __asm__ ("4");       \
		register uint64_t p3 __asm__ ("5");       \
		register uint64_t p4 __asm__ ("6");       \
		register uint64_t p5 __asm__ ("7");       \
		register uint64_t p6 __asm__ ("8");       \
		register uint64_t p7 __asm__ ("9");       \
		register uint64_t p8 __asm__ ("10");      \
		register uint64_t pn  __asm__ ("11") = n; \
		\
		__asm__ volatile ("sc" \
			: "=r" (p1), "=r" (p2), "=r" (p3), "=r" (p4),            \
			  "=r" (p5), "=r" (p6), "=r" (p7), "=r" (p8), "=r" (pn)  \
			: "r" (p1), "r" (p2), "r" (p3), "r" (p4),                \
			  "r" (p5), "r" (p6), "r" (p7), "r" (p8), "r" (pn)       \
			: "0", "12", "lr", "ctr", "xer", "cr0", "cr1", "cr5", "cr6", "cr7", "memory" \
		); \
		\
		return static_cast<error_code>(p1); \
	}


/*
 * Time measurement
 */
inline u64 _mftb()
{
	u64 ret;
	while (!(ret = __mftb()));
	return ret;
}

inline u64 get_time(void) {
	return _mftb();
}