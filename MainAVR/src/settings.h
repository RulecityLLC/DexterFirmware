#ifndef SETTINGS_H
#define SETTINGS_H

#ifdef __GNUC__
#include <stdint.h>
#else
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;
#endif //
#include <ldp-abst/ldpc.h>

// The number of bytes in the hello packet payload before the VBI data.
// Change this when the hello command data's size changes
#define SETTINGS_BYTES_BEFORE_VBI_PATTERN	18

typedef enum
{
	LDP_NONE = 0,	// used to designate that the player type must be auto-detected
	LDP_LDV1000 = 1,
	LDP_LDV1000_BL = 2,
	LDP_PR7820 = 3,
	LDP_VP931 = 4,
	LDP_PR8210 = 5,
	LDP_PR8210A = 6,
	LDP_VP932 = 7,
	LDP_VP380 = 8,
	LDP_SIMUTREK = 9,
	LDP_LDV8000 = 10,
	LDP_LDP1450 = 11,
	LDP_LDP1000A = 12,
	LDP_VIP9500SG = 13,
	LDP_LD700 = 14,
	LDP_OTHER = 32	// this enum is useful to trigger auto-detection of which adapter is plugged in
} LDPType;

typedef enum
{
	AUTODETECT_NONE = 0,	// no auto-detect suggestions
	AUTODETECT_LDV1000_OR_PR7820 = 0x81
} AutoDetectStrategy;

void load_settings_from_eeprom();
void apply_new_settings(uint8_t u8OverwriteEeprom, const void *pSrcBuf, uint16_t stBufSize);

// save settings in RAM to eeprom. (used by deferred eeprom writer)
uint8_t save_current_settings_to_eeprom();

// returns 1 if settings were saved or 0 if buffer is too big
uint8_t save_settings_to_eeprom(const void *pSrcBuf, uint16_t stBufSize);

// common init stuff that happens after media server settings are in RAM
void post_apply_settings();

uint8_t IsSpinupDelayEnabledEeprom();
LDPCDiscType_t GetDiscTypeEeprom();
LDPType GetManualLDPTypeEeprom();	// whatever player would be manually selected if auto-detection was disabled, value saved to eeprom
LDPType GetAutodetectedLDPType();	// saved only in ram
LDPType GetActiveLDPType();	// whatever player is currently active, after auto-detection/manual rules are applied
void SetAutodetectedLDPType(LDPType type);
void SetActiveLDPType(LDPType type);
uint16_t GetPersistentIdentifierEeprom();
uint16_t GetDynamicIdentifierEeprom();
uint8_t IsSearchDelayEnabledEeprom();
uint8_t IsSearchDelayEnabledMemory();
void SetSearchDelayEnabledMemory(uint8_t bEnabled);
uint8_t IsSearchBlankingEnabledEeprom();
uint8_t IsDiagnosticsEnabledEeprom();
uint8_t IsVideoMuteDuringSearchEnabledEeprom();
uint8_t Is4800BaudEnabledEeprom();
uint8_t IsAutoDetectionEnabledEeprom();
AutoDetectStrategy GetAutoDetectStrategyEeprom();
uint8_t GetDefaultDiscIdEeprom();
const uint8_t *GetAvailableDiscIdsEeprom();	// returns pointer to array of 3 bytes with null termination

uint8_t GetActiveDiscIdMemory();
void SetActiveDiscIdMemory(uint8_t id);

uint8_t IsSuperModeEnabledMemory();
void OnSuperModeChanged(uint8_t bEnabled);

// whether or not stop codes are honored by the current laserdisc player type
uint8_t GetHonorStopCodesMemory();
void SetHonorStopCodesMemory(uint8_t bHonored);

// callback for when mode button is pressed
void OnModePressed();

// callback for when mode button is held down for a while
void OnModeHeld();

// used to set the LDP type if it has been auto detected
void SetAutoDetectedLDPType(LDPType t);

// callback for when diagnose button is pressed
void OnDiagnosePressed();

// forces bootloader to go into reprogramming mode
void ForceReprogram();

#endif //  SETTINGS_H
