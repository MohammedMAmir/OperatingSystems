#include "thread.h"
#include "interrupt.h"
#include "test_thread.h"

int
main(int argc, char **argv)
{
	thread_init();
	register_interrupt_handler(0);
	/* Test wait when target thread has exited prior to wait. */
	test_wait_exited();	
	return 0;
}
