#include "nmea_waypoint_handler.h"
#include "sync_manager.h"
#include <NMEA2000.h>
#include <N2kMessages.h>
#include "NMEA2000_CAN.h"
#include <NMEA2000_CAN.h>
#include "N2kDeviceList.h"
#include <wiringPi.h>
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

// Define GPIO pins for LEDS (wiringPi Pin numbering)
#define Transmitting_LED_PIN 0      //GPIO 17 (Yellow LED for Transmitting)


// Example map of device name to device type 
std::unordered_map<uint64_t, std::string> deviceNameMap = {
    {0x123456789ABCDEF0, "Garmin"}, // Example Garmin device
    {0x0987654321ABCDEF, "Lowrance"} // Example Lowrance device
};

// NMEA 2000 PGNs for waypoint operations
const unsigned long PGN_WAYPOINT_LIST = 130074;

NMEAWaypointHandler::NMEAWaypointHandler(SyncManager& sm, std::unique_ptr<tNMEA2000> nmea2000Instance)
    : syncManager(sm), nmea2000(std::move(nmea2000Instance)) {
    instance = this;

    // Initialize WiringPi and set the PIN modes
    if (wiringPiSetup() == -1) {
        std::cerr << "Failed to initialize WiringPi" << std::endl;
        exit(1);
    }

    pinMode(Transmitting_LED_PIN, OUTPUT);

    nmea2000->SetProductInformation("00000001", 100, "Waypoint Handler", "1.0.0.0", "1.0.0");
    nmea2000->SetDeviceInformation(1, 130, 60, 4096);
    nmea2000->SetMode(tNMEA2000::N2km_ListenAndNode);
    nmea2000->SetMsgHandler(NMEAWaypointHandler::MessageHandler);

    deviceList = std::make_unique<tN2kDeviceList>(nmea2000.get());
}

NMEAWaypointHandler::~NMEAWaypointHandler() {
    instance = nullptr;
}

NMEAWaypointHandler* NMEAWaypointHandler::instance = nullptr; 

void NMEAWaypointHandler::MessageHandler(const tN2kMsg &N2kMsg) {
    if (instance) {
        instance->OnN2kMessage(N2kMsg);
    }
}
 
const std::vector<std::string>& NMEAWaypointHandler::getDetectedDevices()  {
    detectedDevices.clear();

        // Debug output for mockMode status and mockDevices content
    std::cout << "Mock mode status: " << (mockMode ? "Enabled" : "Disabled") << std::endl;
    std::cout << "Mock devices: ";
    for (const auto& device : mockDevices) {
        std::cout << device << " ";
    }
    std::cout << std::endl;

    if (mockMode) {
        std::cout << "Mock mode enabled: Returning mock devices.\n";
        detectedDevices = mockDevices;
        return detectedDevices;

    }

    std::cout << "Scanning for connected NMEA devices..." << std::endl;
    for (uint8_t i = 0; i < deviceList->Count(); ++i) {
        const tNMEA2000::tDevice* device = deviceList->FindDeviceBySource(i);
        if(device) {
            uint64_t deviceName = device->GetName();
            auto it = deviceNameMap.find(deviceName);
            if (it != deviceNameMap.end()) {
                std::string deviceType = it->second;
                detectedDevices.push_back(deviceType);
                std::cout << "Detected device: " << deviceType << std::endl;

                // output the receive and transmit PGNs for the device
                const unsigned long* receivePGNs = device->GetReceivePGNs();
                const unsigned long* transmitPGNs = device->GetTransmitPGNs();

                if (receivePGNs) {
                    while(*receivePGNs) {
                        std::cout << "Receive PGN: " << *receivePGNs << std::endl;
                        receivePGNs++;
                    }
                }

                if (transmitPGNs) {
                    while(*transmitPGNs) {
                        std::cout << " Transmit PGN: " << *transmitPGNs << std::endl;
                        ++transmitPGNs;
                    }
                }
            }
        }
    }

    if (detectedDevices.empty()) {
        std::cout << "No devices detected." << std::endl;
    }
    return detectedDevices;
}

void NMEAWaypointHandler::OnN2kMessage(const tN2kMsg &N2kMsg) {
    unsigned long PGN = N2kMsg.PGN;
    if (PGN == PGN_WAYPOINT_LIST) {
        handleWaypointList(N2kMsg);
    } else  {

    }
}



void NMEAWaypointHandler::enableMockMode(const std::vector<std::string>& devices) {
    mockMode = true;
    mockDevices = devices;
}

void NMEAWaypointHandler::handleWaypointList(const tN2kMsg &N2kMsg) {
    std::cout << "Received waypoint list message." << std::endl;
}

void NMEAWaypointHandler::start() {
    static bool alreadyStarted = false;
    if (!alreadyStarted && nmea2000) {
        std::cout << "Calling Open() on nmea2000 from: " << __FILE__ << ":" << __LINE__ << std::endl;
        nmea2000->Open();
        alreadyStarted = true;
    }
}

void NMEAWaypointHandler::addWaypoint(uint16_t waypointID, const std::string& name, double latitude, double longitude) {
    // Check if the waypoint already exists
    if (waypointMap.find(waypointID) != waypointMap.end()) {
        std::cerr << "Waypoint ID " << waypointID << " already exists. Use updateWaypoint to modify it." << std::endl;
        return;
    }

    // Add a new waypoint
    waypointMap[waypointID] = {name, {latitude, longitude}};
    std::cout << "Adding new waypoint ID: " << waypointID << " [" << name << ", " << latitude << ", " << longitude << "]" << std::endl;

    // Send the NMEA message
    tN2kMsg N2kMsg;
    SetN2kPGN130074(N2kMsg, waypointID, 1, 0); 
    AppendN2kPGN130074(N2kMsg, waypointID, const_cast<char*>(name.c_str()), latitude, longitude);

    // Turn on the Transmitting LED
    digitalWrite(Transmitting_LED_PIN, HIGH);

    if (nmea2000) {
        std::cout << "Sending waypoint message\n";
        nmea2000->SendMsg(N2kMsg);
        std::cout << "Added waypoint: " << name << " [" << latitude << ", " << longitude << "]" << std::endl;
    } else {
        std::cerr << "nmea2000 is nullptr; cannot send message." << std::endl;
    }

    // Turn off the Transmitting LED
    digitalWrite(Transmitting_LED_PIN, LOW);
}

void NMEAWaypointHandler::updateWaypoint(uint16_t waypointID, const std::string& newName, double latitude, double longitude) {
    // Check if the waypoint exists
    auto it = waypointMap.find(waypointID);
    if (it == waypointMap.end()) {
        std::cerr << "Waypoint ID " << waypointID << " does not exist. Use addWaypoint to create it first." << std::endl;
        return;
    }

    // Check if there is an actual change
    if (it->second.first == newName && it->second.second.first == latitude && it->second.second.second == longitude) {
        std::cout << "No changes detected for waypoint ID: " << waypointID << std::endl;
        return;
    }

    // Update the stored waypoint
    std::cout << "Updating waypoint ID: " << waypointID
              << " from [" << it->second.first << ", "
              << it->second.second.first << ", "
              << it->second.second.second << "] "
              << "to [" << newName << ", " << latitude << ", " << longitude << "]" << std::endl;

    it->second = {newName, {latitude, longitude}};

    // Send the NMEA message
    tN2kMsg N2kMsg;
    SetN2kPGN130074(N2kMsg, waypointID, 1, 0);
    AppendN2kPGN130074(N2kMsg, waypointID, const_cast<char*>(newName.c_str()), latitude, longitude);


    // Turn on the Transmitting LED
    digitalWrite(Transmitting_LED_PIN, HIGH);

    if (nmea2000) {
        std::cout << "Sending waypoint update message\n";
        nmea2000->SendMsg(N2kMsg);
        std::cout << "Updated waypoint: " << newName << " [" << latitude << ", " << longitude << "]" << std::endl;
    } else {
        std::cerr << "nmea2000 is nullptr; cannot send message." << std::endl;
    }

    // Turn off the Transmitting LED
    digitalWrite(Transmitting_LED_PIN, LOW);
}


void NMEAWaypointHandler::convertAndSendWaypoint(const std::string& waypointData, const std::string& format) {
    std::cout << "Converting and sending waypoint data..." << waypointData << "to format: " << format <<  std::endl;
}

