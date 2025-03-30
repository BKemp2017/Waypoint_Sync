#ifndef NMEA_WAYPOINT_HANDLER_H
#define NMEA_WAYPOINT_HANDLER_H

#include <string>
#include <vector>
#include <memory> 
#include "NMEA2000.h"
#include "N2kMessages.h"
#include "N2kDeviceList.h"
#include <iostream>
#include <unordered_map>

class SyncManager;

class NMEAWaypointHandler {
protected:
    std::unique_ptr<tNMEA2000> nmea2000;
    std::unique_ptr<tN2kDeviceList> deviceList;
    SyncManager& syncManager;
    std::vector<std::string> detectedDevices;
    bool mockMode = false;
    std::vector<std::string> mockDevices;
    static NMEAWaypointHandler* instance;

    void handleWaypointList(const tN2kMsg &N2kMsg);
    void detectConnectedDevices();

private:
    std::unordered_map<uint16_t, std::pair<std::string, std::pair<double, double>>> waypointMap;

public:
    NMEAWaypointHandler(SyncManager& sm, std::unique_ptr<tNMEA2000> nmea2000Instance);
    ~NMEAWaypointHandler();
    
    void convertAndSendWaypoint(const std::string& waypointData, const std::string& format);
    void addWaypoint(uint16_t waypointID, const std::string& name, double latitude, double longitude);
    void updateWaypoint(uint16_t waypointID, const std::string &newName, double latitude, double longitude);
    void start();
    void OnN2kMessage(const tN2kMsg &N2kMsg);
    static void MessageHandler(const tN2kMsg &N2kMsg);
    const std::vector<std::string>& getDetectedDevices();
    void enableMockMode(const std::vector<std::string>& devices);

    void setNMEA2000(std::unique_ptr<tNMEA2000> nmea2000Instance) {
    nmea2000 = std::move(nmea2000Instance);
}
    bool isMockMode() const { return mockMode; }
    friend class NMEAWaypointHandlerTest;
};

#endif // NMEA_WAYPOINT_HANDLER_H
