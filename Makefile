# Milestone 1: Build the dijkstra executable
milestone1:
	cmake -B build-m1 -D target_name=dijkstra
	cmake --build build-m1

# Milestone 2: Build the sim executable
milestone2:
	cmake -B build-m2 -D target_name=sim
	cmake --build build-m2

# Clean build files
clean:
	rm -rf build-m1 build-m2 cmake-build-debug sim dijkstra
milestone3:
	gcc main.c graph.c -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -o sim
