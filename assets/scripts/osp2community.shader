gfx/2d/hudres/Grid
{
    {
        map gfx/2d/hudres/Grid.tga
		tcMod scale 2 2
        blendfunc blend
        rgbGen vertex
    }

}

gfx/2d/hudres/background
{
    
    {
		map $lightmap
		rgbGen const ( 0 0 0 )
    }
    {
        map gfx/2d/hudres/background.tga
        blendfunc GL_ONE GL_ONE
        tcMod scroll 0.1 0
        tcMod scale 1 0.25
        rgbGen identity
    }
    {
        map $lightmap
        blendFunc GL_DST_COLOR GL_ONE_MINUS_DST_ALPHA
        rgbGen identity
    }
}

gfx/2d/hudres/Timer
{
    {
        map gfx/2d/hudres/Timer.tga
        blendfunc blend
        rgbgen vertex   
    }
	{
        map gfx/2d/hudres/ClockHand.tga
        tcMod rotate 40
        blendfunc blend
		rgbgen vertex   
    }
}


gfx/2d/hudres/Timer2
{
    {
        map gfx/2d/hudres/Timer.tga
        blendfunc blend
        rgbgen vertex   
    }
}

gfx/2d/hudres/ekg
{
   nopicmip
    {
        map gfx/2d/hudres/ekg.tga
        blendfunc blend
        tcMod scroll 1 0
        tcMod scale 1 1
        rgbGen identity
    }
}

gfx/2d/hudres/s_ekg
{
   nopicmip
    {
        map gfx/2d/hudres/s_ekg.tga
        blendfunc blend
        tcMod scroll 0.7 0
        tcMod scale 1 1
        rgbGen identity
    }
}

gfx/2d/n8hudres/heart_attack
{
    {
        map gfx/2d/n8hudres/heart_attack.tga
        blendfunc add
        tcMod scroll -0.1 0
    }
}