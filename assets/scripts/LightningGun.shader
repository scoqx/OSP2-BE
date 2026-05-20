//lightningBolt // cg_altLightning 0 from vanilla
// {
// 	cull none
// 	{
// 		map gfx/misc/lightning3.tga
// 		blendFunc GL_ONE GL_ONE
//                 rgbgen wave sin 1 0.5 0 7.1
//                  tcmod scale  2 1
// 		tcMod scroll -5 0
// 	}
//     {
// 		map gfx/misc/lightning3.tga
// 		blendFunc GL_ONE GL_ONE
//                 rgbgen wave sin 1 0.8 0 8.1
//                 tcmod scale  -1.3 -1
// 		tcMod scroll -7.2 0
// 	}
// }

// lightningBoltNew // cg_altLightning 1 from OSP1.03
// {
// 	cull none
// 	{
// 		map gfx/misc/lightning3new.tga
// 		blendFunc GL_ONE GL_ONE
// 		rgbgen wave sin 1 0.5 0 7.1
// 		tcmod scale  2 1
// 		tcMod scroll -5 0
// 	}

// 	{
// 		map gfx/misc/lightning3new.tga
// 		blendFunc GL_ONE GL_ONE
// 		rgbgen wave sin 1 0.8 0 8.1
// 		tcmod scale  -1.3 -1
// 		tcMod scroll -7.2 0
// 	}
// }

lightningBoltNew2  //cg_altLightning 2
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

lightningBoltNew3 //cg_altLightning 3
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

lightningBoltNew4  //cg_altLightning 4
{
	nopicmip
	nomipmaps
	cull none
	{
		map gfx/misc/lightning0.tga
    	blendFunc GL_ONE GL_ONE
		rgbgen wave sin 1 0.5 0 1.1
		tcmod scale  2 1
		tcMod scroll -5 0
	}
	{
		map gfx/misc/lightning0.tga
    	blendFunc GL_ONE GL_ONE
		rgbgen wave sin 1 0.8 0 1.1
		tcmod scale  -1.3 -1
		tcMod scroll -7.2 0
	}
}
lightningBoltNewNoPicmip4
{
	nopicmip
	nomipmaps
	cull none
	{
		map gfx/misc/lightning0.tga
    	blendFunc GL_ONE GL_ONE
		rgbgen wave sin 1 0.5 0 1.1
		tcmod scale  2 1
		tcMod scroll -5 0
	}
	{
		map gfx/misc/lightning0.tga
    	blendFunc GL_ONE GL_ONE
		rgbgen wave sin 1 0.8 0 1.1
		tcmod scale  -1.3 -1
		tcMod scroll -7.2 0
	}
}

lightningBoltNew5 //cg_altLightning 5
{
  nopicmip
  nomipmaps
  cull none
  {
    map gfx/misc/lightning2.tga
		blendfunc add
		tcMod scale 1.5 1
		tcMod scroll -1.8 0
  }
  {
    map gfx/misc/lightning2.tga
		blendfunc add
		tcMod scale -1.5 -1
		tcMod scroll -4 0
  }
}
lightningBoltNewNoPicmip5
{
  nopicmip
  nomipmaps
  cull none
  {
    map gfx/misc/lightning2.tga
		blendfunc add
		tcMod scale 1.5 1
		tcMod scroll -1.8 0
  }
  {
    map gfx/misc/lightning2.tga
		blendfunc add
		tcMod scale -1.5 -1
		tcMod scroll -4 0
  }
}


lightningBoltNew6 //cg_altLightning 6
{
  nopicmip
  nomipmaps
  cull none
  {
    map gfx/misc/lightning3.tga
    	blendFunc GL_ONE GL_ONE
		rgbgen wave sin 1 0.5 0 7.1
		tcMod scroll -5 0
  }
  {
    map gfx/misc/lightning3.tga
		blendFunc GL_ONE GL_ONE
		rgbgen wave sin 1 0.8 0 8.1
		tcMod scroll -7.2 0
  }
}
lightningBoltNewNoPicmip6
{
  nopicmip
  nomipmaps
  cull none
  {
    map gfx/misc/lightning3.tga
    	blendFunc GL_ONE GL_ONE
		rgbgen wave sin 1 0.5 0 7.1
		tcMod scroll -5 0
  }
  {
    map gfx/misc/lightning3.tga
		blendFunc GL_ONE GL_ONE
		rgbgen wave sin 1 0.8 0 8.1
		tcMod scroll -7.2 0
  }
}


lightningBoltNew7 //cg_altLightning 7
{
	nopicmip
	//novlcollapse
	cull disable
	{
		map gfx/misc/lightning4.tga
		blendfunc add
		tcMod scale 4 1
		tcMod scroll -4 0
	}
}
lightningBoltNewNoPicmip7
{
	nopicmip
	//novlcollapse
	cull disable
	{
		map gfx/misc/lightning4.tga
		blendfunc add
		tcMod scale 4 1
		tcMod scroll -4 0
	}
}


lightningBoltNew8 //cg_altLightning 8
{
	nopicmip
	//novlcollapse
	cull disable
	{
		map gfx/misc/lightning5.tga
		blendfunc add
		rgbgen wave sin 1 0.1 0 3
	}
}
lightningBoltNewNoPicmip8
{
	nopicmip
	//novlcollapse
	cull disable
	{
		map gfx/misc/lightning5.tga
		blendfunc add
		rgbgen wave sin 1 0.1 0 3
	}
}


lightningBoltNew9 //cg_altLightning 9
{
  nopicmip
  nomipmaps
  cull none
    {
        map gfx/misc/lightning6.tga
        blendFunc add
		rgbgen wave sin 1 0.1 0 3
  }
  {
        map gfx/misc/lightning6.tga
        blendFunc add
		rgbgen wave sin 1 0.1 0 3
	}
}
lightningBoltNewNoPicmip9
{
  nopicmip
  nomipmaps
  cull none
    {
        map gfx/misc/lightning6.tga
        blendFunc add
		rgbgen wave sin 1 0.1 0 3
  }
  {
        map gfx/misc/lightning6.tga
        blendFunc add
		rgbgen wave sin 1 0.1 0 3
	}
}

lightningBoltNew10 //cg_altLightning 10
{
  nopicmip
  nomipmaps
  cull none
    {
		map gfx/misc/lightning7.tga
		blendFunc add
		rgbgen wave sin 1 0.1 0 3
  }
  {
        map gfx/misc/lightning7.tga
        blendFunc add
		rgbgen wave sin 1 0.1 0 3
	}
}
lightningBoltNewNoPicmip10
{
  nopicmip
  nomipmaps
  cull none
    {
		map gfx/misc/lightning7.tga
		blendFunc add
		rgbgen wave sin 1 0.1 0 3
  }
  {
        map gfx/misc/lightning7.tga
        blendFunc add
		rgbgen wave sin 1 0.1 0 3
	}
}

lightningBoltNew11  //cg_altLightning 11
{
  nopicmip
  nomipmaps
  cull none
  {
    map gfx/misc/lightning1.tga
		blendfunc add
		tcMod scale 1 1
		tcMod scroll -2.5 0
  }
  {
    map gfx/misc/lightning1.tga
		blendfunc add
		tcMod scale -1 -1
		tcMod scroll -6 0
  }
}
lightningBoltNoPicMipNew11
{
  nopicmip
  nomipmaps
  cull none
  {
    map gfx/misc/lightning1.tga
		blendfunc add
		tcMod scale 1 1
		tcMod scroll -2.5 0
  }
  {
    map gfx/misc/lightning1.tga
		blendfunc add
		tcMod scale -1 -1
		tcMod scroll -6 0
  }
}

// Enemy lightning bolt

enemyLightningBoltNew //cg_altLightning 0 
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

enemyLightningBoltNoPicMip //cg_altLightning 1 (nomip 1)
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

enemyLightningBolt //cg_altLightning 1
{
	nopicmip
	//novlcollapse
	cull none
  {
    map gfx/misc/enm_lightning3.tga
    	blendfunc add
		//rgbgen wave sin 1 0.5 0 7.1
		tcMod scroll -5 0
		rgbGen vertex
  }
  {
    map gfx/misc/enm_lightning3.tga
		blendfunc add
		//rgbgen wave sin 1 0.8 0 8.1
		tcMod scroll -7.2 0
		rgbGen vertex
  }
}

enemyLightningBoltNew2 //cg_altLightning 2
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

enemyLightningBoltNew3 //cg_altLightning 3
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
// New here

enemyLightningBoltNew4 //cg_altLightning 4
{
  nopicmip
  nomipmaps
  cull none
  {
    map gfx/misc/enm_lightning0.tga
		blendfunc add
		tcMod scale 1 1
		tcMod scroll -2.5 0
		rgbGen vertex
  }
  {
    map gfx/misc/enm_lightning0.tga
		blendfunc add
		tcMod scale -1 -1
		tcMod scroll -6 0
		rgbGen vertex
  }
}

enemyLightningBoltNew5 //cg_altLightning 5
{
  nopicmip
  nomipmaps
  cull none
  {
    map gfx/misc/enm_lightning2.tga
		blendfunc add
		tcMod scale 1.5 1
		tcMod scroll -1.8 0
        rgbGen vertex
  }
  {
    map gfx/misc/enm_lightning2.tga
		blendfunc add
		tcMod scale -1.5 -1
		tcMod scroll -4 0
        rgbGen vertex
  }
}

enemyLightningBoltNew6 //cg_altLightning 6
{
	nopicmip
	//novlcollapse
	cull none
  {
    map gfx/misc/enm_lightning3.tga
    	blendfunc add
		//rgbgen wave sin 1 0.5 0 7.1
		tcMod scroll -5 0
		rgbGen vertex
  }
  {
    map gfx/misc/enm_lightning3.tga
		blendfunc add
		//rgbgen wave sin 1 0.8 0 8.1
		tcMod scroll -7.2 0
		rgbGen vertex
  }
}

enemyLightningBoltNew7 //cg_altLightning 7
{
	nopicmip
	//novlcollapse
	cull disable
	{
		map gfx/misc/enm_lightning4.tga
		blendfunc add
		tcMod scale 4 1
		tcMod scroll -4 0
		rgbGen vertex
	}
}

enemyLightningBoltNew8 //cg_altLightning 8
{
	nopicmip
	//novlcollapse
	cull disable
	{
    map gfx/misc/enm_lightning5.tga
		blendfunc add
		rgbgen wave sin 1 0.1 0 3
	    rgbGen vertex
	}
}

enemyLightningBoltNew9 //cg_altLightning 9
{
	nopicmip
	nomipmaps
	cull none
	{
    map gfx/misc/enm_lightning6.tga
    blendFunc add
	rgbgen wave sin 1 0.1 0 3
	rgbGen vertex
    }
    {
	map gfx/misc/enm_lightning6.tga
	blendFunc add
	rgbgen wave sin 1 0.1 0 3
	rgbGen vertex
	}
}

enemyLightningBoltNew10 //cg_altLightning 10
{
	nopicmip
	nomipmaps
	cull none
	{
    map gfx/misc/enm_lightning7.tga
    blendFunc add
	rgbgen wave sin 1 0.1 0 3
	rgbGen vertex
    }
    {
	map gfx/misc/enm_lightning7.tga
	blendFunc add
	rgbgen wave sin 1 0.1 0 3
	rgbGen vertex
	}
}

enemyLightningBoltNew11 //cg_altLightning 11
{
	nopicmip
	nomipmaps
	cull none
	{
		map gfx/misc/enm_lightning1.tga
    	blendFunc GL_ONE GL_ONE
		//rgbgen wave sin 1 0.5 0 1.1
		tcmod scale  2 1
		tcMod scroll -5 0
		rgbGen vertex
	}
	{
		map gfx/misc/enm_lightning1.tga
    	blendFunc GL_ONE GL_ONE
		//rgbgen wave sin 1 0.8 0 1.1
		tcmod scale  -1.3 -1
		tcMod scroll -7.2 0
		rgbGen vertex
	}
}