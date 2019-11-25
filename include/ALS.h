#ifndef ALS_H
#define ALS_H

#include <cstdint>

struct AlsConfig {
    uint16_t AlsI2CTimeInteval;                 // Int interval (ms) for ALS I2C task.
    uint16_t AlsADCTimeInteval;                 // Int interval (ms) for ALS ADC ISR.
    uint16_t LMax;                              // Maximum cd/m^2 for SIL.
    uint16_t LMin;                              // Minimum cd/m^2 for SIL.
    uint16_t ELow;                              // Low room illum threshold (lux).
    uint16_t EHigh;                             // High room illum threshold (lux).
    uint16_t ReflectionCoeff;                   // Bezel reflection coefficient.
    uint8_t  AlsSensorNum;                      // Actual number of ALS sensors in system.
    uint8_t  LidDelay;                          // Delay after lid opens (in tenths of seconds) during which ALS readings don't affect the SIL.
};

extern AlsConfig    AlsConfiguration;
extern uint16_t     AlsScaleFactor;
extern uint16_t     AlsAverageAmbientLight;

// alsUpdateScaleFactor(): recalculates AlsScaleFactor

void     alsUpdateScaleFactor();

// alsSetDefaultValues(): initializes AlsScaleFactor and AlsAverageAmbientLight to default values

void     alsSetDefaultValues();

#endif  // ALS_H