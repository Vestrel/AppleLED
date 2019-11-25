#include "../include/TOD.h"

TodDailyHoursConfig TodDailyHours       = { 0U, 0U };

TodTimeCLC          TodCLCStruct        = { 3600U, 3600U, 6540U };

static uint32_t     TodTimeOfDayInSecs  = 0U;       // AT: Public;  TYPE: UINT32
uint16_t            TodScaleFactor      = 0U;	    // AT: Public;  TYPE: FIXED_POINT-1.15

// todSetTimeOfDay(): updates time of day
// uint32_t time: new time in seconds since midnight

void todSetTimeOfDay(uint32_t time) {
    TodTimeOfDayInSecs = time % 86400;
}

// todGetTimeOfDay(): returns time of day
// return: current time of day since midnight

uint32_t todGetTimeOfDay() {
    return TodTimeOfDayInSecs;
}

// todScaleFactorUpdate(): recalculates TOD scale factor

void todScaleFactorUpdate() {

    if (TodDailyHours.Start == TodDailyHours.End) {
		return;
	}

    const uint32_t upTransitionEnd = TodDailyHours.Start + TodCLCStruct.TimeFromMinToMax;
    const uint32_t upTransitionEndNorm = (TodDailyHours.Start + TodCLCStruct.TimeFromMinToMax + 1) % 86400U;
    const uint32_t downTransitionEnd = TodDailyHours.End + TodCLCStruct.TimeFromMaxToMin;
    const uint32_t downTransitionEndNorm = (TodDailyHours.End + TodCLCStruct.TimeFromMaxToMin + 1) % 86400U;

	uint32_t dayStart = TodDailyHours.End - 1U;
	if (dayStart < upTransitionEndNorm) {
		dayStart += 86400U;
	}

	uint32_t dayEnd = TodDailyHours.Start - 1U;
	if (dayEnd < downTransitionEndNorm) {
		dayEnd += 86400U;
	}

    const uint32_t invertedScaleVal = 32768U - TodCLCStruct.MinScaleVal;

    if ((TodDailyHours.Start <= TodTimeOfDayInSecs && TodTimeOfDayInSecs <= upTransitionEnd) || (upTransitionEnd >= 86400U && TodTimeOfDayInSecs <= upTransitionEnd % 86400U)) {

        TodScaleFactor = (TodCLCStruct.TimeFromMinToMax - (upTransitionEnd - TodTimeOfDayInSecs)) * invertedScaleVal / TodCLCStruct.TimeFromMinToMax + TodCLCStruct.MinScaleVal;

    } else if ((TodDailyHours.End <= TodTimeOfDayInSecs && TodTimeOfDayInSecs <= downTransitionEnd) || (downTransitionEnd >= 86400U && TodTimeOfDayInSecs <= downTransitionEnd % 86400U)) {

        TodScaleFactor = 32768U - (TodCLCStruct.TimeFromMaxToMin - (downTransitionEnd - TodTimeOfDayInSecs)) * invertedScaleVal / TodCLCStruct.TimeFromMaxToMin;
        
    } else if ((upTransitionEndNorm <= TodTimeOfDayInSecs && TodTimeOfDayInSecs <= dayStart) || (dayStart >= 86400U && TodTimeOfDayInSecs <= dayStart % 86400U)) {

        TodScaleFactor = 32768U;

    } else if ((downTransitionEndNorm <= TodTimeOfDayInSecs && TodTimeOfDayInSecs <= dayEnd) || (dayEnd >= 86400U && TodTimeOfDayInSecs <= dayEnd % 86400U)) {

        TodScaleFactor = TodCLCStruct.MinScaleVal;

    } else {

        TodScaleFactor = (TodCLCStruct.TimeFromMinToMax - (upTransitionEnd - TodTimeOfDayInSecs)) * invertedScaleVal / TodCLCStruct.TimeFromMinToMax + TodCLCStruct.MinScaleVal;

    }
}