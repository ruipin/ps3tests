#include <stdio.h>
#include <stdlib.h>

#include "../ppu_header.h"

// Set priority and stack size for the primary PPU thread.
// Priority : 1000
// Stack    : 64KB
SYS_PROCESS_PARAM(1000, 0x10000)

static u64 _start_time;

typedef u32 error_code;
typedef u32 sys_config_t;
typedef u32 sys_config_listener_t;
typedef u32 sys_config_service_t;

#define MAX_EVENT_SIZE 256
typedef struct sys_config_service_event_t {
	u32 service_listener_handle; // describes the listener this config event is for
	u32 event_added; // 1 if this is a notification that a config event was added; 0 if the config event was removed
	u64 service_id; // describes the service that sent this config event
	u64 data1; // 'c' parameter supplied to sys_config_register_service
	/* if event_added==0, the following fields will not be present */
	u64 data2; // 'd' parameter supplied to sys_config_register_service
	u64 buf_size; // size of 'buf' supplied to sys_config_register_service
	u32 padding; // not 100% sure (sometimes non-zero), haven't been able to find any place where this is written to and/or read from
	u8 buf[256]; // 'buf' supplied to sys_config_register_service
} sys_config_service_event_t;

#define SOURCE_SERVICE_EVENT 1ll
#define SOURCE_IO_EVENT 2ll

/*
 * sys_config syscalls
 */
SYSCALL(516, sys_config_open                         , sys_event_queue_t equeue_id, sys_config_t *out_handle)
SYSCALL(517, sys_config_close                        , sys_config_t config)
SYSCALL(518, sys_config_get_service_event            , sys_config_t config, u32 event_id, void* dst, u64 size)
SYSCALL(519, sys_config_add_service_listener         , sys_config_t config, s64 service_id, u32 flag1, void *in, u32 size, u32 flag2, sys_config_listener_t *out_listener)
SYSCALL(520, sys_config_remove_service_listener      , sys_config_t config, sys_config_listener_t listener)
SYSCALL(521, sys_config_register_service             , sys_config_t config, s64 service_id, u64 data1, u64 data2, void *data_buf, u32 size, sys_config_service_t *out_service)
SYSCALL(522, sys_config_unregister_service           , sys_config_t config, sys_config_service_t service)
//SYSCALL(523, sys_config_io_event                     , ...)
//SYSCALL(524, sys_config_register_io_error_listener   , ...)
//SYSCALL(525, sys_config_unregister_io_error_listener , ...)


/*
 * Static variables & defines
 */
#define CFG0 (sys_config_t)(0)
#define CFG1 (sys_config_t)(1)

#define NUM_LISTENER_THREADS 2
static sys_ppu_thread_t tids[NUM_LISTENER_THREADS];

#define SERVICE_1 0x8000000000010001ll

static sys_config_t configs[NUM_LISTENER_THREADS];
static sys_event_queue_t equeues[NUM_LISTENER_THREADS];

#define NUM_SERVICE_HANDLES 64
static sys_config_service_t services[NUM_SERVICE_HANDLES];

#define NUM_LISTENER_HANDLES 64
static sys_config_listener_t listeners[NUM_LISTENER_HANDLES];

/*
 * Utilities
 */
sys_config_service_t register_service(int config_num, s64 service_id, u64 data1, u64 data2, void *data_buf, u32 size) {
	sys_config_service_t service;

	ERROR_EXIT(sys_config_register_service(configs[config_num], service_id, data1, data2, data_buf, size, &service));
	INFO("REGISTERED: service 0x%x => config=0x%x (num=%d), sid=0x%llx, data1=0x%llx, data2=0x%llx, buf_size=%u", (u32)service, (u32)configs[config_num], config_num, service_id, data1, data2, size);

	return service;
}

void unregister_service(int config_num, int service_num) {
	sys_config_t config = configs[config_num];
	sys_config_service_t service = services[service_num];

	ERROR_EXIT(sys_config_unregister_service(configs[config_num], services[service_num]));
	INFO("UNREGISTERED: service 0x%x (num=%d) from config=0x%x (num=%d)", (u32)service, service_num, (u32)config, config_num); 
}

sys_config_listener_t add_listener(int config_num, s64 service_id, u32 flag1, u32 flag2, void *data_buf, u32 size) {
	sys_config_listener_t listener;

	ERROR_EXIT(sys_config_add_service_listener(configs[config_num], service_id, flag1, data_buf, size, flag2, &listener));
	INFO("ADDED: listener 0x%x => config=0x%x (num=%d), sid=0x%llx, flag1=0x%llx, flag2=0x%llx, buf_size=%u", (u32)listener, (u32)configs[config_num], config_num, service_id, flag1, flag2, size);

	return listener;
}

void remove_listener(int config_num, int listener_num) {
	sys_config_t config = configs[config_num];
	sys_config_listener_t listener = listeners[listener_num];

	ERROR_EXIT(sys_config_remove_service_listener(config, listener));
	INFO("REMOVED: listener 0x%x (num=%d) from config=0x%x (num=%d)", (u32)listener, listener_num, (u32)config, config_num); 
}



int dump_to_string(void* mem, char* dest, uint32_t size)
{
	uint8_t *mem8 = (uint8_t*)mem;

	uint32_t i;
	for(i = 0; i < size; i++)
	{
		dest += sprintf(dest, "%02x", *mem8);
		mem8 += 1;
	}

	return i * 2;
}

void print_service(u64 tid, u32 event_id, sys_config_service_event_t *sev) {
	char buf[2048];
	char *cur = buf;

	cur += sprintf(cur, "TID%lld Service Event 0x%x:\n", tid, event_id);
	cur += sprintf(cur, "\tlistener= 0x%llx    added= %d\n", sev->service_listener_handle, sev->event_added);
	cur += sprintf(cur, "\tservice_id= 0x%llx\n", sev->service_id);
	cur += sprintf(cur, "\tdata1= 0x%llx\n", sev->data1);
	cur += sprintf(cur, "\tdata2= 0x%llx\n", sev->data2);
	cur += sprintf(cur, "\tbuf_size= 0x%x\n", sev->buf_size);
	cur += sprintf(cur, "\tpadding= 0x%x\n", sev->padding);
	cur += sprintf(cur, "\tbuf= ", sev->buf_size);
	cur += dump_to_string(sev->buf, cur, sev->buf_size);

	INFO("%s", buf);
}


/*
 * Threads
 */
void listener_thread(u64 tid)
{
	INFO("TID%llu started.", tid);

	for(;;) {
		sys_event_t queue_event;

		ERROR_EXIT(sys_event_queue_receive(equeues[tid], &queue_event, 0));

		INFO("TID%llu received queue event: source=0x%llx data1=0x%llx data2=0x%llx data3=0x%llx", tid, queue_event.source, queue_event.data1, queue_event.data2, queue_event.data3);

		// if service event
		if(queue_event.source == SOURCE_SERVICE_EVENT) {
			u32 config_handle = (u32)(queue_event.data1);
			u32 event_id = (u32)(queue_event.data2 & 0xffffffff);
			u32 buf_size = queue_event.data3;

			sys_config_service_event_t service_event;
			ERROR_EXIT(sys_config_get_service_event(config_handle, event_id, (void*)&service_event, buf_size));

			print_service(tid, event_id, &service_event);
		}
	}

	sys_ppu_thread_exit(0);
}




/*
 * Main
 */
int main() 
{
	_start_time = get_time();

	// Create event queues
	INFO("Initializing...");

	// Create queues
	for(int i = 0; i < NUM_LISTENER_THREADS; i++) {
		sys_event_queue_attr queue_attr;
		sys_event_queue_attribute_initialize(queue_attr);
		sprintf(queue_attr.name, "equeue%d", i);

		ERROR_EXIT(sys_event_queue_create(&equeues[i], &queue_attr, SYS_EVENT_QUEUE_LOCAL, 127));
		INFO("\tQueue %d: %lx", i, (u64)equeues[i]);
	}

	// Open first sys_config handle
	ERROR_EXIT(sys_config_open(equeues[0], &configs[0]));
	ERROR_EXIT(sys_config_open(equeues[1], &configs[1]));
	INFO("Sysconfig handle 0= %lx", (u64)configs[0]);
	INFO("Sysconfig handle 1= %lx", (u64)configs[1]);

	// Spawn threads
	ERROR_EXIT(sys_ppu_thread_create(&tids[0], &listener_thread, 0, 1001, 0x10000, 1, "t0"));
	ERROR_EXIT(sys_ppu_thread_create(&tids[1], &listener_thread, 1, 1001, 0x10000, 1, "t1"));
	sys_timer_sleep(1);

	// Register the first listener
	listeners[0] = add_listener(0, SERVICE_1, 1, 1, NULL, 0);

	// Register first service
	services[0] = register_service(CFG0, SERVICE_1, 0x1a, 0x1b, NULL, 0);
	services[1] = register_service(CFG0, SERVICE_1, 0x2a, 0x2b, NULL, 0);
	services[2] = register_service(CFG0, SERVICE_1, 0x3a, 0x3b, NULL, 0);

	sys_timer_sleep(1);
	unregister_service(CFG1, 1);
	sys_timer_sleep(1);

	// Register the second listener
	listeners[1] = add_listener(CFG1, SERVICE_1, 1, 1, NULL, 0);
	sys_timer_sleep(1);
	remove_listener(CFG0, 1);

	sys_timer_sleep(5);

	return 0;
}