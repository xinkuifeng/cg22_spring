clean:
	rm *.out

compile: game.cc
	clang++ --std=c++17 game.cc
