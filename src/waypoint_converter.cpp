#include "waypoint_converter.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <unordered_map>

// Utility function to check file existence
bool checkFileExists(const std::string &filePath) {
    std::ifstream fileCheck(filePath);
    if (!fileCheck.good()) {
        std::cerr << "Error: Input File " << filePath << " does not exist or cannot be opened." << std::endl;
        return false;
    }
    return true;
}

// Function to convert a waypoint file from one format to another using formatMap
std::string convertWaypoint(const std::string &input, const std::string &format, const std::unordered_map<std::string, std::string> &formatMap) {
    auto formatIt = formatMap.find(format);
    if (formatIt == formatMap.end()) {
        std::cerr << "Error: Unsupported format " << format << std::endl;
        return "";
    }

    std::string output = "test_output." + format;
    return convertWaypointFile(input, output, "gpx", formatIt->second) ? output : "";
}

// Function to convert waypoint files across formats, using formatMap
bool convertWaypointFile(const std::string &inputFile, const std::string &outputFile, const std::string &inputFormat, const std::string &outputFormat) {
    if (!checkFileExists(inputFile)) return false;

    std::string command = "gpsbabel -i " + inputFormat + " -f " + inputFile + " -o " + outputFormat + " -F " + outputFile;
    std::cout << "Running command: " << command << std::endl;

    int result = std::system(command.c_str());
    if (result != 0) {
        std::cerr << "gpsbabel command exited with an error." << std::endl;
        return false;
    }

    std::cout << "Successfully converted " << inputFile << " to " << outputFile << std::endl;
    return true;
}

// Function to convert input waypoint file to all other formats, using formatMap
void convertToAllFormats(const std::string &inputFile, const std::string &inputFormat, const std::unordered_map<std::string, std::string> &formatMap) {
    // Use the formatMap to find the corresponding GPSBabel format
    std::string inputFormatMapped;
    if (inputFormat == "gpx") {
        inputFormatMapped = "gpx"; // explicitly map "gpx" to "gpx" format
    } else {
        auto inputFormatMappedIt = formatMap.find(inputFormat);
        if (inputFormatMappedIt == formatMap.end()) {
            std::cerr << "Error: Unsupported input format " << inputFormat << std::endl;
            return;
        }
        inputFormatMapped = inputFormatMappedIt->second;
    }

    std::cout << "Input format found in map: " << inputFormatMapped << std::endl;

    for (const auto &[outputFormat, gpsBabelFormat] : formatMap) {
        if (outputFormat != inputFormat) { // Skip converting to the same format
            std::string outputFile = "output." + outputFormat;
            std::cout << "Converting " << inputFile << " to format " << gpsBabelFormat << " as " << outputFile << std::endl;
            convertWaypointFile(inputFile, outputFile, inputFormatMapped, gpsBabelFormat);
        }
    }
}

