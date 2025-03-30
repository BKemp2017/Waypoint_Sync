#include <NMEA2000.h>
#include <NMEA2000_SocketCAN.h> // For Raspberry Pi CAN

// Create an instance of the NMEA2000_SocketCAN class
const char* can_interface = "can0";
tNMEA2000_SocketCAN NMEA2000((char*)can_interface);

// Setup function
void setup() {
    NMEA2000.SetProductInformation(
        "123456",       // Unique serial number
        100,            // Product code
        "Waypoint Sync", // Model ID
        "1.0.0.0",       // Software version
        "1.0.0.0"        // Hardware version
    );

    NMEA2000.SetDeviceInformation(
        123456,  // Unique ID (choose a number)
        130,     // Device function (e.g., sensor, chart plotter)
        25,      // Device class (e.g., sensor)
        2046     // Manufacturer code (NMEA reserved)
    );

    // Set mode and open the connection
    NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode, 25); // Node mode
    NMEA2000.EnableForward(false); // Disable forwarding
    NMEA2000.Open();
}

// Loop function
void loop() {
    NMEA2000.ParseMessages();
}

// Main function
int main() {
    setup();  // Initialize the device

    while (true) { // Infinite loop to simulate Arduino-style behavior
        loop();
    }

    return 0;
}
