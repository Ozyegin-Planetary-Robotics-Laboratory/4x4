#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include "tmotorcan-cpp/include/tmotor.hpp" // Adjust path if necessary

// Constants for buffer sizes
const int BUFFER_SIZE = 65536; // 64 KB buffer size

int main() {
    // Create socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return -1;
    }
    std::cout << "Socket created successfully.\n";

    // Set socket receive buffer size
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &BUFFER_SIZE, sizeof(BUFFER_SIZE)) < 0) {
        std::cerr << "Failed to set receive buffer size" << std::endl;
    } else {
        std::cout << "Receive buffer size set to " << BUFFER_SIZE << " bytes" << std::endl;
    }

    // Setup rover address and bind socket
    sockaddr_in rover_addr, client_addr;
    rover_addr.sin_family = AF_INET;
    rover_addr.sin_addr.s_addr = INADDR_ANY;
    rover_addr.sin_port = htons(12345); // Same port as used in the client program

    if (bind(sockfd, (const sockaddr*)&rover_addr, sizeof(rover_addr)) < 0) {
        std::cerr << "Failed to bind socket" << std::endl;
        close(sockfd);
        return -1;
    }
    std::cout << "Socket bound successfully to port 12345.\n";

    // Initialize motor managers
    std::vector<std::pair<int, std::shared_ptr<TMotor::AKManager>>> managers;
    std::vector<int> motor_ids = {0xF, 0xC, 0xD, 0xE}; // Motor IDs

    for (int id : motor_ids) {
        try {
            auto manager = std::make_shared<TMotor::AKManager>(id);
            manager->connect("can0");
            std::cout << "Connected to motor ID: " << std::hex << id << std::endl;
            managers.emplace_back(id, manager);
        } catch (const std::exception &e) {
            std::cerr << "Failed to connect to motor ID " << std::hex << id << ": " << e.what() << std::endl;
            close(sockfd);
            return -1;
        }
    }

    char buffer[BUFFER_SIZE];
    socklen_t len = sizeof(client_addr);

    while (true) {
        // Receive data from client
        int n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&client_addr, &len);
        if (n < 0) {
            std::cerr << "Failed to receive data" << std::endl;
            continue;
        }
        buffer[n] = '\0';
        std::cout << "Received data: " << buffer << std::endl;

        // Parse received data
        std::istringstream iss(buffer);
        float motor_f_velocity = 0.0f, motor_c_velocity = 0.0f, motor_d_velocity = 0.0f, motor_e_velocity = 0.0f;
        iss >> motor_f_velocity >> motor_c_velocity >> motor_d_velocity >> motor_e_velocity;
        std::cout << "Parsed velocities - F: " << motor_f_velocity << ", C: " << motor_c_velocity
                  << ", D: " << motor_d_velocity << ", E: " << motor_e_velocity << std::endl;

        // Send velocities to motors and log the actions
        try {
            for (auto& [id, manager] : managers) {
                if (id == 0xF) {
                    manager->sendVelocity(motor_f_velocity);
                    std::cout << "Sent F motor velocity: " << motor_f_velocity << std::endl;
                } else if (id == 0xC) {
                    manager->sendVelocity(motor_c_velocity);
                    std::cout << "Sent C motor velocity: " << motor_c_velocity << std::endl;
                } else if (id == 0xD) {
                    manager->sendVelocity(motor_d_velocity);
                    std::cout << "Sent D motor velocity: " << motor_d_velocity << std::endl;
                } else if (id == 0xE) {
                    manager->sendVelocity(motor_e_velocity);
                    std::cout << "Sent E motor velocity: " << motor_e_velocity << std::endl;
                }
            }
        } catch (const std::exception &e) {
            std::cerr << "Error sending motor velocity: " << e.what() << std::endl;
            continue;
        }
    }

    close(sockfd);
    return 0;
}
