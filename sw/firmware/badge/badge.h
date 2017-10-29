#ifndef _BADGE_H_
#define _BADGE_H_
#include "chprintf.h"

#define printf(fmt, ...)                                        \
    chprintf((BaseSequentialStream*)&SD1, fmt, ##__VA_ARGS__)

/* Orchard event wrappers.
   These simplify the ChibiOS eventing system.  To use, initialize the event
   table, hook an event, and then dispatch events.

  static void shell_termination_handler(eventid_t id) {
    chprintf(stream, "Shell exited.  Received id %d\r\n", id);
  }

  void main(int argc, char **argv) {
    struct evt_table events;

    // Support up to 8 events
    evtTableInit(events, 8);

    // Call shell_termination_handler() when 'shell_terminated' is emitted
    evtTableHook(events, shell_terminated, shell_termination_handler);

    // Dispatch all events
    while (TRUE)
      chEvtDispatch(evtHandlers(events), chEvtWaitOne(ALL_EVENTS));
   }
 */

struct evt_table {
	int size;
	int next;
	evhandler_t *handlers;
	event_listener_t *listeners;
};

#define evtTableInit(table, capacity)					\
	do {								\
		static evhandler_t handlers[capacity];			\
		static event_listener_t listeners[capacity];		\
		table.size = capacity;					\
		table.next = 0;						\
		table.handlers = handlers;				\
		table.listeners = listeners;				\
	} while(0)

#define evtTableHook(table, event, callback)				\
	do {								\
		if (CH_DBG_ENABLE_ASSERTS != FALSE)			\
			if (table.next >= table.size)			\
				chSysHalt("event table overflow");	\
		chEvtRegister(&event, &table.listeners[table.next],	\
		    table.next);					\
		table.handlers[table.next] = callback;			\
		table.next++;						\
	} while(0)

#define evtTableUnhook(table, event, callback)				\
	do {								\
		int i;							\
		for (i = 0; i < table.next; i++) {			\
			if (table.handlers[i] == callback)		\
			chEvtUnregister(&event, &table.listeners[i]);	\
		}							\
  } while(0)

#define evtHandlers(table)						\
	table.handlers

#define evtListeners(table)						\
	table.listeners

#define orchard_command_start()						\
	static char start[4] __attribute__((unused,			\
	    aligned(4), section(".chibi_list_cmd_1")))

#define orchard_commands      (const ShellCommand *)&start[4]

#define orchard_command(_name, _func)					\
	const ShellCommand _orchard_cmd_list_##_func			\
	__attribute__((used, aligned(4),				\
	     section(".chibi_list_cmd_2_" _name))) = { _name, _func }

#define orchard_command_end()						\
	const ShellCommand _orchard_cmd_list_##_func			\
	__attribute__((used, aligned(4),				\
	    section(".chibi_list_cmd_3_end"))) = { NULL, NULL }


#endif /* _BADGE_H_ */
