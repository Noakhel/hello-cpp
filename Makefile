# Milestone 1: Build the dijkstra executable
milestone1:
	cmake -B build-m1 -D target_name=dijkstra
	cmake --build build-m1

# Milestone 2: Build the sim executable
milestone2:
	cmake -B build-m2 -D target_name=sim
	cmake --build build-m2

# Milestone 3: Build the simulation with raylib animation
milestone3:
	gcc main.c graph.c -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -o sim

# Milestone 4: Build the multi-process traveler simulation
milestone4:
	gcc main.c graph.c -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -o sim

# Milestone 5: Build the dynamic and independent simulation
milestone5:
	gcc main.c graph.c -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -o sim

# Milestone 6: Build the next step simulation
milestone6:
	gcc main.c graph.c -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -o sim

# Milestone 7: Build the simulation with scheduling algorithms (FCFS/SJF)
milestone7:
	cmake -B cmake-build-debug
	cmake --build cmake-build-debug
	cp cmake-build-debug/sim .

# Clean all build and executable files
clean:
	rm -rf build-m1 build-m2 cmake-build-debug sim dijkstra