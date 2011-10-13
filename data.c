/*
 * Read a scroll and let it happen
 *
 * @(#)data.c	4.44 (Berkeley) 02/05/99
 */

#include <curses.h>
#include <ctype.h>
#include "netprot.h"

/*
 * read_scroll:
 *	Read a scroll from the pack and do the appropriate thing
 */
void
read_scroll(void)
{
    THING *obj;
    PLACE *pp;
    int y, x;
    char ch;
    int i;
    bool discardit = FALSE;
    struct room *cur_room;
    THING *orig_obj;
    static coord mp;

    obj = get_item("read", SCROLL);
    if (obj == NULL)
	return;
    if (obj->o_type != SCROLL)
    {
	if (!Terse)
	    msg("there is nothing on it to read");
	else
	    msg("nothing to read");
	return;
    }
    /*
     * Calculate the effect it has on the poor guy.
     */
    if (obj == Cur_weapon)
	Cur_weapon = NULL;
    /*
     * Get rid of the thing
     */
    discardit = (obj->o_count == 1);
    leave_pack(obj, FALSE, FALSE);
    orig_obj = obj;

    switch (obj->o_which)
    {
	when S_CONFUSE:
	    /*
	     * Scroll of monster confusion.  Give him that power.
	     */
	    Player.t_flags |= CANHUH;
	    msg("your hands begin to glow %s", pick_color("red"));
	when S_ARMOR:
	    if (Cur_armor != NULL)
	    {
		Cur_armor->o_arm--;
		Cur_armor->o_flags &= ~ISCURSED;
		msg("your armor glows %s for a moment", pick_color("silver"));
	    }
	when S_HOLD:
	    /*
	     * Hold monster scroll.  Stop all monsters within two spaces
	     * from chasing after the hero.
	     */

	    ch = 0;
	    for (x = Hero.x - 2; x <= Hero.x + 2; x++)
		if (x >= 0 && x < NUMCOLS)
		    for (y = Hero.y - 2; y <= Hero.y + 2; y++)
			if (y >= 0 && y <= NUMLINES - 1)
			    if ((obj = moat(y, x)) != NULL && on(*obj, ISRUN))
			    {
				obj->t_flags &= ~ISRUN;
				obj->t_flags |= ISHELD;
				ch++;
			    }
	    if (ch)
	    {
		addmsg("the monster");
		if (ch > 1)
		    addmsg("s around you");
		addmsg(" freeze");
		if (ch == 1)
		    addmsg("s");
		endmsg();
		Scr_info[S_HOLD].oi_know = TRUE;
	    }
	    else
		msg("you feel a strange sense of loss");
	when S_SLEEP:
	    /*
	     * Scroll which makes you fall asleep
	     */
	    Scr_info[S_SLEEP].oi_know = TRUE;
	    No_command += rnd(SLEEPTIME) + 4;
	    Player.t_flags &= ~ISRUN;
	    msg("you fall asleep");
	when S_CREATE:
	    /*
	     * Create a monster:
	     * First look in a circle around him, next try his room
	     * otherwise give up
	     */
	    i = 0;
	    for (y = Hero.y - 1; y <= Hero.y + 1; y++)
		for (x = Hero.x - 1; x <= Hero.x + 1; x++)
		    /*
		     * Don't put a monster in top of the player.
		     */
		    if (y == Hero.y && x == Hero.x)
			continue;
		    /*
		     * Or anything else nasty
		     */
		    else if (step_ok(ch = winat(y, x)))
			if (ch == SCROLL
			    && find_obj(y, x)->o_which == S_SCARE)
				continue;
			else if (rnd(++i) == 0)
			{
			    mp.y = y;
			    mp.x = x;
			}
	    if (i == 0)
		msg("you hear a faint cry of anguish in the distance");
	    else
	    {
		obj = new_item();
		new_monster(obj, randmonster(FALSE), &mp);
	    }
	when S_ID_POTION:
	case S_ID_SCROLL:
	case S_ID_WEAPON:
	case S_ID_ARMOR:
	case S_ID_R_OR_S:
	{
	    static char id_type[S_ID_R_OR_S + 1] =
		{ 0, 0, 0, 0, 0, POTION, SCROLL, WEAPON, ARMOR, R_OR_S };
	    /*
	     * Identify, let him figure something out
	     */
	    Scr_info[obj->o_which].oi_know = TRUE;
	    msg("this scroll is an %s scroll", Scr_info[obj->o_which].oi_name);
	    whatis(TRUE, id_type[obj->o_which]);
	}
	when S_MAP:
	    /*
	     * Scroll of magic mapping.
	     */
	    Scr_info[S_MAP].oi_know = TRUE;
	    msg("oh, now this scroll has a map on it");
	    /*
	     * Take all the things we want to keep hidden out of the window
	     */
	    for (y = 1; y < NUMLINES - 1; y++)
		for (x = 0; x < NUMCOLS; x++)
		{
		    pp = INDEX(y, x);
		    switch (ch = pp->p_ch)
		    {
			case DOOR:
			case STAIRS:
			    break;

			case '-':
			case '|':
			    if (!(pp->p_flags & F_REAL))
			    {
				ch = pp->p_ch = DOOR;
				pp->p_flags |= F_REAL;
			    }
			    break;

			case ' ':
			    if (pp->p_flags & F_REAL)
				goto def;
			    pp->p_flags |= F_REAL;
			    ch = pp->p_ch = PASSAGE;
			    /* FALLTHROUGH */

			case PASSAGE:
pass:
			    if (!(pp->p_flags & F_REAL))
				pp->p_ch = PASSAGE;
			    pp->p_flags |= (F_SEEN|F_REAL);
			    ch = PASSAGE;
			    break;

			case FLOOR:
			    if (pp->p_flags & F_REAL)
				ch = ' ';
			    else
			    {
				ch = TRAP;
				pp->p_ch = TRAP;
				pp->p_flags |= (F_SEEN|F_REAL);
			    }
			    break;

			default:
def:
			    if (pp->p_flags & F_PASS)
				goto pass;
			    ch = ' ';
			    break;
		    }
		    if (ch != ' ')
		    {
			if ((obj = pp->p_monst) != NULL)
			    obj->t_oldch = ch;
			if (obj == NULL || !on(Player, SEEMONST))
			    mvaddch(y, x, ch);
		    }
		}
	when S_FDET:
	    /*
	     * Potion of gold detection
	     */
	    ch = FALSE;
	    wclear(Hw);
	    for (obj = Lvl_obj; obj != NULL; obj = next(obj))
		if (obj->o_type == FOOD)
		{
		    ch = TRUE;
		    wmove(Hw, obj->o_pos.y, obj->o_pos.x);
		    waddch(Hw, FOOD);
		}
	    if (ch)
	    {
		Scr_info[S_FDET].oi_know = TRUE;
		show_win("Your nose tingles and you smell food.--More--");
	    }
	    else
		msg("your nose tingles");
	when S_TELEP:
	    /*
	     * Scroll of teleportation:
	     * Make him dissapear and reappear
	     */
	    {
		cur_room = Proom;
		teleport();
		if (cur_room != Proom)
		    Scr_info[S_TELEP].oi_know = TRUE;
	    }
	when S_ENCH:
	    if (Cur_weapon == NULL || Cur_weapon->o_type != WEAPON)
		msg("you feel a strange sense of loss");
	    else
	    {
		Cur_weapon->o_flags &= ~ISCURSED;
		if (rnd(2) == 0)
		    Cur_weapon->o_hplus++;
		else
		    Cur_weapon->o_dplus++;
		msg("your %s glows %s for a moment",
		    Weap_info[Cur_weapon->o_which].oi_name, pick_color("blue"));
	    }
	when S_SCARE:
	    /*
	     * Reading it is a mistake and produces laughter at her
	     * poor boo boo.
	     */
	    msg("you hear maniacal laughter in the distance");
	when S_REMOVE:
	    uncurse(Cur_armor);
	    uncurse(Cur_weapon);
	    uncurse(Cur_ring[LEFT]);
	    uncurse(Cur_ring[RIGHT]);
	    msg(choose_str("you feel in touch with the Universal Onenes",
			   "you feel as if somebody is watching over you"));
	when S_AGGR:
	    /*
	     * This scroll aggravates all the monsters on the current
	     * level and sets them running towards the hero
	     */
	    aggravate();
	    msg("you hear a high pitched humming noise");
	when S_PROTECT:
	    if (Cur_armor != NULL)
	    {
		Cur_armor->o_flags |= ISPROT;
		msg("your armor is covered by a shimmering %s shield",
		    pick_color("gold"));
	    }
	    else
		msg("you feel a strange sense of loss");
#ifdef MASTER
	otherwise:
	    msg("what a puzzling scroll!");
	    return;
#endif
    }
    obj = orig_obj;
    look(TRUE);	/* put the result of the scroll on the screen */
    status();

    call_it(&Scr_info[obj->o_which]);

    if (discardit)
	discard(obj);
}

/*
 * uncurse:
 *	Uncurse an item
 */
void
uncurse(THING *obj)
{
    if (obj != NULL)
	obj->o_flags &= ~ISCURSED;
}
