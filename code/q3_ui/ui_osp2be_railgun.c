#include "ui_osp2be.h"

/* Railgun category controls */
const osp2beCvarDef_t osp2beRailgunControls[] =
{
	/* Railgun */
	{ "Railgun", NULL, OSP2BE_CONTROL_FIELD, OSP2BE_SPECIAL_SECTION_HEADER },
	{ "Rail Trail Time", "cg_railTrailTime", OSP2BE_CONTROL_FIELD, OSP2BE_SPECIAL_NONE },
	{ "Old Rail", "cg_oldRail", OSP2BE_CONTROL_TOGGLE, OSP2BE_SPECIAL_TOGGLE_CHECKBOX },
	{ "Team Rails", "cg_teamRails", OSP2BE_CONTROL_TOGGLE, OSP2BE_SPECIAL_TOGGLE_CHECKBOX },
	{ "Player Rail Colors", "cg_playerRailColors", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_COLOR_GENERIC },
	{ "Team Rail Colors", "cg_teamRailColors", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_COLOR_GENERIC },
	{ "Enemy Rail Colors", "cg_enemyRailColors", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_COLOR_GENERIC },
	{ "Rail Custom Chamber", "cg_railCustomChamber", OSP2BE_CONTROL_FIELD, OSP2BE_SPECIAL_NONE },
	{ "Rail Rings Radius", "cg_railRingsRadius", OSP2BE_CONTROL_FIELD, OSP2BE_SPECIAL_NONE },
	{ "Rail Rings Rotation", "cg_railRingsRotation", OSP2BE_CONTROL_FIELD, OSP2BE_SPECIAL_NONE },
	{ "Rail Rings Spacing", "cg_railRingsSpacing", OSP2BE_CONTROL_FIELD, OSP2BE_SPECIAL_NONE },
	{ "Rail Rings Size", "cg_railRingsSize", OSP2BE_CONTROL_FIELD, OSP2BE_SPECIAL_NONE },
	{ "Rail Static Rings", "cg_railStaticRings", OSP2BE_CONTROL_TOGGLE, OSP2BE_SPECIAL_TOGGLE_CHECKBOX },
	{ "Core Width", "r_railCoreWidth", OSP2BE_CONTROL_FIELD, OSP2BE_SPECIAL_NONE },
	{ "Segment Length", "r_railSegmentLength", OSP2BE_CONTROL_FIELD, OSP2BE_SPECIAL_NONE },
	{ "Rail Width", "r_railWidth", OSP2BE_CONTROL_FIELD, OSP2BE_SPECIAL_NONE }
};

const int osp2beRailgunControlsCount = ARRAY_LEN(osp2beRailgunControls);


