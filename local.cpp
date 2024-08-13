#include <iostream>
#include <SDL2/SDL.h>
#include <string>
#include <sstream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

float mapValue(int value, int min_in, int max_in, float min_out, float max_out) {
    return (float(value - min_in) / float(max_in - min_in)) * (max_out - min_out) + min_out;
}

int main() {
    if (SDL_Init(SDL_INIT_JOYSTICK) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return -1;
    }
    std::cout << "SDL initialized successfully.\n";

    SDL_Joystick *joystick = SDL_JoystickOpen(0);
    if (joystick == nullptr) {
        std::cerr << "Failed to open joystick: " << SDL_GetError() << std::endl;
        return -1;
    }
    std::cout << "Joystick opened successfully.\n";
    std::cout << "Joystick name: " << SDL_JoystickName(joystick) << std::endl;
    std::cout << "Number of Axes: " << SDL_JoystickNumAxes(joystick) << std::endl;

    std::string rover_ip = "192.168.1.3"; // Replace with your rover's IP address
    int rover_port = 12345; // Choose a port to communicate with the rover

    // Create a socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return -1;
    }
    std::cout << "Socket created successfully.\n";

    sockaddr_in rover_addr;
    rover_addr.sin_family = AF_INET;
    rover_addr.sin_port = htons(rover_port);
    inet_pton(AF_INET, rover_ip.c_str(), &rover_addr.sin_addr);
    std::cout << "Socket address set up for rover at " << rover_ip << ":" << rover_port << std::endl;

    bool running = true;
    while (running) {
        SDL_JoystickUpdate();

        int x_axis = SDL_JoystickGetAxis(joystick, 0);
        int y_axis = SDL_JoystickGetAxis(joystick, 1);

        float linear_velocity = mapValue(y_axis, -32768, 32767, -1.0, 1.0);
        float angular_velocity = mapValue(x_axis, -32768, 32767, -1.0, 1.0);

        // Debug output for the joystick values and calculated velocities
        std::cout << "Joystick X-axis: " << x_axis << " | Y-axis: " << y_axis << std::endl;
        std::cout << "Mapped Linear Velocity: " << linear_velocity << " | Mapped Angular Velocity: " << angular_velocity << std::endl;

        // Prepare data to send
        std::ostringstream oss;
        oss << linear_velocity << " " << angular_velocity;
        std::string data = oss.str();

        // Send data to the rover
        ssize_t sent_bytes = sendto(sockfd, data.c_str(), data.size(), 0, (sockaddr*)&rover_addr, sizeof(rover_addr));
        if (sent_bytes < 0) {
            std::cerr << "Failed to send data to rover.\n";
        } else {
            std::cout << "Sent data to rover: " << data << std::endl;
        }

      	 // Adjust delay as needed
	 SDL_Delay(50);
    }

    SDL_JoystickClose(joystick);
    SDL_Quit();
    close(sockfd);
    return 0;
}
