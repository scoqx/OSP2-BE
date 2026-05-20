#include "ui_osp2be.h"

/* General category controls */
const osp2beCvarDef_t osp2beGeneralControls[] =
{
	{ "Sensitivity", "sensitivity", OSP2BE_CONTROL_SLIDER, OSP2BE_SPECIAL_SLIDER_SENSITIVITY },
	{ "Field of View", "cg_fov", OSP2BE_CONTROL_SLIDER, OSP2BE_SPECIAL_SLIDER_FOV }
};

const int osp2beGeneralControlsCount = ARRAY_LEN(osp2beGeneralControls);

