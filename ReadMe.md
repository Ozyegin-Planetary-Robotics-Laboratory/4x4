g++ -o joystick local.cpp -lSDL2 for local computer compile also need sdl download.(sudo apt get install sdl2
g++ -I./tmotorcan-cpp/include -o start rover.cpp ./tmotorcan-cpp/src/tmotor.cpp -lpthread you need to add tmotorcan-cpp to same directory as rover.cpp inside orin.
run local then rover so it will connect to socket and get data from it. I dont have enough time to add position velocity accel so it will be nice to add position control according to the input from joystick.

