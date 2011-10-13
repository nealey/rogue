/*
 * Contains functions for dealing with things that happen in the
 * future.
 *
 * @(#)rread.c	4.7 (Berkeley) 02/05/99
 */

#include <curses.h>
#include "netprot.h"

#define EMPTY 0
#define DAEMON -1
#define MAXDAEMONS 20

#define _X_ { EMPTY }

struct delayed_action {
    int d_type;
    void (*d_func)(int);
    int d_arg;
    int d_time;
} d_list[MAXDAEMONS] = {
    _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_,
    _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, _X_, 
};

/*
 * d_slot:
 *	Find an empty slot in the daemon/fuse list
 */
struct delayed_action *
d_slot(void)
{
    struct delayed_action *dev;

    for (dev = d_list; dev < &d_list[MAXDAEMONS]; dev++)
	if (dev->d_type == EMPTY)
	    return dev;
#ifdef MASTER
    debug("Ran out of fuse slots");
#endif
    return NULL;
}

/*
 * find_slot:
 *	Find a particular slot in the table
 */
struct delayed_action *
find_slot(void (*func)(int))
{
    struct delayed_action *dev;

    for (dev = d_list; dev < &d_list[MAXDAEMONS]; dev++)
	if (dev->d_type != EMPTY && func == dev->d_func)
	    return dev;
    return NULL;
}

/*
 * start_daemon:
 *	Start a daemon, takes a function.
 */
void
start_daemon(void (*func)(int), int arg, int type)
{
    struct delayed_action *dev;

    dev = d_slot();
    dev->d_type = type;
    dev->d_func = func;
    dev->d_arg = arg;
    dev->d_time = DAEMON;
}

/*
 * kill_daemon:
 *	Remove a daemon from the list
 */
void
kill_daemon(void (*func)(int))
{
    struct delayed_action *dev;

    if ((dev = find_slot(func)) == NULL)
	return;
    /*
     * Take it out of the list
     */
    dev->d_type = EMPTY;
}

/*
 * do_daemons:
 *	Run all the daemons that are active with the current flag,
 *	passing the argument to the function.
 */
void
do_daemons(flag)
int flag;
{
    struct delayed_action *dev;

    /*
     * Loop through the devil list
     */
    for (dev = d_list; dev < &d_list[MAXDAEMONS]; dev++)
	/*
	 * Executing each one, giving it the proper arguments
	 */
	if (dev->d_type == flag && dev->d_time == DAEMON)
	    (*dev->d_func)(dev->d_arg);
}

/*
 * fuse:
 *	Start a fuse to go off in a certain number of turns
 */
void
fuse(void (*func)(int), int arg, int tm, int type)
{
    struct delayed_action *wire;

    wire = d_slot();
    wire->d_type = type;
    wire->d_func = func;
    wire->d_arg = arg;
    wire->d_time = tm;
}

/*
 * lengthen:
 *	Increase the time until a fuse goes off
 */
void
lengthen(void (*func)(int), int xtime)
{
    struct delayed_action *wire;

    if ((wire = find_slot(func)) == NULL)
	return;
    wire->d_time += xtime;
}

/*
 * extinguish:
 *	Put out a fuse
 */
void
extinguish(void (*func)(int))
{
    struct delayed_action *wire;

    if ((wire = find_slot(func)) == NULL)
	return;
    wire->d_type = EMPTY;
}

/*
 * do_fuses:
 *	Decrement counters and start needed fuses
 */
void
do_fuses(int flag)
{
    struct delayed_action *wire;

    /*
     * Step though the list
     */
    for (wire = d_list; wire < &d_list[MAXDAEMONS]; wire++)
	/*
	 * Decrementing counters and starting things we want.  We also need
	 * to remove the fuse from the list once it has gone off.
	 */
	if (flag == wire->d_type && wire->d_time > 0 && --wire->d_time == 0)
	{
	    wire->d_type = EMPTY;
	    (*wire->d_func)(wire->d_arg);
	}
}
