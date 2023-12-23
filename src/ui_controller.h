#ifndef UI_CONTROLLER_H
#define UI_CONTROLLER_H
#include "game_controller.h"

#ifdef __WIN32

#define KEY_UP_ARROW 72
#define KEY_DOWN_ARROW 80
#define KEY_LEFT_ARROW 75
#define KEY_RIGHT_ARROW 77
#define KEY_ESC 27

#else

#define KEY_UP_ARROW 65
#define KEY_DOWN_ARROW 66
#define KEY_LEFT_ARROW 68
#define KEY_RIGHT_ARROW 67
#define KEY_ESC 27

#endif

void initConsole();
void closeConsole();
void hideCursor();
void showCursor();
void getConsoleWidthHeight(int& width, int& height);
void clearScreen(int mode);
void clearScreenInline(int mode);
bool screenToBoard(GameState& state,
                   int screen_r,
                   int screen_c,
                   int& board_r,
                   int& board_c);
void render(const GameState& state,
            int cursor_r,
            int cursor_c,
            bool skipHeader = false,
            bool skipBoard = false,
            bool skipFooter = false);
int getInput();
bool getMouseInput(int& r, int& c, int& event);
int mainMenu(bool saved);
void startGameMenu(int& rows, int& cols, int& bombCount);
bool loseMenu(const GameState& state, int cursor_r, int cursor_c);
bool winMenu(const GameState& state, int bestTime);
int pauseMenu(const GameState& state);
void wait();

#endif
