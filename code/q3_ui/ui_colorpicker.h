#ifndef UI_COLORPICKER_H
#define UI_COLORPICKER_H

#include "ui_local.h"

typedef struct
{
	qboolean active;
	vec4_t currentColor;
	char hexBuffer[8];
	void (*callback)(const char* hexColor, void* userData);
	void* userData;
	int x;
	int y;
	int width;
	int height;
	int mode;
	float sliderValues[3];
	char componentText[3][4];
	int focusedField;
	int selectedPaletteIndex;
} uiColorPicker_t;

void UI_ColorPicker_Init(uiColorPicker_t* picker);
void UI_ColorPicker_Open(uiColorPicker_t* picker, const char* initialValue, void (*callback)(const char* hexColor, void* userData), void* userData);
void UI_ColorPicker_Close(uiColorPicker_t* picker);
qboolean UI_ColorPicker_IsActive(uiColorPicker_t* picker);
void UI_ColorPicker_Draw(uiColorPicker_t* picker);
qboolean UI_ColorPicker_HandleKey(uiColorPicker_t* picker, int key);
qboolean UI_ColorPicker_HandleMouse(uiColorPicker_t* picker, int x, int y, qboolean click);

#endif /* UI_COLORPICKER_H */
