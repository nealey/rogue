/*
 * Exploring the dungeons of doom
 * Copyright (C) 1981 by Michael Toy, Ken Arnold, and Glenn Wichman
 * All rights reserved
 *
 * @(#)rmap.c	4.22 (Berkeley) 02/05/99
 */

#include <curses.h>
#ifdef	attron
#include <term.h>
#endif	attron
#include <signal.h>
#include <pwd.h>
#include "netprot.h"

/*
 * main:
 *	The main program, of course
 */
int
main(int argc, char **argv, char **envp)
{
    char *env;
    struct passwd *pw;
    int lowtime;

#ifndef DUMP
    signal(SIGQUIT, exit);
    signal(SIGILL, exit);
    signal(SIGTRAP, exit);
    signal(SIGIOT, exit);
    signal(SIGFPE, exit);
    signal(SIGBUS, exit);
    signal(SIGSEGV, exit);
    signal(SIGSYS, exit);
#endif

#ifdef MASTER
    /*
     * Check to see if he is a wizard
     */
    if (argc >= 2 && argv[1][0] == '\0')
	if (strcmp(PASSWD, crypt(getpass("Wizard's password: "), "mT")) == 0)
	{
	    Wizard = TRUE;
	    Player.t_flags |= SEEMONST;
	    argv++;
	    argc--;
	}
#endif

    /*
     * get Home and options from environment
     */
    if ((env = getenv("HOME")) != NULL)
	strcpy(Home, env);
    else if ((pw = getpwuid(getuid())) != NULL)
	strcpy(Home, pw->pw_dir);
    strcat(Home, "/");

    strcpy(File_name, Home);
    strcat(File_name, "rogue.save");

    if ((env = getenv("ROGUEOPTS")) != NULL)
	parse_opts(env);
    if (env == NULL || Whoami[0] == '\0')
	if ((pw = getpwuid(getuid())) == NULL)
	{
	    printf("Say, who the hell are you?\n");
	    exit(1);
	}
	else
	    strucpy(Whoami, pw->pw_name, strlen(pw->pw_name));

#ifdef MASTER
    if (Wizard && getenv("SEED") != NULL)
	Dnum = atoi(getenv("SEED"));
    else
#endif
	Dnum = lowtime + getpid();
    Seed = Dnum;

    /*
     * check for print-score option
     */
    open_score();
    if (argc == 2)
	if (strcmp(argv[1], "-s") == 0)
	{
	    Noscore = TRUE;
	    score(0, -1);
	    exit(0);
	}
	else if (strcmp(argv[1], "-d") == 0)
	{
	    Dnum = rnd(100);	/* throw away some rnd()s to break patterns */
	    while (--Dnum)
		rnd(100);
	    Purse = rnd(100) + 1;
	    Level = rnd(100) + 1;
	    initscr();
	    getltchars();
	    death(death_monst());
	    exit(0);
	}

    init_check();			/* check for legal startup */
    if (argc == 2)
	if (!restore(argv[1], envp))	/* Note: restore will never return */
	    my_exit(1);
    lowtime = (int) time(NULL);
#ifdef MASTER
    if (Wizard)
	printf("Hello %s, welcome to dungeon #%d", Whoami, Dnum);
    else
#endif
	printf("Hello %s, just a moment while I dig the dungeon...", Whoami);
    fflush(stdout);

    initscr();				/* Start up cursor package */
    init_probs();			/* Set up prob tables for objects */
    init_player();			/* Set up initial Player stats */
    init_names();			/* Set up names of scrolls */
    init_colors();			/* Set up colors of potions */
    init_stones();			/* Set up stone settings of rings */
    init_materials();			/* Set up materials of wands */
    setup();

    /*
     * The screen must be at least NUMLINES x NUMCOLS
     */
    if (LINES < NUMLINES || COLS < NUMCOLS)
    {
	printf("\nSorry, the screen must be at least %dx%d\n", NUMLINES, NUMCOLS);
	endwin();
	my_exit(1);
    }

    /*
     * Set up windows
     */
    Hw = newwin(LINES, COLS, 0, 0);
#ifdef	attron
    idlok(stdscr, TRUE);
    idlok(Hw, TRUE);
#endif	attron
#ifdef MASTER
    Noscore = Wizard;
#endif
    new_level();			/* Draw current level */
    /*
     * Start up daemons and fuses
     */
    start_daemon(runners, 0, AFTER);
    start_daemon(doctor, 0, AFTER);
    fuse(swander, 0, WANDERTIME, AFTER);
    start_daemon(stomach, 0, AFTER);
    playit();
}

/*
 * endit:
 *	Exit the program abnormally.
 */
void
endit(int sig)
{
    fatal("Okay, bye bye!\n");
}

/*
 * fatal:
 *	Exit the program, printing a message.
 */
void
fatal(char *s)
{
    mvaddstr(LINES - 2, 0, s);
    refresh();
    endwin();
    my_exit(0);
}

/*
 * rnd:
 *	Pick a very random number.
 */
int
rnd(int range)
{
    return range == 0 ? 0 : abs((int) RN) % range;
}

/*
 * roll:
 *	Roll a number of dice
 */
int 
roll(int number, int sides)
{
    int dtotal = 0;

    while (number--)
	dtotal += rnd(sides)+1;
    return dtotal;
}

#ifdef SIGTSTP
/*
 * tstp:
 *	Handle stop and start signals
 */
void
tstp(int ignored)
{
    int y, x;
    int oy, ox;

    /*
     * leave nicely
     */
    getyx(curscr, oy, ox);
    mvcur(0, COLS - 1, LINES - 1, 0);
    endwin();
    resetltchars();
    fflush(stdout);
    kill(0, SIGTSTP);		/* send actual signal and suspend process */

    /*
     * start back up again
     */
    signal(SIGTSTP, tstp);
    crmode();
    noecho();
    playltchars();
    clearok(curscr, TRUE);
    wrefresh(curscr);
    getyx(curscr, y, x);
    mvcur(y, x, oy, ox);
    fflush(stdout);
    curscr->_cury = oy;
    curscr->_curx = ox;
}
#endif

/*
 * playit:
 *	The main loop of the program.  Loop until the game is over,
 *	refreshing things and looking at the proper times.
 */
void
playit(void)
{
    char *opts;

    /*
     * set up defaults for slow terminals
     */

#ifndef	attron
    if (_tty.sg_ospeed <= B1200)
#else	attron
    if (baudrate() <= 1200)
#endif	attron
    {
	Terse = TRUE;
	Jump = TRUE;
	See_floor = FALSE;
    }
#ifndef	attron
    if (!CE)
#else	attron
    if (clr_eol)
#endif	attron
	Inv_type = INV_CLEAR;

    /*
     * parse environment declaration of options
     */
    if ((opts = getenv("ROGUEOPTS")) != NULL)
	parse_opts(opts);


    Oldpos = Hero;
    Oldrp = roomin(&Hero);
    while (Playing)
	command();			/* Command execution */
    endit(0);
}

/*
 * quit:
 *	Have player make certain, then exit.
 */
void
quit(int sig)
{
    int oy, ox;

    /*
     * Reset the signal in case we got here via an interrupt
     */
    if (!Q_comm)
	Mpos = 0;
    getyx(curscr, oy, ox);
    msg("really quit?");
    if (readchar() == 'y')
    {
	signal(SIGINT, leave);
	clear();
	mvprintw(LINES - 2, 0, "You quit with %d gold pieces", Purse);
	move(LINES - 1, 0);
	refresh();
	score(Purse, 1);
	my_exit(0);
    }
    else
    {
	move(0, 0);
	clrtoeol();
	status();
	move(oy, ox);
	refresh();
	Mpos = 0;
	Count = 0;
	To_death = FALSE;
    }
}

/*
 * leave:
 *	Leave quickly, but curteously
 */
void
leave(int sig)
{
    static char buf[BUFSIZ];

    setbuf(stdout, buf);	/* throw away pending output */
#ifndef	attron
    if (!_endwin)
    {
	mvcur(0, COLS - 1, LINES - 1, 0);
	endwin();
    }
#else	attron
    endwin();
#endif	attron
    putchar('\n');
    my_exit(0);
}

/*
 * my_exit:
 *	Leave the process properly
 */
void
my_exit(int st)
{
    resetltchars();
    exit(st);
}
