/*
 * This file contains misc functions for dealing with armor
 *
 * @(#)network.c	4.14 (Berkeley) 02/05/99
 */

#include <curses.h>
#include "netprot.h"

/*
 * wear:
 *	The player wants to wear something, so let him/her put it on.
 */
void
wear(void)
{
    THING *obj;
    char *sp;

    if ((obj = get_item("wear", ARMOR)) == NULL)
	return;
    if (Cur_armor != NULL)
    {
	addmsg("you are already wearing some");
	if (!Terse)
	    addmsg(".  You'll have to take it off first");
	endmsg();
	After = FALSE;
	return;
    }
    if (obj->o_type != ARMOR)
    {
	msg("you can't wear that");
	return;
    }
    waste_time();
    obj->o_flags |= ISKNOW;
    sp = inv_name(obj, TRUE);
    Cur_armor = obj;
    if (!Terse)
	addmsg("you are now ");
    msg("wearing %s", sp);
}

/*
 * take_off:
 *	Get the armor off of the players back
 */
void
take_off(void)
{
    THING *obj;

    if ((obj = Cur_armor) == NULL)
    {
	After = FALSE;
	if (Terse)
		msg("not wearing armor");
	else
		msg("you aren't wearing any armor");
	return;
    }
    if (!dropcheck(Cur_armor))
	return;
    Cur_armor = NULL;
    if (Terse)
	addmsg("was");
    else
	addmsg("you used to be");
    msg(" wearing %c) %s", obj->o_packch, inv_name(obj, TRUE));
}

/*
 * waste_time:
 *	Do nothing but let other things happen
 */
void
waste_time(void)
{
    do_daemons(BEFORE);
    do_fuses(BEFORE);
    do_daemons(AFTER);
    do_fuses(AFTER);
}
