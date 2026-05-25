/*
===========================================================================
osp_fonts.c — built-in font name and .cfg path table (cgame + ui)
===========================================================================
*/

#include "q_shared.h"
#include "osp_fonts.h"

const ospBuiltinFont_t ospBuiltinFonts[OSP_BUILTIN_FONT_COUNT] =
{
	{ "id",                    "gfx/2d/bigchars.cfg" },
	{ "idblock",               "gfx/2d/numbers.cfg" },
	{ "sansman",               "gfx/2d/sansman.cfg" },
	{ "cpma",                  "gfx/2d/sansman.cfg" },
	{ "m1rage",                "gfx/2d/m1rage.cfg" },
	{ "elite_emoji",           "gfx/2d/elite_emoji.cfg" },
	{ "diablo",                "gfx/2d/diablo.cfg" },
	{ "eternal",               "gfx/2d/eternal.cfg" },
	{ "qlnumbers",             "gfx/2d/qlnumbers.cfg" },
	{ "elite",                 "gfx/2d/Elite.cfg" },
	{ "elitebigchars",         "gfx/2d/EliteBigchars.cfg" },
	{ "eliteeternal",          "gfx/2d/EliteEternal.cfg" },
	{ "bigcharseliteeternal",  "gfx/2d/BigcharsEliteEternal.cfg" },
	{ "elitestoropia",         "gfx/2d/EliteStoropia.cfg" },
	{ "bigcharselitestoropia", "gfx/2d/BigcharsEliteStoropia.cfg" },
	{ "bigcharseliteorbitron", "gfx/2d/BigcharsEliteOrbitron.cfg" },
	{ "eliteorbitron",         "gfx/2d/EliteOrbitron.cfg" },
	{ "q4snumbers",            "gfx/2d/q4snumbers.cfg" },
	{ "q4mnumbers",            "gfx/2d/q4mnumbers.cfg" },
	{ "conthrax",              "gfx/2d/conthrax.cfg" },
	{ "qlhandelgothic",        "gfx/2d/qlhandelgothic.cfg" },
	{ "quakeworld",            "gfx/2d/quakeworld.cfg" },
	{ "quakeworld_numbers",    "gfx/2d/quakeworld_numbers.cfg" },
	{ "odyssey",               "gfx/2d/odyssey.cfg" },
	{ "verdana",               "gfx/2d/verdana.cfg" },
};
