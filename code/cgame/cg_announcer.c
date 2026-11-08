/*
===========================================================================
Announcer captions (cg_announcerText): what the announcer says, as text.

For players who do not hear the game. Every sound the announcer plays, from this
module or from the server, puts a line into a feed of the last few; the SuperHUD
element "announcer" draws it.
===========================================================================
*/

#include "cg_local.h"

#define ANNOUNCER_DEDUP_MS  500     /* the same line again this soon is the same call: one sound from two sides */

typedef struct
{
	const char*     path;       /* as the server names it */
	const char*     text;
	sfxHandle_t*    handle;     /* as this module plays it, or NULL */
} announcerCaption_t;

static const announcerCaption_t cg_announcerCaptions[] =
{
	{ "sound/feedback/prepare.wav",             "Prepare to fight",             &cgs.media.countPrepareSound },
	{ "sound/feedback/three.wav",               "Three",                        &cgs.media.count3Sound },
	{ "sound/feedback/two.wav",                 "Two",                          &cgs.media.count2Sound },
	{ "sound/feedback/one.wav",                 "One",                          &cgs.media.count1Sound },
	{ "sound/feedback/fight.wav",               "Fight!",                       &cgs.media.countFightSound },
	{ "sound/feedback/5_minute.wav",            "Five minute warning",          &cgs.media.fiveMinuteSound },
	{ "sound/feedback/1_minute.wav",            "One minute warning",           &cgs.media.oneMinuteSound },
	{ "sound/feedback/sudden_death.wav",        "Sudden death",                 &cgs.media.suddenDeathSound },
	{ "sound/feedback/3_frags.wav",             "Three frags left",             &cgs.media.threeFragSound },
	{ "sound/feedback/2_frags.wav",             "Two frags left",               &cgs.media.twoFragSound },
	{ "sound/feedback/1_frag.wav",              "One frag left",                &cgs.media.oneFragSound },
	{ "sound/feedback/takenlead.wav",           "You have taken the lead",      &cgs.media.takenLeadSound },
	{ "sound/feedback/tiedlead.wav",            "You are tied for the lead",    &cgs.media.tiedLeadSound },
	{ "sound/feedback/lostlead.wav",            "You have lost the lead",       &cgs.media.lostLeadSound },
	{ "sound/feedback/redleads.wav",            "Red leads!",                   &cgs.media.redLeadsSound },
	{ "sound/feedback/blueleads.wav",           "Blue leads!",                  &cgs.media.blueLeadsSound },
	{ "sound/feedback/teamstied.wav",           "Teams are tied",               &cgs.media.teamsTiedSound },
	{ "sound/feedback/impressive.wav",          "Impressive",                   &cgs.media.impressiveSound },
	{ "sound/feedback/excellent.wav",           "Excellent",                    &cgs.media.excellentSound },
	{ "sound/feedback/denied.wav",              "Denied",                       &cgs.media.deniedSound },
	{ "sound/feedback/humiliation.wav",         "Humiliation",                  &cgs.media.humiliationSound },
	{ "sound/feedback/assist.wav",              "Assist",                       &cgs.media.assistSound },
	{ "sound/feedback/defense.wav",             "Defense",                      &cgs.media.defendSound },
	{ "sound/feedback/voc_holyshit.wav",        "Holy shit",                    &cgs.media.holyShitSound },
	{ "sound/feedback/laststanding.wav",        "Last standing",                &cgs.media.lastStandingSound },
	{ "sound/teamplay/voc_red_scores.wav",      "Red scores",                   &cgs.media.redScoredSound },
	{ "sound/teamplay/voc_blue_scores.wav",     "Blue scores",                  &cgs.media.blueScoredSound },
	{ "sound/teamplay/voc_you_flag.wav",        "You have the flag",            &cgs.media.youHaveFlagSound },
	{ "sound/teamplay/voc_team_flag.wav",       "Your team has the enemy flag", &cgs.media.yourTeamTookEnemyFlagSound },
	{ "sound/teamplay/voc_enemy_flag.wav",      "The enemy has your flag",      &cgs.media.enemyTookYourFlagSound },
	{ "sound/teamplay/voc_team_1flag.wav",      "Your team has the flag",       &cgs.media.yourTeamTookTheFlagSound },
	{ "sound/teamplay/voc_enemy_1flag.wav",     "The enemy has the flag",       &cgs.media.enemyTookTheFlagSound },
	{ "sound/teamplay/voc_red_returned.wav",    "Red flag returned",            &cgs.media.redFlagReturnedSound },
	{ "sound/teamplay/voc_blue_returned.wav",   "Blue flag returned",           &cgs.media.blueFlagReturnedSound },
	{ "sound/teamplay/flagcapture_yourteam.wav", "Your team captured the flag", &cgs.media.captureYourTeamSound },
	{ "sound/teamplay/flagcapture_opponent.wav", "The enemy captured the flag", &cgs.media.captureOpponentSound },
	{ "sound/teamplay/flagtaken_yourteam.wav",  "Your flag was taken",          &cgs.media.takenYourTeamSound },
	{ "sound/teamplay/flagtaken_opponent.wav",  "The enemy flag was taken",     &cgs.media.takenOpponentSound },
	{ "sound/teamplay/flagreturn_yourteam.wav", "Your flag was returned",       &cgs.media.returnYourTeamSound },
	{ "sound/teamplay/flagreturn_opponent.wav", "The enemy flag was returned",  &cgs.media.returnOpponentSound },
};

#define ANNOUNCER_NUM_CAPTIONS ((int)(sizeof(cg_announcerCaptions) / sizeof(cg_announcerCaptions[0])))

typedef struct
{
	const char* text;
	int         time;
} announcerLine_t;

static announcerLine_t  cg_announcerLines[ANNOUNCER_MAX_LINES];
static int              cg_announcerNext;     /* where the next line goes */

void CG_AnnouncerClear(void)
{
	memset(cg_announcerLines, 0, sizeof(cg_announcerLines));
	cg_announcerNext = 0;
}

/* A line into the feed */
void CG_AnnouncerText(const char* text)
{
	const announcerLine_t* last;

	if (!cg_announcerText.integer || !text || !text[0])
	{
		return;
	}
	last = &cg_announcerLines[(cg_announcerNext + ANNOUNCER_MAX_LINES - 1) % ANNOUNCER_MAX_LINES];
	if (last->text == text && cg.time - last->time >= 0 && cg.time - last->time < ANNOUNCER_DEDUP_MS)
	{
		return;
	}
	cg_announcerLines[cg_announcerNext].text = text;
	cg_announcerLines[cg_announcerNext].time = cg.time;
	cg_announcerNext = (cg_announcerNext + 1) % ANNOUNCER_MAX_LINES;
}

/* The line said age lines ago (0 the newest), and its time; NULL when there is none */
const char* CG_AnnouncerLine(int age, int* time)
{
	const announcerLine_t* line;

	if (age < 0 || age >= ANNOUNCER_MAX_LINES)
	{
		return NULL;
	}
	line = &cg_announcerLines[(cg_announcerNext + ANNOUNCER_MAX_LINES - 1 - age) % ANNOUNCER_MAX_LINES];
	/* time going back (a killcam, a map restart) leaves lines from the future: they are not said yet */
	if (!line->text || line->time > cg.time)
	{
		return NULL;
	}
	*time = line->time;
	return line->text;
}

/* A sound this module plays */
void CG_AnnouncerSound(sfxHandle_t sfx)
{
	int i;

	if (!sfx || !cg_announcerText.integer)
	{
		return;
	}
	for (i = 0; i < ANNOUNCER_NUM_CAPTIONS; i++)
	{
		if (*cg_announcerCaptions[i].handle == sfx)
		{
			CG_AnnouncerText(cg_announcerCaptions[i].text);
			return;
		}
	}
}

/* A sound the server plays, by its name */
void CG_AnnouncerSoundName(const char* name)
{
	char    want[MAX_QPATH];
	char    have[MAX_QPATH];
	int     i;

	if (!name || !name[0] || !cg_announcerText.integer)
	{
		return;
	}
	COM_StripExtension(name, want);
	for (i = 0; i < ANNOUNCER_NUM_CAPTIONS; i++)
	{
		COM_StripExtension(cg_announcerCaptions[i].path, have);
		if (!Q_stricmp(want, have))
		{
			CG_AnnouncerText(cg_announcerCaptions[i].text);
			return;
		}
	}
}

/* The beeps before a round: the seconds that are left, then the start */
void CG_AnnouncerCountdown(int secondsLeft)
{
	static const char* const words[] = { "Fight!", "One", "Two", "Three", "Four", "Five" };

	if (secondsLeft >= 0 && secondsLeft < (int)(sizeof(words) / sizeof(words[0])))
	{
		CG_AnnouncerText(words[secondsLeft]);
	}
}
