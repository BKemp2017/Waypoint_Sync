#ifndef SYNC_MANAGER_H
#define SYNC_MANAGER_H

#include <unordered_map>
#include <string>
#include <ctime>
#include <memory>
#include "nmea_waypoint_handler.h" 

class SyncManager {
public:
    SyncManager();
    ~SyncManager();
    
    void checkForChanges();
    bool checkInotifyChanges();
    void checkPollingChanges(const std::string &path);
    void loadFormatMappings();
    void initialize(bool reloadedFormats = true, bool addWatches = true);
    void addWatch(const std::string &path);
    void syncWaypointsOnBoot();
    void resetChangeFlags();
    void clearFileTimestamps();
    bool isInotifyChangeDetected();
    bool isPollChangeDetected();
    void syncWaypointsAcrossDevices();
    void syncWaypoint(double lat, double lon, const std::string &name);
    void setNMEAHandler(std::shared_ptr<NMEAWaypointHandler> handler);  

    std::shared_ptr<NMEAWaypointHandler> getNmeaHandler();

    int getInotifyFdForTesting() const { return inotifyFd; }
    void setInotifyFdForTesting(int fd) { inotifyFd = fd; }

private:
    int inotifyFd = -1;  
    bool inotifyChangeDetected = false; 
    bool pollChangeDetected = false;
    uint16_t nextWaypointId = 1000;
    void handleFileChange(const char *buffer); 
    void pollForChanges(const std::string &path); 
    std::shared_ptr<NMEAWaypointHandler> nmeaHandler;

};


#endif // SYNC_MANAGER_H
