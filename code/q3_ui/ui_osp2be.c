#include "ui_local.h"

#define ART_BACK0			"menu/art/back_0"
#define ART_BACK1			"menu/art/back_1"

#define ID_BACK				100

typedef struct {
	menuframework_s	menu;
	menubitmap_s	back;
} osp2beMenu_t;

static osp2beMenu_t s_osp2be;

static const char *osp2beInfoLines[] = {
	"Welcome to the OSP2 Balance Edition hub.",
	"Explore upcoming competitive tweaks and beta tools.",
	"Feedback helps shape the next release."
};

static void Osp2Be_MenuEvent( void *ptr, int event ) {
	menucommon_s	*item;

	if ( event != QM_ACTIVATED ) {
		return;
	}

	item = (menucommon_s *)ptr;
	switch ( item->id ) {
	case ID_BACK:
		UI_PopMenu();
		break;
	}
}

static sfxHandle_t Osp2Be_MenuKey( int key ) {
	return Menu_DefaultKey( &s_osp2be.menu, key );
}


static void Osp2Be_MenuDraw( void ) {
	int		i;
	int		count;
	int		y;

	Menu_Draw( &s_osp2be.menu );

	UI_DrawProportionalString( 320, 120, "OSP2-BE", UI_CENTER|UI_BIGFONT, color_red );

	y = 200;
	count = ARRAY_LEN( osp2beInfoLines );
	for ( i = 0; i < count; i++ ) {
		UI_DrawProportionalString( 320, y, osp2beInfoLines[i], UI_CENTER|UI_SMALLFONT, text_color_normal );
		y += PROP_HEIGHT;
	}
}


void UI_Osp2BeMenu( void ) {
	unsigned char	*ptr;
	int				index;
	int				count;

	ptr = (unsigned char *)&s_osp2be;
	count = (int)sizeof( s_osp2be );
	for ( index = 0; index < count; index++ ) {
		ptr[index] = 0;
	}

	s_osp2be.menu.draw = Osp2Be_MenuDraw;
	s_osp2be.menu.key = Osp2Be_MenuKey;
	s_osp2be.menu.fullscreen = qtrue;
	s_osp2be.menu.wrapAround = qtrue;

	s_osp2be.back.generic.type = MTYPE_BITMAP;
	s_osp2be.back.generic.name = ART_BACK0;
	s_osp2be.back.generic.flags = QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_osp2be.back.generic.callback = Osp2Be_MenuEvent;
	s_osp2be.back.generic.id = ID_BACK;
	s_osp2be.back.generic.x = 0;
	s_osp2be.back.generic.y = 480 - 64;
	s_osp2be.back.width = 128;
	s_osp2be.back.height = 64;
	s_osp2be.back.focuspic = ART_BACK1;

	Menu_AddItem( &s_osp2be.menu, &s_osp2be.back );

	UI_PushMenu( &s_osp2be.menu );
	Menu_SetCursorToItem( &s_osp2be.menu, &s_osp2be.back );
}

