#include "../include/AppleLED.h"

using namespace std;

struct LmsStateConfig {
	uint16_t			CycleCounterMaxVal;
	uint16_t			CycleCounter;
	uint16_t			ModeSwitchTick;
	uint16_t			BreathingCounterStartVal;
	uint16_t			TotalBreathingTicks;
	LmsStateConfig *	NewModeStructAddr;
	uint16_t *			MaxPWMValPtr;
	uint16_t			(*FuncLink)(uint16_t cycleIdx);
	uint8_t				Unused;
};

static uint16_t lmsGetTargetVal(uint16_t cycleIdx);

static const uint16_t		highCycle0[]						= { 0U, 273U, 5139U, 207U, 111U, 47U, 17U, 5U, 1U, 0U };
static const uint16_t		highCycle1[]						= { 5801U, 12003U, 7311U, 1753U, 935U, 399U, 142U, 43U, 12U, 3U, 1U, 0U };
static const uint16_t		highCycle2[]						= { 28402U, 38898U, 25666U, 14811U, 7902U, 3373U, 1200U, 366U, 98U, 23U, 5U, 1U, 0U };
static const uint16_t		lowCycle0[]							= { 0U, 418U, 12020U, 742U, 605U, 395U, 215U, 100U, 41U, 15U, 5U, 1U, 0U };
static const uint16_t		lowCycle1[]							= { 14558U, 33605U, 29175U, 19407U, 15837U, 10339U, 5625U, 2623U, 1070U, 388U, 127U, 38U, 10U, 3U, 1U, 0U };
static const uint16_t *		highCycle[]							= { highCycle0, highCycle1, highCycle2 };
static const uint16_t *		lowCycle[]							= { lowCycle0, lowCycle1 };

LmsGeneralConfig			LmsConfig							= { 200U, 400U, 32768U, LMS_SCALE_AUTO, 2U, 0U, 32U };	// AT: Public; TYPE: LmsGeneralConfig
LmsDwellConfig				LmsDwellFadeUpConfig				= { 3U, 2U, 20U, 3U };									// AT: Public; TYPE: LmsDwellConfig
LmsDwellConfig				LmsDwellFadeDownConfig				= { 2U, 3U, 3U, 25U };									// AT: Public; TYPE: LmsDwellConfig
LmsFlareConfig				LmsFlareFadeUpConfig				= { 0U, 4U, 140U };										// AT: Public; TYPE: LmsFlareConfig
LmsFlareConfig				LmsFlareFadeDownConfig				= { 0U, 4U, 140U };										// AT: Public; TYPE: LmsFlareConfig
PWMValue					LmsPWMOffValueArr					= { 0U };												// AT: Public; TYPE: { UINT16 }
PWMValue					LmsPWMOnValueArr					= { UINT16_MAX };										// AT: Public; TYPE: { UINT16 }
PWMValue					LmsPWMFullOnValueArr				= { UINT16_MAX };										// AT: Public; TYPE: { UINT16 }

static LmsStateConfig		LmsOffStateConfig					= { 1U,		0U, 0U, 0U,		760U, &LmsOffStateConfig,			&LmsPWMOffValueArr,		NULL,				1U };
static LmsStateConfig		LmsOnStateConfig					= { 1U,		0U, 0U, 274U,	760U, &LmsOnStateConfig,			&LmsPWMOnValueArr,		NULL,				1U };
static LmsStateConfig		LmsBreathingStateConfig				= { 761U,	0U, 0U, 0U,		760U, &LmsBreathingStateConfig,		NULL,					lmsGetTargetVal,	1U };
static LmsStateConfig		LmsBrightNoScaleStateConfig			= { 1U,		0U, 0U, 684U,	684U, &LmsBrightNoScaleStateConfig,	&LmsPWMFullOnValueArr,	NULL,				0U };

static uint16_t				LmsMinBrightness					= 0U;		// AT: Private; TYPE: UINT16
static uint16_t				LmsRampStep							= 0U;		// AT: Private; TYPE: UINT16
static uint16_t				LmsScaleParam						= 0U;		// AT: Private; TYPE: FIXED_POINT-1.15
static uint16_t				LmsBreathingModeSwitchDelayTimer	= 0U;		// AT: Private; TYPE: UINT16
static LmsStateConfig *		LmsCurrentStateConfig				= nullptr;	// AT: Private; TYPE: LmsStateConfig *
uint16_t					LmsScaleValue						= 0U;		// AT: Public;  TYPE: UINT16

bool						LmsUseAlsSensorValue				= false;	// AT: Public;  TYPE: BOOL
bool						LmsScalingEnabled					= false;	// AT: Public;  TYPE: BOOL
bool						LmsUseAlsScaleFactor				= false;	// AT: Public;  TYPE: BOOL
static bool					LmsShouldCalcRampVal				= false;	// AT: Private; TYPE: BOOL
static bool					LmsRampFlag							= false;	// AT: Private; TYPE: BOOL

// lmsChangeBehavior(): changes state of SIL
// LmsSelect newBehavior: new state
// bool newRampFlag: set if additional adjustments should be made
// bool powerSwitchFlag: set to override current state with LMS_BRIGHT_NO_SCALE

void lmsChangeBehavior(LmsSelect newBehavior, bool newRampFlag, bool powerSwitchFlag) {
	LmsSelect behavior = newBehavior;
	bool rampFlag = newRampFlag;

	if (LmsConfig.PowerSwitchOverridesSIL && powerSwitchFlag) {
		behavior = LMS_BRIGHT_NO_SCALE;
		rampFlag = false;
	}

	if ((behavior == LMS_BREATHE) && (LmsCurrentStateConfig == &LmsBreathingStateConfig)) {
		return;
	}

	switch (behavior) {
		case LMS_OFF:
			LmsOffStateConfig.NewModeStructAddr = &LmsOffStateConfig;
			LmsCurrentStateConfig->NewModeStructAddr = &LmsOffStateConfig;
			LmsBreathingStateConfig.ModeSwitchTick = LmsOffStateConfig.TotalBreathingTicks;
			break;
		case LMS_ON:
			LmsOnStateConfig.NewModeStructAddr = &LmsOnStateConfig;
			LmsCurrentStateConfig->NewModeStructAddr = &LmsOnStateConfig;
			LmsBreathingStateConfig.ModeSwitchTick = LmsOnStateConfig.TotalBreathingTicks;
			break;
		case LMS_BREATHE:
			LmsBreathingStateConfig.CycleCounter = LmsCurrentStateConfig->BreathingCounterStartVal;
			LmsBreathingStateConfig.NewModeStructAddr = &LmsBreathingStateConfig;
			LmsCurrentStateConfig->NewModeStructAddr = &LmsBreathingStateConfig;
			LmsBreathingStateConfig.ModeSwitchTick = LmsBreathingStateConfig.TotalBreathingTicks;
			break;
		case LMS_BRIGHT_NO_SCALE:
			LmsBrightNoScaleStateConfig.NewModeStructAddr = &LmsBrightNoScaleStateConfig;
			LmsCurrentStateConfig->NewModeStructAddr = &LmsBrightNoScaleStateConfig;
			LmsBreathingStateConfig.ModeSwitchTick = LmsBrightNoScaleStateConfig.TotalBreathingTicks;
			break;
		default:
			break;
	}

	LmsRampFlag = rampFlag;
}

// lmsTargetValCalc(): support function for initial PWM calculation

static uint16_t lmsTargetValCalc(const uint16_t *selectedArray, uint16_t base) {
	uint32_t result = selectedArray[0] << 16;
	uint16_t arrIdx = 1U;

	for (uint16_t modif = base; modif != 0U; modif = (modif * base + 32768U) >> 16) {
		const uint16_t currentElement = selectedArray[arrIdx++];
		if (currentElement == 0U) {
			break;
		}
		result += currentElement * modif;
	}

	return (result + 32768U) >> 16;
}

// lmsGetTargetVal(): support function for initial PWM calculation

static uint16_t lmsGetTargetVal(uint16_t cycleIdx) {

	const uint32_t base = (55188U * cycleIdx + 64U) >> 7;

	if (cycleIdx < 259U) {
		return lmsTargetValCalc(lowCycle[base >> 16], base);
	} else if (cycleIdx < 289U) {
		return 65534U;
	} else if (cycleIdx < 684U) {
		return lmsTargetValCalc(highCycle[(0x48000U - base) >> 16], 32768U - base);
	}

	return 0U;
}

// lmsApplyMinBreatheScale(): support function for adding minimal value to PWM value

static uint16_t lmsApplyMinBreatheScale(uint16_t val) {
	if (LmsConfig.ModvBrightnessBreatheMin) {
		return LmsConfig.ModvBrightnessBreatheMin + ((val * LmsMinBrightness + 32768U) >> 16);
	}
	return val;
}

// lmsGetBreatheValAndApplyMinBrightness(): combines lmsGetTargetVal() and lmsApplyMinBreatheScale()

static uint16_t lmsGetBreatheValAndApplyMinBrightness(uint16_t cycleIdx) {
	return lmsApplyMinBreatheScale(lmsGetTargetVal(cycleIdx));
}

// lmsGetScaleVal(): updates scaling value for PWM scaling according to LmsConfig.ScaleMode

static void lmsUpdateScaleVal() {
	switch (LmsConfig.ScaleMode) {
		case LMS_SCALE_ALS:
			LmsScaleParam = AlsScaleFactor;
			return;
		case LMS_SCALE_TOD:
			LmsScaleParam = TodScaleFactor;
			return;
		case LMS_SCALE_AUTO:
			LmsScaleParam = LmsUseAlsScaleFactor ? AlsScaleFactor : TodScaleFactor;
			return;
		case LMS_SCALE_CONST:
			LmsScaleParam = LmsConfig.ScaleConstant;
			return;
		default:
			return;
	}
}

// lmsUpdateBreathingParameters(): calculates support values from LmsConfig.ModvBrightnessBreatheMin and LmsConfig.RampDuration,
// and should be called if they've been changed

void lmsUpdateBreathingParameters() {
	if (LmsConfig.ModvBrightnessBreatheMin) {
		LmsMinBrightness = (((UINT16_MAX - static_cast<uint32_t>(LmsConfig.ModvBrightnessBreatheMin)) << 16) + 32768U) / UINT16_MAX;
		LmsBreathingStateConfig.FuncLink = lmsGetBreatheValAndApplyMinBrightness;
	} else {
		LmsBreathingStateConfig.FuncLink = lmsGetTargetVal;
	}

	LmsRampStep = LmsConfig.RampDuration ? (((LmsConfig.RampDuration + 1U) >> 1) + UINT16_MAX) / LmsConfig.RampDuration : UINT16_MAX;
}

// lmsResetState(): resets SIL state to default state

void lmsResetState() {
	AlsAverageAmbientLight = 0U;
	AlsScaleFactor = 0U;
	LmsUseAlsSensorValue = false;
	LmsUseAlsScaleFactor = false;
	LmsCurrentStateConfig = &LmsOffStateConfig;
	LmsScalingEnabled = true;

	lmsUpdateScaleVal();
	lmsChangeBehavior(LMS_OFF, true, false);
}

// lmsInit(): initializes LMS

void lmsInit() {
	TodScaleFactor = 32768U;
	LmsScaleValue = UINT16_MAX;

	lmsUpdateBreathingParameters();
	lmsResetState();
}

// lmsCalcFlareAdjustment(): calculate flare value
// uint16_t targetVal: target value
// uint16_t prevVal: previous value
// uint16_t *valToModify: value to modify

static void lmsCalcFlareAdjustment(uint16_t targetVal, uint16_t prevVal, uint16_t *valToModify) {

	const LmsFlareConfig *flareConfig = targetVal <= prevVal ? &LmsFlareFadeDownConfig : &LmsFlareFadeUpConfig;
	if (flareConfig->ModvFlareCeiling > prevVal) {
		const uint16_t flareAdjustment = (flareConfig->ModvFlareCeiling - prevVal) / flareConfig->FlareAdjust;
		*valToModify = flareAdjustment <= *valToModify ? max(flareConfig->ModvMinChange, static_cast<uint16_t>(*valToModify - flareAdjustment)) : flareConfig->ModvMinChange;
	}
}

// lmsSlowSlewRate(): slow the slew rate so that it takes at least LmsConfig.MinTicksToTarget to reach the target from the prev PWM value
// uint16_t targetVal: target value
// uint16_t prevVal: previous value
// uint16_t *valToModify: value to modify

static void lmsSlowSlewRate(uint16_t targetVal, uint16_t prevVal, uint16_t *valToModify) {

	if (LmsConfig.MinTicksToTarget > 1U) {
		uint16_t targetValDiff;
		if (targetVal >= prevVal) {
			targetValDiff = targetVal - prevVal;
		} else {
			targetValDiff = prevVal - targetVal;
		}
		const uint16_t stepPerTick = (targetValDiff >= 2U * LmsConfig.MinTicksToTarget) ? (targetValDiff / LmsConfig.MinTicksToTarget) : 1U;
		*valToModify = min(stepPerTick, *valToModify);
	}
}

// lmsCalcRampVal(): support function for additional PWM adjustment

static uint16_t lmsCalcRampVal(uint16_t targetVal, uint16_t cycleIdx) {

	static uint16_t LmsLastRampVal = 0U;

	if (LmsShouldCalcRampVal && LmsLastRampVal != targetVal) {
		uint16_t valIncr = 0;

		if (cycleIdx < 684U) {

			const uint16_t ambLight = LmsUseAlsSensorValue ? AlsAverageAmbientLight : 300U;
			const uint16_t refLight = max((ambLight * AlsConfiguration.ReflectionCoeff + 32768U) >> 16, 6U);
			const uint16_t refToMaxLightRatio = (refLight + (AlsConfiguration.LMax << 8)) / (2U * refLight);
			uint32_t maxLightAvailable = ((UINT16_MAX - LmsLastRampVal) * refToMaxLightRatio + 0x1008000U) >> 16;
			maxLightAvailable *= maxLightAvailable;
			uint16_t rampTargetVal;
			if (maxLightAvailable >= 0xFFFF80U) {
				rampTargetVal = (refToMaxLightRatio + ((((maxLightAvailable + 32768U) >> 16) + 1U) >> 1) + 256U) / ((maxLightAvailable + 32768U) >> 16);
			} else {
				rampTargetVal = ((refToMaxLightRatio << 8) + 65536U + ((((maxLightAvailable + 128U) >> 8) + 1U) >> 1)) / ((maxLightAvailable + 128U) >> 8);
			}
			rampTargetVal = max(rampTargetVal, static_cast<uint16_t>(1));
			const uint32_t stepPerRampTick = (rampTargetVal * LmsRampStep + 128U) >> 8;
			valIncr = min(stepPerRampTick, static_cast<uint32_t>(LmsConfig.ModvMaxChangePerTick));

			// Do a flare

			lmsCalcFlareAdjustment(targetVal, LmsLastRampVal, &valIncr);

			// Do a slew rate slowdown

			lmsSlowSlewRate(targetVal, LmsLastRampVal, &valIncr);

		} else {

			static uint16_t LmsRampEndIncrTarget	= 0U;
			static uint16_t LmsRampStartIncr		= 0U;
			static uint16_t LmsRampMidIncr			= 0U;
			static uint16_t LmsRampEndIncr			= 0U;
			static uint16_t LmsRampStartTick		= 0U;
			static bool		LmsRampIncrDir			= false;

			if (cycleIdx == 684U) {
				LmsDwellConfig *dwellConfig;
				uint16_t targetValDiff;
				LmsRampIncrDir = targetVal > LmsLastRampVal;
				if (LmsRampIncrDir) {
					dwellConfig = &LmsDwellFadeUpConfig;
					targetValDiff = targetVal - LmsLastRampVal;
				} else {
					dwellConfig = &LmsDwellFadeDownConfig;
					targetValDiff = LmsLastRampVal - targetVal;
				}
				LmsRampStartTick = dwellConfig->StartTicks + 684U;
				const uint16_t freeTicks = LmsCurrentStateConfig->TotalBreathingTicks - LmsRampStartTick - dwellConfig->EndTicks;
				LmsRampMidIncr =	static_cast<uint32_t>(dwellConfig->MidToStartRatio * dwellConfig->MidToEndRatio * targetValDiff) /
									static_cast<uint16_t>(dwellConfig->MidToStartRatio * dwellConfig->MidToEndRatio * freeTicks +
															dwellConfig->EndTicks * dwellConfig->MidToStartRatio +
															dwellConfig->StartTicks * dwellConfig->MidToEndRatio)
									+ 1U;
				LmsRampStartIncr = LmsRampMidIncr / dwellConfig->MidToStartRatio + 1U;
				LmsRampEndIncr = LmsRampMidIncr / dwellConfig->MidToEndRatio + 1U;
				const uint16_t totalEndIncr = dwellConfig->EndTicks * LmsRampEndIncr;
				LmsRampEndIncrTarget = LmsRampIncrDir ? targetVal - totalEndIncr : targetVal + totalEndIncr;
			}

			if (LmsRampStartTick > cycleIdx) {
				valIncr = LmsRampStartIncr;
			} else if (LmsRampIncrDir ? LmsRampEndIncrTarget >= LmsLastRampVal : LmsRampEndIncrTarget <= LmsLastRampVal) {
				valIncr = LmsRampMidIncr;
			} else {
				valIncr = LmsRampEndIncr;
			}
		}

		const uint16_t maxVal = UINT16_MAX - valIncr > LmsLastRampVal ? (LmsLastRampVal + valIncr) : UINT16_MAX;
		const uint16_t minVal = LmsLastRampVal > valIncr ? (LmsLastRampVal - valIncr) : 0U;

		if (targetVal <= minVal) {
			LmsLastRampVal = minVal;
		} else if (maxVal < targetVal) {
			LmsLastRampVal = maxVal;
		} else {
			LmsLastRampVal = targetVal;
		}

	} else {
		LmsLastRampVal = targetVal;
	}

	return LmsLastRampVal;
}

// lmsGetNewPWMValue(): main function for PWM value calculation
// bool powerSwitchFlag: set to override current state with LMS_BRIGHT_NO_SCALE

uint16_t lmsGetNewPWMValue(bool powerSwitchFlag) {
	uint16_t newPWMVal = 0U;
	LmsStateConfig *lmsStateConfig = LmsCurrentStateConfig;

	// Decrement breathing state switch delay timer (if we switch from breathing state to any other - we will get 1 sec delay
	// to switch back to breathing state)

	if (LmsBreathingModeSwitchDelayTimer) {
		LmsBreathingModeSwitchDelayTimer--;
	}

	if (LmsCurrentStateConfig) {
		const uint16_t cycleIdx = LmsCurrentStateConfig->CycleCounter++;

		// Get new base PWM value

		if (lmsStateConfig->CycleCounterMaxVal) {
			if (lmsStateConfig->MaxPWMValPtr) {
				newPWMVal = lmsStateConfig->MaxPWMValPtr[cycleIdx];
			} else if (lmsStateConfig->FuncLink) {
				newPWMVal = lmsStateConfig->FuncLink(cycleIdx);
			}
		}

		// If SIL isn't being overridden and current state isn't LMS_BRIGHT_NO_SCALE then try to scale PWM value

		if (!((LmsConfig.PowerSwitchOverridesSIL && powerSwitchFlag) || (lmsStateConfig == &LmsBrightNoScaleStateConfig))) {
			if ((LmsConfig.ScaleMode != LMS_SCALE_ALS || LmsUseAlsSensorValue) && LmsScaleParam) {
				if (LmsScaleParam < 32768U) {
					newPWMVal = (newPWMVal * LmsScaleParam + 16384U) >> 15;
				}
			} else {
				newPWMVal = 0U;
			}
		}

		// If current state is breathing - allow second half of ramp calculation

		if (lmsStateConfig == &LmsBreathingStateConfig) {
			LmsShouldCalcRampVal = cycleIdx >= 684U;
		}

		newPWMVal = lmsCalcRampVal(newPWMVal, cycleIdx);

		// Reset cycle counter

		if (lmsStateConfig->CycleCounterMaxVal <= lmsStateConfig->CycleCounter) {
			lmsStateConfig->CycleCounter = 0U;
		}

		// If switch to new state requested and current cycle is a switch cycle - perform switch and if previous state was breathing,
		// then set LmsBreathingModeSwitchDelayTimer

		if (lmsStateConfig->NewModeStructAddr != lmsStateConfig &&
			((!LmsBreathingModeSwitchDelayTimer && lmsStateConfig->CycleCounter == lmsStateConfig->ModeSwitchTick) || (LmsConfig.PowerSwitchOverridesSIL && powerSwitchFlag))) {
			if (LmsCurrentStateConfig == &LmsBreathingStateConfig) {
				LmsBreathingModeSwitchDelayTimer = 304U;
			}
			LmsCurrentStateConfig = lmsStateConfig->NewModeStructAddr;
		}

		// If cycle is correct or if SIL mode has been updated - update scaling factor

		if ((lmsStateConfig == &LmsBreathingStateConfig ? cycleIdx == 683U : !lmsStateConfig->CycleCounter) || LmsCurrentStateConfig != lmsStateConfig) {
			lmsUpdateScaleVal();
		}

		// If SIL mode has been updated - change ramp calculation state

		if (LmsCurrentStateConfig != lmsStateConfig) {
			LmsShouldCalcRampVal = LmsRampFlag;
		}
	}

	// Do a static scaling of the final PWM value

	if (LmsScalingEnabled && newPWMVal && LmsScaleValue != UINT16_MAX) {
		newPWMVal = (newPWMVal * LmsScaleValue + 32768U) >> 16;
	}

	return newPWMVal;
}
