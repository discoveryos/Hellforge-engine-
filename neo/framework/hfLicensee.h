/*
===========================================================================

        Hellforge Engine - Cross-platform game engine
        Powered by Hellforge Power 1
        Copyright (C) 1993-2026

        Definitions for engine identity and game configuration.

===========================================================================
*/

#ifndef __LICENSEE_H__
#define __LICENSEE_H__

#define GAME_NAME                       "Hellforge Engine"          // appears on window titles and errors
#define SAVE_PATH                       "/hellforge"

#define ENGINE_NAME                     "Hellforge"
#define ENGINE_VERSION                  "1.0.0"                     // printed in console
#define ENGINE_BUILD                    "Hellforge 1.0.0"

#define BASE_GAMEDIR                    "base"

#define CONFIG_FILE                     "hellforgeConfig.cfg"
#define COMMAND_HISTORY_FILE            "hellforgeHistory.cfg"
#define SINGLE_PLAYER_SETTINGS_FILE     "hellforgeSinglePlayerSettings.cfg"
#define MULTIPLAYER_SETTINGS_FILE       "hellforgeMultiplayerSettings.cfg"

// see ASYNC_PROTOCOL_VERSION
// use a different major for each game
#define ASYNC_PROTOCOL_MAJOR            1

// render demo version
#define RENDERDEMO_VERSION              3

// platform info
#define PLATFORM_WINDOW_CLASS_NAME      "HellforgeWin"
#define PLATFORM_CONSOLE_CLASS          "HellforgeConsole"
#define PLATFORM_FAKE_WINDOW_CLASS_NAME "HellforgeWGL_FAKE"

#endif /* !__LICENSEE_H__ */