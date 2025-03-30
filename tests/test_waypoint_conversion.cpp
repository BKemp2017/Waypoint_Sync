#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include "../src/waypoint_converter.h"
#include "../src/sync_manager.h"
#include <unordered_map>
#include <iostream>

// Mock map format 
std::unordered_map<std::string, std::string> createFormatMap() {
    return {
        {"usr", "lowranceusr"},
        {"hwr", "humminbird"},
        {"gpx", "gpx"}
    };
}

TEST_CASE("Test waypoint conversion from GPX to USR", "[waypoint_conversion]") {
    std::unordered_map<std::string, std::string> formatMap = createFormatMap();
    std::cout << "Running Test Case: GPX to USR" << std::endl;
    std::string input = "./test_waypoint.gpx";
    std::string output = convertWaypoint(input, "usr", formatMap);

    REQUIRE(output == "test_output.usr");
    std::remove(output.c_str());
}

TEST_CASE("Test waypoint conversion from GPX to HWR", "[waypoint_conversion]") {
    std::unordered_map<std::string, std::string> formatMap = createFormatMap();
    std::cout << "Running Test Case: GPX to HWR" << std::endl;
    std::string input = "./test_waypoint.gpx";
    std::string output = convertWaypoint(input, "hwr", formatMap);

    REQUIRE(output == "test_output.hwr");
    std::remove(output.c_str());
}

TEST_CASE("Test waypoint conversion from USR to GPX", "[waypoint_conversion]") {
    std::unordered_map<std::string, std::string> formatMap = createFormatMap();
    std::cout << "Running Test Case: USR to GPX" << std::endl;
    std::string input = "./test_waypoint.usr";
    std::string output = "test_output.gpx";

    bool success = convertWaypointFile(input, output, formatMap["usr"], formatMap["gpx"]);

    REQUIRE(success);
    REQUIRE(output == "test_output.gpx");
}

TEST_CASE("Test waypoint conversion from HWR to GPX", "[waypoint_conversion]") {
    std::unordered_map<std::string, std::string> formatMap = createFormatMap();
    std::cout << "Running Test Case: HWR to GPX" << std::endl;
    std::string input = "./test_waypoint.hwr";
    std::string output = "test_output.gpx";

    bool success = convertWaypointFile(input, output, formatMap["hwr"], formatMap["gpx"]);

    REQUIRE(success);
    REQUIRE(output == "test_output.gpx");
}

TEST_CASE("Test waypoint conversion from HWR to USR", "[waypoint_conversion]") {
    std::unordered_map<std::string, std::string> formatMap = createFormatMap();
    std::cout << "Running Test Case: HWR to USR" << std::endl;
    std::string input = "./test_waypoint.hwr";
    std::string output = "test_output.usr";

    bool success = convertWaypointFile(input, output, formatMap["hwr"], formatMap["usr"]);

    REQUIRE(success);
    REQUIRE(output == "test_output.usr");
}

TEST_CASE("Test convertToAllFormats function", "[convert_all_formats]") {
    std::unordered_map<std::string, std::string> formatMap = createFormatMap();
    std::cout << "Running Test Case: Convert GPX to all Formats" << std::endl;
    std::string input = "./test_waypoint.gpx";
    
    convertToAllFormats(input, "gpx", formatMap);

    REQUIRE(convertWaypointFile(input, "output.usr", formatMap["gpx"], formatMap["usr"]) == true);
    REQUIRE(convertWaypointFile(input, "output.hwr", formatMap["gpx"], formatMap["hwr"]) == true);
}