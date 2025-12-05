#include "ui_osp2be.h"

/* Player category controls */
const osp2beCvarDef_t osp2bePlayerControls[] = {
	{ "Select Model", "model", OSP2BE_CONTROL_BUTTON, OSP2BE_SPECIAL_MODEL_SELECTOR },
	{ "Head", "cg_playerModelColors", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_MODEL_HEAD },
	{ "Torso", "cg_playerModelColors", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_MODEL_TORSO },
	{ "Legs", "cg_playerModelColors", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_MODEL_LEGS },
	{ "Rail Beam", "cg_playerRailColors", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_RAIL_BEAM },
	{ "Rail Ring", "cg_playerRailColors", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_RAIL_RING },
	{ "Outline", "cg_teamOutlineColor", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_COLOR_GENERIC },
	{ "Size", "cg_teamOutlineSize", OSP2BE_CONTROL_SIZE_BUTTON, OSP2BE_SPECIAL_OUTLINE_SIZE },
	{ "Frozen", "cg_playerFrozenColor", OSP2BE_CONTROL_COLOR_BUTTON, OSP2BE_SPECIAL_COLOR_GENERIC }
};

const int osp2bePlayerControlsCount = 9;

