# Define directories
SRC_DIR := src
TEST_DIR := /home/blake/waypoint_sync_project/tests
BUILD_DIR := build
NMEA2000_LIB_DIR := /home/blake/waypoint_sync_project/NMEA2000
NMEA2000_SOCKETCAN_LIB_DIR := /home/blake/waypoint_sync_project/NMEA2000_socketCAN
JSON_LIB_DIR := /home/blake/waypoint_sync_project/json/include

# Define compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -g -Wall -Wextra \
            -I/usr/src/googletest/include \
            -I$(NMEA2000_LIB_DIR)/src \
            -I$(NMEA2000_SOCKETCAN_LIB_DIR) \
            -I$(SRC_DIR) \
            -I$(TEST_DIR) \
            -I$(JSON_LIB_DIR)

LDFLAGS := -L/usr/src/googletest/lib \
           -lgtest -lgmock -lgtest_main \
           -lpthread \
           -L$(NMEA2000_LIB_DIR) \
           -L$(NMEA2000_SOCKETCAN_LIB_DIR) \
           -lNMEA2000 -lNMEA2000_socketCAN \
           -lwiringPi

# Define test executables
TEST_EXECUTABLES := build/test_nmea_waypoint_handler \
                   build/test_sync_manager \
                   build/test_waypoint_conversion \
                   build/test_led

# Default target
all: $(TEST_EXECUTABLES)

# Add a target for the main executable
build/main: build/main.o build/nmea_waypoint_handler.o build/sync_manager.o build/waypoint_converter.o
	$(CXX) $^ -o $@ $(LDFLAGS)

# Compile main.cpp
build/main.o: $(SRC_DIR)/main.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build each test executable
build/test_nmea_waypoint_handler: build/test_nmea_waypoint_handler.o build/nmea_waypoint_handler.o build/sync_manager.o build/waypoint_converter.o
	$(CXX) $^ -o $@ $(LDFLAGS)

build/test_sync_manager: build/test_sync_manager.o build/nmea_waypoint_handler.o build/sync_manager.o build/waypoint_converter.o
	$(CXX) $^ -o $@ $(LDFLAGS)

build/test_waypoint_conversion: build/test_waypoint_conversion.o build/nmea_waypoint_handler.o build/sync_manager.o build/waypoint_converter.o
	$(CXX) $^ -o $@ $(LDFLAGS)

build/test_led: build/test_led.o
	$(CXX) $^ -o $@ -lwiringPi $(LDFLAGS)

# Compile source files to object files
build/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile test files to object files
build/%.o: $(TEST_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Ensure build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Clean build files
clean:
	rm -rf $(BUILD_DIR)/*.o $(TEST_EXECUTABLES)

# Run all tests
test: $(TEST_EXECUTABLES)
	@echo "Running all tests..."
	@for test in $(TEST_EXECUTABLES); do \
		./$$test --gtest_color=yes; \
	done

# Run specific test
test_%: build/test_%
	./build/test_$* --gtest_color=yes

.PHONY: all clean test
