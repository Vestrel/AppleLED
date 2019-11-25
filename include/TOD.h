#ifndef TOD_H
#define TOD_H

#include <cstdint>

struct TodDailyHoursConfig {
    uint32_t Start;                                 // Start of the daytime in seconds since midnight.
    uint32_t End;                                   // End of the daytime in seconds since midnight.
};

struct TodTimeCLC {
    uint32_t TimeFromMinToMax;                      // Length of night to day transition in seconds.
    uint32_t TimeFromMaxToMin;                      // Length of day to night transition in seconds.
    uint16_t MinScaleVal;                           // Minimum value of TodScaleFactor.
};

extern TodTimeCLC           TodCLCStruct;           // CLC configuration.
extern TodDailyHoursConfig  TodDailyHours;          // DailyHours configuration. Must be initialized before using TOD scaling.
extern uint16_t             TodScaleFactor;         // TOD Scale Factor for SIL in 1.15 fixed-point representation.
 
// todSetTimeOfDay(): updates time of day
// uint32_t time: new time in seconds since midnight

void     todSetTimeOfDay(uint32_t time);

// todGetTimeOfDay(): returns time of day
// return: current time of day since midnight

uint32_t todGetTimeOfDay();

// todScaleFactorUpdate(): recalculates TOD scale factor

void     todScaleFactorUpdate();

#endif  // TOD_H