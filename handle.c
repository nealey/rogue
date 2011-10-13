#include <curses.h>
#include "netprot.h"

#define TREAS_ROOM 20	/* one chance in TREAS_ROOM for a treasure room */
#define MAXTREAS 10	/* maximum number of treasures in a treasure room */
#define MINTREAS 2	/* minimum number of treasures in a treasure room */

/*
 * new_level:
 *	Dig and draw a new level
 *
 * @(#)handle.c	4.38 (Berkeley) 02/05/99
 */
void
new_level(void)
{
    THING *tp;
    PLACE *pp;
    char *sp;
    int i;

    Player.t_flags &= ~ISHELD;	/* unhold when you go down just in case */
    if (Level > Max_level)
	Max_level = Level;
    /*
     * Clean things off from last level
     */
    for (pp = Places; pp < &Places[MAXCOLS*MAXLINES]; pp++)
    {
	pp->p_ch = ' ';
	pp->p_flags = F_REAL;
	pp->p_monst = NULL;
    }
    clear();
    /*
     * Free up the monsters on the last level
     */
    for (tp = Mlist; tp != NULL; tp = next(tp))
	free_list(tp->t_pack);
    free_list(Mlist);
    /*
     * Throw away stuff left on the previous level (if anything)
     */
    free_list(Lvl_obj);
    do_rooms();				/* Draw rooms */
    do_passages();			/* Draw passages */
    No_food++;
    put_things();			/* Place objects (if any) */
    /*
     * Place the traps
     */
    if (rnd(10) < Level)
    {
	Ntraps = rnd(Level / 4) + 1;
	if (Ntraps > MAXTRAPS)
	    Ntraps = MAXTRAPS;
	i = Ntraps;
	while (i--)
	{
	    /*
	     * not only wouldn't it be NICE to have traps in mazes
	     * (not that we care about being nice), since the trap
	     * number is stored where the passage number is, we
	     * can't actually do it.
	     */
	    do
	    {
		find_floor((struct room *) NULL, &Stairs, FALSE, FALSE);
	    } while (chat(Stairs.y, Stairs.x) != FLOOR);
	    sp = &flat(Stairs.y, Stairs.x);
	    *sp &= ~F_REAL;
	    *sp |= rnd(NTRAPS);
	}
    }
    /*
     * Place the staircase down.
     */
    find_floor((struct room *) NULL, &Stairs, FALSE, FALSE);
    chat(Stairs.y, Stairs.x) = STAIRS;
    Seenstairs = FALSE;

    for (tp = Mlist; tp != NULL; tp = next(tp))
	tp->t_room = roomin(&tp->t_pos);

    find_floor((struct room *) NULL, &Hero, FALSE, TRUE);
    enter_room(&Hero);
    mvaddch(Hero.y, Hero.x, PLAYER);
    if (on(Player, SEEMONST))
	turn_see(FALSE);
    if (on(Player, ISHALU))
	visuals();
}

/*
 * rnd_room:
 *	Pick a room that is really there
 */
int
rnd_room(void)
{
    int rm;

    do
    {
	rm = rnd(MAXROOMS);
    } while (Rooms[rm].r_flags & ISGONE);
    return rm;
}

/*
 * put_things:
 *	Put potions and scrolls on this level
 */
void
put_things(void)
{
    int i;
    THING *obj;

    /*
     * Once you have found the amulet, the only way to get new stuff is
     * go down into the dungeon.
     */
    if (Amulet && Level < Max_level)
	return;
    /*
     * check for treasure rooms, and if so, put it in.
     */
    if (rnd(TREAS_ROOM) == 0)
	treas_room();
    /*
     * Do MAXOBJ attempts to put things on a level
     */
    for (i = 0; i < MAXOBJ; i++)
	if (rnd(100) < 36)
	{
	    /*
	     * Pick a new object and link it in the list
	     */
	    obj = new_thing();
	    attach(Lvl_obj, obj);
	    /*
	     * Put it somewhere
	     */
	    find_floor((struct room *) NULL, &obj->o_pos, FALSE, FALSE);
	    chat(obj->o_pos.y, obj->o_pos.x) = obj->o_type;
	}
    /*
     * If he is really deep in the dungeon and he hasn't found the
     * amulet yet, put it somewhere on the ground
     */
    if (Level >= AMULETLEVEL && !Amulet)
    {
	obj = new_item();
	attach(Lvl_obj, obj);
	obj->o_hplus = 0;
	obj->o_dplus = 0;
	obj->o_damage = obj->o_hurldmg = "0x0";
	obj->o_arm = 11;
	obj->o_type = AMULET;
	/*
	 * Put it somewhere
	 */
	find_floor((struct room *) NULL, &obj->o_pos, FALSE, FALSE);
	chat(obj->o_pos.y, obj->o_pos.x) = AMULET;
    }
}

/*
 * treas_room:
 *	Add a treasure room
 */
#define MAXTRIES 10	/* max number of tries to put down a monster */

void
treas_room(void)
{
    int nm;
    THING *tp;
    struct room *rp;
    int spots, num_monst;
    static coord mp;

    rp = &Rooms[rnd_room()];
    spots = (rp->r_max.y - 2) * (rp->r_max.x - 2) - MINTREAS;
    if (spots > (MAXTREAS - MINTREAS))
	spots = (MAXTREAS - MINTREAS);
    num_monst = nm = rnd(spots) + MINTREAS;
    while (nm--)
    {
	find_floor(rp, &mp, 2 * MAXTRIES, FALSE);
	tp = new_thing();
	tp->o_pos = mp;
	attach(Lvl_obj, tp);
	chat(mp.y, mp.x) = tp->o_type;
    }

    /*
     * fill up room with monsters from the next level down
     */

    if ((nm = rnd(spots) + MINTREAS) < num_monst + 2)
	nm = num_monst + 2;
    spots = (rp->r_max.y - 2) * (rp->r_max.x - 2);
    if (nm > spots)
	nm = spots;
    Level++;
    while (nm--)
    {
	spots = 0;
	if (find_floor(rp, &mp, MAXTRIES, TRUE))
	{
	    tp = new_item();
	    new_monster(tp, randmonster(FALSE), &mp);
	    tp->t_flags |= ISMEAN;	/* no sloughers in THIS room */
	    give_pack(tp);
	}
    }
    Level--;
}
