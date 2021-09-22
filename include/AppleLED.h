#pragma once

#include <cstdint>
#include <algorithm>
#include "TOD.h"
#include "ALS.h"

typedef uint16_t PWMValue;

enum LmsScaleMode {
	LMS_SCALE_ALS	= 0,							// Ambient Light Sensor autoscale
	LMS_SCALE_TOD	= 1,							// Time Of Day autoscale
	LMS_SCALE_AUTO	= 2,							// Automatic (between ALS and TOD)
	LMS_SCALE_CONST	= 3,							// Static scale
};

enum LmsSelect {
	LMS_OFF				= 0,						// SIL off
	LMS_ON				= 1,						// SIL on, autoscale OK
	LMS_BREATHE			= 2,						// SIL breathing, autoscale OK
	LMS_BRIGHT_NO_SCALE	= 3,						// SIL on bright, no autoscale (for power switch override)
};

struct LmsGeneralConfig {
	PWMValue		ModvBrightnessBreatheMin;		// Breathe dwell PWM setting
	PWMValue		ModvMaxChangePerTick;			// Max PWM change per 1/152 sec
	uint16_t		ScaleConstant;					// Scale constant (1.15 fixed-point representation) if not using ALS or TOD scaling
	LmsScaleMode	ScaleMode;						// Scale by ALS, TOD, or constant
	uint8_t			RampDuration;					// Ramp length (equals 152 * ramp time in seconds)
	bool			PowerSwitchOverridesSIL;		// TRUE if pressing the power switch should force the SIL to full brightness
	uint8_t			MinTicksToTarget;				// Slow the slew rate so that it takes at least this many ticks to reach the target from the prev PWM value.
};

struct LmsDwellConfig {
	uint16_t MidToStartRatio;						// Mid-step size / start-step size
	uint16_t MidToEndRatio;							// Mid-step size / end-step	size
	uint16_t StartTicks;							// # of ticks using start-step size
	uint16_t EndTicks;								// # of ticks using end-step size
};

struct LmsFlareConfig {
	PWMValue ModvFlareCeiling;						// Flare algorithm is active below this value.
	PWMValue ModvMinChange;							// Minimum rate of change while flaring.
	uint16_t FlareAdjust;							// Smaller value causes stronger flare as PWM value descends below modvFlareCeiling.
};

extern LmsGeneralConfig	LmsConfig;					// Provides overall system-specific config info for the SIL.
extern LmsDwellConfig	LmsDwellFadeUpConfig;		// Provides dwell fade-up configuration.
extern LmsDwellConfig	LmsDwellFadeDownConfig;		// Provides dwell fade-down configuration.
extern LmsFlareConfig	LmsFlareFadeUpConfig;		// Provides flare config for non-breathing fade-up.
extern LmsFlareConfig	LmsFlareFadeDownConfig;		// Provides flare config for non-breathing fade-down.

extern PWMValue			LmsPWMOffValueArr;			// SIL's PWM "Off" value (usually 0).
extern PWMValue			LmsPWMOnValueArr;			// SIL's PWM "On" value (varies per system).
extern PWMValue			LmsPWMFullOnValueArr;		// SIL's PWM "Full On" value (usually 0xFFFF, used for power switch override).
extern uint16_t			LmsScaleValue;				// Holds the SIL's per-unit max brightness scale value. A value of 0xFFFF indicates that no scaling will be done for this particular unit.
extern bool				LmsScalingEnabled;			// This flag will normally be 1 (i.e, TRUE), which enables per-unit scaling. Set this flag to 0 (FALSE) to disable per-unit scaling.
extern bool				LmsUseAlsSensorValue;		// Set this flag to 1 in order to enable ALS sensor value reading. Value of 0 will force the use of default value.
extern bool				LmsUseAlsScaleFactor;		// This flag controls whether ALS scale should be used (flag is set to 1) or TOD scale value (flag is set to 0).

// lmsChangeBehavior(): changes state of SIL
// LmsSelect newBehavior: new state
// bool newRampFlag: set to 1 for a slew-rate controlled transition. Set to 0 for a step change.
// bool powerSwitchFlag: set to override current state with LMS_BRIGHT_NO_SCALE

void		lmsChangeBehavior(LmsSelect newBehavior, bool newRampFlag, bool powerSwitchFlag);

// lmsUpdateBreathingParameters(): calculates support values from LmsConfig.ModvBrightnessBreatheMin and LmsConfig.RampDuration,
// and should be called if they've been changed

void		lmsUpdateBreathingParameters();

// lmsResetState(): resets SIL state to default state

void		lmsResetState();

// lmsInit(): initializes LMS

void		lmsInit();

// lmsGetNewPWMValue(): main function for PWM value calculation
// bool powerSwitchFlag: set to override current state with LMS_BRIGHT_NO_SCALE
// return: new PWM value

uint16_t	lmsGetNewPWMValue(bool powerSwitchFlag);
