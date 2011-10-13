/*
 * Score file structure
 *
 * @(#)netwait.h	4.6 (Berkeley) 02/05/99
 */

struct sc_ent {
    unsigned int sc_uid;
    unsigned short sc_score;
    unsigned int sc_flags;
    unsigned short sc_monster;
    char sc_name[MAXSTR];
    unsigned short sc_level;
    long sc_time;
};

typedef struct sc_ent SCORE;
