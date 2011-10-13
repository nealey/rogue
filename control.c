/*
 * Various input/output functions
 *
 * @(#)control.c	4.32 (Berkeley) 02/05/99
 */

#include <curses.h>
#include <ctype.h>
#include "netprot.h"

/*
 * msg:
 *	Display a message at the top of the screen.
 */
#define MAXMSG	(NUMCOLS - sizeof "--More--")

static char Msgbuf[MAXMSG+1];
static int Newpos = 0;

/* VARARGS1 */
int
msg(char *fmt, ...)
{
    va_list args;

    /*
     * if the string is "", just clear the line
     */
    if (*fmt == '\0')
    {
	move(0, 0);
	clrtoeol();
	Mpos = 0;
	return ~ESCAPE;
    }
    /*
     * otherwise add to the message and flush it out
     */
    va_start(args, fmt);
    doadd(fmt, args);
    va_end(args);
    return endmsg();
}

/*
 * addmsg:
 *	Add things to the current message
 */
/* VARARGS1 */
void
addmsg(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    doadd(fmt, args);
    va_end(args);
}

/*
 * endmsg:
 *	Display a new msg (giving him a chance to see the previous one
 *	if it is up there with the --More--)
 */
int
endmsg(void)
{
    char ch;

    if (Save_msg)
	strcpy(Huh, Msgbuf);
    if (Mpos)
    {
	look(FALSE);
	mvaddstr(0, Mpos, "--More--");
	refresh();
	if (!Msg_esc)
	    wait_for(' ');
	else
	{
	    while ((ch = readchar()) != ' ')
		if (ch == ESCAPE)
		{
		    Msgbuf[0] = '\0';
		    Mpos = 0;
		    Newpos = 0;
		    Msgbuf[0] = '\0';
		    return ESCAPE;
		}
	}
    }
    /*
     * All messages should start with uppercase, except ones that
     * start with a pack addressing character
     */
    if (islower(Msgbuf[0]) && !Lower_msg && Msgbuf[1] != ')')
	Msgbuf[0] = toupper(Msgbuf[0]);
    mvaddstr(0, 0, Msgbuf);
    clrtoeol();
    Mpos = Newpos;
    Newpos = 0;
    Msgbuf[0] = '\0';
    refresh();
    return ~ESCAPE;
}

/*
 * doadd:
 *	Perform an add onto the message buffer
 */
void
doadd(char *fmt, va_list args)
{
    static char buf[MAXSTR];

    /*
     * Do the printf into buf
     */
    vsprintf(buf, fmt, args);
    if (strlen(buf) + Newpos >= MAXMSG)
	endmsg();
    strcat(Msgbuf, buf);
    Newpos = strlen(Msgbuf);
}

/*
 * step_ok:
 *	Returns true if it is ok to step on ch
 */
bool
step_ok(char ch)
{
    switch (ch)
    {
	case ' ':
	case '|':
	case '-':
	    return FALSE;
	default:
	    return (!isalpha(ch));
    }
}

/*
 * readchar:
 *	Reads and returns a character, checking for gross input errors
 */
int
readchar(void)
{
    int cnt;
    static char c;

    cnt = 0;
    while (read(0, &c, 1) <= 0)
	if (cnt++ > 100)	/* if we are getting infinite EOFs */
	    auto_save(0);	/* save the game */
    return c;
}

/*
 * status:
 *	Display the important stats line.  Keep the cursor where it was.
 */
void
status(void)
{
    int (*print_func)(char *fmt, ...);
# define	PRINT_FUNC	(*print_func)
    int oy, ox, temp;
    static int hpwidth = 0;
    static int s_hungry;
    static int s_lvl;
    static int s_pur = -1;
    static int s_hp;
    static int s_arm;
    static str_t s_str;
    static long s_exp = 0;
    static char *state_name[] =
    {
	"", "Hungry", "Weak", "Faint"
    };

    /*
     * If nothing has changed since the last status, don't
     * bother.
     */
    temp = (Cur_armor != NULL ? Cur_armor->o_arm : Pstats.s_arm);
    if (s_hp == Pstats.s_hpt && s_exp == Pstats.s_exp && s_pur == Purse
	&& s_arm == temp && s_str == Pstats.s_str && s_lvl == Level
	&& s_hungry == Hungry_state
	&& !Stat_msg
	)
	    return;

    s_arm = temp;

    getyx(stdscr, oy, ox);
    if (s_hp != Max_hp)
    {
	temp = Max_hp;
	s_hp = Max_hp;
	for (hpwidth = 0; temp; hpwidth++)
	    temp /= 10;
    }

    /*
     * Save current status
     */
    s_lvl = Level;
    s_pur = Purse;
    s_hp = Pstats.s_hpt;
    s_str = Pstats.s_str;
    s_exp = Pstats.s_exp; 
    s_hungry = Hungry_state;

    if (Stat_msg)
    {
	print_func = msg;
	move(0, 0);
    }
    else
    {
	move(STATLINE, 0);
	print_func = printw;
    }
    PRINT_FUNC("Level: %d  Gold: %-5d  Hp: %*d(%*d)  Str: %2d(%d)  Arm: %-2d  Exp: %d/%ld  %s",
	    Level, Purse, hpwidth, Pstats.s_hpt, hpwidth, Max_hp, Pstats.s_str,
	    Max_stats.s_str, 10 - s_arm, Pstats.s_lvl, Pstats.s_exp,
	    state_name[Hungry_state]);

    clrtoeol();
    move(oy, ox);
}

/*
 * wait_for
 *	Sit around until the guy types the right key
 */
void
wait_for(char ch)
{
    char c;

    if (ch == '\n')
        while ((c = readchar()) != '\n' && c != '\r')
	    continue;
    else
        while (readchar() != ch)
	    continue;
}

/*
 * show_win:
 *	Function used to display a window and wait before returning
 */
void
show_win(char *message)
{
    WINDOW *win;

    win = Hw;
    wmove(win, 0, 0);
    waddstr(win, message);
    touchwin(win);
    wmove(win, Hero.y, Hero.x);
    wrefresh(win);
    wait_for(' ');
    clearok(curscr, TRUE);
#ifdef	attron
    touchwin(stdscr);
#endif	attron
}
