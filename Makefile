all: minesweeper

minesweeper:
	mkdir -p build/ && cd src/ && \
	g++ -Wall -O2 -std=c++17 \
	minesweeper.cpp game_controller.cpp ui_controller.cpp \
	-o ../build/minesweeper \
	-static-libstdc++ -static-libgcc \
	-Wl,-Bstatic -lstdc++ -lpthread \
	-Wl,-Bdynamic \
	-Wl,--as-needed -Wl,--strip-all
