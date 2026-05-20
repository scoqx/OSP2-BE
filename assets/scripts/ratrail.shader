// -================== ratrail.shader

ratRailParticle
{
	nopicmip
	cull disable
	{
		map gfx/misc/ratrail/particle.tga
		blendfunc add
		rgbGen entity
	}
}
ratRailSpiral1
{
	nopicmip
	sort nearest
	cull disable
	{
		map gfx/misc/ratrail/railspiral1.tga
		blendfunc add
		rgbGen entity
	}
}
ratRailSpiral
{
	nopicmip
	sort nearest
	cull disable
	{
		clampmap gfx/misc/ratrail/railspiral.tga
		tcMod transform 10 0 0 1 0 0
		blendfunc add
		rgbGen entity
	}
}
//ratRailCoreBeefy
ratRailCore
{
	nopicmip
	sort nearest
	cull disable
	{
		animmap 64 gfx/misc/ratrail/beefyrail/rail1.tga  gfx/misc/ratrail/beefyrail/rail2.tga  gfx/misc/ratrail/beefyrail/rail3.tga  gfx/misc/ratrail/beefyrail/rail4.tga  gfx/misc/ratrail/beefyrail/rail5.tga  gfx/misc/ratrail/beefyrail/rail6.tga  gfx/misc/ratrail/beefyrail/rail7.tga  
		tcMod transform 2 0 0 0.75 0 0.125

		blendfunc add
		rgbGen entity
	}
}
//ratRailCoreBeefyOverlay
ratRailCoreOverlay
{
	nopicmip
	sort nearest
	cull disable
	{
		animmap 40 gfx/misc/ratrail/beefyrail/railoverlay1.tga  gfx/misc/ratrail/beefyrail/railoverlay2.tga  gfx/misc/ratrail/beefyrail/railoverlay3.tga  gfx/misc/ratrail/beefyrail/railoverlay4.tga  gfx/misc/ratrail/beefyrail/railoverlay5.tga  gfx/misc/ratrail/beefyrail/railoverlay6.tga  gfx/misc/ratrail/beefyrail/railoverlay7.tga  
		tcMod transform 2 0 0 0.75 0 0.125
		blendfunc add
		rgbGen entity
	}
}

ratRailCoreFat
{
	nopicmip
	sort nearest
	cull disable
	{
		animmap 40 gfx/misc/ratrailfat/rail1.tga  gfx/misc/ratrailfat/rail2.tga  gfx/misc/ratrailfat/rail3.tga  gfx/misc/ratrailfat/rail4.tga  gfx/misc/ratrailfat/rail5.tga  gfx/misc/ratrailfat/rail6.tga  gfx/misc/ratrailfat/rail7.tga  
		blendfunc add
		rgbGen entity
	}
}

ratRailCoreFat
{
	nopicmip
	sort nearest
	cull disable
	{
		animmap 40 gfx/misc/ratrailfat/rail1.tga  gfx/misc/ratrailfat/rail2.tga  gfx/misc/ratrailfat/rail3.tga  gfx/misc/ratrailfat/rail4.tga  gfx/misc/ratrailfat/rail5.tga  gfx/misc/ratrailfat/rail6.tga  gfx/misc/ratrailfat/rail7.tga  
		blendfunc add
		rgbGen entity
	}
}
ratRailCoreOverlayFat
{
	nopicmip
	sort nearest
	cull disable
	{
		animmap 24 gfx/misc/ratrailfat/railoverlay1.tga  gfx/misc/ratrailfat/railoverlay2.tga  gfx/misc/ratrailfat/railoverlay3.tga  gfx/misc/ratrailfat/railoverlay4.tga  gfx/misc/ratrailfat/railoverlay5.tga  gfx/misc/ratrailfat/railoverlay6.tga  gfx/misc/ratrailfat/railoverlay7.tga  
		blendfunc add
		rgbGen entity
	}
}

// -================== ratrail_spiral.shader

ratRailSpiralLightning0
 {
 	nopicmip
 	nomipmaps
 	sort nearest
 	cull disable
 	{
 		map gfx/misc/ratrail/lspiral0.tga
 		tcMod transform 25 0 0 1 0 0
 		blendFunc add
 		rgbGen entity
 	}
 	{
 		map gfx/misc/ratrail/lspiral0.tga
 		tcMod transform 25 0 0 1 0 0
 		blendFunc add
 		rgbGen wave sin 0 0.25 0.25 0.7
 	}
 }

ratRailSpiralLightning1
 {
 	nopicmip
 	nomipmaps
 	sort nearest
 	cull disable
 	{
 		map gfx/misc/ratrail/lspiral1.tga
 		tcMod transform 25 0 0 1 0 0
 		blendFunc add
 		rgbGen entity
 	}
 	{
 		map gfx/misc/ratrail/lspiral1.tga
 		tcMod transform 25 0 0 1 0 0
 		blendFunc add
 		rgbGen wave sin 0 0.25 0.25 0.7
 	}
 }

ratRailSpiralLightning2
 {
 	nopicmip
 	nomipmaps
 	sort nearest
 	cull disable
 	{
 		map gfx/misc/ratrail/lspiral2.tga
 		tcMod transform 25 0 0 1 0 0
 		blendFunc add
 		rgbGen entity
 	}
 	{
 		map gfx/misc/ratrail/lspiral2.tga
 		tcMod transform 25 0 0 1 0 0
 		blendFunc add
 		rgbGen wave sin 0 0.25 0.25 0.7
 	}
 }

ratRailSpiralLightning3
 {
 	nopicmip
 	nomipmaps
 	sort nearest
 	cull disable
 	{
 		map gfx/misc/ratrail/lspiral3.tga
 		tcMod transform 25 0 0 1 0 0
 		blendFunc add
 		rgbGen entity
 	}
 	{
 		map gfx/misc/ratrail/lspiral3.tga
 		tcMod transform 25 0 0 1 0 0
 		blendFunc add
 		rgbGen wave sin 0 0.25 0.25 0.7
 	}
 }

ratRailSpiralLightning4
 {
 	nopicmip
 	nomipmaps
 	sort nearest
 	cull disable
 	{
 		map gfx/misc/ratrail/lspiral4.tga
 		tcMod transform 25 0 0 1 0 0
 		blendFunc add
 		rgbGen entity
 	}
 	{
 		map gfx/misc/ratrail/lspiral4.tga
 		tcMod transform 25 0 0 1 0 0
 		blendFunc add
 		rgbGen wave sin 0 0.25 0.25 0.7
 	}
 }

ratRailSpiralLightning5
 {
 	nopicmip
 	nomipmaps
 	sort nearest
 	cull disable
 	{
 		map gfx/misc/ratrail/lspiral5.tga
 		tcMod transform 25 0 0 1 0 0
 		blendFunc add
 		rgbGen entity
 	}
 	{
 		map gfx/misc/ratrail/lspiral5.tga
 		tcMod transform 25 0 0 1 0 0
 		blendFunc add
 		rgbGen wave sin 0 0.25 0.25 0.7
 	}
 }

ratRailSpiralLightning6
 {
 	nopicmip
 	nomipmaps
 	sort nearest
 	cull disable
 	{
 		map gfx/misc/ratrail/lspiral6.tga
 		tcMod transform 25 0 0 1 0 0
 		blendFunc add
 		rgbGen entity
 	}
 	{
 		map gfx/misc/ratrail/lspiral6.tga
 		tcMod transform 25 0 0 1 0 0
 		blendFunc add
 		rgbGen wave sin 0 0.25 0.25 0.7
 	}
 }

ratRailSpiralLightning7
 {
 	nopicmip
 	nomipmaps
 	sort nearest
 	cull disable
 	{
 		map gfx/misc/ratrail/lspiral7.tga
 		tcMod transform 25 0 0 1 0 0
 		blendFunc add
 		rgbGen entity
 	}
 	{
 		map gfx/misc/ratrail/lspiral7.tga
 		tcMod transform 25 0 0 1 0 0
 		blendFunc add
 		rgbGen wave sin 0 0.25 0.25 0.7
 	}
 }

ratRailSpiralLightning8
 {
 	nopicmip
 	nomipmaps
 	sort nearest
 	cull disable
 	{
 		map gfx/misc/ratrail/lspiral8.tga
 		tcMod transform 25 0 0 1 0 0
 		blendFunc add
 		rgbGen entity
 	}
 	{
 		map gfx/misc/ratrail/lspiral8.tga
 		tcMod transform 25 0 0 1 0 0
 		blendFunc add
 		rgbGen wave sin 0 0.25 0.25 0.7
 	}
 }

ratRailSpiralLightning9
 {
 	nopicmip
 	nomipmaps
 	sort nearest
 	cull disable
 	{
 		map gfx/misc/ratrail/lspiral9.tga
 		tcMod transform 25 0 0 1 0 0
 		blendFunc add
 		rgbGen entity
 	}
 	{
 		map gfx/misc/ratrail/lspiral9.tga
 		tcMod transform 25 0 0 1 0 0
 		blendFunc add
 		rgbGen wave sin 0 0.25 0.25 0.7
 	}
 }

// -================== ratrail_tube.shader

railTube100
{
 	nopicmip
 	nomipmaps
 	sort nearest
 	cull disable
 	{
 		map gfx/misc/ratrail/railtube1.tga
 		blendFunc add
 		rgbGen entity
 	}
 	{
 		map gfx/misc/ratrail/railtube1_glow.tga
 		blendFunc add
 		rgbGen wave sin 0 0.5 0.25 0.6
 	}
}
// scaled up texture for shorter tubes
railTube50
{
 	nopicmip
 	nomipmaps
 	sort nearest
 	cull disable
 	{
 		map gfx/misc/ratrail/railtube1.tga
		tcMod scale 0.5 1

 		blendFunc add
 		rgbGen entity
 	}
 	{
 		map gfx/misc/ratrail/railtube1_glow.tga
		tcMod scale 0.5 1
 		blendFunc add
 		rgbGen wave sin 0 0.5 0.25 0.6
 	}
}
