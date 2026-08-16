// Ruthless map banners — canonical NEW shaders only.
// Stock map names (textures/ad_content/*, main_q3abanner, …) are NOT redefined here.
// Server remaps those via CS_SHADERSTATE when g_mapBanners 1.
// Layer setup matches Mus1n zz-Banners.shaderx (FX → Banner* → glow).

textures/banners/1x1
{
	nopicmip
	{
		map textures/banners/FX.tga
		tcMod scroll 0 -0.15
	}
	{
		map textures/banners/Banner1x1.tga
		rgbGen const ( 0.5 0.5 0.5 )
		blendFunc blend
	}
	{
		map textures/banners/glow2.tga
		blendfunc add
		rgbGen Wave sin .6 0 0 0
		tcmod scroll 0 -0.75
	}
}

textures/banners/1x1_flipped
{
	nopicmip
	{
		map textures/banners/FX.tga
		tcMod scroll 0 -0.15
		tcMod transform -1 0 0 1 1 0
	}
	{
		map textures/banners/Banner1x1.tga
		rgbGen const ( 0.5 0.5 0.5 )
		blendFunc blend
		tcMod transform -1 0 0 1 1 0
	}
	{
		map textures/banners/SkullEyes.tga
		blendFunc blend
		rgbgen wave sin 1 0.7 0.3 0.7
		tcMod transform -1 0 0 1 1 0
	}
	{
		map textures/banners/glow2.tga
		blendfunc add
		rgbGen Wave sin .6 0 0 0
		tcmod scroll 0 -0.75
		tcMod transform -1 0 0 1 1 0
	}
}

textures/banners/2x1
{
	nopicmip
	{
		map textures/banners/FX2.tga
		tcMod rotate 10
	}
	{
		map textures/banners/BladeLightningFX.tga
		blendFunc GL_ONE GL_ONE
		tcMod scroll 1 0.5
	}
	{
		map textures/banners/FXglass.tga
		tcGen environment
		tcMod stretch sin 1 .1 .5 0.25
		blendfunc GL_ONE GL_ONE
		rgbGen lightingDiffuse
	}
	{
		map textures/banners/Banner2x1.tga
		rgbGen const ( 0.8 0.8 0.8 )
		blendFunc blend
	}
	{
		map textures/banners/glow2.tga
		blendfunc add
		rgbGen Wave sin .6 0 0 0
		tcmod scroll 0 -0.5
	}
}

textures/banners/2x1_reverse
{
	nopicmip
	{
		map textures/banners/FX2.tga
		tcMod rotate 10
		tcMod transform -1 0 0 1 1 0
	}
	{
		map textures/banners/BladeLightningFX.tga
		blendFunc GL_ONE GL_ONE
		tcMod scroll 1 0.5
		tcMod transform -1 0 0 1 1 0
	}
	{
		map textures/banners/FXglass.tga
		tcGen environment
		tcMod stretch sin 1 .1 .5 0.25
		blendfunc GL_ONE GL_ONE
		rgbGen lightingDiffuse
		tcMod transform -1 0 0 1 1 0
	}
	{
		map textures/banners/Banner2x1.tga
		rgbGen const ( 0.8 0.8 0.8 )
		blendFunc blend
		tcMod transform -1 0 0 1 1 0
	}
	{
		map textures/banners/glow2.tga
		blendfunc add
		rgbGen Wave sin .6 0 0 0
		tcmod scroll 0 -0.5
		tcMod transform -1 0 0 1 1 0
	}
}

textures/banners/2x1_trans
{
	nopicmip
	{
		map textures/banners/FX2.tga
		tcMod rotate 10
		blendfunc blend
	}
	{
		map textures/banners/BladeLightningFX.tga
		blendFunc GL_ONE GL_ONE
		tcMod scroll 1 0.5
	}
	{
		map textures/banners/Banner2x1.tga
		rgbGen const ( 0.8 0.8 0.8 )
		blendFunc blend
	}
	{
		map textures/banners/glow2.tga
		blendfunc add
		rgbGen Wave sin .6 0 0 0
		tcmod scroll 0 -0.5
	}
	{
		map textures/banners/adbrightoverlay.tga
		rgbGen Wave sin .17 0.05 0 1.7
		blendfunc add
	}
}

textures/banners/4x1
{
	nopicmip
	{
		map textures/banners/Background.png
		rgbGen const ( 0.5 0.5 0.5 )
	}
	{
		map textures/banners/Banner4x1.tga
		rgbGen const ( 0.3 0.3 0.3 )
		blendFunc blend
	}
	{
		map textures/banners/glow2.tga
		blendfunc add
		rgbGen Wave sin .6 0 0 0
		tcmod scroll 0 -0.5
	}
}

textures/banners/main_q3abanner
{
	nopicmip
	{
		map textures/banners/FX2.tga
		tcMod rotate 10
	}
	{
		map textures/banners/BladeLightningFX.tga
		blendFunc GL_ONE GL_ONE
		tcMod scroll 1 0.5
	}
	{
		map textures/banners/FXglass.tga
		tcGen environment
		tcMod stretch sin 1 .1 .5 0.25
		blendfunc GL_ONE GL_ONE
		rgbGen lightingDiffuse
	}
	{
		map textures/banners/OldQ3Banner.tga
		rgbGen const ( 0.8 0.8 0.8 )
		blendFunc blend
	}
	{
		map textures/banners/glow2.tga
		blendfunc add
		rgbGen Wave sin .6 0 0 0
		tcmod scroll 0 -0.5
	}
}

textures/banners/main_q3abanner_x
{
	nopicmip
	{
		map textures/banners/FX2.tga
		tcMod rotate 10
	}
	{
		map textures/banners/BladeLightningFX.tga
		blendFunc GL_ONE GL_ONE
		tcMod scroll 1 0.5
	}
	{
		map textures/banners/FXglass.tga
		tcGen environment
		tcMod stretch sin 1 .1 .5 0.25
		blendfunc GL_ONE GL_ONE
		rgbGen lightingDiffuse
	}
	{
		map textures/banners/OldQ3BannerX.tga
		rgbGen const ( 0.8 0.8 0.8 )
		blendFunc blend
	}
	{
		map textures/banners/glow2.tga
		blendfunc add
		rgbGen Wave sin .6 0 0 0
		tcmod scroll 0 -0.5
	}
}
