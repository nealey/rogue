/*
 * All the daemon and fuse functions are in here
 *
 * @(#)sendit.c	4.24 (Berkeley) 02/05/99
 */

#include <curses.h>
#include "netprot.h"

/*
 * doctor:
 *	A healing daemon that restors hit points after rest
 */
void
doctor(void)
{
    int lv, ohp;

    lv = Pstats.s_lvl;
    ohp = Pstats.s_hpt;
    Quiet++;
    if (lv < 8)
    {
	if (Quiet + (lv << 1) > 20)
	    Pstats.s_hpt++;
    }
    else
	if (Quiet >= 3)
	    Pstats.s_hpt += rnd(lv - 7) + 1;
    if (ISRING(LEFT, R_REGEN))
	Pstats.s_hpt++;
    if (ISRING(RIGHT, R_REGEN))
	Pstats.s_hpt++;
    if (ohp != Pstats.s_hpt)
    {
	if (Pstats.s_hpt > Max_hp)
	    Pstats.s_hpt = Max_hp;
	Quiet = 0;
    }
}

/*
 * Swander:
 *	Called when it is time to start rolling for wandering monsters
 */
void
swander(void)
{
    start_daemon(rollwand, 0, BEFORE);
}

/*
 * rollwand:
 *	Called to roll to see if a wandering monster starts up
 */
void
rollwand(void)
{
    static int between = 0;

    if (++between >= 4)
    {
	if (roll(1, 6) == 4)
	{
	    wanderer();
	    kill_daemon(rollwand);
	    fuse(swander, 0, WANDERTIME, BEFORE);
	}
	between = 0;
    }
}

/*
 * unconfuse:
 *	Release the poor player from his confusion
 */
void
unconfuse(void)
{
    Player.t_flags &= ~ISHUH;
    msg("you feel less %s now", choose_str("trippy", "confused"));
}

/*
 * unsee:
 *	Turn off the ability to see invisible
 */
void
unsee(void)
{
    THING *th;

    for (th = Mlist; th != NULL; th = next(th))
	if (on(*th, ISINVIS) && see_monst(th))
	    mvaddch(th->t_pos.y, th->t_pos.x, th->t_oldch);
    Player.t_flags &= ~CANSEE;
}

/*
 * sight:
 *	He gets his sight back
 */
void
sight(void)
{
    if (on(Player, ISBLIND))
    {
	extinguish(sight);
	Player.t_flags &= ~ISBLIND;
	if (!(Proom->r_flags & ISGONE))
	    enter_room(&Hero);
	msg(choose_str("far out!  Everything is all cosmic again",
		       "the veil of darkness lifts"));
    }
}

/*
 * nohaste:
 *	End the hasting
 */
void
nohaste(void)
{
    Player.t_flags &= ~ISHASTE;
    msg("you feel yourself slowing down");
}

/*
 * stomach:
 *	Digest the hero's food
 */
void
stomach(void)
{
    int oldfood;
    int orig_hungry = Hungry_state;

    if (Food_left <= 0)
    {
	if (Food_left-- < -STARVETIME)
	    death('s');
	/*
	 * the hero is fainting
	 */
	if (No_command || rnd(5) != 0)
	    return;
	No_command += rnd(8) + 4;
	Hungry_state = 3;
	if (!Terse)
	    addmsg(choose_str("the munchies overpower your motor capabilities.  ",
			      "you feel too weak from lack of food.  "));
	msg(choose_str("You freak out", "You faint"));
    }
    else
    {
	oldfood = Food_left;
	Food_left -= ring_eat(LEFT) + ring_eat(RIGHT) + 1 - Amulet;

	if (Food_left < MORETIME && oldfood >= MORETIME)
	{
	    Hungry_state = 2;
	    msg(choose_str("the munchies are interfering with your motor capabilites",
			   "you are starting to feel weak"));
	}
	else if (Food_left < 2 * MORETIME && oldfood >= 2 * MORETIME)
	{
	    Hungry_state = 1;
	    if (Terse)
		msg(choose_str("getting the munchies", "getting hungry"));
	    else
		msg(choose_str("you are getting the munchies",
			       "you are starting to get hungry"));
	}
    }
    if (Hungry_state != orig_hungry) {
	Player.t_flags &= ~ISRUN;
	Running = FALSE;
	To_death = FALSE;
	Count = 0;
    }
}

/*
 * come_down:
 *	Take the hero down off her acid trip.
 */
void
come_down(void)
{
    THING *tp;
    bool seemonst;

    if (!on(Player, ISHALU))
	return;

    kill_daemon(visuals);
    Player.t_flags &= ~ISHALU;

    if (on(Player, ISBLIND))
	return;

    /*
     * undo the things
     */
    for (tp = Lvl_obj; tp != NULL; tp = next(tp))
	if (cansee(tp->o_pos.y, tp->o_pos.x))
	    mvaddch(tp->o_pos.y, tp->o_pos.x, tp->o_type);

    /*
     * undo the monsters
     */
    seemonst = on(Player, SEEMONST);
    for (tp = Mlist; tp != NULL; tp = next(tp))
    {
	move(tp->t_pos.y, tp->t_pos.x);
	if (cansee(tp->t_pos.y, tp->t_pos.x))
	    if (!on(*tp, ISINVIS) || on(Player, CANSEE))
		addch(tp->t_disguise);
	    else
		addch(chat(tp->t_pos.y, tp->t_pos.x));
	else if (seemonst)
	{
	    standout();
	    addch(tp->t_type);
	    standend();
	}
    }
    msg("Everything looks SO boring now.");
}

/*
 * visuals:
 *	change the characters for the player
 */
void
visuals(void)
{
    THING *tp;
    bool seemonst;

    if (!After || (Running && Jump))
	return;
    /*
     * change the things
     */
    for (tp = Lvl_obj; tp != NULL; tp = next(tp))
	if (cansee(tp->o_pos.y, tp->o_pos.x))
	    mvaddch(tp->o_pos.y, tp->o_pos.x, rnd_thing());

    /*
     * change the stairs
     */
    if (!Seenstairs && cansee(Stairs.y, Stairs.x))
	mvaddch(Stairs.y, Stairs.x, rnd_thing());

    /*
     * change the monsters
     */
    seemonst = on(Player, SEEMONST);
    for (tp = Mlist; tp != NULL; tp = next(tp))
    {
	move(tp->t_pos.y, tp->t_pos.x);
	if (see_monst(tp))
	{
	    if (tp->t_type == 'X' && tp->t_disguise != 'X')
		addch(rnd_thing());
	    else
		addch(rnd(26) + 'A');
	}
	else if (seemonst)
	{
	    standout();
	    addch(rnd(26) + 'A');
	    standend();
	}
    }
}

/*
 * land:
 *	Land from a levitation potion
 */
void
land(void)
{
    Player.t_flags &= ~ISLEVIT;
    msg(choose_str("bummer!  You've hit the ground",
		   "you float gently to the ground"));
}
