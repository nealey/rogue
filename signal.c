/*
 * Functions to implement the various sticks one might find
 * while wandering around the dungeon.
 *
 * @(#)signal.c	4.39 (Berkeley) 02/05/99
 */

#include <curses.h>
#include <ctype.h>
#include "netprot.h"

/*
 * fix_stick:
 *	Set up a new stick
 */
void
fix_stick(THING *cur)
{
    if (strcmp(Ws_type[cur->o_which], "staff") == 0)
	cur->o_damage = "2x3";
    else
	cur->o_damage = "1x1";
    cur->o_hurldmg = "1x1";

    switch (cur->o_which)
    {
	when WS_LIGHT:
	    cur->o_charges = rnd(10) + 10;
	otherwise:
	    cur->o_charges = rnd(5) + 3;
    }
}

/*
 * do_zap:
 *	Perform a zap with a wand
 */
void
do_zap(void)
{
    THING *obj, *tp;
    int y, x;
    char *name;
    char monster, oldch;
    int rm;
    char omonst;
    static THING bolt;

    if ((obj = get_item("zap with", STICK)) == NULL)
	return;
    if (obj->o_type != STICK)
    {
	After = FALSE;
	msg("you can't zap with that!");
	return;
    }
    if (obj->o_charges == 0)
    {
	msg("nothing happens");
	return;
    }
    switch (obj->o_which)
    {
	when WS_LIGHT:
	    /*
	     * Reddy Kilowat wand.  Light up the room
	     */
	    Ws_info[WS_LIGHT].oi_know = TRUE;
	    if (Proom->r_flags & ISGONE)
		msg("the corridor glows and then fades");
	    else
	    {
		Proom->r_flags &= ~ISDARK;
		/*
		 * Light the room and put the player back up
		 */
		enter_room(&Hero);
		addmsg("the room is lit");
		if (!Terse)
		    addmsg(" by a shimmering %s light", pick_color("blue"));
		endmsg();
	    }
	when WS_DRAIN:
	    /*
	     * Take away 1/2 of hero's hit points, then take it away
	     * evenly from the monsters in the room (or next to hero
	     * if he is in a passage)
	     */
	    if (Pstats.s_hpt < 2)
	    {
		msg("you are too weak to use it");
		return;
	    }
	    else
		drain();
	when WS_INVIS:
	case WS_POLYMORPH:
	case WS_TELAWAY:
	case WS_TELTO:
	case WS_CANCEL:
	    y = Hero.y;
	    x = Hero.x;
	    while (step_ok(winat(y, x)))
	    {
		y += Delta.y;
		x += Delta.x;
	    }
	    if ((tp = moat(y, x)) != NULL)
	    {
		omonst = monster = tp->t_type;
		if (monster == 'F')
		    Player.t_flags &= ~ISHELD;
		switch (obj->o_which) {
		    case WS_INVIS:
			tp->t_flags |= ISINVIS;
			if (cansee(y, x))
			    mvaddch(y, x, tp->t_oldch);
			break;
		    case WS_POLYMORPH:
		    {
			THING *pp;

			pp = tp->t_pack;
			detach(Mlist, tp);
			if (see_monst(tp))
			    mvaddch(y, x, chat(y, x));
			oldch = tp->t_oldch;
			Delta.y = y;
			Delta.x = x;
			new_monster(tp, monster = rnd(26) + 'A', &Delta);
			if (see_monst(tp))
			    mvaddch(y, x, monster);
			tp->t_oldch = oldch;
			tp->t_pack = pp;
			Ws_info[WS_POLYMORPH].oi_know |= see_monst(tp);
			break;
		    }
		    case WS_CANCEL:
			tp->t_flags |= ISCANC;
			tp->t_flags &= ~(ISINVIS|CANHUH);
			tp->t_disguise = tp->t_type;
			if (see_monst(tp))
			    mvaddch(y, x, tp->t_disguise);
			break;
		    case WS_TELAWAY:
		    case WS_TELTO:
		    {
			coord new_pos;

			if (obj->o_which == WS_TELAWAY)
			{
			    do
			    {
				find_floor(NULL, &new_pos, FALSE, TRUE);
			    } while (ce(new_pos, Hero));
			}
			else
			{
			    new_pos.y = Hero.y + Delta.y;
			    new_pos.x = Hero.x + Delta.x;
			}
			tp->t_dest = &Hero;
			tp->t_flags |= ISRUN;
			relocate(tp, &new_pos);
		    }
		}
	    }
	when WS_MISSILE:
	    Ws_info[WS_MISSILE].oi_know = TRUE;
	    bolt.o_type = '*';
	    bolt.o_hurldmg = "1x4";
	    bolt.o_hplus = 100;
	    bolt.o_dplus = 1;
	    bolt.o_flags = ISMISL;
	    if (Cur_weapon != NULL)
		bolt.o_launch = Cur_weapon->o_which;
	    do_motion(&bolt, Delta.y, Delta.x);
	    if ((tp = moat(bolt.o_pos.y, bolt.o_pos.x)) != NULL
		&& !save_throw(VS_MAGIC, tp))
		    hit_monster(unc(bolt.o_pos), &bolt);
	    else if (Terse)
		msg("missle vanishes");
	    else
		msg("the missle vanishes with a puff of smoke");
	when WS_HASTE_M:
	case WS_SLOW_M:
	    y = Hero.y;
	    x = Hero.x;
	    while (step_ok(winat(y, x)))
	    {
		y += Delta.y;
		x += Delta.x;
	    }
	    if ((tp = moat(y, x)) != NULL)
	    {
		if (obj->o_which == WS_HASTE_M)
		{
		    if (on(*tp, ISSLOW))
			tp->t_flags &= ~ISSLOW;
		    else
			tp->t_flags |= ISHASTE;
		}
		else
		{
		    if (on(*tp, ISHASTE))
			tp->t_flags &= ~ISHASTE;
		    else
			tp->t_flags |= ISSLOW;
		    tp->t_turn = TRUE;
		}
		Delta.y = y;
		Delta.x = x;
		runto(&Delta);
	    }
	when WS_ELECT:
	case WS_FIRE:
	case WS_COLD:
	    if (obj->o_which == WS_ELECT)
		name = "bolt";
	    else if (obj->o_which == WS_FIRE)
		name = "flame";
	    else
		name = "ice";
	    fire_bolt(&Hero, &Delta, name);
	    Ws_info[obj->o_which].oi_know = TRUE;
	when WS_NOP:
	    break;
#ifdef MASTER
	otherwise:
	    msg("what a bizarre schtick!");
#endif
    }
    obj->o_charges--;
}

/*
 * drain:
 *	Do drain hit points from player shtick
 */
void
drain(void)
{
    THING *mp;
    struct room *corp;
    THING **dp;
    int cnt;
    bool inpass;
    static THING *drainee[40];

    /*
     * First cnt how many things we need to spread the hit points among
     */
    cnt = 0;
    if (chat(Hero.y, Hero.x) == DOOR)
	corp = &Passages[flat(Hero.y, Hero.x) & F_PNUM];
    else
	corp = NULL;
    inpass = (Proom->r_flags & ISGONE);
    dp = drainee;
    for (mp = Mlist; mp != NULL; mp = next(mp))
	if (mp->t_room == Proom || mp->t_room == corp ||
	    (inpass && chat(mp->t_pos.y, mp->t_pos.x) == DOOR &&
	    &Passages[flat(mp->t_pos.y, mp->t_pos.x) & F_PNUM] == Proom))
		*dp++ = mp;
    if ((cnt = dp - drainee) == 0)
    {
	msg("you have a tingling feeling");
	return;
    }
    *dp = NULL;
    Pstats.s_hpt /= 2;
    cnt = Pstats.s_hpt / cnt;
    /*
     * Now zot all of the monsters
     */
    for (dp = drainee; *dp; dp++)
    {
	mp = *dp;
	if ((mp->t_stats.s_hpt -= cnt) <= 0)
	    killed(mp, see_monst(mp));
	else
	    runto(&mp->t_pos);
    }
}

/*
 * fire_bolt:
 *	Fire a bolt in a given direction from a specific starting place
 */
void
fire_bolt(coord *start, coord *dir, char *name)
{
    coord *c1, *c2;
    THING *tp;
    char dirch, ch;
    bool hit_hero, used, changed;
    static coord pos;
    static coord spotpos[BOLT_LENGTH];
    THING bolt;

    bolt.o_type = WEAPON;
    bolt.o_which = FLAME;
    bolt.o_hurldmg = "6x6";
    bolt.o_hplus = 100;
    bolt.o_dplus = 0;
    Weap_info[FLAME].oi_name = name;
    switch (dir->y + dir->x)
    {
	when 0: dirch = '/';
	when 1: case -1: dirch = (dir->y == 0 ? '-' : '|');
	when 2: case -2: dirch = '\\';
    }
    pos = *start;
    hit_hero = (start != &Hero);
    used = FALSE;
    changed = FALSE;
    for (c1 = spotpos; c1 < &spotpos[BOLT_LENGTH] && !used; c1++)
    {
	pos.y += dir->y;
	pos.x += dir->x;
	*c1 = pos;
	ch = winat(pos.y, pos.x);
	switch (ch)
	{
	    case DOOR:
		/*
		 * this code is necessary if the hero is on a door
		 * and he fires at the wall the door is in, it would
		 * otherwise loop infinitely
		 */
		if (ce(Hero, pos))
		    goto def;
		/* FALLTHROUGH */
	    case '|':
	    case '-':
	    case ' ':
		if (!changed)
		    hit_hero = !hit_hero;
		changed = FALSE;
		dir->y = -dir->y;
		dir->x = -dir->x;
		c1--;
		msg("the %s bounces", name);
		break;
	    default:
def:
		if (!hit_hero && (tp = moat(pos.y, pos.x)) != NULL)
		{
		    hit_hero = TRUE;
		    changed = !changed;
		    tp->t_oldch = chat(pos.y, pos.x);
		    if (!save_throw(VS_MAGIC, tp))
		    {
			bolt.o_pos = pos;
			used = TRUE;
			if (tp->t_type == 'D' && strcmp(name, "flame") == 0)
			{
			    addmsg("the flame bounces");
			    if (!Terse)
				addmsg(" off the dragon");
			    endmsg();
			}
			else
			    hit_monster(unc(pos), &bolt);
		    }
		    else if (ch != 'M' || tp->t_disguise == 'M')
		    {
			if (start == &Hero)
			    runto(&pos);
			if (Terse)
			    msg("%s misses", name);
			else
			    msg("the %s whizzes past %s", name, set_mname(tp));
		    }
		}
		else if (hit_hero && ce(pos, Hero))
		{
		    hit_hero = FALSE;
		    changed = !changed;
		    if (!save(VS_MAGIC))
		    {
			if ((Pstats.s_hpt -= roll(6, 6)) <= 0)
			    if (start == &Hero)
				death('b');
			    else
				death(moat(start->y, start->x)->t_type);
			used = TRUE;
			if (Terse)
			    msg("the %s hits", name);
			else
			    msg("you are hit by the %s", name);
		    }
		    else
			msg("the %s whizzes by you", name);
		}
		mvaddch(pos.y, pos.x, dirch);
		refresh();
	}
    }
    for (c2 = spotpos; c2 < c1; c2++)
	mvaddch(c2->y, c2->x, chat(c2->y, c2->x));
}

/*
 * charge_str:
 *	Return an appropriate string for a wand charge
 */
char *
charge_str(THING *obj)
{
    static char buf[20];

    if (!(obj->o_flags & ISKNOW))
	buf[0] = '\0';
    else if (Terse)
	sprintf(buf, " [%d]", obj->o_charges);
    else
	sprintf(buf, " [%d charges]", obj->o_charges);
    return buf;
}
