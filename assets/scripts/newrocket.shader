// cg_newRocket 1: rocket and its explosion from Black edition

models/ammo/rocket_new/rockflar
{
	cull disable
	nopicmip
	nomipmaps
	{
		map models/ammo/rocket_new/rockflar.tga
		blendfunc GL_ONE GL_ONE
	}
}

models/ammo/rocket_new/rockfls1
{
	cull disable
	nopicmip
	nomipmaps
	{
		map models/ammo/rocket_new/rockfls1.jpg
		blendfunc GL_ONE GL_ONE
	}
}

rocketExplosionNew
{
	cull disable
	{
		animmap 8 models/weaphits/rlboom_new/rlboom_1.jpg models/weaphits/rlboom_new/rlboom_2.jpg models/weaphits/rlboom_new/rlboom_3.jpg models/weaphits/rlboom_new/rlboom_4.jpg models/weaphits/rlboom_new/rlboom_5.jpg models/weaphits/rlboom_new/rlboom_6.jpg models/weaphits/rlboom_new/rlboom_7.jpg models/weaphits/rlboom_new/rlboom_8.jpg
		blendfunc add
		rgbGen wave inversesawtooth 0 1 0 8
	}
	{
		animmap 8 models/weaphits/rlboom_new/rlboom_2.jpg models/weaphits/rlboom_new/rlboom_3.jpg models/weaphits/rlboom_new/rlboom_4.jpg models/weaphits/rlboom_new/rlboom_5.jpg models/weaphits/rlboom_new/rlboom_6.jpg models/weaphits/rlboom_new/rlboom_7.jpg models/weaphits/rlboom_new/rlboom_8.jpg gfx/colors/black.jpg
		blendfunc add
		rgbGen wave sawtooth 0 1 0 8
	}
}

rocketExplosionNewNoPicMip
{
	cull disable
	{
		animmap 8 models/weaphits/rlboom_new/rlboom_1.jpg models/weaphits/rlboom_new/rlboom_2.jpg models/weaphits/rlboom_new/rlboom_3.jpg models/weaphits/rlboom_new/rlboom_4.jpg models/weaphits/rlboom_new/rlboom_5.jpg models/weaphits/rlboom_new/rlboom_6.jpg models/weaphits/rlboom_new/rlboom_7.jpg models/weaphits/rlboom_new/rlboom_8.jpg
		blendfunc add
		rgbGen wave inversesawtooth 0 1 0 8
	}
	{
		animmap 8 models/weaphits/rlboom_new/rlboom_2.jpg models/weaphits/rlboom_new/rlboom_3.jpg models/weaphits/rlboom_new/rlboom_4.jpg models/weaphits/rlboom_new/rlboom_5.jpg models/weaphits/rlboom_new/rlboom_6.jpg models/weaphits/rlboom_new/rlboom_7.jpg models/weaphits/rlboom_new/rlboom_8.jpg gfx/colors/black.jpg
		blendfunc add
		rgbGen wave sawtooth 0 1 0 8
	}
}
