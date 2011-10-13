/*
 * File for the fun ends
 * Death or a total win
 *
 * @(#)inter.c	4.57 (Berkeley) 02/05/99
 */

#include <curses.h>
#ifdef	attron
#include <term.h>
#endif	attron
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <pwd.h>
#include <ctype.h>
#include "netprot.h"
#include "netwait.h"

#include <fcntl.h>

#ifdef	attron
int	_putchar(char c);
# define	_puts(s)	tputs(s, 0, _putchar);
# define	SO		enter_standout_mode
# define	SE		exit_standout_mode
#endif

static char *Rip[] = {
"                       __________\n",
"                      /          \\\n",
"                     /    REST    \\\n",
"                    /      IN      \\\n",
"                   /     PEACE      \\\n",
"                  /                  \\\n",
"                  |                  |\n",
"                  |                  |\n",
"                  |   killed by a    |\n",
"                  |                  |\n",
"                  |       1980       |\n",
"                 *|     *  *  *      | *\n",
"         ________)/\\\\_//(\\/(/\\)/\\//\\/|_)_______\n",
    0
};

/*
 * score:
 *	Figure score and post it.
 */
/* VARARGS2 */
void
score(int amount, int flags, char monst)
{
    SCORE *scp;
    int i;
    SCORE *sc2;
    SCORE *top_ten, *endp;
    FILE *outf;
# ifdef MASTER
    int prflags = 0;
# endif
    void (*fp)(int);
    int uid;
    static char *reason[] = {
	"killed",
	"quit",
	"A total winner",
	"killed with Amulet"
    };

    start_score();

    if (flags >= 0)
    {
	endwin();
	resetltchars();
	/*
	 * free up space to "guarantee" there is space for the top_ten
	 */
	delwin(stdscr);
	delwin(curscr);
	if (Hw != NULL)
	    delwin(Hw);
    }

    if (Fd >= 0)
	outf = fdopen(Fd, "w");
    else
	return;

    top_ten = (SCORE *) malloc(numscores * sizeof (SCORE));
    endp = &top_ten[numscores];
    for (scp = top_ten; scp < endp; scp++)
    {
	scp->sc_score = 0;
	for (i = 0; i < MAXSTR; i++)
	    scp->sc_name[i] = rnd(255);
	scp->sc_flags = RN;
	scp->sc_level = RN;
	scp->sc_monster = RN;
	scp->sc_uid = RN;
    }

    signal(SIGINT, SIG_DFL);
    if (flags >= 0
#ifdef MASTER
	    || Wizard
#endif
	)
    {
	printf("[Press return to continue]");
	fflush(stdout);
	fgets(Prbuf,10,stdin);
    }
#ifdef MASTER
    if (Wizard)
	if (strcmp(Prbuf, "names") == 0)
	    prflags = 1;
	else if (strcmp(Prbuf, "edit") == 0)
	    prflags = 2;
#endif
    rd_score(top_ten, Fd);
    fclose(outf);
    close(Fd);
    Fd = open(SCOREFILE, O_RDWR);
    outf = fdopen(Fd, "w");
    /*
     * Insert her in list if need be
     */
    sc2 = NULL;
    if (!Noscore)
    {
	uid = getuid();
	for (scp = top_ten; scp < endp; scp++)
	    if (amount > scp->sc_score)
		break;
	    else if (!Allscore &&	/* only one score per nowin uid */
		flags != 2 && scp->sc_uid == uid && scp->sc_flags != 2)
		    scp = endp;
	if (scp < endp)
	{
	    if (flags != 2 && !Allscore)
	    {
		for (sc2 = scp; sc2 < endp; sc2++)
		{
		    if (sc2->sc_uid == uid && sc2->sc_flags != 2)
			break;
		}
		if (sc2 >= endp)
		    sc2 = endp - 1;
	    }
	    else
		sc2 = endp - 1;
	    while (sc2 > scp)
	    {
		*sc2 = sc2[-1];
		sc2--;
	    }
	    scp->sc_score = amount;
	    strncpy(scp->sc_name, Whoami, MAXSTR);
	    scp->sc_flags = flags;
	    if (flags == 2)
		scp->sc_level = Max_level;
	    else
		scp->sc_level = Level;
	    scp->sc_monster = monst;
	    scp->sc_uid = uid;
	    sc2 = scp;
	}
    }
    /*
     * Print the list
     */
    if (flags != -1)
	putchar('\n');
    printf("Top %s %s:\n", Numname, Allscore ? "Scores" : "Rogueists");
    printf("Rank\tScore\tName\n");
    for (scp = top_ten; scp < endp; scp++)
    {
	if (scp->sc_score) {
	    if (sc2 == scp && SO)
		_puts(SO);
	    printf("%d\t%d\t%s: %s on level %d", scp - top_ten + 1,
		scp->sc_score, scp->sc_name, reason[scp->sc_flags],
		scp->sc_level);
	    if (scp->sc_flags == 0 || scp->sc_flags == 3)
		printf(" by %s", killname((char) scp->sc_monster, TRUE));
# ifdef MASTER
	    if (prflags == 1)
	    {
		struct passwd *pp, *getpwuid();

		if ((pp = getpwuid(scp->sc_uid)) == NULL)
		    printf(" (%d)", scp->sc_uid);
		else
		    printf(" (%s)", pp->pw_name);
		putchar('\n');
	    }
	    else if (prflags == 2)
	    {
		fflush(stdout);
		fgets(Prbuf,10,stdin);
		if (Prbuf[0] == 'd')
		{
		    for (sc2 = scp; sc2 < endp - 1; sc2++)
			*sc2 = *(sc2 + 1);
		    sc2 = endp - 1;
		    sc2->sc_score = 0;
		    for (i = 0; i < MAXSTR; i++)
			sc2->sc_name[i] = rnd(255);
		    sc2->sc_flags = RN;
		    sc2->sc_level = RN;
		    sc2->sc_monster = RN;
		    scp--;
		}
	    }
	    else
# endif MASTER
		printf(".\n");
	    if (sc2 == scp && SE)
		_puts(SE);
	}
	else
	    break;
    }
    fseek(outf, 0L, SEEK_SET);
    /*
     * Update the list file
     */
    if (sc2 != NULL)
    {
	if (lock_sc())
	{
	    fp = signal(SIGINT, SIG_IGN);
	    wr_score(top_ten, outf);
	    unlock_sc();
	    signal(SIGINT, fp);
	}
    }
    fclose(outf);
}

/*
 * death:
 *	Do something really fun when he dies
 */
void
death(char monst)
{
    char **dp, *killer;
    struct tm *lt;
    static time_t date;
    struct tm *localtime();

    signal(SIGINT, SIG_IGN);
    Purse -= Purse / 10;
    signal(SIGINT, leave);
    clear();
    killer = killname(monst, FALSE);
    if (!Tombstone)
    {
	mvprintw(LINES - 2, 0, "Killed by ");
	killer = killname(monst, FALSE);
	if (monst != 's' && monst != 'h')
	    printw("a%s ", vowelstr(killer));
	printw("%s with %d gold", killer, Purse);
    }
    else
    {
	time(&date);
	lt = localtime(&date);
	move(8, 0);
	dp = Rip;
	while (*dp)
	    addstr(*dp++);
	mvaddstr(17, center(killer), killer);
	if (monst == 's' || monst == 'h')
	    mvaddch(16, 32, ' ');
	else
	    mvaddstr(16, 33, vowelstr(killer));
	mvaddstr(14, center(Whoami), Whoami);
	sprintf(Prbuf, "%d Au", Purse);
	move(15, center(Prbuf));
	addstr(Prbuf);
	sprintf(Prbuf, "%2d", lt->tm_year);
	mvaddstr(18, 28, Prbuf);
    }
    move(LINES - 1, 0);
    refresh();
    score(Purse, Amulet ? 3 : 0, monst);
    my_exit(0);
}

/*
 * center:
 *	Return the index to center the given string
 */
int
center(char *str)
{
    return 28 - ((strlen(str) + 1) / 2);
}

/*
 * total_winner:
 *	Code for a winner
 */
void
total_winner(void)
{
    THING *obj;
    struct obj_info *op;
    int worth;
    int oldpurse;

    clear();
    standout();
    addstr("                                                               \n");
    addstr("  @   @               @   @           @          @@@  @     @  \n");
    addstr("  @   @               @@ @@           @           @   @     @  \n");
    addstr("  @   @  @@@  @   @   @ @ @  @@@   @@@@  @@@      @  @@@    @  \n");
    addstr("   @@@@ @   @ @   @   @   @     @ @   @ @   @     @   @     @  \n");
    addstr("      @ @   @ @   @   @   @  @@@@ @   @ @@@@@     @   @     @  \n");
    addstr("  @   @ @   @ @  @@   @   @ @   @ @   @ @         @   @  @     \n");
    addstr("   @@@   @@@   @@ @   @   @  @@@@  @@@@  @@@     @@@   @@   @  \n");
    addstr("                                                               \n");
    addstr("     Congratulations, you have made it to the light of day!    \n");
    standend();
    addstr("\nYou have joined the elite ranks of those who have escaped the\n");
    addstr("Dungeons of Doom alive.  You journey home and sell all your loot at\n");
    addstr("a great profit and are admitted to the Fighters' Guild.\n");
    mvaddstr(LINES - 1, 0, "--Press space to continue--");
    refresh();
    wait_for(' ');
    clear();
    mvaddstr(0, 0, "   Worth  Item\n");
    oldpurse = Purse;
    for (obj = Pack; obj != NULL; obj = next(obj))
    {
	switch (obj->o_type)
	{
	    when FOOD:
		worth = 2 * obj->o_count;
	    when WEAPON:
		worth = Weap_info[obj->o_which].oi_worth;
		worth *= 3 * (obj->o_hplus + obj->o_dplus) + obj->o_count;
		obj->o_flags |= ISKNOW;
	    when ARMOR:
		worth = Arm_info[obj->o_which].oi_worth;
		worth += (9 - obj->o_arm) * 100;
		worth += (10 * (A_class[obj->o_which] - obj->o_arm));
		obj->o_flags |= ISKNOW;
	    when SCROLL:
		worth = Scr_info[obj->o_which].oi_worth;
		worth *= obj->o_count;
		op = &Scr_info[obj->o_which];
		if (!op->oi_know)
		    worth /= 2;
		op->oi_know = TRUE;
	    when POTION:
		worth = Pot_info[obj->o_which].oi_worth;
		worth *= obj->o_count;
		op = &Pot_info[obj->o_which];
		if (!op->oi_know)
		    worth /= 2;
		op->oi_know = TRUE;
	    when RING:
		op = &Ring_info[obj->o_which];
		worth = op->oi_worth;
		if (obj->o_which == R_ADDSTR || obj->o_which == R_ADDDAM ||
		    obj->o_which == R_PROTECT || obj->o_which == R_ADDHIT)
			if (obj->o_arm > 0)
			    worth += obj->o_arm * 100;
			else
			    worth = 10;
		if (!(obj->o_flags & ISKNOW))
		    worth /= 2;
		obj->o_flags |= ISKNOW;
		op->oi_know = TRUE;
	    when STICK:
		op = &Ws_info[obj->o_which];
		worth = op->oi_worth;
		worth += 20 * obj->o_charges;
		if (!(obj->o_flags & ISKNOW))
		    worth /= 2;
		obj->o_flags |= ISKNOW;
		op->oi_know = TRUE;
	    when AMULET:
		worth = 1000;
	}
	if (worth < 0)
	    worth = 0;
	printw("%c) %5d  %s\n", obj->o_packch, worth, inv_name(obj, FALSE));
	Purse += worth;
    }
    printw("   %5d  Gold Pieces          ", oldpurse);
    refresh();
    score(Purse, 2, ' ');
    my_exit(0);
}

/*
 * killname:
 *	Convert a code to a monster name
 */
char *
killname(char monst, bool doart)
{
    struct h_list *hp;
    char *sp;
    bool article;
    static struct h_list nlist[] = {
	'a',	"arrow",		TRUE,
	'b',	"bolt",			TRUE,
	'd',	"dart",			TRUE,
	'h',	"hypothermia",		FALSE,
	's',	"starvation",		FALSE,
	'\0'
    };

    if (isupper(monst))
    {
	sp = Monsters[monst-'A'].m_name;
	article = TRUE;
    }
    else
    {
	sp = "Wally the Wonder Badger";
	article = FALSE;
	for (hp = nlist; hp->h_ch; hp++)
	    if (hp->h_ch == monst)
	    {
		sp = hp->h_desc;
		article = hp->h_print;
		break;
	    }
    }
    if (doart && article)
	sprintf(Prbuf, "a%s ", vowelstr(sp));
    else
	Prbuf[0] = '\0';
    strcat(Prbuf, sp);
    return Prbuf;
}

/*
 * death_monst:
 *	Return a monster appropriate for a random death.
 */
char
death_monst(void)
{
    static char poss[] =
    {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
	'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'h', 'd', 's',
	' '	/* This is provided to generate the "Wally the Wonder Badger"
		   message for killer */
    };

    return poss[rnd(sizeof poss / sizeof (char))];
}

#ifdef	attron
int
_putchar(char c)
{
	putchar(c);
}
#endif	attron
