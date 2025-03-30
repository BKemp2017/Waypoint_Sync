#ifndef WAYPOINT_CONVERTER_H
#define WAYPOINT_CONVERTER_H

#include <string>
#include <unordered_map>

std::string convertWaypoint(const std::string& input, const std::string& format, const std::unordered_map<std::string, std::string>& formatMap);
bool convertWaypointFile(const std::string &inputFile, const std::string &outputFile, const std::string &inputFormat, const std::string &outputFormat);
void convertToAllFormats(const std::string &inputFile, const std::string &inputFormat, const std::unordered_map<std::string, std::string>& formatMap);

// Optionally include this if `checkFileExists` elsewhere
// bool checkFileExists(const std::string &filePath);

#endif

