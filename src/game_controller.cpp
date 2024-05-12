#include "game_controller.h"
#include <algorithm>
#include <chrono>
#include <random>

const int dr[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
const int dc[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

std::mt19937_64 rng(
    std::chrono::steady_clock::now().time_since_epoch().count());

void initBoard(GameState& state, int rows, int cols, int bombCount) {
  state.rows = rows;
  state.cols = cols;
  state.elapsedTime = 0;
  state.bombCount = bombCount;
  state.generated = false;

  for (int r = 0; r <= rows + 1; r++) {
    for (int c = 0; c <= cols + 1; c++) {
      state.board[r][c] = 0;
      state.display[r][c] = 9;
    }
  }
}

void genBoard(GameState& state, int r, int c) {
  int* bombCandidate = new int[state.rows * state.cols - 1];

  for (int i = 0, p = 0; i < state.rows * state.cols; i++)
    if (i != (r - 1) * state.cols + c - 1)
      bombCandidate[p++] = i;

  std::shuffle(bombCandidate, bombCandidate + state.rows * state.cols - 1, rng);

  for (int i = 0; i < state.bombCount; i++)
    state.board[bombCandidate[i] / state.cols + 1]
               [bombCandidate[i] % state.cols + 1] = 1;
  state.generated = true;
}

bool inBound(const GameState& state, int r, int c) {
  return r > 0 && c > 0 && r <= state.rows && c <= state.cols;
}

int updateDisplayPosition(GameState& state, int r, int c) {
  if (state.board[r][c] == 1)
    return -1;
  int count = 0;
  for (int i = 0; i < 8; i++)
    count += state.board[r + dr[i]][c + dc[i]];
  state.display[r][c] = count;
  return count != 0;
}

bool openPosition(GameState& state, int r, int c) {
  if (!inBound(state, r, c))
    return true;

  int stack[MAX_M * MAX_N + 10][2];
  int sTop = 0;

  if (state.display[r][c] <= 8) {
    int count = 0;
    for (int i = 0; i < 8; i++)
      count += state.display[r + dr[i]][c + dc[i]] == 10;
    if (count == state.display[r][c]) {
      for (int i = 0; i < 8; i++)
        if (state.display[r + dr[i]][c + dc[i]] == 9 &&
            state.board[r + dr[i]][c + dc[i]]) {
          return false;
        } else if (state.display[r + dr[i]][c + dc[i]] == 9) {
          stack[sTop][0] = r + dr[i];
          stack[sTop++][1] = c + dc[i];
        }
    }
  }

  stack[sTop][0] = r;
  stack[sTop++][1] = c;

  while (sTop > 0) {
    int sR = stack[--sTop][0];
    int sC = stack[sTop][1];

    if (state.display[sR][sC] <= 8)
      continue;

    int updateResult = updateDisplayPosition(state, sR, sC);

    if (updateResult == -1) {
      state.display[sR][sC] = 11;
      return false;
    }

    if (updateResult != 0)
      continue;

    for (int i = 0; i < 8; i++) {
      if (inBound(state, sR + dr[i], sC + dc[i]) &&
          state.display[sR + dr[i]][sC + dc[i]] > 8) {
        stack[sTop][0] = sR + dr[i];
        stack[sTop++][1] = sC + dc[i];
      }
    }
  }

  return true;
}

void toggleFlagPosition(GameState& state, int r, int c) {
  if (!inBound(state, r, c) || state.display[r][c] <= 8)
    return;
  state.display[r][c] = state.display[r][c] == 9 ? 10 : 9;
}

void openAllBomb(GameState& state) {
  for (int r = 1; r <= state.rows; r++)
    for (int c = 1; c <= state.cols; c++)
      if (state.board[r][c]) {
        if (state.display[r][c] != 10)
          state.display[r][c] = 11;
      } else if (state.display[r][c] == 10)
        state.display[r][c] = 12;
}

bool isWinState(GameState& state) {
  for (int r = 1; r <= state.rows; r++)
    for (int c = 1; c <= state.cols; c++)
      if (!state.board[r][c] && state.display[r][c] > 8)
        return false;
  return true;
}
