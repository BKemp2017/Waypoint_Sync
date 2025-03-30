#include "sync_manager.h"
#include "waypoint_converter.h"
#include "nmea_waypoint_handler.h"
#include "nlohmann/json.hpp"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <sys/inotify.h>
#include "NMEA2000_SocketCAN.h"
#include <unistd.h>
#include <vector>
#include <chrono>
#include <thread>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

const std::string waypointFile = "/mnt/nvme/waypoints.json";
std::unordered_map<std::string, std::string> formatMap;
std::unordered_map<std::string, std::time_t> fileTimestamps;

SyncManager::SyncManager() : nmeaHandler(nullptr) {
    initialize(true, true);
}

SyncManager::~SyncManager() {
    if (inotifyFd >= 0) {
        close(inotifyFd);
        std::cout << "inotify file descriptor closed." << std::endl;
    }
}

// Function to reset detection flags
void SyncManager::resetChangeFlags() {
    inotifyChangeDetected = false;
    pollChangeDetected = false;
}

bool SyncManager::isInotifyChangeDetected() {
    return inotifyChangeDetected;
}

bool SyncManager::isPollChangeDetected() {
    return pollChangeDetected;
}

void SyncManager::clearFileTimestamps() {
    fileTimestamps.clear();
}

void SyncManager::loadFormatMappings() {
    std::ifstream configFile("/home/blake/waypoint_sync_project/format_mapping.json");
    if (!configFile.is_open()) {
        std::cerr << "Error opening format_mapping.json" << std::endl;
        return;
    }

    json config;
    configFile >> config;

    for (auto& [key, value] : config.items()) {
        formatMap[key] = value["format_name"];
        std::cout << "Loaded format: " << key << " as " << formatMap[key] << std::endl;
    }
}

void SyncManager::initialize(bool reloadedFormats, bool addWatches) {
    static bool formatsLoaded = false;
    if (reloadedFormats && !formatsLoaded) {
        loadFormatMappings();
        formatsLoaded = true;
    }

    if (!nmeaHandler) {
        nmeaHandler = std::make_shared<NMEAWaypointHandler>(*this, std::make_unique<tNMEA2000_SocketCAN>()); 
    }

    if (nmeaHandler) {
        nmeaHandler->start(); 
    } else {
        std::cerr << "Error: Failed to initialize NMEAWaypointHandler." << std::endl;
        return;
    }

    if (addWatches && inotifyFd < 0) {
        inotifyFd = inotify_init();
        if (inotifyFd < 0) {
            std::cerr << "Error initializing inotify. Exiting..." << std::endl;
            return;
        }

        // addWatch("/path/to/local/storage");
        // addWatch("/path/to/sdcard/path");
        addWatch("/home/blake/waypoint_sync_test_dir");
    }
}


void SyncManager::addWatch(const std::string &path) {
    int watchDescriptor = inotify_add_watch(inotifyFd, path.c_str(), IN_MODIFY | IN_CREATE);
    if (watchDescriptor < 0) {
        std::cerr << "Failed to add inotify watch for path: " << path << std::endl;
    } else {
        std::cout << "Added inotify watch for path: " << path << std::endl;
    }
}

void SyncManager::syncWaypointsOnBoot() {
    std::ifstream file(waypointFile);
    if (file.is_open()) {
        std::cout << "Syncing waypoints from SSD..." << std::endl;
        syncWaypointsAcrossDevices();
    }
}

void SyncManager::checkForChanges() {
    std::cout << "Entering checkForChanges()" << std::endl;

    bool inotifyEvent = checkInotifyChanges(); // Check inotify changes
    if (!inotifyEvent) {
        std::cout << "No inotify event, checking polling..." << std::endl;
        pollForChanges("/home/blake/waypoint_sync_test_dir"); // Check polling changes only if no inotify event
    }

    std::this_thread::sleep_for(std::chrono::seconds(4)); // Optional delay to control frequency
    std::cout << "Exiting checkForChanges()" << std::endl;
}

bool SyncManager::checkInotifyChanges() {
    std::vector<char> buffer(1024);
    ssize_t length = read(inotifyFd, buffer.data(), buffer.size());

    if (length > 0) {
        handleFileChange(buffer.data());
        return true;
    }
    return false;
}

void SyncManager::pollForChanges(const std::string &path) {
    std::cout << "Entering pollForChanges() for path: " << path << std::endl;

    for (const auto &entry : fs::directory_iterator(path)) {
        std::string filepath = entry.path().string();
        std::time_t currentTimestamp = fs::last_write_time(entry).time_since_epoch().count();

        std::cout << "Polling file: " << filepath 
                  << " | Current timestamp: " << currentTimestamp
                  << " | Last known timestamp: " 
                  << (fileTimestamps.count(filepath) ? fileTimestamps[filepath] : 0)
                  << std::endl;

        if (fileTimestamps.find(filepath) == fileTimestamps.end() || fileTimestamps[filepath] != currentTimestamp) {
            std::cout << "Polling detected a change in file: " << filepath << std::endl;
            pollChangeDetected = true;
            syncWaypointsAcrossDevices();
            fileTimestamps[filepath] = currentTimestamp;
        } else { 
            std::cout << "No change detected for: " << filepath << std::endl;
        }
        std::cout << "Exiting pollForChanges()" << std::endl;
    }
}

void SyncManager::handleFileChange(const char *buffer) {
    std::cout << "File change detected by inotify, syncing waypoints..." << std::endl;
    inotifyChangeDetected = true;
    syncWaypointsAcrossDevices();
}

void SyncManager::syncWaypointsAcrossDevices() {
    if (nmeaHandler) {
        const auto& devices = nmeaHandler->getDetectedDevices();
        for (const auto& device : devices) {
            auto it = formatMap.find(device);
            if (it != formatMap.end()) {
                convertToAllFormats("test_waypoint.gpx", "gpx", { {device, it->second} });
            }
            std::cout << "Syncing waypoints to device: " << device << std::endl;  
        }
    } else { 
        std::cerr << "NMEA handler not set." << std::endl;
    }
}

void SyncManager::syncWaypoint(double lat, double lon, const std::string &name) {
    std::cout << "Syncing waypoint: " << name << " [" << lat << ", " << lon << "] across devices." << std::endl;
    nmeaHandler->addWaypoint(nextWaypointId++, name, lat, lon);
}

void SyncManager::setNMEAHandler(std::shared_ptr<NMEAWaypointHandler> handler) {
    nmeaHandler = handler;
}

std::shared_ptr<NMEAWaypointHandler> SyncManager::getNmeaHandler() {
    return nmeaHandler;
}

