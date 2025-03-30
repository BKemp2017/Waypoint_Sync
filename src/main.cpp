#include "nmea_waypoint_handler.h"
#include "sync_manager.h"
#include <wiringPi.h>
#include <unordered_map>
#include <iostream>
#include <chrono>
#include <thread>
#include <csignal>


// GPIO pin numbers based on WiringPi numbering
#define POWER_LED_PIN 7            // GPIO 4 (Blue LED for Power)
#define LISTENING_LED_PIN 3        // GPIO 22 (Green LED for Listening)
#define TRANSMITTING_LED_PIN 0     // GPIO 17 (Yellow LED for Transmitting)
#define ERROR_LED_PIN 2            // GPIO 27 (Red LED for Error)

// Function to turn off all LEDs
void cleanup() {
    std::cout << "Entering cleanup function." << std::endl;
    digitalWrite(POWER_LED_PIN, LOW);
    digitalWrite(LISTENING_LED_PIN, LOW);
    digitalWrite(TRANSMITTING_LED_PIN, LOW);
    digitalWrite(ERROR_LED_PIN, LOW);
    std::cout << "All LEDs turned off. Exiting program." << std::endl;
    std::cout.flush();  // Flush the output
    delay(500);         // Delay to ensure LEDs are visually turned off before the program ends
}

// Signal handler for SIGINT (CTRL+C)
void handleSignal(int signal) {
    std::cout << " Signal received: " << signal << std::endl;
    cleanup();

    // Re-raise the signal to ensure default handling after cleanup
    std::signal(signal, SIG_DFL);
    std::raise(signal);
}

int main() {
    // Register the signal handlers for SIGINT (CTRL+C), SIGTERM, and SIGHUP
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);
    std::signal(SIGHUP, handleSignal);

    // Initialize WiringPi
    if (wiringPiSetup() == -1) {
        std::cerr << "Failed to initialize WiringPi" << std::endl;
        return 1;
    }

    // Set the pin modes for the LEDs
    pinMode(POWER_LED_PIN, OUTPUT);
    pinMode(LISTENING_LED_PIN, OUTPUT);
    pinMode(TRANSMITTING_LED_PIN, OUTPUT);
    pinMode(ERROR_LED_PIN, OUTPUT);

    // Turn on the Power LED to indicate the system is running
    std::cout << "Turning on Power LED" << std::endl;
    digitalWrite(POWER_LED_PIN, HIGH);

    try {
        // Initialize SyncManager and load format mappings
        SyncManager syncManager;

        // Load format mappings from JSON
        syncManager.loadFormatMappings();

        // Initialize inotify and add watches
        syncManager.initialize();

        // Sync the waypoints on boot using loaded format mappings
        syncManager.syncWaypointsOnBoot();

        syncManager.getNmeaHandler()->start();

        // Monitor for real-time data changes, including inotify events and polling
        while (true) {
            // Turn on the listening LED to indicate the system is monitoring for changes
            std::cout << "Listening LED ON during checkForChanges()" << std::endl;
            digitalWrite(LISTENING_LED_PIN, HIGH);
            syncManager.checkForChanges();

            std::this_thread::sleep_for(std::chrono::seconds(2));

            // Turn off the listening LED to indicate the system is not monitoring for changes
            digitalWrite(LISTENING_LED_PIN, LOW);
        }
    } catch (std::exception& e) {
        // If an exception is thrown, turn the Error LED on to indicate a failure.
        std::cerr << "An exception occurred: " << e.what() << std::endl;
        std::cout << "Turning on Error LED" << std::endl;
        digitalWrite(ERROR_LED_PIN, HIGH);
        delay(1000);
        digitalWrite(ERROR_LED_PIN, LOW);
    }

    // Cleanup before exiting
    cleanup();

    return 0;
}
