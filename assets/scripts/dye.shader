powerups/battleSuitDye
{
	deformVertexes wave 100 sin 1 0 0 0
	{
		map textures/effects/envmapmono.tga
		tcGen environment
		tcMod turb 0 0.15 0 0.3
                tcmod rotate 333
                tcmod scroll .3 .3
		blendfunc GL_ONE GL_ONE
        rgbGen entity
	}
    {
		map textures/effects/envmapmono2.tga
		tcGen environment
		tcMod turb 0 0.15 0 0.3
        tcmod rotate 333
        tcmod scroll .3 .3
		blendfunc GL_ONE GL_ONE
	}
}

powerups/battleWeaponDye
{
	deformVertexes wave 100 sin 0.5 0 0 0
	{
		map textures/effects/envmapmono.tga
		tcGen environment
		tcMod turb 0 0.15 0 0.3
        tcmod rotate 333
        tcmod scroll .3 .3
		blendfunc GL_ONE GL_ONE
        rgbGen entity
	}
    {
		map textures/effects/envmapmono2.tga
		tcGen environment
		tcMod turb 0 0.15 0 0.3
        tcmod rotate 333
        tcmod scroll .3 .3
		blendfunc GL_ONE GL_ONE
	}
}

powerups/quadDye
{
	deformVertexes wave 100 sin 3 0 0 0
	{
		map textures/effects/quadmapmono.tga
		blendfunc GL_ONE GL_ONE
		tcGen environment
		tcmod rotate 30
		tcmod scroll 1 .1
		rgbGen entity
	}
	{
		map textures/effects/quadmapmono2.tga
		blendfunc GL_ONE GL_ONE
		tcGen environment
		tcmod rotate 30
		tcmod scroll 1 .1
	}
}

powerups/quadWeaponDye
{
	deformVertexes wave 100 sin 0.5 0 0 0
	{
		map textures/effects/quadmapmono.tga
		blendfunc GL_ONE GL_ONE
		tcGen environment
		tcmod rotate 30
		tcmod scroll 1 .1
		rgbGen entity
	}
	{
		map textures/effects/quadmapmono2.tga
		blendfunc GL_ONE GL_ONE
		tcGen environment
		tcmod rotate 30
		tcmod scroll 1 .1
	}
}

powerups/quadRed
{
	deformVertexes wave 100 sin 3 0 0 0
	{
		map textures/effects/quadmapred.tga
		blendfunc GL_ONE GL_ONE
		tcGen environment
		tcmod rotate 30
		tcmod scroll 1 .1
	}
	// {
	// 	map textures/effects/quadmapmono2.tga
	// 	blendfunc GL_ONE GL_ONE
	// 	tcGen environment
	// 	tcmod rotate 30
	// 	tcmod scroll 1 .1
	// }
}

models/powerups/instant/quadDye
{
	//{
	//	map gfx/colors/darkblue.tga
	//}
	{
		map textures/effects/envmapwhiteb.tga
		blendfunc GL_ONE GL_ZERO
		tcGen environment
        rgbGen entity
	}
	{
		map textures/effects/envmapwhiteb2.tga
		blendfunc GL_ONE GL_ONE
		tcGen environment
	}
}

models/powerups/instant/enviroDye
{
	//{
	//	map models/powerups/instant/enviro.tga
	//}
	{
		map textures/effects/envmapwhiteg.tga
		blendfunc GL_ONE GL_ZERO
		tcGen environment
		rgbGen entity
	}
		{
		map textures/effects/envmapwhiteg2.tga
		blendfunc GL_ONE GL_ONE
		tcGen environment
	}
}

//=========================

//
// holdable item icons
//
icons/dye/teleporter
{
	nopicmip
	{
		map icons/dye/teleporter.tga
		blendFunc blend
		rgbgen vertex
	}
}
icons/dye/medkit
{
	nopicmip
	{
		map icons/dye/medkit.tga
		blendFunc blend
		rgbgen vertex
	}
}

//
// powerup icons
//
icons/dye/envirosuit
{
	nopicmip
	nomipmaps
	{
		map icons/dye/envirosuit.tga
		blendFunc blend
		rgbgen vertex
	}
}
icons/dye/quad
{
	nopicmip
	nomipmaps
	{
		map icons/dye/quad.tga
		blendFunc blend
		rgbgen vertex
	}
}
icons/dye/haste
{
	nopicmip
	{
		map icons/dye/haste.tga
		blendFunc blend
		rgbgen vertex
	}
}
icons/dye/invis
{
	nopicmip
	{
		map icons/dye/invis.tga
		blendFunc blend
		rgbgen vertex
	}
}
icons/dye/regen
{
	nopicmip
	{
		map icons/dye/regen.tga
		blendFunc blend
		rgbgen vertex
	}
}
icons/dye/flight
{
	nopicmip
	{
		map icons/dye/flight.tga
		blendFunc blend
		rgbgen vertex
	}
}


// reward medals used in the game floating over client's heads
medal_impressive
{
	nopicmip
	{
		clampmap menu/medals/medal_impressive.tga
		blendFunc blend
		rgbgen vertex
	}
}

medal_excellent
{
	nopicmip
	{
		clampmap menu/medals/medal_excellent.tga
		blendFunc blend
		rgbgen vertex
	}
}

medal_gauntlet
{
	nopicmip
	{
		clampmap menu/medals/medal_gauntlet.tga
		blendFunc blend
		rgbgen vertex
	}
}


//
// weapon icons
//
icons/dye/iconw_gauntlet
{
	nopicmip
	{
		map icons/dye/iconw_gauntlet.tga
		blendFunc blend
		rgbgen vertex
	}
}
icons/dye/iconw_machinegun
{
	nopicmip
	{
		map icons/dye/iconw_machinegun.tga
		blendFunc blend
		rgbgen vertex
	}
}
icons/dye/iconw_rocket
{
	nopicmip
	{
		map icons/dye/iconw_rocket.tga
		blendFunc blend
		rgbgen vertex
	}
}

icons/dye/iconw_shotgun
{
	nopicmip
	{
		map icons/dye/iconw_shotgun.tga
		blendFunc blend
		rgbgen vertex
	}
}

icons/dye/iconw_grenade
{
	nopicmip
	{
		map icons/dye/iconw_grenade.tga
		blendFunc blend
		rgbgen vertex
	}
}

icons/dye/iconw_lightning
{
	nopicmip
	{
		map icons/dye/iconw_lightning.tga
		blendFunc blend
		rgbgen vertex
	}
}

icons/dye/iconw_plasma
{
	nopicmip
	{
		map icons/dye/iconw_plasma.tga
		blendFunc blend
		rgbgen vertex
	}
}

icons/dye/iconw_railgun
{
	nopicmip
	{
		map icons/dye/iconw_railgun.tga
		blendFunc blend
		rgbgen vertex
	}
}

icons/dye/iconw_bfg
{
	nopicmip
	{
		map icons/dye/iconw_bfg.tga
		blendFunc blend
		rgbgen vertex
	}
}

icons/dye/iconw_grapple
{
	nopicmip
	{
		map icons/dye/iconw_grapple.tga
		blendFunc blend
		rgbgen vertex
	}
}

//
// ammo icons
//
icons/dye/icona_machinegun
{
	nopicmip
	{
		map icons/dye/icona_machinegun.tga
		blendFunc blend
		rgbgen vertex
	}
}
icons/dye/icona_rocket
{
	nopicmip
	{
		map icons/dye/icona_rocket.tga
		blendFunc blend
		rgbgen vertex
	}
}

icons/dye/icona_shotgun
{
	nopicmip
	{
		map icons/dye/icona_shotgun.tga
		blendFunc blend
rgbgen vertex
		rgbgen identitylighting
	}
}

icons/dye/icona_grenade
{
	nopicmip
	{
		map icons/dye/icona_grenade.tga
		blendFunc blend
		rgbgen vertex
	}
}

icons/dye/icona_lightning
{
	nopicmip
	{
		map icons/dye/icona_lightning.tga
		blendFunc blend
		rgbgen vertex
	}
}

icons/dye/icona_plasma
{
	nopicmip
	{
		map icons/dye/icona_plasma.tga
		blendFunc blend
		rgbgen vertex
	}
}

icons/dye/icona_railgun
{
	nopicmip
	{
		map icons/dye/icona_railgun.tga
		blendFunc blend
		rgbgen vertex
	}
}

icons/dye/icona_bfg
{
	nopicmip
	{
		map icons/dye/icona_bfg.tga
		blendFunc blend
		rgbgen vertex
	}
}


//
// armor icons
//
icons/dye/iconr_shard
{
	nopicmip
	{
		map icons/dye/iconr_shard.tga
		blendFunc blend
		rgbgen vertex
	}
}

icons/dye/iconr_yellow
{
	nopicmip
	{
		map icons/dye/iconr_yellow.tga
		blendFunc blend
		rgbgen vertex
	}
}

icons/dye/iconr_red
{
	nopicmip
	{
		map icons/dye/iconr_red.tga
		blendFunc blend
		rgbgen vertex
	}
}

//
// health icons
//
icons/dye/iconh_green
{
	nopicmip
	{
		map icons/dye/iconh_green.tga
		blendFunc blend
		rgbgen vertex
	}
}

icons/dye/iconh_yellow
{
	nopicmip
	{
		map icons/dye/iconh_yellow.tga
		blendFunc blend
		rgbgen vertex
	}
}

icons/dye/iconh_red
{
	nopicmip
	{
		map icons/dye/iconh_red.tga
		blendFunc blend
		rgbgen vertex
	}
        //{
	//	clampmap icons/dye/iconh_red2.tga
	//	blendFunc blend
        //        tcMod stretch sin .04 1 0 1
	//}
}

icons/dye/iconh_mega
{
	nopicmip
	{
		map icons/dye/iconh_mega.tga
		blendFunc blend
		rgbgen vertex
	}
}

//
// CTF
//

models/flags/flag2Dye
{
      
	deformVertexes wave 100 sin 4 3 0 0.3
    deformVertexes wave 30 sin 0 3 0 .8
    deformVertexes normal .3 .2
    cull none
        
    {
        map models/flags/white_fx.tga
        tcGen environment
        blendFunc GL_ONE GL_ONE
        rgbGen entity
	}
    {
		map models/flags/flag2Dye.tga
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen entity
	}
    {
		map textures/sfx/shadow.tga
        tcGen environment 
        blendFunc GL_DST_COLOR GL_ZERO
        rgbGen identity
	}
}

// fullbright shaders
models/flags/flag2Fullbright
{
	cull none
	{
		map $whiteimage
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen entity
		alphaGen entity
	}
}

models/flags/flag2Fullbright2
{
	cull none
    {
        map textures/effects/envmapghost.jpg
        blendfunc GL_SRC_ALPHA GL_ONE
	    tcmod rotate 30
        tcmod scroll 1 .1
        rgbGen entity
	    alphaGen entity
    }
}
