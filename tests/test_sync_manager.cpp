#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../src/sync_manager.h"
#include "NMEA2000.h"
#include "mock_nmea2000.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

namespace fs = std::filesystem;

// Utility function to create or modify a test file
void modifyTestFile(const std::string &path) {
    fs::remove(path);
    std::ofstream file(path);
    file << "Waypoint test data" << std::endl;
    file.close();

    // Ensure the file modification time is updated by adding a slight delay
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// Mock class for NMEAWaypointHandler
class MockNMEAWaypointHandler : public NMEAWaypointHandler {
public:
    MockNMEAWaypointHandler(SyncManager& sm)
        : NMEAWaypointHandler(sm, std::make_unique<MockNMEA2000>()) {
        enableMockMode({"Garmin", "Lowrance"});
    }

    MOCK_METHOD(void, addWaypoint, (uint16_t waypointID, const std::string &name, double latitude, double longitude), ());
    MOCK_METHOD(const std::vector<std::string>&, getDetectedDevices, (), ());
    MOCK_METHOD(void, start, (), ());
};

// Define a fixture for SyncManager tests
class SyncManagerTest : public ::testing::Test {
protected:
    std::shared_ptr<MockNMEAWaypointHandler> mockNmeaHandler;
    SyncManager syncManager;

    SyncManagerTest() : mockNmeaHandler(std::make_shared<MockNMEAWaypointHandler>(syncManager)) {
        syncManager.setNMEAHandler(mockNmeaHandler);
        syncManager.initialize();
    }

    void SetUp() override {
        syncManager.clearFileTimestamps();
    }
};

// Test inotify-based change detection
TEST_F(SyncManagerTest, DetectsInotifyChanges) {
    std::string testDir = "/home/blake/waypoint_sync_test_dir";
    std::string testFilePath = testDir + "/test_waypoint_change.gpx";

    // Create test directory and file
    fs::create_directory(testDir);
    modifyTestFile(testFilePath);

    // Add watch on the temporary directory
    syncManager.addWatch(testDir);

    // Simulate a file change to trigger inotify
    modifyTestFile(testFilePath);

    // Check if inotify detected the change
    syncManager.checkForChanges();
    EXPECT_TRUE(syncManager.isInotifyChangeDetected());

    // Reset flags and ensure inotify is cleared
    syncManager.resetChangeFlags();
    EXPECT_FALSE(syncManager.isInotifyChangeDetected());

    // Clean up
    fs::remove(testFilePath);
    fs::remove(testDir);
}

// Test polling-based change detection
TEST_F(SyncManagerTest, DetectsPollingChanges) {
    std::string testDir = "/home/blake/waypoint_sync_test_dir";
    std::string testFilePath = testDir + "/test_waypoint_change.gpx";

    // Create test directory and file
    fs::create_directory(testDir);
    modifyTestFile(testFilePath);

    // Disable inotify by closing its file descriptor
    close(syncManager.getInotifyFdForTesting());
    syncManager.setInotifyFdForTesting(-1);

    // Simulate a file change to trigger polling
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait for polling interval
    modifyTestFile(testFilePath);

    // Check if polling detected the change
    syncManager.checkForChanges();
    EXPECT_TRUE(syncManager.isPollChangeDetected());

    // Reset flags and ensure polling is cleared
    syncManager.resetChangeFlags();
    EXPECT_FALSE(syncManager.isPollChangeDetected());

    // Reinitialize inotify for further tests
    syncManager.initialize(false, true);

    // Clean up
    fs::remove(testFilePath);
    fs::remove(testDir);
}
