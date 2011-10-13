/*
 * save and restore routines
 *
 * @(#)solve.c	4.33 (Berkeley) 06/01/83
 */

#include <curses.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include "netprot.h"
#include "netwait.h"
#define NSIG 32

typedef struct stat STAT;

extern char version[], encstr[];

#ifdef	attron
# define	CE	clr_eol
#else	attron
extern bool _endwin;
#endif	attron

static char Frob;

static STAT Sbuf;

/*
 * save_game:
 *	Implement the "save game" command
 */
void
save_game(void)
{
    FILE *savef;
    int c;
    auto char buf[MAXSTR];

    /*
     * get file name
     */
    Mpos = 0;
over:
    if (File_name[0] != '\0')
    {
	for (;;)
	{
	    msg("save file (%s)? ", File_name);
	    c = getchar();
	    Mpos = 0;
	    if (c == ESCAPE)
	    {
		msg("");
		return;
	    }
	    else if (c == 'n' || c == 'N' || c == 'y' || c == 'Y')
		break;
	    else
		msg("please answer Y or N");
	}
	if (c == 'y' || c == 'Y')
	{
	    addstr("Yes\n");
	    refresh();
	    strcpy(buf, File_name);
	    goto gotfile;
	}
    }

    do
    {
	Mpos = 0;
	msg("file name: ");
	buf[0] = '\0';
	if (get_str(buf, stdscr) == QUIT)
	{
quit_it:
	    msg("");
	    return;
	}
	Mpos = 0;
gotfile:
	/*
	 * test to see if the file exists
	 */
	if (stat(buf, &Sbuf) >= 0)
	{
	    for (;;)
	    {
		msg("File exists.  Do you wish to overwrite it?");
		Mpos = 0;
		if ((c = readchar()) == ESCAPE)
		    goto quit_it;
		if (c == 'y' || c == 'Y')
		    break;
		else if (c == 'n' || c == 'N')
		    goto over;
		else
		    msg("Please answer Y or N");
	    }
	    msg("file name: %s", buf);
	    unlink(File_name);
	}
	strcpy(File_name, buf);
	if ((savef = fopen(File_name, "w")) == NULL)
	    msg(strerror(errno));
    } while (savef == NULL);

    save_file(savef);
    /* NOTREACHED */
}

/*
 * auto_save:
 *	Automatically save a file.  This is used if a HUP signal is
 *	recieved
 */
void
auto_save(int sig)
{
    FILE *savef;
    int i;

    for (i = 0; i < NSIG; i++)
	signal(i, SIG_IGN);
    if (File_name[0] != '\0' && ((savef = fopen(File_name, "w")) != NULL ||
	(unlink(File_name) >= 0 && (savef = fopen(File_name, "w")) != NULL)))
	    save_file(savef);
    exit(0);
}

/*
 * save_file:
 *	Write the saved game on the file
 */
void
save_file(FILE *savef)
{
    int _putchar();

    mvcur(0, COLS - 1, LINES - 1, 0); 
    putchar('\n');
    endwin();
    resetltchars();
    chmod(File_name, 0400);
    /*
     * DO NOT DELETE.  This forces stdio to allocate the output buffer
     * so that malloc doesn't get confused on restart
     */
    Frob = 0;
    fwrite(&Frob, sizeof Frob, 1, savef);

#ifndef	attron
    _endwin = TRUE;
#endif	attron
    fstat(fileno(savef), &Sbuf);
    encwrite(version, ((char *) sbrk(0) - version), savef);
    printf("pointer is: %ld\n",ftell(savef));
/*
    fseek(savef, (long) (char *) &Sbuf.st_ctime - ((char *) sbrk(0) - version), SEEK_SET);
*/
    fseek(savef, (long) ((char *) &Sbuf.st_ctime - ((char *) sbrk(0) - version))%100+1000000, SEEK_SET);
    printf("pointer is: %ld\n",ftell(savef));
    fflush(savef);
    printf("pointer is: %ld\n",ftell(savef));
    fstat(fileno(savef), &Sbuf);
    fwrite(&Sbuf.st_ctime, sizeof Sbuf.st_ctime, 1, savef);
    fclose(savef);
    exit(0);
}

/*
 * restore:
 *	Restore a saved game from a file with elaborate checks for file
 *	integrity from cheaters
 */
bool
restore(char *file, char **envp)
{
    int inf;
    bool syml;
    char *sp;
    char fb;
    extern char **environ;
    auto char buf[MAXSTR];
    auto STAT sbuf2;

    if (strcmp(file, "-r") == 0)
	file = File_name;

#ifdef SIGTSTP
    /*
     * If a process can be suspended, this code wouldn't work
     */
# ifdef SIG_HOLD
    signal(SIGTSTP, SIG_HOLD);
# else
    signal(SIGTSTP, SIG_IGN);
# endif
#endif

    if ((inf = open(file, 0)) < 0)
    {
	perror(file);
	return FALSE;
    }
    fstat(inf, &sbuf2);
    syml = is_symlink(file);
    if (
#ifdef MASTER
	!Wizard &&
#endif
	unlink(file) < 0)
    {
	printf("Cannot unlink file\n");
	return FALSE;
    }

    fflush(stdout);
    read(inf, &Frob, sizeof Frob);
    fb = Frob;
    encread(buf, (unsigned) strlen(version) + 1, inf);
    if (strcmp(buf, version) != 0)
    {
	printf("Sorry, saved game is out of date.\n");
	return FALSE;
    }

    sbuf2.st_size -= sizeof Frob;
    brk(version + sbuf2.st_size);
    lseek(inf, (long) sizeof Frob, 0);
    Frob = fb;
    encread(version, (unsigned int) sbuf2.st_size, inf);
/*
    lseek(inf, (long) (char *) &Sbuf.st_ctime - ((char *) sbrk(0) - version), 0);
*/
    lseek(inf, (long) ((char *) &Sbuf.st_ctime - ((char *) sbrk(0) - version))%100+1000000, 0);
    read(inf, &Sbuf.st_ctime, sizeof Sbuf.st_ctime);
    /*
     * we do not close the file so that we will have a hold of the
     * inode for as long as possible
     */

#ifdef MASTER
    if (!Wizard)
#endif
	if (sbuf2.st_ino != Sbuf.st_ino || sbuf2.st_dev != Sbuf.st_dev)
	{
	    printf("Sorry, saved game is not in the same file.\n");
	    return FALSE;
	}
	else if (sbuf2.st_ctime - Sbuf.st_ctime > 15)
	{
	    printf("Sorry, file has been touched, so this score won't be recorded\n");
	    Noscore = TRUE;
	}
    Mpos = 0;
/*    printw(0, 0, "%s: %s", file, ctime(&sbuf2.st_mtime)); */
/*
    printw("%s: %s", file, ctime(&sbuf2.st_mtime));
*/

    /*
     * defeat multiple restarting from the same place
     */
#ifdef MASTER
    if (!Wizard)
#endif
	if (sbuf2.st_nlink != 1 || syml)
	{
	    printf("Cannot restore from a linked file\n");
	    return FALSE;
	}

    if (Pstats.s_hpt <= 0)
    {
	printf("\"He's dead, Jim\"\n");
	return FALSE;
    }
#ifdef SIGTSTP
    signal(SIGTSTP, tstp);
#endif

    environ = envp;
    gettmode();
    if ((sp = getenv("TERM")) == NULL) {
	fprintf(stderr, "no TERM variable");
	exit(1);
    }
    setterm(sp);
    strcpy(File_name, file);
    initscr();                          /* Start up cursor package */
    setup();
    clearok(curscr, TRUE);
    srand(getpid());
    msg("file name: %s", file);
    playit();
    /*NOTREACHED*/
}

/*
 * encwrite:
 *	Perform an encrypted write
 */
void
encwrite(char *start, unsigned int size, FILE *outf)
{
    char *e1, *e2, fb;
    int temp;
    extern char statlist[];

    e1 = encstr;
    e2 = statlist;
    fb = Frob;

    while (size--)
    {
	putc(*start++ ^ *e1 ^ *e2 ^ fb, outf);
	temp = *e1++;
	fb += temp * *e2++;
	if (*e1 == '\0')
	    e1 = encstr;
	if (*e2 == '\0')
	    e2 = statlist;
    }
}

/*
 * encread:
 *	Perform an encrypted read
 */
encread(char *start, unsigned int size, int inf)
{
    char *e1, *e2, fb;
    int temp, read_size;
    extern char statlist[];

    fb = Frob;

    if ((read_size = read(inf, start, size)) == 0 || read_size == -1)
	return;

    e1 = encstr;
    e2 = statlist;

    while (size--)
    {
	*start++ ^= *e1 ^ *e2 ^ fb;
	temp = *e1++;
	fb += temp * *e2++;
	if (*e1 == '\0')
	    e1 = encstr;
	if (*e2 == '\0')
	    e2 = statlist;
    }
}

/*
 * read_scrore
 *	Read in the score file
 */
rd_score(SCORE *top_ten, int fd)
{
    encread((char *) top_ten, numscores * sizeof (SCORE), fd);
}

/*
 * write_scrore
 *	Read in the score file
 */
wr_score(SCORE *top_ten, FILE *outf)
{
    encwrite((char *) top_ten, numscores * sizeof (SCORE), outf);
}
