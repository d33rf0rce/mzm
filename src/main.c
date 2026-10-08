#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include <SDL3/SDL.h>

#include "syscalls.h"
#include "data/generic_data.h"
#include "data/menus/language_select_data.h"

#include "gba.h"
#include "menus/erase_sram.h"
#include "init_game.h"
#include "soft_reset_input.h"
#include "audio/track_internal.h"
#include "constants/game_state.h"

#include "structs/cutscene.h"
#include "structs/demo.h"
#include "structs/game_state.h"
#include "structs/display.h"

#include "intro.h"
#include "in_game.h"
#include "demo.h"
#include "soft_reset.h"
#include "chozodia_escape.h"
#include "menus/title_screen.h"
#include "menus/file_select.h"
#include "menus/pause_screen.h"
#include "menus/game_over.h"
#include "cutscenes/cutscene_utils.h"
#include "ending_and_gallery.h"
#include "fusion_gallery.h"

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    InitializeGame();
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    gVblankActive = FALSE;
    // UpdateAudio()

    if (gResetGame)
        return SDL_APP_SUCCESS;

    // UpdateInput()
    // SoftResetCheck()

    APPLY_DELTA_TIME_INC(gFrameCounter8Bit);
    APPLY_DELTA_TIME_INC(gFrameCounter16Bit);

    switch (gMainGameMode) {
        case GM_SOFT_RESET:
            if (SoftResetHandler()) {
                gMainGameMode = GM_INTRO;
                gSubGameMode1 = 0;
            }
            break;

        case GM_INTRO:
            if (IntroHandler()) {
                gMainGameMode = GM_TITLE;
                gSubGameMode1 = 0;
            }
            break;

        case GM_TITLE:
            if (TitleScreenHandler()) {
                if (gSubGameMode2 == 1) {
                    gMainGameMode = GM_FILE_SELECT;
                } else if (gSubGameMode2 == 2) {
                    DemoStart();
                    gMainGameMode = GM_DEMO;
                } else {
                    gMainGameMode = GM_INTRO;
                }

                gSubGameMode1 = 0;
                gPauseScreenFlag = 0;
                gSubGameMode2 = 0;
            }
            break;

        case GM_FILE_SELECT:
            if (FileSelectMenuHandler())
            {
                if (gSubGameMode2 == 1) // If continuing file
                    gMainGameMode = GM_INGAME;
                else if (gSubGameMode2 == 2) // If starting new file
                    gMainGameMode = GM_INGAME;
                else if (gSubGameMode2 == 4)
                    gMainGameMode = GM_FUSION_GALLERY;
                else if (gSubGameMode2 == 5)
                    gMainGameMode = GM_GALLERY;
                else
                    gMainGameMode = GM_INTRO;

                gSubGameMode1 = 0;
                gSubGameMode3 = 0;
                gSubGameMode2 = 0;
            }
            break;

        case GM_INGAME:
                if (InGameHandler())
                {
                    if (gPauseScreenFlag == PAUSE_SCREEN_NONE)
                    {
                        if (gCurrentCutscene != 0)
                        {
                            gMainGameMode = GM_CUTSCENE;
                        }
                        else if (gTourianEscapeCutsceneStage != 0)
                        {
                            gMainGameMode = GM_TOURIAN_ESCAPE;
                        }
                        else
                        {
#ifdef DEBUG
                            gMainGameMode = GM_DEBUG_MENU;
#else // !DEBUG
                            gMainGameMode = GM_TITLE;
#endif // DEBUG
                            gSubGameMode1 = 0;
                        }
                    }
                    else
                    {
                        gMainGameMode = GM_MAP_SCREEN;
                    }
                }
                break;
                 case GM_MAP_SCREEN:
                if (PauseScreenHandler())
                {
                    gMainGameMode = gSubGameMode2;
                    gSubGameMode2 = 0;

                    switch (gPauseScreenFlag)
                    {
                        case PAUSE_SCREEN_UNKNOWN_1:
                            gSubGameMode3 = 0;

                        case PAUSE_SCREEN_SUITLESS_ITEMS:
                            gPauseScreenFlag = PAUSE_SCREEN_NONE;
                            break;

                        case PAUSE_SCREEN_UNKNOWN_9:
                            gPauseScreenFlag = PAUSE_SCREEN_NONE;
                            gSubGameMode2 = 1;
                            break;

                        case PAUSE_SCREEN_PAUSE_OR_CUTSCENE:
                        case PAUSE_SCREEN_UNKNOWN_3:
                        case PAUSE_SCREEN_CHOZO_HINT:
                        case PAUSE_SCREEN_MAP_DOWNLOAD:
                        case PAUSE_SCREEN_FULLY_POWERED_SUIT_ITEMS:
                            break;
                    }

                    gSubGameMode1 = 0;
                }
                break;
                case GM_GAMEOVER:
                if (GameOverHandler())
                {
                    gMainGameMode = gSubGameMode2;
                    gSubGameMode1 = 0;
                    gSubGameMode2 = 0;
                }
                break;

            case GM_CHOZODIA_ESCAPE:
                if (ChozodiaEscapeHandler())
                {
                    gSubGameMode1 = 0;
                    gMainGameMode = GM_CREDITS;
                }
                break;

            case GM_CREDITS:
                if (CreditsHandler())
                {
                    gSubGameMode1 = 0;
                    gMainGameMode = GM_INTRO;
#ifdef DEBUG
                    if (gBootDebugActive || gDebugMode)
                        gMainGameMode = GM_DEBUG_MENU;
#endif // DEBUG
                }
                break;

            case GM_TOURIAN_ESCAPE:
                if (TourianEscapeHandler())
                {
                    gSubGameMode1 = 0;
                    gMainGameMode = gSubGameMode2;
#ifdef DEBUG
                    if (gBootDebugActive)
                        gMainGameMode = GM_DEBUG_MENU;
#endif // DEBUG
                }
                break;

            case GM_CUTSCENE:
                if (CutsceneHandler())
                {
                    gSubGameMode1 = 0;

                    if (gPauseScreenFlag == PAUSE_SCREEN_SUITLESS_ITEMS || gPauseScreenFlag == PAUSE_SCREEN_FULLY_POWERED_SUIT_ITEMS)
                    {
                        gMainGameMode = GM_MAP_SCREEN;
                    }
                    else
                    {
                        gMainGameMode = GM_INGAME;
#ifdef DEBUG
                        if (gBootDebugActive)
                            gMainGameMode = gBootDebugActive;
#endif // DEBUG
                    }
                }
                break;

            case GM_DEMO:
                if (InGameHandler())
                {
                    if (gPauseScreenFlag == PAUSE_SCREEN_PAUSE_OR_CUTSCENE)
                    {
                        gPauseScreenFlag = PAUSE_SCREEN_NONE;
                        gSubGameMode3 = 0;
                        gSubGameMode1 = 0;
                        if (gDemoState == 0)
                        {
                            gMainGameMode = gSubGameMode2;
                            gSubGameMode2 = gCurrentDemo.endedWithInput;
                        }
                        else {
                            DemoStart();
                            gMainGameMode = GM_DEMO;
                        }
                    }
                    else
                        gMainGameMode = GM_MAP_SCREEN;
                }
                break;

            case GM_GALLERY:
                if (GalleryHandler())
                {
                    gSubGameMode1 = 0;
                    gMainGameMode = GM_FILE_SELECT;
                }
                break;

            case GM_FUSION_GALLERY:
                if (FusionGalleryHandler())
                {
                    gSubGameMode1 = 0;
                    gMainGameMode = GM_FILE_SELECT;
                }
                break;

            case GM_START_SOFT_RESET:
                SoftReset();
                break;

            case GM_ERASE_SRAM:
                if (EraseSramHandler())
                {
                    if (gSubGameMode2 == 1)
                    {
                        gResetGame = TRUE;
                    }
                    else
                    {
                        gMainGameMode = GM_SOFT_RESET;
#ifdef DEBUG
                        if (gDebugMode)
                            gMainGameMode = GM_DEBUG_MENU;
#endif // DEBUG
                    }

                    gSubGameMode1 = 0;
                    gSubGameMode2 = 0;
                }
                break;

    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    if (event->type == SDL_EVENT_QUIT) return SDL_APP_SUCCESS;

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    SDL_Log("Exiting with result %d...", result);
}
