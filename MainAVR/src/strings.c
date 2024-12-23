#include "protocol.h"
#include "strings.h"
#include <avr/pgmspace.h>

// IMPORTANT: keep these strings shorter than the buffer in log_string!!
const char strDefault[] PROGMEM = "Default ";
const char strSettingsApplied[] PROGMEM = "Settings applied";
const char strUnsupported[] PROGMEM = "Unsupported";
const char strLD700[] PROGMEM = "LD700 ";
const char strLDP1000A[] PROGMEM = "LDP-1000A ";
const char strLDP1450[] PROGMEM = "LDP-1450 ";
const char strLDV1000[] PROGMEM = "LD-V1000 ";
const char strLDV1000Super[] PROGMEM = "LD-V1000 (super) ";
const char strPR7820[] PROGMEM = "PR-7820 ";
const char strPR8210[] PROGMEM = "PR-8210 ";
const char strPR8210A[] PROGMEM = "PR-8210A ";
const char strBadlands[] PROGMEM = "Badlands ";
const char strVIP9500SG[] PROGMEM = "VIP9500SG ";
const char strVP932[] PROGMEM = "VP932 ";
const char strStart[] PROGMEM = "start";
const char strAmHere[] PROGMEM = "AM HERE!";
const char strError[] PROGMEM = "Error";
const char strStopped[] PROGMEM = "Stopped";
const char strSearching[] PROGMEM = "Searching";
const char strPaused[] PROGMEM = "Paused";
const char strSpinningUp[] PROGMEM = "Spinning Up";
const char strPlaying[] PROGMEM = "Playing";
const char strCmdPlay[] PROGMEM = "Cmd: Play";
const char strCmdPause[] PROGMEM = "Cmd: Pause";
const char strCmdStop[] PROGMEM = "Cmd: Stop";
const char strCmdStep[] PROGMEM = "Cmd: Still-Step";
const char strCmdSeek[] PROGMEM = "Cmd: Seek to frame %u";	// NOTE : this will assume frame number is 16-bit so will display incorrect results for any number over 2^16
const char strLengthMismatch[] PROGMEM = "Length mismatch";
const char strCRCError[] PROGMEM = "CRC error";
const char strSaveSettingsFailed[] PROGMEM = "Save settings failed";
const char strUnimplemented[] PROGMEM = "Unimplemented";
const char strCriticalSectionViolation[] PROGMEM = "Crit section fail";
const char strUnknownPacket[] PROGMEM = "Unknown packet: ";
const char strIncomingByte[] PROGMEM = "<- %x";
const char strOutgoingByte[] PROGMEM = "-> %x";
const char strUnknownByte[] PROGMEM = "Unknown byte: %x";
const char strUnsupportedByte[] PROGMEM = "Unsupported:  %x";
const char strTooManyDigits[] PROGMEM = "Too many digits ";
const char strUnhandledSituation[] PROGMEM = "Unhandled situa.";
const char strAUXPacketTooBig[] PROGMEM = "AUX packet too big";
const char strAUXMismatch[] PROGMEM = "AUX Length mismatch";
const char strAUXCRCError[] PROGMEM = "AUX CRC error";
const char strAUXUnknownPacket[] PROGMEM = "Unknown AUX packet: ";
const char strCorrupt16BitInput[] PROGMEM = "Corrupt input: %02x";
const char strSearchDone[] PROGMEM = "Search done";
const char strSearchFailed[] PROGMEM = "Search failed";
const char strBaudRate[] PROGMEM = "Baud rate set to: %04u";
const char strOutgoingStatus[] PROGMEM = "-> %02x%02x%02x %02x%02x%02x";
const char strIncomingCmd[] PROGMEM = "<- %02x%02x%02x";
const char strEventMissed[] PROGMEM = "Event missed";
const char strAutoDetected[] PROGMEM = "Auto-detected ";
const char strAutoDetectionUnavailable[] PROGMEM = "Auto-detection unavailable";
const char strEEPROMWritten[] PROGMEM = "New settings saved to EEPROM";
const char strDiscSwitchFailed[] PROGMEM = "Disc switch failed";
const char strDiscSwitchUnknown[] PROGMEM = "Unknown switch code %02x";
const char strALGMultiRomDetected[] PROGMEM = "ALG multi detected";

PGM_P const string_table[] PROGMEM = 
{
    strDefault,
    strSettingsApplied,
    strUnsupported,
	strLD700,
	strLDP1000A,
	strLDP1450,
    strLDV1000,
	strLDV1000Super,
	strPR7820,
	strPR8210,
	strPR8210A,
	strBadlands,
	strVIP9500SG,
	strVP932,
	strStart,
	strAmHere,
	strError,
	strStopped,
	strSearching,
	strPaused,
	strSpinningUp,
	strPlaying,
	strCmdPlay,
	strCmdPause,
	strCmdStop,
	strCmdStep,
	strCmdSeek,
	strLengthMismatch,
	strCRCError,
	strSaveSettingsFailed,
	strUnimplemented,
	strCriticalSectionViolation,
	strUnknownPacket,
	strIncomingByte,
	strOutgoingByte,
	strUnknownByte,
	strUnsupportedByte,
	strTooManyDigits,
	strUnhandledSituation,
	strAUXPacketTooBig,
	strAUXMismatch,
	strAUXCRCError,
	strAUXUnknownPacket,
	strCorrupt16BitInput,
	strSearchDone,
	strSearchFailed,
	strBaudRate,
	strOutgoingStatus,
	strIncomingCmd,
	strEventMissed,
	strAutoDetected,
	strAutoDetectionUnavailable,
	strEEPROMWritten,
	strDiscSwitchFailed,
	strDiscSwitchUnknown,
	strALGMultiRomDetected
};

void string_to_buf(char *s, StringID id)
{
	strcpy_P(s, (PGM_P)pgm_read_word(&(string_table[id])));
}

void log_string(StringID id)
{
	char s[30];	// should be as small as possible
	string_to_buf(s, id);
	LOG(s);
}
