#include "cg_local.h"
#include "cg_superhud_private.h"
#include "../qcommon/qcommon.h"

typedef enum
{
	SHUD_ELEMENT_OBSERVER_TOOLS_TEXT,
	SHUD_ELEMENT_OBSERVER_TOOLS_ICON,
} shudElementDataType_t;

typedef enum
{
	SHUD_ELEMENT_OBSERVER_TOOLS_FIRING
} shudElementObserverToolsType_t;

typedef struct
{
	superhudConfig_t config;
	superhudTextContext_t textContext;
	superhudDrawContext_t drawContext;
	shudElementObserverToolsType_t type;
	shudElementDataType_t dataType;
} shudElementObserverTools_t;

void* CG_SHUDElementObserverToolsCreate(const superhudConfig_t* config, int type)
{
	shudElementObserverTools_t* element;

	SHUD_ELEMENT_INIT(element, config);
	
	element->type = type;


	if (type == SHUD_ELEMENT_OBSERVER_TOOLS_TEXT)
	{
		CG_SHUDTextMakeContext(&element->config, &element->textContext);
		CG_SHUDFillAndFrameForText(&element->config, &element->textContext);
	}
	else
	{
		CG_SHUDDrawMakeContext(&element->config, &element->drawContext);
	}

	return element;
}

void* CG_SHUDElementOTRealFireTime(const superhudConfig_t* config)
{
	return CG_SHUDElementObserverToolsCreate(config, SHUD_ELEMENT_OBSERVER_TOOLS_TEXT);
}

#define MAX_ATTACK_RECORDS 5

// Массивы для хранения времени и количества кадров
static float attackDurations[MAX_ATTACK_RECORDS];
static int attackFrames[MAX_ATTACK_RECORDS];
static int attackIndex = 0;
static int isCurrentlyFiring = 0;
static int fireStartTime = 0;

void CG_SHUDElementObserverToolsRoutine(void* context) {
    shudElementObserverTools_t* element = (shudElementObserverTools_t*)context;
    superhudTextContext_t* ctx = &element->textContext;

    static int snapCount = 0;  // Счётчик кадров для отслеживания времени активности флага
    int now = cg.time, i;  // Текущее время в миллисекундах
    qboolean isRealFiring = (cg.snap->ps.eFlags & EF_FIRING) ? qtrue : qfalse;

    // Проверка на начало удержания флага
    if (isRealFiring && !isCurrentlyFiring) {
        // Начало удержания флага
        fireStartTime = now;
        snapCount = 0;  // Сбросить счётчик кадров
        isCurrentlyFiring = 1;
    } 
    else if (!isRealFiring && isCurrentlyFiring) {
        // Конец удержания флага
        float fireDuration = (now - fireStartTime) / 1000.0f;  // Продолжительность в секундах

        // Сдвигаем массивы, если нужно
        if (attackIndex < MAX_ATTACK_RECORDS) {
            attackDurations[attackIndex] = fireDuration;
            attackFrames[attackIndex] = snapCount;
            attackIndex++;
        } else {
            // Сдвиг данных, если мы достигли максимума записей
            memmove(&attackDurations[0], &attackDurations[1], sizeof(float) * (MAX_ATTACK_RECORDS - 1));
            memmove(&attackFrames[0], &attackFrames[1], sizeof(int) * (MAX_ATTACK_RECORDS - 1));
            attackDurations[MAX_ATTACK_RECORDS - 1] = fireDuration;
            attackFrames[MAX_ATTACK_RECORDS - 1] = snapCount;
        }
        isCurrentlyFiring = 0;
    }

    // Если флаг активен, увеличиваем счётчик кадров
    if (isRealFiring) {
        snapCount++;  // Увеличиваем счётчик кадров, пока флаг активен
    }

    // Отображение текста +attack и времени удержания
    if (element->type == SHUD_ELEMENT_OBSERVER_TOOLS_FIRING &&
        element->dataType == SHUD_ELEMENT_OBSERVER_TOOLS_TEXT) {

        // Основной текст +attack
		superhudTextContext_t tempCtx;
		superhudTextContext_t tempCtx2;
		float attackWidth = 0.0f;
        char tempText[128];
        Com_sprintf(tempText, sizeof(tempText), "+attack");

        // Создадим временный контекст для вывода

        memcpy(&tempCtx, ctx, sizeof(superhudTextContext_t)); // Копируем существующий контекст
		memcpy(&tempCtx2, ctx, sizeof(superhudTextContext_t)); // Копируем существующий контекст

        // Присваиваем текст и выводим
        tempCtx2.text = tempText;
		if (isRealFiring)
		Vector4Copy(element->config.color2.value.rgba, tempCtx2.color);
		else
		Vector4Copy(element->config.color.value.rgba, tempCtx2.color);

        CG_SHUDTextPrintNew(&element->config, &tempCtx2, qfalse);

        // Сдвигаем координаты для следующей строки
		tempCtx.coord.named.y += tempCtx2.coord.named.h + 2.0f;

		CG_FontSelect(tempCtx2.fontIndex);

		attackWidth = CG_OSPDrawStringLenPix(tempCtx2.text, tempCtx2.coord.named.w, tempCtx2.flags, SCREEN_WIDTH);

        tempCtx2.coord.named.x += attackWidth + 2.0f;

        // Добавляем текущее время удержания флага
        if (isRealFiring) {
            float realTime = (now - fireStartTime) / 1000.0f;  // Текущее время, сколько флаг активен
            Com_sprintf(tempText, sizeof(tempText), "(%.3fs)", realTime);
            // Присваиваем текст и выводим
            tempCtx2.text = tempText;
            CG_SHUDTextPrintNew(&element->config, &tempCtx2, qfalse);

        }

        // Добавляем историю для 5 последних записей
        for (i = 0; i < attackIndex; i++) {
            Com_sprintf(tempText, sizeof(tempText), " %d frames (%.3fs)", 
                        attackFrames[i], attackDurations[i]);

            // Присваиваем текст и выводим
            tempCtx.text = tempText;
            CG_SHUDTextPrintNew(&element->config, &tempCtx, qfalse);

            // Сдвигаем координаты для следующей строки
            tempCtx.coord.named.y += tempCtx.coord.named.h + 2.0f;
        }
    }
}



void CG_SHUDElementObserverToolsCDestroy(void* context)
{
	if (context)
	{
		Z_Free(context);
	}
}
