#include "ui_osp2be.h"

/* Teams category controls */
const osp2beCvarDef_t osp2beTeamsControls[] =
{
	{ "Select Model", "cg_teamModel", OSP2BE_CONTROL_BUTTON, OSP2BE_SPECIAL_MODEL_SELECTOR },
	{ "Default", "cg_teamModel", OSP2BE_CONTROL_BUTTON, OSP2BE_SPECIAL_MODEL_DEFAULT },
	{ "Head", "cg_teamModelColors", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_MODEL_HEAD },
	{ "Torso", "cg_teamModelColors", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_MODEL_TORSO },
	{ "Legs", "cg_teamModelColors", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_MODEL_LEGS },
	{ "Rail Beam", "cg_teamRailColors", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_RAIL_BEAM },
	{ "Rail Ring", "cg_teamRailColors", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_RAIL_RING },
	{ "Outline", "cg_teamOutlineColor", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_COLOR_GENERIC },
	{ "Size", "cg_teamOutlineSize", OSP2BE_CONTROL_SIZE_BUTTON, OSP2BE_SPECIAL_OUTLINE_SIZE },
	{ "Frozen", "cg_teamFrozenColor", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_COLOR_GENERIC },
	{ "Select Model", "cg_enemyModel", OSP2BE_CONTROL_BUTTON, OSP2BE_SPECIAL_MODEL_SELECTOR },
	{ "Default", "cg_enemyModel", OSP2BE_CONTROL_BUTTON, OSP2BE_SPECIAL_MODEL_DEFAULT },
	{ "Head", "cg_enemyModelColors", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_MODEL_HEAD },
	{ "Torso", "cg_enemyModelColors", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_MODEL_TORSO },
	{ "Legs", "cg_enemyModelColors", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_MODEL_LEGS },
	{ "Rail Beam", "cg_enemyRailColors", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_RAIL_BEAM },
	{ "Rail Ring", "cg_enemyRailColors", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_RAIL_RING },
	{ "Outline", "cg_enemyOutlineColor", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_COLOR_GENERIC },
	{ "Size", "cg_enemyOutlineSize", OSP2BE_CONTROL_SIZE_BUTTON, OSP2BE_SPECIAL_OUTLINE_SIZE },
	{ "Frozen", "cg_enemyFrozenColor", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_COLOR_GENERIC }
};

const int osp2beTeamsControlsCount = 20;

