/*
 * Special wizard commands (some of which are also non-wizard commands
 * under strange circumstances)
 *
 * @(#)ctlmod.c	4.30 (Berkeley) 02/05/99
 */

#include <curses.h>
#include <ctype.h>
#include "netprot.h"

/*
 * whatis:
 *	What a certin object is
 */
void
whatis(bool insist, int type)
{
    THING *obj;

    if (Pack == NULL)
    {
	msg("you don't have anything in your pack to identify");
	return;
    }

    for (;;)
    {
	obj = get_item("identify", type);
	if (insist)
	{
	    if (N_objs == 0)
		return;
	    else if (obj == NULL)
		msg("you must identify something");
	    else if (type && obj->o_type != type &&
	       !(type == R_OR_S && obj->o_type == RING || obj->o_type == STICK))
		    msg("you must identify a %s", type_name(type));
	    else
		break;
	}
	else
	    break;
    }

    if (obj == NULL)
	return;

    switch (obj->o_type)
    {
        when SCROLL:
	    set_know(obj, Scr_info);
        when POTION:
	    set_know(obj, Pot_info);
	when STICK:
	    set_know(obj, Ws_info);
        when WEAPON:
        case ARMOR:
	    obj->o_flags |= ISKNOW;
        when RING:
	    set_know(obj, Ring_info);
    }
    msg(inv_name(obj, FALSE));
}

/*
 * set_know:
 *	Set things up when we really know what a thing is
 */
void
set_know(THING *obj, struct obj_info *info)
{
    char **guess;

    info[obj->o_which].oi_know = TRUE;
    obj->o_flags |= ISKNOW;
    guess = &info[obj->o_which].oi_guess;
    if (*guess)
    {
	free(*guess);
	*guess = NULL;
    }
}

/*
 * type_name:
 *	Return a pointer to the name of the type
 */
char *
type_name(int type)
{
    struct h_list *hp;
    static struct h_list tlist[] = {
	POTION,	 "potion",		FALSE,
	SCROLL,	 "scroll",		FALSE,
	FOOD,	 "food",		FALSE,
	R_OR_S,	 "ring, wand or staff",	FALSE,
	RING,	 "ring",		FALSE,
	STICK,	 "wand or staff",	FALSE,
	WEAPON,	 "weapon",		FALSE,
	ARMOR,	 "suit of armor",	FALSE,
    };

    for (hp = tlist; hp->h_ch; hp++)
	if (type == hp->h_ch)
	    return hp->h_desc;
    /* NOTREACHED */
}

#ifdef MASTER
/*
 * create_obj:
 *	Wizard command for getting anything he wants
 */
void
create_obj(void)
{
    THING *obj;
    char ch, bless;

    obj = new_item();
    msg("type of item: ");
    obj->o_type = readchar();
    Mpos = 0;
    msg("which %c do you want? (0-f)", obj->o_type);
    obj->o_which = (isdigit((ch = readchar())) ? ch - '0' : ch - 'a' + 10);
    obj->o_group = 0;
    obj->o_count = 1;
    Mpos = 0;
    if (obj->o_type == WEAPON || obj->o_type == ARMOR)
    {
	msg("blessing? (+,-,n)");
	bless = readchar();
	Mpos = 0;
	if (bless == '-')
	    obj->o_flags |= ISCURSED;
	if (obj->o_type == WEAPON)
	{
	    init_weapon(obj, obj->o_which);
	    if (bless == '-')
		obj->o_hplus -= rnd(3)+1;
	    if (bless == '+')
		obj->o_hplus += rnd(3)+1;
	}
	else
	{
	    obj->o_arm = A_class[obj->o_which];
	    if (bless == '-')
		obj->o_arm += rnd(3)+1;
	    if (bless == '+')
		obj->o_arm -= rnd(3)+1;
	}
    }
    else if (obj->o_type == RING)
	switch (obj->o_which)
	{
	    case R_PROTECT:
	    case R_ADDSTR:
	    case R_ADDHIT:
	    case R_ADDDAM:
		msg("blessing? (+,-,n)");
		bless = readchar();
		Mpos = 0;
		if (bless == '-')
		    obj->o_flags |= ISCURSED;
		obj->o_arm = (bless == '-' ? -1 : rnd(2) + 1);
	    when R_AGGR:
	    case R_TELEPORT:
		obj->o_flags |= ISCURSED;
	}
    else if (obj->o_type == STICK)
	fix_stick(obj);
    else if (obj->o_type == GOLD)
    {
	msg("how much?");
	get_num(&obj->o_goldval, stdscr);
    }
    add_pack(obj, FALSE);
}
#endif

/*
 * telport:
 *	Bamf the hero someplace else
 */
void
teleport(void)
{
    int rm;
    static coord c;

    mvaddch(Hero.y, Hero.x, floor_at());
    find_floor((struct room *) NULL, &c, FALSE, TRUE);
    if (roomin(&c) != Proom)
    {
	leave_room(&Hero);
	Hero = c;
	enter_room(&Hero);
    }
    else
    {
	Hero = c;
	look(TRUE);
    }
    mvaddch(Hero.y, Hero.x, PLAYER);
    /*
     * turn off ISHELD in case teleportation was done while fighting
     * a Flytrap
     */
    if (on(Player, ISHELD)) {
	Player.t_flags &= ~ISHELD;
	Vf_hit = 0;
	strcpy(Monsters['F'-'A'].m_stats.s_dmg, "000x0");
    }
    No_move = 0;
    Count = 0;
    Running = FALSE;
    flush_type();
}

#ifdef MASTER
/*
 * passwd:
 *	See if user knows password
 */
bool
passwd(void)
{
    char *sp, c;
    static char buf[MAXSTR];
    char *crypt();

    msg("wizard's Password:");
    Mpos = 0;
    sp = buf;
    while ((c = getchar()) != '\n' && c != '\r' && c != ESCAPE)
	*sp++ = c;
    if (sp == buf)
	return FALSE;
    *sp = '\0';
    return (strcmp(PASSWD, crypt(buf, "mT")) == 0);
}

/*
 * show_map:
 *	Print out the map for the wizard
 */
void
show_map(void)
{
    int y, x, real;

    wclear(Hw);
    for (y = 1; y < NUMLINES - 1; y++)
	for (x = 0; x < NUMCOLS; x++)
	{
	    if (!(real = flat(y, x) & F_REAL))
		wstandout(Hw);
	    wmove(Hw, y, x);
	    waddch(Hw, chat(y, x));
	    if (!real)
		wstandend(Hw);
	}
    show_win("---More (level map)---");
}
#endif
