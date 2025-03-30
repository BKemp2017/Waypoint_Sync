#include "nmea_waypoint_handler.h"
#include "gpio_pins.h"
#include "sync_manager.h"
#include <NMEA2000.h>
#include <N2kMessages.h>
#include "NMEA2000_CAN.h"
#include "N2kDeviceList.h"
#include <wiringPi.h>
#include <fstream>
#include <iostream>
#include <thread>


std::unordered_map<uint64_t, std::string> deviceNameMap = {
    {0x123456789ABCDEF0, "Garmin"},
    {0x0987654321ABCDEF, "Lowrance"}
};

const unsigned long PGN_WAYPOINT_LIST = 130074; // Placeholder PGN, replace with real

NMEAWaypointHandler* NMEAWaypointHandler::instance = nullptr;

NMEAWaypointHandler::NMEAWaypointHandler(SyncManager& sm, std::unique_ptr<tNMEA2000> nmea2000Instance)
    : syncManager(sm), nmea2000(std::move(nmea2000Instance)) {
    instance = this;

    if (wiringPiSetup() == -1) {
        std::cerr << "Failed to initialize WiringPi" << std::endl;
        exit(1);
    }
    pinMode(TRANSMITTING_LED_PIN, OUTPUT);

    nmea2000->SetProductInformation("00000001", 100, "Waypoint Handler", "1.0.0.0", "1.0.0");
    nmea2000->SetDeviceInformation(1, 130, 60, 4096);
    nmea2000->SetMode(tNMEA2000::N2km_ListenAndNode);
    nmea2000->SetMsgHandler(NMEAWaypointHandler::MessageHandler);

    deviceList = std::make_unique<tN2kDeviceList>(nmea2000.get());
}

void NMEAWaypointHandler::MessageHandler(const tN2kMsg &N2kMsg) {
    if (instance) {
        instance->OnN2kMessage(N2kMsg);
    }
}

void NMEAWaypointHandler::OnN2kMessage(const tN2kMsg &N2kMsg) {
    std::ofstream log("/mnt/nvme/logs/pgn_log.txt", std::ios::app);
    log << "Received PGN: " << N2kMsg.PGN << ", LEN: " << N2kMsg.DataLen << std::endl;
    log.close();

    unsigned long PGN = N2kMsg.PGN;
    if (PGN == PGN_WAYPOINT_LIST) {
        handleWaypointList(N2kMsg);
    }
}

void NMEAWaypointHandler::handleWaypointList(const tN2kMsg &N2kMsg) {
    std::cout << "Received waypoint list message." << std::endl;

    // Dump PGN raw data to console
    std::cout << "Raw PGN Data: ";
    for (int i = 0; i < N2kMsg.DataLen; ++i) {
        printf("%02X ", N2kMsg.Data[i]);
    }
    std::cout << std::endl;

    // Log raw PGN data to file
    std::ofstream log("/mnt/nvme/logs/pgn_data_log.txt", std::ios::app);
    log << "PGN: " << N2kMsg.PGN << " | LEN: " << N2kMsg.DataLen << " | DATA: ";
    for (int i = 0; i < N2kMsg.DataLen; ++i) {
        log << std::hex << std::uppercase << (int)N2kMsg.Data[i] << " ";
    }
    log << std::endl;
    log.close();

    // Placeholder values until we parse real data
    std::string name = "PGNWaypoint";
    double lat = 34.1234567;   // Replace with decoded latitude
    double lon = -84.1234567;  // Replace with decoded longitude

    syncManager.syncWaypoint(lat, lon, name);
}


void NMEAWaypointHandler::start() {
    static bool alreadyStarted = false;
    if (!alreadyStarted && nmea2000) {
        std::cout << "Calling Open() on nmea2000 from: " << __FILE__ << ":" << __LINE__ << std::endl;
        nmea2000->Open();
        std::thread([] {
            while (true) {
                NMEA2000.ParseMessages();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }).detach();
        alreadyStarted = true;
    }
}

void NMEAWaypointHandler::addWaypoint(uint16_t waypointID, const std::string& name, double latitude, double longitude) {
    if (waypointMap.find(waypointID) != waypointMap.end()) {
        std::cerr << "Waypoint ID " << waypointID << " already exists. Use updateWaypoint to modify it." << std::endl;
        return;
    }

    waypointMap[waypointID] = {name, {latitude, longitude}};

    // Send PGN (placeholder, customize for each vendor)
    tN2kMsg msg;
    msg.SetPGN(PGN_WAYPOINT_LIST);  // Replace with Garmin or Humminbird PGN when known
    msg.Priority = 3;
    msg.Source = 1;
    msg.Destination = 255; // broadcast to all

    msg.Add8ByteDouble(latitude, 1e-7, N2kDoubleNA);
    msg.Add8ByteDouble(longitude, 1e-7, N2kDoubleNA);
    msg.AddStr(name.c_str(), strlen(name.c_str()), false);

    


    std::cout << "Broadcasting waypoint: " << name << " @ [" << latitude << ", " << longitude << "]" << std::endl;
    digitalWrite(TRANSMITTING_LED_PIN, HIGH);
    nmea2000->SendMsg(msg);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    digitalWrite(TRANSMITTING_LED_PIN, LOW);
}

void NMEAWaypointHandler::enableMockMode(const std::vector<std::string>& devices) {
    mockMode = true;
    mockDevices = devices;
}

const std::vector<std::string>& NMEAWaypointHandler::getDetectedDevices() {
    detectedDevices.clear();

    if (mockMode) {
        detectedDevices = mockDevices;
        return detectedDevices;
    }

    for (uint8_t i = 0; i < deviceList->Count(); ++i) {
        const tNMEA2000::tDevice* device = deviceList->FindDeviceBySource(i);
        if (device) {
            uint64_t deviceName = device->GetName();
            auto it = deviceNameMap.find(deviceName);
            if (it != deviceNameMap.end()) {
                detectedDevices.push_back(it->second);
            }
        }
    }

    return detectedDevices;
}
