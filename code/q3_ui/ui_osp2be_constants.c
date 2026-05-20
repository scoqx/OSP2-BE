#include "ui_osp2be_shared.h"

/* Layout parameters */
osp2beLayoutParams_t osp2be_layout =
{
	640,  /* design_width */
	480,  /* design_height */
	320,  /* design_center_x */
	704,  /* single_page_bg_width */
	0,    /* margin_left */
	0,    /* margin_right */
	0,    /* margin_top */
	0,    /* margin_bottom */
	90,   /* sidebar_width */
	4,    /* content_gap */
	320,  /* form_width */
	20,   /* field_padding */
	24,   /* field_width_chars */
	16,   /* color_swatch_size */
	4,    /* color_swatch_gap */
	192,  /* preview_width */
	240,  /* preview_height */
	64    /* preview_y */
};

/* Button parameters */
osp2beButtonParams_t osp2be_button =
{
	18,  /* height */
	3,   /* default_width_multiplier */
	56,  /* weapon_width */
	3,   /* gap */
	4    /* weapon_spacing */
};

/* Row parameters */
osp2beRowParams_t osp2be_row =
{
	20,  /* height */
	4,   /* gap_max */
	8    /* weapon_offset */
};

/* Square/Color swatch parameters */
osp2beSquareParams_t osp2be_square =
{
	6,   /* size_offset */
	12,  /* size_min */
	4,   /* size_fallback_offset */
	1,   /* border_offset */
	12,  /* swatch_padding */
	8,   /* swatch_padding_head */
	14   /* swatch_rail_extra_width */
};

/* Icon parameters */
osp2beIconParams_t osp2be_icon =
{
	2,   /* size_offset */
	16,  /* size_min */
	14,  /* size_min_head */
	4    /* gap */
};

/* Position offsets */
osp2bePositionParams_t osp2be_position =
{
	4,   /* square_x_offset */
	8,   /* content_gap */
	2,   /* square_x_offset_weapon */
	4,   /* content_gap_weapon */
	2,   /* weapon_icon_right_margin */
	2,   /* model_icon_x_offset */
	52,  /* label_x_offset */
	16   /* label_y_offset_calc_base */
};

/* Text parameters */
osp2beTextParams_t osp2be_text =
{
	16   /* char_height */
};

/* Background alpha parameters */
osp2beBackgroundAlphaParams_t osp2be_bg_alpha =
{
	0.0f,   /* bg_ingame */
	1.0f,   /* bg_menu */
	0.65f,  /* header_ingame */
	0.4f,   /* header_menu */
	0.4f,   /* params_ingame */
	0.90f,  /* params_menu */
	0.4f,   /* preview_ingame */
	0.90f   /* preview_menu */
};

/* Background layout parameters */
osp2beBackgroundLayoutParams_t osp2be_bg_layout =
{
	18,  /* header_padding_left */
	18,  /* header_padding_right */
	-4,  /* params_side_padding */
	-4,  /* params_top_offset */
	32   /* params_bottom_offset */
};

/* Font parameters */
osp2beFontParams_t osp2be_font =
{
	0,              /* index */
	DS_PROPORTIONAL  /* flags */
};

/* Preview button parameters */
osp2bePreviewButtonParams_t osp2be_preview_button =
{
	0.12f,  /* bg_r */
	0.12f,  /* bg_g */
	0.12f,  /* bg_b */
	1.0f,   /* border_r */
	1.0f,   /* border_g */
	1.0f,   /* border_b */
	0.3f,   /* border_inactive_r */
	0.3f,   /* border_inactive_g */
	0.3f,   /* border_inactive_b */
	0.85f,  /* bg_alpha_active */
	0.5f    /* bg_alpha_inactive */
};

