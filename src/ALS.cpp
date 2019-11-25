#include "../include/ALS.h"

AlsConfig   AlsConfiguration        = { 200U, 150U, 1000U, 15U, 1U, 350U, 7427U, 1U, 6U };      // AT: Public;  TYPE: AlsConfig
uint16_t    AlsScaleFactor          = 0U;		                                                // AT: Public;  TYPE: FIXED_POINT-1.15
uint16_t    AlsAverageAmbientLight  = 0U;		                                                // AT: Public;  TYPE: UINT16 (IN LUX)

uint16_t alsCalcReflectedLight(uint16_t ambLight);
uint16_t alsCalcLuxRatio(uint16_t silLux, uint16_t roomLux);
uint16_t alsCalcLightIntensity(uint16_t ambLight);

// alsUpdateScaleFactor(): recalculates AlsScaleFactor

void alsUpdateScaleFactor() {
    uint16_t refLight;

    if (AlsConfiguration.ELow >= AlsAverageAmbientLight) {
        refLight = AlsConfiguration.LMin;
    } else if (AlsAverageAmbientLight < AlsConfiguration.EHigh) {
        refLight = alsCalcReflectedLight(AlsAverageAmbientLight);
    } else {
        refLight = AlsConfiguration.LMax;
    }
    
    AlsScaleFactor = (AlsConfiguration.LMax + (refLight << 16)) / (2 * AlsConfiguration.LMax & 0xFFFFU);
}

// alsSetDefaultValues(): initializes AlsScaleFactor and AlsAverageAmbientLight to default values

void alsSetDefaultValues() {
    AlsAverageAmbientLight = 300U;
    AlsScaleFactor = 32768U;
}

uint16_t alsCalcLuxRatio(uint16_t silLux, uint16_t roomLux) {
    if (silLux || roomLux) {
        const uint16_t totalLux = ((2 * roomLux * AlsConfiguration.ReflectionCoeff + 2048) >> 12) + 16 * silLux;
        const uint32_t modifSilLux = (silLux << 20) + ((totalLux + 1) >> 1);
        if (modifSilLux == (uint32_t)(totalLux << 16)) {
            return UINT16_MAX;
        } else {
            return modifSilLux / totalLux;
        }
    }

    return 0;
}

uint16_t alsCalcLightIntensity(uint16_t ambLight) {
    const uint32_t lowLuxRatio = alsCalcLuxRatio(AlsConfiguration.LMin, AlsConfiguration.ELow);
    const uint32_t highLuxRatio = alsCalcLuxRatio(AlsConfiguration.LMax, AlsConfiguration.EHigh);
    
    return (lowLuxRatio * (AlsConfiguration.EHigh - ambLight) + highLuxRatio * (ambLight - AlsConfiguration.ELow) 
        + (((uint16_t)(AlsConfiguration.EHigh - AlsConfiguration.ELow) + 1) >> 1)) 
        / (uint16_t)(AlsConfiguration.EHigh - AlsConfiguration.ELow);
}

uint16_t alsCalcReflectedLight(uint16_t ambLight) {
    uint16_t v2 = alsCalcLightIntensity(ambLight);
    uint16_t v3 = v2 ? -v2 : UINT16_MAX;
    
    return ((v2 * (((uint32_t)AlsConfiguration.ReflectionCoeff * (ambLight << 7) + 32768U) >> 16) + ((v3 + 1) >> 1)) / v3 + 32) >> 6;
}