// NEW BANNER
// TOP (QUAKE III)
models/mapobjects/banner/q3banner12
{      
    cull disable
    nomipmaps
    //deformVertexes wave 70 sin 0 .7 0 .4

	{
		map models/mapobjects/banner/q3banner12.tga
		blendFunc add
		tcmod scale  2  1
		tcmod scroll .33 0
		rgbGen wave sin .5 .5 0 .5
		rgbGen entity
	}
	{
		map models/mapobjects/banner/q3banner12.tga
		blendFunc add
		tcmod scale  3  1
		tcMod turb 1 .25 0 .6
		tcmod scroll -.45 0
		rgbGen wave sin 0 1 0 1
		rgbGen const ( 0 0 0 )
	} 
	{
		map models/mapobjects/banner/q3banner12x.tga
		blendFunc add
		tcmod scale  4  1
		tcmod scroll 1 0
		rgbGen entity
		alphaGen wave sin .2 1 0 1
	} 
}

// BOTTOM: (ARENA)
models/mapobjects/banner/q3banner14
{      
  cull disable 
  nomipmaps
  
	{
		map textures/sfx/clouds.jpg
		blendFunc blend
		tcMod scale 0.2 0.2
		tcMod scroll 0.02 0
		rgbGen identity
	}
	{
		map textures/sfx/clouds.jpg
		blendFunc blend
		tcMod scale 0.25 0.25
		tcMod scroll -0.015 0
		rgbGen identity
	}
	{
		map textures/sfx/lightnings.jpg
		blendfunc add
		rgbGen entity	
		tcmod scale .05 .05
		tcMod turb 0.1 .1 0 .1
		tcmod scroll .2 .15
		alphaGen wave triangle 0.3 0.3 0 1
	}  	
}
