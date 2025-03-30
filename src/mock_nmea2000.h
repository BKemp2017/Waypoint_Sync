#ifndef MOCK_NMEA2000_H
#define MOCK_NMEA2000_H

#include "NMEA2000.h"
#include <gmock/gmock.h>

// Mock class for tNMEA2000
class MockNMEA2000 : public tNMEA2000 {
public:
    MOCK_METHOD(void, Open, ());
    MOCK_METHOD(bool, SendMsg, (const tN2kMsg &N2kMsg));
    MOCK_METHOD(void, SetMsgHandler, (void (*MsgHandler)(const tN2kMsg &N2kMsg)));
    MOCK_METHOD(void, SetProductInformation,
                (const char *SerialCode, unsigned short ProductCode, 
                 const char *ModelID, const char *SoftwareVersion, const char *ModelVersion));
    MOCK_METHOD(void, SetDeviceInformation, 
                (unsigned long UniqueNumber, unsigned char DeviceFunction, 
                 unsigned char DeviceClass, unsigned short ManufacturerCode));

    // Add mock implementations of the pure virtual functions in tNMEA2000
    MOCK_METHOD(bool, CANSendFrame, (unsigned long id, unsigned char len, const unsigned char *buf, bool rtr), (override));
    MOCK_METHOD(bool, CANOpen, (), (override));
    MOCK_METHOD(bool, CANGetFrame, (unsigned long &id, unsigned char &len, unsigned char *buf), (override));
};

#endif // MOCK_NMEA2000_H