/*
 * global variable initializaton
 *
 * @(#)rinit.c	4.31 (Berkeley) 02/05/99
 */

#include <curses.h>
#include <ctype.h>
#include "netprot.h"

/*
 * init_player:
 *	Roll her up
 */
void
init_player(void)
{
    THING *obj;

    Pstats = Max_stats;
    Food_left = HUNGERTIME;
    /*
     * Give him some food
     */
    obj = new_item();
    obj->o_type = FOOD;
    obj->o_count = 1;
    add_pack(obj, TRUE);
    /*
     * And his suit of armor
     */
    obj = new_item();
    obj->o_type = ARMOR;
    obj->o_which = RING_MAIL;
    obj->o_arm = A_class[RING_MAIL] - 1;
    obj->o_flags |= ISKNOW;
    obj->o_count = 1;
    Cur_armor = obj;
    add_pack(obj, TRUE);
    /*
     * Give him his weaponry.  First a mace.
     */
    obj = new_item();
    init_weapon(obj, MACE);
    obj->o_hplus = 1;
    obj->o_dplus = 1;
    obj->o_flags |= ISKNOW;
    add_pack(obj, TRUE);
    Cur_weapon = obj;
    /*
     * Now a +1 bow
     */
    obj = new_item();
    init_weapon(obj, BOW);
    obj->o_hplus = 1;
    obj->o_flags |= ISKNOW;
    add_pack(obj, TRUE);
    /*
     * Now some arrows
     */
    obj = new_item();
    init_weapon(obj, ARROW);
    obj->o_count = rnd(15) + 25;
    obj->o_flags |= ISKNOW;
    add_pack(obj, TRUE);
}

/*
 * Contains defintions and functions for dealing with things like
 * potions and scrolls
 */

static char *Rainbow[] = {
    "amber",
    "aquamarine",
    "black",
    "blue",
    "brown",
    "clear",
    "crimson",
    "cyan",
    "ecru",
    "gold",
    "green",
    "grey",
    "magenta",
    "orange",
    "pink",
    "plaid",
    "purple",
    "red",
    "silver",
    "tan",
    "tangerine",
    "topaz",
    "turquoise",
    "vermilion",
    "violet",
    "white",
    "yellow",
};

#define NCOLORS (sizeof Rainbow / sizeof (char *))

static char *Sylls[] = {
    "a", "ab", "ag", "aks", "ala", "an", "app", "arg", "arze", "ash",
    "bek", "bie", "bit", "bjor", "blu", "bot", "bu", "byt", "comp",
    "con", "cos", "cre", "dalf", "dan", "den", "do", "e", "eep", "el",
    "eng", "er", "ere", "erk", "esh", "evs", "fa", "fid", "fri", "fu",
    "gan", "gar", "glen", "gop", "gre", "ha", "hyd", "i", "ing", "ip",
    "ish", "it", "ite", "iv", "jo", "kho", "kli", "klis", "la", "lech",
    "mar", "me", "mi", "mic", "mik", "mon", "mung", "mur", "nej",
    "nelg", "nep", "ner", "nes", "nes", "nih", "nin", "o", "od", "ood",
    "org", "orn", "ox", "oxy", "pay", "ple", "plu", "po", "pot",
    "prok", "re", "rea", "rhov", "ri", "ro", "rog", "rok", "rol", "sa",
    "san", "sat", "sef", "seh", "shu", "ski", "sna", "sne", "snik",
    "sno", "so", "sol", "sri", "sta", "sun", "ta", "tab", "tem",
    "ther", "ti", "tox", "trol", "tue", "turs", "u", "ulk", "um", "un",
    "uni", "ur", "val", "viv", "vly", "vom", "wah", "wed", "werg",
    "wex", "whon", "wun", "xo", "y", "yot", "yu", "zant", "zeb", "zim",
    "zok", "zon", "zum",
};

typedef struct {
    char	*st_name;
    int		st_value;
} STONE;

static STONE Stones[] = {
    { "agate",		 25},
    { "alexandrite",	 40},
    { "amethyst",	 50},
    { "carnelian",	 40},
    { "diamond",	300},
    { "emerald",	300},
    { "germanium",	225},
    { "granite",	  5},
    { "garnet",		 50},
    { "jade",		150},
    { "kryptonite",	300},
    { "lapis lazuli",	 50},
    { "moonstone",	 50},
    { "obsidian",	 15},
    { "onyx",		 60},
    { "opal",		200},
    { "pearl",		220},
    { "peridot",	 63},
    { "ruby",		350},
    { "sapphire",	285},
    { "stibotantalite",	200},
    { "tiger eye",	 50},
    { "topaz",		 60},
    { "turquoise",	 70},
    { "taaffeite",	300},
    { "zircon",	 	 80},
};

#define NSTONES (sizeof Stones / sizeof (STONE))

static char *Wood[] = {
    "avocado wood",
    "balsa",
    "bamboo",
    "banyan",
    "birch",
    "cedar",
    "cherry",
    "cinnibar",
    "cypress",
    "dogwood",
    "driftwood",
    "ebony",
    "elm",
    "eucalyptus",
    "fall",
    "hemlock",
    "holly",
    "ironwood",
    "kukui wood",
    "mahogany",
    "manzanita",
    "maple",
    "oaken",
    "persimmon wood",
    "pecan",
    "pine",
    "poplar",
    "redwood",
    "rosewood",
    "spruce",
    "teak",
    "walnut",
    "zebrawood",
};

#define NWOOD (sizeof Wood / sizeof (char *))

static char *Metal[] = {
    "aluminum",
    "beryllium",
    "bone",
    "brass",
    "bronze",
    "copper",
    "electrum",
    "gold",
    "iron",
    "lead",
    "magnesium",
    "mercury",
    "nickel",
    "pewter",
    "platinum",
    "steel",
    "silver",
    "silicon",
    "tin",
    "titanium",
    "tungsten",
    "zinc",
};

#define NMETAL (sizeof Metal / sizeof (char *))

#define MAX3(a,b,c)	(a > b ? (a > c ? a : c) : (b > c ? b : c))

static bool Used[MAX3(NCOLORS, NSTONES, NWOOD)];

/*
 * init_colors:
 *	Initialize the potion color scheme for this time
 */
void
init_colors(void)
{
    int i, j;

    for (i = 0; i < NCOLORS; i++)
	Used[i] = FALSE;
    for (i = 0; i < MAXPOTIONS; i++)
    {
	do
	    j = rnd(NCOLORS);
	until (!Used[j]);
	Used[j] = TRUE;
	P_colors[i] = Rainbow[j];
    }
}

/*
 * init_names:
 *	Generate the names of the various scrolls
 */
#define MAXNAME	40	/* Max number of characters in a name */

void
init_names(void)
{
    int nsyl;
    char *cp, *sp;
    int i, nwords;

    for (i = 0; i < MAXSCROLLS; i++)
    {
	cp = Prbuf;
	nwords = rnd(3) + 2;
	while (nwords--)
	{
	    nsyl = rnd(3) + 1;
	    while (nsyl--)
	    {
		sp = Sylls[rnd((sizeof Sylls) / (sizeof (char *)))];
		if (&cp[strlen(sp)] > &Prbuf[MAXNAME])
			break;
		while (*sp)
		    *cp++ = *sp++;
	    }
	    *cp++ = ' ';
	}
	*--cp = '\0';
	S_names[i] = (char *) malloc((unsigned) strlen(Prbuf)+1);
	strcpy(S_names[i], Prbuf);
    }
}

/*
 * init_stones:
 *	Initialize the ring stone setting scheme for this time
 */
void
init_stones(void)
{
    int i, j;

    for (i = 0; i < NSTONES; i++)
	Used[i] = FALSE;
    for (i = 0; i < MAXRINGS; i++)
    {
	do
	    j = rnd(NSTONES);
	until (!Used[j]);
	Used[j] = TRUE;
	R_stones[i] = Stones[j].st_name;
	Ring_info[i].oi_worth += Stones[j].st_value;
    }
}

/*
 * init_materials:
 *	Initialize the construction materials for wands and staffs
 */
void
init_materials(void)
{
    int i, j;
    char *str;
    static bool metused[NMETAL];

    for (i = 0; i < NWOOD; i++)
	Used[i] = FALSE;
    for (i = 0; i < NMETAL; i++)
	metused[i] = FALSE;
    for (i = 0; i < MAXSTICKS; i++)
    {
	for (;;)
	    if (rnd(2) == 0)
	    {
		j = rnd(NMETAL);
		if (!metused[j])
		{
		    Ws_type[i] = "wand";
		    str = Metal[j];
		    metused[j] = TRUE;
		    break;
		}
	    }
	    else
	    {
		j = rnd(NWOOD);
		if (!Used[j])
		{
		    Ws_type[i] = "staff";
		    str = Wood[j];
		    Used[j] = TRUE;
		    break;
		}
	    }
	Ws_made[i] = str;
    }
}

#ifdef MASTER
# define	NT	NUMTHINGS, "things"
# define	MP	MAXPOTIONS, "potions"
# define	MS	MAXSCROLLS, "scrolls"
# define	MR	MAXRINGS, "rings"
# define	MWS	MAXSTICKS, "sticks"
# define	MW	MAXWEAPONS, "weapons"
# define	MA	MAXARMORS, "armor"
#else
# define	NT	NUMTHINGS
# define	MP	MAXPOTIONS
# define	MS	MAXSCROLLS
# define	MR	MAXRINGS
# define	MWS	MAXSTICKS
# define	MW	MAXWEAPONS
# define	MA	MAXARMORS
#endif

/*
 * sumprobs:
 *	Sum up the probabilities for items appearing
 */
void
sumprobs(struct obj_info *info, int bound
#ifdef MASTER
	, char *name
#endif
)
{
    struct obj_info *last, *endp, *start;

    start = info;
    endp = info + bound;
    while (++info < endp)
	info->oi_prob += (info - 1)->oi_prob;
#ifdef MASTER
    badcheck(name, start, bound);
#endif
}

/*
 * init_probs:
 *	Initialize the probabilities for the various items
 */
void
init_probs(void)
{
    sumprobs(Things, NT);
    sumprobs(Pot_info, MP);
    sumprobs(Scr_info, MS);
    sumprobs(Ring_info, MR);
    sumprobs(Ws_info, MWS);
    sumprobs(Weap_info, MW);
    sumprobs(Arm_info, MA);
}

#ifdef MASTER
/*
 * badcheck:
 *	Check to see if a series of probabilities sums to 100
 */
void
badcheck(char *name, struct obj_info *info, int bound)
{
    struct obj_info *end;

    if (info[bound - 1].oi_prob == 100)
	return;
    printf("\nBad percentages for %s (bound = %d):\n", name, bound);
    for (end = &info[bound]; info < end; info++)
	printf("%3d%% %s\n", info->oi_prob, info->oi_name);
    printf("[hit RETURN to continue]");
    fflush(stdout);
    while (getchar() != '\n')
	continue;
}
#endif

/*
 * pick_color:
 *	If he is halucinating, pick a random color name and return it,
 *	otherwise return the given color.
 */
char *
pick_color(char *col)
{
    return (on(Player, ISHALU) ? Rainbow[rnd(NCOLORS)] : col);
}
