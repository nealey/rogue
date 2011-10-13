/*
 * Code for one creature to chase another
 *
 * @(#)netprot.c	4.57 (Berkeley) 02/05/99
 */

#include <curses.h>
#include "netprot.h"

#define DRAGONSHOT  5	/* one chance in DRAGONSHOT that a dragon will flame */

static coord Ch_ret;				/* Where chasing takes you */

/*
 * runners:
 *	Make all the running monsters move.
 */
void
runners(void)
{
    THING *tp;
    THING *next;
    bool wastarget;
    static coord orig_pos;

    for (tp = Mlist; tp != NULL; tp = next)
    {
	/* remember this in case the monster's "next" is changed */
	next = next(tp);

	if (!on(*tp, ISHELD) && on(*tp, ISRUN))
	{
	    orig_pos = tp->t_pos;
	    wastarget = on(*tp, ISTARGET);
	    move_monst(tp);
	    if (on(*tp, ISFLY) && dist_cp(&Hero, &tp->t_pos) >= 3)
		move_monst(tp);
	    if (wastarget && !ce(orig_pos, tp->t_pos))
	    {
		tp->t_flags &= ~ISTARGET;
		To_death = FALSE;
	    }
	}
    }
    if (Has_hit)
    {
	endmsg();
	Has_hit = FALSE;
    }
}

/*
 * move_monst:
 *	Execute a single turn of running for a monster
 */
void
move_monst(THING *tp)
{
    if (!on(*tp, ISSLOW) || tp->t_turn)
	do_chase(tp);
    if (on(*tp, ISHASTE))
	do_chase(tp);
    tp->t_turn ^= TRUE;
}

/*
 * relocate:
 *	Make the monster's new location be the specified one, updating
 *	all the relevant state.
 */
void
relocate(THING *th, coord *new_loc)
{
    struct room *oroom;

    if (!ce(*new_loc, th->t_pos))
    {
	mvaddch(th->t_pos.y, th->t_pos.x, th->t_oldch);
	th->t_room = roomin(new_loc);
	set_oldch(th, new_loc);
	oroom = th->t_room;
	moat(th->t_pos.y, th->t_pos.x) = NULL;

	if (oroom != th->t_room)
	    th->t_dest = find_dest(th);
	th->t_pos = *new_loc;
	moat(new_loc->y, new_loc->x) = th;
    }
    move(new_loc->y, new_loc->x);
    if (see_monst(th))
	addch(th->t_disguise);
    else if (on(Player, SEEMONST))
    {
	standout();
	addch(th->t_type);
	standend();
    }
}

/*
 * do_chase:
 *	Make one thing chase another.
 */
void
do_chase(THING *th)
{
    coord *cp;
    struct room *rer, *ree;	/* room of chaser, room of chasee */
    int mindist = 32767, curdist;
    bool stoprun = FALSE;	/* TRUE means we are there */
    bool door;
    THING *obj;
    static coord this;			/* Temporary destination for chaser */

    rer = th->t_room;		/* Find room of chaser */
    if (on(*th, ISGREED) && rer->r_goldval == 0)
	th->t_dest = &Hero;	/* If gold has been taken, run after hero */
    if (th->t_dest == &Hero)	/* Find room of chasee */
	ree = Proom;
    else
	ree = roomin(th->t_dest);
    /*
     * We don't count doors as inside rooms for this routine
     */
    door = (chat(th->t_pos.y, th->t_pos.x) == DOOR);
    /*
     * If the object of our desire is in a different room,
     * and we are not in a corridor, run to the door nearest to
     * our goal.
     */
over:
    if (rer != ree)
    {
	for (cp = rer->r_exit; cp < &rer->r_exit[rer->r_nexits]; cp++)
	{
	    curdist = dist_cp(th->t_dest, cp);
	    if (curdist < mindist)
	    {
		this = *cp;
		mindist = curdist;
	    }
	}
	if (door)
	{
	    rer = &Passages[flat(th->t_pos.y, th->t_pos.x) & F_PNUM];
	    door = FALSE;
	    goto over;
	}
    }
    else
    {
	this = *th->t_dest;
	/*
	 * For dragons check and see if (a) the hero is on a straight
	 * line from it, and (b) that it is within shooting distance,
	 * but outside of striking range.
	 */
	if (th->t_type == 'D' && (th->t_pos.y == Hero.y || th->t_pos.x == Hero.x
	    || abs(th->t_pos.y - Hero.y) == abs(th->t_pos.x - Hero.x))
	    && dist_cp(&th->t_pos, &Hero) <= BOLT_LENGTH * BOLT_LENGTH
	    && !on(*th, ISCANC) && rnd(DRAGONSHOT) == 0)
	{
	    Delta.y = sign(Hero.y - th->t_pos.y);
	    Delta.x = sign(Hero.x - th->t_pos.x);
	    if (Has_hit)
		endmsg();
	    fire_bolt(&th->t_pos, &Delta, "flame");
	    Running = FALSE;
	    Count = 0;
	    Quiet = 0;
	    if (To_death && !on(*th, ISTARGET))
	    {
		To_death = FALSE;
		Kamikaze = FALSE;
	    }
	    return;
	}
    }
    /*
     * This now contains what we want to run to this time
     * so we run to it.  If we hit it we either want to fight it
     * or stop running
     */
    if (!chase(th, &this))
    {
	if (ce(this, Hero))
	{
	    attack(th);
	    return;
	}
	else if (ce(this, *th->t_dest))
	{
	    for (obj = Lvl_obj; obj != NULL; obj = next(obj))
		if (th->t_dest == &obj->o_pos)
		{
		    detach(Lvl_obj, obj);
		    attach(th->t_pack, obj);
		    chat(obj->o_pos.y, obj->o_pos.x) =
			(th->t_room->r_flags & ISGONE) ? PASSAGE : FLOOR;
		    th->t_dest = find_dest(th);
		    break;
		}
	    if (th->t_type != 'F')
		stoprun = TRUE;
	}
    }
    else
    {
	if (th->t_type == 'F')
	    return;
    }
    relocate(th, &Ch_ret);
    /*
     * And stop running if need be
     */
    if (stoprun && ce(th->t_pos, *(th->t_dest)))
	th->t_flags &= ~ISRUN;
}

/*
 * set_oldch:
 *	Set the oldch character for the monster
 */
void
set_oldch(THING *tp, coord *cp)
{
    char sch;

    if (ce(tp->t_pos, *cp))
	return;

    sch = tp->t_oldch;
    tp->t_oldch = mvinch(cp->y, cp->x);
    if (!on(Player, ISBLIND))
	if ((sch == FLOOR || tp->t_oldch == FLOOR) &&
	    (tp->t_room->r_flags & ISDARK))
		tp->t_oldch = ' ';
	else if (dist_cp(cp, &Hero) <= LAMPDIST && See_floor)
	    tp->t_oldch = chat(cp->y, cp->x);
}

/*
 * see_monst:
 *	Return TRUE if the hero can see the monster
 */
bool
see_monst(THING *mp)
{
    int y, x;

    if (on(Player, ISBLIND))
	return FALSE;
    if (on(*mp, ISINVIS) && !on(Player, CANSEE))
	return FALSE;
    y = mp->t_pos.y;
    x = mp->t_pos.x;
    if (dist(y, x, Hero.y, Hero.x) < LAMPDIST)
    {
	if (y != Hero.y && x != Hero.x &&
	    !step_ok(chat(y, Hero.x)) && !step_ok(chat(Hero.y, x)))
		return FALSE;
	return TRUE;
    }
    if (mp->t_room != Proom)
	return FALSE;
    return (!(mp->t_room->r_flags & ISDARK));
}

/*
 * runto:
 *	Set a monster running after the hero.
 */
void
runto(coord *runner)
{
    THING *tp;

    /*
     * If we couldn't find him, something is funny
     */
#ifdef MASTER
    if ((tp = moat(runner->y, runner->x)) == NULL)
	msg("couldn't find monster in runto at (%d,%d)", runner->y, runner->x);
#else
    tp = moat(runner->y, runner->x);
#endif
    /*
     * Start the beastie running
     */
    tp->t_flags |= ISRUN;
    tp->t_flags &= ~ISHELD;
    tp->t_dest = find_dest(tp);
}

/*
 * chase:
 *	Find the spot for the chaser(er) to move closer to the
 *	chasee(ee).  Returns TRUE if we want to keep on chasing later
 *	FALSE if we reach the goal.
 */
bool
chase(THING *tp, coord *ee)
{
    THING *obj;
    int x, y;
    int curdist, thisdist;
    coord *er = &tp->t_pos;
    char ch;
    int plcnt = 1;
    static coord tryp;

    /*
     * If the thing is confused, let it move randomly. Invisible
     * Stalkers are slightly confused all of the time, and bats are
     * quite confused all the time
     */
    if ((on(*tp, ISHUH) && rnd(5) != 0) || (tp->t_type == 'P' && rnd(5) == 0)
	|| (tp->t_type == 'B' && rnd(2) == 0))
    {
	/*
	 * get a valid random move
	 */
	Ch_ret = *rndmove(tp);
	curdist = dist_cp(&Ch_ret, ee);
	/*
	 * Small chance that it will become un-confused 
	 */
	if (rnd(20) == 0)
	    tp->t_flags &= ~ISHUH;
    }
    /*
     * Otherwise, find the empty spot next to the chaser that is
     * closest to the chasee.
     */
    else
    {
	int ey, ex;
	/*
	 * This will eventually hold where we move to get closer
	 * If we can't find an empty spot, we stay where we are.
	 */
	curdist = dist_cp(er, ee);
	Ch_ret = *er;

	ey = er->y + 1;
	if (ey >= NUMLINES - 1)
	    ey = NUMLINES - 2;
	ex = er->x + 1;
	if (ex >= NUMCOLS)
	    ex = NUMCOLS - 1;

	for (x = er->x - 1; x <= ex; x++)
	{
	    if (x < 0)
		continue;
	    tryp.x = x;
	    for (y = er->y - 1; y <= ey; y++)
	    {
		tryp.y = y;
		if (!diag_ok(er, &tryp))
		    continue;
		ch = winat(y, x);
		if (step_ok(ch))
		{
		    /*
		     * If it is a scroll, it might be a scare monster scroll
		     * so we need to look it up to see what type it is.
		     */
		    if (ch == SCROLL)
		    {
			for (obj = Lvl_obj; obj != NULL; obj = next(obj))
			{
			    if (y == obj->o_pos.y && x == obj->o_pos.x)
				break;
			}
			if (obj != NULL && obj->o_which == S_SCARE)
			    continue;
		    }
		    /*
		     * It can also be a Xeroc, which we shouldn't step on
		     */
		    if ((obj = moat(y, x)) != NULL && obj->t_type == 'X')
			continue;
		    /*
		     * If we didn't find any scrolls at this place or it
		     * wasn't a scare scroll, then this place counts
		     */
		    thisdist = dist(y, x, ee->y, ee->x);
		    if (thisdist < curdist)
		    {
			plcnt = 1;
			Ch_ret = tryp;
			curdist = thisdist;
		    }
		    else if (thisdist == curdist && rnd(++plcnt) == 0)
		    {
			Ch_ret = tryp;
			curdist = thisdist;
		    }
		}
	    }
	}
    }
    return (curdist != 0 && !ce(Ch_ret, Hero));
}

/*
 * roomin:
 *	Find what room some coordinates are in. NULL means they aren't
 *	in any room.
 */
struct room *
roomin(coord *cp)
{
    struct room *rp;
    char *fp;


    fp = &flat(cp->y, cp->x);
    if (*fp & F_PASS)
	return &Passages[*fp & F_PNUM];

    for (rp = Rooms; rp < &Rooms[MAXROOMS]; rp++)
	if (cp->x <= rp->r_pos.x + rp->r_max.x && rp->r_pos.x <= cp->x
	 && cp->y <= rp->r_pos.y + rp->r_max.y && rp->r_pos.y <= cp->y)
	    return rp;

    msg("in some bizarre place (%d, %d)", unc(*cp));
#ifdef MASTER
    abort();
    /* NOTREACHED */
#else
    return NULL;
#endif
}

/*
 * diag_ok:
 *	Check to see if the move is legal if it is diagonal
 */
bool
diag_ok(coord *sp, coord *ep)
{
    if (ep->x < 0 || ep->x >= NUMCOLS || ep->y <= 0 || ep->y >= NUMLINES - 1)
	return FALSE;
    if (ep->x == sp->x || ep->y == sp->y)
	return TRUE;
    return (step_ok(chat(ep->y, sp->x)) && step_ok(chat(sp->y, ep->x)));
}

/*
 * cansee:
 *	Returns true if the hero can see a certain coordinate.
 */
bool
cansee(int y, int x)
{
    struct room *rer;
    static coord tp;

    if (on(Player, ISBLIND))
	return FALSE;
    if (dist(y, x, Hero.y, Hero.x) < LAMPDIST)
    {
	if (flat(y, x) & F_PASS)
	    if (y != Hero.y && x != Hero.x &&
		!step_ok(chat(y, Hero.x)) && !step_ok(chat(Hero.y, x)))
		    return FALSE;
	return TRUE;
    }
    /*
     * We can only see if the hero in the same room as
     * the coordinate and the room is lit or if it is close.
     */
    tp.y = y;
    tp.x = x;
    return ((rer = roomin(&tp)) == Proom && !(rer->r_flags & ISDARK));
}

/*
 * find_dest:
 *	find the proper destination for the monster
 */
coord *
find_dest(THING *tp)
{
    THING *obj;
    int prob;

    if ((prob = Monsters[tp->t_type - 'A'].m_carry) <= 0 || tp->t_room == Proom
	|| see_monst(tp))
	    return &Hero;
    for (obj = Lvl_obj; obj != NULL; obj = next(obj))
    {
	if (obj->o_type == SCROLL && obj->o_which == S_SCARE)
	    continue;
	if (roomin(&obj->o_pos) == tp->t_room && rnd(100) < prob)
	{
	    for (tp = Mlist; tp != NULL; tp = next(tp))
		if (tp->t_dest == &obj->o_pos)
		    break;
	    if (tp == NULL)
		return &obj->o_pos;
	}
    }
    return &Hero;
}

/*
 * dist:
 *	Calculate the "distance" between to points.  Actually,
 *	this calculates d^2, not d, but that's good enough for
 *	our purposes, since it's only used comparitively.
 */
int
dist(int y1, int x1, int y2, int x2)
{
    return ((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

/*
 * dist_cp:
 *	Call dist() with appropriate arguments for coord pointers
 */
int
dist_cp(coord *c1, coord *c2)
{
    return dist(c1->y, c1->x, c2->y, c2->x);
}
