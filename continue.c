/*
 * All the fighting gets done here
 *
 * @(#)continue.c	4.67 (Berkeley) 09/06/83
 */

#include <curses.h>
#include <ctype.h>
#include "netprot.h"

#define	EQSTR(a, b)	(strcmp(a, b) == 0)

char *H_names[] = {		/* strings for hitting */
	" scored an excellent hit on ",
	" hit ",
	" have injured ",
	" swing and hit ",
	" scored an excellent hit on ",
	" hit ",
	" has injured ",
	" swings and hits "
};

char *M_names[] = {		/* strings for missing */
	" miss",
	" swing and miss",
	" barely miss",
	" don't hit",
	" misses",
	" swings and misses",
	" barely misses",
	" doesn't hit",
};

/*
 * adjustments to hit probabilities due to strength
 */
static int Str_plus[] = {
    -7, -6, -5, -4, -3, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
    1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3,
};

/*
 * adjustments to damage done due to strength
 */
static int Add_dam[] = {
    -7, -6, -5, -4, -3, -2, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3,
    3, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6
};

/*
 * fight:
 *	The player attacks the monster.
 */
bool
fight(coord *mp, THING *weap, bool thrown)
{
    THING *tp;
    bool did_hit = TRUE;
    char *mname, ch;

    /*
     * Find the monster we want to fight
     */
#ifdef MASTER
    if ((tp = moat(mp->y, mp->x)) == NULL)
	debug("Fight what @ %d,%d", mp->y, mp->x);
#else
    tp = moat(mp->y, mp->x);
#endif
    /*
     * Since we are fighting, things are not quiet so no healing takes
     * place.
     */
    Count = 0;
    Quiet = 0;
    runto(mp);
    /*
     * Let him know it was really a xeroc (if it was one).
     */
    ch = '\0';
    if (tp->t_type == 'X' && tp->t_disguise != 'X' && !on(Player, ISBLIND))
    {
	tp->t_disguise = 'X';
	if (on(Player, ISHALU)) {
	    ch = rnd(26) + 'A';
	    mvaddch(tp->t_pos.y, tp->t_pos.x, ch);
	}
	msg(choose_str("heavy!  That's a nasty critter!",
		       "wait!  That's a xeroc!"));
	if (!thrown)
	    return FALSE;
    }
    mname = set_mname(tp);
    did_hit = FALSE;
    Has_hit = (Terse && !To_death);
    if (roll_em(&Player, tp, weap, thrown))
    {
	did_hit = FALSE;
	if (thrown)
	    thunk(weap, mname, Terse);
	else
	    hit((char *) NULL, mname, Terse);
	if (on(Player, CANHUH))
	{
	    did_hit = TRUE;
	    tp->t_flags |= ISHUH;
	    Player.t_flags &= ~CANHUH;
	    endmsg();
	    Has_hit = FALSE;
	    msg("your hands stop glowing %s", pick_color("red"));
	}
	if (tp->t_stats.s_hpt <= 0)
	    killed(tp, TRUE);
	else if (did_hit && !on(Player, ISBLIND))
	    msg("%s appears confused", mname);
	did_hit = TRUE;
    }
    else
	if (thrown)
	    bounce(weap, mname, Terse);
	else
	    miss((char *) NULL, mname, Terse);
    return did_hit;
}

/*
 * attack:
 *	The monster attacks the player
 */
void
attack(THING *mp)
{
    char *mname, ch;
    int oldhp;

    /*
     * Since this is an attack, stop running and any healing that was
     * going on at the time.
     */
    Running = FALSE;
    Count = 0;
    Quiet = 0;
    if (To_death && !on(*mp, ISTARGET))
    {
	To_death = FALSE;
	Kamikaze = FALSE;
    }
    if (mp->t_type == 'X' && mp->t_disguise != 'X' && !on(Player, ISBLIND))
    {
	mp->t_disguise = 'X';
	if (on(Player, ISHALU))
	    mvaddch(mp->t_pos.y, mp->t_pos.x, rnd(26) + 'A');
    }
    mname = set_mname(mp);
    oldhp = Pstats.s_hpt;
    if (roll_em(mp, &Player, (THING *) NULL, FALSE))
    {
	if (mp->t_type != 'I')
	{
	    if (Has_hit)
		addmsg(".  ");
	    hit(mname, (char *) NULL, FALSE);
	}
	else
	    if (Has_hit)
		endmsg();
	Has_hit = FALSE;
	if (Pstats.s_hpt <= 0)
	    death(mp->t_type);	/* Bye bye life ... */
	else if (!Kamikaze)
	{
	    oldhp -= Pstats.s_hpt;
	    if (oldhp > Max_hit)
		Max_hit = oldhp;
	    if (Pstats.s_hpt <= Max_hit)
		To_death = FALSE;
	}
	if (!on(*mp, ISCANC))
	    switch (mp->t_type)
	    {
		when 'A':
		    /*
		     * If an aquator hits, you can lose armor class.
		     */
		    rust_armor(Cur_armor);
		when 'I':
		    /*
		     * The ice monster freezes you
		     */
		    Player.t_flags &= ~ISRUN;
		    if (!No_command)
		    {
			addmsg("you are frozen");
			if (!Terse)
			    addmsg(" by the %s", mname);
			endmsg();
		    }
		    No_command += rnd(2) + 2;
		    if (No_command > BORE_LEVEL)
			death('h');
		when 'R':
		    /*
		     * Rattlesnakes have poisonous bites
		     */
		    if (!save(VS_POISON))
			if (!ISWEARING(R_SUSTSTR))
			{
			    chg_str(-1);
			    if (!Terse)
				msg("you feel a bite in your leg and now feel weaker");
			    else
				msg("a bite has weakened you");
			}
			else if (!To_death)
			    if (!Terse)
				msg("a bite momentarily weakens you");
			    else
				msg("bite has no effect");
		when 'W':
		case 'V':
		    /*
		     * Wraiths might drain energy levels, and Vampires
		     * can steal Max_hp
		     */
		    if (rnd(100) < (mp->t_type == 'W' ? 15 : 30))
		    {
			int fewer;

			if (mp->t_type == 'W')
			{
			    if (Pstats.s_exp == 0)
				death('W');		/* All levels gone */
			    if (--Pstats.s_lvl == 0)
			    {
				Pstats.s_exp = 0;
				Pstats.s_lvl = 1;
			    }
			    else
				Pstats.s_exp = E_levels[Pstats.s_lvl-1]+1;
			    fewer = roll(1, 10);
			}
			else
			    fewer = roll(1, 3);
			Pstats.s_hpt -= fewer;
			Max_hp -= fewer;
			if (Pstats.s_hpt <= 0)
			    Pstats.s_hpt = 1;
			if (Max_hp <= 0)
			    death(mp->t_type);
			msg("you suddenly feel weaker");
		    }
		when 'F':
		    /*
		     * Venus Flytrap stops the poor guy from moving
		     */
		    Player.t_flags |= ISHELD;
		    sprintf(Monsters['F'-'A'].m_stats.s_dmg,"%dx1", ++Vf_hit);
		    if (--Pstats.s_hpt <= 0)
			death('F');
		when 'L':
		{
		    /*
		     * Leperachaun steals some gold
		     */
		    long lastpurse;

		    lastpurse = Purse;
		    Purse -= GOLDCALC;
		    if (!save(VS_MAGIC))
			Purse -= GOLDCALC + GOLDCALC + GOLDCALC + GOLDCALC;
		    if (Purse < 0)
			Purse = 0;
		    remove_mon(&mp->t_pos, mp, FALSE);
		    if (Purse != lastpurse)
			msg("your purse feels lighter");
		}
		when 'N':
		{
		    THING *obj, *steal;
		    int nobj;

		    /*
		     * Nymph's steal a magic item, look through the pack
		     * and pick out one we like.
		     */
		    steal = NULL;
		    for (nobj = 0, obj = Pack; obj != NULL; obj = next(obj))
			if (obj != Cur_armor && obj != Cur_weapon
			    && obj != Cur_ring[LEFT] && obj != Cur_ring[RIGHT]
			    && is_magic(obj) && rnd(++nobj) == 0)
				steal = obj;
		    if (steal != NULL)
		    {
			remove_mon(&mp->t_pos, moat(mp->t_pos.y, mp->t_pos.x), FALSE);
			leave_pack(steal, FALSE, FALSE);
			msg("she stole %s!", inv_name(steal, TRUE));
			discard(steal);
		    }
		}
		otherwise:
		    break;
	    }
    }
    else if (mp->t_type != 'I')
    {
	if (Has_hit)
	{
	    addmsg(".  ");
	    Has_hit = FALSE;
	}
	if (mp->t_type == 'F')
	{
	    Pstats.s_hpt -= Vf_hit;
	    if (Pstats.s_hpt <= 0)
		death(mp->t_type);	/* Bye bye life ... */
	}
	miss(mname, (char *) NULL, FALSE);
    }
    if (Fight_flush && !To_death)
	flush_type();
    Count = 0;
    status();
}

/*
 * set_mname:
 *	return the monster name for the given monster
 */
char *
set_mname(THING *tp)
{
    int ch;
    char *mname;
    static char tbuf[MAXSTR] = { 't', 'h', 'e', ' ' };

    if (!see_monst(tp) && !on(Player, SEEMONST))
	return (Terse ? "it" : "something");
    else if (on(Player, ISHALU))
    {
	move(tp->t_pos.y, tp->t_pos.x);
	ch = toascii(inch());
	if (!isupper(ch))
	    ch = rnd(26);
	else
	    ch -= 'A';
	mname = Monsters[ch].m_name;
    }
    else
	mname = Monsters[tp->t_type - 'A'].m_name;
    strcpy(&tbuf[4], mname);
    return tbuf;
}

/*
 * swing:
 *	Returns true if the swing hits
 */
bool
swing(int at_lvl, int op_arm, int wplus)
{
    int res = rnd(20);
    int need = (20 - at_lvl) - op_arm;

    return (res + wplus >= need);
}

/*
 * roll_em:
 *	Roll several attacks
 */
bool
roll_em(THING *thatt, THING *thdef, THING *weap, bool hurl)
{
    struct stats *att, *def;
    char *cp;
    int ndice, nsides, def_arm;
    bool did_hit = FALSE;
    int hplus;
    int dplus;
    int damage;
    char *index();

    att = &thatt->t_stats;
    def = &thdef->t_stats;
    if (weap == NULL)
    {
	cp = att->s_dmg;
	dplus = 0;
	hplus = 0;
    }
    else
    {
	hplus = (weap == NULL ? 0 : weap->o_hplus);
	dplus = (weap == NULL ? 0 : weap->o_dplus);
	if (weap == Cur_weapon)
	{
	    if (ISRING(LEFT, R_ADDDAM))
		dplus += Cur_ring[LEFT]->o_arm;
	    else if (ISRING(LEFT, R_ADDHIT))
		hplus += Cur_ring[LEFT]->o_arm;
	    if (ISRING(RIGHT, R_ADDDAM))
		dplus += Cur_ring[RIGHT]->o_arm;
	    else if (ISRING(RIGHT, R_ADDHIT))
		hplus += Cur_ring[RIGHT]->o_arm;
	}
	cp = weap->o_damage;
	if (hurl)
	    if ((weap->o_flags&ISMISL) && Cur_weapon != NULL &&
	      Cur_weapon->o_which == weap->o_launch)
	    {
		cp = weap->o_hurldmg;
		hplus += Cur_weapon->o_hplus;
		dplus += Cur_weapon->o_dplus;
	    }
	    else if (weap->o_launch < 0)
		cp = weap->o_hurldmg;
    }
    /*
     * If the creature being attacked is not running (alseep or held)
     * then the attacker gets a plus four bonus to hit.
     */
    if (!on(*thdef, ISRUN))
	hplus += 4;
    def_arm = def->s_arm;
    if (def == &Pstats)
    {
	if (Cur_armor != NULL)
	    def_arm = Cur_armor->o_arm;
	if (ISRING(LEFT, R_PROTECT))
	    def_arm -= Cur_ring[LEFT]->o_arm;
	if (ISRING(RIGHT, R_PROTECT))
	    def_arm -= Cur_ring[RIGHT]->o_arm;
    }
    while (cp != NULL && *cp != '\0')
    {
	ndice = atoi(cp);
	if ((cp = index(cp, 'x')) == NULL)
	    break;
	nsides = atoi(++cp);
	if (swing(att->s_lvl, def_arm, hplus + Str_plus[att->s_str]))
	{
	    int proll;

	    proll = roll(ndice, nsides);
#ifdef MASTER
	    if (ndice + nsides > 0 && proll <= 0)
		debug("Damage for %dx%d came out %d, dplus = %d, Add_dam = %d, def_arm = %d", ndice, nsides, proll, dplus, Add_dam[att->s_str], def_arm);
#endif
	    damage = dplus + proll + Add_dam[att->s_str];
	    def->s_hpt -= max(0, damage);
	    did_hit = TRUE;
	}
	if ((cp = index(cp, '/')) == NULL)
	    break;
	cp++;
    }
    return did_hit;
}

/*
 * prname:
 *	The print name of a combatant
 */
char *
prname(char *mname, bool upper)
{
    static char tbuf[MAXSTR];

    *tbuf = '\0';
    if (mname == 0)
	strcpy(tbuf, "you"); 
    else
	strcpy(tbuf, mname);
    if (upper)
	*tbuf = toupper(*tbuf);
    return tbuf;
}

/*
 * thunk:
 *	A missile hits a monster
 */
void
thunk(THING *weap, char *mname, bool noend)
{
    if (To_death)
	return;
    if (weap->o_type == WEAPON)
	addmsg("the %s hits ", Weap_info[weap->o_which].oi_name);
    else
	addmsg("you hit ");
    addmsg("%s", mname);
    if (!noend)
	endmsg();
}

/*
 * hit:
 *	Print a message to indicate a succesful hit
 */
void
hit(char *er, char *ee, bool noend)
{
    int i;
    char *s;
    extern char *H_names[];

    if (To_death)
	return;
    addmsg(prname(er, TRUE));
    if (Terse)
	s = " hit";
    else
    {
	i = rnd(4);
	if (er != NULL)
	    i += 4;
	s = H_names[i];
    }
    addmsg(s);
    if (!Terse)
	addmsg(prname(ee, FALSE));
    if (!noend)
	endmsg();
}

/*
 * miss:
 *	Print a message to indicate a poor swing
 */
void
miss(char *er, char *ee, bool noend)
{
    int i;
    extern char *M_names[];

    if (To_death)
	return;
    addmsg(prname(er, TRUE));
    if (Terse)
	i = 0;
    else
	i = rnd(4);
    if (er != NULL)
	i += 4;
    addmsg(M_names[i]);
    if (!Terse)
	addmsg(" %s", prname(ee, FALSE));
    if (!noend)
	endmsg();
}

/*
 * bounce:
 *	A missile misses a monster
 */
void
bounce(THING *weap, char *mname, bool noend)
{
    if (To_death)
	return;
    if (weap->o_type == WEAPON)
	addmsg("the %s misses ", Weap_info[weap->o_which].oi_name);
    else
	addmsg("you missed ");
    addmsg(mname);
    if (!noend)
	endmsg();
}

/*
 * remove_mon:
 *	Remove a monster from the screen
 */
void
remove_mon(coord *mp, THING *tp, bool waskill)
{
    THING *obj, *nexti;

    for (obj = tp->t_pack; obj != NULL; obj = nexti)
    {
	nexti = next(obj);
	obj->o_pos = tp->t_pos;
	detach(tp->t_pack, obj);
	if (waskill)
	    fall(obj, FALSE);
	else
	    discard(obj);
    }
    moat(mp->y, mp->x) = NULL;
    mvaddch(mp->y, mp->x, tp->t_oldch);
    detach(Mlist, tp);
    if (on(*tp, ISTARGET))
    {
	Kamikaze = FALSE;
	To_death = FALSE;
	if (Fight_flush)
	    flush_type();
    }
    discard(tp);
}

/*
 * killed:
 *	Called to put a monster to death
 */
void
killed(THING *tp, bool pr)
{
    char *mname;

    Pstats.s_exp += tp->t_stats.s_exp;

    /*
     * If the monster was a venus flytrap, un-hold him
     */
    switch (tp->t_type)
    {
	when 'F':
	    Player.t_flags &= ~ISHELD;
	    Vf_hit = 0;
	    strcpy(Monsters['F'-'A'].m_stats.s_dmg, "000x0");
	when 'L':
	{
	    THING *gold;

	    if (fallpos(&tp->t_pos, &tp->t_room->r_gold) && Level >= Max_level)
	    {
		gold = new_item();
		gold->o_type = GOLD;
		gold->o_goldval = GOLDCALC;
		if (save(VS_MAGIC))
		    gold->o_goldval += GOLDCALC + GOLDCALC
				     + GOLDCALC + GOLDCALC;
		attach(tp->t_pack, gold);
	    }
	}
    }
    /*
     * Get rid of the monster.
     */
    mname = set_mname(tp);
    remove_mon(&tp->t_pos, tp, TRUE);
    if (pr)
    {
	if (Has_hit)
	{
	    addmsg(".  Defeated ");
	    Has_hit = FALSE;
	}
	else
	{
	    if (!Terse)
		addmsg("you have ");
	    addmsg("defeated ");
	}
	msg(mname);
    }
    /*
     * Do adjustments if he went up a level
     */
    check_level();
    if (Fight_flush)
	flush_type();
}
