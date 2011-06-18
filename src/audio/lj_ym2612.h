#ifndef LJ_YM2612_HH
#define LJ_YM2612_HH

typedef enum LJ_YM2612_RESULT { 
	LJ_YM2612_OK = 0,
	LJ_YM2612_ERROR = -1
} LJ_YM2612_RESULT;

enum LJ_YM2612_FLAGS { 
	LJ_YM2612_DEBUG =							(1<<0),
	LJ_YM2612_NODAC =							(1<<1),
	LJ_YM2612_NOFM =							(1<<2),
	LJ_YM2612_ONECHANNEL =				(1<<3),
	LJ_YM2612_ONECHANNEL_SHIFT =	(4),
	LJ_YM2612_ONECHANNEL_MASK =		(0x7),
	LJ_YM2612_NEXT_DEBUG_THINGY =	(1<<7)
};

typedef struct LJ_YM2612 LJ_YM2612;

typedef unsigned char LJ_YM_UINT8;
typedef unsigned short LJ_YM_UINT16;
typedef unsigned int LJ_YM_UINT32;

#define LJ_YM2612_DEFAULT_CLOCK_RATE (7670453)

typedef short LJ_YM_INT16;

LJ_YM2612* LJ_YM2612_create(const int clockRate, const int outputSampleRate);

LJ_YM2612_RESULT LJ_YM2612_setFlags(LJ_YM2612* const ym2612Ptr, const unsigned int flags);

LJ_YM2612_RESULT LJ_YM2612_destroy(LJ_YM2612* const ym2612Ptr);

/* To set a value on the data pins D0-D7 - use for register address and register data value */
/* call setAddressPinsCSRDWRA1A0 to copy the data in D0-D7 to either the register address or register data setting */
LJ_YM2612_RESULT LJ_YM2612_setDataPinsD07(LJ_YM2612* const ym2612Ptr, LJ_YM_UINT8 data);

/* To read a value on the data pins D0-D7 - use to get back the status register */
/* call setAddressPinsCSRDWRA1A0 to put the status register output onto the pins */
LJ_YM2612_RESULT LJ_YM2612_getDataPinsD07(LJ_YM2612* const ym2612Ptr, LJ_YM_UINT8* const data);

/* To write data must have: notCS = 0, notRD = 1, notWR = 0 */
/* A1 = 0, A0 = 0 : D0-D7 is latched as the register address for part 0 i.e. Genesis memory address 0x4000 */
/* A1 = 0, A0 = 1 : D0-D7 is written to the latched register address for part 0 i.e. Genesis memory address 0x4001 */
/* A1 = 1, A0 = 0 : D0-D7 is latched as the register address for part 1 i.e. Genesis memory address 0x4002 */
/* A1 = 1, A0 = 1 : D0-D7 is written to the latched register address for part 1 i.e. Genesis memory address 0x4003 */
/* To read data must have: notCS = 0, notRD = 0, notWR = 1 */
/* A1 = 0, A0 = 0 : D0-D7 will then contain status 0 value - timer A */
/* A1 = 1, A0 = 0 : D0-D7 will then contain status 1 value - timer B */
/* Nemesis forum posts say any combination of A1/A0 work and just return the same status value */
LJ_YM2612_RESULT LJ_YM2612_setAddressPinsCSRDWRA1A0(LJ_YM2612* const ym2612Ptr, LJ_YM_UINT8 notCS, LJ_YM_UINT8 notRD, LJ_YM_UINT8 notWR, 
																						 				LJ_YM_UINT8 A1, LJ_YM_UINT8 A0);

LJ_YM2612_RESULT LJ_YM2612_generateOutput(LJ_YM2612* const ym2612Ptr, int numCycles, LJ_YM_INT16* output[2]);

#endif /* #ifndef LJ_YM2612_HH */
