#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

const int MAX_M = 120;
const int MAX_N = 120;

struct GameState_s {
  bool board[MAX_M][MAX_N];
  int display[MAX_M][MAX_N];
  int rows, cols, bombCount, elapsedTime;
  bool generated;
};
typedef struct GameState_s GameState;

void initBoard(GameState& state, int rows, int cols, int bombCount);
void genBoard(GameState& state, int r, int c);
bool inBound(const GameState& state, int r, int c);
int updateDisplayPosition(GameState& state, int r, int c);
bool openPosition(GameState& state, int r, int c);
void toggleFlagPosition(GameState& state, int r, int c);
void openAllBomb(GameState& state);
bool isWinState(GameState& state);

#endif
