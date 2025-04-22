gfx/misc/hbox
{
	nopicmip

	{
		map gfx/misc/hbox.tga
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}

gfx/misc/hbox_nocull
{
	nopicmip
	cull none

	{
		map gfx/misc/hbox.tga
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}

sprites/frozenFoeTag
{
  nomipmaps
  nopicmip
  {
    map sprites/frozenFoeTag.tga
    blendFunc blend
    rgbgen vertex
  }
}

sprites/foe
{
	nomipmaps
	nopicmip
	{
		map sprites/foe2.tga
    blendFunc GL_ONE GL_ONE
    rgbgen entity
	}
}

textures/effects/frozen
{
  deformvertexes wave 100 sin 3 0 0 0
  {
    map textures/effects/frozen.tga
    blendFunc GL_ONE GL_ONE
		tcGen environment
    tcmod rotate 30
    tcmod scroll 1 .1
    rgbGen entity
  }
}

lightningBoltNew2
{
	cull none
	{
		map gfx/misc/shaft2.jpg
		blendFunc GL_ONE GL_ONE
		rgbgen wave sin 1 0.5 0 7.1
		tcmod scale  0.2 1
		tcMod scroll -5 0
	}
	{
		map gfx/misc/shaft2.jpg
		blendFunc GL_ONE GL_ONE
		rgbgen wave sin 1 0.8 0 8.1
		tcmod scale  -0.3 -1
		tcMod scroll -7.2 0
	}
}

lightningBoltNew3
{
	cull none
	nopicmip
	nomipmaps
	{
 		map gfx/misc/shaftBE.tga
    		blendFunc GL_ONE GL_ONE
		rgbgen wave sin 1 0.5 0 7.1
		tcmod scale  2 1
		tcMod scroll -5 0
	}
	{
		map gfx/misc/shaftBE.tga
		blendFunc GL_ONE GL_ONE
		rgbgen wave sin 1 0.8 0 8.1
		tcmod scale  -1.3 -1
		tcMod scroll -7.2 0
	}
}

// Enemy lightning bolt



enemyLightningBolt
{
	nopicmip
  	nomipmaps
	cull none
	{
		map gfx/misc/lightning3_alpha.tga
		blendFunc blend
                rgbgen vertex
                tcmod scale  2 1
		tcMod scroll -5 0
	}
    {
		map gfx/misc/lightning3_alpha.tga
		blendFunc blend
                rgbgen vertex
                tcmod scale  -1.3 -1
		tcMod scroll -7.2 0
	}
}

enemyLightningBoltNew
{
	nopicmip
  	nomipmaps
	cull none
	{
		map gfx/misc/lightning3new_alpha.tga
		blendFunc blend
		rgbgen vertex
		tcmod scale  2 1
		tcMod scroll -5 0
	}

	{
		map gfx/misc/lightning3new_alpha.tga
		blendFunc blend
		rgbgen vertex
		tcmod scale  -1.3 -1
		tcMod scroll -7.2 0
	}
}

enemyLightningBoltNew2
{
	nopicmip
  	nomipmaps
	cull none
	{
		map gfx/misc/shaft2_alpha.jpg
		blendFunc blend
		rgbgen vertex
		tcmod scale  0.2 1
		tcMod scroll -5 0
	}
	{
		map gfx/misc/shaft2_alpha.jpg
		blendFunc blend
		rgbgen vertex
		tcmod scale  -0.3 -1
		tcMod scroll -7.2 0
	}
}

enemyLightningBoltNew3
{
  nopicmip
  nomipmaps
  cull none
  {
    map gfx/misc/shaftBE_alpha.tga
    		blendFunc blend
		rgbgen vertex
		tcmod scale  2 1
		tcMod scroll -5 0
  }
    {
    map gfx/misc/shaftBE_alpha.tga
		blendFunc blend
		rgbgen wave sin 1 0.8 0 8.1
		tcmod scale  -1.3 -1
		tcMod scroll -7.2 0
  }
}

damageIndicator2
{
	sort nearest
	nopicmip
	{
		clampmap gfx/2d/damage_indicator2.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
	}
}

//first person  Gun (cg_drawgun 3 shader)

firstPersonGun {
    {
        map gfx/2d/white64.tga
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen entity
	alphaGen entity
    }
}




// obituaries
ObituariesSkull {
    {
        map icons/obituaries/ObituariesSkull.tga
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen identity
    }
}

ObituariesFallenCrashed {
    {
        map icons/obituaries/fallencrashed.tga
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen identity
    }
}

ObituariesFalling {
    {
        map icons/obituaries/falling.tga
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen identity
    }
}

ObituariesDrowned {
    {
        map icons/obituaries/drowned.tga
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen identity
    }
}

ObituariesLava {
    {
        map icons/obituaries/lava.tga
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen identity
    }
}

ObituariesTelefrag {
    {
        map icons/obituaries/telefrag.tga
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen identity
    }
}

ObituariesMelted {
    {
        map icons/obituaries/melted.tga
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen identity
    }
}

ObituariesRocketDirect {
    {
        map icons/iconw_rocket.tga
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen identity
    }
    {
        map icons/obituaries/targetHit.tga
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen identity
    }
}

ObituariesGrenadeDirect {
    {
        map icons/iconw_grenade.tga
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen identity
    }
    {
        map icons/obituaries/targetHit.tga
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen identity
    }
}

ObituariesPlasmaDirect {
    {
        map icons/iconw_plasma.tga
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen identity
    }
    {
        map icons/obituaries/targetHit.tga
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen identity
    }
}

ObituariesBFGDirect {
    {
        map icons/iconw_BFG.tga
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen identity
    }
    {
        map icons/obituaries/targetHit.tga
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
        rgbGen identity
    }
}


whiteAlpha
{
	nopicmip
	{
		map *white
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}

whiteAlpha_nocull
{
	nopicmip
	cull none
	{
		map *white
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}

whiteAlpha_cullback
{
	polygonOffset
	nopicmip
	cull back
	{
		map *white
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}

outlineWide
{
	nopicmip
    deformVertexes wave 1 sin 1.8 0 0 0
	cull back
	{
		map *white
		blendFunc GL_ONE GL_ONE
		rgbGen entity	
	}
}

outlineMedium
{
	nopicmip
    deformVertexes wave 1 sin 1.1 0 0 0
	cull back
	{
		map *white
		blendFunc GL_ONE GL_ONE
		rgbGen entity	
	}
}   

outlineThin
{
	nopicmip
    deformVertexes wave 1000 sin 0.45 0 1000 1
	cull back
	{
		map *white
		blendFunc GL_ONE GL_ONE
		rgbGen entity	
	}
}

plasmaExplosion_old
{
	cull disable
	{
		clampmap models/weaphits/plasmaboom_old.tga
		blendfunc add
                tcMod stretch triangle .6 0.1 0 8
                tcmod rotate 999
                rgbGen wave inversesawtooth 0 1 0 1.5
	}
}

sprites/plasma_old
{
	cull disable
	{
		clampmap sprites/plasma_old.tga
		blendfunc GL_ONE GL_ONE
                tcMod rotate 931
	}
}

gfx/damage/plasma_mrk_be
{
	polygonOffset
	{
		map gfx/damage/plasma_mrk_be.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
	}
}

gfx/misc/spark
{
	nopicmip
	{
		map gfx/misc/spark.jpg
		blendfunc add
		rgbGen entity
	}
}

gfx/2d/arrow_down
{
    nopicmip
    {
       clampmap gfx/2d/arrow.tga
       blendFunc blend
       rgbGen vertex
    }
}
gfx/2d/arrow_up
{
    nopicmip
    {
       clampmap gfx/2d/arrow.tga
       blendFunc blend
	   tcMod transform -1 0 0 -1 1 1
       rgbGen vertex
    }
}
markShadowNew
{
	polygonOffset
	{
		map gfx/misc/whiteCircle.tga
		blendfunc add
		rgbGen vertex
	}	
}
