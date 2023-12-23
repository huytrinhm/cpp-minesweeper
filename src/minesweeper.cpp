#include <chrono>
#include <filesystem>
#include <fstream>
#include "game_controller.h"
#include "ui_controller.h"

bool gameLoop(GameState& state, bool isSaved);
void saveGame(const GameState& state);
bool loadGame(GameState& state);
void deleteSave();
void loadHighscores(int highScores[40][40][40 * 20], size_t size);
void saveHighscores(int highScores[40][40][40 * 20], size_t size);

int highScores[40][40][40 * 20] = {0};

int main() {
  initConsole();
  hideCursor();

  loadHighscores(highScores, sizeof highScores);

  bool playing = true;

  do {
    GameState state;
    GameState savedState;
    bool saved = loadGame(savedState);

    int result = mainMenu(saved);

    if (result == 0)
      break;
    else if (result == 1) {
      int rows, cols, bombCount;
      startGameMenu(rows, cols, bombCount);
      initBoard(state, rows, cols, bombCount);
    } else if (result == 2) {
      state = savedState;
    }

    if (!gameLoop(state, result == 2))
      break;
  } while (playing);

  showCursor();
  closeConsole();
  return 0;
}

bool gameLoop(GameState& state, bool isSaved) {
  clearScreenInline(40);
  int cursor_r = 1, cursor_c = 1;

  bool paused = false;
  std::chrono::steady_clock::time_point startTimepoint, lastTimepoint,
      currentTimepoint;
  std::chrono::steady_clock::duration pauseDuration =
      std::chrono::steady_clock::duration::zero();
  if (isSaved)
    startTimepoint = std::chrono::steady_clock::now() -
                     std::chrono::duration_cast<std::chrono::seconds>(
                         std::chrono::duration<float>(state.elapsedTime));
  while (true) {
    render(state, cursor_r, cursor_c);
    currentTimepoint = std::chrono::steady_clock::now();
    if (paused) {
      paused = false;
      if (state.generated)
        pauseDuration += currentTimepoint - lastTimepoint;
    }
    lastTimepoint = currentTimepoint;
    if (state.generated)
      state.elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(
                              currentTimepoint - startTimepoint - pauseDuration)
                              .count();

    int keyCode = getInput();
    int mouse_r, mouse_c, mouse_event;
    if (getMouseInput(mouse_r, mouse_c, mouse_event)) {
      if (mouse_event == 0) {
        screenToBoard(state, mouse_r, mouse_c, cursor_r, cursor_c);
      } else if (mouse_event == 1) {
        if (screenToBoard(state, mouse_r, mouse_c, cursor_r, cursor_c))
          keyCode = ' ';
      } else if (mouse_event == 2) {
        if (screenToBoard(state, mouse_r, mouse_c, cursor_r, cursor_c))
          keyCode = 'f';
      }
    }
    if (keyCode == KEY_RIGHT_ARROW) {
      cursor_c = std::min(state.cols, cursor_c + 1);
    } else if (keyCode == KEY_LEFT_ARROW) {
      cursor_c = std::max(1, cursor_c - 1);
    } else if (keyCode == KEY_DOWN_ARROW) {
      cursor_r = std::min(state.rows, cursor_r + 1);
    } else if (keyCode == KEY_UP_ARROW) {
      cursor_r = std::max(1, cursor_r - 1);
    } else if (keyCode == 'f') {
      toggleFlagPosition(state, cursor_r, cursor_c);
    } else if (keyCode == ' ') {
      if (!state.generated) {
        genBoard(state, cursor_r, cursor_c);
        startTimepoint = std::chrono::steady_clock::now();
      }

      if (!openPosition(state, cursor_r, cursor_c)) {
        openAllBomb(state);
        deleteSave();
        return loseMenu(state, cursor_r, cursor_c);
      }
      if (isWinState(state)) {
        deleteSave();
        int modeHighscores =
            highScores[state.rows - 1][state.cols - 1][state.bombCount - 1];
        if (modeHighscores == 0 || modeHighscores - 1 > state.elapsedTime) {
          highScores[state.rows - 1][state.cols - 1][state.bombCount - 1] =
              state.elapsedTime + 1;
          saveHighscores(highScores, sizeof highScores);
          return winMenu(state, state.elapsedTime);
        } else {
          return winMenu(state, modeHighscores - 1);
        }
      }
    } else if (keyCode == KEY_ESC) {
      paused = true;
      int result = pauseMenu(state);
      if (result == 0)
        return false;
      else if (result == 1)
        continue;
      else if (result == 2) {
        saveGame(state);
        return true;
      }
    }
  }
}

void saveGame(const GameState& state) {
  std::ofstream file("game_state.bin", std::ios::binary);
  if (file.is_open()) {
    file.write((char*)&state, sizeof state);
  }
  file.close();
}

bool loadGame(GameState& state) {
  std::ifstream file("game_state.bin", std::ios::binary);
  if (!file.is_open())
    return false;

  if (file.read((char*)&state, sizeof state)) {
    file.close();
    return true;
  } else {
    file.close();
    return false;
  }
}

void deleteSave() {
  std::filesystem::remove("game_state.bin");
}

void loadHighscores(int highScores[40][40][40 * 20], size_t size) {
  std::ifstream file("highScores.bin", std::ios::binary);
  if (!file.is_open())
    return;

  file.read((char*)highScores, size);
  file.close();
}

void saveHighscores(int highScores[40][40][40 * 20], size_t size) {
  std::ofstream file("highScores.bin", std::ios::binary);
  if (!file.is_open())
    return;

  file.write((char*)highScores, size);
  file.close();
}
