/* Internal shared definitions for OSP2BE menu modules */
#ifndef UI_OSP2BE_SHARED_H
#define UI_OSP2BE_SHARED_H

#include "ui_osp2be.h"

/* Layout parameters structure */
typedef struct
{
	int design_width;
	int design_height;
	int design_center_x;
	int single_page_bg_width;
	int margin_left;
	int margin_right;
	int margin_top;
	int margin_bottom;
	int sidebar_width;
	int content_gap;
	int form_width;
	int field_padding;
	int field_width_chars;
	int color_swatch_size;
	int color_swatch_gap;
	int preview_width;
	int preview_height;
	int preview_y;
} osp2beLayoutParams_t;

/* Button parameters structure */
typedef struct
{
	int height;
	int default_width_multiplier;
	int weapon_width;
	int gap;
	int weapon_spacing;
} osp2beButtonParams_t;

/* Row parameters structure */
typedef struct
{
	int height;
	int gap_max;
	int weapon_offset;
} osp2beRowParams_t;

/* Square/Color swatch parameters structure */
typedef struct
{
	int size_offset;
	int size_min;
	int size_fallback_offset;
	int border_offset;
	int swatch_padding;
	int swatch_padding_head;
	int swatch_rail_extra_width;
} osp2beSquareParams_t;

/* Icon parameters structure */
typedef struct
{
	int size_offset;
	int size_min;
	int size_min_head;
	int gap;
} osp2beIconParams_t;

/* Position offsets structure */
typedef struct
{
	int square_x_offset;
	int content_gap;
	int square_x_offset_weapon;
	int content_gap_weapon;
	int weapon_icon_right_margin;
	int model_icon_x_offset;
	int label_x_offset;
	int label_y_offset_calc_base;
} osp2bePositionParams_t;

/* Text parameters structure */
typedef struct
{
	int char_height;
} osp2beTextParams_t;

/* Background alpha parameters structure */
typedef struct
{
	float bg_ingame;
	float bg_menu;
	float header_ingame;
	float header_menu;
	float params_ingame;
	float params_menu;
	float preview_ingame;
	float preview_menu;
} osp2beBackgroundAlphaParams_t;

/* Background layout parameters structure */
typedef struct
{
	int header_padding_left;
	int header_padding_right;
	int params_side_padding;
	int params_top_offset;
	int params_bottom_offset;
} osp2beBackgroundLayoutParams_t;

/* Font parameters structure */
typedef struct
{
	int index;
	int flags;
} osp2beFontParams_t;

/* Preview button parameters structure */
typedef struct
{
	float bg_r;
	float bg_g;
	float bg_b;
	float border_r;
	float border_g;
	float border_b;
	float border_inactive_r;
	float border_inactive_g;
	float border_inactive_b;
	float bg_alpha_active;
	float bg_alpha_inactive;
} osp2bePreviewButtonParams_t;

/* Theme colors structure */
typedef struct
{
	/* Button colors */
	vec4_t button_bg;
	vec4_t button_border_normal;
	vec4_t button_border_focus;
	vec4_t button_default_color;
	vec4_t button_icon_disabled;
	vec4_t button_icon_enabled;

	/* Slider colors */
	vec4_t slider_track_normal;
	vec4_t slider_track_focus;
	vec4_t slider_track_disabled;
	vec4_t slider_border_normal;
	vec4_t slider_border_focus;
	vec4_t slider_border_disabled;
	vec4_t slider_tick_normal;
	vec4_t slider_tick_focus;
	vec4_t slider_tick_disabled;
	vec4_t slider_handle_normal;
	vec4_t slider_handle_focus;
	vec4_t slider_handle_disabled;

	/* Toggle/Checkbox colors */
	vec4_t toggle_bg_normal;
	vec4_t toggle_bg_focus;
	vec4_t toggle_border_normal;
	vec4_t toggle_border_focus;
	vec4_t toggle_text_checked;
	vec4_t toggle_text_unchecked;

	/* Category list colors */
	vec4_t category_bg_normal;
	vec4_t category_bg_hover;
	vec4_t category_bg_selected;
	vec4_t category_border_normal;
	vec4_t category_border_hover;
	vec4_t category_border_selected;
	vec4_t category_text_normal;
	vec4_t category_text_hover;
	vec4_t category_text_selected;

	/* Preview button text colors */
	vec4_t preview_text_active;
	vec4_t preview_text_inactive;
	vec4_t preview_text_focus;
	vec4_t preview_icon_active;
	vec4_t preview_icon_inactive;
} osp2beThemeColors_t;

/* All parameters structures - defined in ui_osp2be_constants.c */
extern osp2beLayoutParams_t osp2be_layout;
extern osp2beButtonParams_t osp2be_button;
extern osp2beRowParams_t osp2be_row;
extern osp2beSquareParams_t osp2be_square;
extern osp2beIconParams_t osp2be_icon;
extern osp2bePositionParams_t osp2be_position;
extern osp2beTextParams_t osp2be_text;
extern osp2beBackgroundAlphaParams_t osp2be_bg_alpha;
extern osp2beBackgroundLayoutParams_t osp2be_bg_layout;
extern osp2beFontParams_t osp2be_font;
extern osp2bePreviewButtonParams_t osp2be_preview_button;
extern osp2beThemeColors_t osp2be_theme;

/* Global menu state - declared in ui_osp2be.c */
extern osp2beMenu_t s_osp2be;

#define ID_CATEGORY_LIST 10
#define ID_APPLY 20
#define ID_RELOAD 21
#define ID_CLOSE 22
#define ID_TOGGLE_BASE 200
#define ID_FIELD_BASE 300
#define ID_SLIDER_BASE 350
#define ID_PREVIEW_FROZEN 400
#define ID_TEAMS_ALLY 401
#define ID_TEAMS_ENEMY 402

#endif /* UI_OSP2BE_SHARED_H */


