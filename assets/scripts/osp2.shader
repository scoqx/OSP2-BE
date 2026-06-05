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
    map sprites/frozenFoeTag2.tga
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

textures/effects/frozenNew
{
  deformvertexes wave 100 sin 1.15 0 0 0
  {
    map textures/effects/frozen2.tga
    blendFunc GL_SRC_ALPHA GL_ONE
	tcGen environment
    tcmod rotate 30
    tcmod scroll 0.4 .1
    rgbGen entity
  }
}

// Honey Bunny QT's wet melting ice 
textures/effects/frozenNew2
{
    // inflate the model by 1.15 units
    deformvertexes wave 100 sin 1.15 0 0 0
    
    // physics and rendering rules
    surfaceparm trans
    surfaceparm nolightmap
    surfaceparm slick      

    // the milky inner frost
    {
        map textures/effects/milky-ice.tga
        blendFunc blend
        tcMod turb 0 0.05 0 0.1
        rgbGen entity
    }

    // the ice scratches
    {
        map textures/effects/scratches.tga
        tcMod scroll 0.001  0.003
        blendFunc add 
    }

    // the hard glass reflection (ambient)
    {
        map textures/effects/tinfx2c.tga
        tcGen environment 
        blendFunc add
        rgbGen wave sin 0.5 0 0 0
    }

    // the "wet jesus" hack (dynamic specular if this even works)
    {
        map textures/effects/drops.tga
        blendFunc GL_SRC_ALPHA GL_ONE
        alphaGen lightingSpecular
        tcMod scroll 0 -0.05
        rgbGen entity
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

firstPersonGun 
{
	sort nearest
    {
        map gfx/2d/white64.tga
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen entity
		alphaGen entity
    }
}

// fullbright shaders
// for player model
fBright
{
	{
		map $whiteimage
		blendFunc GL_ONE GL_ZERO
		rgbGen entity
	}
	{
		map $whiteimage
		blendFunc GL_DST_COLOR GL_ZERO
		rgbGen identityLighting
	}
}

fBrightVertexLight
{
	{
		map $whiteimage
		blendFunc GL_ONE GL_ZERO
		rgbGen entity
	}
}

// for weapons and items
fBrightItems
{
	{
		map $whiteimage
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen entity
		alphaGen entity
	}
}

// obituaries
ObituariesSkull {
    {
        map icons/obituaries/ObituariesSkull.tga
        blendfunc blend
        rgbgen vertex
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


hboxNew
{
	nopicmip
	{
		map *white
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}

hboxNew_nocull
{
	nopicmip
	cull none
	{
		map *white
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}

hboxNew_cullback
{
	nopicmip
	cull back
	{
		map *white
		blendFunc GL_ONE GL_ONE
		rgbGen vertex
	}
}

outlineMedium
{
	nopicmip
  	nomipmaps
    deformVertexes wave 1 sin 1.1 0 0 0
	cull back
	{
		map *white
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen entity	
	}
}   

outlineThin
{
	nopicmip
  	nomipmaps
    deformVertexes wave 1000 sin 0.45 0 1000 1
	cull back
	{
		map *white
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen entity	
	}
}

// cg_altPlasma
// 2 (BE)
plasmaExplosion_old
{
	nopicmip
  	nomipmaps
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
	nopicmip
  	nomipmaps
	cull disable
	{
		clampmap sprites/plasma_old.tga
		blendfunc GL_ONE GL_ONE
                tcMod rotate 931
	}
}

gfx/damage/plasma_mrk_be
{
	nopicmip
  	nomipmaps
	polygonOffset
	{
		map gfx/damage/plasma_mrk_be.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen vertex
		alphaGen vertex
	}
}

// 3 (QL-like)
sprites/plasmaQL
{
	nopicmip
  	nomipmaps
	cull disable
	{
		clampmap sprites/plasmaQL.jpg
		blendfunc add
                tcMod rotate 931
	}
}

plasmaExplosionQL
{
	nopicmip
	cull disable
	{
		clampmap models/weaphits/plasmaboom_ql.jpg
		blendfunc add
                tcMod stretch triangle .6 0.1 0 8
                tcmod rotate 999
                rgbGen wave inversesawtooth 0 1 0 1.5
	}
}

// cg_altBlood
bloodExplosionNew
{
	nopicmip
  	nomipmaps
	{
		map gfx/damage/altBlood.jpg
		blendfunc add
		rgbGen entity
	}
}

gfx/2d/arrow_down
{
    {
       map gfx/2d/arrow.tga
       blendFunc blend
       rgbGen vertex
    }
}

gfx/2d/arrow_up
{
    {
       map gfx/2d/arrow.tga
       blendFunc blend
	   tcMod transform -1 0 0 -1 1 1
       rgbGen vertex
    }
}

markShadowNew
{
	nopicmip
  	nomipmaps
	polygonOffset
	{
		map gfx/damage/altShadow.tga
		blendfunc add
		rgbGen vertex
	}	
}

markShadowNew2
{
	nopicmip
  	nomipmaps
	polygonOffset
	{
		clampmap gfx/damage/altShadow2.tga
		blendfunc add
		rgbGen vertex
		tcmod rotate 50
	}	
}

markShadowNew3
{
	nopicmip
  	nomipmaps
	polygonOffset
	{
		map gfx/damage/shadow_fx02.tga
		blendfunc add
		rgbGen vertex
	}	
}

// temporal accuracy
tempAcc_LG_Icon 
{
    {
        map icons/interim.tga
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen identity
    }
    {
        map icons/iconw_lightning.tga
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        rgbGen identity
    }
}

disconnectedNew
{
	nomipmaps
	nopicmip
	{
		map gfx/2d/net.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
        depthWrite
		rgbGen vertex
	}
}

grenadeCPMANew
{
	nopicmip
    cull none
    {
        map models/ammo/grenadecpmanew_inv.tga
        rgbGen entity
        alphaGen entity
        blendFunc GL_ONE GL_ZERO
    }

    {
        map models/ammo/grenadecpmanew.tga
        rgbGen identity
        alphaGen vertex
        blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
    }
}

snowflake
{
	sort	underwater
	cull none
	entityMergable
	{
		map sprites/frozenFoeTag2.tga
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		rgbGen		vertex
		alphaGen	vertex
	}
}

altFullBrightShader
{
	sort nearest
    {
        map textures/effects/envmapghost.jpg
        blendfunc GL_SRC_ALPHA GL_ONE
	    tcmod rotate 30
        tcmod scroll 1 .1
        rgbGen entity
	    alphaGen entity
    }
}

models/weapons2/grapple2/ball
{
	//novlcollapse
	nopicmip
	{
		map models/weapons2/grapple2/ball.jpg
		blendFunc GL_ONE GL_ZERO
		tcGen environment
		rgbGen lightingDiffuse
	}
}

models/weapons2/grapple2/spikes
{
	//novlcollapse
	nopicmip
	{
		map models/weapons2/grapple2/spikes.jpg
		blendFunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
		tcGen environment
		rgbGen lightingDiffuse
	}
}

grappleRope
{
	//novlcollapse
	nopicmip
	cull none
	{
		map models/weapons2/grapple2/grapplerope.tga
		blendfunc GL_SRC_ALPHA GL_ONE_MINUS_SRC_ALPHA
	}
}

weapons/fragEffect
{
  nopicmip  
    {
    map models/weapons2/fragEffect.tga
    blendFunc GL_ONE GL_ONE
    tcmod scroll -1.5 0
    rgbGen entity
  }
}


// From nemesis for cg_altPlasma 0/1/3
sprites/plasma_color
{
	cull disable
	{
		clampmap sprites/plasma_color.tga
		blendfunc GL_ONE GL_ONE
                tcMod rotate 931
			rgbgen entity
	}
    	{
		clampmap sprites/plasma_color2.tga
		blendfunc GL_ONE GL_ONE
                tcMod rotate 931
	}
}

plasmaExplosion2
{
	cull disable
	{
		clampmap models/weaphits/plasmaboom_color.tga
		blendfunc add
                tcMod stretch triangle .6 0.1 0 8
                tcmod rotate 999
                rgbGen wave inversesawtooth 0 1 0 1.5
                rgbGen entity
	}
}

// For cg_altPlasma 2
sprites/plasma_color_old
{
	cull disable
	{
		clampmap sprites/plasma_old_color.tga
		blendfunc GL_ONE GL_ONE
                tcMod rotate 931
			rgbgen entity
	}
    {
		clampmap sprites/plasma_old_color2.tga
		blendfunc GL_ONE GL_ONE
                tcMod rotate 931
	}
}

plasmaExplosion2_old
{
	cull disable
	{
		clampmap models/weaphits/plasmaboom_old_color.tga
		blendfunc add
                tcMod stretch triangle .6 0.1 0 8
                tcmod rotate 999
                rgbGen wave inversesawtooth 0 1 0 1.5
                rgbGen entity
	}
}
