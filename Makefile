clean:
	rm *.out

compile: src/game.cc
	clang++ --std=c++17 -o game.out src/game.cc
