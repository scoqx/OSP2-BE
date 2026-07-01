/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// cg_main.c -- initialization and primary entry point for cgame
#include "cg_local.h"
#include "../qcommon/qcommon.h"
#include "../qcommon/beinfo_cfg.h"

int forceModelModificationCount = -1;

int CG_Init(int serverMessageNum, int serverCommandSequence, int clientNum);
void CG_Shutdown(void);


/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
Q_EXPORT
int vmMain(int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11)
{

	switch (command)
	{
		case CG_INIT:
			return CG_Init(arg0, arg1, arg2);
		case CG_SHUTDOWN:
			CG_Shutdown();
			return 0;
		case CG_CONSOLE_COMMAND:
			return CG_ConsoleCommand();
		case CG_DRAW_ACTIVE_FRAME:
			CG_DrawActiveFrame(arg0, arg1, arg2);
			return 0;
		case CG_CROSSHAIR_PLAYER:
			return CG_CrosshairPlayer();
		case CG_LAST_ATTACKER:
			return CG_LastAttacker();
		case CG_KEY_EVENT:
			CG_KeyEvent(arg0, arg1);
			return 0;
		case CG_MOUSE_EVENT:
			CG_MouseEvent(arg0, arg1);
			return 0;
		case CG_EVENT_HANDLING:
			CG_EventHandling(arg0);
			return 0;
		default:
			CG_Error("vmMain: unknown command %i", command);
			break;
	}
	return -1;
}


cg_t                cg;
cgs_t               cgs;
crosshairData_t     crosshairData;
centity_t           cg_entities[MAX_GENTITIES];
weaponInfo_t        cg_weapons[MAX_WEAPONS];
itemInfo_t          cg_items[MAX_ITEMS];


vmCvar_t           osp_client;
vmCvar_t           osp_hidden;
vmCvar_t           osp_debug;
vmCvar_t           cg_autoswitch;
vmCvar_t           cg_drawGun;
vmCvar_t           cg_zoomFov;
vmCvar_t           cg_zoomTime;
vmCvar_t           cg_zoomSensitivityAuto;
vmCvar_t           cg_zoomSensitivityMultiplier;
vmCvar_t           cg_fov;
vmCvar_t           cg_viewsize;
vmCvar_t           cg_stereoSeparation;
vmCvar_t           cg_shadows;
vmCvar_t           cg_gibs;
vmCvar_t           cg_draw2D;
vmCvar_t           cg_drawStatus;
vmCvar_t           cg_drawTimer;
vmCvar_t           cg_drawFPS;
vmCvar_t           cg_drawSnapshot;
vmCvar_t           cg_draw3dIcons;
vmCvar_t           cg_drawIcons;
vmCvar_t           cg_drawAmmoWarning;
vmCvar_t           cg_drawAttacker;
vmCvar_t           cg_drawRewards;
vmCvar_t           cg_brassTime;
vmCvar_t           cg_simpleItems;
vmCvar_t           cg_addMarks;
vmCvar_t           cg_lagometer;
vmCvar_t           cg_railTrailTime;
vmCvar_t           cg_gun_x;
vmCvar_t           cg_gun_y;
vmCvar_t           cg_gun_z;
vmCvar_t           cg_centertime;
vmCvar_t           cg_runpitch;
vmCvar_t           cg_runroll;
vmCvar_t           cg_bobup;
vmCvar_t           cg_bobpitch;
vmCvar_t           cg_bobroll;
vmCvar_t           cg_swingSpeed;
vmCvar_t           cg_animSpeed;
vmCvar_t           cg_debugAnim;
vmCvar_t           cg_debugPosition;
vmCvar_t           cg_debugEvents;
vmCvar_t           cg_errorDecay;
vmCvar_t           cg_nopredict;
vmCvar_t           cg_noPlayerAnims;
vmCvar_t           cg_showmiss;
vmCvar_t           cg_footsteps;
vmCvar_t           cg_tracerChance;
vmCvar_t           cg_tracerWidth;
vmCvar_t           cg_tracerLength;
vmCvar_t           cg_thirdPersonRange;
vmCvar_t           cg_thirdPersonAngle;
vmCvar_t           cg_thirdPerson;
vmCvar_t           cg_teamChatTime;
vmCvar_t           cg_teamChatHeight;
vmCvar_t           cg_forceModel;
vmCvar_t           cg_predictItems;
vmCvar_t           cg_deferPlayers;
vmCvar_t           cg_drawTeamOverlay;
vmCvar_t           teamoverlay;
vmCvar_t           cg_stats;
vmCvar_t           cg_drawFriend;
vmCvar_t           cg_buildScript;
vmCvar_t           cg_paused;
vmCvar_t           com_blood;
vmCvar_t           cg_synchronousClients;
vmCvar_t           cg_cameraOrbit;
vmCvar_t           cg_cameraOrbitDelay;
vmCvar_t           cg_timescaleFadeEnd;
vmCvar_t           cg_timescaleFadeSpeed;
vmCvar_t           cg_timescale;
vmCvar_t           cg_scorePlums;
vmCvar_t           cg_smoothClients;
vmCvar_t           cg_cameraMode;
vmCvar_t           pmove_fixed;
vmCvar_t           pmove_msec;
vmCvar_t		   phy_movetype;
vmCvar_t		   phy_snapVectors;
vmCvar_t           cg_noTaunt;
vmCvar_t           cg_noProjectileTrail;
vmCvar_t           cg_oldRail;
vmCvar_t           cg_oldRocket;
vmCvar_t           cg_oldPlasma;
vmCvar_t           cg_altGrenades;
vmCvar_t           cg_altLightning;
vmCvar_t           cg_altPlasma;
vmCvar_t           cg_ammoCheck;
vmCvar_t           cg_autoAction;
vmCvar_t           cg_clientLog;
vmCvar_t           cg_customLoc;
vmCvar_t           cg_damageSound;
vmCvar_t           cg_damageDraw;
vmCvar_t           cg_damageIndicatorScale;
vmCvar_t           cg_damageIndicatorOpaque;
vmCvar_t           cg_damageKick;
vmCvar_t           cg_deadBodyFilter;
vmCvar_t           cg_deadBodyBlack;
vmCvar_t           cg_deadBodyInvisible;
vmCvar_t           cg_drawDecals;
vmCvar_t           cg_drawPing;
vmCvar_t           cg_enableOSPHUD;
vmCvar_t           cg_shud;
vmCvar_t           cg_enableBreath;
vmCvar_t           cg_enemyColors;
vmCvar_t           cg_enemyModel;
vmCvar_t           cg_teamColors;
vmCvar_t           cg_teamModel;
vmCvar_t           cg_execVstr;
vmCvar_t           cg_fallKick;
vmCvar_t           cg_followkiller;
vmCvar_t           cg_followpowerup;
vmCvar_t           cg_followviewcam;
vmCvar_t           cg_forceColors;
vmCvar_t           cg_lightningImpact;
vmCvar_t           cg_MaxlocationWidth;
vmCvar_t           cg_muzzleFlash;
vmCvar_t           cg_nochatbeep;
vmCvar_t           cg_noTeamChatBeep;
vmCvar_t           cg_NoMOTDDisplayDuringDemo;
vmCvar_t           cg_nomip;
vmCvar_t           cg_oversampleMouse;
vmCvar_t           cg_statScrollTime;
vmCvar_t           cg_q3compScoreboard;
vmCvar_t           cg_scoreTransparency;
vmCvar_t           cg_showPlayerLean;
vmCvar_t           cg_smoke_sg;
vmCvar_t           cg_smokegrowth_gl;
vmCvar_t           cg_smokegrowth_rl;
vmCvar_t           cg_smokeradius_gl;
vmCvar_t           cg_smokeradius_rl;
vmCvar_t           cg_smokedensity_gl;
vmCvar_t           cg_smokedensity_rl;
vmCvar_t           cg_swapSkins;
vmCvar_t           cg_teamRails;
vmCvar_t           cg_trueLightning;
vmCvar_t           cg_useScreenShotJPEG;
vmCvar_t           ch_3waveFont;
vmCvar_t           ch_ColorLocations;
vmCvar_t           ch_drawHealthMeters;
vmCvar_t           ch_drawFlagNames;
vmCvar_t           ch_drawPickup;
vmCvar_t           ch_drawSpeed;
vmCvar_t           ch_TeamBackground;
vmCvar_t           cf_Following;
vmCvar_t           ch_FilterLocationsTeamchat;
vmCvar_t           ch_fragMessage;
vmCvar_t           cf_Fragmsg;
vmCvar_t           ch_graphs;
vmCvar_t           ch_InverseTeamChat;
vmCvar_t           ch_Obituaries;
vmCvar_t           ch_Pickupitem;
vmCvar_t           cf_Pickupstring;
vmCvar_t           ch_recordMessage;
vmCvar_t           cf_Scores;
vmCvar_t           cf_Statusbar;
vmCvar_t           ch_StatusbarFormat;
vmCvar_t           ch_statusbarStyle;
vmCvar_t           ch_teamBackground;
vmCvar_t           cf_Teamchat;
vmCvar_t           ch_TeamchatOnly;
vmCvar_t           ch_Teamoverlay;
vmCvar_t           cf_Teamoverlay;
vmCvar_t           cf_Vote;
vmCvar_t           ch_weaponList;
vmCvar_t           ch_weaponListBackground;
vmCvar_t           ch_weaponListDrawAll;
vmCvar_t           ch_weaponListFlash;
vmCvar_t           ch_Weaponswitch;
vmCvar_t           cf_WeaponSwitch;
vmCvar_t           cf_WeaponName;
vmCvar_t           cf_AmmoStatusbar;
vmCvar_t           cf_timer;
vmCvar_t           s_ambient;
vmCvar_t           cl_maxpackets;
vmCvar_t           cl_timenudge;
vmCvar_t           snaps;
vmCvar_t           r_shownormals;
vmCvar_t           r_fullbright;
vmCvar_t           r_lodCurveError;
vmCvar_t           r_showtris;
vmCvar_t           r_subdivisions;
vmCvar_t           r_znear;
vmCvar_t           handicap;
vmCvar_t           m_pitch;
vmCvar_t           com_maxfps;
vmCvar_t           cl_pitchspeed;
vmCvar_t           cl_yawspeed;

vmCvar_t           cg_noLeadSounds;
vmCvar_t           cg_fragSound;
vmCvar_t           cg_lightningHideCrosshair;
vmCvar_t           cg_lightningSilent;
vmCvar_t           cg_lightningHide;

vmCvar_t           cg_delag;
vmCvar_t           cg_drawHitBox;
vmCvar_t           cg_projectileNudge;
vmCvar_t           cg_hideScores;

vmCvar_t           cg_enemyModelColorsUnique;
vmCvar_t           cg_playerModelColors;
vmCvar_t           cg_playerRailColors;
vmCvar_t           cg_playerFrozenColor;

vmCvar_t           cg_teamModelColors;
vmCvar_t           cg_teamRailColors;
vmCvar_t           cg_teamFrozenColor;

vmCvar_t           cg_teamFrozenFoe;

vmCvar_t           cg_enemyModelColors;
vmCvar_t           cg_enemyRailColors;
vmCvar_t           cg_enemyFrozenColor;


vmCvar_t           cg_spectGlow;
vmCvar_t           cg_spectOrigModel;
vmCvar_t           cg_hitSounds;

vmCvar_t           ch_file;
vmCvar_t           cg_playersXID;
vmCvar_t           cg_playersID;

vmCvar_t           cg_chatEnable;
vmCvar_t           cg_shudChatEnable;

vmCvar_t           cg_healthMid;
vmCvar_t           cg_healthLow;

vmCvar_t           cg_healthColorLevels;

vmCvar_t           cg_printui;
vmCvar_t           cg_printui_template;
vmCvar_t           cg_printui_template2;

vmCvar_t           cg_drawCrosshair;
vmCvar_t           cg_drawCrosshairGauntlet;
vmCvar_t           cg_drawCrosshairMachinegun;
vmCvar_t           cg_drawCrosshairShotgun;
vmCvar_t           cg_drawCrosshairGrenadeLauncher;
vmCvar_t           cg_drawCrosshairRocketLauncher;
vmCvar_t           cg_drawCrosshairLightning;
vmCvar_t           cg_drawCrosshairRailgun;
vmCvar_t           cg_drawCrosshairPlasmagun;
vmCvar_t           cg_drawCrosshairBFG;
vmCvar_t           cg_drawCrosshairNames;

vmCvar_t           cg_crosshairSize;
vmCvar_t           cg_crosshairAspectRatioFix;
vmCvar_t           cg_crosshairHealth;
vmCvar_t           cg_crosshairX;
vmCvar_t           cg_crosshairY;
vmCvar_t           ch_CrosshairColor;
vmCvar_t           cg_crosshairPulse;
vmCvar_t           cf_CrosshairNames;
vmCvar_t           ch_CrosshairNamesLeft;
vmCvar_t           ch_crosshairTeamInfo;
vmCvar_t           ch_TeamCrosshairHealth;
vmCvar_t           cf_TeamCrosshairHealth;
vmCvar_t           ch_crosshairDecor;
vmCvar_t           ch_crosshairDecorColor;
vmCvar_t           ch_crosshair45;
vmCvar_t           ch_crosshairDecorSize;
vmCvar_t           ch_crosshairDecor45;
vmCvar_t           ch_crosshairOpaque;
vmCvar_t           ch_crosshairDecorOpaque;

vmCvar_t           ch_crosshairAction;
vmCvar_t           ch_crosshairActionColor;
vmCvar_t           ch_crosshairActionScale;
vmCvar_t           ch_crosshairActionTime;
vmCvar_t           ch_crosshairDecorAction;
vmCvar_t           ch_crosshairDecorActionScale;
vmCvar_t           ch_crosshairDecorActionColor;
vmCvar_t           ch_crosshairDecorActionTime;

vmCvar_t           ch_crosshairAutoScale;

vmCvar_t           cg_dlightGauntlet;
vmCvar_t           cg_dlightMG;
vmCvar_t           cg_dlightSG;
vmCvar_t           cg_dlightGL;
vmCvar_t           cg_dlightRL;
vmCvar_t           cg_dlightLG;
vmCvar_t           cg_dlightRG;
vmCvar_t           cg_dlightPG;
vmCvar_t           cg_dlightBFG;
vmCvar_t           cg_gunColor;
vmCvar_t           cg_gunOpaque;
vmCvar_t           cg_drawBrightWeapons;
vmCvar_t           cg_conObituaries;
vmCvar_t           cg_lightningHitsoundRateFix;
vmCvar_t           cg_stackHitSounds;
vmCvar_t           cg_stackHitSoundsTimeout;
vmCvar_t           cg_drawCenterMessages;
vmCvar_t        cg_predictStepOffset;
vmCvar_t        cg_uniqueColorTable;
vmCvar_t        cg_noVoteBeep;
vmCvar_t        cg_damageDrawFrame;
vmCvar_t        cg_damageFrameSize;
vmCvar_t        cg_damageFrameOpaque;
vmCvar_t            cg_shud_currentWeapons;
vmCvar_t        cg_hitBoxColor;
vmCvar_t        cg_drawGunForceAspect;
vmCvar_t        cg_drawOutline;
vmCvar_t        cg_enemyOutlineColor;
vmCvar_t        cg_teamOutlineColor;
vmCvar_t        cg_enemyOutlineColorUnique;
vmCvar_t        cg_enemyOutlineSize;
vmCvar_t        cg_teamOutlineSize;
vmCvar_t        cg_underwaterFovWarp;
vmCvar_t        cg_altBlood;
vmCvar_t        cg_altBloodColor;
vmCvar_t        cg_shotGunTracer;
vmCvar_t        cg_railRingsRadius;
vmCvar_t        cg_railRingsRotation;
vmCvar_t        cg_railRingsSpacing;
vmCvar_t        cg_railRingsSize;
vmCvar_t        cg_railStaticRings;
vmCvar_t        cg_gunPos;
vmCvar_t        cg_altShadow;
vmCvar_t        cg_altShadowColor;
vmCvar_t        cg_scoreboardShowId;
vmCvar_t        cg_teamIndicator;
vmCvar_t        cg_teamIndicatorAdjust;
vmCvar_t        cg_teamIndicatorColor;
vmCvar_t        cg_teamIndicatorOpaque;
vmCvar_t        cg_teamIndicatorBgColor;
vmCvar_t        cg_teamIndicatorBgOpaque;
vmCvar_t        cg_teamIndicatorOffset;
vmCvar_t        cg_teamIndicatorMaxLength;
vmCvar_t        cg_scoreboardBE;
vmCvar_t        cg_scoreboardFont;
vmCvar_t        cg_teamIndicatorFont;
vmCvar_t        cg_centerMessagesFont;
vmCvar_t        cg_railCustomChamber;
vmCvar_t        cg_altGrenadesColor;
vmCvar_t        cg_enemyGrenadesColor;
vmCvar_t        cg_drawGrenadeOwner;
vmCvar_t        cg_itemFx;
vmCvar_t        cg_bubbleTrail;
vmCvar_t        cg_ignoreServerMessages;
vmCvar_t        cg_healthColor;
vmCvar_t        cg_healthLowColor;
vmCvar_t        cg_healthMidColor;
vmCvar_t        cg_redTeamColor;
vmCvar_t        cg_blueTeamColor;
vmCvar_t        cg_drawAccuracy;
vmCvar_t        cg_accuracyFontSize;
vmCvar_t        cg_accuracyIconSize;
vmCvar_t        cg_accuracyFont;

vmCvar_t        ch_crosshairActionColorLow;
vmCvar_t        ch_crosshairActionColorMid;
vmCvar_t        ch_crosshairActionColorHigh;
vmCvar_t        ch_crosshairDecorActionColorLow;
vmCvar_t        ch_crosshairDecorActionColorMid;
vmCvar_t        ch_crosshairDecorActionColorHigh;

vmCvar_t        cg_friendHudMarkerMaxDist;
vmCvar_t        cg_friendHudMarkerSize;
vmCvar_t        cg_friendHudMarkerMaxScale;
vmCvar_t        cg_friendHudMarkerMinScale;
vmCvar_t        cg_friendsWallhack;
vmCvar_t        cg_drawHudMarkers;
vmCvar_t        cg_markTeam;
vmCvar_t        cg_markTeamColor;
vmCvar_t        cg_mySound;
vmCvar_t        cg_teamSound;
vmCvar_t        cg_enemySound;
vmCvar_t        cg_scoreboardRtColors;
vmCvar_t        cg_scoreboardBtColors;
vmCvar_t        cg_scoreboardSpecColor;
vmCvar_t        cg_scoreboardDrawPowerUps;
vmCvar_t        cg_bestats_style;
vmCvar_t        cg_bestats_textSize;
vmCvar_t        cg_bestats_font;
vmCvar_t        cg_bestats_pos;
vmCvar_t        cg_bestats_bgColor;
vmCvar_t        cg_bestats_bgOpaque;
vmCvar_t        cg_bestats_spacingAdjust;
vmCvar_t        cg_bestats_widthCutoff;
vmCvar_t        cg_teamIndicatorFade;
vmCvar_t        cg_teamIndicatorFadeRadius;
vmCvar_t        be_features;
vmCvar_t        compilation_build;
vmCvar_t        be_enabled;
vmCvar_t        cg_scoreboardTextSize;
vmCvar_t        cg_scoreboardScale;
vmCvar_t        cg_scoreboardWidth;
vmCvar_t        cg_scoreboardColors;
vmCvar_t        cg_clearOnLevelLoad;
vmCvar_t        cg_spectPOV;
vmCvar_t        cg_drawRealHeads;
vmCvar_t        cg_bestats_autoaction;
vmCvar_t        be_run;
vmCvar_t        cg_teamTask;
vmCvar_t        cg_teamTaskMarkTime;
vmCvar_t        cg_teamTaskScale;
vmCvar_t        cg_teamTaskOpaque;
vmCvar_t        cg_teamTaskFade;
vmCvar_t        cg_teamTaskFadeRadius;
vmCvar_t        cg_teamTaskFont;
vmCvar_t        cg_teamTaskWheel;
vmCvar_t        cg_teamTaskWheelSet;
vmCvar_t        cg_teamTaskWheelSensitivity;
vmCvar_t        cg_teamTaskWheelScale;
vmCvar_t        cg_storedSensitivity;
vmCvar_t        sensitivity;
vmCvar_t        cg_teamTaskThemeColor;
vmCvar_t        cg_teamTaskSmartDefault;
vmCvar_t        cg_teamTaskHudMarkerMaxDist;
vmCvar_t        cg_teamTaskHudMarkerSize;
vmCvar_t        cg_teamTaskHudMarkerMaxScale;
vmCvar_t        cg_teamTaskHudMarkerMinScale;
vmCvar_t        cg_zoomAutoReset;
vmCvar_t        cg_smoothDucking;
vmCvar_t        cg_respawnWeapon;
vmCvar_t        cg_exec_weapons;
vmCvar_t        cg_exec_mg;
vmCvar_t        cg_exec_sg;
vmCvar_t        cg_exec_gl;
vmCvar_t        cg_exec_rl;
vmCvar_t        cg_exec_lg;
vmCvar_t        cg_exec_rg;
vmCvar_t        cg_exec_pg;
vmCvar_t        cg_exec_gametypes;
vmCvar_t        cg_exec_dm;
vmCvar_t        cg_exec_tourney;
vmCvar_t        cg_exec_tdm;
vmCvar_t        cg_exec_ca;
vmCvar_t        cg_exec_ctf;
vmCvar_t        cg_exec_freeze;
/* vmCvar_t        cg_autoHop; */
vmCvar_t        cg_damagePlums;
vmCvar_t        cg_damagePlumSize;
vmCvar_t        cg_damagePlumsWeapons;
vmCvar_t        cg_chatColor;
vmCvar_t        cg_teamChatColor;
vmCvar_t        cg_countdownTimer;
vmCvar_t        cg_teamIndicatorFontSize;
vmCvar_t        cg_exec_intermission;
vmCvar_t        cg_exec_warmupEnded;
vmCvar_t        cg_exec_joinGame;
vmCvar_t        cg_exec_joinSpec;
vmCvar_t        cg_exec_gauntlet;
vmCvar_t        cg_exec_bfg;
vmCvar_t        cg_exec_shud;
vmCvar_t        cg_simpleItemsRadius;
vmCvar_t        cg_simpleItemsBob;
vmCvar_t        cg_brightWeaponsOpaque;
vmCvar_t        uc_sensitivity;
vmCvar_t        cg_teamTaskIconMinSize;
vmCvar_t        cg_hitSoundsQC;
vmCvar_t        cg_brightWeaponsShader;
vmCvar_t        teamoverlay2;
vmCvar_t        cg_noExplosions;
vmCvar_t        cg_noItemUseSound;
vmCvar_t        cg_friendsWallhackFrozen;
vmCvar_t        cg_teleportEffect;
vmCvar_t        cg_infoVersionFont;
vmCvar_t        cl_reconnectArgs;
vmCvar_t        cg_altRail;
vmCvar_t        cg_altRailBeefy;
vmCvar_t        cg_teamTaskNewSounds;
vmCvar_t        cg_teamTaskVersion;
vmCvar_t        cg_railFix;
vmCvar_t        cg_freezeSoundFix;
vmCvar_t           cg_infoDetail;
vmCvar_t        cg_enhancedInterpolation;
vmCvar_t        cg_entityOverpolate;
// vmCvar_t        cg_snow;

vmCvar_t        cg_healthBarStyle;
vmCvar_t		cg_teamHealthBar;
vmCvar_t        cg_emojiListFont;
vmCvar_t        cg_emojiListBgColor;
vmCvar_t        cg_emojiListOpaque;
vmCvar_t        cg_battleSuitColor;
vmCvar_t        cg_enemyBattleSuitColor;
vmCvar_t        cg_mutePowerUpCountdown;
vmCvar_t        cg_myTeam;
vmCvar_t        cg_roundSound;
vmCvar_t        cg_debugServer;
vmCvar_t        cg_quadDamageColor;
vmCvar_t        cg_enemyQuadDamageColor;
vmCvar_t        cg_floatSpriteMaxDist;
vmCvar_t        cg_floatSpriteMinSize;
vmCvar_t        cg_altLightningColor;
vmCvar_t        cg_teamLightningColor;
vmCvar_t        cg_enemyLightningColor;
vmCvar_t        cg_enemyAltLightning;
vmCvar_t        cg_teamAltLightning;
vmCvar_t        cg_muteMyJump;
vmCvar_t        cg_muteWorldSounds;
vmCvar_t        cg_disableTextEffects;
vmCvar_t        cg_fragEffectTime;
vmCvar_t        cg_fragEffectColor;
vmCvar_t        cg_fragEffect;
vmCvar_t        cg_drawUniqueColors;
vmCvar_t        cg_killerGhost;
vmCvar_t        cg_numbersSet;
vmCvar_t        cg_oldFlags;
vmCvar_t        cg_drawBrightItems;
vmCvar_t        cg_brightItemsOpaque;
vmCvar_t        cg_myFootsteps;
vmCvar_t        cg_teamFootsteps;
vmCvar_t        cg_enemyFootsteps;
vmCvar_t        cg_oldMedals;
vmCvar_t        cg_damagePlumsTimeout;
vmCvar_t        cg_freezeShader;
vmCvar_t        cg_plasmaColor;
vmCvar_t        cg_enemyPlasmaColor;
vmCvar_t        cg_shudStyle;
vmCvar_t        cg_chatWindow;
vmCvar_t        cg_chatWindowMessageTime;
vmCvar_t        cg_chatWindowPos;
vmCvar_t        cg_chatWindowSize;
vmCvar_t        cg_chatWindowFont;
vmCvar_t        cg_chatWindowFontSize;
vmCvar_t        cg_chatWindowBgColor;
vmCvar_t        cg_chatWindowBgOpaque;
vmCvar_t        cg_drawSpawns;
vmCvar_t        cg_drawTPexit;
vmCvar_t        cg_shadowItems;
vmCvar_t        cg_worldFlags;

cgFontIndices_t cgFontIndices;

static qboolean teamoverlay_registered = qfalse;
static qboolean teamoverlay2_registered = qfalse;

cvarTable_t cvarTable[] =
{
	{ &osp_client, "osp_client", "1008_OSP2_"OSP_VERSION, CVAR_USERINFO | CVAR_ROM },
	{ &osp_hidden, "osp_print_issues", "0", CVAR_ARCHIVE },
	{ &osp_debug, "osp_debug", "0", CVAR_ARCHIVE },
	{ &cg_autoswitch, "cg_autoswitch", "0", CVAR_ARCHIVE },
	{ &cg_drawGun, "cg_drawGun", "1", CVAR_ARCHIVE },
	{ &cg_zoomFov, "cg_zoomfov", "22.5", CVAR_ARCHIVE },
	{ &cg_zoomTime,  "cg_zoomTime", "150",  CVAR_ARCHIVE, NULL},
	{ &cg_zoomSensitivityAuto, "cg_zoomSensitivityAuto", "1",  CVAR_ARCHIVE, NULL},
	{ &cg_zoomSensitivityMultiplier, "cg_zoomSensitivityMultiplier", "0.1",  CVAR_ARCHIVE, NULL},
	{ &cg_fov, "cg_fov", "90", CVAR_ARCHIVE },
	{ &cg_viewsize, "cg_viewsize", "100", CVAR_ARCHIVE },
	{ &cg_stereoSeparation, "cg_stereoSeparation", "0.4", CVAR_ARCHIVE },
	{ &cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE },
	{ &cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE },
	{ &cg_drawTimer, "cg_drawTimer", "1", CVAR_ARCHIVE },
	{ &cg_drawFPS, "cg_drawFPS", "0", CVAR_ARCHIVE },
	{ &cg_drawSnapshot, "cg_drawSnapshot", "0", CVAR_ARCHIVE },
	{ &cg_draw3dIcons, "cg_draw3dIcons", "1", CVAR_ARCHIVE },
	{ &cg_drawIcons, "cg_drawIcons", "1", CVAR_ARCHIVE },
	{ &cg_drawAttacker, "cg_drawAttacker", "1", CVAR_ARCHIVE },
	{ &cg_drawCrosshair, "cg_drawCrosshair", "16", CVAR_ARCHIVE },
	{ &cg_drawCrosshairGauntlet, "cg_drawCrosshairGauntlet", "-1", CVAR_ARCHIVE },
	{ &cg_drawCrosshairMachinegun, "cg_drawCrosshairMachinegun", "-1", CVAR_ARCHIVE },
	{ &cg_drawCrosshairShotgun, "cg_drawCrosshairShotgun", "-1", CVAR_ARCHIVE },
	{ &cg_drawCrosshairGrenadeLauncher, "cg_drawCrosshairGrenadeLauncher", "-1", CVAR_ARCHIVE },
	{ &cg_drawCrosshairRocketLauncher, "cg_drawCrosshairRocketLauncher", "-1", CVAR_ARCHIVE },
	{ &cg_drawCrosshairLightning, "cg_drawCrosshairLightning", "-1", CVAR_ARCHIVE },
	{ &cg_drawCrosshairRailgun, "cg_drawCrosshairRailgun", "-1", CVAR_ARCHIVE },
	{ &cg_drawCrosshairPlasmagun, "cg_drawCrosshairPlasmagun", "-1", CVAR_ARCHIVE },
	{ &cg_drawCrosshairBFG, "cg_drawCrosshairBFG", "-1", CVAR_ARCHIVE },
	{ &cg_crosshairSize, "cg_crosshairSize", "24", CVAR_ARCHIVE },
	{ &cg_crosshairAspectRatioFix, "cg_crosshairAspectRatioFix", "1", CVAR_ARCHIVE },
	{ &cg_crosshairHealth, "cg_crosshairHealth", "0", CVAR_ARCHIVE },
	{ &cg_crosshairX, "cg_crosshairX", "0", CVAR_ARCHIVE },
	{ &cg_crosshairY, "cg_crosshairY", "0", CVAR_ARCHIVE },
	{ &cg_brassTime, "cg_brassTime", "2500", CVAR_ARCHIVE },
	{ &cg_simpleItems, "cg_simpleItems", "0", CVAR_ARCHIVE },
	{ &cg_addMarks, "cg_marks", "1", CVAR_ARCHIVE },
	{ &cg_lagometer, "cg_lagometer", "1", CVAR_ARCHIVE },
	{ &cg_railTrailTime, "cg_railTrailTime", "400", CVAR_ARCHIVE },
	{ &cg_gun_x, "cg_gunX", "0", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_gunX },
	{ &cg_gun_y, "cg_gunY", "0", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_gunY },
	{ &cg_gun_z, "cg_gunZ", "0", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_gunZ },
	{ &cg_centertime, "cg_centertime", "3", CVAR_CHEAT },
	{ &cg_runpitch, "cg_runpitch", "0.000", CVAR_ARCHIVE },
	{ &cg_runroll, "cg_runroll", "0.000", CVAR_ARCHIVE },
	{ &cg_bobup, "cg_bobup", "0.000", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_bobup },
	{ &cg_bobpitch, "cg_bobpitch", "0.000", CVAR_ARCHIVE },
	{ &cg_bobroll, "cg_bobroll", "0.000", CVAR_ARCHIVE },
	{ &cg_swingSpeed, "cg_swingSpeed", "0.3", CVAR_CHEAT },
	{ &cg_animSpeed, "cg_animspeed", "1", CVAR_CHEAT },
	{ &cg_debugAnim, "cg_debuganim", "0", CVAR_CHEAT },
	{ &cg_debugPosition, "cg_debugposition", "0", CVAR_CHEAT },
	{ &cg_debugEvents, "cg_debugevents", "0", CVAR_CHEAT },
	{ &cg_errorDecay, "cg_errordecay", "100",  },
	{ &cg_nopredict, "cg_nopredict", "0",  },
	{ &cg_noPlayerAnims, "cg_noplayeranims", "0", CVAR_CHEAT },
	{ &cg_showmiss, "cg_showmiss", "0",  },
	{ &cg_footsteps, "cg_footsteps", "1", CVAR_ARCHIVE | CVAR_LATCH, CG_LocalEventBeFeaturesChanged },
	{ &cg_tracerChance, "cg_tracerchance", "0.4", CVAR_CHEAT },
	{ &cg_tracerWidth, "cg_tracerwidth", "1", CVAR_CHEAT },
	{ &cg_tracerLength, "cg_tracerlength", "100", CVAR_CHEAT },
	{ &cg_thirdPersonRange, "cg_thirdPersonRange", "40", CVAR_CHEAT },
	{ &cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", CVAR_CHEAT },
	{ &cg_thirdPerson, "cg_thirdPerson", "0",  },
	{ &cg_teamChatTime, "cg_teamChatTime", "3000", CVAR_ARCHIVE },
	{ &cg_teamChatHeight, "cg_teamChatHeight", "0", CVAR_ARCHIVE },
	{ &cg_forceModel, "cg_forceModel", "0", CVAR_ARCHIVE },
	{ &cg_predictItems, "cg_predictItems", "1", CVAR_ARCHIVE },
	{ &cg_deferPlayers, "cg_deferPlayers", "1", CVAR_ARCHIVE },
	{ &cg_drawTeamOverlay, "cg_drawTeamOverlay", "0", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_drawTeamOverlay},
	{ &cg_stats, "cg_stats", "0",  },
	{ &cg_buildScript, "com_buildScript", "0",  },
	{ &cg_paused, "cl_paused", "0", CVAR_ROM },
	{ &com_blood, "com_blood", "1", CVAR_ARCHIVE },
	{ &cg_synchronousClients, "g_synchronousClients", "0",  },
	{ &cg_cameraOrbit, "cg_cameraOrbit", "0", CVAR_CHEAT },
	{ &cg_cameraOrbitDelay, "cg_cameraOrbitDelay", "50", CVAR_ARCHIVE },
	// { &cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1",  },
	// { &cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0",  },
	{ &cg_timescale, "timescale", "1",  },
	{ &cg_scorePlums, "cg_scorePlums", "1", CVAR_ARCHIVE | CVAR_USERINFO },
	{ &cg_smoothClients, "cg_smoothClients", "0", CVAR_ARCHIVE | CVAR_USERINFO },
	{ &cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT },
	{ &pmove_fixed, "pmove_fixed", "0", 0, CG_LocalEventCvarChanged_pmove_fixed},
	{ &pmove_msec, "pmove_msec", "8",  },
	{ &phy_movetype, "phy_movetype", "-1", CVAR_SYSTEMINFO },
	{ &phy_snapVectors, "g_snapVectors", "0", CVAR_SYSTEMINFO },
	{ &cg_noTaunt, "cg_noTaunt", "0", CVAR_ARCHIVE },
	{ &cg_noProjectileTrail, "cg_noProjectileTrail", "0", CVAR_ARCHIVE },
	{ &cg_oldRail, "cg_oldRail", "1", CVAR_ARCHIVE },
	{ &cg_oldRocket, "cg_oldRocket", "1", CVAR_ARCHIVE },
	{ &cg_oldPlasma, "cg_oldPlasma", "1", CVAR_ARCHIVE },
	{ &cg_altGrenades, "cg_altGrenades", "0", CVAR_ARCHIVE, CG_LocalEventBeFeaturesChanged},
	{ &cg_altLightning, "cg_altLightning", "0", CVAR_ARCHIVE},
	{ &cg_ammoCheck, "cg_ammoCheck", "0", CVAR_ARCHIVE },
	{ &cg_autoAction, "cg_autoAction", "0", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_autoAction},
	{ &cg_clientLog, "cg_clientLog", "0", CVAR_ARCHIVE },
	{ &cg_crosshairPulse, "cg_crosshairPulse", "0", CVAR_ARCHIVE },
	{ &cg_customLoc, "cg_customLoc", "0", CVAR_ARCHIVE | CVAR_LATCH },
	{ &cg_damageSound, "cg_damageSound", "0", CVAR_ARCHIVE },
	{ &cg_damageDraw, "cg_damageDraw", "1", CVAR_ARCHIVE },
	{ &cg_damageIndicatorScale, "cg_damageIndicatorScale", "1.0", CVAR_ARCHIVE},
	{ &cg_damageIndicatorOpaque, "cg_damageIndicatorOpaque", "0", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_damageIndicatorOpaque},
	{ &cg_damageKick, "cg_damageKick", "0", CVAR_ARCHIVE },
	{ &cg_deadBodyFilter, "cg_deadBodyFilter", "0", CVAR_ARCHIVE },
	{ &cg_deadBodyInvisible, "cg_deadBodyInvisible", "0", CVAR_ARCHIVE },
	{ &cg_drawDecals, "cg_drawDecals", "1", CVAR_ARCHIVE | CVAR_LATCH },
	{ &cg_drawPing, "cg_drawPing", "0", CVAR_ARCHIVE },
	{ &cg_enableOSPHUD, "cg_enableOSPHUD", "1", CVAR_ARCHIVE },
	{ &cg_shud, "cg_shud", "1", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_shud},
	{ &cg_enableBreath, "cg_enableBreath", "1",  CVAR_ARCHIVE},
	{ &cg_enemyColors, "cg_enemyColors", "0", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_enemyColors},
	{ &cg_enemyModel, "cg_enemyModel", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_enemyModel},
	{ &cg_teamColors, "cg_teamColors", "0", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_teamColors},
	{ &cg_teamModel, "cg_teamModel", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_teamModel},
	{ &cg_execVstr, "cg_execVstr", "", CVAR_ARCHIVE },
	{ &cg_fallKick, "cg_fallKick", "0", CVAR_ARCHIVE },
	{ &cg_followkiller, "cg_followkiller", "0", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_followkiller},
	{ &cg_followpowerup, "cg_followpowerup", "0", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_followpowerup},
	{ &cg_followviewcam, "cg_followviewcam", "1", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_followviewcam},
	{ &cg_forceColors, "cg_forceColors", "0", CVAR_ARCHIVE },
	{ &cg_lightningImpact, "cg_lightningImpact", "0", CVAR_ARCHIVE },
	{ &cg_MaxlocationWidth, "cg_MaxlocationWidth", "16", CVAR_ARCHIVE },
	{ &cg_muzzleFlash, "cg_muzzleFlash", "1", CVAR_ARCHIVE },
	{ &cg_nochatbeep, "cg_noChatBeep", "0", CVAR_ARCHIVE },
	{ &cg_noTeamChatBeep, "cg_noTeamChatBeep", "0", CVAR_ARCHIVE },
	{ &cg_NoMOTDDisplayDuringDemo, "cg_NoMOTDDisplayDuringDemo", "0", CVAR_ARCHIVE },
	{ &cg_nomip, "cg_nomip", "0", CVAR_ARCHIVE },
	{ &cg_oversampleMouse, "cg_oversampleMouse", "0", CVAR_ARCHIVE },
	{ &cg_statScrollTime, "cg_statScrollTime", "0.15", CVAR_ARCHIVE },
	{ &cg_q3compScoreboard, "cg_q3compScoreboard", "1", CVAR_ARCHIVE },
	{ &cg_scoreTransparency, "cg_scoreTransparency", "1.0", CVAR_ARCHIVE },
	{ &cg_showPlayerLean, "cg_showPlayerLean", "1", CVAR_ARCHIVE },
	{ &cg_smoke_sg, "cg_smoke_sg", "1", CVAR_ARCHIVE },
	{ &cg_smokegrowth_gl, "cg_smokegrowth_gl", "1", CVAR_ARCHIVE },
	{ &cg_smokegrowth_rl, "cg_smokegrowth_rl", "1", CVAR_ARCHIVE },
	{ &cg_smokeradius_gl, "cg_smokeradius_gl", "32", CVAR_ARCHIVE },
	{ &cg_smokeradius_rl, "cg_smokeradius_rl", "64", CVAR_ARCHIVE },
	{ &cg_smokedensity_gl, "cg_smokedensity_gl", "50", CVAR_ARCHIVE },
	{ &cg_smokedensity_rl, "cg_smokedensity_rl", "50", CVAR_ARCHIVE },
	{ &cg_swapSkins, "cg_swapSkins", "0", 0, CG_LocalEventCvarChanged_cg_swapSkins},
	{ &cg_teamRails, "cg_teamRails", "0", CVAR_ARCHIVE },
	{ &cg_trueLightning, "cg_trueLightning", "1.0", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_trueLightning},
	{ &cg_useScreenShotJPEG, "cg_useScreenShotJPEG", "1", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_useScreenShotJPEG},
	{ &ch_3waveFont, "ch_3waveFont", "0", CVAR_ARCHIVE },
	{ &ch_ColorLocations, "ch_ColorLocations", "1", CVAR_ARCHIVE },
	{ &ch_CrosshairColor, "ch_crosshairColor", "White", CVAR_ARCHIVE, CG_LocalEventCvarChanged_ch_crosshairColor},
	{ &cf_CrosshairNames, "cf_CrosshairNames", "12x12", CVAR_ARCHIVE },
	{ &ch_CrosshairNamesLeft, "ch_CrosshairNamesLeft", "0", CVAR_ARCHIVE },
	{ &ch_crosshairTeamInfo, "ch_crosshairTeamInfo", "1", CVAR_ARCHIVE },
	{ &ch_drawHealthMeters, "ch_drawHealthMeters", "0", CVAR_ARCHIVE },
	{ &ch_drawFlagNames, "ch_drawFlagNames", "2", CVAR_ARCHIVE },
	{ &ch_drawPickup, "ch_drawPickup", "1", CVAR_ARCHIVE },
	{ &ch_drawSpeed, "ch_drawSpeed", "0", CVAR_ARCHIVE },
	{ &ch_TeamBackground, "ch_TeamBackground", "0", CVAR_ARCHIVE },
	{ &cf_Following, "cf_Following", "24x24", CVAR_ARCHIVE },
	{ &ch_FilterLocationsTeamchat, "ch_FilterLocationsTeamchat", "0", CVAR_ARCHIVE },
	{ &ch_fragMessage, "ch_fragMessage", "1", CVAR_ARCHIVE },
	{ &ch_file, "ch_file", "roamer", CVAR_ARCHIVE, CG_LocalEventCvarChanged_ch_file},
	{ &cf_Fragmsg, "cf_Fragmsg", "16x16", CVAR_ARCHIVE },
	{ &ch_graphs, "ch_graphs", "1", CVAR_ARCHIVE },
	{ &ch_InverseTeamChat, "ch_InverseTeamChat", "1", CVAR_ARCHIVE },
	{ &ch_Obituaries, "ch_Obituaries", "1", CVAR_ARCHIVE },
	{ &ch_Pickupitem, "ch_Pickupitem", "1", CVAR_ARCHIVE },
	{ &cf_Pickupstring, "cf_Pickupstring", "10x14", CVAR_ARCHIVE },
	{ &ch_recordMessage, "ch_recordMessage", "1", CVAR_ARCHIVE, CG_LocalEventCvarChanged_ch_recordMessage},
	{ &cf_Scores, "cf_Scores", "16x16", CVAR_ARCHIVE },
	{ &cf_Statusbar, "cf_Statusbar", "32x32", CVAR_ARCHIVE },
	{ &ch_StatusbarFormat, "ch_StatusbarFormat", "10", CVAR_ARCHIVE },
	{ &ch_statusbarStyle, "ch_statusbarStyle", "1", CVAR_ARCHIVE },
	{ &ch_teamBackground, "ch_teamBackground", "1", CVAR_ARCHIVE },
	{ &cf_Teamchat, "cf_Teamchat", "8x12", CVAR_ARCHIVE },
	{ &ch_TeamchatOnly, "ch_TeamchatOnly", "0", CVAR_ARCHIVE },
	{ &ch_TeamCrosshairHealth, "ch_TeamCrosshairHealth", "1", CVAR_ARCHIVE },
	{ &cf_TeamCrosshairHealth, "cf_TeamCrosshairHealth", "12x12", CVAR_ARCHIVE },
	{ &ch_Teamoverlay, "ch_Teamoverlay", "p12n h/aw l", CVAR_ARCHIVE },
	{ &cf_Teamoverlay, "cf_Teamoverlay", "6x11", CVAR_ARCHIVE },
	{ &cf_Vote, "cf_Vote", "10x14", CVAR_ARCHIVE },
	{ &ch_weaponList, "ch_weaponList", "1", CVAR_ARCHIVE },
	{ &ch_weaponListBackground, "ch_weaponListBackground", "1", CVAR_ARCHIVE },
	{ &ch_weaponListDrawAll, "ch_weaponListDrawAll", "0", CVAR_ARCHIVE },
	{ &ch_weaponListFlash, "ch_weaponListFlash", "1", CVAR_ARCHIVE },
	{ &ch_Weaponswitch, "ch_Weaponswitch", "4", CVAR_ARCHIVE },
	{ &cf_WeaponSwitch, "cf_WeaponSwitch", "32x32", CVAR_ARCHIVE },
	{ &cf_WeaponName, "cf_WeaponName", "12x12", CVAR_ARCHIVE },
	{ &cf_AmmoStatusbar, "cf_AmmoStatusbar", "8x8", CVAR_ARCHIVE },
	{ &cf_timer, "cf_timer", "16x16", CVAR_ARCHIVE },
	{ &s_ambient, "s_ambient", "1", CVAR_ARCHIVE, CG_LocalEventCvarChanged_s_ambient},
	{ &cl_maxpackets, "cl_maxpackets", "30", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cl_maxpackets},
	{ &cl_timenudge, "cl_timenudge", "0", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cl_timenudge},
	{ &snaps, "snaps", "40", CVAR_ARCHIVE, CG_LocalEventCvarChanged_snaps},
	{ &r_shownormals, "r_shownormals", "0", CVAR_LATCH | CVAR_CHEAT, CG_LocalEventCvarChanged_r_shownormals},
	{ &r_fullbright, "r_fullbright", "0", CVAR_LATCH, /* CG_LocalEventCvarChanged_r_fullbright */},
	{ &r_lodCurveError, "r_lodCurveError", "250", CVAR_LATCH, CG_LocalEventCvarChanged_r_lodCurveError},
	{ &r_showtris, "r_showtris", "0", 0, CG_LocalEventCvarChanged_r_showtris},
	{ &r_subdivisions, "r_subdivisions", "4", CVAR_LATCH, CG_LocalEventCvarChanged_r_subdivisions},
	{ &r_znear, "r_znear", "4",  0, CG_LocalEventCvarChanged_r_znear},
	{ &handicap, "handicap", "100", 0, CG_LocalEventCvarChanged_handicap},
	{ &m_pitch, "m_pitch", "0.022", CVAR_ARCHIVE },
	{ &com_maxfps, "com_maxfps", "125", CVAR_ARCHIVE, CG_LocalEventCvarChanged_com_maxfps },
	{ &cg_noLeadSounds, "cg_noLeadSounds", "0", CVAR_ARCHIVE },
	{ &cg_fragSound, "cg_fragSound", "1", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_fragSound},
	{ &cg_lightningHideCrosshair, "cg_lightningHideCrosshair", "0", CVAR_ARCHIVE },
	{ &cg_lightningSilent, "cg_lightningSilent", "0", CVAR_ARCHIVE },
	{ &cg_lightningHide, "cg_lightningHide", "0", CVAR_ARCHIVE },
	{ &cg_delag, "cg_delag", "0", CVAR_ARCHIVE },
	{ &cg_projectileNudge, "cg_projectileNudge", "3", CVAR_ARCHIVE },
	{ &cg_hideScores, "cg_hideScores", "0", CVAR_ARCHIVE },
	{ &cg_deadBodyBlack, "cg_deadBodyBlack", "1", CVAR_ARCHIVE },
	{ &cg_spectGlow, "cg_spectGlow", "0", CVAR_ARCHIVE },
	{ &cg_spectOrigModel, "cg_spectOrigModel", "0", CVAR_ARCHIVE },
	{ &cg_hitSounds, "cg_hitSounds", "0", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_hitSounds },
	{ &cg_playersXID, "cg_playersXID", "0", CVAR_ARCHIVE},
	{ &cg_playersID, "cg_playersID", "0", CVAR_ARCHIVE},

	{ &cg_playerModelColors, "cg_playerModelColors", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_playerModelColors},
	{ &cg_playerRailColors,  "cg_playerRailColors", "",  CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_playerRailColors},
	{ &cg_playerFrozenColor, "cg_playerFrozenColor", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_playerFrozenColor},
	{ &cg_teamModelColors,   "cg_teamModelColors", "",   CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_teamModelColors},
	{ &cg_teamRailColors,    "cg_teamRailColors", "",    CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_teamRailColors},
	{ &cg_teamFrozenColor,   "cg_teamFrozenColor", "",   CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_teamFrozenColor},
	{ &cg_enemyModelColors,  "cg_enemyModelColors", "",  CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_enemyModelColors},
	{ &cg_enemyRailColors,   "cg_enemyRailColors", "",   CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_enemyRailColors},
	{ &cg_enemyFrozenColor,  "cg_enemyFrozenColor", "",  CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_enemyFrozenColor},
	{ &cg_enemyModelColorsUnique, "cg_enemyModelColorsUnique", "1", CVAR_ARCHIVE, NULL},
	{ &cg_teamFrozenFoe,   "cg_teamFrozenFoe", "1",   CVAR_ARCHIVE },

	{ &cg_chatEnable, "cg_chatEnable", "3", CVAR_ARCHIVE},
	{ &cg_shudChatEnable, "cg_shudChatEnable", "3", CVAR_ARCHIVE},
	{ &cg_healthMid,   "cg_healthMid", "200",   CVAR_ARCHIVE},
	{ &cg_healthLow,  "cg_healthLow", "100",  CVAR_ARCHIVE},
	{ &cg_healthColorLevels,  "cg_healthColorLevels", "1",  CVAR_ARCHIVE},

	{ &ch_crosshairDecor,  "ch_crosshairDecor", "0",  CVAR_ARCHIVE},
	{ &ch_crosshairDecorSize,  "ch_crosshairDecorSize", "24",  CVAR_ARCHIVE},
	{ &ch_crosshairDecorColor,  "ch_crosshairDecorColor", "white",  CVAR_ARCHIVE, CG_LocalEventCvarChanged_ch_crosshairDecorColor},
	{ &ch_crosshair45,  "ch_crosshair45", "0",  CVAR_ARCHIVE},
	{ &ch_crosshairDecor45,  "ch_crosshairDecor45", "0",  CVAR_ARCHIVE},
	{ &ch_crosshairDecorOpaque,  "ch_crosshairDecorOpaque", "0",  CVAR_ARCHIVE, CG_LocalEventCvarChanged_ch_crosshairDecorOpaque},
	{ &ch_crosshairOpaque,  "ch_crosshairOpaque", "0",  CVAR_ARCHIVE, CG_LocalEventCvarChanged_ch_crosshairOpaque},

	{ &ch_crosshairAction,  "ch_crosshairAction", "0",  CVAR_ARCHIVE},
	{ &ch_crosshairActionColor,  "ch_crosshairActionColor", "red",  CVAR_ARCHIVE, CG_LocalEventCvarChanged_ch_crosshairActionColor},
	{ &ch_crosshairActionScale,  "ch_crosshairActionScale", "2.0",  CVAR_ARCHIVE, CG_LocalEventCvarChanged_ch_crosshairActionScale },
	{ &ch_crosshairActionTime,  "ch_crosshairActionTime", "400",  CVAR_ARCHIVE, CG_LocalEventCvarChanged_ch_crosshairActionTime},

	{ &ch_crosshairDecorAction,  "ch_crosshairDecorAction", "4",  CVAR_ARCHIVE},
	{ &ch_crosshairDecorActionColor,  "ch_crosshairDecorActionColor", "red",  CVAR_ARCHIVE, CG_LocalEventCvarChanged_ch_crosshairDecorActionColor},
	{ &ch_crosshairDecorActionScale,  "ch_crosshairDecorActionScale", "2.0",  CVAR_ARCHIVE, CG_LocalEventCvarChanged_ch_crosshairDecorActionScale},
	{ &ch_crosshairDecorActionTime,  "ch_crosshairDecorActionTime", "400",  CVAR_ARCHIVE, CG_LocalEventCvarChanged_ch_crosshairDecorActionTime},

	{ &ch_crosshairAutoScale,  "ch_crosshairAutoScale", "1",  CVAR_ARCHIVE},

	{ &cg_dlightGauntlet, "cg_dlightGauntlet", "9999FF", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_dlightGauntlet },
	{ &cg_dlightMG,       "cg_dlightMG",       "FFFF00", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_dlightMG },
	{ &cg_dlightSG,       "cg_dlightSG",       "FFFF00", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_dlightSG },
	{ &cg_dlightGL,       "cg_dlightGL",       "FFBF00", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_dlightGL },
	{ &cg_dlightRL,       "cg_dlightRL",       "FFBF00", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_dlightRL },
	{ &cg_dlightLG,       "cg_dlightLG",       "9999FF", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_dlightLG },
	{ &cg_dlightRG,       "cg_dlightRG",       "FF8000", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_dlightRG },
	{ &cg_dlightPG,       "cg_dlightPG",       "9999FF", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_dlightPG },
	{ &cg_dlightBFG,      "cg_dlightBFG",      "FFB2FF", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_dlightBFG },
	{ &cg_gunColor,       "cg_gunColor",      "White", CVAR_ARCHIVE },
	{ &cg_gunOpaque,      "cg_gunOpaque",      "0.15", CVAR_ARCHIVE },
	{ &cg_drawBrightWeapons, "cg_drawBrightWeapons", "0", CVAR_ARCHIVE, CG_LocalEventBeFeaturesChanged },
	{ &cg_conObituaries,  "cg_conObituaries",   "1", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_conObituaries },
	{ &cg_lightningHitsoundRateFix, "cg_lightningHitsoundRateFix",      "1", CVAR_ARCHIVE },
	{ &cg_stackHitSounds,           "cg_stackHitSounds",   "1", CVAR_ARCHIVE },
	{ &cg_stackHitSoundsTimeout,    "cg_stackHitSoundsTimeout",   "500", CVAR_ARCHIVE },
	{ &cg_drawCenterMessages, "cg_drawCenterMessages", "1", CVAR_ARCHIVE },
	{ &cg_predictStepOffset, "cg_predictStepOffset", "1", CVAR_ARCHIVE },
	{ &cg_uniqueColorTable, "cg_uniqueColorTable", "1", CVAR_ARCHIVE },
	{ &cg_noVoteBeep, "cg_noVoteBeep", "0", CVAR_ARCHIVE },
	{ &cg_damageDrawFrame, "cg_damageDrawFrame", "2", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_damageDrawFrame },
	{ &cg_damageFrameSize, "cg_damageFrameSize", "2", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_damageFrameSize },
	{ &cg_damageFrameOpaque, "cg_damageFrameOpaque", "0.65", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_damageFrameOpaque },
	{ &cg_shud_currentWeapons, "cg_shud_currentWeapons", "226",  CVAR_ARCHIVE },
	{ &cg_drawHitBox, "cg_drawHitBox", "0", CVAR_ARCHIVE, CG_LocalEventBeFeaturesChanged },
	{ &cg_hitBoxColor, "cg_hitBoxColor", "004444", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_hitBoxColor },
	{ &cg_drawGunForceAspect, "cg_drawGunForceAspect", "0", CVAR_ARCHIVE },
	{ &cg_drawOutline, "cg_drawOutline", "3", CVAR_ARCHIVE, CG_LocalEventBeFeaturesChanged },
	{ &cg_enemyOutlineSize, "cg_enemyOutlineSize", "1", CVAR_ARCHIVE | CVAR_LATCH },
	{ &cg_enemyOutlineColor, "cg_enemyOutlineColor", "00FF00", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_enemyOutlineColor },
	{ &cg_enemyOutlineColorUnique, "cg_enemyOutlineColorUnique", "0", CVAR_ARCHIVE, CG_LocalEventBeFeaturesChanged },
	{ &cg_teamOutlineSize, "cg_teamOutlineSize", "1", CVAR_ARCHIVE | CVAR_LATCH },
	{ &cg_teamOutlineColor, "cg_teamOutlineColor", "FFFFFF", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_teamOutlineColor },
	{ &cg_underwaterFovWarp, "cg_underwaterFovWarp", "1", CVAR_ARCHIVE },
	{ &cg_altBlood, "cg_altBlood", "0", CVAR_ARCHIVE, CG_LocalEventBeFeaturesChanged },
	{ &cg_altBloodColor, "cg_altBloodColor", "White", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_altBloodColor },
	{ &cg_altPlasma, "cg_altPlasma", "0", CVAR_ARCHIVE },
	{ &cg_drawRewards, "cg_drawRewards", "1", CVAR_ARCHIVE },
	{ &cg_shotGunTracer, "cg_shotGunTracer", "1", CVAR_ARCHIVE },
	{ &cg_railRingsRadius, "cg_railRingsRadius", "4.0", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_railRingsRadius },
	{ &cg_railRingsRotation, "cg_railRingsRotation", "1", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_railRingsRotation },
	{ &cg_railRingsSpacing, "cg_railRingsSpacing", "5", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_railRingsSpacing },
	{ &cg_railRingsSize, "cg_railRingsSize", "1.1", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_railRingsSize },
	{ &cg_railStaticRings, "cg_railStaticRings", "0", CVAR_ARCHIVE },
	{ &cg_gunPos, "cg_gunPos", "1", CVAR_ARCHIVE  },
	{ &cg_shadows, "cg_shadows", "1", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_shadows},
	{ &cg_altShadow, "cg_altShadow", "0", CVAR_ARCHIVE, CG_LocalEventBeFeaturesChanged },
	{ &cg_altShadowColor, "cg_altShadowColor", "White", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_altShadowColor },
	{ &cg_scoreboardShowId, "cg_scoreboardShowId", "0", CVAR_ARCHIVE },
	{ &cg_drawFriend, "cg_drawFriend", "1", CVAR_ARCHIVE },
	{ &cg_teamIndicator, "cg_teamIndicator", "10", CVAR_ARCHIVE, CG_LocalEventBeFeaturesChanged },
	{ &cg_teamIndicatorAdjust, "cg_teamIndicatorAdjust", "0.9", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_teamIndicatorAdjust },
	{ &cg_teamIndicatorColor, "cg_teamIndicatorColor", "White", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_teamIndicatorColor },
	{ &cg_teamIndicatorOpaque, "cg_teamIndicatorOpaque", "1", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_teamIndicatorOpaque },
	{ &cg_teamIndicatorBgColor, "cg_teamIndicatorBgColor", "000000", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_teamIndicatorBgColor },
	{ &cg_teamIndicatorBgOpaque, "cg_teamIndicatorBgOpaque", "0.2", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_teamIndicatorBgOpaque },
	{ &cg_teamIndicatorOffset, "cg_teamIndicatorOffset", "12", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_teamIndicatorOffset },
	{ &cg_teamIndicatorMaxLength, "cg_teamIndicatorMaxLength", "13", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_teamIndicatorMaxLength },
	{ &cg_teamIndicatorFont, "cg_teamIndicatorFont", "2", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_teamIndicatorFont, },
	{ &cg_scoreboardBE, "cg_scoreboardBE", "0", CVAR_ARCHIVE, },
	{ &cg_scoreboardFont, "cg_scoreboardFont", "2", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_scoreboardFont },
	{ &cg_centerMessagesFont, "cg_centerMessagesFont", "0", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_centerMessagesFont },
	{ &cg_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE },
	{ &cg_railCustomChamber, "cg_railCustomChamber", "1", CVAR_ARCHIVE },
	{ &cg_altGrenadesColor, "cg_altGrenadesColor", "7", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_altGrenadesColor },
	{ &cg_enemyGrenadesColor, "cg_enemyGrenadesColor", "2", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_enemyGrenadesColor },
	{ &cg_itemFx, "cg_itemFx", "7", CVAR_ARCHIVE },
	{ &cg_bubbleTrail, "cg_bubbleTrail", "1", CVAR_ARCHIVE },
	{ &cg_gibs, "cg_gibs", "2", CVAR_ARCHIVE },
	{ &cg_ignoreServerMessages, "cg_ignoreServerMessages", "0", CVAR_ARCHIVE },
	{ &cg_drawAmmoWarning, "cg_drawAmmoWarning", "1", CVAR_ARCHIVE },
	{ &cg_healthColor, "cg_healthColor", "Yellow", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_healthColor },
	{ &cg_healthLowColor, "cg_healthLowColor", "Red", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_healthLowColor },
	{ &cg_healthMidColor, "cg_healthMidColor", "White", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_healthMidColor },
	{ &cg_redTeamColor, "cg_redTeamColor", "Red", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_redTeamColor },
	{ &cg_blueTeamColor, "cg_blueTeamColor", "Blue", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_blueTeamColor },
	{ &cg_friendHudMarkerMaxDist, "cg_friendHudMarkerMaxDist", "0", CVAR_ARCHIVE },
	{ &cg_friendHudMarkerSize, "cg_friendHudMarkerSize", "1", CVAR_ARCHIVE },
	{ &cg_friendHudMarkerMaxScale, "cg_friendHudMarkerMaxScale", "1", CVAR_ARCHIVE },
	{ &cg_friendHudMarkerMinScale, "cg_friendHudMarkerMinScale", "0.3", CVAR_ARCHIVE },
	{ &cg_friendsWallhack, "cg_friendsWallhack", "7", CVAR_ARCHIVE, CG_LocalEventBeFeaturesChanged },
	{ &cg_drawHudMarkers, "cg_drawHudMarkers", "1", CVAR_ARCHIVE },
	{ &cg_drawAccuracy, "cg_drawAccuracy", "4", CVAR_ARCHIVE },
	{ &cg_accuracyFontSize, "cg_accuracyFontSize", "10 14", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_accuracyFontSize },
	{ &cg_accuracyIconSize, "cg_accuracyIconSize", "18", CVAR_ARCHIVE },
	{ &cg_accuracyFont, "cg_accuracyFont", "2", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_accuracyFont, },
	{ &ch_crosshairActionColorLow,  "ch_crosshairActionColorLow", "white",  CVAR_ARCHIVE, CG_LocalEventCvarChanged_ch_crosshairActionColor },
	{ &ch_crosshairActionColorMid,  "ch_crosshairActionColorMid", "yellow",  CVAR_ARCHIVE, CG_LocalEventCvarChanged_ch_crosshairActionColor },
	{ &ch_crosshairActionColorHigh,  "ch_crosshairActionColorHigh", "orange",  CVAR_ARCHIVE, CG_LocalEventCvarChanged_ch_crosshairActionColor },
	{ &ch_crosshairDecorActionColorLow,  "ch_crosshairDecorActionColorLow", "white",  CVAR_ARCHIVE, CG_LocalEventCvarChanged_ch_crosshairDecorActionColor },
	{ &ch_crosshairDecorActionColorMid,  "ch_crosshairDecorActionColorMid", "yellow",  CVAR_ARCHIVE, CG_LocalEventCvarChanged_ch_crosshairDecorActionColor },
	{ &ch_crosshairDecorActionColorHigh,  "ch_crosshairDecorActionColorHigh", "orange",  CVAR_ARCHIVE, CG_LocalEventCvarChanged_ch_crosshairDecorActionColor },
	{ &cg_markTeam, "cg_markTeam", "-1", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_markTeam },
	{ &cg_markTeamColor, "cg_markTeamColor", "Pink", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_markTeamColor  },
	{ &cg_mySound, "cg_mySound", "", CVAR_ARCHIVE | CVAR_LATCH, CG_LocalEventCvarChanged_cg_customSound },
	{ &cg_teamSound, "cg_teamSound", "", CVAR_ARCHIVE | CVAR_LATCH, CG_LocalEventCvarChanged_cg_customSound },
	{ &cg_enemySound, "cg_enemySound", "", CVAR_ARCHIVE | CVAR_LATCH, CG_LocalEventCvarChanged_cg_customSound },
	{ &cg_scoreboardRtColors, "cg_scoreboardRtColors", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_scoreboardRtColors },
	{ &cg_scoreboardBtColors, "cg_scoreboardBtColors", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_scoreboardBtColors },
	{ &cg_scoreboardSpecColor, "cg_scoreboardSpecColor", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_scoreboardSpecColor },
	{ &cg_scoreboardDrawPowerUps, "cg_scoreboardDrawPowerUps", "1", CVAR_ARCHIVE },
	{ &cg_bestats_style, "cg_bestats_style", "1", CVAR_ARCHIVE },
	{ &cg_bestats_textSize, "cg_bestats_textSize", "6 8", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_bestats_textSize },
	{ &cg_bestats_font, "cg_bestats_font", "2", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_bestats_font },
	{ &cg_bestats_pos, "cg_bestats_pos", "4 320", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_bestats_pos },
	{ &cg_bestats_bgColor, "cg_bestats_bgColor", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_bestats_bgColor },
	{ &cg_bestats_bgOpaque, "cg_bestats_bgOpaque", "0.7", CVAR_ARCHIVE },
	{ &cg_bestats_spacingAdjust, "cg_bestats_spacingAdjust", "1", CVAR_ARCHIVE },
	{ &cg_bestats_widthCutoff, "cg_bestats_widthCutoff", "0", CVAR_ARCHIVE },
	{ &cg_teamIndicatorFade, "cg_teamIndicatorFade", "1", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_teamIndicatorFade },
	{ &cg_teamIndicatorFadeRadius, "cg_teamIndicatorFadeRadius", "256", CVAR_ARCHIVE, },
	{ &be_features, "be_features", "", CVAR_ROM | CVAR_USERINFO },
	{ &compilation_build, "compilation_build", "", CVAR_ROM | CVAR_USERINFO },
	{ &be_enabled, "be_enabled", "1", CVAR_ARCHIVE /* | CVAR_USERINFO */, CG_LocalEventBeFeaturesChanged },
	{ &cg_clearOnLevelLoad, "cg_clearOnLevelLoad", "0", CVAR_ARCHIVE },
	{ &cg_spectPOV, "cg_spectPOV", "0", CVAR_ARCHIVE, CG_LocalEventBeFeaturesChanged },
	{ &cg_drawRealHeads, "cg_drawRealHeads", "0", CVAR_ARCHIVE },
	{ &cg_bestats_autoaction, "cg_bestats_autoaction", "1", CVAR_ARCHIVE },
	{ &cg_zoomAutoReset, "cg_zoomAutoReset", "3", CVAR_ARCHIVE },
	{ &cg_smoothDucking, "cg_smoothDucking", "1", CVAR_ARCHIVE },
	{ &cg_respawnWeapon, "cg_respawnWeapon", "0", CVAR_ARCHIVE },
	{ &cg_countdownTimer, "cg_countdownTimer", "0", CVAR_ARCHIVE },
	{ &cg_exec_weapons, "cg_exec_weapons", "1", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_exec_weapon },
	{ &cg_exec_mg, "cg_exec_mg", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_exec_weapon },
	{ &cg_exec_sg, "cg_exec_sg", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_exec_weapon },
	{ &cg_exec_gl, "cg_exec_gl", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_exec_weapon },
	{ &cg_exec_rl, "cg_exec_rl", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_exec_weapon },
	{ &cg_exec_lg, "cg_exec_lg", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_exec_weapon },
	{ &cg_exec_rg, "cg_exec_rg", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_exec_weapon },
	{ &cg_exec_pg, "cg_exec_pg", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_exec_weapon },
	{ &cg_exec_gametypes, "cg_exec_gametypes", "1", CVAR_ARCHIVE },
	{ &cg_exec_dm, "cg_exec_dm", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_exec_gametype },
	{ &cg_exec_tourney, "cg_exec_tourney", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_exec_gametype },
	{ &cg_exec_tdm, "cg_exec_tdm", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_exec_gametype },
	{ &cg_exec_ca, "cg_exec_ca", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_exec_gametype },
	{ &cg_exec_ctf, "cg_exec_ctf", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_exec_gametype },
	{ &cg_exec_freeze, "cg_exec_freeze", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_exec_gametype },
	{ &cg_damagePlums, "cg_damagePlums", "1", CVAR_ARCHIVE | CVAR_USERINFO },
	{ &cg_damagePlumSize, "cg_damagePlumSize", "8.0", CVAR_ARCHIVE },
	{ &cg_damagePlumsWeapons, "cg_damagePlumsWeapons", "511", CVAR_ARCHIVE | CVAR_USERINFO },
	/* { &cg_autoHop, "cg_autoHop", "1", CVAR_ARCHIVE | CVAR_USERINFO, CG_LocalEventCvarChanged_cg_autoHop}, */  /* autohop commented out */
	{ &cg_chatColor, "cg_chatColor", "2", CVAR_ARCHIVE },
	{ &cg_teamChatColor, "cg_teamChatColor", "5", CVAR_ARCHIVE },
	{ &cg_teamTaskMarkTime, "cg_teamTaskMarkTime", "5000", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_teamTaskMarkTime },
	{ &cg_teamTask, "cg_teamTask", "31", CVAR_ARCHIVE, CG_LocalEventBeFeaturesChanged },
	{ &cg_teamTaskScale, "cg_teamTaskScale", "1.0", CVAR_ARCHIVE },
	{ &cg_teamTaskOpaque, "cg_teamTaskOpaque", "0.7", CVAR_ARCHIVE },
	{ &cg_teamTaskFade, "cg_teamTaskFade", "0.5", CVAR_ARCHIVE },
	{ &cg_teamTaskFadeRadius, "cg_teamTaskFadeRadius", "128", CVAR_ARCHIVE },
	{ &cg_teamTaskFont, "cg_teamTaskFont", "2", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_teamTaskFont, },
	{ &cg_teamTaskWheel, "cg_teamTaskWheel", "12", CVAR_ARCHIVE, CG_LocalEventCvarChanged_ReloadTeamTaskWheel },
	{ &cg_teamTaskWheelSet, "cg_teamTaskWheelSet", "defence here medic smart yes no warn pickup", CVAR_ARCHIVE, CG_LocalEventCvarChanged_ReloadTeamTaskWheel },
	{ &cg_teamTaskWheelSensitivity, "cg_teamTaskWheelSensitivity", "3.0", CVAR_ARCHIVE },
	{ &cg_teamTaskWheelScale, "cg_teamTaskWheelScale", "1.0", CVAR_ARCHIVE },
	{ &cg_storedSensitivity, "cg_storedSensitivity", "1.0", CVAR_NOTABCOMPLETE | CVAR_ARCHIVE | CVAR_INIT },
	{ &cg_teamTaskThemeColor, "cg_teamTaskThemeColor", "Red", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_teamTaskThemeColor },
	{ &cg_teamTaskSmartDefault, "cg_teamTaskSmartDefault", "attack", CVAR_ARCHIVE, CG_LocalEventCvarChanged_ReloadTeamTaskWheel }, /* Default: location */
	{ &cg_teamTaskHudMarkerMaxDist, "cg_teamTaskHudMarkerMaxDist", "0", CVAR_ARCHIVE },
	{ &cg_teamTaskHudMarkerSize, "cg_teamTaskHudMarkerSize", "1", CVAR_ARCHIVE },
	{ &cg_teamTaskHudMarkerMaxScale, "cg_teamTaskHudMarkerMaxScale", "1", CVAR_ARCHIVE },
	{ &cg_teamTaskHudMarkerMinScale, "cg_teamTaskHudMarkerMinScale", "0.3", CVAR_ARCHIVE },
	{ &cg_printui, "cg_printui", "1", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_printui },
	{ &cg_printui_template, "cg_printui_template", "640 20 18x24 hR font 2 time 2000", CVAR_ARCHIVE },
	{ &cg_printui_template2, "cg_printui_template2", "320 100 22x30 hC font 4 time 2000", CVAR_ARCHIVE },
	// { &cg_snow, "cg_snow", "1", CVAR_ARCHIVE | CVAR_LATCH },
	{ &cg_teamIndicatorFontSize, "cg_teamIndicatorFontSize", "8 10", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_teamIndicatorFontSize },
	{ &cg_exec_intermission, "cg_exec_intermission", "", CVAR_ARCHIVE },
	{ &cg_exec_warmupEnded, "cg_exec_warmupEnded", "", CVAR_ARCHIVE },
	{ &cg_exec_joinGame, "cg_exec_joinGame", "", CVAR_ARCHIVE },
	{ &cg_exec_joinSpec, "cg_exec_joinSpec", "", CVAR_ARCHIVE },
	{ &cg_exec_gauntlet, "cg_exec_gauntlet", "", CVAR_ARCHIVE },
	{ &cg_exec_bfg, "cg_exec_bfg", "", CVAR_ARCHIVE },
	{ &cg_exec_shud, "cg_exec_shud", "1", CVAR_ARCHIVE },
	{ &cg_simpleItemsRadius, "cg_simpleItemsRadius", "14", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_simpleItemsRadius },
	{ &cg_simpleItemsBob, "cg_simpleItemsBob", "0", CVAR_ARCHIVE },
	{ &cg_brightWeaponsOpaque, "cg_brightWeaponsOpaque", "1.0", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_brightWeaponsOpaque },
	{ &uc_sensitivity, "uc_sensitivity", "5.0", CVAR_ARCHIVE | CVAR_NOTABCOMPLETE },
	{ &cg_teamTaskIconMinSize, "cg_teamTaskIconMinSize", "3", CVAR_ARCHIVE },
	{ &cg_hitSoundsQC, "cg_hitSoundsQC", "0", CVAR_ARCHIVE | CVAR_LATCH },
	{ &cg_brightWeaponsShader, "cg_brightWeaponsShader", "1", CVAR_ARCHIVE },
	{ &cg_noExplosions, "cg_noExplosions", "0", CVAR_ARCHIVE, CG_LocalEventBeFeaturesChanged },
	{ &cg_noItemUseSound, "cg_noItemUseSound", "0", CVAR_ARCHIVE },
	{ &cg_friendsWallhackFrozen, "cg_friendsWallhackFrozen", "1", CVAR_ARCHIVE, CG_LocalEventBeFeaturesChanged },
	{ &cg_teleportEffect, "cg_teleportEffect", "1", CVAR_ARCHIVE },
	{ &cg_infoVersionFont, "cg_infoVersionFont", "2", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_infoVersionFont },
	{ &cl_reconnectArgs, "cl_reconnectArgs", "", CVAR_ARCHIVE_ND | CVAR_NOTABCOMPLETE | CVAR_NORESTART, },
	{ &cg_altRail, "cg_altRail", "0", CVAR_ARCHIVE | CVAR_UPDATED },
	{ &cg_altRailBeefy, "cg_altRailBeefy", "0", CVAR_ARCHIVE },
	{ &cg_teamTaskNewSounds, "cg_teamTaskNewSounds", "1", CVAR_ARCHIVE },
	// { &cg_teamTaskVersion, "cg_teamTaskVersion", "0", CVAR_ARCHIVE | CVAR_NEW },
	{ &cg_railFix, "cg_railFix", "0", CVAR_ARCHIVE },
	{ &cg_freezeSoundFix, "cg_freezeSoundFix", "", CVAR_ARCHIVE },
	{ &cl_pitchspeed, "cl_pitchspeed", "140", CVAR_ARCHIVE_ND, CG_LocalEventCvarChanged_cl_pitchspeed },
	{ &cl_yawspeed, "cl_yawspeed", "140", CVAR_ARCHIVE_ND, CG_LocalEventCvarChanged_cl_yawSpeed },
	{ &cg_infoDetail, "cg_infoDetail", "1", CVAR_ARCHIVE },
	{ &cg_enhancedInterpolation, "cg_enhancedInterpolation", "0", CVAR_ARCHIVE },
	{ &cg_entityOverpolate, "cg_entityOverpolate", "0", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_entityOverpolate },
	{ &cg_healthBarStyle, "cg_healthBarStyle", "10", CVAR_ARCHIVE },
	{ &cg_teamHealthBar, "cg_teamHealthBar", "0", CVAR_ARCHIVE },
	{ &cg_emojiListFont, "cg_emojiListFont", "2", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_emojiListFont },
	{ &cg_emojiListBgColor, "cg_emojiListBgColor", "00000A", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_emojiListBgColor },
	{ &cg_emojiListOpaque, "cg_emojiListOpaque", "0.8", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_emojiListOpaque },
	{ &cg_battleSuitColor, "cg_battleSuitColor", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_battleSuitColor },
	{ &cg_enemyBattleSuitColor, "cg_enemyBattleSuitColor", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_enemyBattleSuitColor },
	{ &cg_mutePowerUpCountdown, "cg_mutePowerUpCountdown", "0", CVAR_ARCHIVE },
	{ &cg_myTeam, "cg_myTeam", "0", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_myTeam },
	{ &cg_roundSound, "cg_roundSound", "2", CVAR_ARCHIVE },
	{ &cg_debugServer, "cg_debugServer", "0", CVAR_ARCHIVE | CVAR_NOTABCOMPLETE },
	{ &cg_quadDamageColor, "cg_quadDamageColor", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_quadDamageColor },
	{ &cg_enemyQuadDamageColor, "cg_enemyQuadDamageColor", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_enemyQuadDamageColor  },
	{ &cg_floatSpriteMaxDist, "cg_floatSpriteMaxDist", "0", CVAR_ARCHIVE },
	{ &cg_floatSpriteMinSize, "cg_floatSpriteMinSize", "1", CVAR_ARCHIVE },
	{ &cg_altLightningColor, "cg_altLightningColor", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_enemyLightningColor },
	{ &cg_teamLightningColor, "cg_teamLightningColor", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_enemyLightningColor },
	{ &cg_enemyLightningColor, "cg_enemyLightningColor", "", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_enemyLightningColor },
	{ &cg_enemyAltLightning, "cg_enemyAltLightning", "", CVAR_ARCHIVE },
	{ &cg_teamAltLightning, "cg_teamAltLightning", "", CVAR_ARCHIVE },
	{ &cg_muteMyJump, "cg_muteMyJump", "0", CVAR_ARCHIVE },
	{ &cg_muteWorldSounds, "cg_muteWorldSounds", "0", CVAR_ARCHIVE },
	{ &cg_disableTextEffects, "cg_disableTextEffects", "0", CVAR_ARCHIVE },
	{ &cg_fragEffectTime, "cg_fragEffectTime", "500", CVAR_ARCHIVE },
	{ &cg_fragEffectColor, "cg_fragEffectColor", "White", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_fragEffectColor },
	{ &cg_fragEffect, "cg_fragEffect", "1", CVAR_ARCHIVE },
	{ &cg_drawUniqueColors, "cg_drawUniqueColors", "0", CVAR_ARCHIVE },
	{ &cg_killerGhost, "cg_killerGhost", "1", CVAR_ARCHIVE },
	{ &cg_numbersSet, "cg_numbersSet", "1", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_numbersSet },
	{ &cg_oldFlags, "cg_oldFlags", "0", CVAR_ARCHIVE, CG_LocalEventCvarChanged_cg_oldFlags },
	{ &cg_drawBrightItems, "cg_drawBrightItems", "0", CVAR_ARCHIVE, CG_LocalEventBeFeaturesChanged },
	{ &cg_brightItemsOpaque, "cg_brightItemsOpaque", "0.8", CVAR_ARCHIVE },
	{ &cg_myFootsteps, "cg_myFootsteps", "-1", CVAR_ARCHIVE | CVAR_NEW },
	{ &cg_teamFootsteps, "cg_teamFootsteps", "-1", CVAR_ARCHIVE | CVAR_NEW },
	{ &cg_enemyFootsteps, "cg_enemyFootsteps", "-1", CVAR_ARCHIVE | CVAR_NEW },
	{ &cg_oldMedals, "cg_oldMedals", "0", CVAR_ARCHIVE | CVAR_NEW },
	{ &cg_damagePlumsTimeout, "cg_damagePlumsTimeout", "750", CVAR_ARCHIVE | CVAR_NEW },
	{ &cg_freezeShader, "cg_freezeShader", "1", CVAR_ARCHIVE | CVAR_NEW },
	{ &cg_plasmaColor, "cg_plasmaColor", "", CVAR_ARCHIVE | CVAR_NEW, CG_LocalEventCvarChanged_cg_plasmaColor },
	{ &cg_enemyPlasmaColor, "cg_enemyPlasmaColor", "", CVAR_ARCHIVE | CVAR_NEW, CG_LocalEventCvarChanged_cg_enemyPlasmaColor },
	{ &cg_shudStyle, "cg_shudStyle", "1", CVAR_ARCHIVE | CVAR_NEW, CG_LocalEventCvarChanged_cg_shudStyle },
	{ &cg_chatWindow, "cg_chatWindow", "0", CVAR_ARCHIVE | CVAR_NEW },
	{ &cg_chatWindowMessageTime, "cg_chatWindowMessageTime", "3000", CVAR_ARCHIVE | CVAR_NEW },
	{ &cg_chatWindowPos, "cg_chatWindowPos", "140 300", CVAR_ARCHIVE | CVAR_NEW },
	{ &cg_chatWindowSize, "cg_chatWindowSize", "360 120", CVAR_ARCHIVE | CVAR_NEW },
	{ &cg_chatWindowFont, "cg_chatWindowFont", "2", CVAR_ARCHIVE | CVAR_NEW, CG_LocalEventCvarChanged_cg_chatWindowFont },
	{ &cg_chatWindowFontSize, "cg_chatWindowFontSize", "6 10", CVAR_ARCHIVE | CVAR_NEW, CG_LocalEventCvarChanged_cg_chatWindowFontSize },
	{ &cg_chatWindowBgColor, "cg_chatWindowBgColor", "000000", CVAR_ARCHIVE | CVAR_NEW, CG_LocalEventCvarChanged_cg_chatWindowBgColor },
	{ &cg_chatWindowBgOpaque, "cg_chatWindowBgOpaque", "0.25", CVAR_ARCHIVE | CVAR_NEW, CG_LocalEventCvarChanged_cg_chatWindowBgOpaque },
	{ &cg_drawSpawns, "cg_drawSpawns", "0", CVAR_ARCHIVE | CVAR_NEW },
	{ &cg_drawTPexit, "cg_drawTPexit", "0", CVAR_ARCHIVE | CVAR_NEW },
	{ &cg_shadowItems, "cg_shadowItems", "63", CVAR_ARCHIVE | CVAR_NEW },
	{ &cg_worldFlags, "cg_worldFlags", "0", CVAR_ARCHIVE | CVAR_NEW },
	// { &be_run, "be_run", "0", CVAR_ARCHIVE },

};

// temp cvars
vmCvar_t           cg_shud_chatindex;
vmCvar_t           cg_shud_chatmsg0;
vmCvar_t           cg_shud_chatmsg1;
vmCvar_t           cg_shud_chatmsg2;
vmCvar_t           cg_shud_chatmsg3;
vmCvar_t           cg_shud_chatmsg4;
vmCvar_t           cg_shud_chatmsg5;
vmCvar_t           cg_shud_chatmsg6;
vmCvar_t           cg_shud_chatmsg7;
vmCvar_t           cg_shud_chatmsg8;
vmCvar_t           cg_shud_chatmsg9;
vmCvar_t           cg_shud_chatmsg10;
vmCvar_t           cg_shud_chatmsg11;
vmCvar_t           cg_shud_chatmsg12;
vmCvar_t           cg_shud_chatmsg13;
vmCvar_t           cg_shud_chatmsg14;
vmCvar_t           cg_shud_chatmsg15;
vmCvar_t           cg_bechat_hist0;
vmCvar_t           cg_bechat_hist1;
vmCvar_t           cg_bechat_hist2;
vmCvar_t           cg_bechat_hist3;
vmCvar_t           cg_bechat_hist4;
vmCvar_t           cg_bechat_hist5;
vmCvar_t           cg_bechat_hist6;
vmCvar_t           cg_bechat_hist7;
vmCvar_t           cg_bechat_hist8;
vmCvar_t           cg_bechat_hist9;
vmCvar_t           cg_bechat_hist10;
vmCvar_t           cg_bechat_hist11;
vmCvar_t           cg_bechat_hist12;
vmCvar_t           cg_bechat_hist13;
vmCvar_t           cg_bechat_hist14;
vmCvar_t           cg_bechat_hist15;
vmCvar_t           cg_bechat_sent0;
vmCvar_t           cg_bechat_sent1;
vmCvar_t           cg_bechat_sent2;
vmCvar_t           cg_bechat_sent3;
vmCvar_t           cg_bechat_sent4;
vmCvar_t           cg_bechat_sent5;
vmCvar_t           cg_bechat_sent6;
vmCvar_t           cg_bechat_sent7;
vmCvar_t           cg_bechat_sent8;
vmCvar_t           cg_bechat_sent9;
vmCvar_t           cg_bechat_sent10;
vmCvar_t           cg_bechat_sent11;
vmCvar_t           cg_bechat_sent12;
vmCvar_t           cg_bechat_sent13;
vmCvar_t           cg_bechat_sent14;
vmCvar_t           cg_bechat_sent15;
vmCvar_t           cg_beLastTellSender;

static cvarTable_t cvarTable_temp[] =
{
	{ &cg_shud_chatindex, "cg_shud_chatindex", "0",   CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_shud_chatmsg0, "cg_shud_chatmsg0", "",   CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_shud_chatmsg1, "cg_shud_chatmsg1", "",   CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_shud_chatmsg2, "cg_shud_chatmsg2", "",   CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_shud_chatmsg3, "cg_shud_chatmsg3", "",   CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_shud_chatmsg4, "cg_shud_chatmsg4", "",   CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_shud_chatmsg5, "cg_shud_chatmsg5", "",   CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_shud_chatmsg6, "cg_shud_chatmsg6", "",   CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_shud_chatmsg7, "cg_shud_chatmsg7", "",   CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_shud_chatmsg8, "cg_shud_chatmsg8", "",   CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_shud_chatmsg9, "cg_shud_chatmsg9", "",   CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_shud_chatmsg10, "cg_shud_chatmsg10", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_shud_chatmsg11, "cg_shud_chatmsg11", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_shud_chatmsg12, "cg_shud_chatmsg12", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_shud_chatmsg13, "cg_shud_chatmsg13", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_shud_chatmsg14, "cg_shud_chatmsg14", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_shud_chatmsg15, "cg_shud_chatmsg15", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_hist0, "cg_bechat_hist0", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_hist1, "cg_bechat_hist1", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_hist2, "cg_bechat_hist2", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_hist3, "cg_bechat_hist3", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_hist4, "cg_bechat_hist4", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_hist5, "cg_bechat_hist5", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_hist6, "cg_bechat_hist6", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_hist7, "cg_bechat_hist7", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_hist8, "cg_bechat_hist8", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_hist9, "cg_bechat_hist9", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_hist10, "cg_bechat_hist10", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_hist11, "cg_bechat_hist11", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_hist12, "cg_bechat_hist12", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_hist13, "cg_bechat_hist13", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_hist14, "cg_bechat_hist14", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_hist15, "cg_bechat_hist15", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_sent0, "cg_bechat_sent0", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_sent1, "cg_bechat_sent1", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_sent2, "cg_bechat_sent2", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_sent3, "cg_bechat_sent3", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_sent4, "cg_bechat_sent4", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_sent5, "cg_bechat_sent5", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_sent6, "cg_bechat_sent6", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_sent7, "cg_bechat_sent7", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_sent8, "cg_bechat_sent8", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_sent9, "cg_bechat_sent9", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_sent10, "cg_bechat_sent10", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_sent11, "cg_bechat_sent11", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_sent12, "cg_bechat_sent12", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_sent13, "cg_bechat_sent13", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_sent14, "cg_bechat_sent14", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_bechat_sent15, "cg_bechat_sent15", "", CVAR_INIT | CVAR_NORESTART | CVAR_TEMP | CVAR_NOTABCOMPLETE },
	{ &cg_beLastTellSender, "cg_beLastTellSender", "", CVAR_TEMP | CVAR_NORESTART | CVAR_NOTABCOMPLETE },
};

#define CG_VARS_HASH_SIZE 512

static  cvarTable_t* hashTable[CG_VARS_HASH_SIZE];

int cvarTableSize = sizeof(cvarTable) / sizeof(cvarTable[0]);

/*
=================
CG_GetCgCvarByName
=================
*/

cvarTable_t* CG_GetCgCvarByName(const char* name)
{
	cvarTable_t* target = hashTable[Com_GenerateHashValue(name, CG_VARS_HASH_SIZE)];

	while (target && Q_stricmp(name, target->cvarName))
	{
		target = target->next;
	}
	if (target == NULL)
	{
		return NULL;
	}
	return Q_stricmp(name, target->cvarName) == 0 ? target : NULL;
}

void CG_CvarTouch(const char* name)
{
	cvarTable_t* cvart;
	cvart = CG_GetCgCvarByName(name);
	if (!cvart) CG_Error("CG_CvarTouch couldn't find cvar %s", name);
	++cvart->forceModelModificationCount;
}

void CG_CvarResetToDefault(const char* name)
{
	cvarTable_t* cvart;
	cvart = CG_GetCgCvarByName(name);
	if (!cvart) CG_Error("CG_CvarResetToDefault couldn't find cvar %s", name);
	CG_Printf("^1Reset ^3%s ^1to default value: ^3%s\n", cvart->cvarName, cvart->defaultString);
	Q_strncpyz(cvart->vmCvar->string, cvart->defaultString, MAX_CVAR_VALUE_STRING);
}

/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars(void)
{
	int         i;
	cvarTable_t* cv;
	char        var[MAX_TOKEN_CHARS];

	for (i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++)
	{
		cvarTable_t** target = NULL;

		unsigned long hash = Com_GenerateHashValue(cv->cvarName, CG_VARS_HASH_SIZE);

		trap_Cvar_Register(cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags);
		cv->forceModelModificationCount = cv->vmCvar->modificationCount;
		if (cv->vmCvar == &cg_drawTeamOverlay)
		{
			cv->forceModelModificationCount = -2;
		}

		/* store in the hash table */

		target = &hashTable[hash];
		if (*target == NULL)
		{
			*target = cv;
		}
		else
		{
			//collision
			while ((*target)->next)
			{
				target = &(*target)->next;
			}
			cv->prev = *target;
			(*target)->next = cv;
		}
	}

	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer("sv_running", var, sizeof(var));
	cgs.localServer = atoi(var) == 0 ? 0 : 1;

	trap_Cvar_VariableStringBuffer("sv_cheats", var, sizeof(var));
	cgs.cheatsEnabled = atoi(var) == 0 ? 0 : 1;

	trap_Cvar_Set("ui_recordSPDemo", ch_recordMessage.integer > 0 ? "0" : "1");
	//forceModelModificationCount = cg_forceModel.modificationCount;

	trap_Cvar_Register(NULL, "model", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE);
	trap_Cvar_Register(NULL, "headmodel", DEFAULT_MODEL, CVAR_USERINFO | CVAR_ARCHIVE);
	trap_Cvar_Register(NULL, "team_model", DEFAULT_TEAM_MODEL, CVAR_USERINFO | CVAR_ARCHIVE);
	trap_Cvar_Register(NULL, "team_headmodel", DEFAULT_TEAM_HEAD, CVAR_USERINFO | CVAR_ARCHIVE);
	trap_Cvar_Register(&sensitivity, "sensitivity", "5.0", CVAR_ARCHIVE);

	// temp table, no update (only manual with trap_Cvar_Update)
	for (i = 0, cv = cvarTable_temp; i < sizeof(cvarTable_temp) / sizeof(cvarTable_temp[0]); i++, cv++)
	{
		trap_Cvar_Register(cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags);
	}

	CG_OSPCvarsRestrictValues();
}

#define CG_CVARS_UPDATE_SLICES  6

static int cvarUpdateSliceIndex;

void CG_UpdateCvars(int maxEntries)
{
	int         processed;
	cvarTable_t* cv;

	if (cvarTableSize <= 0)
	{
		return;
	}

	if (maxEntries == 0)
	{
		maxEntries = (cvarTableSize + CG_CVARS_UPDATE_SLICES - 1) / CG_CVARS_UPDATE_SLICES;
	}
	else if (maxEntries < 0 || maxEntries > cvarTableSize)
	{
		maxEntries = cvarTableSize;
		cvarUpdateSliceIndex = 0;
	}

	for (processed = 0; processed < maxEntries; processed++)
	{
		cv = &cvarTable[cvarUpdateSliceIndex];
		cvarUpdateSliceIndex++;
		if (cvarUpdateSliceIndex >= cvarTableSize)
		{
			cvarUpdateSliceIndex = 0;
		}

		if (!cv->vmCvar)
		{
			continue;
		}
		trap_Cvar_Update(cv->vmCvar);

		if (cv->forceModelModificationCount == cv->vmCvar->modificationCount)
		{
			continue;
		}
		cv->forceModelModificationCount = cv->vmCvar->modificationCount;

		if (cv->onChanged)
		{
			cv->onChanged(cv);
		}
	}
}

int CG_CrosshairPlayer(void)
{
	if (cg.time > (cg.crosshairClientTime + 1000))
	{
		return -1;
	}
	return cg.crosshairClientNum;
}

int CG_LastAttacker(void)
{
	if (!cg.attackerTime)
	{
		return -1;
	}
	return cg.snap->ps.persistant[PERS_ATTACKER];
}

void CG_PrintLog(char* msg)
{
	if (cgs.osp.logFileHandle)
	{
		char* tmp;
		char buffer[1024];
		CG_OSPNormalizeText(msg, strlen(msg), &buffer[0]);
		tmp = va("%.1f %s\n", (float)cg.time / 1000.0, &buffer[0]);
		trap_FS_Write(tmp, strlen(tmp), cgs.osp.logFileHandle);
	}
}

void QDECL CG_Printf(const char* msg, ...)
{
	va_list     argptr;
	char        text[1024];

	if (msg == NULL)
	{
		return;
	}

	va_start(argptr, msg);
	vsprintf(text, msg, argptr);
	va_end(argptr);

	CG_PrintLog(text);
	trap_Print(text);
}

/*
================
CG_PrintOnce

Prints a message only once per unique message string within a time window.
Uses a hash-based tracking system with timestamps to allow re-printing after a delay.
================
*/
void QDECL CG_PrintOnce(const char* msg, ...)
{
	typedef struct
	{
		int hash;
		int lastPrintTime;
	} printEntry_t;

	static printEntry_t printedHashes[32];
	static int hashCount = 0;
	va_list     argptr;
	char        text[1024];
	int         hash;
	int         i;
	int         currentTime;
	const int   minInterval = 500;

	va_start(argptr, msg);
	vsprintf(text, msg, argptr);
	va_end(argptr);

	hash = 0;
	for (i = 0; text[i]; i++)
	{
		hash = (hash * 31) + text[i];
	}
	hash = hash & 0x7FFFFFFF;

	currentTime = cg.time;

	for (i = 0; i < hashCount; i++)
	{
		if (printedHashes[i].hash == hash)
		{
			if (currentTime - printedHashes[i].lastPrintTime < minInterval)
			{
				return;
			}
			printedHashes[i].lastPrintTime = currentTime;
			CG_PrintLog(text);
			trap_Print(text);
			return;
		}
	}

	if (hashCount < 32)
	{
		printedHashes[hashCount].hash = hash;
		printedHashes[hashCount].lastPrintTime = currentTime;
		hashCount++;
	}

	CG_PrintLog(text);
	trap_Print(text);
}

#ifdef __GNUC__
__attribute__((noreturn))
#endif
void QDECL CG_Error(const char* msg, ...)
{
	va_list     argptr;
	char        text[1024];

	va_start(argptr, msg);
	vsprintf(text, msg, argptr);
	va_end(argptr);

	CG_PrintLog(text);
	trap_Error(text);
#ifdef __GNUC__
	while (1) {}
#endif
}

#ifndef CGAME_HARD_LINKED
// this is only here so the functions in q_shared.c and bg_*.c can link (FIXME)

#ifdef __GNUC__
__attribute__((noreturn))
#endif
void QDECL Com_Error(int level, const char* error, ...)
{
	va_list     argptr;
	char        text[1024];

	va_start(argptr, error);
	vsprintf(text, error, argptr);
	va_end(argptr);

	CG_Error("%s", text);
}

void QDECL Com_Printf(const char* msg, ...)
{
	va_list     argptr;
	char        text[1024];

	va_start(argptr, msg);
	vsprintf(text, msg, argptr);
	va_end(argptr);

	CG_Printf("%s", text);
}

#endif

/*
================
CG_Argv
================
*/
const char* CG_Argv(int arg)
{
	static char buffer[MAX_STRING_CHARS];

	trap_Argv(arg, buffer, sizeof(buffer));

	return buffer;
}


//========================================================================

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level
=================
*/
static void CG_RegisterItemSounds(int itemNum)
{
	gitem_t*         item;
	char            data[MAX_QPATH];
	char*            s, *start;
	int             len;

	item = &bg_itemlist[ itemNum ];

	if (item->pickup_sound)
	{
		trap_S_RegisterSound(item->pickup_sound, qfalse);
	}

	// parse the space seperated precache string for other media
	s = item->sounds;
	if (!s || !s[0])
		return;

	while (*s)
	{
		start = s;
		while (*s && *s != ' ')
		{
			s++;
		}

		len = s - start;
		if (len >= MAX_QPATH || len < 5)
		{
			CG_Error("PrecacheItem: %s has bad precache string",
			         item->classname);
			return;
		}
		memcpy(data, start, len);
		data[len] = 0;
		if (*s)
		{
			s++;
		}

		if (!strcmp(data + len - 3, "wav"))
		{
			trap_S_RegisterSound(data, qfalse);
		}
	}
}


/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds(void)
{
	int     i;
	char    name[MAX_QPATH];
	const char*  soundName;


	cgs.media.oneMinuteSound = trap_S_RegisterSound("sound/feedback/1_minute.wav", qtrue);
	cgs.media.fiveMinuteSound = trap_S_RegisterSound("sound/feedback/5_minute.wav", qtrue);
	cgs.media.suddenDeathSound = trap_S_RegisterSound("sound/feedback/sudden_death.wav", qtrue);
	cgs.media.oneFragSound = trap_S_RegisterSound("sound/feedback/1_frag.wav", qtrue);
	cgs.media.twoFragSound = trap_S_RegisterSound("sound/feedback/2_frags.wav", qtrue);
	cgs.media.threeFragSound = trap_S_RegisterSound("sound/feedback/3_frags.wav", qtrue);

	cgs.media.count3Sound = trap_S_RegisterSound("sound/feedback/three.wav", qtrue);
	cgs.media.count2Sound = trap_S_RegisterSound("sound/feedback/two.wav", qtrue);
	cgs.media.count1Sound = trap_S_RegisterSound("sound/feedback/one.wav", qtrue);

	cgs.media.countFightSound = trap_S_RegisterSound("sound/feedback/fight.wav", qtrue);
	cgs.media.countPrepareSound = trap_S_RegisterSound("sound/feedback/prepare.wav", qtrue);

	if (cgs.gametype >= GT_TEAM || cg_buildScript.integer)
	{

		cgs.media.captureAwardSound = trap_S_RegisterSound("sound/teamplay/flagcapture_yourteam.wav", qtrue);
		cgs.media.redLeadsSound = trap_S_RegisterSound("sound/feedback/redleads.wav", qtrue);
		cgs.media.blueLeadsSound = trap_S_RegisterSound("sound/feedback/blueleads.wav", qtrue);
		cgs.media.teamsTiedSound = trap_S_RegisterSound("sound/feedback/teamstied.wav", qtrue);
		cgs.media.hitTeamSound = trap_S_RegisterSound("sound/feedback/hit_teammate.wav", qtrue);

		cgs.media.redScoredSound = trap_S_RegisterSound("sound/teamplay/voc_red_scores.wav", qtrue);
		cgs.media.blueScoredSound = trap_S_RegisterSound("sound/teamplay/voc_blue_scores.wav", qtrue);

		cgs.media.captureYourTeamSound = trap_S_RegisterSound("sound/teamplay/flagcapture_yourteam.wav", qtrue);
		cgs.media.captureOpponentSound = trap_S_RegisterSound("sound/teamplay/flagcapture_opponent.wav", qtrue);

		cgs.media.returnYourTeamSound = trap_S_RegisterSound("sound/teamplay/flagreturn_yourteam.wav", qtrue);
		cgs.media.returnOpponentSound = trap_S_RegisterSound("sound/teamplay/flagreturn_opponent.wav", qtrue);

		cgs.media.takenYourTeamSound = trap_S_RegisterSound("sound/teamplay/flagtaken_yourteam.wav", qtrue);
		cgs.media.takenOpponentSound = trap_S_RegisterSound("sound/teamplay/flagtaken_opponent.wav", qtrue);

		if (cgs.gametype == GT_CTF || cg_buildScript.integer)
		{
			cgs.media.redFlagReturnedSound = trap_S_RegisterSound("sound/teamplay/voc_red_returned.wav", qtrue);
			cgs.media.blueFlagReturnedSound = trap_S_RegisterSound("sound/teamplay/voc_blue_returned.wav", qtrue);
			cgs.media.enemyTookYourFlagSound = trap_S_RegisterSound("sound/teamplay/voc_enemy_flag.wav", qtrue);
			cgs.media.yourTeamTookEnemyFlagSound = trap_S_RegisterSound("sound/teamplay/voc_team_flag.wav", qtrue);
		}

		cgs.media.youHaveFlagSound = trap_S_RegisterSound("sound/teamplay/voc_you_flag.wav", qtrue);
		cgs.media.holyShitSound = trap_S_RegisterSound("sound/feedback/voc_holyshit.wav", qtrue);
		cgs.media.neutralFlagReturnedSound = trap_S_RegisterSound("sound/teamplay/flagreturn_opponent.wav", qtrue);
		cgs.media.yourTeamTookTheFlagSound = trap_S_RegisterSound("sound/teamplay/voc_team_1flag.wav", qtrue);
		cgs.media.enemyTookTheFlagSound = trap_S_RegisterSound("sound/teamplay/voc_enemy_1flag.wav", qtrue);
	}

	cgs.media.tracerSound = trap_S_RegisterSound("sound/weapons/machinegun/buletby1.wav", qfalse);
	cgs.media.selectSound = trap_S_RegisterSound("sound/weapons/change.wav", qfalse);
	cgs.media.wearOffSound = trap_S_RegisterSound("sound/items/wearoff.wav", qfalse);
	cgs.media.useNothingSound = trap_S_RegisterSound("sound/items/use_nothing.wav", qfalse);
	cgs.media.gibSound = trap_S_RegisterSound("sound/player/gibsplt1.wav", qfalse);
	cgs.media.gibBounce1Sound = trap_S_RegisterSound("sound/player/gibimp1.wav", qfalse);
	cgs.media.gibBounce2Sound = trap_S_RegisterSound("sound/player/gibimp2.wav", qfalse);
	cgs.media.gibBounce3Sound = trap_S_RegisterSound("sound/player/gibimp3.wav", qfalse);

	cgs.media.teleInSound = trap_S_RegisterSound("sound/world/telein.wav", qfalse);
	cgs.media.teleOutSound = trap_S_RegisterSound("sound/world/teleout.wav", qfalse);
	cgs.media.respawnSound = trap_S_RegisterSound("sound/items/respawn1.wav", qfalse);

	cgs.media.noAmmoSound = trap_S_RegisterSound("sound/weapons/noammo.wav", qfalse);
	cgs.media.lowAmmoSound = trap_S_RegisterSound("sound/weapons/lowammo.wav", qfalse);

	cgs.media.talkSound = trap_S_RegisterSound("sound/player/talk.wav", qfalse);
	cgs.media.landSound = trap_S_RegisterSound("sound/player/land1.wav", qfalse);


	// ========= Not using in code. But this files are originally in OSP1.03
	cgs.media.hitLowestSound = trap_S_RegisterSound("sound/feedback/hitlowest.wav", qfalse);
	cgs.media.hitLowSound = trap_S_RegisterSound("sound/feedback/hitlow.wav", qfalse);
	cgs.media.hitHighSound = trap_S_RegisterSound("sound/feedback/hithigh.wav", qfalse);
	// =========
	if (!cg_hitSoundsQC.integer)
	{
		cgs.media.hitSound = trap_S_RegisterSound("sound/feedback/hit.wav", qfalse);
		cgs.media.hitSounds[0] = trap_S_RegisterSound("sound/feedback/hit25.wav", qfalse);
		cgs.media.hitSounds[1] = trap_S_RegisterSound("sound/feedback/hit50.wav", qfalse);
		cgs.media.hitSounds[2] = trap_S_RegisterSound("sound/feedback/hit75.wav", qfalse);
		cgs.media.hitSounds[3] = trap_S_RegisterSound("sound/feedback/hit100.wav", qfalse);
	}
	else
	{
		cgs.media.hitSound = trap_S_RegisterSound("sound/feedback/hitQC.wav", qfalse);
		cgs.media.hitSounds[0] = trap_S_RegisterSound("sound/feedback/hit25QC.wav", qfalse);
		cgs.media.hitSounds[1] = trap_S_RegisterSound("sound/feedback/hit50QC.wav", qfalse);
		cgs.media.hitSounds[2] = trap_S_RegisterSound("sound/feedback/hit75QC.wav", qfalse);
		cgs.media.hitSounds[3] = trap_S_RegisterSound("sound/feedback/hit100QC.wav", qfalse);
	}
	// QC incoming damage sound
	cgs.media.gotDamageSounds[0] = trap_S_RegisterSound("sound/feedback/damage_qc25.wav", qfalse);
	cgs.media.gotDamageSounds[1] = trap_S_RegisterSound("sound/feedback/damage_qc50.wav", qfalse);
	cgs.media.gotDamageSounds[2] = trap_S_RegisterSound("sound/feedback/damage_qc75.wav", qfalse);
	cgs.media.gotDamageSounds[3] = trap_S_RegisterSound("sound/feedback/damage_qc100.wav", qfalse);

	cgs.media.impressiveSound = trap_S_RegisterSound("sound/feedback/impressive.wav", qtrue);
	cgs.media.excellentSound = trap_S_RegisterSound("sound/feedback/excellent.wav", qtrue);
	cgs.media.deniedSound = trap_S_RegisterSound("sound/feedback/denied.wav", qtrue);
	cgs.media.humiliationSound = trap_S_RegisterSound("sound/feedback/humiliation.wav", qtrue);
	cgs.media.assistSound = trap_S_RegisterSound("sound/feedback/assist.wav", qtrue);
	cgs.media.defendSound = trap_S_RegisterSound("sound/feedback/defense.wav", qtrue);

	cgs.media.takenLeadSound = trap_S_RegisterSound("sound/feedback/takenlead.wav", qtrue);
	cgs.media.tiedLeadSound = trap_S_RegisterSound("sound/feedback/tiedlead.wav", qtrue);
	cgs.media.lostLeadSound = trap_S_RegisterSound("sound/feedback/lostlead.wav", qtrue);

	cgs.media.watrInSound = trap_S_RegisterSound("sound/player/watr_in.wav", qfalse);
	cgs.media.watrOutSound = trap_S_RegisterSound("sound/player/watr_out.wav", qfalse);
	cgs.media.watrUnSound = trap_S_RegisterSound("sound/player/watr_un.wav", qfalse);

	cgs.media.jumpPadSound = trap_S_RegisterSound("sound/world/jumppad.wav", qfalse);

	for (i = 0 ; i < 4 ; i++)
	{
		Com_sprintf(name, sizeof(name), "sound/player/footsteps/step%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_NORMAL][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/boot%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_BOOT][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/flesh%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_FLESH][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/mech%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_MECH][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/energy%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_ENERGY][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/splash%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/clank%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_METAL][i] = trap_S_RegisterSound(name, qfalse);
	}

	// only register the items that the server says we need
	//strcpy( items, CG_ConfigString( CS_ITEMS ) );

	for (i = 1 ; i < bg_numItems ; i++)
	{
//		if ( items[ i ] == '1' || cg_buildScript.integer ) {
		CG_RegisterItemSounds(i);
//		}
	}

	for (i = 1 ; i < MAX_SOUNDS ; i++)
	{
		soundName = CG_ConfigString(CS_SOUNDS + i);
		if (!soundName[0])
		{
			break;
		}
		if (soundName[0] == '*')
		{
			continue;   // custom sound
		}
		cgs.gameSounds[i] = trap_S_RegisterSound(soundName, qfalse);
	}

	// FIXME: only needed with item
	cgs.media.flightSound = trap_S_RegisterSound("sound/items/flight.wav", qfalse);
	cgs.media.medkitSound = trap_S_RegisterSound("sound/items/use_medkit.wav", qfalse);
	cgs.media.quadSound = trap_S_RegisterSound("sound/items/damage3.wav", qfalse);
	cgs.media.sfx_ric1 = trap_S_RegisterSound("sound/weapons/machinegun/ric1.wav", qfalse);
	cgs.media.sfx_ric2 = trap_S_RegisterSound("sound/weapons/machinegun/ric2.wav", qfalse);
	cgs.media.sfx_ric3 = trap_S_RegisterSound("sound/weapons/machinegun/ric3.wav", qfalse);
	cgs.media.sfx_railg = trap_S_RegisterSound("sound/weapons/railgun/railgf1a.wav", qfalse);

	cgs.media.sfx_rockexp = trap_S_RegisterSound("sound/weapons/rocket/rocklx1a.wav", qfalse);
	cgs.media.sfx_plasmaexp = trap_S_RegisterSound("sound/weapons/plasma/plasmx1a.wav", qfalse);

	cgs.media.regenSound = trap_S_RegisterSound("sound/items/regen.wav", qfalse);
	cgs.media.protectSound = trap_S_RegisterSound("sound/items/protect3.wav", qfalse);
	cgs.media.n_healthSound = trap_S_RegisterSound("sound/items/n_health.wav", qfalse);
	cgs.media.hgrenb1aSound = trap_S_RegisterSound("sound/weapons/grenade/hgrenb1a.wav", qfalse);
	cgs.media.hgrenb2aSound = trap_S_RegisterSound("sound/weapons/grenade/hgrenb2a.wav", qfalse);

	cgs.media.freezeSoundReplacement = trap_S_RegisterSound("sound/misc/unfreeze2.wav", qfalse);

	// Team Task
	cgs.media.pingSimpleSound = trap_S_RegisterSound("sound/teamplay/ping-simple.wav", qfalse);
	cgs.media.pingDeathSound = trap_S_RegisterSound("sound/teamplay/ping-death.wav", qfalse);
	cgs.media.pingAcceptSound = trap_S_RegisterSound("sound/teamplay/ping-accept.wav", qfalse);
	cgs.media.pingDenySound = trap_S_RegisterSound("sound/teamplay/ping-deny.wav", qfalse);
	cgs.media.pingAttentionSound = trap_S_RegisterSound("sound/teamplay/ping-attention.wav", qfalse);
	cgs.media.pingPickupSound = trap_S_RegisterSound("sound/teamplay/ping-pickup.wav", qfalse);
	cgs.media.pingSimpleSoundNew = trap_S_RegisterSound("sound/teamplay/ping-simple-new.wav", qfalse);
	cgs.media.pingAcceptSoundNew = trap_S_RegisterSound("sound/teamplay/ping-accept-new.wav", qfalse);
	cgs.media.pingDenySoundNew = trap_S_RegisterSound("sound/teamplay/ping-deny-new.wav", qfalse);

	// Countdown sounds
	cgs.media.roundWarmup321Sound = trap_S_RegisterSound("sound/feedback/321.wav", qtrue);
	cgs.media.roundWarmup0Sound = trap_S_RegisterSound("sound/feedback/0.wav", qtrue);

}

qhandle_t CG_GetFragSound(void)
{
	static int loadedIndex = 0;
	int sound_index = cg_fragSound.integer;

	if (sound_index <= 0) sound_index = 1;

	if (loadedIndex != sound_index)
	{
		char path[MAX_QPATH];
		Com_sprintf(path, MAX_QPATH, "sound/feedback/fragSound%i.wav", sound_index);
		cgs.media.fragSound = trap_S_RegisterSound(path, qfalse);
		if (!cgs.media.fragSound)
		{
			cgs.media.fragSound = trap_S_RegisterSound("sound/feedback/fragSound1.wav", qfalse);
		}
		loadedIndex = sound_index;
	}
	return cgs.media.fragSound;
}

//===================================================================================


static const char* cg_numberSuffix[11] =
{
	"zero_32b",
	"one_32b",
	"two_32b",
	"three_32b",
	"four_32b",
	"five_32b",
	"six_32b",
	"seven_32b",
	"eight_32b",
	"nine_32b",
	"minus_32b",
};

static const char* CG_NumberShaderPrefix(void)
{
	switch (cg_numbersSet.integer)
	{
	case 2:
		return "gfx/2d/customnumbers/elite/";
	case 3:
		return "gfx/2d/customnumbers/eternal/";
	case 4:
		return "gfx/2d/customnumbers/odyssey/";
	case 1:
	default:
		return "gfx/2d/numbers/";
	}
}

void CG_RegisterNumberShaders(void)
{
	int i;
	const char* prefix;

	prefix = CG_NumberShaderPrefix();
	for (i = 0; i < 11; i++)
	{
		cgs.media.numberShaders[i] = trap_R_RegisterShader(va("%s%s", prefix, cg_numberSuffix[i]));
	}
}

/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics(void)
{
	int         i;
	char        items[MAX_ITEMS + 1];

	// clear any references to old media
	memset(&cg.refdef, 0, sizeof(cg.refdef));
	trap_R_ClearScene();

	CG_LoadingString(cgs.mapname);

	trap_R_LoadWorldMap(cgs.mapname);

	// precache status bar pics
	CG_LoadingString("game media");

	CG_RegisterNumberShaders();

	cgs.media.botSkillShaders[0] = trap_R_RegisterShader("menu/art/skill1.tga");
	cgs.media.botSkillShaders[1] = trap_R_RegisterShader("menu/art/skill2.tga");
	cgs.media.botSkillShaders[2] = trap_R_RegisterShader("menu/art/skill3.tga");
	cgs.media.botSkillShaders[3] = trap_R_RegisterShader("menu/art/skill4.tga");
	cgs.media.botSkillShaders[4] = trap_R_RegisterShader("menu/art/skill5.tga");
	cgs.media.botSkillShaders[5] = trap_R_RegisterShader("menu/art/skill6.tga");


	cgs.media.viewBloodShader = trap_R_RegisterShader("viewBloodBlend");
	cgs.media.damageIndicatorCenter = trap_R_RegisterShaderNoMip("damageIndicator2");

	cgs.media.deferShader = trap_R_RegisterShaderNoMip("gfx/2d/defer.tga");

	cgs.media.scoreboardName = trap_R_RegisterShaderNoMip("menu/tab/name.tga");
	cgs.media.scoreboardPing = trap_R_RegisterShaderNoMip("menu/tab/ping.tga");
	cgs.media.scoreboardScore = trap_R_RegisterShaderNoMip("menu/tab/score.tga");
	cgs.media.scoreboardTime = trap_R_RegisterShaderNoMip("menu/tab/time.tga");

	cgs.media.smokePuffShader = trap_R_RegisterShader("smokePuff");

	cgs.media.smokePuffNoPicMipShader = trap_R_RegisterShader("smokePuffNoPicMip");
	if (!cgs.media.smokePuffNoPicMipShader)
	{
		cgs.media.smokePuffNoPicMipShader = cgs.media.smokePuffShader;
	}


	cgs.media.smokePuffRageProShader = trap_R_RegisterShader("smokePuffRagePro");
	cgs.media.shotgunSmokePuffShader = trap_R_RegisterShader("shotgunSmokePuff");
	cgs.media.shotgunSmokePuffNoPicMipShader = trap_R_RegisterShader("shotgunSmokePuff");
	if (!cgs.media.shotgunSmokePuffNoPicMipShader)
	{
		cgs.media.shotgunSmokePuffNoPicMipShader = cgs.media.shotgunSmokePuffShader;
	}

	cgs.media.plasmaBallShader = trap_R_RegisterShader("sprites/plasma1");
	cgs.media.plasmaBallNoPicMipShader = trap_R_RegisterShader("sprites/plasma1NoPicMip");
	if (!cgs.media.plasmaBallNoPicMipShader)
	{
		cgs.media.plasmaBallNoPicMipShader = cgs.media.plasmaBallShader;
	}

	cgs.media.plasmaNewBallShader = trap_R_RegisterShader("sprites/plasma1new");
	if (!cgs.media.plasmaNewBallShader)
	{
		cgs.media.plasmaNewBallShader = cgs.media.plasmaBallShader;
	}

	cgs.media.plasmaNewBallNoPicMipShader = trap_R_RegisterShader("sprites/plasma1newNoPicMip");
	if (!cgs.media.plasmaNewBallNoPicMipShader)
	{
		cgs.media.plasmaNewBallNoPicMipShader = cgs.media.plasmaNewBallShader;
	}

	// pre alpha plasma ball
	cgs.media.plasmaOldBallShader = trap_R_RegisterShader("sprites/plasma_old");
	cgs.media.plasmaOldBallNoPicMipShader = cgs.media.plasmaOldBallShader;

	cgs.media.plasmaQLBallShader = trap_R_RegisterShader("sprites/plasmaQL");
	if (!cgs.media.plasmaQLBallShader)
		cgs.media.plasmaQLBallShader = cgs.media.plasmaBallShader;

	cgs.media.plasmaBallColorShader = trap_R_RegisterShader("sprites/plasma_color");
	cgs.media.plasmaBallColorOldShader = trap_R_RegisterShader("sprites/plasma_color_old");
	if (!cgs.media.plasmaBallColorOldShader)
		cgs.media.plasmaBallColorOldShader = cgs.media.plasmaBallColorShader;

	cgs.media.grenadeCPMANoPicMipShader = trap_R_RegisterShaderNoMip("grenadeCPMA_NPM");
	cgs.media.grenadeCPMANoPicMipShaderNew = trap_R_RegisterShaderNoMip("grenadeCPMANew");
	cgs.media.grenadeCPMAModel = trap_R_RegisterModel("models/ammo/grenadeCPMA.md3");

	if (!cgs.media.grenadeCPMAModel)
	{
		/* Bug? */
		cgs.media.grenadeCPMAModel = trap_R_RegisterModel("models/ammo/grenade1.md3");
		cgs.media.grenadeCPMANoPicMipShader = 0;
	}

	cgs.media.firstPersonGun = trap_R_RegisterShader("firstPersonGun");
	cgs.media.fullBrightShader = trap_R_RegisterShader("fBrightItems");
	cgs.media.altFullBrightShader = trap_R_RegisterShader("altFullBrightShader");
	cgs.media.shadowItemShader = trap_R_RegisterShader("beShadowItem");
	cgs.media.shadowItemFlagShader = trap_R_RegisterShader("beShadowItemFlag");
	cgs.media.flagBrightShader = trap_R_RegisterShader("models/flags/flag2Fullbright");
	cgs.media.flagBrightAltShader = trap_R_RegisterShader("models/flags/flag2Fullbright2");
	cgs.media.spawnModelShader = trap_R_RegisterShader("models/players/spawn");
	cgs.media.spawnMarkPlasmaShader = trap_R_RegisterShader("sprites/spawn_mark_plasma");
	if (!cgs.media.spawnMarkPlasmaShader)
	{
		cgs.media.spawnMarkPlasmaShader = cgs.media.plasmaBallColorShader;
	}
	cgs.media.bloodTrailShader = trap_R_RegisterShader("bloodTrail");
	cgs.media.lagometerShader = trap_R_RegisterShader("lagometer");
	cgs.media.connectionShader = trap_R_RegisterShader("disconnectedNew");
	cgs.media.waterBubbleShader = trap_R_RegisterShader("waterBubble");
	cgs.media.snowflakeShader = trap_R_RegisterShader("snowflake");


	cgs.media.tracerShader = trap_R_RegisterShader("gfx/misc/tracer");
	cgs.media.selectShader = trap_R_RegisterShader("gfx/2d/select");

	// osp crosshairs
	for (i = 0 ; i < 10 ; i++)
	{
		cgs.media.crosshairShader[i] = trap_R_RegisterShader(va("gfx/2d/crosshair%i", i));
		cgs.media.crosshairShader45[i] = trap_R_RegisterShader(va("gfx/2d/crosshair%i_r45", i));
	}
	// osp2 crosshairs
	for (; i < NUM_CROSSHAIRS ; i++)
	{
		cgs.media.crosshairShader[i] = trap_R_RegisterShader(va("gfx/2d/crosshair_%i",  i));
		cgs.media.crosshairShader45[i] = trap_R_RegisterShaderNoMip(va("gfx/2d/crosshair_%i_r45",  i));
	}
	// save how many crosshair do we have
	cgs.media.numberOfCrosshairs = NUM_CROSSHAIRS;

	// decors
	for (i = 1; i <= NUM_DECORS ; i++)
	{
		cgs.media.crosshairDecorShader[i] = trap_R_RegisterShader(va("gfx/2d/decor_%i",  i));
		cgs.media.crosshairDecorShader45[i] = trap_R_RegisterShaderNoMip(va("gfx/2d/decor_%i_r45",  i));
	}
	// save how many decors do we have
	cgs.media.numberOfCrosshairDecors = i;


	cgs.media.blender180Shader = trap_R_RegisterShader("gfx/2d/blender_180.png");

	/* team tasks */
	cgs.media.teamTasksAttack = trap_R_RegisterShader("teamTasks_attack");
	cgs.media.teamTasksDefence = trap_R_RegisterShader("teamTasks_defence");
	cgs.media.teamTasksWatch = trap_R_RegisterShader("teamTasks_watch");
	cgs.media.teamTasksDead = trap_R_RegisterShader("teamTasks_skull");
	cgs.media.teamTasksYes = trap_R_RegisterShader("teamTasks_yes");
	cgs.media.teamTasksNo = trap_R_RegisterShader("teamTasks_no");
	cgs.media.teamTasksLocation = trap_R_RegisterShader("teamTasks_location");
	cgs.media.teamTasksTarget = trap_R_RegisterShader("teamTasks_target");
	cgs.media.teamTasksRedarmor = trap_R_RegisterShader("teamTasks_redarmor");
	cgs.media.teamTasksMegahealth = trap_R_RegisterShader("teamTasks_megahealth");
	cgs.media.teamTasksQuad = trap_R_RegisterShader("teamTasks_quad");
	cgs.media.teamTasksRegen = trap_R_RegisterShader("teamTasks_regen");
	cgs.media.teamTasksBattlesuit = trap_R_RegisterShader("teamTasks_battlesuit");
	cgs.media.teamTasksHaste = trap_R_RegisterShader("teamTasks_haste");
	cgs.media.teamTasksYellowarmor = trap_R_RegisterShader("teamTasks_yellowarmor");
	cgs.media.teamTasksHealth = trap_R_RegisterShader("teamTasks_health");
	cgs.media.teamTasksMedic = trap_R_RegisterShader("teamTasks_medic");
	cgs.media.teamTasksWarning = trap_R_RegisterShader("teamTasks_warning");
	cgs.media.teamTasksFlag = trap_R_RegisterShader("teamTasks_flag");
	cgs.media.worldFlagsFlagTaken = trap_R_RegisterShader("teamTasks_flag_taken");
	cgs.media.worldFlagsFlagTaken_marker = trap_R_RegisterShader("teamTasks_flagTaken_marker");
	cgs.media.teamTasksFlagcarrier = trap_R_RegisterShader("teamTasks_flagcarrier");
	/* team tasks pickup items */
	cgs.media.teamTasksRedarmorPickup = trap_R_RegisterShader("teamTasks_redarmor_pickup");
	cgs.media.teamTasksMegahealthPickup = trap_R_RegisterShader("teamTasks_megahealth_pickup");
	cgs.media.teamTasksYellowarmorPickup = trap_R_RegisterShader("teamTasks_yellowarmor_pickup");
	/* team tasks overhead sprites (world only) */
	cgs.media.teamTasksYesSprite = trap_R_RegisterShader("sprites/teamTasks/yes");
	cgs.media.teamTasksNoSprite = trap_R_RegisterShader("sprites/teamTasks/no");
	cgs.media.teamTasksLocationSprite = trap_R_RegisterShader("sprites/teamTasks/location");
	cgs.media.teamTasksMedicSprite = trap_R_RegisterShader("sprites/teamTasks/medic");
	cgs.media.teamTasksWarningSprite = trap_R_RegisterShader("sprites/teamTasks/warning");
	cgs.media.teamTasksFlagcarrierSprite = trap_R_RegisterShader("sprites/teamTasks/flagcarrier");
	cgs.media.teamTasksRedarmorPickupSprite = trap_R_RegisterShader("sprites/teamTasks/item_redarmor_pickup");
	cgs.media.teamTasksMegahealthPickupSprite = trap_R_RegisterShader("sprites/teamTasks/item_megahealth_pickup");
	cgs.media.teamTasksYellowarmorPickupSprite = trap_R_RegisterShader("sprites/teamTasks/item_yellowarmor_pickup");
	/*====================
	team tasks hud markers
	====================*/
	cgs.media.teamTasksAttack_marker = trap_R_RegisterShader("teamTasks_attack_m");
	cgs.media.teamTasksDefence_marker = trap_R_RegisterShader("teamTasks_defence_m");
	cgs.media.teamTasksWatch_marker = trap_R_RegisterShader("teamTasks_watch_m");
	cgs.media.teamTasksDead_marker = trap_R_RegisterShader("teamTasks_skull_m");
	cgs.media.teamTasksYes_marker = trap_R_RegisterShader("teamTasks_yes_m");
	cgs.media.teamTasksNo_marker = trap_R_RegisterShader("teamTasks_no_m");
	cgs.media.teamTasksLocation_marker = trap_R_RegisterShader("teamTasks_location_m");
	cgs.media.teamTasksTarget_marker = trap_R_RegisterShader("teamTasks_target_m");
	cgs.media.teamTasksRedarmor_marker = trap_R_RegisterShader("teamTasks_redarmor_m");
	cgs.media.teamTasksMegahealth_marker = trap_R_RegisterShader("teamTasks_megahealth_m");
	cgs.media.teamTasksQuad_marker = trap_R_RegisterShader("teamTasks_quad_m");
	cgs.media.teamTasksRegen_marker = trap_R_RegisterShader("teamTasks_regen_m");
	cgs.media.teamTasksBattlesuit_marker = trap_R_RegisterShader("teamTasks_battlesuit_m");
	cgs.media.teamTasksHaste_marker = trap_R_RegisterShader("teamTasks_haste_m");
	cgs.media.teamTasksYellowarmor_marker = trap_R_RegisterShader("teamTasks_yellowarmor_m");
	cgs.media.teamTasksHealth_marker = trap_R_RegisterShader("teamTasks_health_m");
	cgs.media.teamTasksMedic_marker = trap_R_RegisterShader("teamTasks_medic_m");
	cgs.media.teamTasksWarning_marker = trap_R_RegisterShader("teamTasks_warning_m");
	cgs.media.teamTasksFlag_marker = trap_R_RegisterShader("teamTasks_flag_m");
	cgs.media.teamTasksFlagcarrier_marker = trap_R_RegisterShader("teamTasks_flagcarrier_m");
	cgs.media.teamTasksFrozen_marker = trap_R_RegisterShader("teamTasks_frozen_m");
	/* team tasks item pickup markers */
	cgs.media.teamTasksRedarmorPickup_marker = trap_R_RegisterShader("teamTasks_redarmor_pickup_m");
	cgs.media.teamTasksMegahealthPickup_marker = trap_R_RegisterShader("teamTasks_megahealth_pickup_m");
	cgs.media.teamTasksYellowarmorPickup_marker = trap_R_RegisterShader("teamTasks_yellowarmor_pickup_m");
	/* team tasks misc */
	cgs.media.teamTasksCircle = trap_R_RegisterShader("teamTasks_circle");
	cgs.media.teamTasksSmart = trap_R_RegisterShader("teamTasks_smart");
	cgs.media.teamTasksWheelbg = trap_R_RegisterShader("teamTasks_wheelbg");
	cgs.media.teamTasksPointing = trap_R_RegisterShader("teamTasks_pointing");
	cgs.media.teamTasksUnknown = trap_R_RegisterShader("teamTasks_unknown");
	cgs.media.teamTasksPickupEmpty = trap_R_RegisterShader("teamTasks_pickupEmpty");

	cgs.osp.decalOSPShaderCopyCount = 0;
	cgs.osp.decalOSPShader[0] = trap_R_RegisterShader("decalOSP");
	if (cgs.osp.decalOSPShader[0])
	{
		cgs.osp.blender180ShaderCopy[cgs.osp.decalOSPShaderCopyCount] = cgs.media.blender180Shader;
		++cgs.osp.decalOSPShaderCopyCount;
	}
	/*
	 * Bug! blender180ShaderCopy and decalOSPShader overflow possible
	 */
	for (i = 0 ; i < 16; ++i)
	{
		cgs.media.customShader[i] = trap_R_RegisterShaderNoMip(va("gfx/2d/custom%d", i + 1));
		if (!cgs.media.customShader[i])
		{
			break;
		}
		cgs.osp.decalOSPShader[cgs.osp.decalOSPShaderCopyCount] = trap_R_RegisterShaderNoMip(va("decal%d", i + 1));
		if (cgs.osp.decalOSPShader[cgs.osp.decalOSPShaderCopyCount])
		{
			cgs.osp.blender180ShaderCopy[cgs.osp.decalOSPShaderCopyCount] = cgs.media.customShader[i];
		}
		++cgs.osp.decalOSPShaderCopyCount;
	}

	//--------------------------
	cgs.media.backTileShader = trap_R_RegisterShader("gfx/2d/backtile");
	cgs.media.noammoShader = trap_R_RegisterShader("icons/noammo");

	// powerup shaders
	cgs.media.quadShader = trap_R_RegisterShader("powerups/quad");
	cgs.media.quadShaderDye = trap_R_RegisterShader("powerups/quadDye");
	cgs.media.quadWeaponShader = trap_R_RegisterShader("powerups/quadWeapon");
	cgs.media.quadWeaponShaderDye = trap_R_RegisterShader("powerups/quadWeaponDye");
	cgs.media.fragEffectShader = trap_R_RegisterShader("weapons/fragEffect");
	cgs.media.battleSuitShader = trap_R_RegisterShader("powerups/battleSuit");
	cgs.media.battleSuitShaderDye = trap_R_RegisterShader("powerups/battleSuitDye");
	cgs.media.battleWeaponShader = trap_R_RegisterShader("powerups/battleWeapon");
	cgs.media.battleWeaponShaderDye = trap_R_RegisterShader("powerups/battleWeaponDye");
	cgs.media.invisShader = trap_R_RegisterShader("powerups/invisibility");
	cgs.media.regenShader = trap_R_RegisterShader("powerups/regen");
	cgs.media.hastePuffShader = trap_R_RegisterShader("hasteSmokePuff");

	cgs.media.freezeShader = trap_R_RegisterShader("freezeShader");
	cgs.media.freezeMarkShader = trap_R_RegisterShader("freezeMarkShader");

	// powerup icons
	cgs.media.quadDamageIcon = trap_R_RegisterShaderNoMip("icons/quad");
	cgs.media.battleSuitIcon = trap_R_RegisterShaderNoMip("icons/envirosuit");
	cgs.media.hasteIcon = trap_R_RegisterShaderNoMip("icons/haste");
	cgs.media.invisIcon = trap_R_RegisterShaderNoMip("icons/invis");
	cgs.media.regenIcon = trap_R_RegisterShaderNoMip("icons/regen");
	cgs.media.flightIcon = trap_R_RegisterShaderNoMip("icons/flight");
	cgs.media.teleporterIcon = trap_R_RegisterShaderNoMip("icons/teleporter");
	cgs.media.medkitIcon = trap_R_RegisterShaderNoMip("icons/medkit");

	if (cgs.gametype == GT_CTF || cg_buildScript.integer)
	{
		cgs.media.redCubeModel = trap_R_RegisterModel("models/powerups/orb/r_orb.md3");
		cgs.media.blueCubeModel = trap_R_RegisterModel("models/powerups/orb/b_orb.md3");
	}

	if (cgs.gametype == GT_CTF || cg_buildScript.integer)
	{
		cgs.media.dyeFlagModel = trap_R_RegisterModel("models/flags/flagDye.md3");
		cgs.media.dyeFlagShader[0] = trap_R_RegisterShaderNoMip("icons/dye/iconf_dye1");
		cgs.media.dyeFlagShader[1] = trap_R_RegisterShaderNoMip("icons/dye/iconf_dye2");
		cgs.media.dyeFlagShader[2] = trap_R_RegisterShaderNoMip("icons/dye/iconf_dye3");
		cgs.media.dyeFlagShaderInv[0] = trap_R_RegisterShaderNoMip("icons/dye/iconf_dye1inv");
		cgs.media.dyeFlagShaderInv[1] = trap_R_RegisterShaderNoMip("icons/dye/iconf_dye2inv");
		cgs.media.dyeFlagShaderInv[2] = trap_R_RegisterShaderNoMip("icons/dye/iconf_dye3inv");
		cgs.media.classicRedFlagModel = trap_R_RegisterModel("models/flags/r_flag.md3");
		cgs.media.classicBlueFlagModel = trap_R_RegisterModel("models/flags/b_flag.md3");
		cgs.media.classicRedFlagShader[0] = trap_R_RegisterShaderNoMip("icons/iconf_red1");
		cgs.media.classicRedFlagShader[1] = trap_R_RegisterShaderNoMip("icons/iconf_red2");
		cgs.media.classicRedFlagShader[2] = trap_R_RegisterShaderNoMip("icons/iconf_red3");
		cgs.media.classicBlueFlagShader[0] = trap_R_RegisterShaderNoMip("icons/iconf_blu1");
		cgs.media.classicBlueFlagShader[1] = trap_R_RegisterShaderNoMip("icons/iconf_blu2");
		cgs.media.classicBlueFlagShader[2] = trap_R_RegisterShaderNoMip("icons/iconf_blu3");
		CG_UpdateFlagMedia();
	}


	if (cgs.gametype >= GT_TEAM || cg_buildScript.integer)
	{
		cgs.media.frozenFoeTagShader = trap_R_RegisterShader("sprites/frozenFoeTag");
		cgs.media.frozenFoeTagShaderWallhack = trap_R_RegisterShader("sprites/frozenFoeTag2");
		cgs.media.frozenShader = trap_R_RegisterShader("textures/effects/frozen");
		cgs.media.frozenNewShader = trap_R_RegisterShader("textures/effects/frozenNew");
		cgs.media.frozenNewShader2 = trap_R_RegisterShader("textures/effects/frozenNew2");
		cgs.media.friendShader = trap_R_RegisterShader("sprites/foe");
		cgs.media.friendShaderWallhack = trap_R_RegisterShader("sprites/foe2");
		cgs.media.redQuadShader = trap_R_RegisterShader("powerups/quadRed");
		cgs.media.circleShader = trap_R_RegisterShader("gfx/2d/hudres/rec");
	}


	cgs.media.teamStatusBar = trap_R_RegisterShader("gfx/2d/colorbar.tga");

	cgs.media.armorModel[0] = trap_R_RegisterModel("models/powerups/armor/armor_gre.md3");
	cgs.media.armorIcon[0]  = trap_R_RegisterShaderNoMip("icons/iconr_green");

	cgs.media.armorModel[1] = trap_R_RegisterModel("models/powerups/armor/armor_yel.md3");
	cgs.media.armorIcon[1]  = trap_R_RegisterShaderNoMip("icons/iconr_yellow");

	cgs.media.armorModel[2] = trap_R_RegisterModel("models/powerups/armor/armor_red.md3");
	cgs.media.armorIcon[2]  = trap_R_RegisterShaderNoMip("icons/iconr_red");

	cgs.media.healthIcon = trap_R_RegisterShaderNoMip("icons/iconh_yellow");

	cgs.media.machinegunBrassModel = trap_R_RegisterModel("models/weapons2/shells/m_shell.md3");
	cgs.media.shotgunBrassModel = trap_R_RegisterModel("models/weapons2/shells/s_shell.md3");

	cgs.media.gibAbdomen = trap_R_RegisterModel("models/gibs/abdomen.md3");
	cgs.media.gibArm = trap_R_RegisterModel("models/gibs/arm.md3");
	cgs.media.gibChest = trap_R_RegisterModel("models/gibs/chest.md3");
	cgs.media.gibFist = trap_R_RegisterModel("models/gibs/fist.md3");
	cgs.media.gibFoot = trap_R_RegisterModel("models/gibs/foot.md3");
	cgs.media.gibForearm = trap_R_RegisterModel("models/gibs/forearm.md3");
	cgs.media.gibIntestine = trap_R_RegisterModel("models/gibs/intestine.md3");
	cgs.media.gibLeg = trap_R_RegisterModel("models/gibs/leg.md3");
	cgs.media.gibSkull = trap_R_RegisterModel("models/gibs/skull.md3");
	cgs.media.gibBrain = trap_R_RegisterModel("models/gibs/brain.md3");

	cgs.media.smoke2 = trap_R_RegisterModel("models/weapons2/shells/s_shell.md3");

	cgs.media.balloonShader = trap_R_RegisterShader("sprites/balloon3NoPicMip");
	if (!cgs.media.balloonShader)
	{
		cgs.media.balloonShader = trap_R_RegisterShader("sprites/balloon3");
	}

	cgs.media.bloodExplosionShader = trap_R_RegisterShader("bloodExplosion");

	cgs.media.bloodExplosionNoPicMipShader = trap_R_RegisterShader("bloodExplosionNoPicMip");
	if (!cgs.media.bloodExplosionNoPicMipShader)
	{
		cgs.media.bloodExplosionNoPicMipShader = cgs.media.bloodExplosionShader;


	}
	CG_RegisterAltShaderSeries(cgs.media.bloodExplosionShaderNew, "bloodExplosionNew");

	cgs.media.bulletFlashModel = trap_R_RegisterModel("models/weaphits/bullet.md3");
	cgs.media.ringFlashModel = trap_R_RegisterModel("models/weaphits/ring02.md3");
	cgs.media.dishFlashModel = trap_R_RegisterModel("models/weaphits/boom01.md3");
	cgs.media.teleportEffectModel = trap_R_RegisterModel("models/misc/telep.md3");
	cgs.media.teleportEffectShader = trap_R_RegisterShader("teleportEffect");
	cgs.media.teleporterStarsShader = trap_R_RegisterShader("teleporterStars");

	cgs.media.cursor = trap_R_RegisterShaderNoMip("menu/art/3_cursor2");

	cgs.media.invulnerabilityPowerupModel = trap_R_RegisterModel("models/powerups/shield/shield.md3");
	cgs.media.medalImpressive = trap_R_RegisterShaderNoMip("medal_impressive");
	cgs.media.medalExcellent = trap_R_RegisterShaderNoMip("medal_excellent");
	cgs.media.medalGauntlet = trap_R_RegisterShaderNoMip("medal_gauntlet");
	cgs.media.medalDefend = trap_R_RegisterShaderNoMip("medal_defend");
	cgs.media.medalAssist = trap_R_RegisterShaderNoMip("medal_assist");
	cgs.media.medalCapture = trap_R_RegisterShaderNoMip("medal_capture");

	CG_RegisterMedalAnimationShaders();

	// superhud
	cgs.media.obituariesGrenadeDirect = trap_R_RegisterShader("ObituariesGrenadeDirect");
	cgs.media.obituariesRocketDirect = trap_R_RegisterShader("ObituariesRocketDirect");
	cgs.media.obituariesBFGDirect = trap_R_RegisterShader("ObituariesBFGDirect");
	cgs.media.obituariesDrowned = trap_R_RegisterShader("ObituariesDrowned");
	cgs.media.obituariesMelted = trap_R_RegisterShader("ObituariesMelted");
	cgs.media.obituariesLava = trap_R_RegisterShader("ObituariesLava");
	cgs.media.obituariesTelefrag = trap_R_RegisterShader("ObituariesTelefrag");
	cgs.media.obituariesFallenCrashed = trap_R_RegisterShader("ObituariesFallenCrashed");
	cgs.media.obituariesFalling = trap_R_RegisterShader("ObituariesFalling");
	cgs.media.obituariesSkull = trap_R_RegisterShader("ObituariesSkull");

	cgs.media.arrowUp = trap_R_RegisterShader("gfx/2d/arrow_up");
	cgs.media.arrowDown = trap_R_RegisterShader("gfx/2d/arrow_down");

	cgs.media.tempAccIcon = trap_R_RegisterShader("tempAcc_LG_Icon");

	// be extention
	cgs.media.hboxShaderNew      = trap_R_RegisterShader("hboxNew");
	cgs.media.hboxShaderNew_nocull      = trap_R_RegisterShader("hboxNew_nocull");
	cgs.media.hboxShaderNew_cullback    = trap_R_RegisterShader("hboxNew_cullback");

	// Enemy outline
	cgs.media.outlineShader =
	    (cg_enemyOutlineSize.integer == 1) ? trap_R_RegisterShader("outlineThin") :
	    (cg_enemyOutlineSize.integer == 2) ? trap_R_RegisterShader("outlineMedium") :
	    trap_R_RegisterShader("outlineMedium");

	// Team outline
	cgs.media.teamOutlineShader =
	    (cg_teamOutlineSize.integer == 1) ? trap_R_RegisterShader("outlineThin") :
	    (cg_teamOutlineSize.integer == 2) ? trap_R_RegisterShader("outlineMedium") :
	    trap_R_RegisterShader("outlineMedium");

	cgs.media.unknownSkinIcon = trap_R_RegisterShaderNoMip("models/players/icon_unknown.tga");

	cgs.media.unknownHeadModel = trap_R_RegisterModel("models/players/sarge/head.md3");
	if (cgs.media.unknownHeadModel)
	{
		cgs.media.unknownHeadSkin = trap_R_RegisterSkin("models/players/sarge/head_default.skin");
	}

	memset(cg_items, 0, sizeof(cg_items));
	memset(cg_weapons, 0, sizeof(cg_weapons));

	// only register the items that the server says we need
	strcpy(items, CG_ConfigString(CS_ITEMS));

	for (i = 1 ; i < bg_numItems ; i++)
	{
		if (items[ i ] == '1' || cg_buildScript.integer)
		{
			CG_LoadingItem(i);
			CG_RegisterItemVisuals(i);
		}
	}

	// wall marks
	cgs.media.bulletMarkShader = trap_R_RegisterShader("gfx/damage/bullet_mrk");
	cgs.media.bulletMarkNoPicMipShader = trap_R_RegisterShader("gfx/damage/bullet_mrkNopPicMip");
	if (!cgs.media.bulletMarkNoPicMipShader)
	{
		cgs.media.bulletMarkNoPicMipShader = cgs.media.bulletMarkShader;
	}

	cgs.media.burnMarkShader = trap_R_RegisterShader("gfx/damage/burn_med_mrk");
	cgs.media.burnMarkNoPicMipShader = trap_R_RegisterShader("gfx/damage/burn_med_mrkNopPicMip");
	if (!cgs.media.burnMarkNoPicMipShader)
	{
		cgs.media.burnMarkNoPicMipShader = cgs.media.burnMarkShader;
	}

	cgs.media.holeMarkShader = trap_R_RegisterShader("gfx/damage/hole_lg_mrk");
	cgs.media.holeMarkNoPicMipShader = trap_R_RegisterShader("gfx/damage/hole_lg_mrkNopPicMip");
	if (!cgs.media.holeMarkNoPicMipShader)
	{
		cgs.media.holeMarkNoPicMipShader = cgs.media.holeMarkShader;
	}

	cgs.media.energyMarkShader = trap_R_RegisterShader("gfx/damage/plasma_mrk");
	cgs.media.energyMarkPlasmaShader = trap_R_RegisterShader("gfx/damage/plasma_mrk_be");
	cgs.media.energyMarkNoPicMipShader = trap_R_RegisterShader("gfx/damage/plasma_mrkNopPicMip");
	if (!cgs.media.energyMarkNoPicMipShader)
	{
		cgs.media.energyMarkNoPicMipShader = cgs.media.energyMarkShader;
	}

	cgs.media.shadowMarkShader = trap_R_RegisterShader("markShadow");
	CG_RegisterAltShaderSeries(cgs.media.shadowMarkShaderNew, "markShadowNew");

	cgs.media.wakeMarkShader = trap_R_RegisterShader("wake");
	cgs.media.bloodMarkShader = trap_R_RegisterShader("bloodMark");
	// get the shader handles
	cgs.osp.hboxShader = trap_R_RegisterShader("gfx/misc/hbox");
	cgs.osp.hboxShader_nocull = trap_R_RegisterShader("gfx/misc/hbox_nocull");

	// register the inline models
	cgs.numInlineModels = trap_CM_NumInlineModels();
	for (i = 1 ; i < cgs.numInlineModels ; i++)
	{
		char    name[10];
		vec3_t          mins, maxs;
		int             j;

		Com_sprintf(name, sizeof(name), "*%i", i);
		cgs.inlineDrawModel[i] = trap_R_RegisterModel(name);
		trap_R_ModelBounds(cgs.inlineDrawModel[i], mins, maxs);
		for (j = 0 ; j < 3 ; j++)
		{
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * (maxs[j] - mins[j]);
		}
	}

	// register all the server specified models
	for (i = 1 ; i < MAX_MODELS ; i++)
	{
		const char*      modelName;

		modelName = CG_ConfigString(CS_MODELS + i);
		if (!modelName[0])
		{
			break;
		}
		cgs.gameModels[i] = trap_R_RegisterModel(modelName);
	}

	CG_ClearParticles();
}



/*
=======================
CG_BuildSpectatorString

=======================
*/
void CG_BuildSpectatorString()
{
	int i;
	cg.spectatorList[0] = 0;
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_SPECTATOR)
		{
			Q_strcat(cg.spectatorList, sizeof(cg.spectatorList), va("%s     ", cgs.clientinfo[i].name));
		}
	}
	i = strlen(cg.spectatorList);
	if (i != cg.spectatorLen)
	{
		cg.spectatorLen = i;
		cg.spectatorWidth = -1;
	}
}


/*
===================
CG_RegisterClients
===================
*/
static void CG_RegisterClients(void)
{
	int     i;
	const char*      clientInfo;

	CG_LoadingClient(cg.clientNum);
	CG_NewClientInfo(cg.clientNum);

	/* Check if server supports team task by looking for "tt" field in our configstring */
	clientInfo = CG_ConfigString(CS_PLAYERS + cg.clientNum);
	if (clientInfo[0] && strstr(clientInfo, "\\tt"))
	{
		cg.teamMarkers.supportedServer = qtrue;
	}

	for (i = 0 ; i < MAX_CLIENTS ; i++)
	{
		if (cg.clientNum == i)
		{
			continue;
		}

		clientInfo = CG_ConfigString(CS_PLAYERS + i);
		if (!clientInfo[0])
		{
			continue;
		}
		CG_LoadingClient(i);
		CG_NewClientInfo(i);
	}
	CG_BuildSpectatorString();
}

//===========================================================================

/*
=================
CG_ConfigString
=================
*/
const char* CG_ConfigString(int index)
{
	if (index < 0 || index >= MAX_CONFIGSTRINGS)
	{
		CG_Error("CG_ConfigString: bad index: %i", index);
	}
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[ index ];
}

//==================================================================

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic(void)
{
	char*    s;
	char    parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	s = (char*)CG_ConfigString(CS_MUSIC);
	Q_strncpyz(parm1, COM_Parse(&s), sizeof(parm1));
	Q_strncpyz(parm2, COM_Parse(&s), sizeof(parm2));

	trap_S_StartBackgroundTrack(parm1, parm2);
}

/*
======================
CG_CheckFogBypass
Ignore fog on some maps
======================
*/
void CG_CheckFogBypass(void)
{
	char mapname[64];
	int i;
	const char* fogBypassMaps[] = { "oxodm68.bsp",
	                                "oxodm19.bsp",
	                                "quarantine.bsp",
	                                "asylum.bsp",
	                                "rjldm3.bsp",
	                                "gen_q1dm1.bsp",
	                                "13death_xt_b1",
	                                "13excave",
	                                "q3dm4",
	                                NULL
	                              };

	if (cgs.mapname[0] != '\0')
	{
		const char* mapPrefix = "maps/";
		const char* mapStart = strstr(cgs.mapname, mapPrefix);

		if (mapStart)
		{
			Q_strncpyz(mapname, mapStart + strlen(mapPrefix), sizeof(mapname));
		}
		else
		{
			Q_strncpyz(mapname, cgs.mapname, sizeof(mapname));
		}

		for (i = 0; fogBypassMaps[i] != NULL; i++)
		{
			if (Q_stricmp(fogBypassMaps[i], mapname) == 0)
			{
				cg.crosshairIgnoreFog = 1;
				return;
			}
		}
	}

	cg.crosshairIgnoreFog = 0;
}

void CG_InitCvars(void)
{
	CG_CvarTouch("ch_crosshairColor");
	CG_CvarTouch("ch_crosshairActionColor");
	CG_CvarTouch("ch_crosshairActionColorLow");
	CG_CvarTouch("ch_crosshairActionColorMid");
	CG_CvarTouch("ch_crosshairActionColorHigh");

	CG_CvarTouch("ch_crosshairDecorColor");
	CG_CvarTouch("ch_crosshairDecorActionColor");
	CG_CvarTouch("ch_crosshairDecorActionColorLow");
	CG_CvarTouch("ch_crosshairDecorActionColorMid");
	CG_CvarTouch("ch_crosshairDecorActionColorHigh");


	CG_CvarTouch("cg_hitBoxColor");
	CG_CvarTouch("cg_enemyOutlineColor");
	CG_CvarTouch("cg_teamOutlineColor");
	CG_CvarTouch("cg_altBloodColor");
	CG_CvarTouch("cg_altShadowColor");

	CG_CvarTouch("ch_crosshairDecorOpaque");
	CG_CvarTouch("ch_crosshairOpaque");
	CG_CvarTouch("ch_crosshairActionScale");
	CG_CvarTouch("ch_crosshairActionTime");
	CG_CvarTouch("ch_crosshairDecorActionScale");
	CG_CvarTouch("ch_crosshairDecorActionTime");

	CG_CvarTouch("cg_damageIndicatorOpaque");

	CG_CvarTouch("cg_dlightGauntlet");
	CG_CvarTouch("cg_dlightMG");
	CG_CvarTouch("cg_dlightSG");
	CG_CvarTouch("cg_dlightGL");
	CG_CvarTouch("cg_dlightRL");
	CG_CvarTouch("cg_dlightLG");
	CG_CvarTouch("cg_dlightRG");
	CG_CvarTouch("cg_dlightPG");
	CG_CvarTouch("cg_dlightBFG");

	CG_CvarTouch("cg_teamIndicatorColor");
	CG_CvarTouch("cg_teamIndicatorOpaque");
	CG_CvarTouch("cg_teamIndicatorBgColor");
	CG_CvarTouch("cg_teamIndicatorBgOpaque");
	CG_CvarTouch("cg_teamIndicatorOffset");
	CG_CvarTouch("cg_teamIndicatorMaxLength");
	CG_CvarTouch("cg_emojiListBgColor");
	CG_CvarTouch("cg_emojiListOpaque");
	CG_CvarTouch("cg_chatWindowBgColor");
	CG_CvarTouch("cg_chatWindowBgOpaque");
	CG_CvarTouch("cg_teamIndicatorFontSize");
	CG_CvarTouch("cg_teamIndicatorAdjust");

	CG_CvarTouch("cg_teamIndicatorFont");
	CG_CvarTouch("cg_scoreboardFont");
	CG_CvarTouch("cg_centerMessagesFont");
	CG_CvarTouch("cg_infoVersionFont");
	CG_CvarTouch("cg_numbersSet");

	CG_CvarTouch("cg_altGrenadesColor");
	CG_CvarTouch("cg_enemyGrenadesColor");
	CG_CvarTouch("cg_plasmaColor");
	CG_CvarTouch("cg_enemyPlasmaColor");
	CG_CvarTouch("cg_battleSuitColor");
	CG_CvarTouch("cg_enemyBattleSuitColor");
	CG_CvarTouch("cg_quadDamageColor");
	CG_CvarTouch("cg_enemyQuadDamageColor");
	CG_CvarTouch("cg_fragEffectColor");

	CG_CvarTouch("cg_healthColor");
	CG_CvarTouch("cg_healthLowColor");
	CG_CvarTouch("cg_healthMidColor");

	CG_CvarTouch("cg_redTeamColor");
	CG_CvarTouch("cg_blueTeamColor");

	CG_CvarTouch("cg_markTeam");
	CG_CvarTouch("cg_markTeamColor");

	CG_CvarTouch("cg_scoreboardRtColors");
	CG_CvarTouch("cg_scoreboardBtColors");

	CG_CvarTouch("cg_scoreboardSpecColor");


	CG_CvarTouch("cg_bestats_textSize");
	CG_CvarTouch("cg_bestats_pos");
	CG_CvarTouch("cg_bestats_font");
	CG_CvarTouch("cg_bestats_bgColor");

	CG_CvarTouch("cg_accuracyFontSize");

	/* CG_CvarTouch("cg_autoHop"); */  /* autohop commented out */
	CG_CvarTouch("cg_teamTaskFont");
	CG_CvarTouch("cg_teamTaskThemeColor");

	CG_CvarTouch("cg_brightWeaponsOpaque");

	CG_CvarTouch("cg_gunX");
	CG_CvarTouch("cg_gunY");
	CG_CvarTouch("cg_gunZ");
	CG_CvarTouch("cg_mySound");
	CG_CvarTouch("cg_enemySound");
	CG_CvarTouch("cg_teamSound");

	CG_CvarTouch("cg_railRingsSize");
	CG_CvarTouch("cg_railRingsRotation");
	CG_CvarTouch("cg_railRingsRadius");
	CG_CvarTouch("cg_railRingsSpacing");

	CG_CvarTouch("cl_pitchspeed");
	CG_CvarTouch("cl_yawspeed");
}

/*
=================
CG_InitTeamColors

Force-initialize base and effective team colors from cg_redTeamColor/cg_blueTeamColor
so they are correct on first game entry (before CG_UpdateMyTeam or cvar callbacks run).
=================
*/
static void CG_InitTeamColors(void)
{
	cvarTable_t* cvart;

	cvart = CG_GetCgCvarByName("cg_redTeamColor");
	if (cvart && cvart->vmCvar && cvart->onChanged)
	{
		trap_Cvar_Update(cvart->vmCvar);
		cvart->onChanged(cvart);
	}
	cvart = CG_GetCgCvarByName("cg_blueTeamColor");
	if (cvart && cvart->vmCvar && cvart->onChanged)
	{
		trap_Cvar_Update(cvart->vmCvar);
		cvart->onChanged(cvart);
	}
}

/*
=================
CG_InitPlayerColors

Rebuild my/team/enemy colors and refresh all client infos after map load.
Required so that in non-team modes (e.g. GT_TOURNAMENT) cgs.osp.enemyColors
is set and other players do not render black.
=================
*/
static void CG_InitPlayerColors(void)
{
	// Always rebuild and refresh: in non-team modes (e.g. GT_TOURNAMENT) enemy
	// colors come from cgs.osp.enemyColors; if we never call CG_RebuildPlayerColors
	// (e.g. when no cvar contains 'E' or 'T'), enemyColors stay zero and other
	// players render black until user manually reloads colors.
	CG_RebuildPlayerColors();
	CG_UpdateBeFeatures();
	CG_UpdateAllClientsInfo();
}

/*
=================
CG_ParseCompilationInfo

Parse compilation info from 'cversion' file and set compilation_build cvar
Format: Compilation "NAME" Version "VERSION"
=================
*/
void CG_ParseCompilationInfo(void)
{
	fileHandle_t fileHandle;
	int fileSize;
	char* fileContent;
	char buildString[MAX_QPATH * 2 + 2];
	char name[MAX_QPATH];
	char version[MAX_QPATH];
	int parseResult;

	// Open the 'cversion' file
	fileSize = trap_FS_FOpenFile("cversion", &fileHandle, FS_READ);
	if (fileSize <= 0 || !fileHandle)
	{
		trap_Cvar_Set("compilation_build", "");
		return;
	}

	// Read the file contents
	fileContent = Z_Malloc(fileSize + 1);
	OSP_MEMORY_CHECK(fileContent);
	trap_FS_Read(fileContent, fileSize, fileHandle);
	trap_FS_FCloseFile(fileHandle);
	fileContent[fileSize] = 0;

	// Remove newline characters and trailing spaces
	while (fileSize > 0 && (fileContent[fileSize - 1] == '\n' || fileContent[fileSize - 1] == '\r' || fileContent[fileSize - 1] == ' '))
	{
		fileContent[fileSize - 1] = 0;
		fileSize--;
	}

	// Parse the file contents
	parseResult = Q_sscanf(fileContent, "Compilation \"%s\" Version \"%s\"", name, version);

	if (parseResult == 2)
	{
		Com_sprintf(buildString, sizeof(buildString), "%s: %s", name, version);
		trap_Cvar_Set("compilation_build", buildString);
	}
	else
	{
		trap_Cvar_Set("compilation_build", "");
	}

	Z_Free(fileContent);
}

/*
================
CG_ExecGametypeCommand

Execute commands when gametype is set
================
*/
static void CG_ExecGametypeCommand(void)
{
	qboolean isFreeze;
	vmCvar_t* cmdCvar;

	isFreeze = (cgs.gametype == GT_TEAM && cgs.osp.gameTypeFreeze);

	/* Check for freeze tag first */
	if (isFreeze)
	{
		cmdCvar = &cg_exec_freeze;
	}
	else
	{
		switch (cgs.gametype)
		{
			case GT_FFA:
				cmdCvar = &cg_exec_dm;
				break;
			case GT_TOURNAMENT:
				cmdCvar = &cg_exec_tourney;
				break;
			case GT_TEAM:
				cmdCvar = &cg_exec_tdm;
				break;
			case GT_CA:
				cmdCvar = &cg_exec_ca;
				break;
			case GT_CTF:
				cmdCvar = &cg_exec_ctf;
				break;
			default:
				cmdCvar = NULL;
				break;
		}
	}

	if (cmdCvar && cmdCvar->string[0])
	{
		trap_SendConsoleCommand(cmdCvar->string);
	}
}

/*
=================
CG_InitTeamTaskMarkers

Initialize team task markers
=================
*/
void CG_InitTeamTaskMarkers(void)
{
	int i;

	/* numTeamTaskMarks = 0: no iteration over array, memset not needed */
	cg.teamMarkers.numTeamTaskMarks = 0;

	cg.teamMarkers.packRoundNextFrame = qfalse;
	cg.teamMarkers.packLocalDeathNextFrame = qfalse;
	cg.teamMarkers.packHereDeadClientMask0 = 0;
	cg.teamMarkers.packHereDeadClientMask1 = 0;

	cg.teamMarkers.markDurationMs = 2800; /* default; CG_TeamTaskUpdateFrame refreshes from cvar */

	/* Initialize anti-flood token bucket for sending team task commands: fast limit (1 token, refill 1 token per 150ms) */
	TokenBucket_Init(&cg.teamMarkers.sendCommandBucketFast, 1, 150, cg.time);
	/* Initialize anti-flood token bucket for sending team task commands: bucket limit (4 tokens, refill 1 token per 800ms) */
	TokenBucket_Init(&cg.teamMarkers.sendCommandBucket, 10, 600, cg.time);
	/* Initialize anti-flood token bucket for sending team task commands: flood protection (16 tokens, refill 1 token per 1100ms) */
	TokenBucket_Init(&cg.teamMarkers.sendCommandBucketFlood, 64, 1000, cg.time);
	/* Initialize cooldown token bucket for tHere command (1 token, refill 1 token per 2000ms = 2 seconds) */
	TokenBucket_Init(&cg.teamMarkers.sendCommandBucketHere, 1, 2000, cg.time);
	/* Initialize cooldown token bucket for tWarn command (1 token, refill 1 token per 2000ms = 2 seconds) */
	TokenBucket_Init(&cg.teamMarkers.sendCommandBucketWarn, 1, 2000, cg.time);

	/* Initialize death position storage */
	VectorClear(cg.teamMarkers.deathPosition);
	cg.teamMarkers.deathPositionValid = qfalse;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		cg.teamMarkers.allyDeathPositionValid[i] = qfalse;
	}

	/* Initialize pickup item storage */
	cg.teamMarkers.pickupItemType = 0;
	VectorClear(cg.teamMarkers.pickupItemPosition);
	cg.teamMarkers.pickupItemTime = 0;
}

/*
 * Returns qtrue if s equals "RateX7" when Q3 color codes (^x) are skipped.
 */
qboolean CG_AdminNameEqualsRateX7(const char* s)
{
	static const char needle[] = "RateX7";
	const char* n;

	if (!s)
		return qfalse;
	n = needle;
	while (*n)
	{
		while (Q_IsColorString(s))
			s += 2;
		if (!*s || *s != *n)
			return qfalse;
		s++;
		n++;
	}
	return qtrue;
}

/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/
int CG_Init(int serverMessageNum, int serverCommandSequence, int clientNum)
{
	const char*  s;
	int          i;
	// clear everything
	memset(&cgs, 0, sizeof(cgs));
	memset(&cg, 0, sizeof(cg));
	cg.teamMarkerStackCacheTime = -1;
	for (i = 0; i < MAX_CLIENTS; i++)
		cg.predictedDeadInFreeze[i] = -1;
	memset(cg_entities, 0, sizeof(cg_entities));
	memset(cg_weapons, 0, sizeof(cg_weapons));
	memset(cg_items, 0, sizeof(cg_items));

	Com_InitZoneMemory();

	cg.clientNum = clientNum;

	cgs.processedSnapshotNum = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	// load a few needed things before we do any screen updates
	cgs.media.charsetShader1        = trap_R_RegisterShader("gfx/2d/bigchars1");
	cgs.media.charsetShader       = trap_R_RegisterShader("gfx/2d/bigchars");
	cgs.media.whiteShader           = trap_R_RegisterShader("white");

	cgs.media.charsetProp           = trap_R_RegisterShaderNoMip("menu/art/font1_prop.tga");
	cgs.media.charsetPropGlow     = trap_R_RegisterShaderNoMip("menu/art/font1_prop_glo.tga");
	// cgs.media.charsetPropB        = trap_R_RegisterShaderNoMip("menu/art/font2_prop.tga");
	cgs.media.charsetPropBNew        = trap_R_RegisterShaderNoMip("menu/art/font2_propNew.tga");

	CG_RegisterCvars();
	CG_RegisterCvarDescriptions();

	/* restore r_gamma from uc_gamma only when ui_forceGamma 1; when 0 we don't restore */
	{
		char forceGammaStr[8];
		trap_Cvar_VariableStringBuffer("ui_forceGamma", forceGammaStr, sizeof(forceGammaStr));
		if (forceGammaStr[0] == '1')
		{
			char ucGammaStr[16];
			trap_Cvar_VariableStringBuffer("uc_gamma", ucGammaStr, sizeof(ucGammaStr));
			trap_Cvar_Set("r_gamma", (ucGammaStr[0] != '\0') ? ucGammaStr : "1");
		}
	}

	// Parse compilation info from 'cversion' file
	CG_ParseCompilationInfo();

	// Initialize bright weapons colors from default values
	CG_InitBrightWeaponsColors();

	//init variables
	CG_InitCvars();
	CG_InitTeamColors();
	CG_InitBrightItemsColors();
	BeinfoCfg_SetBrightColorHandler(CG_BeinfoApplyBrightColor);
	BeinfoCfg_Load();

	/* Update cached font indices and be_features */
	CG_UpdateFontIndices();
	CG_UpdateBeFeatures();

	CG_InitConsoleCommands();
	CG_BEChat_Init();

	if (cg_clientLog.integer)
	{
		char* tmp = va("client_logs/client%d.txt", cg_clientLog.integer);
		trap_FS_FOpenFile(tmp, &cgs.osp.logFileHandle, FS_APPEND);
	}

	cg.weaponSelect = WP_MACHINEGUN;

	cgs.redflag = cgs.blueflag = -1; // For compatibily, default to unset for
	cgs.flagStatus = -1;
	// old servers

	// get the rendering configuration from the client system
	trap_GetGlconfig(&cgs.glconfig);
	cgs.screenXScale_Old = cgs.glconfig.vidWidth / 640.0;
	cgs.screenYScale_Old = cgs.glconfig.vidHeight / 480.0;

	cgs.screenXBias = 0.0;
	cgs.screenYBias = 0.0;

	if (cgs.glconfig.vidWidth * 480 > cgs.glconfig.vidHeight * 640)
	{
		// wide screen, scale by height
		cgs.screenXScale = cgs.screenYScale = cgs.glconfig.vidHeight * (1.0 / 480.0);
		cgs.screenXBias = 0.5 * (cgs.glconfig.vidWidth - (cgs.glconfig.vidHeight * (640.0 / 480.0)));
	}
	else
	{
		// no wide screen, scale by width
		cgs.screenXScale = cgs.screenYScale = cgs.glconfig.vidWidth * (1.0 / 640.0);
		cgs.screenYBias = 0.5 * (cgs.glconfig.vidHeight - (cgs.glconfig.vidWidth * (480.0 / 640.0)));
	}

	cgs.screenXmin = 0.0 - (cgs.screenXBias / cgs.screenXScale);
	cgs.screenXmax = 640.0 + (cgs.screenXBias / cgs.screenXScale);

	cgs.screenYmin = 0.0 - (cgs.screenYBias / cgs.screenYScale);
	cgs.screenYmax = 480.0 + (cgs.screenYBias / cgs.screenYScale);

	// init fonts
	CG_LoadFonts();

	// get the gamestate from the client system
	trap_GetGameState(&cgs.gameState);

	// check version
	s = CG_ConfigString(CS_GAME_VERSION);
	if (strcmp(s, GAME_VERSION))
	{
		CG_Error("Client/Server game mismatch: %s/%s", GAME_VERSION, s);
	}

// Different constants for different OSP versions; probably should keep only one variant
	cgs.osp.stats_mask = 0xffff;
	cgs.osp.stats_shift = 0x10;

	s = CG_ConfigString(CS_LEVEL_START_TIME);
	cgs.levelStartTime = atoi(s);

	CG_ParseServerinfo();
	/* fpsclassico, q3msk known at connect - inline, run once */
	{
		const char* serverStr;
		const char* adminPrefix;
		serverStr = CG_ConfigString(CS_SERVERINFO);
		if (strstr(serverStr, "administrator") || strstr(serverStr, "adminless"))
		{
			cgs.fpsclassico = qtrue;
		}
		adminPrefix = strstr(serverStr, ".Admin\\");
		if (strstr(serverStr, ".Admin\\X") || (adminPrefix && CG_AdminNameEqualsRateX7(adminPrefix + 7)))
		{
			cgs.q3msk = qtrue;
		}
	}
	/* teamoverlay - fpsclassico known at connect, run once; only in team gametypes */
	if (cgs.gametype >= GT_TEAM)
	{
		if (cgs.fpsclassico)
		{
			if (!teamoverlay2_registered)
			{
				trap_Cvar_Register(&teamoverlay2, "teamoverlay2", "1", CVAR_USERINFO | CVAR_ROM | CVAR_NOTABCOMPLETE);
				teamoverlay2_registered = qtrue;
			}
			trap_Cvar_Set("teamoverlay2", "1");
			trap_Cvar_Update(&teamoverlay2);
		}
		else
		{
			if (!teamoverlay_registered)
			{
				trap_Cvar_Register(&teamoverlay, "teamoverlay", "1", CVAR_USERINFO | CVAR_ROM);
				teamoverlay_registered = qtrue;
			}
			trap_Cvar_Set("teamoverlay", "1");
			trap_Cvar_Update(&teamoverlay);
		}
	}

	CG_Printf("\n^7Loaded [^9OSP2-BE^7] client version: ^9%s\n", OSP_VERSION);

	CG_OSPPrintTime_f();

	CG_Printf("------------------------------------------------\n");

	/* Init OSP */
	{
		int i;
		char tmp_str[1024];
		char* tmp_ptr;
		const char* conf;

		CG_OSPConfigModeSet(atoi(CG_ConfigString(CS_OSP_SERVER_MODE)));

		conf = CG_ConfigString(CS_OSP_CUSTOM_CLIENT);
		if (conf)//-V547
		{
			CG_OSPConfigCustomClientSet(atoi(conf));
		}
		else
		{
			CG_OSPConfigCustomClientSet(CS_OSP_CUSTOM_CLIENT_DEFAULT);
		}

		conf = CG_ConfigString(CS_OSP_CUSTOM_CLIENT2);
		if (conf)//-V547
		{
			CG_OSPConfigCustomClient2Set(atoi(conf));
		}
		else
		{
			CG_OSPConfigCustomClient2Set(CS_OSP_CUSTOM_CLIENT2_DEFAULT);
		}

		CG_OSPConfig0x368Set(atoi(CG_ConfigString(CS_OSP_AUTH)) & 1);
		CG_OSPConfigPmoveSet(atoi(CG_ConfigString(CS_OSP_ALLOW_PMOVE)));

		CG_OSPConfigMinMaxPacketsSet(atoi(CG_ConfigString(CS_OSP_MAXPACKETS_MIN)));
		CG_OSPConfigMaxMaxPacketsSet(atoi(CG_ConfigString(CS_OSP_MAXPACKETS_MAX)));
		CG_OSPConfigMinTimenudgeSet(atoi(CG_ConfigString(CS_OSP_TIMENUDGE_MIN)));
		CG_OSPConfigMaxTimenudgeSet(atoi(CG_ConfigString(CS_OSP_TIMENUDGE_MAX)));
		CG_OSPConfigClanBaseTDMSet(atoi(CG_ConfigString(CS_OSP_CLAN_BASE_TEAM_DM)));
		if (!cgs.fpsclassico)
		{
			CG_OSPConfigFreezeModeSet(atoi(CG_ConfigString(CS_OSP_FREEZE_GAME_TYPE)));
		}
		else
		{
			if (strstr(CG_ConfigString(CS_SERVERINFO), "gamename\\unfreeze"))
			{
				cgs.osp.gameTypeFreeze = 1;
			}
		}
		// Every server is supported
		// CG_OSPSupportedBEServer(atoi(CG_ConfigString(CS_OSP2BE_SUPPORTED)));

		CG_OSPConfigDisableBEFeatures(atoi(CG_ConfigString(CS_OSP2BE_DISABLED_FEATURES)));
		s = CG_ConfigString(CS_OSP2BE_SPAWN_ITEMS);
		cgs.beSpawnItems = atoi(s);
		cgs.beSpawnItemsKnown = (s[0] != '\0') ? qtrue : qfalse;

		/* cache damageInfoAllowed - gametype, fpsclassico, custom_client_2, beDisableFeatures all known */
		CG_UpdateDamageInfoAllowed();

		/****/
		CG_OSPCvarsRestrictValues();

		/*Check maps for buggy fog*/
		CG_CheckFogBypass();

		/* MOTD */
		for (i = 0, cgs.osp.numberOfStringsMotd = 0;
		        i < 8;
		        ++i, ++cgs.osp.numberOfStringsMotd)
		{
			s = CG_ConfigString(CS_OSP_MOTD0 + i);
			if (s[0] == 0) break;
			Q_strncpyz(&cgs.osp.motd[cgs.osp.numberOfStringsMotd][0], s, 64);
		}

		/****/
		cgs.osp.custom_gfx_number = 0;
		for (i = 0; i < 16; ++i)
		{
			s = CG_ConfigString(CS_OSP_CUSTOM_GFX + i);
			if (s[0] == 0) break;
			Q_strncpyz(tmp_str, s, 1024);

			tmp_ptr = strchr(tmp_str, ':');
			if (tmp_ptr == NULL)
			{
				CG_Printf("Malformed custom gfx entry: %s\n", tmp_str);
				break;
			}
			*tmp_ptr = 0;
			++tmp_ptr;
			Q_sscanf(tmp_str, "%i %i %i %i",
			         &cgs.osp.custom_gfx[cgs.osp.custom_gfx_number][0],
			         &cgs.osp.custom_gfx[cgs.osp.custom_gfx_number][1],
			         &cgs.osp.custom_gfx[cgs.osp.custom_gfx_number][2],
			         &cgs.osp.custom_gfx[cgs.osp.custom_gfx_number][3]
			        );

			Q_sscanf(tmp_ptr, "%i %i %i %i",
			         &cgs.osp.custom_gfx[cgs.osp.custom_gfx_number][4],
			         &cgs.osp.custom_gfx[cgs.osp.custom_gfx_number][5],
			         &cgs.osp.custom_gfx[cgs.osp.custom_gfx_number][6],
			         &cgs.osp.custom_gfx[cgs.osp.custom_gfx_number][7]
			        );

			if ((cgs.osp.custom_gfx[cgs.osp.custom_gfx_number][0] == -1) ||
			        (cgs.osp.custom_gfx[cgs.osp.custom_gfx_number][1] == -1))
			{
				break;
			}
			++cgs.osp.custom_gfx_number;
		}
		CG_Printf("\n>>> %d custom graphics specified\n\n", cgs.osp.custom_gfx_number);


		CG_RebuildPlayerColors();
	}

	// load the new map
	CG_LoadingString("collision map");

	trap_CM_LoadMap(cgs.mapname);

	cg.loading = qtrue;     // force players to load instead of defer

	CG_LoadingString("sounds");
	CG_RegisterSounds();

	CG_LoadingString("graphics");
	CG_RegisterGraphics();

	CG_UpdateCvars(cvarTableSize);

	if (cg_chatWindow.integer)
	{
		CG_LoadingString("emoji");
		CG_OSPLoadEmoji();
	}

	/* CG_OSP2BEConfigTourneyModeRefresh(); */
	if (cg_execVstr.string[0])
	{
		trap_SendConsoleCommand(cg_execVstr.string);
	}
	if (cg_exec_gametypes.integer != 0)
	{
		CG_ExecGametypeCommand();
	}

	CG_LoadingString("clients");
	CG_RegisterClients();       // if low on memory, some clients will be deferred

	cg.loading = qfalse;    // future players will be deferred

	// Force update player colors if color cvars contain 'T' or 'E'
	CG_InitPlayerColors();

	CG_InitLocalEntities();

	CG_InitMarkPolys();

	CG_InitTeamTaskMarkers();

	CG_InitCTFLocations();

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_StartMusic();

	CG_LoadingString("");

	CG_ShaderStateChanged();

	trap_S_ClearLoopingSounds(qtrue);

	if (CG_BE_FEATURE_ENABLED(CG_BE_MODELSOUND))
	{
		CG_LoadForcedSounds();
	}

	CG_CustomLocationsLoad();
	CG_MapBundle_LoadAllMapData();
	CG_MuteWorldSoundCacheInvalidate(0);

	cgs.osp.decals_number = 0;
	if (cg_drawDecals.integer)
	{
		int i;
		VectorClear(cgs.osp.rail_color);

		for (i = 0; i < 32; ++i)
		{
			cgs.osp.decals[0].v3 = 0;
			cgs.osp.decals[0].v1 = 0;
			cgs.osp.decals[0].v4 = 0;
			cgs.osp.decals[0].v5 = 0;
			cgs.osp.decals[0].v2 = 30;

			cgs.osp.decals[i].v11 = 0;
			cgs.osp.decals[i].v10 = 0;
			cgs.osp.decals[i].v9 = 0;

			cgs.osp.decals[i].v8 = 0;
			cgs.osp.decals[i].v7 = 0;
			cgs.osp.decals[i].v6 = 0;

			s = CG_ConfigString(CS_OSP_DECALS + i);
			if (s[0] == 0) break;

			Q_sscanf(s, "%i %i %f %f %f %f %f %f %f",
			         &cgs.osp.decals[cgs.osp.decals_number].v3,
			         &cgs.osp.decals[cgs.osp.decals_number].v2,
			         &cgs.osp.decals[cgs.osp.decals_number].v5,
			         &cgs.osp.decals[cgs.osp.decals_number].v9,
			         &cgs.osp.decals[cgs.osp.decals_number].v10,
			         &cgs.osp.decals[cgs.osp.decals_number].v11,
			         &cgs.osp.decals[cgs.osp.decals_number].v6,
			         &cgs.osp.decals[cgs.osp.decals_number].v7,
			         &cgs.osp.decals[cgs.osp.decals_number].v8);

			if (i && cgs.osp.decals[0].v3 == -1)
			{
				/* looks like bug */
				CG_Printf("*** Decals disabled\n");
				continue;
			}
			if (cgs.osp.decals[i].v2 == 0)
			{
				continue;
			}
			if (cgs.osp.decals[cgs.osp.decals_number].v3 < cgs.osp.decalOSPShaderCopyCount)
			{
				int tmp;
				tmp = cgs.osp.decals[cgs.osp.decals_number].v3;
				cgs.osp.decals[cgs.osp.decals_number].v1 = 1;

				if (tmp > 800) tmp = 800;
				if (tmp < 10) tmp = 10;

				cgs.osp.decals[cgs.osp.decals_number].v2 = tmp;
				++cgs.osp.decals_number;
			}
			else
			{
				CG_Printf("^3** Illegal decal graphic %d referenced ... skipping\n", cgs.osp.decals[cgs.osp.decals_number].v3);
			}
		}
	}

	CG_ChatfilterLoadFile(CG_CHATFILTER_DEFAULT_FILE);
	CG_TeamTaskFilterLoadFile(CG_TEAMTASKFILTER_DEFAULT_FILE);
	CG_NotifyTeamTaskServerSupportOnMapLoad();
	if (cg_clearOnLevelLoad.integer)
	{
		trap_SendConsoleCommand("clear");
	}
	return 0;
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown(void)
{
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data
}


/*
==================
CG_EventHandling
==================
 type 0 - no event handling
      1 - team menu
      2 - hud editor

*/
void CG_EventHandling(int type)
{
	CG_BEChat_EventHandling(type);
}



void CG_KeyEvent(int key, qboolean down)
{
	CG_BEChat_KeyEvent(key, down);
}

void CG_MouseEvent(int x, int y)
{
	CG_BEChat_MouseEvent(x, y);
}

char* CG_OSPGetCvarName(vmCvar_t* cvar)
{
	cvarTable_t* cv;
	int i;

	for (i = 0, cv = &cvarTable[0]; i < cvarTableSize; ++i)
	{
		if (cv->vmCvar == cvar)
		{
			return cv->cvarName;
		}
	}
	return NULL;
}

void CG_PrintNewCommandsBE_f(void)
{
	int i;
	int startIndex = -1;
	int numCommands = sizeof(cvarTable) / sizeof(cvarTable[0]);
	int commandNum = 0;
	int requestedNum = -1;
	int argc;
	char arg[MAX_TOKEN_CHARS];
	int foundIndex = -1;

	argc = trap_Argc();
	if (argc >= 2)
	{
		Q_strncpyz(arg, CG_Argv(1), sizeof(arg));
		requestedNum = atoi(arg);
	}

	// start from cg_uniqueColorTable (BE cvar block; do not use cg_enemyLightningColor — it moved in the table)
	for (i = 0; i < numCommands; i++)
	{
		if (!strcmp(cvarTable[i].cvarName, "cg_uniqueColorTable"))
		{
			startIndex = i;
			break;
		}
	}

	if (startIndex < 0)
	{
		CG_Printf("cg_uniqueColorTable not found\n");
		return;
	}

	// If a number was provided, find and execute that command
	if (requestedNum > 0)
	{
		commandNum = 0;
		for (i = startIndex; i < numCommands; i++)
		{
			// end of list
			if (!strcmp(cvarTable[i].cvarName, "be_run"))
			{
				break;
			}
			// Skip commands with CVAR_NOTABCOMPLETE flag
			if (cvarTable[i].cvarFlags & CVAR_NOTABCOMPLETE)
			{
				continue;
			}
			commandNum++;
			if (commandNum == requestedNum)
			{
				foundIndex = i;
				break;
			}
		}

		if (foundIndex >= 0)
		{
			const char* cvarName = cvarTable[foundIndex].cvarName;
			const char* description = CG_GetCvarDescription(cvarName);
			char currentValue[MAX_CVAR_VALUE_STRING];
			const char* defaultValue;

			/* Get current value */
			trap_Cvar_VariableStringBuffer(cvarName, currentValue, sizeof(currentValue));

			/* Get default value */
			if (cvarTable[foundIndex].defaultString)
			{
				defaultValue = cvarTable[foundIndex].defaultString;
			}
			else
			{
				defaultValue = "";
			}

			/* Print cvar info */
			CG_Printf("\"%s\" is \"%s\" default:\"%s\"\n", cvarName, currentValue, defaultValue);

			/* Print description if found */
			if (description && description[0])
			{
				CG_Printf("%s\n", description);
			}
			else
			{
				CG_Printf("^3No description available\n");
			}
			return;
		}
		else
		{
			CG_Printf("^1Error: Command number %d not found\n", requestedNum);
			return;
		}
	}

	// Print the list with numbers
	commandNum = 0;
	for (i = startIndex; i < numCommands; i++)
	{
		// end of list
		if (!strcmp(cvarTable[i].cvarName, "be_run"))
		{
			break;
		}

		// Skip commands with CVAR_NOTABCOMPLETE flag
		if (cvarTable[i].cvarFlags & CVAR_NOTABCOMPLETE)
		{
			continue;
		}

		commandNum++;

		if (cvarTable[i].cvarFlags & CVAR_NEW)
		{
			CG_Printf("^2+ %d. %s\n", commandNum, cvarTable[i].cvarName);
		}
		else if (cvarTable[i].cvarFlags & CVAR_UPDATED)
		{
			CG_Printf("^3* %d. %s\n", commandNum, cvarTable[i].cvarName);
		}
		else
		{
			CG_Printf("  %d. %s\n", commandNum, cvarTable[i].cvarName);
		}
	}
}

#ifdef __GNUC__
__attribute__((unused))
#endif
static void trap_Cvar_SetDescription_local(const char* name, const char* description)
{
	(void)name;
	(void)description;
}
typedef void (setCvarDescription_t)(const char* name, const char* description);
typedef qboolean(trap_GetValue_t)(char* value, int valueSize, const char* key);
typedef qboolean(trap_IsRecordingDemo_t)(void);
typedef qboolean(trap_Key_CapsLockOn_t)(void);


// [meta]:
// I have splitted this func for VM and non-VM ( native code ), 'cause
// under native build ( non-vm ) it causes segmentation fault. The
// reason is `addr` var: it's used as function address, but function
// address is valid only for VM build, so it crashes. Call through
// syscall instead should handle it correctly.

// The code differs too much to place it in one function.

#ifdef Q3_VM

void CG_SetCvarDescription(const char* name, const char* description)
{
	static setCvarDescription_t* setDescription = NULL;

	if (!setDescription)
	{
		int addr;
		char  value[MAX_CVAR_VALUE_STRING];
		trap_GetValue_t* trap_GetValue;

		setDescription = trap_Cvar_SetDescription_local;
		trap_Cvar_VariableStringBuffer("//trap_GetValue", value, sizeof(value));

		if (value[0])
		{
			addr = atoi(value);

			addr = ~addr;

			trap_GetValue = (trap_GetValue_t*)(addr);

			if (trap_GetValue(value, sizeof(value), "trap_Cvar_SetDescription_Q3E"))
			{
				addr = atoi(value);

				addr = ~addr;

				setDescription = (setCvarDescription_t*)addr;
			}
		}
	}

	setDescription(name, description);
}

#else

void CG_SetCvarDescription(const char* name, const char* description)
{
	static int setDescriptionFuncId = 0;

	if (!setDescriptionFuncId)
	{
		int getValueFuncId;
		char  value[MAX_CVAR_VALUE_STRING];

		// -1 treats as dummy function
		setDescriptionFuncId = -1;

		trap_Cvar_VariableStringBuffer("//trap_GetValue", value, sizeof(value));

		if (value[0])
		{
			getValueFuncId = atoi(value);

			if (trap_CG_GetValue_Q3E(getValueFuncId, value, sizeof(value), "trap_Cvar_SetDescription_Q3E"))
			{
				setDescriptionFuncId = atoi(value);
			}
		}
	}

	if (setDescriptionFuncId == -1) return;

	trap_CG_SetDescription_Q3E((int)setDescriptionFuncId, name, description);
}

#endif


qboolean CG_IsRecordingDemo(void)
{
#ifdef Q3_VM
	static trap_IsRecordingDemo_t* isRecordingDemo = NULL;

	if (!isRecordingDemo)
	{
		int addr;
		char value[MAX_CVAR_VALUE_STRING];
		trap_GetValue_t* trap_GetValue;

		trap_Cvar_VariableStringBuffer("//trap_GetValue", value, sizeof(value));

		if (value[0])
		{
			addr = atoi(value);
			addr = ~addr;
			trap_GetValue = (trap_GetValue_t*)(addr);

			if (trap_GetValue(value, sizeof(value), "trap_IsRecordingDemo"))
			{
				addr = atoi(value);
				addr = ~addr;
				isRecordingDemo = (trap_IsRecordingDemo_t*)addr;
			}
		}
	}

	if (!isRecordingDemo)
	{
		return qfalse;
	}

	return isRecordingDemo();
#else
	static int isRecordingDemoFuncId = 0;
	int getValueFuncId;
	char value[MAX_CVAR_VALUE_STRING];

	if (!isRecordingDemoFuncId)
	{
		isRecordingDemoFuncId = -1;
		trap_Cvar_VariableStringBuffer("//trap_GetValue", value, sizeof(value));

		if (value[0])
		{
			getValueFuncId = atoi(value);

			if (trap_CG_GetValue_Q3E(getValueFuncId, value, sizeof(value), "trap_IsRecordingDemo"))
			{
				isRecordingDemoFuncId = atoi(value);
			}
		}
	}

	if (isRecordingDemoFuncId == -1)
	{
		return qfalse;
	}

	return (qboolean)trap_IsRecordingDemo_Q3E(isRecordingDemoFuncId);
#endif
}

qboolean cg_keyCapsLockOnAvail;

qboolean CG_KeyCapsLockOn(void)
{
#ifdef Q3_VM
	static trap_Key_CapsLockOn_t* keyCapsLockOn = NULL;

	if (!keyCapsLockOn)
	{
		int addr;
		char value[MAX_CVAR_VALUE_STRING];
		trap_GetValue_t* trap_GetValue;

		trap_Cvar_VariableStringBuffer("//trap_GetValue", value, sizeof(value));

		if (value[0])
		{
			addr = atoi(value);
			addr = ~addr;
			trap_GetValue = (trap_GetValue_t*)(addr);

			if (trap_GetValue(value, sizeof(value), "trap_Key_CapsLockOn_Q3E"))
			{
				addr = atoi(value);
				addr = ~addr;
				keyCapsLockOn = (trap_Key_CapsLockOn_t*)addr;
				cg_keyCapsLockOnAvail = qtrue;
			}
		}
	}

	if (!keyCapsLockOn)
	{
		return qfalse;
	}

	return keyCapsLockOn();
#else
	static int keyCapsLockOnFuncId = 0;

	if (!keyCapsLockOnFuncId)
	{
		int getValueFuncId;
		char value[MAX_CVAR_VALUE_STRING];

		keyCapsLockOnFuncId = -1;
		trap_Cvar_VariableStringBuffer("//trap_GetValue", value, sizeof(value));

		if (value[0])
		{
			getValueFuncId = atoi(value);

			if (trap_CG_GetValue_Q3E(getValueFuncId, value, sizeof(value), "trap_Key_CapsLockOn_Q3E"))
			{
				keyCapsLockOnFuncId = atoi(value);
				cg_keyCapsLockOnAvail = qtrue;
			}
		}
	}

	if (keyCapsLockOnFuncId == -1)
	{
		return qfalse;
	}

	return (qboolean)trap_Key_CapsLockOn_Q3E(keyCapsLockOnFuncId);
#endif
}
