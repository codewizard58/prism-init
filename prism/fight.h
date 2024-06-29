#define FIGHT_TIMEOUT	30	/* fight finishes if no action for ... secs */

struct player_levels {
money pennies;
const char *mtitle, *ftitle;
int mhp;
};

extern struct player_levels player_level_table[];

/* end of fight.h */
