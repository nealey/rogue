/*
 * Contains functions for dealing with things like potions, scrolls,
 * and other items.
 *
 * @(#)rcopy.c	4.53 (Berkeley) 02/05/99
 */

#include <curses.h>
#ifdef	attron
#include <term.h>
#endif	attron
#include <ctype.h>
#include "netprot.h"

/*
 * inv_name:
 *	Return the name of something as it would appear in an
 *	inventory.
 */
char *
inv_name(THING *obj, bool drop)
{
    char *pb;
    struct obj_info *op;
    char *sp;
    int which;

    pb = Prbuf;
    which = obj->o_which;
    switch (obj->o_type)
    {
        when POTION:
	    nameit(obj, "potion", P_colors[which], &Pot_info[which], nullstr);
	when RING:
	    nameit(obj, "ring", R_stones[which], &Ring_info[which], ring_num);
	when STICK:
	    nameit(obj, Ws_type[which], Ws_made[which], &Ws_info[which], charge_str);
	when SCROLL:
	    if (obj->o_count == 1)
	    {
		strcpy(pb, "A scroll ");
		pb = &Prbuf[9];
	    }
	    else
	    {
		sprintf(pb, "%d scrolls ", obj->o_count);
		pb = &Prbuf[strlen(Prbuf)];
	    }
	    op = &Scr_info[which];
	    if (op->oi_know)
		sprintf(pb, "of %s", op->oi_name);
	    else if (op->oi_guess)
		sprintf(pb, "called %s", op->oi_guess);
	    else
		sprintf(pb, "titled '%s'", S_names[which]);
	when FOOD:
	    if (which == 1)
		if (obj->o_count == 1)
		    sprintf(pb, "A%s %s", vowelstr(Fruit), Fruit);
		else
		    sprintf(pb, "%d %ss", obj->o_count, Fruit);
	    else
		if (obj->o_count == 1)
		    strcpy(pb, "Some food");
		else
		    sprintf(pb, "%d rations of food", obj->o_count);
	when WEAPON:
	    sp = Weap_info[which].oi_name;
	    if (obj->o_count > 1)
		sprintf(pb, "%d ", obj->o_count);
	    else
		sprintf(pb, "A%s ", vowelstr(sp));
	    pb = &Prbuf[strlen(Prbuf)];
	    if (obj->o_flags & ISKNOW)
		sprintf(pb, "%s %s", num(obj->o_hplus,obj->o_dplus,WEAPON), sp);
	    else
		sprintf(pb, "%s", sp);
	    if (obj->o_count > 1)
		strcat(pb, "s");
	    if (obj->o_label != NULL)
	    {
		pb = &Prbuf[strlen(Prbuf)];
		sprintf(pb, " called %s", obj->o_label);
	    }
	when ARMOR:
	    sp = Arm_info[which].oi_name;
	    if (obj->o_flags & ISKNOW)
	    {
		sprintf(pb, "%s %s [",
		    num(A_class[which] - obj->o_arm, 0, ARMOR), sp);
		if (!Terse)
		    strcat(pb, "protection ");
		pb = &Prbuf[strlen(Prbuf)];
		sprintf(pb, "%d]", 10 - obj->o_arm);
	    }
	    else
		sprintf(pb, "%s", sp);
	    if (obj->o_label != NULL)
	    {
		pb = &Prbuf[strlen(Prbuf)];
		sprintf(pb, " called %s", obj->o_label);
	    }
	when AMULET:
	    strcpy(pb, "The Amulet of Yendor");
	when GOLD:
	    sprintf(Prbuf, "%d Gold pieces", obj->o_goldval);
#ifdef MASTER
	otherwise:
	    debug("Picked up something funny %s", unctrl(obj->o_type));
	    sprintf(pb, "Something bizarre %s", unctrl(obj->o_type));
#endif
    }
    if (Inv_describe)
    {
	if (obj == Cur_armor)
	    strcat(pb, " (being worn)");
	if (obj == Cur_weapon)
	    strcat(pb, " (weapon in hand)");
	if (obj == Cur_ring[LEFT])
	    strcat(pb, " (on left hand)");
	else if (obj == Cur_ring[RIGHT])
	    strcat(pb, " (on right hand)");
    }
    if (drop && isupper(Prbuf[0]))
	Prbuf[0] = tolower(Prbuf[0]);
    else if (!drop && islower(*Prbuf))
	*Prbuf = toupper(*Prbuf);
    Prbuf[MAXSTR-1] = '\0';
    return Prbuf;
}

/*
 * drop:
 *	Put something down
 */
void
drop(void)
{
    char ch;
    THING *obj;

    ch = chat(Hero.y, Hero.x);
    if (ch != FLOOR && ch != PASSAGE)
    {
	After = FALSE;
	msg("there is something there already");
	return;
    }
    if ((obj = get_item("drop", 0)) == NULL)
	return;
    if (!dropcheck(obj))
	return;
    obj = leave_pack(obj, TRUE, !ISMULT(obj->o_type));
    /*
     * Link it into the level object list
     */
    attach(Lvl_obj, obj);
    chat(Hero.y, Hero.x) = obj->o_type;
    flat(Hero.y, Hero.x) |= F_DROPPED;
    obj->o_pos = Hero;
    if (obj->o_type == AMULET)
	Amulet = FALSE;
    msg("dropped %s", inv_name(obj, TRUE));
}

/*
 * dropcheck:
 *	Do special checks for dropping or unweilding|unwearing|unringing
 */
bool
dropcheck(THING *obj)
{
    if (obj == NULL)
	return TRUE;
    if (obj != Cur_armor && obj != Cur_weapon
	&& obj != Cur_ring[LEFT] && obj != Cur_ring[RIGHT])
	    return TRUE;
    if (obj->o_flags & ISCURSED)
    {
	msg("you can't.  It appears to be cursed");
	return FALSE;
    }
    if (obj == Cur_weapon)
	Cur_weapon = NULL;
    else if (obj == Cur_armor)
    {
	waste_time();
	Cur_armor = NULL;
    }
    else
    {
	Cur_ring[obj == Cur_ring[LEFT] ? LEFT : RIGHT] = NULL;
	switch (obj->o_which)
	{
	    case R_ADDSTR:
		chg_str(-obj->o_arm);
		break;
	    case R_SEEINVIS:
		unsee();
		extinguish(unsee);
		break;
	}
    }
    return TRUE;
}

/*
 * new_thing:
 *	Return a new thing
 */
THING *
new_thing(void)
{
    THING *cur;
    int r;

    cur = new_item();
    cur->o_hplus = 0;
    cur->o_dplus = 0;
    cur->o_damage = cur->o_hurldmg = "0x0";
    cur->o_arm = 11;
    cur->o_count = 1;
    cur->o_group = 0;
    cur->o_flags = 0;
    /*
     * Decide what kind of object it will be
     * If we haven't had food for a while, let it be food.
     */
    switch (No_food > 3 ? 2 : pick_one(Things, NUMTHINGS))
    {
	when 0:
	    cur->o_type = POTION;
	    cur->o_which = pick_one(Pot_info, MAXPOTIONS);
	when 1:
	    cur->o_type = SCROLL;
	    cur->o_which = pick_one(Scr_info, MAXSCROLLS);
	when 2:
	    cur->o_type = FOOD;
	    No_food = 0;
	    if (rnd(10) != 0)
		cur->o_which = 0;
	    else
		cur->o_which = 1;
	when 3:
	    init_weapon(cur, pick_one(Weap_info, MAXWEAPONS));
	    if ((r = rnd(100)) < 10)
	    {
		cur->o_flags |= ISCURSED;
		cur->o_hplus -= rnd(3) + 1;
	    }
	    else if (r < 15)
		cur->o_hplus += rnd(3) + 1;
	when 4:
	    cur->o_type = ARMOR;
	    cur->o_which = pick_one(Arm_info, MAXARMORS);
	    cur->o_arm = A_class[cur->o_which];
	    if ((r = rnd(100)) < 20)
	    {
		cur->o_flags |= ISCURSED;
		cur->o_arm += rnd(3) + 1;
	    }
	    else if (r < 28)
		cur->o_arm -= rnd(3) + 1;
	when 5:
	    cur->o_type = RING;
	    cur->o_which = pick_one(Ring_info, MAXRINGS);
	    switch (cur->o_which)
	    {
		when R_ADDSTR:
		case R_PROTECT:
		case R_ADDHIT:
		case R_ADDDAM:
		    if ((cur->o_arm = rnd(3)) == 0)
		    {
			cur->o_arm = -1;
			cur->o_flags |= ISCURSED;
		    }
		when R_AGGR:
		case R_TELEPORT:
		    cur->o_flags |= ISCURSED;
	    }
	when 6:
	    cur->o_type = STICK;
	    cur->o_which = pick_one(Ws_info, MAXSTICKS);
	    fix_stick(cur);
#ifdef MASTER
	otherwise:
	    debug("Picked a bad kind of object");
	    wait_for(' ');
#endif
    }
    return cur;
}

/*
 * pick_one:
 *	Pick an item out of a list of nitems possible objects
 */
int
pick_one(struct obj_info *info, int nitems)
{
    struct obj_info *end;
    struct obj_info *start;
    int i;

    start = info;
    for (end = &info[nitems], i = rnd(100); info < end; info++)
	if (i < info->oi_prob)
	    break;
    if (info == end)
    {
#ifdef MASTER
	if (Wizard)
	{
	    msg("bad pick_one: %d from %d items", i, nitems);
	    for (info = start; info < end; info++)
		msg("%s: %d%%", info->oi_name, info->oi_prob);
	}
#endif
	info = start;
    }
    return info - start;
}

/*
 * discovered:
 *	list what the player has discovered in this game of a certain type
 */
static int Line_cnt = 0;

static bool Newpage = FALSE;

static char *Lastfmt, *Lastarg;

void
discovered(void)
{
    char ch;
    bool disc_list;

    do {
	disc_list = FALSE;
	if (!Terse)
	    addmsg("for ");
	addmsg("what type");
	if (!Terse)
	    addmsg(" of object do you want a list");
	msg("? (* for all)");
	ch = readchar();
	switch (ch)
	{
	    case ESCAPE:
		msg("");
		return;
	    case POTION:
	    case SCROLL:
	    case RING:
	    case STICK:
	    case '*':
		disc_list = TRUE;
		break;
	    default:
		if (Terse)
		    msg("Not a type");
		else
		    msg("Please type one of %c%c%c%c (ESCAPE to quit)", POTION, SCROLL, RING, STICK);
	}
    } while (!disc_list);
    if (ch == '*')
    {
	print_disc(POTION);
	add_line("", NULL);
	print_disc(SCROLL);
	add_line("", NULL);
	print_disc(RING);
	add_line("", NULL);
	print_disc(STICK);
	end_line();
    }
    else
    {
	print_disc(ch);
	end_line();
    }
}

/*
 * print_disc:
 *	Print what we've discovered of type 'type'
 */

#define MAX4(a,b,c,d)	(a > b ? (a > c ? (a > d ? a : d) : (c > d ? c : d)) : (b > c ? (b > d ? b : d) : (c > d ? c : d)))

void
print_disc(char type)
{
    struct obj_info *info;
    int i, maxnum, num_found;
    static THING obj;
    static short order[MAX4(MAXSCROLLS, MAXPOTIONS, MAXRINGS, MAXSTICKS)];

    switch (type)
    {
	case SCROLL:
	    maxnum = MAXSCROLLS;
	    info = Scr_info;
	    break;
	case POTION:
	    maxnum = MAXPOTIONS;
	    info = Pot_info;
	    break;
	case RING:
	    maxnum = MAXRINGS;
	    info = Ring_info;
	    break;
	case STICK:
	    maxnum = MAXSTICKS;
	    info = Ws_info;
	    break;
    }
    set_order(order, maxnum);
    obj.o_count = 1;
    obj.o_flags = 0;
    num_found = 0;
    for (i = 0; i < maxnum; i++)
	if (info[order[i]].oi_know || info[order[i]].oi_guess)
	{
	    obj.o_type = type;
	    obj.o_which = order[i];
	    add_line("%s", inv_name(&obj, FALSE));
	    num_found++;
	}
    if (num_found == 0)
	add_line(nothing(type), NULL);
}

/*
 * set_order:
 *	Set up order for list
 */
void
set_order(short *order, int numthings)
{
    int i, r, t;

    for (i = 0; i< numthings; i++)
	order[i] = i;

    for (i = numthings; i > 0; i--)
    {
	r = rnd(i);
	t = order[i - 1];
	order[i - 1] = order[r];
	order[r] = t;
    }
}

/*
 * add_line:
 *	Add a line to the list of discoveries
 */
/* VARARGS1 */
char
add_line(char *fmt, char *arg)
{
    WINDOW *tw, *sw;
    int x, y;
    char *prompt = "--Press space to continue--";
    static int maxlen = -1;

    if (Line_cnt == 0)
    {
	    wclear(Hw);
	    if (Inv_type == INV_SLOW)
		Mpos = 0;
    }
    if (Inv_type == INV_SLOW)
    {
	if (*fmt != '\0')
	    if (msg(fmt, arg) == ESCAPE)
		return ESCAPE;
	Line_cnt++;
    }
    else
    {
	if (maxlen < 0)
	    maxlen = strlen(prompt);
	if (Line_cnt >= LINES - 1 || fmt == NULL)
	{
	    if (Inv_type == INV_OVER && fmt == NULL && !Newpage)
	    {
		msg("");
		refresh();
		tw = newwin(Line_cnt + 1, maxlen + 2, 0, COLS - maxlen - 3);
		sw = subwin(tw, Line_cnt + 1, maxlen + 1, 0, COLS - maxlen - 2);
		for (y = 0; y <= Line_cnt; y++)
		{
		    wmove(sw, y, 0);
		    for (x = 0; x <= maxlen; x++)
			waddch(sw, mvwinch(Hw, y, x));
		}
		wmove(tw, Line_cnt, 1);
		waddstr(tw, prompt);
		/*
		 * if there are lines below, use 'em
		 */
		if (LINES > NUMLINES)
		    if (NUMLINES + Line_cnt > LINES)
			mvwin(tw, LINES - (Line_cnt + 1), COLS - maxlen - 3);
		    else
			mvwin(tw, NUMLINES, 0);
		touchwin(tw);
		wrefresh(tw);
		wait_for(' ');
#ifndef	attron
		if (CE)
#else	attron
		if (clr_eol)
#endif	attron
		{
		    werase(tw);
		    leaveok(tw, TRUE);
		    wrefresh(tw);
		}
		delwin(tw);
		touchwin(stdscr);
	    }
	    else
	    {
		wmove(Hw, LINES - 1, 0);
		waddstr(Hw, prompt);
		wrefresh(Hw);
		wait_for(' ');
		clearok(curscr, TRUE);
		wclear(Hw);
#ifdef	attron
		touchwin(stdscr);
#endif	attron
	    }
	    Newpage = TRUE;
	    Line_cnt = 0;
	    maxlen = strlen(prompt);
	}
	if (fmt != NULL && !(Line_cnt == 0 && *fmt == '\0'))
	{
	    mvwprintw(Hw, Line_cnt++, 0, fmt, arg);
	    getyx(Hw, y, x);
	    if (maxlen < x)
		maxlen = x;
	    Lastfmt = fmt;
	    Lastarg = arg;
	}
    }
    return ~ESCAPE;
}

/*
 * end_line:
 *	End the list of lines
 */
void
end_line(void)
{
    if (Inv_type != INV_SLOW)
	if (Line_cnt == 1 && !Newpage)
	{
	    Mpos = 0;
	    msg(Lastfmt, Lastarg);
	}
	else
	    add_line((char *) NULL, NULL);
    Line_cnt = 0;
    Newpage = FALSE;
}

/*
 * nothing:
 *	Set up Prbuf so that message for "nothing found" is there
 */
char *
nothing(char type)
{
    char *sp, *tystr;

    if (Terse)
	sprintf(Prbuf, "Nothing");
    else
	sprintf(Prbuf, "Haven't discovered anything");
    if (type != '*')
    {
	sp = &Prbuf[strlen(Prbuf)];
	switch (type)
	{
	    when POTION: tystr = "potion";
	    when SCROLL: tystr = "scroll";
	    when RING: tystr = "ring";
	    when STICK: tystr = "stick";
	}
	sprintf(sp, " about any %ss", tystr);
    }
    return Prbuf;
}

/*
 * nameit:
 *	Give the proper name to a potion, stick, or ring
 */
void
nameit(THING *obj, char *type, char *which, struct obj_info *op,
    char *(*prfunc)(THING *))
{
    char *pb;

    if (op->oi_know || op->oi_guess)
    {
	if (obj->o_count == 1)
	    sprintf(Prbuf, "A %s ", type);
	else
	    sprintf(Prbuf, "%d %ss ", obj->o_count, type);
	pb = &Prbuf[strlen(Prbuf)];
	if (op->oi_know)
	    sprintf(pb, "of %s%s(%s)", op->oi_name, (*prfunc)(obj), which);
	else if (op->oi_guess)
	    sprintf(pb, "called %s%s(%s)", op->oi_guess, (*prfunc)(obj), which);
    }
    else if (obj->o_count == 1)
	sprintf(Prbuf, "A%s %s %s", vowelstr(which), which, type);
    else
	sprintf(Prbuf, "%d %s %ss", obj->o_count, which, type);
}

/*
 * nullstr:
 *	Return a pointer to a null-length string
 */
char *
nullstr(THING *ignored)
{
    return "";
}

# ifdef	MASTER
/*
 * pr_list:
 *	List possible potions, scrolls, etc. for wizard.
 */
void
pr_list(void)
{
    int ch;

    if (!Terse)
	addmsg("for ");
    addmsg("what type");
    if (!Terse)
	addmsg(" of object do you want a list");
    msg("? ");
    ch = readchar();
    switch (ch)
    {
	when POTION:
	    pr_spec(Pot_info, MAXPOTIONS);
	when SCROLL:
	    pr_spec(Scr_info, MAXSCROLLS);
	when RING:
	    pr_spec(Ring_info, MAXRINGS);
	when STICK:
	    pr_spec(Ws_info, MAXSTICKS);
	when ARMOR:
	    pr_spec(Arm_info, MAXARMORS);
	when WEAPON:
	    pr_spec(Weap_info, MAXWEAPONS);
	otherwise:
	    return;
    }
}

/*
 * pr_spec:
 *	Print specific list of possible items to choose from
 */
void
pr_spec(struct obj_info *info, int nitems)
{
    struct obj_info *endp;
    int i, lastprob;

    endp = &info[nitems];
    lastprob = 0;
    for (i = '0'; info < endp; i++)
    {
	if (i == '9' + 1)
	    i = 'a';
	sprintf(Prbuf, "%c: %%s (%d%%%%)", i, info->oi_prob - lastprob);
	lastprob = info->oi_prob;
	add_line(Prbuf, info->oi_name);
	info++;
    }
    end_line();
}
# endif	MASTER
