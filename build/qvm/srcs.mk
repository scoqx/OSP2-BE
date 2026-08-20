QA_SRC = \
 g_main $(QADIR)/g_syscalls.asm \
 bg_misc bg_lib bg_pmove bg_slidemove \
 q_math q_shared \
 ai_dmnet ai_dmq3 ai_team ai_main ai_chat ai_cmd ai_vcmd \
 g_active g_arenas g_bot g_client g_cmds g_combat g_items g_mem g_misc \
 g_missile g_mover g_rotation g_session g_spawn g_svcmds g_target g_team \
 g_trigger g_unlagged g_utils g_weapon \

CG_SRC = \
 cg_main $(CGDIR)/cg_syscalls.asm \
 cg_consolecmds cg_draw cg_drawtools cg_ospdrawstring cg_dyeicons cg_effects cg_ents cg_event cg_info \
 cg_localents cg_players cg_playerstate cg_predict cg_scoreboard cg_marks cg_medals \
 cg_servercmds cg_snapshot cg_view cg_weapons cg_printui cg_teamtasks_utils cg_teamtasks_draw cg_teamtasks_selwheel \
 bg_slidemove bg_pmove bg_lib bg_misc \
 core slidemove q3a vq1 vjk vq4 \
 q_math q_shared cg_ospconfig cg_osputil cg_osphud cg_unlagged cg_playerscolors\
 cg_localevents cg_customloc cg_maplocs cg_mapbundle cg_spawns cg_shadowitems cg_killconfirm cg_spectatoritems cg_tpexit cg_globalpos cg_worldflags cg_cvardescriptions common\
 superhud/superhud superhud/superhud_configparser superhud/superhud_util superhud/superhud_private\
 superhud/editor/shud_editor superhud/editor/shud_editor_select \
 superhud/editor/shud_editor_props superhud/editor/shud_editor_input \
 superhud/editor/shud_editor_draw \
 superhud/superhud_serialize\
 superhud/elements/element_fps \
 superhud/elements/element_sbhc superhud/elements/element_sbhi superhud/elements/element_sbhb \
 superhud/elements/element_sbac superhud/elements/element_sbai superhud/elements/element_sbab \
 superhud/elements/element_sbamc superhud/elements/element_sbami superhud/elements/element_sbamb \
 superhud/elements/element_target_name superhud/elements/element_target_status \
 superhud/elements/element_vmw \
 superhud/elements/element_attacker_name superhud/elements/element_attacker_icon \
 superhud/elements/element_fragmessage \
 superhud/elements/element_rankmessage \
 superhud/elements/element_ng superhud/elements/element_ngp \
 superhud/elements/element_pred \
 superhud/elements/element_speed \
 superhud/elements/element_localtime superhud/elements/element_gametime \
 superhud/elements/element_ammomessage \
 superhud/elements/element_chat \
 superhud/elements/element_specmessage superhud/elements/element_followmessage \
 superhud/elements/element_respawnmessage \
 superhud/elements/element_spectators \
 superhud/elements/element_itempickup superhud/elements/element_itempickupicon \
 superhud/elements/element_spectator_itemspawns superhud/elements/element_spectator_itempickups \
 superhud/elements/element_teamcount \
 superhud/elements/element_flagstatus \
 superhud/elements/element_powerup \
 superhud/elements/element_name \
 superhud/elements/element_score \
 superhud/elements/element_reward \
 superhud/elements/element_recordmessage \
 superhud/elements/element_team \
 superhud/elements/element_weaponlist \
 superhud/elements/element_obituaries \
 superhud/elements/element_warmupinfo superhud/elements/element_gametype superhud/elements/element_location \
 superhud/elements/element_tempAcc \
 superhud/elements/element_weapon_stats superhud/elements/element_player_stats superhud/elements/element_player_name \
 superhud/elements/element_grid \
 superhud/elements/element_teamtasks \
 superhud/elements/element_roundwarmup \
 superhud/elements/element_laststanding \
 superhud/elements/element_timelimit \
 superhud/elements/element_thaw \
 superhud/elements/element_killconfirm \
 superhud/elements/element_centermessage \
 l_crc \
 cg_chatfilter cg_crosshair \
 cg_be_util cg_be_auth cg_be_autologin cg_be_stats cg_round_stats \
 chatWindow/be_chatwindow chatWindow/be_chatwindow_state chatWindow/be_chatwindow_persist chatWindow/be_chatwindow_cmdac \
 chatWindow/be_admincmds chatWindow/be_chatwindow_history chatWindow/be_chatwindow_input \
 chatWindow/be_chatwindow_emoji chatWindow/be_chatwindow_emoji_picker \
 cg_brightitems beinfo_cfg osp_fonts engine_fonts be_font_shadow \
 cg_emoji
UI_SRC = \
 ui_main $(UIDIR)/ui_syscalls.asm \
ui_gameinfo beinfo_cfg osp_fonts engine_fonts be_font_shadow ui_atoms ui_cvardescriptions ui_colorparse ui_cinematics ui_connect ui_controls2 ui_demo2 \
ui_md5 ui_status_sign ui_server_status ui_fav_server_parse ui_favourite_server \
ui_mfield ui_credits ui_credits2 ui_menu ui_ingame ui_confirm ui_setup ui_options \
ui_display ui_sound ui_network ui_playermodel ui_players ui_playersettings \
ui_preferences ui_qmenu ui_serverinfo ui_servers2 ui_sparena ui_colorpicker \
ui_superhudconfig ui_ospdrawtools ui_advanced \
ui_specifyserver ui_sppostgame ui_splevel ui_spskill ui_startserver ui_team \
ui_video ui_addbots ui_removebots ui_teamorders ui_loadconfig ui_saveconfig \
ui_cdkey ui_mods \
ui_votemenu ui_votemenu_map ui_votemenu_kick ui_votemenu_gametype \
ui_votemenu_fraglimit ui_votemenu_timelimit ui_votemenu_custom \
 bg_misc bg_lib \
 q_math q_shared \
