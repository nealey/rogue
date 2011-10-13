/*
 * File with various monster functions in it
 *
 * @(#)rexec.c	4.46 (Berkeley) 02/05/99
 */

#include <curses.h>
#include "netprot.h"
#include <ctype.h>

/*
 * List of monsters in rough order of vorpalness
 */
static char Lvl_mons[] =  {
    'K', 'E', 'B', 'S', 'H', 'I', 'R', 'O', 'Z', 'L', 'C', 'Q', 'A',
    'N', 'Y', 'F', 'T', 'W', 'P', 'X', 'U', 'M', 'V', 'G', 'J', 'D'
};

static char Wand_mons[] = {
    'K', 'E', 'B', 'S', 'H',   0, 'R', 'O', 'Z',   0, 'C', 'Q', 'A',
      0, 'Y',   0, 'T', 'W', 'P',   0, 'U', 'M', 'V', 'G', 'J',   0
};

/*
 * randmonster:
 *	Pick a monster to show up.  The lower the level,
 *	the meaner the monster.
 */
char
randmonster(bool wander)
{
    int d;
    char *mons;

    mons = (wander ? Wand_mons : Lvl_mons);
    do
    {
	d = Level + (rnd(10) - 6);
	if (d < 0)
	    d = rnd(5);
	if (d > 25)
	    d = rnd(5) + 21;
    } while (mons[d] == 0);
    return mons[d];
}

/*
 * new_monster:
 *	Pick a new monster and add it to the list
 */
void
new_monster(THING *tp, char type, coord *cp)
{
    struct monster *mp;
    int lev_add;

    if ((lev_add = Level - AMULETLEVEL) < 0)
	lev_add = 0;
    attach(Mlist, tp);
    tp->t_type = type;
    tp->t_disguise = type;
    tp->t_pos = *cp;
    move(cp->y, cp->x);
    tp->t_oldch = inch();
    tp->t_room = roomin(cp);
    moat(cp->y, cp->x) = tp;
    mp = &Monsters[tp->t_type-'A'];
    tp->t_stats.s_lvl = mp->m_stats.s_lvl + lev_add;
    tp->t_stats.s_maxhp = tp->t_stats.s_hpt = roll(tp->t_stats.s_lvl, 8);
    tp->t_stats.s_arm = mp->m_stats.s_arm - lev_add;
    strcpy(tp->t_stats.s_dmg,mp->m_stats.s_dmg);
    tp->t_stats.s_str = mp->m_stats.s_str;
    tp->t_stats.s_exp = mp->m_stats.s_exp + lev_add * 10 + exp_add(tp);
    tp->t_flags = mp->m_flags;
    if (Level > 29)
	tp->t_flags |= ISHASTE;
    tp->t_turn = TRUE;
    tp->t_pack = NULL;
    if (ISWEARING(R_AGGR))
	runto(cp);
    if (type == 'X')
	tp->t_disguise = rnd_thing();
}

/*
 * expadd:
 *	Experience to add for this monster's level/hit points
 */
int
exp_add(THING *tp)
{
    int mod;

    if (tp->t_stats.s_lvl == 1)
	mod = tp->t_stats.s_maxhp / 8;
    else
	mod = tp->t_stats.s_maxhp / 6;
    if (tp->t_stats.s_lvl > 9)
	mod *= 20;
    else if (tp->t_stats.s_lvl > 6)
	mod *= 4;
    return mod;
}

/*
 * wanderer:
 *	Create a new wandering monster and aim it at the player
 */
void
wanderer(void)
{
    int i;
    struct room *rp;
    THING *tp;
    static coord cp;

    tp = new_item();
    do
    {
	find_floor((struct room *) NULL, &cp, FALSE, TRUE);
    } while (roomin(&cp) == Proom);
    new_monster(tp, randmonster(TRUE), &cp);
    if (on(Player, SEEMONST))
    {
	standout();
	if (!on(Player, ISHALU))
	    addch(tp->t_type);
	else
	    addch(rnd(26) + 'A');
	standend();
    }
    runto(&tp->t_pos);
#ifdef MASTER
    if (Wizard)
	msg("started a wandering %s", Monsters[tp->t_type-'A'].m_name);
#endif
}

/*
 * wake_monster:
 *	What to do when the hero steps next to a monster
 */
THING *
wake_monster(int y, int x)
{
    THING *tp;
    struct room *rp;
    char ch, *mname;

#ifdef MASTER
    if ((tp = moat(y, x)) == NULL)
	msg("can't find monster in wake_monster");
#else
    tp = moat(y, x);
    if (tp == NULL)
	endwin(), abort();
#endif
    ch = tp->t_type;
    /*
     * Every time he sees mean monster, it might start chasing him
     */
    if (!on(*tp, ISRUN) && rnd(3) != 0 && on(*tp, ISMEAN) && !on(*tp, ISHELD)
	&& !ISWEARING(R_STEALTH) && !on(Player, ISLEVIT))
    {
	tp->t_dest = &Hero;
	tp->t_flags |= ISRUN;
    }
    if (ch == 'M' && !on(Player, ISBLIND) && !on(Player, ISHALU)
	&& !on(*tp, ISFOUND) && !on(*tp, ISCANC) && on(*tp, ISRUN))
    {
        rp = Proom;
	if ((rp != NULL && !(rp->r_flags & ISDARK))
	    || dist(y, x, Hero.y, Hero.x) < LAMPDIST)
	{
	    tp->t_flags |= ISFOUND;
	    if (!save(VS_MAGIC))
	    {
		if (on(Player, ISHUH))
		    lengthen(unconfuse, spread(HUHDURATION));
		else
		    fuse(unconfuse, 0, spread(HUHDURATION), AFTER);
		Player.t_flags |= ISHUH;
		mname = set_mname(tp);
		addmsg("%s", mname);
		if (strcmp(mname, "it") != 0)
		    addmsg("'");
		msg("s gaze has confused you");
	    }
	}
    }
    /*
     * Let greedy ones guard gold
     */
    if (on(*tp, ISGREED) && !on(*tp, ISRUN))
    {
	tp->t_flags |= ISRUN;
	if (Proom->r_goldval)
	    tp->t_dest = &Proom->r_gold;
	else
	    tp->t_dest = &Hero;
    }
    return tp;
}

/*
 * give_pack:
 *	Give a pack to a monster if it deserves one
 */
void
give_pack(THING *tp)
{
    if (Level >= Max_level && rnd(100) < Monsters[tp->t_type-'A'].m_carry)
	attach(tp->t_pack, new_thing());
}

/*
 * save_throw:
 *	See if a creature save against something
 */
int
save_throw(int which, THING *tp)
{
    int need;

    need = 14 + which - tp->t_stats.s_lvl / 2;
    return (roll(1, 20) >= need);
}

/*
 * save:
 *	See if he saves against various nasty things
 */
int
save(int which)
{
    if (which == VS_MAGIC)
    {
	if (ISRING(LEFT, R_PROTECT))
	    which -= Cur_ring[LEFT]->o_arm;
	if (ISRING(RIGHT, R_PROTECT))
	    which -= Cur_ring[RIGHT]->o_arm;
    }
    return save_throw(which, &Player);
}
