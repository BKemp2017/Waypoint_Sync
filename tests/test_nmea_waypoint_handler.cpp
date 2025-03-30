#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <wiringPi.h>
#include "NMEA2000.h"
#include "nmea_waypoint_handler.h"
#include "sync_manager.h"
#include "mock_nmea2000.h"

// GPIO pin definitions (WiringPi pin numbers)
#define POWER_LED_PIN 7            // GPIO 4 (Blue LED for Power)
#define LISTENING_LED_PIN 3        // GPIO 22 (Green LED for Listening)
#define TRANSMITTING_LED_PIN 0     // GPIO 17 (Yellow LED for Transmitting)
#define ERROR_LED_PIN 2            // GPIO 27 (Red LED for Error)

// Mock class for SyncManager
class MockSyncManager : public SyncManager {
public:
    MOCK_METHOD(void, syncWaypoint, (double lat, double lon, const std::string &name));
};

// Mock class for NMEAWaypointHandler with simulated devices
class MockNMEAWaypointHandler : public NMEAWaypointHandler {
public: 
    MockNMEAWaypointHandler(SyncManager& sm, std::unique_ptr<tNMEA2000> nmea2000Instance)
        : NMEAWaypointHandler(sm, std::move(nmea2000Instance)) {
        enableMockMode({"Garmin", "Lowrance"});
    }


    // MOCK_METHOD(std::vector<std::string>&, getDetectedDevices, (), (override));
    MOCK_METHOD(void, addWaypoint, (uint16_t waypointID, const std::string &name, double latitude, double longitude));
    MOCK_METHOD(void, updateWaypoint, (uint16_t waypointID, const std::string &newName, double latitude, double longitude));
    // Directly implement getDetectedDevices to return mockDevices
    std::vector<std::string> mockDevices = { "Garmin", "Lowrance" };
    
};

// Define a fixture for the NMEAWaypointHandler tests
class NMEAWaypointHandlerTest : public ::testing::Test {
protected:
    MockNMEA2000* mockNMEA2000;                       // Raw pointer for mock
    MockSyncManager mockSyncManager;                 // Mock SyncManager
    std::unique_ptr<NMEAWaypointHandler> nmeaHandler;

void SetUp() override {
        // Initialize WiringPi
        if (wiringPiSetup() == -1) {
            std::cerr << "WiringPi initialization failed" << std::endl;
            FAIL() << "Failed to initialize WiringPi";
        }

                // Set GPIO pin modes for LEDs
        pinMode(POWER_LED_PIN, OUTPUT);
        pinMode(LISTENING_LED_PIN, OUTPUT);
        pinMode(TRANSMITTING_LED_PIN, OUTPUT);
        pinMode(ERROR_LED_PIN, OUTPUT);

        // Turn on Power LED to indicate the test setup is starting
        digitalWrite(POWER_LED_PIN, HIGH);
         // Delay for 2 seconds

        // Create a mock instance as a unique_ptr
        auto mockInstance = std::make_unique<MockNMEA2000>();
        mockNMEA2000 = mockInstance.get(); 

        // Pass the unique_ptr to NMEAWaypointHandler's constructor
        nmeaHandler = std::make_unique<NMEAWaypointHandler>(mockSyncManager, std::move(mockInstance));

        // Set up expectations on the mock instance before calling start
        EXPECT_CALL(*mockNMEA2000, Open()).Times(1);
        EXPECT_CALL(*mockNMEA2000, SendMsg(::testing::_)).Times(::testing::AnyNumber());

        // Enable mock mode and start the handler
        nmeaHandler->enableMockMode({"Garmin", "Lowrance"});
        nmeaHandler->start();
        
    }

    void TearDown() override {

        // Turn on Error LED to indicate teardown is happening (for testing purposes)
        digitalWrite(ERROR_LED_PIN, HIGH);
        delay(500);
        digitalWrite(ERROR_LED_PIN, LOW);
        digitalWrite(POWER_LED_PIN, LOW);

        // Reset the handler and mock pointer to clean up after tests
        nmeaHandler.reset();
        mockNMEA2000 = nullptr;
    }
};



// Test for verifying device detection
TEST_F(NMEAWaypointHandlerTest, DetectsDevicesCorrectly) {
    // Turn on Listening LED to indicate device detection is being tested
    digitalWrite(LISTENING_LED_PIN, HIGH);
    delay(1000);

    const auto& devices = nmeaHandler->getDetectedDevices();

    EXPECT_FALSE(devices.empty());
    EXPECT_EQ(devices.size(), 2);
    EXPECT_EQ(devices[0], "Garmin");
    EXPECT_EQ(devices[1], "Lowrance");

    // Turn off Listening LED after test is done
    digitalWrite(LISTENING_LED_PIN, LOW);

    testing::Mock::AllowLeak(mockNMEA2000);
}

// Test for adding a waypoint
TEST_F(NMEAWaypointHandlerTest, AddsWaypointCorrectly) {
    std::vector<std::tuple<uint16_t, std::string, double, double>> waypoints = {
        {101, "Waypoint 1", 37.7749, -122.4194},
        {102, "Waypoint 2", 40.7128, -74.0060},
        {103, "Waypoint 3", 34.0522, -118.2437}, 
        {104, "Waypoint 4", 41.8781, -87.6298},
        {105, "Waypoint 5", 29.7604, -95.3698},
        {106, "Waypoint 6", 32.7767, -96.7970},
        {107, "Waypoint 7", 39.9526, -75.1652},
        {108, "Waypoint 8", 33.4484, -112.0740},
        {109, "Waypoint 9", 37.7749, -122.4194},
        {110, "Waypoint 10", 40.7128, -74.0060}
    };

    // Set up a single EXPECT_CALL for all iterations
    EXPECT_CALL(*mockNMEA2000, SendMsg(::testing::Field(&tN2kMsg::PGN, 130074))).Times(waypoints.size());

    for (const auto& [waypointID, name, latitude, longitude] : waypoints) {

        digitalWrite(TRANSMITTING_LED_PIN, HIGH);
        delay(200);

        auto start = std::chrono::high_resolution_clock::now();
        nmeaHandler->addWaypoint(waypointID, name, latitude, longitude);
        auto end = std::chrono::high_resolution_clock::now();

        // Turn off the Transmitting LED after transmission
        digitalWrite(TRANSMITTING_LED_PIN, LOW);
        delay(200); 

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "Added " << name << " in " << duration << " ms" << std::endl;

    }

    // Turn off Transmitting LED after waypoints are added
    digitalWrite(TRANSMITTING_LED_PIN, LOW);
}

// Test for handling waypoint list message
TEST_F(NMEAWaypointHandlerTest, HandlesWaypointListMessage) {
    // Turn on Listening LED to indicate the message handler is being tested
    digitalWrite(LISTENING_LED_PIN, HIGH);


    tN2kMsg receivedMsg;
    receivedMsg.PGN = 130074;

    nmeaHandler->OnN2kMessage(receivedMsg);
    
    // Turn off Listening LED after message handling is complete
    digitalWrite(LISTENING_LED_PIN, LOW);

}
