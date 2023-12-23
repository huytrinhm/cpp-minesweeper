#include "ui_controller.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <algorithm>

#ifdef _WIN32

#include <conio.h>
#include <windows.h>

void initConsole() {
  HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
  HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD idwMode, odwMode;

  GetConsoleMode(hInput, &idwMode);
  GetConsoleMode(hOutput, &odwMode);

  idwMode = ENABLE_PROCESSED_INPUT | ENABLE_EXTENDED_FLAGS | ENABLE_MOUSE_INPUT;
  odwMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;

  SetConsoleMode(hInput, idwMode);
  SetConsoleMode(hOutput, odwMode);
  SetConsoleOutputCP(CP_UTF8);
  clearScreen(40);
}

void closeConsole() {
  HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
  DWORD idwMode;
  GetConsoleMode(hInput, &idwMode);
  idwMode &= ~ENABLE_MOUSE_INPUT;
  SetConsoleMode(hInput, idwMode);
  clearScreen(0);
}

void getConsoleWidthHeight(int& width, int& height) {
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
  width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
  height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

int getInput() {
  int ch = -1;
  if (kbhit()) {
    ch = getch();
    if (ch >= 'A' && ch <= 'Z')
      ch = tolower(ch);
    if (ch == 224)
      ch = getch();
    else if (ch == 0)
      getch();
  }
  return ch;
}

bool getMouseInput(int& r, int& c, int& event) {
  HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
  DWORD cNumRead = 0;
  INPUT_RECORD irInBuf[1];
  bool retcode = PeekConsoleInput(hInput, irInBuf, 1, &cNumRead);
  if (!retcode || cNumRead == 0)
    return false;

  ReadConsoleInput(hInput, irInBuf, 1, &cNumRead);
  // FlushConsoleInputBuffer(hInput);
  if (irInBuf[0].EventType != MOUSE_EVENT)
    return false;

  MOUSE_EVENT_RECORD mouseEvent = irInBuf[0].Event.MouseEvent;
  r = mouseEvent.dwMousePosition.Y + 1;
  c = mouseEvent.dwMousePosition.X + 1;

  if (mouseEvent.dwEventFlags == MOUSE_MOVED) {
    event = 0;
    return true;
  }

  if (mouseEvent.dwEventFlags != 0)
    return false;

  if (mouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED)
    event = 1;
  else if (mouseEvent.dwButtonState == RIGHTMOST_BUTTON_PRESSED)
    event = 2;
  else
    return false;

  return true;
}

#else

#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

void initConsole() {
  termios term;
  tcgetattr(0, &term);
  term.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(0, TCSANOW, &term);
  clearScreen(40);
}

void closeConsole() {
  clearScreen(0);
  termios term;
  tcgetattr(0, &term);
  term.c_lflag |= ICANON | ECHO;
  tcsetattr(0, TCSANOW, &term);
  tcflush(0, TCIFLUSH);
}

bool kbhit() {
  int byteswaiting;
  ioctl(0, FIONREAD, &byteswaiting);
  return byteswaiting > 0;
}

int getch() {
  unsigned char ch;
  int retcode;
  retcode = read(STDIN_FILENO, &ch, 1);
  return retcode <= 0 ? EOF : (int)ch;
}

static int getchs(char* buffer) {
  return read(STDIN_FILENO, buffer, 8);
}

void getConsoleWidthHeight(int& width, int& height) {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

  width = w.ws_col;
  height = w.ws_row;
}

int getInput() {
  if (!kbhit())
    return -1;

  char buffer[8];
  int bytes = getchs(buffer);

  if (bytes <= 0)
    return -1;

  if (bytes == 1)
    return tolower(buffer[0]);

  if (bytes == 3 && buffer[0] == KEY_ESC && buffer[1] == 91)
    return buffer[2];

  return -1;
}

bool getMouseInput(int& r, int& c, int& event) {
  return false;
}

#endif

int lastWidth = -1;
int lastHeight = -1;

char symbols[][32] = {
    " ",         "\x1b[94m𝟏", "\x1b[32m𝟐",          "\x1b[91m𝟑", "\x1b[34m𝟒",
    "\x1b[31m𝟓", "\x1b[36m𝟔", "\x1b[30m𝟕",          "\x1b[37m𝟖", "■",
    "\x1b[93m⚑", "\x1b[95m*", "\x1b[7;93m⚑\x1b[27m"};

void clearScreen(int mode) {
  printf("\x1b[%d;97m", mode);
  printf("\x1b[2J");
  printf("\x1b[H");
}

void clearScreenInline(int mode) {
  printf("\x1b[%d;97m", mode);
  int rows, cols;
  getConsoleWidthHeight(cols, rows);
  for (int r = 1; r <= rows; r++)
    for (int c = 1; c <= cols; c++)
      printf("\x1b[%d;%dH ", r, c);
  printf("\x1b[H");
}

void hideCursor() {
  printf("\x1b[?25l");
}

void showCursor() {
  printf("\x1b[?25h");
}

static void assertScreenSize(int rows, int cols) {
  int width, height;
  getConsoleWidthHeight(width, height);
  if (width >= cols && height >= rows)
    return;

  char msg[] = "Screen too small!";

  printf("\x1b[2J");
  printf("\x1b\x1b[%d;%dH%s", (height - 1) / 2 + 1,
         (width - (int)strlen(msg)) / 2 + 1, msg);
  fflush(stdout);

  while (width < cols || height < rows) {
    if (width != lastWidth || height != lastHeight) {
      printf("\x1b[2J");
      printf("\x1b\x1b[%d;%dH%s", (height - 1) / 2 + 1,
             (width - (int)strlen(msg)) / 2 + 1, msg);
      fflush(stdout);
    }

    lastWidth = width;
    lastHeight = height;

    getConsoleWidthHeight(width, height);
  }
}

static char* cellToChar(int cell) {
  return symbols[cell];
}

bool screenToBoard(GameState& state,
                   int screen_r,
                   int screen_c,
                   int& board_r,
                   int& board_c) {
  int boardWidth = 2 * state.cols - 1 + 4;
  int boardHeight = state.rows + 2;
  int consoleWidth, consoleHeight;
  getConsoleWidthHeight(consoleWidth, consoleHeight);
  int pos_r = (consoleHeight - boardHeight - 3) / 2 + 3 + 1;
  int pos_c = (consoleWidth - boardWidth) / 2 + 1 + 2;

  if ((screen_r < pos_r) || (screen_r >= pos_r + state.rows))
    return false;
  if ((screen_c < pos_c) || (screen_c >= pos_c + 2 * state.cols - 1))
    return false;

  if ((screen_c - pos_c) % 2 != 0)
    return false;

  board_r = screen_r - pos_r + 1;
  board_c = (screen_c - pos_c) / 2 + 1;

  return true;
}

static void printBoardBorder(int pos_r, int pos_c, int rows, int cols) {
  printf("\x1b[%d;%dH", pos_r, pos_c + 1);
  for (int i = 1; i <= 2 * cols + 1; i++)
    printf("═");
  printf("\x1b[%d;%dH", pos_r + rows + 1, pos_c + 1);
  for (int i = 1; i <= 2 * cols + 1; i++)
    printf("═");
  for (int i = pos_r + 1; i <= pos_r + rows; i++)
    printf("\x1b[%d;%dH║", i, pos_c);
  for (int i = pos_r + 1; i <= pos_r + rows; i++)
    printf("\x1b[%d;%dH║", i, pos_c + 2 * cols + 2);

  printf("\x1b[%d;%dH╔", pos_r, pos_c);
  printf("\x1b[%d;%dH╗", pos_r, pos_c + 2 * cols + 2);
  printf("\x1b[%d;%dH╚", pos_r + rows + 1, pos_c);
  printf("\x1b[%d;%dH╝", pos_r + rows + 1, pos_c + 2 * cols + 2);
}

static void printBoard(const GameState& state,
                       int pos_r,
                       int pos_c,
                       int cursor_r,
                       int cursor_c) {
  int innerBoard_r = pos_r + 1;
  int innerBoard_c = pos_c + 2;

  printBoardBorder(pos_r, pos_c, state.rows, state.cols);
  for (int r = 1; r <= state.rows; r++) {
    for (int c = 1; c <= state.cols; c++) {
      if (r == cursor_r && c == cursor_c)
        printf("\x1b[7m");
      printf("\x1b[%d;%dH", innerBoard_r + r - 1, innerBoard_c + 2 * (c - 1));
      printf("%s", cellToChar(state.display[r][c]));
      if (r == cursor_r && c == cursor_c)
        printf("\x1b[27m");
      printf("\x1b[97m");
    }
  }
}

static void renderHeader(char fmt[], char header[], int pos_r, int pos_c) {
  printf("\x1b[%d;%dH", pos_r, pos_c);
  printf("\x1b[2K");
  printf(fmt, header);
}

void render(const GameState& state,
            int cursor_r,
            int cursor_c,
            bool skipHeader,
            bool skipBoard,
            bool skipFooter) {
  int boardWidth = 2 * state.cols - 1 + 4;
  int boardHeight = state.rows + 2;
  int consoleWidth, consoleHeight;
  getConsoleWidthHeight(consoleWidth, consoleHeight);
  int board_r = (consoleHeight - boardHeight - 3) / 2 + 3;
  int board_c = (consoleWidth - boardWidth) / 2 + 1;
  if (consoleWidth != lastWidth || consoleHeight != lastHeight)
    clearScreenInline(40);

  lastWidth = consoleWidth;
  lastHeight = consoleHeight;

  assertScreenSize(boardHeight + 3, std::max(boardWidth, 63));

  // HEADER
  if (!skipHeader) {
    int flagCount = 0;
    for (int r = 1; r <= state.rows; r++)
      for (int c = 1; c <= state.cols; c++)
        if (state.display[r][c] == 10)
          flagCount++;
    char header[100];
    sprintf(header, "Time: %3ds   |   Mines: %2d/%d", state.elapsedTime,
            flagCount, state.bombCount);
    renderHeader((char*)"%s", header, 1,
                 (consoleWidth - strlen(header)) / 2 + 1);
  }

  // FOOTER
  if (!skipFooter) {
    printf("\x1b[%d;1H", consoleHeight);
    printf(
        "\x1b[34m[ESC]\x1b[97m Pause   \x1b[34m[SPACE]\x1b[97m Open cell   "
        "\x1b[34m[F]\x1b[97m Flag cell   \x1b[34m[ARROWS]\x1b[97m Move");
  }

  if (!skipBoard)
    printBoard(state, board_r, board_c, cursor_r, cursor_c);
  fflush(stdout);
}

static void renderPauseMenu(const GameState& state) {
  int boardWidth = 2 * state.cols - 1 + 4;
  int boardHeight = state.rows + 2;
  int consoleWidth, consoleHeight;
  getConsoleWidthHeight(consoleWidth, consoleHeight);
  int board_r = (consoleHeight - boardHeight - 3) / 2 + 3;
  int board_c = (consoleWidth - boardWidth) / 2 + 1;

  lastWidth = consoleWidth;
  lastHeight = consoleHeight;

  assertScreenSize(boardHeight + 3, std::max(boardWidth, 41));

  // HEADER
  int flagCount = 0;
  for (int r = 1; r <= state.rows; r++)
    for (int c = 1; c <= state.cols; c++)
      if (state.display[r][c] == 10)
        flagCount++;
  char header[100];
  sprintf(header, "Time: %3ds   |   Mines: %2d/%d", state.elapsedTime,
          flagCount, state.bombCount);
  renderHeader((char*)"%s", header, 1, (consoleWidth - strlen(header)) / 2 + 1);

  // FOOTER
  printf("\x1b[%d;1H", consoleHeight);
  printf("\x1b[2K");
  printf(
      "\x1b[34m[ESC]\x1b[97m Continue   \x1b[34m[S]\x1b[97m Save game   "
      "\x1b[34m[Q]\x1b[97m Quit");

  printBoardBorder(board_r, board_c, state.rows, state.cols);

  for (int r = board_r + 1; r <= board_r + state.rows; r++)
    for (int c = board_c + 1; c <= board_c + 2 * state.cols + 1; c++)
      printf("\x1b[%d;%dH ", r, c);

  printf("\x1b[%d;%dH%s", board_r + (state.rows - 1) / 2 + 1,
         board_c + (boardWidth - 2 - 8) / 2 + 1, "⏳PAUSED⏳");
  fflush(stdout);
}

int pauseMenu(const GameState& state) {
  renderPauseMenu(state);
  while (true) {
    int consoleWidth, consoleHeight;
    getConsoleWidthHeight(consoleWidth, consoleHeight);
    if (consoleWidth != lastWidth || consoleHeight != lastHeight) {
      clearScreenInline(40);
      renderPauseMenu(state);
    }
    lastWidth = consoleWidth;
    lastHeight = consoleHeight;

    int keyCode = getInput();
    if (keyCode == 'q')
      return 0;
    else if (keyCode == KEY_ESC) {
      clearScreenInline(40);
      return 1;
    } else if (keyCode == 's')
      return 2;
  }
}

int mainMenu(bool saved) {
  clearScreenInline(40);
  int numberOfOptions = saved ? 3 : 2;
  int select = 1;
  while (true) {
    int width, height;
    getConsoleWidthHeight(width, height);
    if (width != lastWidth || height != lastHeight)
      clearScreenInline(40);
    lastWidth = width;
    lastHeight = height;
    int menuHeight = 2 + 4 + 3 * numberOfOptions + 1;
    int menuWidth = 36;
    int menu_r = (height - menuHeight) / 2 + 1;
    int menu_c = (width - menuWidth) / 2 + 1;
    assertScreenSize(menuHeight, menuWidth);

    printf("\x1b[%d;%dH", menu_r, menu_c + 1);
    for (int i = 1; i < menuWidth - 1; i++)
      printf("═");
    printf("\x1b[%d;%dH", menu_r + menuHeight - 1, menu_c + 1);
    for (int i = 1; i < menuWidth - 1; i++)
      printf("═");
    for (int i = menu_r + 1; i < menu_r + menuHeight - 1; i++)
      printf("\x1b[%d;%dH║", i, menu_c);
    for (int i = menu_r + 1; i < menu_r + menuHeight - 1; i++)
      printf("\x1b[%d;%dH║", i, menu_c + menuWidth - 1);

    printf("\x1b[%d;%dH╔", menu_r, menu_c);
    printf("\x1b[%d;%dH╗", menu_r, menu_c + menuWidth - 1);
    printf("\x1b[%d;%dH╚", menu_r + menuHeight - 1, menu_c);
    printf("\x1b[%d;%dH╝", menu_r + menuHeight - 1, menu_c + menuWidth - 1);

    int newGame_r = menu_r + 2 + 4;
    int newGame_c = menu_c + (menuWidth - 10) / 2;
    int resumeGame_r = menu_r + 2 + 4 + 3;
    int resumeGame_c = menu_c + (menuWidth - 12) / 2;
    int quitGame_r = menu_r + 2 + 4 + (numberOfOptions - 1) * 3;
    int quitGame_c = menu_c + (menuWidth - 6) / 2;

    printf("\x1b[%d;%dH\x1b[91m%s\x1b[97m", menu_r + 2,
           menu_c + (menuWidth - 16) / 2, "🚩 MINESWEEPER 🚩");
    printf("\x1b[%d;%dH\x1b[%dm%s\x1b[27m", newGame_r, newGame_c,
           select == 1 ? 7 : 27, "[NEW GAME]");
    if (saved)
      printf("\x1b[%d;%dH\x1b[%dm%s\x1b[27m", resumeGame_r, resumeGame_c,
             select == 2 ? 7 : 27, "[RESUME GAME]");
    printf("\x1b[%d;%dH\x1b[%dm%s\x1b[27m", quitGame_r, quitGame_c,
           select + !saved == 3 ? 7 : 27, "[QUIT]");
    fflush(stdout);

    int keyCode = getInput();
    int mouse_r, mouse_c, mouse_event;
    if (getMouseInput(mouse_r, mouse_c, mouse_event)) {
      if (mouse_event == 0) {
        if (mouse_r == newGame_r && mouse_c >= newGame_c &&
            mouse_c < newGame_c + 10)
          select = 1;
        else if (mouse_r == resumeGame_r && mouse_c >= resumeGame_c &&
                 mouse_c < resumeGame_c + 13 && saved)
          select = 2;
        else if (mouse_r == quitGame_r && mouse_c >= quitGame_c &&
                 mouse_c < quitGame_c + 6)
          select = 2 + saved;
      } else if (mouse_event == 1) {
        if (mouse_r == newGame_r && mouse_c >= newGame_c &&
            mouse_c < newGame_c + 10)
          return 1;
        else if (mouse_r == resumeGame_r && mouse_c >= resumeGame_c &&
                 mouse_c < resumeGame_c + 13 && saved)
          return 2;
        else if (mouse_r == quitGame_r && mouse_c >= quitGame_c &&
                 mouse_c < quitGame_c + 6)
          return 0;
      }
    }

    if (keyCode == KEY_DOWN_ARROW)
      select = std::min(numberOfOptions, select + 1);
    else if (keyCode == KEY_UP_ARROW)
      select = std::max(1, select - 1);
    else if (keyCode == ' ' || keyCode == '\r' || keyCode == '\n') {
      if (select == 1)
        return 1;
      else if (select == 2)
        return saved ? 2 : 0;
      else
        return 0;
    }
  }
}

int lastRows = 16, lastCols = 30, lastBombCount = 99;

void startGameMenu(int& rows, int& cols, int& bombCount) {
  clearScreenInline(40);
  int select = 1;
  rows = lastRows, cols = lastCols, bombCount = lastBombCount;
  while (true) {
    int width, height;
    getConsoleWidthHeight(width, height);
    if (width != lastWidth || height != lastHeight)
      clearScreenInline(40);
    lastWidth = width;
    lastHeight = height;
    int menuHeight = 2 + 2 * 3 + 1;
    int menuWidth = 36;
    int menu_r = (height - menuHeight - 1) / 2 + 1;
    int menu_c = (width - menuWidth) / 2 + 1;

    assertScreenSize(menuHeight + 1, std::max(menuWidth, 70));

    printf("\x1b[%d;%dH", menu_r, menu_c + 1);
    for (int i = 1; i < menuWidth - 1; i++)
      printf("═");
    printf("\x1b[%d;%dH", menu_r + menuHeight - 1, menu_c + 1);
    for (int i = 1; i < menuWidth - 1; i++)
      printf("═");
    for (int i = menu_r + 1; i < menu_r + menuHeight - 1; i++)
      printf("\x1b[%d;%dH║", i, menu_c);
    for (int i = menu_r + 1; i < menu_r + menuHeight - 1; i++)
      printf("\x1b[%d;%dH║", i, menu_c + menuWidth - 1);

    printf("\x1b[%d;%dH╔", menu_r, menu_c);
    printf("\x1b[%d;%dH╗", menu_r, menu_c + menuWidth - 1);
    printf("\x1b[%d;%dH╚", menu_r + menuHeight - 1, menu_c);
    printf("\x1b[%d;%dH╝", menu_r + menuHeight - 1, menu_c + menuWidth - 1);

    int MIN_ROWS = 3;
    int MIN_COLS = 5;
    int MAX_ROWS = 40;
    int MAX_COLS = 40;
    int MIN_BOMB = 1;
    int MAX_BOMB = rows * cols / 2;

    if (bombCount > MAX_BOMB)
      bombCount = MAX_BOMB;

    char str_rows[100], str_cols[100], str_bomb[100];
    sprintf(str_rows, "Minefield Height: \x1b[%dm<  %2d  >\x1b[27m",
            select == 1 ? 7 : 27, rows);
    sprintf(str_cols, "Minefield Width:  \x1b[%dm<  %2d  >\x1b[27m",
            select == 2 ? 7 : 27, cols);
    sprintf(str_bomb, "Number of Mines:  \x1b[%dm< %*s%2d%*s >\x1b[27m",
            select == 3 ? 7 : 27, bombCount > 99 ? 0 : 1, "", bombCount,
            bombCount > 999 ? 0 : 1, "");
    printf("\x1b[%d;%dH%s", menu_r + 2, menu_c + 4, str_rows);
    printf("\x1b[%d;%dH%s", menu_r + 2 + 2, menu_c + 4, str_cols);
    printf("\x1b[%d;%dH%s", menu_r + 2 + 2 + 2, menu_c + 4, str_bomb);

    // FOOTER
    printf("\x1b[%d;1H", height);
    printf(
        "\x1b[34m[UP/DOWN]\x1b[97m Choose options   "
        "\x1b[34m[LEFT/RIGHT]\x1b[97m Change value   \x1b[34m[ENTER]\x1b[97m "
        "Confirm");
    fflush(stdout);

    int keyCode = getInput();
    if (keyCode == KEY_DOWN_ARROW)
      select = std::min(3, select + 1);
    else if (keyCode == KEY_UP_ARROW)
      select = std::max(1, select - 1);
    else if (keyCode == KEY_LEFT_ARROW) {
      if (select == 1)
        rows = std::max(MIN_ROWS, rows - 1);
      else if (select == 2)
        cols = std::max(MIN_COLS, cols - 1);
      else
        bombCount = std::max(MIN_BOMB, bombCount - 1);
    } else if (keyCode == KEY_RIGHT_ARROW) {
      if (select == 1)
        rows = std::min(MAX_ROWS, rows + 1);
      else if (select == 2)
        cols = std::min(MAX_COLS, cols + 1);
      else
        bombCount = std::min(MAX_BOMB, bombCount + 1);
    } else if (keyCode == '\r' || keyCode == '\n') {
      lastRows = rows, lastCols = cols, lastBombCount = bombCount;
      return;
    }
  }
}

static void renderLoseMenu(const GameState& state, int cursor_r, int cursor_c) {
  int consoleWidth, consoleHeight;
  getConsoleWidthHeight(consoleWidth, consoleHeight);
  render(state, cursor_r, cursor_c, false, false, true);

  // HEADER
  char msg[] = "YOU LOSE!";
  renderHeader((char*)"\x1b[41m%s\x1b[40m", msg, 2,
               (consoleWidth - strlen(msg)) / 2 + 1);

  // FOOTER
  printf("\x1b[%d;1H", consoleHeight);
  printf("\x1b[2K");
  printf("\x1b[34m[ESC]\x1b[97m Back to Menu   \x1b[34m[Q]\x1b[97m Quit");
  fflush(stdout);
}

bool loseMenu(const GameState& state, int cursor_r, int cursor_c) {
  renderLoseMenu(state, cursor_r, cursor_c);
  while (true) {
    int consoleWidth, consoleHeight;
    getConsoleWidthHeight(consoleWidth, consoleHeight);
    if (consoleWidth != lastWidth || consoleHeight != lastHeight)
      renderLoseMenu(state, cursor_r, cursor_c);

    int keyCode = getInput();
    if (keyCode == 'q')
      return false;
    else if (keyCode == KEY_ESC)
      return true;
  }
}

static void renderWinMenu(const GameState& state, int bestTime) {
  int consoleWidth, consoleHeight;
  getConsoleWidthHeight(consoleWidth, consoleHeight);
  render(state, 0, 0, true, false, true);

  // HEADER
  char header[100];
  sprintf(header, "Time: %3ds   |   Best Time: %3ds", state.elapsedTime,
          bestTime);
  renderHeader((char*)"%s", header, 1, (consoleWidth - strlen(header)) / 2 + 1);
  char msg[] = "YOU WIN!";
  renderHeader((char*)"\x1b[42m%s\x1b[40m", msg, 2,
               (consoleWidth - strlen(msg)) / 2 + 1);

  // FOOTER
  printf("\x1b[%d;1H", consoleHeight);
  printf("\x1b[2K");
  printf("\x1b[34m[ESC]\x1b[97m Back to Menu   \x1b[34m[Q]\x1b[97m Quit");
  fflush(stdout);
}

bool winMenu(const GameState& state, int bestTime) {
  renderWinMenu(state, bestTime);
  while (true) {
    int consoleWidth, consoleHeight;
    getConsoleWidthHeight(consoleWidth, consoleHeight);
    if (consoleWidth != lastWidth || consoleHeight != lastHeight)
      renderWinMenu(state, bestTime);

    int keyCode = getInput();
    if (keyCode == 'q')
      return false;
    else if (keyCode == KEY_ESC)
      return true;
  }
}

void wait() {
  getch();
}
