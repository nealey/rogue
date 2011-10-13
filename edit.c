/*
 * All sorts of miscellaneous routines
 *
 * @(#)edit.c	4.66 (Berkeley) 08/06/83
 */

#include <curses.h>
#include "netprot.h"
#include <ctype.h>

/*
 * look:
 *	A quick glance all around the player
 */
# undef	DEBUG

void
look(bool wakeup)
{
    int x, y;
    int ch;
    THING *tp;
    PLACE *pp;
    struct room *rp;
    int ey, ex;
    int passcount;
    char pfl, *fp, pch;
    int sy, sx, sumhero, diffhero;
# ifdef DEBUG
    static bool done = FALSE;

    if (done)
	return;
    done = TRUE;
# endif DEBUG
    passcount = 0;
    rp = Proom;
    if (!ce(Oldpos, Hero))
    {
	erase_lamp(&Oldpos, Oldrp);
	Oldpos = Hero;
	Oldrp = rp;
    }
    ey = Hero.y + 1;
    ex = Hero.x + 1;
    sx = Hero.x - 1;
    sy = Hero.y - 1;
    if (Door_stop && !Firstmove && Running)
    {
	sumhero = Hero.y + Hero.x;
	diffhero = Hero.y - Hero.x;
    }
    pp = INDEX(Hero.y, Hero.x);
    pch = pp->p_ch;
    pfl = pp->p_flags;

    for (y = sy; y <= ey; y++)
	if (y > 0 && y < NUMLINES - 1) for (x = sx; x <= ex; x++)
	{
	    if (x < 0 || x >= NUMCOLS)
		continue;
	    if (!on(Player, ISBLIND))
	    {
		if (y == Hero.y && x == Hero.x)
		    continue;
	    }

	    pp = INDEX(y, x);
	    ch = pp->p_ch;
	    if (ch == ' ')		/* nothing need be done with a ' ' */
		    continue;
	    fp = &pp->p_flags;
	    if (pch != DOOR && ch != DOOR)
		if ((pfl & F_PASS) != (*fp & F_PASS))
		    continue;
	    if (((*fp & F_PASS) || ch == DOOR) && 
		 ((pfl & F_PASS) || pch == DOOR))
	    {
		if (Hero.x != x && Hero.y != y &&
		    !step_ok(chat(y, Hero.x)) && !step_ok(chat(Hero.y, x)))
			continue;
	    }

	    if ((tp = pp->p_monst) == NULL)
		ch = trip_ch(y, x, ch);
	    else
		if (on(Player, SEEMONST) && on(*tp, ISINVIS))
		{
		    if (Door_stop && !Firstmove)
			Running = FALSE;
		    continue;
		}
		else
		{
		    if (wakeup)
			wake_monster(y, x);
		    if (see_monst(tp))
			if (on(Player, ISHALU))
			    ch = rnd(26) + 'A';
			else
			    ch = tp->t_disguise;
		}
	    if (on(Player, ISBLIND) && (y != Hero.y || x != Hero.x))
		continue;

	    move(y, x);

	    if ((Proom->r_flags & ISDARK) && !See_floor && ch == FLOOR)
		ch = ' ';

	    if (tp != NULL || ch != inch())
		addch(ch);

	    if (Door_stop && !Firstmove && Running)
	    {
		switch (Runch)
		{
		    when 'h':
			if (x == ex)
			    continue;
		    when 'j':
			if (y == sy)
			    continue;
		    when 'k':
			if (y == ey)
			    continue;
		    when 'l':
			if (x == sx)
			    continue;
		    when 'y':
			if ((y + x) - sumhero >= 1)
			    continue;
		    when 'u':
			if ((y - x) - diffhero >= 1)
			    continue;
		    when 'n':
			if ((y + x) - sumhero <= -1)
			    continue;
		    when 'b':
			if ((y - x) - diffhero <= -1)
			    continue;
		}
		switch (ch)
		{
		    case DOOR:
			if (x == Hero.x || y == Hero.y)
			    Running = FALSE;
			break;
		    case PASSAGE:
			if (x == Hero.x || y == Hero.y)
			    passcount++;
			break;
		    case FLOOR:
		    case '|':
		    case '-':
		    case ' ':
			break;
		    default:
			Running = FALSE;
			break;
		}
	    }
	}
    if (Door_stop && !Firstmove && passcount > 1)
	Running = FALSE;
    if (!Running || !Jump)
	mvaddch(Hero.y, Hero.x, PLAYER);
# ifdef DEBUG
    done = FALSE;
# endif DEBUG
}

/*
 * trip_ch:
 *	Return the character appropriate for this space, taking into
 *	account whether or not the player is tripping.
 */
int
trip_ch(int y, int x, char ch)
{
    if (on(Player, ISHALU) && After)
	switch (ch)
	{
	    case FLOOR:
	    case ' ':
	    case PASSAGE:
	    case '-':
	    case '|':
	    case DOOR:
	    case TRAP:
		break;
	    default:
		if (y != Stairs.y || x != Stairs.x || !Seenstairs)
		    ch = rnd_thing();
		break;
	}
    return ch;
}

/*
 * erase_lamp:
 *	Erase the area shown by a lamp in a dark room.
 */
void
erase_lamp(coord *pos, struct room *rp)
{
    int y, x, ey, sy, ex;

    if (!(See_floor && (rp->r_flags & (ISGONE|ISDARK)) == ISDARK
	&& !on(Player,ISBLIND)))
	    return;

    ey = pos->y + 1;
    ex = pos->x + 1;
    sy = pos->y - 1;
    for (x = pos->x - 1; x <= ex; x++)
	for (y = sy; y <= ey; y++)
	{
	    if (y == Hero.y && x == Hero.x)
		continue;
	    move(y, x);
	    if (inch() == FLOOR)
		addch(' ');
	}
}

/*
 * show_floor:
 *	Should we show the floor in her room at this time?
 */
bool
show_floor(void)
{
    if ((Proom->r_flags & (ISGONE|ISDARK)) == ISDARK && !on(Player, ISBLIND))
	return See_floor;
    else
	return TRUE;
}

/*
 * find_obj:
 *	Find the unclaimed object at y, x
 */
THING *
find_obj(int y, int x)
{
    THING *obj;

    for (obj = Lvl_obj; obj != NULL; obj = next(obj))
    {
	if (obj->o_pos.y == y && obj->o_pos.x == x)
		return obj;
    }
#ifdef MASTER
    msg(sprintf(Prbuf, "Non-object %d,%d", y, x));
    return NULL;
#else
    /* NOTREACHED */
#endif
}

/*
 * eat:
 *	She wants to eat something, so let her try
 */
void
eat(void)
{
    THING *obj;

    if ((obj = get_item("eat", FOOD)) == NULL)
	return;
    if (obj->o_type != FOOD)
    {
	if (!Terse)
	    msg("ugh, you would get ill if you ate that");
	else
	    msg("that's Inedible!");
	return;
    }
    if (Food_left < 0)
	Food_left = 0;
    if ((Food_left += HUNGERTIME - 200 + rnd(400)) > STOMACHSIZE)
	Food_left = STOMACHSIZE;
    Hungry_state = 0;
    if (obj == Cur_weapon)
	Cur_weapon = NULL;
    leave_pack(obj, FALSE, FALSE);
    if (obj->o_which == 1)
	msg("my, that was a yummy %s", Fruit);
    else
	if (rnd(100) > 70)
	{
	    Pstats.s_exp++;
	    msg("%s, this food tastes awful", choose_str("bummer", "yuk"));
	    check_level();
	}
	else
	    msg("%s, that tasted good", choose_str("oh, wow", "yum"));
}

/*
 * check_level:
 *	Check to see if the guy has gone up a level.
 */
void
check_level(void)
{
    int i, add, olevel;

    for (i = 0; E_levels[i] != 0; i++)
	if (E_levels[i] > Pstats.s_exp)
	    break;
    i++;
    olevel = Pstats.s_lvl;
    Pstats.s_lvl = i;
    if (i > olevel)
    {
	add = roll(i - olevel, 10);
	Max_hp += add;
	Pstats.s_hpt += add;
	msg("welcome to level %d", i);
    }
}

/*
 * chg_str:
 *	Used to modify the playes strength.  It keeps track of the
 *	highest it has been, just in case
 */
void
chg_str(int amt)
{
    auto str_t comp;

    if (amt == 0)
	return;
    add_str(&Pstats.s_str, amt);
    comp = Pstats.s_str;
    if (ISRING(LEFT, R_ADDSTR))
	add_str(&comp, -Cur_ring[LEFT]->o_arm);
    if (ISRING(RIGHT, R_ADDSTR))
	add_str(&comp, -Cur_ring[RIGHT]->o_arm);
    if (comp > Max_stats.s_str)
	Max_stats.s_str = comp;
}

/*
 * add_str:
 *	Perform the actual add, checking upper and lower bound limits
 */
add_str(str_t *sp, int amt)
{
    if ((*sp += amt) < 3)
	*sp = 3;
    else if (*sp > 31)
	*sp = 31;
}

/*
 * add_haste:
 *	Add a haste to the player
 */
bool
add_haste(bool potion)
{
    if (on(Player, ISHASTE))
    {
	No_command += rnd(8);
	Player.t_flags &= ~(ISRUN|ISHASTE);
	extinguish(nohaste);
	msg("you faint from exhaustion");
	return FALSE;
    }
    else
    {
	Player.t_flags |= ISHASTE;
	if (potion)
	    fuse(nohaste, 0, rnd(4)+4, AFTER);
	return TRUE;
    }
}

/*
 * aggravate:
 *	Aggravate all the monsters on this level
 */
void
aggravate(void)
{
    THING *mp;

    for (mp = Mlist; mp != NULL; mp = next(mp))
	runto(&mp->t_pos);
}

/*
 * vowelstr:
 *      For printfs: if string starts with a vowel, return "n" for an
 *	"an".
 */
char *
vowelstr(char *str)
{
    switch (*str)
    {
	case 'a': case 'A':
	case 'e': case 'E':
	case 'i': case 'I':
	case 'o': case 'O':
	case 'u': case 'U':
	    return "n";
	default:
	    return "";
    }
}

/* 
 * is_current:
 *	See if the object is one of the currently used items
 */
bool
is_current(THING *obj)
{
    if (obj == NULL)
	return FALSE;
    if (obj == Cur_armor || obj == Cur_weapon || obj == Cur_ring[LEFT]
	|| obj == Cur_ring[RIGHT])
    {
	if (!Terse)
	    addmsg("That's already ");
	msg("in use");
	return TRUE;
    }
    return FALSE;
}

/*
 * get_dir:
 *      Set up the direction co_ordinate for use in varios "prefix"
 *	commands
 */
bool
get_dir(void)
{
    char *prompt;
    bool gotit;
    static coord last_delt;

    if (Again && Last_dir != '\0')
    {
	Delta.y = last_delt.y;
	Delta.x = last_delt.x;
	Dir_ch = Last_dir;
    }
    else
    {
	if (!Terse)
	    msg(prompt = "which direction? ");
	else
	    prompt = "direction: ";
	do
	{
	    gotit = TRUE;
	    switch (Dir_ch = readchar())
	    {
		when 'h': case'H': Delta.y =  0; Delta.x = -1;
		when 'j': case'J': Delta.y =  1; Delta.x =  0;
		when 'k': case'K': Delta.y = -1; Delta.x =  0;
		when 'l': case'L': Delta.y =  0; Delta.x =  1;
		when 'y': case'Y': Delta.y = -1; Delta.x = -1;
		when 'u': case'U': Delta.y = -1; Delta.x =  1;
		when 'b': case'B': Delta.y =  1; Delta.x = -1;
		when 'n': case'N': Delta.y =  1; Delta.x =  1;
		when ESCAPE: Last_dir = '\0'; reset_last(); return FALSE;
		otherwise:
		    Mpos = 0;
		    msg(prompt);
		    gotit = FALSE;
	    }
	} until (gotit);
	if (isupper(Dir_ch))
	    Dir_ch = tolower(Dir_ch);
	Last_dir = Dir_ch;
	last_delt.y = Delta.y;
	last_delt.x = Delta.x;
    }
    if (on(Player, ISHUH) && rnd(5) == 0)
	do
	{
	    Delta.y = rnd(3) - 1;
	    Delta.x = rnd(3) - 1;
	} while (Delta.y == 0 && Delta.x == 0);
    Mpos = 0;
    return TRUE;
}

/*
 * sign:
 *	Return the sign of the number
 */
int
sign(int nm)
{
    if (nm < 0)
	return -1;
    else
	return (nm > 0);
}

/*
 * spread:
 *	Give a spread around a given number (+/- 20%)
 */
int
spread(int nm)
{
    return nm - nm / 20 + rnd(nm / 10);
}

/*
 * call_it:
 *	Call an object something after use.
 */
void
call_it(struct obj_info *info)
{
    if (info->oi_know)
    {
	if (info->oi_guess)
	{
	    free(info->oi_guess);
	    info->oi_guess = NULL;
	}
    }
    else if (!info->oi_guess)
    {
	msg(Terse ? "call it: " : "what do you want to call it? ");
	if (get_str(Prbuf, stdscr) == NORM)
	{
	    if (info->oi_guess != NULL)
		free(info->oi_guess);
	    info->oi_guess = malloc((unsigned int) strlen(Prbuf) + 1);
	    strcpy(info->oi_guess, Prbuf);
	}
    }
}

/*
 * rnd_thing:
 *	Pick a random thing appropriate for this level
 */
int
rnd_thing(void)
{
    int i;
    static char thing_list[] = {
	POTION, SCROLL, RING, STICK, FOOD, WEAPON, ARMOR, STAIRS, GOLD, AMULET
    };

    if (Level >= AMULETLEVEL)
        i = rnd(sizeof thing_list / sizeof (char));
    else
        i = rnd(sizeof thing_list / sizeof (char) - 1);
    return thing_list[i];
}

/*
 str str:
 *	Choose the first or second string depending on whether it the
 *	player is tripping
 */
char *
choose_str(char *ts, char *ns)
{
	return (on(Player, ISHALU) ? ts : ns);
}
