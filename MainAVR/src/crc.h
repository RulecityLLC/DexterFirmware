
#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
#else
#include <stdint.h>
#endif

void crc_ccitt_init(void);

void crc_ccitt_update(unsigned char x);

uint16_t crc_ccitt_crc(void);

#ifdef __cplusplus
}
#endif
