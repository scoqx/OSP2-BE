#include "ui_osp2be.h"

/* Explosives category controls */
const osp2beCvarDef_t osp2beExplosivesControls[] =
{
	/* General */
	{ "General", NULL, OSP2BE_CONTROL_FIELD, OSP2BE_SPECIAL_SECTION_HEADER },
	{ "Delag", "cg_delag", OSP2BE_CONTROL_FIELD, OSP2BE_SPECIAL_NONE },

	/* Shotgun */
	{ "Shotgun", NULL, OSP2BE_CONTROL_FIELD, OSP2BE_SPECIAL_SECTION_HEADER },
	{ "Smoke", "cg_smoke_sg", OSP2BE_CONTROL_TOGGLE, OSP2BE_SPECIAL_TOGGLE_CHECKBOX },
	{ "Tracer", "cg_shotGunTracer", OSP2BE_CONTROL_TOGGLE, OSP2BE_SPECIAL_TOGGLE_CHECKBOX },

	/* Grenade Launcher */
	{ "Grenade Launcher", NULL, OSP2BE_CONTROL_FIELD, OSP2BE_SPECIAL_SECTION_HEADER },
	{ "Alt Grenades", "cg_altGrenades", OSP2BE_CONTROL_FIELD, OSP2BE_SPECIAL_NONE },
	{ "Alt Color", "cg_altGrenadesColor", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_COLOR_GENERIC },
	{ "Enemy Color", "cg_enemyGrenadesColor", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_COLOR_GENERIC },

	/* Rocket Launcher */
	{ "Rocket Launcher", NULL, OSP2BE_CONTROL_FIELD, OSP2BE_SPECIAL_SECTION_HEADER },
	{ "Old Rocket", "cg_oldRocket", OSP2BE_CONTROL_TOGGLE, OSP2BE_SPECIAL_TOGGLE_CHECKBOX },
	{ "Smoke Growth", "cg_smokegrowth_rl", OSP2BE_CONTROL_FIELD, OSP2BE_SPECIAL_NONE },
	{ "Smoke Radius", "cg_smokeradius_rl", OSP2BE_CONTROL_FIELD, OSP2BE_SPECIAL_NONE },
	{ "Smoke Density", "cg_smokedensity_rl", OSP2BE_CONTROL_FIELD, OSP2BE_SPECIAL_NONE },

	/* Plasma Gun */
	{ "Plasma Gun", NULL, OSP2BE_CONTROL_FIELD, OSP2BE_SPECIAL_SECTION_HEADER },
	{ "Old Plasma", "cg_oldPlasma", OSP2BE_CONTROL_TOGGLE, OSP2BE_SPECIAL_TOGGLE_CHECKBOX },
	{ "Alt Plasma", "cg_altPlasma", OSP2BE_CONTROL_FIELD, OSP2BE_SPECIAL_NONE }
};

const int osp2beExplosivesControlsCount = ARRAY_LEN(osp2beExplosivesControls);

