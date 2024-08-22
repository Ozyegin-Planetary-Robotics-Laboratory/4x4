#include <iostream>
#include <SDL2/SDL.h>
#include <string>
#include <sstream>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <csignal>

// Constants for the deadzone threshold
const float DEADZONE_THRESHOLD = 0.2f;

// Function to map joystick values to a specified range
float mapValue(int value, int min_in, int max_in, float min_out, float max_out) {
    return (float(value - min_in) / float(max_in - min_in)) * (max_out - min_out) + min_out;
}

// Function to apply a deadzone
float applyDeadzone(float value, float deadzone) {
    if (std::abs(value) < deadzone) {
        return 0.0f;
    }
    return value;
}

// Global running flag for signal handling
volatile bool running = true;

// Signal handler for Ctrl+C
void handleSigint(int sig) {
    running = false;
    std::cout << "\nReceived SIGINT (Ctrl+C), shutting down..." << std::endl;
}

int main() {
    // Register signal handler
    signal(SIGINT, handleSigint);

    // Initialize SDL for joystick
    if (SDL_Init(SDL_INIT_JOYSTICK) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return -1;
    }
    std::cout << "SDL initialized successfully.\n";

    // Open joystick
    SDL_Joystick *joystick = SDL_JoystickOpen(0);
    if (joystick == nullptr) {
        std::cerr << "Failed to open joystick: " << SDL_GetError() << std::endl;
        return -1;
    }
    std::cout << "Joystick opened successfully.\n";
    std::cout << "Joystick name: " << SDL_JoystickName(joystick) << std::endl;
    std::cout << "Number of Axes: " << SDL_JoystickNumAxes(joystick) << std::endl;

    // Set up socket for communication with the rover
    std::string rover_ip = "192.168.1.3"; // Replace with your rover's IP address
    int rover_port = 12345; // Choose a port to communicate with the rover

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

    // Main loop for joystick handling
    while (running) {
        SDL_JoystickUpdate();

        int x_axis = SDL_JoystickGetAxis(joystick, 1);
        int y_axis = SDL_JoystickGetAxis(joystick, 0);

        // Map joystick input to -1.0 to 1.0 range
        float linear_velocity = mapValue(y_axis, -32768, 32767, -1.0, 1.0);
        float angular_velocity = mapValue(x_axis, -32768, 32767, -1.0, 1.0);

        // Apply deadzone
        linear_velocity = applyDeadzone(linear_velocity, DEADZONE_THRESHOLD);
        angular_velocity = applyDeadzone(angular_velocity, DEADZONE_THRESHOLD);

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

    // Clean up
    SDL_JoystickClose(joystick);
    SDL_Quit();
    close(sockfd);

    std::cout << "Program terminated gracefully.\n";
    return 0;
}
