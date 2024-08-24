#include <iostream>
#include <SDL2/SDL.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>
#include <cstring>
#include <sstream>

// Constants for buffer sizes and joystick deadzone
const int BUFFER_SIZE = 65536; // 64 KB buffer size
const float DEADZONE_THRESHOLD = 0.2f;
const float MOTOR_VELOCITY = 100.0f;

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
    std::signal(SIGINT, handleSigint);

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
    std::cout << "Number of Buttons: " << SDL_JoystickNumButtons(joystick) << std::endl;

    // Set up socket for communication with the rover
    std::string rover_ip = "192.168.1.3"; // Replace with your rover's IP address
    int rover_port = 12345; // Choose a port to communicate with the rover

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return -1;
    }
    std::cout << "Socket created successfully.\n";

    // Set socket send buffer size
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &BUFFER_SIZE, sizeof(BUFFER_SIZE)) < 0) {
        std::cerr << "Failed to set send buffer size" << std::endl;
    } else {
        std::cout << "Send buffer size set to " << BUFFER_SIZE << " bytes" << std::endl;
    }

    sockaddr_in rover_addr;
    rover_addr.sin_family = AF_INET;
    rover_addr.sin_port = htons(rover_port);
    inet_pton(AF_INET, rover_ip.c_str(), &rover_addr.sin_addr);

    // Main loop for joystick handling
    while (running) {
        SDL_JoystickUpdate();

        // Read button states
        bool left_button = SDL_JoystickGetButton(joystick, 0); // Left button
        bool right_button = SDL_JoystickGetButton(joystick, 1); // Right button
        bool l1_button = SDL_JoystickGetButton(joystick, 2); // L1 button
        bool l2_button = SDL_JoystickGetButton(joystick, 3); // L2 button
        bool r1_button = SDL_JoystickGetButton(joystick, 4); // R1 button
        bool r2_button = SDL_JoystickGetButton(joystick, 5); // R2 button
        bool up_button = SDL_JoystickGetButton(joystick, 6); // Up button
        bool down_button = SDL_JoystickGetButton(joystick, 7); // Down button

        // Initialize motor velocities
        float motor_f_velocity = 0.0f;
        float motor_c_velocity = 0.0f;
        float motor_d_velocity = 0.0f;
        float motor_e_velocity = 0.0f;

        // Control motor F with left and right buttons
        if (left_button) {
            motor_f_velocity = -MOTOR_VELOCITY;
        }
        if (right_button) {
            motor_f_velocity = MOTOR_VELOCITY;
        }

        // Control motor C with L1 and L2 buttons
        if (l1_button) {
            motor_c_velocity = -MOTOR_VELOCITY;
        }
        if (l2_button) {
            motor_c_velocity = MOTOR_VELOCITY;
        }

        // Control motor D with R1 and R2 buttons
        if (r1_button) {
            motor_d_velocity = -MOTOR_VELOCITY;
        }
        if (r2_button) {
            motor_d_velocity = MOTOR_VELOCITY;
        }

        // Control motor E with up and down buttons
        if (up_button) {
            motor_e_velocity = MOTOR_VELOCITY;
        }
        if (down_button) {
            motor_e_velocity = -MOTOR_VELOCITY;
        }

        // Debug output for the button states and calculated velocities
        std::cout << "Left Button: " << left_button << " | Right Button: " << right_button << std::endl;
        std::cout << "L1 Button: " << l1_button << " | L2 Button: " << l2_button << std::endl;
        std::cout << "R1 Button: " << r1_button << " | R2 Button: " << r2_button << std::endl;
        std::cout << "Up Button: " << up_button << " | Down Button: " << down_button << std::endl;
        std::cout << "Motor F Velocity: " << motor_f_velocity << std::endl;
        std::cout << "Motor C Velocity: " << motor_c_velocity << std::endl;
        std::cout << "Motor D Velocity: " << motor_d_velocity << std::endl;
        std::cout << "Motor E Velocity: " << motor_e_velocity << std::endl;

        // Prepare data to send
        std::ostringstream oss;
        oss << motor_f_velocity << " " << motor_c_velocity << " " << motor_d_velocity << " " << motor_e_velocity;
        std::string data = oss.str();

        // Send data to the rover
        ssize_t sent_bytes = sendto(sockfd, data.c_str(), data.size(), 0, (sockaddr*)&rover_addr, sizeof(rover_addr));
        if (sent_bytes < 0) {
            std::cerr << "Failed to send data to rover.\n";
        } else {
            std::cout << "Sent data to rover: " << data << std::endl;
        }

        // Adjust delay as needed
        SDL_Delay(5);
    }

    // Clean up
    SDL_JoystickClose(joystick);
    SDL_Quit();
    close(sockfd);

    std::cout << "Program terminated gracefully.\n";
    return 0;
}

