#include "lj_ym2612.h"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <math.h>

#ifndef M_PI
#define M_PI (3.14159265358979323846f)
#endif
/*
Registers
		D7		D6		D5		D4		D3			D2			D1		D0
22H										LFO enable	LFO frequency
24H		Timer 		A 			MSBs
25H 	#######################################################	Timer A LSBs
26H		Timer 		B
27H		Ch3 mode		Reset B	Reset A	Enable B	Enable A	Load B	Load A
28H		Slot    						###########	Channel
29H	
2AH		DAC
2BH		DAC en	###########################################################
30H+	#######	DT1						MUL
40H+	#######	TL
50H+	RS				#######	AR
60H+	AM		#######	D1R
70H+	###############	D2R
80H+	D1L								RR
90H+	###############################	SSG-EG
A0H+	Frequency 		number 			LSB
A4H+	###############	Block						Frequency Number MSB
A8H+	Ch3 supplementary frequency number
ACH+	#######	Ch3 supplementary block				Ch3 supplementary frequency number
B0H+	#######	Feedback							Algorithm
B4H+	L		R		AMS				###########	FMS
*/

/*
* ////////////////////////////////////////////////////////////////////
* //
* // Internal data & functions
* //
* ////////////////////////////////////////////////////////////////////
*/

typedef struct LJ_YM2612_PART LJ_YM2612_PART;
typedef struct LJ_YM2612_CHANNEL LJ_YM2612_CHANNEL;
typedef struct LJ_YM2612_SLOT LJ_YM2612_SLOT;
typedef struct LJ_YM2612_TIMER LJ_YM2612_TIMER;
typedef struct LJ_YM2612_EG LJ_YM2612_EG;
typedef struct LJ_YM2612_LFO LJ_YM2612_LFO;
typedef struct LJ_YM2612_CHANNEL2_SLOT_DATA LJ_YM2612_CHANNEL2_SLOT_DATA;

#define LJ_YM2612_NUM_REGISTERS (0xFF)
#define LJ_YM2612_NUM_PARTS (2)
#define LJ_YM2612_NUM_CHANNELS_PER_PART (3)
#define LJ_YM2612_NUM_CHANNELS_TOTAL (LJ_YM2612_NUM_CHANNELS_PER_PART * LJ_YM2612_NUM_PARTS)
#define LJ_YM2612_NUM_SLOTS_PER_CHANNEL (4)

typedef enum LJ_YM2612_REGISTERS {
		LJ_LFO_EN_LFO_FREQ = 0x22,
		LJ_TIMER_A_MSB = 0x24,
		LJ_TIMER_A_LSB = 0x25,
		LJ_TIMER_B = 0x26,
		LJ_CH2_MODE_TIMER_CONTROL = 0x27,
		LJ_KEY_ONOFF = 0x28,
		LJ_DAC = 0x2A,
		LJ_DAC_EN = 0x2B,
		LJ_DETUNE_MULT = 0x30,
		LJ_TOTAL_LEVEL = 0x40,
		LJ_RATE_SCALE_ATTACK_RATE = 0x50,
		LJ_AM_DECAY_RATE = 0x60,
		LJ_SUSTAIN_RATE = 0x70,
		LJ_SUSTAIN_LEVEL_RELEASE_RATE = 0x80,
		LJ_EG_SSG_PARAMS = 0x90,
		LJ_FREQLSB = 0xA0,
		LJ_BLOCK_FREQMSB = 0xA4,
		LJ_CH2_FREQLSB = 0xA8,
		LJ_CH2_BLOCK_FREQMSB = 0xAC,
		LJ_FEEDBACK_ALGO = 0xB0,
		LJ_LR_AMS_PMS = 0xB4
} LJ_YM2612_REGISTERS;

static const char* LJ_YM2612_REGISTER_NAMES[LJ_YM2612_NUM_REGISTERS];
static LJ_YM_UINT8 LJ_YM2612_validRegisters[LJ_YM2612_NUM_REGISTERS];

/* Global fixed point scaling for outputs of things */
#define LJ_YM2612_GLOBAL_SCALE_BITS (16)

/* FREQ scale = 16.16 - used global scale to keep things consistent */
#define LJ_YM2612_FREQ_BITS (LJ_YM2612_GLOBAL_SCALE_BITS)
#define LJ_YM2612_FREQ_MASK ((1 << LJ_YM2612_FREQ_BITS) - 1)

/* Volume scale = xx.yy (-1->1) */
#define LJ_YM2612_VOLUME_SCALE_BITS (13)

/* Number of bits to use for scaling output e.g. choose an output of 1.0 -> 13-bits (8192) */
#define LJ_YM2612_OUTPUT_VOLUME_BITS (13)
#define LJ_YM2612_OUTPUT_SCALE (LJ_YM2612_VOLUME_SCALE_BITS - LJ_YM2612_OUTPUT_VOLUME_BITS)

/* Max volume is -1 -> +1 */
#define LJ_YM2612_VOLUME_MAX (1<<LJ_YM2612_VOLUME_SCALE_BITS)

/* Sin output scale = xx.yy (-1->1) */
#define LJ_YM2612_SIN_SCALE_BITS (LJ_YM2612_GLOBAL_SCALE_BITS)

/* TL table scale - matches volume scale */
#define LJ_YM2612_TL_SCALE_BITS (LJ_YM2612_VOLUME_SCALE_BITS)

/* SL table scale - must match volume scale */
#define LJ_YM2612_SL_SCALE_BITS (LJ_YM2612_VOLUME_SCALE_BITS)

/* DAC is in 7-bit scale (-128->128) so this converts it to volume_scale bits */
#define LJ_YM2612_DAC_SHIFT (LJ_YM2612_VOLUME_SCALE_BITS - 7)

/* FNUM = 11-bit table */
#define LJ_YM2612_FNUM_TABLE_BITS (11)
#define LJ_YM2612_FNUM_TABLE_NUM_RAW_ENTRIES (1 << LJ_YM2612_FNUM_TABLE_BITS)
/* Maximum PMS scale is 80 cents = 80/1200 = 1/15 : /10 to be safe */
#define LJ_YM2612_FNUM_TABLE_PMS_DELTA_ENTRIES (LJ_YM2612_FNUM_TABLE_NUM_RAW_ENTRIES/10)
#define LJ_YM2612_FNUM_TABLE_NUM_ENTRIES (LJ_YM2612_FNUM_TABLE_NUM_RAW_ENTRIES+LJ_YM2612_FNUM_TABLE_PMS_DELTA_ENTRIES)
static int LJ_YM2612_fnumTable[LJ_YM2612_FNUM_TABLE_NUM_ENTRIES+1];

/* SIN table = 10-bit table but stored in LJ_YM2612_SIN_SCALE_BITS format */
#define LJ_YM2612_SIN_TABLE_BITS (10)
#define LJ_YM2612_SIN_TABLE_NUM_ENTRIES (1 << LJ_YM2612_SIN_TABLE_BITS)
#define LJ_YM2612_SIN_TABLE_MASK ((1 << LJ_YM2612_SIN_TABLE_BITS) - 1)
static int LJ_YM2612_sinTable[LJ_YM2612_SIN_TABLE_NUM_ENTRIES];

/* Right shift for delta phi = LJ_YM2612_VOLUME_SCALE_BITS - LJ_YM2612_SIN_TABLE_BITS - 2 */
/* >> LJ_YM2612_VOLUME_SCALE_BITS = to remove the volume scale in the output from the sin table */
/* << LJ_YM2612_SIN_TABLE_BITS = to put the output back into the range of the sin table */
/* << 2 = *4 to convert FM output of -1 -> +1 => -4 -> +4 => -PI (and a bit) -> +PI (and a bit) */
#define LJ_YM2612_DELTA_PHI_SCALE (LJ_YM2612_VOLUME_SCALE_BITS - LJ_YM2612_SIN_TABLE_BITS -2)

/* DETUNE table = this are integer freq shifts in the frequency integer scale */
/* In the docs (YM2608) : the base scale is (0.052982) this equates to: (8*1000*1000/(1*1024*1024))/144 */
/* For YM2612 (Genesis) it would equate to: (7670453/(1*1024*1024)/144 => 0.050799 */
/* 144 is the number clock cycles the chip takes to make 1 sample:  */
/* 144 = 6 channels x 4 operators x 8 cycles per operator  */
/* 144 = 6 channels x 24 cycles per channel */
/* 144 = also the prescaler: YM2612 is 1/6 : one sample for each 24 FM clocks */
#define LJ_YM2612_NUM_KEYCODES (1<<5)
#define LJ_YM2612_NUM_FDS (4)
static const LJ_YM_UINT8 LJ_YM2612_detuneScaleTable[LJ_YM2612_NUM_FDS*LJ_YM2612_NUM_KEYCODES] = {
/* FD=0 : all 0 */
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* FD=1 */
0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2,
2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 8, 8, 8,
/* FD=2 */
1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5,
5, 6, 6, 7, 8, 8, 9,10,11,12,13,14,16,16,16,16,
/* FD=3 */
2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7,
8, 8, 9,10,11,12,13,14,16,17,19,20,22,22,22,22,
};

/* Convert detuneScaleTable into a frequency shift (+ and -) */
static int LJ_YM2612_detuneTable[2*LJ_YM2612_NUM_FDS*LJ_YM2612_NUM_KEYCODES];

/* fnum -> keycode table for computing N3 & N4 */
/* N4 = Fnum Bit 11  */
/* N3 = Fnum Bit 11 * (Bit 10 | Bit 9 | Bit 8) | !Bit 11 * Bit 10 * Bit 9 * Bit 8 */
/* Table = (N4 << 1) | N3 */
static const LJ_YM_UINT8 LJ_YM2612_fnumKeycodeTable[16] = {
0, 0, 0, 0, 0, 0, 0, 1,
2, 3, 3, 3, 3, 3, 3, 3,
};

/* The slots referenced in the registers are not 0,1,2,3 *sigh* */
static LJ_YM_UINT8 LJ_YM2612_slotTable[LJ_YM2612_NUM_SLOTS_PER_CHANNEL] = { 0, 2, 1, 3 };

#define LJ_YM2612_EG_ATTENUATION_DB_NUM_BITS (10)
#define LJ_YM2612_EG_ATTENUATION_DB_TABLE_BITS (LJ_YM2612_EG_ATTENUATION_DB_NUM_BITS)
#define LJ_YM2612_EG_ATTENUATION_TABLE_NUM_ENTRIES (1 << LJ_YM2612_EG_ATTENUATION_DB_TABLE_BITS)
#define LJ_YM2612_EG_ATTENUATION_DB_TABLE_MASK (LJ_YM2612_EG_ATTENUATION_TABLE_NUM_ENTRIES - 1)

#define LJ_YM2612_EG_ATTENUATION_DB_MIN (0)
#define LJ_YM2612_EG_ATTENUATION_DB_MAX (LJ_YM2612_EG_ATTENUATION_TABLE_NUM_ENTRIES - 1)
#define LJ_YM2612_EG_SSG_ATTENUATION_DB_MAX (0x200)

/* Convert EG attenuation into linear volume */
/* EG attenuation log scale of 10-bits, TL is 7-bits and is 2^(-1/8), move to 10-bits gives 2^(-1/(8*8)) */
/* Each step is x0.989228013 */
static int LJ_YM2612_EG_attenuationTable[LJ_YM2612_EG_ATTENUATION_TABLE_NUM_ENTRIES];

/* TL - table (7-bits): 0->0x7F : output = 2^(-TL/8) */
/* From the docs each step is -0.75dB = x0.9172759 = 2^(-1/8) */
/* Use the EG attenuation table but shift up */
#define LJ_YM2612_TL_EG_ATTENUATION_TABLE_SHIFT (3)

/* EG circuit timer units fixed point */
#define LJ_YM2612_EG_TIMER_NUM_BITS (16)

/* From looking at the forums */
#define LJ_YM2612_EG_TIMER_OUTPUT_PER_FM_SAMPLE (3.0f)

/* Timer A & Timer B units fixed point */
#define LJ_YM2612_TIMER_NUM_BITS (16)

/* LFO timer units fixed point */
#define LJ_YM2612_LFO_TIMER_NUM_BITS (16)

/* LFO frequencies */
/*	LFO freq	0				1				2				3				4				5				6				7				*/
/*						3.98Hz	5.56Hz	6.02Hz	6.37Hz	6.88Hz	9.63Hz	48.1Hz	72.2Hz	*/
/* With an 8Mhz clock (to match docs which give LFO frequencies) and 144 cycles per FM sample this gives: */
/*	LFO freq	3.98	5.56	6.02	6.37	6.88	9.63	48.1	72.2	Hz	*/
/* 						109		78		72		68		63		45		9			6			*128 FM samples */

/* This only works for matching AMS frequency doesn't work for matching PMS frequency */
#define HACK_MATCH_KEGA 0

#if HACK_MATCH_KEGA == 0
#define LJ_YM2612_LFO_FREQ_SAMPLE_SHIFT (0)
#else
#define LJ_YM2612_LFO_FREQ_SAMPLE_SHIFT (1)
#endif

static int LJ_YM2612_LFO_freqSampleSteps[8] = {
	109 << LJ_YM2612_LFO_FREQ_SAMPLE_SHIFT,
	78 << LJ_YM2612_LFO_FREQ_SAMPLE_SHIFT,
	72 << LJ_YM2612_LFO_FREQ_SAMPLE_SHIFT,
	68 << LJ_YM2612_LFO_FREQ_SAMPLE_SHIFT,
	63 << LJ_YM2612_LFO_FREQ_SAMPLE_SHIFT,
	45 << LJ_YM2612_LFO_FREQ_SAMPLE_SHIFT,
	9 << LJ_YM2612_LFO_FREQ_SAMPLE_SHIFT,
	6 << LJ_YM2612_LFO_FREQ_SAMPLE_SHIFT
};

/* Make the LFO work in 128 steps per full wave */
#define LJ_YM2612_LFO_PHI_NUM_BITS (7)
#define LJ_YM2612_LFO_PHI_MASK ((1 << LJ_YM2612_LFO_PHI_NUM_BITS) - 1)

/* LFO AM table scale - must match volume scale */
#define LJ_YM2612_LFO_AM_SCALE_BITS (LJ_YM2612_VOLUME_SCALE_BITS)

/* LFO channel ams (0, 1.4, 5.9, 11.8 dB) -> 0, 2, 8, 16 in TL units = 2^(-x/8) */
/* convert to EG attenuation units = 2^(-x/64) so x *= 8 or x <<= 3 */
static int LJ_YM2612_LFO_AMStable[4] = {
	0 << LJ_YM2612_TL_EG_ATTENUATION_TABLE_SHIFT,
	2 << LJ_YM2612_TL_EG_ATTENUATION_TABLE_SHIFT,
	8 << LJ_YM2612_TL_EG_ATTENUATION_TABLE_SHIFT,
	16 << LJ_YM2612_TL_EG_ATTENUATION_TABLE_SHIFT
};

/* LFO channel pms */
/*	PMS 0		1			2			3			4			5			6			7					*/
/*			0		3.4		6.7		10		14		20		40		80	cents	*/
/* 100 cents per semitone, 12 semi-tones in an octave, octave = x2 in frequency, thus 1200 cents = +1 * freq */
/*	PMS	0		1			2			3			4			5			6			7							*/
/*			0		1			2			3			4			6			12		24	*(1/360)	*/

/* LFO PM scale - choose 14-bits because going to be multiplied with 11-bit fnum number and only needs to measure 1/360 minimum */
#define LJ_YM2612_PMS_SCALE_BITS (14)
#define LJ_YM2612_PMS_SCALE_FACTOR (360)

static int LJ_YM2612_LFO_PMStable[8] = {
	(0 << LJ_YM2612_PMS_SCALE_BITS ) / LJ_YM2612_PMS_SCALE_FACTOR,
	(1 << LJ_YM2612_PMS_SCALE_BITS ) / LJ_YM2612_PMS_SCALE_FACTOR,
	(2 << LJ_YM2612_PMS_SCALE_BITS ) / LJ_YM2612_PMS_SCALE_FACTOR,
	(3 << LJ_YM2612_PMS_SCALE_BITS ) / LJ_YM2612_PMS_SCALE_FACTOR,
	(4 << LJ_YM2612_PMS_SCALE_BITS ) / LJ_YM2612_PMS_SCALE_FACTOR,
	(6 << LJ_YM2612_PMS_SCALE_BITS ) / LJ_YM2612_PMS_SCALE_FACTOR,
	(12 << LJ_YM2612_PMS_SCALE_BITS ) / LJ_YM2612_PMS_SCALE_FACTOR,
	(24 << LJ_YM2612_PMS_SCALE_BITS ) / LJ_YM2612_PMS_SCALE_FACTOR
};

typedef enum LJ_YM2612_ADSR {
	LJ_YM2612_ADSR_OFF,
	LJ_YM2612_ADSR_ATTACK,
	LJ_YM2612_ADSR_DECAY,
	LJ_YM2612_ADSR_SUSTAIN,
	LJ_YM2612_ADSR_RELEASE
} LJ_YM2612_ADSR;

struct LJ_YM2612_SLOT
{
	int chanId;
	int id;

	int omega;
	int keycode;
	int omegaDelta;
	int totalLevel;
	unsigned int sustainLevelDB;

	int volume;
	unsigned int attenuationDB;

	/* Algorithm support */
	int fmInputDelta;
	int* modulationOutput[3];
	int carrierOutputMask;
	
	/* ADSR settings */
	LJ_YM2612_ADSR adsrState;
	LJ_YM_UINT8 keyScale;
	LJ_YM_UINT8 keyRateScale;
	LJ_YM_UINT8 attackRate;
	LJ_YM_UINT8 decayRate;
	LJ_YM_UINT8 sustainRate;
	LJ_YM_UINT8 releaseRate;

	LJ_YM_UINT8 egAttackRate;
	LJ_YM_UINT8 egDecayRate;
	LJ_YM_UINT8 egSustainRate;
	LJ_YM_UINT8 egReleaseRate;

	LJ_YM_UINT8 egAttackRateUpdateShift;
	LJ_YM_UINT8 egDecayRateUpdateShift;
	LJ_YM_UINT8 egSustainRateUpdateShift;
	LJ_YM_UINT8 egReleaseRateUpdateShift;

	LJ_YM_UINT32 egAttackRateUpdateMask;
	LJ_YM_UINT32 egDecayRateUpdateMask;
	LJ_YM_UINT32 egSustainRateUpdateMask;
	LJ_YM_UINT32 egReleaseRateUpdateMask;

	/* SSG settings */
	LJ_YM_UINT8 egSSG;
	LJ_YM_UINT8 egSSGEnabled;
	LJ_YM_UINT8 egSSGAttack;
	LJ_YM_UINT8 egSSGInvert;
	LJ_YM_UINT8 egSSGHold;
	LJ_YM_UINT8 egSSGCycleEnded;
	LJ_YM_UINT8 egSSGInvertOutput;

	/* LFO settings */
	LJ_YM_UINT8 amEnable;

	/* frequency settings */
	LJ_YM_UINT8 detune;
	LJ_YM_UINT8 multiple;

	LJ_YM_UINT8 normalKeyOn;

	LJ_YM_UINT8 omegaDirty;
};

struct LJ_YM2612_TIMER
{
	int count;
	int addPerOutputSample;
};

struct LJ_YM2612_EG
{
	LJ_YM2612_TIMER timer;
	int timerCountPerUpdate;
	LJ_YM_UINT32 counter;
};

struct LJ_YM2612_LFO
{
	LJ_YM2612_TIMER timer;

	int timerCountPerUpdate;
	LJ_YM_UINT32 counter;

	int value;

	LJ_YM_UINT8 enable;
	LJ_YM_UINT8 freq;
	LJ_YM_UINT8 changed;
};

struct LJ_YM2612_CHANNEL
{
	LJ_YM2612_SLOT slot[LJ_YM2612_NUM_SLOTS_PER_CHANNEL];

	LJ_YM_UINT32 debugFlags;
	int id;

	int slot0Output[2];

	int feedback;

	int left;
	int right;

	int fnum;
	int block;

	int amsMaxdB;
	int pmsMaxFnumScale;
	int pmsFnumScale;

	LJ_YM_UINT8 omegaDirty;
	LJ_YM_UINT8 block_fnumMSB;
	LJ_YM_UINT8 keycode;

	LJ_YM_UINT8 algorithm;

	LJ_YM_UINT8 ams;
	LJ_YM_UINT8 pms;
};

struct LJ_YM2612_CHANNEL2_SLOT_DATA
{
	int fnum;
	int block;

	LJ_YM_UINT8 block_fnumMSB;
	LJ_YM_UINT8 keycode;
};

struct LJ_YM2612_PART
{
	LJ_YM_UINT8 reg[LJ_YM2612_NUM_REGISTERS];
	LJ_YM2612_CHANNEL channel[LJ_YM2612_NUM_CHANNELS_PER_PART];

	int id;
};

struct LJ_YM2612
{
	LJ_YM2612_PART part[LJ_YM2612_NUM_PARTS];
	LJ_YM2612_CHANNEL2_SLOT_DATA channel2slotData[LJ_YM2612_NUM_SLOTS_PER_CHANNEL-1];
	LJ_YM2612_CHANNEL* channels[LJ_YM2612_NUM_CHANNELS_TOTAL];
	LJ_YM2612_EG eg;
	LJ_YM2612_LFO lfo;
	LJ_YM2612_TIMER aTimer;
	LJ_YM2612_TIMER bTimer;

	float baseFreqScale;

	LJ_YM_UINT32 clockRate;
	LJ_YM_UINT32 outputSampleRate;
	LJ_YM_UINT32 sampleCount;

	LJ_YM_UINT32 debugFlags;

	int dacValue;
	int dacEnable;

	int timerAstart;
	int timerBstart;

	LJ_YM_UINT16 timerAvalue;
	LJ_YM_UINT16 timerBvalue;

	LJ_YM_UINT8 statusOutput;
	LJ_YM_UINT8 D07;
	LJ_YM_UINT8 regAddress;
	LJ_YM_UINT8 slotWriteAddr;	

	LJ_YM_UINT8 csmKeyOn;
	LJ_YM_UINT8 ch2Mode;

	LJ_YM_UINT8 timerMode;

	LJ_YM_UINT8 statusA;
	LJ_YM_UINT8 statusB;
};

/* The rate = ADSRrate * 2 + keyRateScale but rate = 0 if ADSRrate = 0 and clamped between 0->63 */
static LJ_YM_UINT8 ym2612_computeEGRate(const int adsrRate, const int keyRateScale)
{
	int egRate;
	egRate = adsrRate * 2 + keyRateScale;
	if ((egRate < 0) || (adsrRate == 0))
	{
		egRate = 0;
	}
	if (egRate > 63)
	{
		egRate = 63;
	}

	return (LJ_YM_UINT8)egRate;
}

/* From the current rate you get a shift width (1,2,4,8,...) and you only update the envelop every N times of the global counter */
static LJ_YM_UINT8 ym2612_computeEGUpdateShift(LJ_YM_UINT8 egRate)
{
	int egRateShift;

	/* EG rate shift counter is 11 - (egRate / 4 ) but clamped to 0 can't be negative */
	egRateShift = 11 - ((int)egRate >> 2);

	if (egRateShift < 0)
	{
		egRateShift = 0;
	}
	return (LJ_YM_UINT8)egRateShift;
}

static LJ_YM_UINT32 ym2612_computeEGUpdateMask(LJ_YM_UINT32 egRateShift)
{
	LJ_YM_UINT32 egRateUpdateMask;
	/* if (egCounter % (1 << egRateShift) == 0) then do an update */
	/* logically the same as if (egCounter & ((1 << egRateShift)-1) == 0x0) */
	egRateUpdateMask = ((1U << egRateShift) -1U);

	return egRateUpdateMask;
}

LJ_YM_UINT32 ym2612_computeEGAttenuationDelta(LJ_YM_UINT32 egRate, LJ_YM_UINT32 egCounter, LJ_YM_UINT32 egRateUpdateShift)
{
	LJ_YM_UINT32 attenuationDelta = 0;
	/* cycle is 0-7 within how many times this phase has been updated */
	/* cycle counts in the comments start from 0 e.g. 0th, 1st, etc */
	const LJ_YM_UINT32 cycle = (egCounter >> egRateUpdateShift) & 0x7;

	if (egRate < 48)
	{
		/* Basic approximation alternate on/off */
		attenuationDelta = (cycle & 0x1);

		/* Set to 1 for the 4th cycle if rate would be 0 on the 0th cycle if bottom bit of rate = 0x1 */
		attenuationDelta |= (egRate & 0x1) & (((cycle & 0x4) >> 2) & ~(cycle & 0x1) & ~((cycle & 0x2)>>1));

		/* Set to 1 for the 2nd and 6th cycle for rate bottom bits = 0x10 and 0x11 */
		attenuationDelta |= ((cycle & 0x2) >> 1) & ((egRate & 0x2) >> 1);

		if (egRate < 8)
		{
			if (egRate < 2)
			{
				/* Special case for rates 0-1 */
				attenuationDelta = 0;
			}
			else if (egRate < 6)
			{
				/* Special case for rates 2-5 */
				attenuationDelta = (cycle & 0x1);
			}
			else
			{
				/* Special case for rates 6-7 */
				attenuationDelta = (cycle & 0x1);
				attenuationDelta |= ((cycle & 0x2) >> 1) & ((egRate & 0x2) >> 1);
			}
		}
	}
	else
	{
		/* Basic level for attenuation delta */
		const LJ_YM_UINT32 rateMajor = (egRate >> 2U);
		LJ_YM_UINT32 shiftAmount = 0;
		attenuationDelta = (1U << (rateMajor - 12U));

		/* Doubled on the 1st & 3rd cycle when rate bottom bit = 0x10 or 0x11 */
		shiftAmount |= ((cycle & 0x1) & ((egRate & 0x2) >> 1));

		/* Doubled on the 3rd cycle when rate bottom bit = 0x1 */
		shiftAmount |= ((cycle & 0x1) & ((cycle & 0x2) >> 1) & (egRate & 0x1));

		/* Doubled on the 2nd cycle when rate bottom bits = 0x11 */
		shiftAmount |= (((cycle & 0x2) >> 1) & ((egRate & 0x2) >> 1) & (egRate & 0x1));

		shiftAmount = shiftAmount & ((attenuationDelta) | ((attenuationDelta & 0x2) >> 1) | ((attenuationDelta & 0x4) >> 2));

		attenuationDelta = attenuationDelta << shiftAmount;
	}

	return attenuationDelta;
}

static int LJ_YM2612_CLAMP_VOLUME(const int volume)
{
	if (volume > LJ_YM2612_VOLUME_MAX)
	{
		return LJ_YM2612_VOLUME_MAX;
	}
	else if (volume < -LJ_YM2612_VOLUME_MAX)
	{
		return -LJ_YM2612_VOLUME_MAX;
	}
	return volume;
}

static int ym2612_computeDetuneDelta(const int detune, const int keycode, const LJ_YM_UINT32 debugFlags)
{
	const int detuneDelta = LJ_YM2612_detuneTable[detune*LJ_YM2612_NUM_KEYCODES+keycode];

	if (debugFlags & LJ_YM2612_DEBUG)
	{
	}
	return detuneDelta;
}

static int ym2612_computeOmegaDelta(const int fnum, const int block, const int multiple, const int detune, const int keycode,
																		const int fnumScale,
																		const LJ_YM_UINT32 debugFlags)
{
	/* fnum = fnum * ( 1 + fnumScale ) : fnumScale is in units of LJ_YM2612_PMS_SCALE_BITS */
	/* TODO: fnumScale could be precompute for all fnum values and just look up the addition from a table */
	const int fnumDelta = (( fnum * fnumScale ) >> LJ_YM2612_PMS_SCALE_BITS);

	/* F * 2^(B-1) */
	/* Could multiply the fnumTable up by (1<<6) then change this to fnumTable >> (7-B) */
	int omegaDelta = (LJ_YM2612_fnumTable[fnum+fnumDelta] << block) >> 1;

	const int detuneDelta = ym2612_computeDetuneDelta(detune, keycode, debugFlags);
	omegaDelta += detuneDelta;

	if (omegaDelta < 0)
	{
		/* Wrap around */
		omegaDelta += LJ_YM2612_fnumTable[LJ_YM2612_FNUM_TABLE_NUM_ENTRIES];
	}

	/* /2 because multiple is stored as x2 of its value */
	omegaDelta = (omegaDelta * multiple) >> 1;

	return omegaDelta;
}

static void ym2612_slotComputeEGParams(LJ_YM2612_SLOT* const slotPtr, const int slotKeycode, const LJ_YM_UINT32 debugFlags)
{
	slotPtr->keycode = slotKeycode;
	slotPtr->keyRateScale = (LJ_YM_UINT8)(slotKeycode >> slotPtr->keyScale);

	slotPtr->egAttackRate = ym2612_computeEGRate(slotPtr->attackRate, slotPtr->keyRateScale);
	slotPtr->egAttackRateUpdateShift = ym2612_computeEGUpdateShift(slotPtr->egAttackRate);
	slotPtr->egAttackRateUpdateMask = ym2612_computeEGUpdateMask(slotPtr->egAttackRateUpdateShift);

	slotPtr->egDecayRate = ym2612_computeEGRate(slotPtr->decayRate, slotPtr->keyRateScale);
	slotPtr->egDecayRateUpdateShift = ym2612_computeEGUpdateShift(slotPtr->egDecayRate);
	slotPtr->egDecayRateUpdateMask = ym2612_computeEGUpdateMask(slotPtr->egDecayRateUpdateShift);

	slotPtr->egSustainRate = ym2612_computeEGRate(slotPtr->sustainRate, slotPtr->keyRateScale);
	slotPtr->egSustainRateUpdateShift = ym2612_computeEGUpdateShift(slotPtr->egSustainRate);
	slotPtr->egSustainRateUpdateMask = ym2612_computeEGUpdateMask(slotPtr->egSustainRateUpdateShift);

	slotPtr->egReleaseRate = ym2612_computeEGRate(slotPtr->releaseRate, slotPtr->keyRateScale);
	slotPtr->egReleaseRateUpdateShift = ym2612_computeEGUpdateShift(slotPtr->egReleaseRate);
	slotPtr->egReleaseRateUpdateMask = ym2612_computeEGUpdateMask(slotPtr->egReleaseRateUpdateShift);

	if (debugFlags & LJ_YM2612_DEBUG)
	{
	}
}

static void ym2612_slotComputeOmegaDelta(LJ_YM2612_SLOT* const slotPtr,
																				 const int fnum, const int block, const int keycode, const int fnumScale,
																				 const LJ_YM_UINT32 debugFlags)
{
	const int multiple = slotPtr->multiple;
	const int detune = slotPtr->detune;

	const int omegaDelta = ym2612_computeOmegaDelta(fnum, block, multiple, detune, keycode, fnumScale, debugFlags);

	slotPtr->omegaDelta = omegaDelta;

	if (debugFlags & LJ_YM2612_DEBUG)
	{
		printf("Slot:%d omegaDelta %d fnum:%d block:%d multiple:%3.1f detune:%d keycode:%d\n",
					 slotPtr->id, omegaDelta, fnum, block, (float)multiple/2.0f, detune, keycode);
	}
}

static void ym2612_slotComputeWaveParams(LJ_YM2612_SLOT* const slotPtr, LJ_YM2612_CHANNEL* const channelPtr, LJ_YM2612* const ym2612Ptr,
																				const LJ_YM_UINT32 debugFlags)
{
	int slotFnum;
	int slotBlock;
	int slotKeycode;

	const int slot = slotPtr->id;
	const int pmsFnumScale = channelPtr->pmsFnumScale;

	/* For normal mode or channels that aren't channel 2 or for slot 3 */
	if ((ym2612Ptr->ch2Mode == 0) || (channelPtr->id != 2) || (slot == 3))
	{
		const int channelFnum = channelPtr->fnum;
		const int channelBlock = channelPtr->block;
		const int channelKeycode = channelPtr->keycode;

		/* Take the slot frequency from the channel frequency */
		slotFnum = channelFnum;
		slotBlock = channelBlock;
		slotKeycode = channelKeycode;
	}
	else
	{
		/* Channel 2 and it is in special mode with fnum, block (keycode) per slot for slots 0, 1, 2 */
		slotFnum = ym2612Ptr->channel2slotData[slot].fnum;
		slotBlock = ym2612Ptr->channel2slotData[slot].block;
		slotKeycode = ym2612Ptr->channel2slotData[slot].keycode;
	}

	ym2612_slotComputeOmegaDelta(slotPtr, slotFnum, slotBlock, slotKeycode, pmsFnumScale, debugFlags);
	ym2612_slotComputeEGParams(slotPtr, slotKeycode, debugFlags);
	slotPtr->omegaDirty = 0;
}

static void ym2612_slotStartPlaying(LJ_YM2612_SLOT* const slotPtr, LJ_YM2612_CHANNEL* const channelPtr, LJ_YM2612* const ym2612Ptr,
																		const LJ_YM_UINT32 debugFlags)
{
	LJ_YM_UINT8 egRate;

	if (debugFlags & LJ_YM2612_DEBUG)
	{
		printf("Slot[%d] Start\n",slotPtr->id);
	}
	/* Handle start playing but haven't computed the parameters yet */
	if ((slotPtr->omegaDirty == 1) || (channelPtr->omegaDirty == 1))
	{
		ym2612_slotComputeWaveParams(slotPtr, channelPtr, ym2612Ptr, debugFlags);
	}
	egRate = slotPtr->egAttackRate;

	slotPtr->omega = 0;
	slotPtr->fmInputDelta = 0;
	slotPtr->egSSGCycleEnded = 0;
	slotPtr->adsrState = LJ_YM2612_ADSR_ATTACK;
	/* Test for the infinite attack rates (30,31)- e.g. for CSM mode to make noise */
	if (egRate > 59)
	{
		const LJ_YM_UINT32 attenuationDB = LJ_YM2612_EG_ATTENUATION_DB_MIN;
		if (debugFlags & LJ_YM2612_DEBUG)
		{
			printf("Slot[%d] Inf attack rate start\n", slotPtr->id);
		}
		slotPtr->attenuationDB = attenuationDB;
	}
	/* handle special cases of going straight into decay/sustain */
	if (slotPtr->volume == LJ_YM2612_VOLUME_MAX)
	{
		if (slotPtr->sustainLevelDB == LJ_YM2612_EG_ATTENUATION_DB_MIN)
		{
			slotPtr->adsrState = LJ_YM2612_ADSR_SUSTAIN;
		}
		else
		{
			slotPtr->adsrState = LJ_YM2612_ADSR_DECAY;
		}
	}
}

static void ym2612_slotKeyON(LJ_YM2612_SLOT* const slotPtr, LJ_YM2612_CHANNEL* const channelPtr, LJ_YM2612* const ym2612Ptr,
														 const LJ_YM_UINT32 debugFlags)
{
	const LJ_YM_UINT8 csmKeyOn = ym2612Ptr->csmKeyOn;

	/* Only key on if keyed off normally and CSM mode key off */
	if ((slotPtr->normalKeyOn == 0) && (csmKeyOn == 0))
	{
		ym2612_slotStartPlaying(slotPtr, channelPtr, ym2612Ptr, debugFlags);
		slotPtr->egSSGInvertOutput = 0;
	}

	if (debugFlags & LJ_YM2612_DEBUG)
	{
	}
}

static void ym2612_slotKeyOFF(LJ_YM2612_SLOT* const slotPtr, const LJ_YM_UINT32 debugFlags)
{
	/* Handle the channel being OFF (release mode leads to volume being 0) OR ADSR state OFF */
	slotPtr->adsrState = LJ_YM2612_ADSR_RELEASE;

	if (slotPtr->attenuationDB >= LJ_YM2612_EG_ATTENUATION_DB_MAX)
	{
		slotPtr->adsrState = LJ_YM2612_ADSR_OFF;
	}
	if ((slotPtr->egSSGEnabled == 1) && (slotPtr->egSSGInvertOutput ^ slotPtr->egSSGAttack))
	{
		/* When key off convert the inverted attenuation into real attenuation */
		LJ_YM_UINT32 invertAttenuationDB = LJ_YM2612_EG_SSG_ATTENUATION_DB_MAX - slotPtr->attenuationDB;
		invertAttenuationDB &= LJ_YM2612_EG_ATTENUATION_DB_MAX;
		if (debugFlags & LJ_YM2612_DEBUG)
		{
			printf("Key OFF Invert:%d -> %d\n", slotPtr->attenuationDB, invertAttenuationDB);
		}
		slotPtr->attenuationDB = invertAttenuationDB;
	}

	if (debugFlags & LJ_YM2612_DEBUG)
	{
	}
}

static void ym2612_slotCSMKeyOFF(LJ_YM2612_SLOT* const slotPtr, const LJ_YM_UINT32 debugFlags)
{
	if (slotPtr->normalKeyOn == 0)
	{
		ym2612_slotKeyOFF(slotPtr, debugFlags);
	}
	if (debugFlags)
	{
	}
}

#define ADSR_DEBUG (0)

static void ym2612_slotUpdateSSG(LJ_YM2612_SLOT* slotPtr, LJ_YM2612_CHANNEL* channelPtr, LJ_YM2612* ym2612Ptr,
																const LJ_YM_UINT32 debugFlags)
{
/* SSG-EG envelopes :
		Enabled Attack Invert Hold
			 D3		 D2			 D1		 D0
     		1 		0 			0 		0  \|\|\|\

				1 		0 			0 		1  \___

 				1 		0 			1 		0  \/\/
																___
				1 		0 			1 		1  \|

 				1 		1 			0 		0  /|/|/|/
																___
				1 		1 			0 		1  /

 				1 		1 			1 		0  /\/\

				1 		1 			1 		1  /|___

		The SSG EG waveforms in the manual and above refer to the decay & sustain phase of the ADSR
			e.g. the SSG EG cycle ends when the ADSR volume reaches 0x0 and starts on ATTACK

		Enabled = has to be on to do anything
		Attack = inverts the volume during decay & sustain phase at start of SSG EG cycle i.e.key on or repeat
		Invert = inverts the volume during decay & sustain phase at end of SSG EG cycle e.g. when ADSR volume reaches 0x0
		Hold = holds the volume at end of SSG EG cycle e.g. when ADSR volume reaches 0x0, note with invert or attack
						the output volume could be MAX not 0x0 - look at envelopes above

		Lastly when SSG EG is enabled the attenuation deltas are multiplied by 4 for ALL but attack ADSR phases (which is very odd)
		TODO: a test on real genesis to see if attack attenuation delta affected by SSG
		TODO: a test on real genesis to see how release state works with SSG
*/
	if ((slotPtr->egSSGEnabled) && (slotPtr->adsrState != LJ_YM2612_ADSR_OFF))
	{
		const LJ_YM_UINT32 attenuationDB = slotPtr->attenuationDB;
		if (attenuationDB >= LJ_YM2612_EG_SSG_ATTENUATION_DB_MAX)
		{
			if ((slotPtr->egSSGInvert == 1) && ((slotPtr->egSSGHold == 0) || (slotPtr->egSSGInvertOutput == 0)))
			{
				slotPtr->egSSGInvertOutput ^= 1;
				if (debugFlags & LJ_YM2612_DEBUG)
				{
					printf("InvertOutput:%d\n", slotPtr->egSSGInvertOutput);
				}
			}
			if ((slotPtr->egSSGInvert == 0) && (slotPtr->egSSGHold == 0))
			{
				/* From the forums the phase gets locked to 0 when SSG triggered in its special place */
				slotPtr->omega = 0;
			}
			if (slotPtr->adsrState != LJ_YM2612_ADSR_ATTACK)
			{
				if ((slotPtr->adsrState != LJ_YM2612_ADSR_RELEASE) && (slotPtr->adsrState != LJ_YM2612_ADSR_OFF) &&
						(slotPtr->egSSGHold == 0))
				{
					/* Restart the wave */
					if (debugFlags & LJ_YM2612_DEBUG)
					{
						printf("Restart wave\n");
					}
					ym2612_slotStartPlaying(slotPtr, channelPtr, ym2612Ptr, debugFlags);
				}
				else if ((slotPtr->adsrState == LJ_YM2612_ADSR_RELEASE) ||
								((slotPtr->egSSGInvertOutput ^ slotPtr->egSSGAttack) == 0x0))
				{
					/* Not looping but got to SSG special place - force to max attenuation */
					if (debugFlags & LJ_YM2612_DEBUG)
					{
						printf("RELEASE: force to max InvertOutput: %d\n", slotPtr->egSSGInvertOutput);
					}
					slotPtr->attenuationDB = LJ_YM2612_EG_ATTENUATION_DB_MAX;
				}
				else if ((slotPtr->adsrState != LJ_YM2612_ADSR_RELEASE) &&
						((slotPtr->egSSGHold == 1) && ((slotPtr->egSSGInvertOutput ^ slotPtr->egSSGAttack))))
				{
					/* TODO: don't like but otherwise hold+invert doesn't go to max volume */
					/* Invert output calculation doesn't work nicely if volume > SSG max attenuation */
					/* Special case for hold : not looping but got to SSG special place - force to max attenuation */
					/*slotPtr->attenuationDB = LJ_YM2612_EG_SSG_ATTENUATION_DB_MAX;*/
				}
			}
		}
	}
}

static LJ_YM_UINT32 ym2612_slotComputeEGADSRAttenuation(LJ_YM2612_SLOT* const slotPtr, const LJ_YM_UINT32 egCounter)
{
	const LJ_YM2612_ADSR adsrState = slotPtr->adsrState;

	LJ_YM_UINT32 attenuationDB = slotPtr->attenuationDB;

	if (attenuationDB > LJ_YM2612_EG_ATTENUATION_DB_MAX)
	{
#if (ADSR_DEBUG)
		printf("attenuationDB > LJ_YM2612_EG_ATTENUATION_DB_MAX (%d) %d\n", LJ_YM2612_EG_ATTENUATION_DB_MAX, attenuationDB);
#endif
		attenuationDB = LJ_YM2612_EG_ATTENUATION_DB_MAX;
	}
	attenuationDB &= LJ_YM2612_EG_ATTENUATION_DB_MAX;

	if (adsrState == LJ_YM2612_ADSR_ATTACK)
	{
		const LJ_YM_UINT8 egRate = slotPtr->egAttackRate;
		const LJ_YM_UINT8 egRateUpdateShift = slotPtr->egAttackRateUpdateShift;
		const LJ_YM_UINT32 egRateUpdateMask = slotPtr->egAttackRateUpdateMask;
		if ((egCounter & egRateUpdateMask) == 0)
		{
			const LJ_YM_UINT32 oldDB = attenuationDB;
			LJ_YM_UINT32 egAttenuationDelta = ym2612_computeEGAttenuationDelta(egRate, egCounter, egRateUpdateShift);
			LJ_YM_UINT32 deltaDB;

			deltaDB = (((oldDB * egAttenuationDelta)+15) >> 4);
			/* inverted exponential curve: attenuation += increment * ((1024-attenuation) / 16 + 1) : NEMESIS */
			/* inverted exponential curve: attenuation -= (((increment * attenuation)+15) / 16) : JAKE */
			attenuationDB = oldDB - deltaDB;

#if (ADSR_DEBUG)
			printf("ATTACK[%d]:attDBdelta:%d attDB:%d AR:%d kcode:%d krs:%d keyscale:%d egRate:%d egCounter:%d egRateUpdateShift:%d\n",
					slotPtr->id, egAttenuationDelta, attenuationDB, slotPtr->attackRate, keycode, keyRateScale, slotPtr->keyScale,
					egRate, egCounter, egRateUpdateShift);
#endif

			if ((attenuationDB == LJ_YM2612_EG_ATTENUATION_DB_MIN) || (attenuationDB > LJ_YM2612_EG_ATTENUATION_DB_MAX))
			{
				attenuationDB = LJ_YM2612_EG_ATTENUATION_DB_MIN;
			}
		}
	}
	else if (adsrState == LJ_YM2612_ADSR_DECAY)
	{
		const LJ_YM_UINT8 egRate = slotPtr->egDecayRate;
		const LJ_YM_UINT8 egRateUpdateShift = slotPtr->egDecayRateUpdateShift;
		const LJ_YM_UINT32 egRateUpdateMask = slotPtr->egDecayRateUpdateMask;
		if ((egCounter & egRateUpdateMask) == 0)
		{
			LJ_YM_UINT32 egAttenuationDelta = ym2612_computeEGAttenuationDelta(egRate, egCounter, egRateUpdateShift);
			if (slotPtr->egSSGEnabled == 1)
			{
				if (attenuationDB >= LJ_YM2612_EG_SSG_ATTENUATION_DB_MAX)
				{
					egAttenuationDelta = 0;
				}
				else
				{
					/* attenuation delta is *4 when ssg is enabled */
					egAttenuationDelta = egAttenuationDelta << 2;
				}
			}
			attenuationDB += egAttenuationDelta;
#if (ADSR_DEBUG)
			printf("DECAY[%d]:attDBdelta:%d attDB:%d DR:%d kcode:%d krs:%d keyscale:%d egRate:%d egCounter:%d egRateUpdateShift:%d\n",
					slotPtr->id, egAttenuationDelta, attenuationDB, slotPtr->decayRate, keycode, keyRateScale, slotPtr->keyScale,
					egRate, egCounter, egRateUpdateShift);
#endif
			if (attenuationDB >= slotPtr->sustainLevelDB)
			{
				attenuationDB = slotPtr->sustainLevelDB;
			}
		}
	}
	else if (adsrState == LJ_YM2612_ADSR_SUSTAIN)
	{
		const LJ_YM_UINT8 egRate = slotPtr->egSustainRate;
		const LJ_YM_UINT8 egRateUpdateShift = slotPtr->egSustainRateUpdateShift;
		const LJ_YM_UINT32 egRateUpdateMask = slotPtr->egSustainRateUpdateMask;
		if ((egCounter & egRateUpdateMask) == 0)
		{
			LJ_YM_UINT32 egAttenuationDelta = ym2612_computeEGAttenuationDelta(egRate, egCounter, egRateUpdateShift);
			if (slotPtr->egSSGEnabled == 1)
			{
				if (attenuationDB >= LJ_YM2612_EG_SSG_ATTENUATION_DB_MAX)
				{
					egAttenuationDelta = 0;
				}
				else
				{
					/* attenuation delta is *4 when ssg is enabled */
					egAttenuationDelta = egAttenuationDelta << 2;
				}
			}
			attenuationDB += egAttenuationDelta;
#if (ADSR_DEBUG)
			printf("SUSTAIN[%d]:attDBdelta:%d attDB:%d SR:%d kcode:%d krs:%d keyscale:%d egRate:%d egCounter:%d egRateUpdateShift:%d\n",
					slotPtr->id, egAttenuationDelta, attenuationDB, slotPtr->sustainRate, keycode, keyRateScale, slotPtr->keyScale,
					egRate, egCounter, egRateUpdateShift);
#endif
		}
	}
	else if (adsrState == LJ_YM2612_ADSR_RELEASE)
	{
		const LJ_YM_UINT8 egRate = slotPtr->egReleaseRate;
		const LJ_YM_UINT8 egRateUpdateShift = slotPtr->egReleaseRateUpdateShift;
		const LJ_YM_UINT32 egRateUpdateMask = slotPtr->egReleaseRateUpdateMask;
		if ((egCounter & egRateUpdateMask) == 0)
		{
			LJ_YM_UINT32 egAttenuationDelta = ym2612_computeEGAttenuationDelta(egRate, egCounter, egRateUpdateShift);
			if (slotPtr->egSSGEnabled == 1)
			{
				if (attenuationDB >= LJ_YM2612_EG_SSG_ATTENUATION_DB_MAX)
				{
					egAttenuationDelta = 0;
				}
				else
				{
					/* attenuation delta is *4 when ssg is enabled */
					egAttenuationDelta = egAttenuationDelta << 2;
				}
			}
			attenuationDB += egAttenuationDelta;
#if (ADSR_DEBUG)
			printf("RELEASE[%d]:attDBdelta:%d attDB:%d RR:%d kcode:%d krs:%d keyscale:%d egRate:%d egCounter:%d egRateUpdateShift:%d\n",
					slotPtr->id, egAttenuationDelta, attenuationDB, slotPtr->releaseRate, keycode, keyRateScale, slotPtr->keyScale,
					egRate, egCounter, egRateUpdateShift);
#endif
		}
	}
	if (attenuationDB >= LJ_YM2612_EG_ATTENUATION_DB_MAX)
	{
		attenuationDB = LJ_YM2612_EG_ATTENUATION_DB_MAX;
	}

	return attenuationDB;
}

static void ym2612_slotUpdateEGADSRState(LJ_YM2612_SLOT* const slotPtr, const LJ_YM_UINT32 attenuationDB, const LJ_YM_UINT32 egCounter,
																				 const LJ_YM_UINT32 debugFlags)
{
	const LJ_YM2612_ADSR adsrState = slotPtr->adsrState;

	if (adsrState == LJ_YM2612_ADSR_ATTACK)
	{
		const LJ_YM_UINT32 egRateUpdateMask = slotPtr->egAttackRateUpdateMask;
		if ((egCounter & egRateUpdateMask) == 0)
		{
			if ((attenuationDB == LJ_YM2612_EG_ATTENUATION_DB_MIN) || (attenuationDB > LJ_YM2612_EG_ATTENUATION_DB_MAX))
			{
				if (slotPtr->sustainLevelDB == LJ_YM2612_EG_ATTENUATION_DB_MIN)
				{
					slotPtr->adsrState = LJ_YM2612_ADSR_SUSTAIN;
				}
				else
				{
					slotPtr->adsrState = LJ_YM2612_ADSR_DECAY;
				}
			}
		}
	}
	else if (adsrState == LJ_YM2612_ADSR_DECAY)
	{
		const LJ_YM_UINT32 egRateUpdateMask = slotPtr->egDecayRateUpdateMask;
		if ((egCounter & egRateUpdateMask) == 0)
		{
			if (attenuationDB >= slotPtr->sustainLevelDB)
			{
				slotPtr->adsrState = LJ_YM2612_ADSR_SUSTAIN;
			}
		}
	}
	else if (adsrState == LJ_YM2612_ADSR_SUSTAIN)
	{
		const LJ_YM_UINT32 egRateUpdateMask = slotPtr->egSustainRateUpdateMask;
		if ((egCounter & egRateUpdateMask) == 0)
		{
		}
	}
	else if (adsrState == LJ_YM2612_ADSR_RELEASE)
	{
		const LJ_YM_UINT32 egRateUpdateMask = slotPtr->egReleaseRateUpdateMask;
		if ((egCounter & egRateUpdateMask) == 0)
		{
		}
		if (attenuationDB >= LJ_YM2612_EG_ATTENUATION_DB_MAX)
		{
			if (debugFlags & LJ_YM2612_DEBUG)
			{
				printf("ADSR_OFF\n");
			}
			slotPtr->adsrState = LJ_YM2612_ADSR_OFF;
		}
	}
}

static void ym2612_slotUpdateEG(LJ_YM2612_SLOT* const slotPtr, const LJ_YM_UINT32 egCounter, const LJ_YM_UINT32 debugFlags)
{
	LJ_YM_UINT32 attenuationDB;
	
	attenuationDB = ym2612_slotComputeEGADSRAttenuation(slotPtr, egCounter);
	if (attenuationDB > LJ_YM2612_EG_ATTENUATION_DB_MAX)
	{
		attenuationDB = LJ_YM2612_EG_ATTENUATION_DB_MAX;
	}
	ym2612_slotUpdateEGADSRState(slotPtr, attenuationDB, egCounter, debugFlags);
	slotPtr->attenuationDB = attenuationDB;
}

static void ym2612_channelSetConnections(LJ_YM2612_CHANNEL* const channelPtr)
{
	const int algorithm = channelPtr->algorithm;
	const int FULL_OUTPUT_MASK = ~0;
	if (algorithm == 0x0)
	{
		/*
			Each slot outputs to the next FM, only slot 3 has carrier output
			[0] --> [1] --> [2] --> [3] --------------->
		*/
		channelPtr->slot[0].carrierOutputMask = 0;
		channelPtr->slot[0].modulationOutput[0] = &channelPtr->slot[1].fmInputDelta;
		channelPtr->slot[0].modulationOutput[1] = NULL;
		channelPtr->slot[0].modulationOutput[2] = NULL;

		channelPtr->slot[1].carrierOutputMask = 0;
		channelPtr->slot[1].modulationOutput[0] = &channelPtr->slot[2].fmInputDelta;
		channelPtr->slot[1].modulationOutput[1] = NULL;
		channelPtr->slot[1].modulationOutput[2] = NULL;

		channelPtr->slot[2].carrierOutputMask = 0;
		channelPtr->slot[2].modulationOutput[0] = &channelPtr->slot[3].fmInputDelta;
		channelPtr->slot[2].modulationOutput[1] = NULL;
		channelPtr->slot[2].modulationOutput[2] = NULL;

		channelPtr->slot[3].carrierOutputMask = FULL_OUTPUT_MASK;
		channelPtr->slot[3].modulationOutput[0] = NULL;
		channelPtr->slot[3].modulationOutput[1] = NULL;
		channelPtr->slot[3].modulationOutput[2] = NULL;
	}
	else if (algorithm == 0x1)
	{
		/*
			slot 0 & slot 1 both output to slot 2, slot 2 outputs to slot 3, only slot 3 has carrier output
				[0] \
				[1] --> [2] --> [3] ------->
		*/
		channelPtr->slot[0].carrierOutputMask = 0;
		channelPtr->slot[0].modulationOutput[0] = &channelPtr->slot[2].fmInputDelta;
		channelPtr->slot[0].modulationOutput[1] = NULL;
		channelPtr->slot[0].modulationOutput[2] = NULL;

		channelPtr->slot[1].carrierOutputMask = 0;
		channelPtr->slot[1].modulationOutput[0] = &channelPtr->slot[2].fmInputDelta;
		channelPtr->slot[1].modulationOutput[1] = NULL;
		channelPtr->slot[1].modulationOutput[2] = NULL;

		channelPtr->slot[2].carrierOutputMask = 0;
		channelPtr->slot[2].modulationOutput[0] = &channelPtr->slot[3].fmInputDelta;
		channelPtr->slot[2].modulationOutput[1] = NULL;
		channelPtr->slot[2].modulationOutput[2] = NULL;

		channelPtr->slot[3].carrierOutputMask = FULL_OUTPUT_MASK;
		channelPtr->slot[3].modulationOutput[0] = NULL;
		channelPtr->slot[3].modulationOutput[1] = NULL;
		channelPtr->slot[3].modulationOutput[2] = NULL;
	}
	else if (algorithm == 0x2)
	{
		/*
			slot 0 & slot 2 both output to slot 3, slot 1 outputs to slot 2, only slot 3 has carrier output
			[0] --------\
			[1] --> [2] --> [3] ------->
		*/
		channelPtr->slot[0].carrierOutputMask = 0;
		channelPtr->slot[0].modulationOutput[0] = &channelPtr->slot[3].fmInputDelta;
		channelPtr->slot[0].modulationOutput[1] = NULL;
		channelPtr->slot[0].modulationOutput[2] = NULL;

		channelPtr->slot[1].carrierOutputMask = 0;
		channelPtr->slot[1].modulationOutput[0] = &channelPtr->slot[2].fmInputDelta;
		channelPtr->slot[1].modulationOutput[1] = NULL;
		channelPtr->slot[1].modulationOutput[2] = NULL;

		channelPtr->slot[2].carrierOutputMask = 0;
		channelPtr->slot[2].modulationOutput[0] = &channelPtr->slot[3].fmInputDelta;
		channelPtr->slot[2].modulationOutput[1] = NULL;
		channelPtr->slot[2].modulationOutput[2] = NULL;

		channelPtr->slot[3].carrierOutputMask = FULL_OUTPUT_MASK;
		channelPtr->slot[3].modulationOutput[0] = NULL;
		channelPtr->slot[3].modulationOutput[1] = NULL;
		channelPtr->slot[3].modulationOutput[2] = NULL;
	}
	else if (algorithm == 0x3)
	{
		/*
			slot 0 outputs to slot 1, slot 1 & slot 2 output to slot 3, only slot 3 has carrier output
			[0] --> [1] --> [3] ------->
			[2] --------/
		*/
		channelPtr->slot[0].carrierOutputMask = 0;
		channelPtr->slot[0].modulationOutput[0] = &channelPtr->slot[1].fmInputDelta;
		channelPtr->slot[0].modulationOutput[1] = NULL;
		channelPtr->slot[0].modulationOutput[2] = NULL;

		channelPtr->slot[1].carrierOutputMask = 0;
		channelPtr->slot[1].modulationOutput[0] = &channelPtr->slot[3].fmInputDelta;
		channelPtr->slot[1].modulationOutput[1] = NULL;
		channelPtr->slot[1].modulationOutput[2] = NULL;

		channelPtr->slot[2].carrierOutputMask = 0;
		channelPtr->slot[2].modulationOutput[0] = &channelPtr->slot[3].fmInputDelta;
		channelPtr->slot[2].modulationOutput[1] = NULL;
		channelPtr->slot[2].modulationOutput[2] = NULL;

		channelPtr->slot[3].carrierOutputMask = FULL_OUTPUT_MASK;
		channelPtr->slot[3].modulationOutput[0] = NULL;
		channelPtr->slot[3].modulationOutput[1] = NULL;
		channelPtr->slot[3].modulationOutput[2] = NULL;
	}
	else if (algorithm == 0x4)
	{
		/*
			slot 0 outputs to slot 1, slot 2 outputs to slot 3, slot 1 and slot 3 have carrier output
		    [0] --> [1] --------------->
        [2] --> [3] --/
		*/
		channelPtr->slot[0].carrierOutputMask = 0;
		channelPtr->slot[0].modulationOutput[0] = &channelPtr->slot[1].fmInputDelta;
		channelPtr->slot[0].modulationOutput[1] = NULL;
		channelPtr->slot[0].modulationOutput[2] = NULL;

		channelPtr->slot[1].carrierOutputMask = FULL_OUTPUT_MASK;
		channelPtr->slot[1].modulationOutput[0] = NULL;
		channelPtr->slot[1].modulationOutput[1] = NULL;
		channelPtr->slot[1].modulationOutput[2] = NULL;

		channelPtr->slot[2].carrierOutputMask = 0;
		channelPtr->slot[2].modulationOutput[0] = &channelPtr->slot[3].fmInputDelta;
		channelPtr->slot[2].modulationOutput[1] = NULL;
		channelPtr->slot[2].modulationOutput[2] = NULL;

		channelPtr->slot[3].carrierOutputMask = FULL_OUTPUT_MASK;
		channelPtr->slot[3].modulationOutput[0] = NULL;
		channelPtr->slot[3].modulationOutput[1] = NULL;
		channelPtr->slot[3].modulationOutput[2] = NULL;
	}
	else if (algorithm == 0x5)
	{
		/*
			slot 0 outputs to slot 1 and slot 2 and slot 3, only slot 3 has carrier output
						/>	[1] --\
			[0] -->		[2] --------------->
						\>	[3] --/
		*/
		channelPtr->slot[0].carrierOutputMask = 0;
		channelPtr->slot[0].modulationOutput[0] = &channelPtr->slot[1].fmInputDelta;
		channelPtr->slot[0].modulationOutput[1] = &channelPtr->slot[2].fmInputDelta;
		channelPtr->slot[0].modulationOutput[2] = &channelPtr->slot[3].fmInputDelta;

		channelPtr->slot[1].carrierOutputMask = FULL_OUTPUT_MASK;
		channelPtr->slot[1].modulationOutput[0] = NULL;
		channelPtr->slot[1].modulationOutput[1] = NULL;
		channelPtr->slot[1].modulationOutput[2] = NULL;

		channelPtr->slot[2].carrierOutputMask = FULL_OUTPUT_MASK;
		channelPtr->slot[2].modulationOutput[0] = NULL;
		channelPtr->slot[2].modulationOutput[1] = NULL;
		channelPtr->slot[2].modulationOutput[2] = NULL;

		channelPtr->slot[3].carrierOutputMask = FULL_OUTPUT_MASK;
		channelPtr->slot[3].modulationOutput[0] = NULL;
		channelPtr->slot[3].modulationOutput[1] = NULL;
		channelPtr->slot[3].modulationOutput[2] = NULL;
	}
	else if (algorithm == 0x6)
	{
		/*
			slot 0 outputs to slot 1, slot 1 and slot 2 and slot 3 have carrier output
			[0] --> [1] --\
 							[2] --------------->
							[3] --/
		*/
		channelPtr->slot[0].carrierOutputMask = 0;
		channelPtr->slot[0].modulationOutput[0] = &channelPtr->slot[1].fmInputDelta;
		channelPtr->slot[0].modulationOutput[1] = NULL;
		channelPtr->slot[0].modulationOutput[2] = NULL;

		channelPtr->slot[1].carrierOutputMask = FULL_OUTPUT_MASK;
		channelPtr->slot[1].modulationOutput[0] = NULL;
		channelPtr->slot[1].modulationOutput[1] = NULL;
		channelPtr->slot[1].modulationOutput[2] = NULL;

		channelPtr->slot[2].carrierOutputMask = FULL_OUTPUT_MASK;
		channelPtr->slot[2].modulationOutput[0] = NULL;
		channelPtr->slot[2].modulationOutput[1] = NULL;
		channelPtr->slot[2].modulationOutput[2] = NULL;

		channelPtr->slot[3].carrierOutputMask = FULL_OUTPUT_MASK;
		channelPtr->slot[3].modulationOutput[0] = NULL;
		channelPtr->slot[3].modulationOutput[1] = NULL;
		channelPtr->slot[3].modulationOutput[2] = NULL;
	}
	else if (algorithm == 0x7)
	{
		/*
		 Each slot outputs to carrier, no FM
			[0] --\
    	[1] ---\
    	[2] --------------->
    	[3] ---/
		*/
		channelPtr->slot[0].carrierOutputMask = FULL_OUTPUT_MASK;
		channelPtr->slot[0].modulationOutput[0] = NULL;
		channelPtr->slot[0].modulationOutput[1] = NULL;
		channelPtr->slot[0].modulationOutput[2] = NULL;

		channelPtr->slot[1].carrierOutputMask = FULL_OUTPUT_MASK;
		channelPtr->slot[1].modulationOutput[0] = NULL;
		channelPtr->slot[1].modulationOutput[1] = NULL;
		channelPtr->slot[1].modulationOutput[2] = NULL;

		channelPtr->slot[2].carrierOutputMask = FULL_OUTPUT_MASK;
		channelPtr->slot[2].modulationOutput[0] = NULL;
		channelPtr->slot[2].modulationOutput[1] = NULL;
		channelPtr->slot[2].modulationOutput[2] = NULL;

		channelPtr->slot[3].carrierOutputMask = FULL_OUTPUT_MASK;
		channelPtr->slot[3].modulationOutput[0] = NULL;
		channelPtr->slot[3].modulationOutput[1] = NULL;
		channelPtr->slot[3].modulationOutput[2] = NULL;
	}
}

static void ym2612_channelSetLeftRightAmsPms(LJ_YM2612_CHANNEL* const channelPtr, const LJ_YM_UINT8 data)
{
	/* 0xB4-0xB6 Feedback = Left= Bit 7, Right= Bit 6, AMS = Bits 4-5, PMS = Bits 0-2 */
	const int left = (data >> 7) & 0x1;
	const int right = (data >> 6) & 0x1;
	const LJ_YM_UINT8 AMS = (data >> 4) & 0x3;
	const LJ_YM_UINT8 PMS = (data >> 0) & 0x7;

	channelPtr->left = left *	~0;
	channelPtr->right = right * ~0;
	channelPtr->ams = AMS;
	channelPtr->amsMaxdB = LJ_YM2612_LFO_AMStable[AMS];
	channelPtr->pms = PMS;
	channelPtr->pmsMaxFnumScale = LJ_YM2612_LFO_PMStable[PMS];
	channelPtr->pmsFnumScale = 0;

	if (channelPtr->debugFlags & LJ_YM2612_DEBUG)
	{
		printf("SetLeftRightAmsPms channel:%d left:%d right:%d AMS:%d PMS:%d\n", channelPtr->id, left, right, AMS, PMS);
	}

	/* Update the connections for this channel */
	ym2612_channelSetConnections(channelPtr);
}

static void ym2612_channelSetFeedbackAlgorithm(LJ_YM2612_CHANNEL* const channelPtr, const LJ_YM_UINT8 data)
{
	/* 0xB0-0xB2 Feedback = Bits 5-3, Algorithm = Bits 0-2 */
	const LJ_YM_UINT8 algorithm = (data >> 0) & 0x7;
	const int feedback = (data >> 3) & 0x7;

	channelPtr->feedback = feedback;
	channelPtr->algorithm = algorithm;

	if (channelPtr->debugFlags & LJ_YM2612_DEBUG)
	{
		printf("SetFeedbackAlgorithm channel:%d feedback:%d algorithm:%d\n", channelPtr->id, feedback, algorithm);
	}

	/* Update the connections for this channel */
	ym2612_channelSetConnections(channelPtr);
}

static void ym2612_slotSetKeyScaleAttackRate(LJ_YM2612_SLOT* const slotPtr, const LJ_YM_UINT8 RS_AR, const LJ_YM_UINT32 debugFlags)
{
	/* Attack Rate = Bits 0-4 */
	const LJ_YM_UINT8 AR = ((RS_AR >> 0) & 0x1F);

	/* Rate Scale = Bits 6-7 */
	const LJ_YM_UINT8 RS = ((RS_AR >> 6) & 0x3);

	slotPtr->attackRate = AR;
	slotPtr->keyScale = (LJ_YM_UINT8)(3U - RS);
	slotPtr->omegaDirty = 1;

	if (debugFlags & LJ_YM2612_DEBUG)
	{
		printf("SetKeyScaleAttackRate channel:%d slot:%d AR:%d RS:%d\n", slotPtr->chanId, slotPtr->id, AR, RS);
	}
}

static void ym2612_slotSetAMDecayRate(LJ_YM2612_SLOT* const slotPtr, const LJ_YM_UINT8 AM_DR, const LJ_YM_UINT32 debugFlags)
{
	/* Decay Rate = Bits 0-4 */
	const LJ_YM_UINT8 DR = ((AM_DR >> 0) & 0x1F);

	/* AM  = Bit 7 */
	const LJ_YM_UINT8 AM = ((AM_DR >> 7) & 0x1);

	slotPtr->decayRate = DR;
	slotPtr->amEnable = AM;
	slotPtr->omegaDirty = 1;

	if (debugFlags & LJ_YM2612_DEBUG)
	{
		printf("SetAMDecayRate channel:%d slot:%d DR:%d AM:%d\n", slotPtr->chanId, slotPtr->id, DR, AM);
	}
}

static void ym2612_slotSetEGSSG(LJ_YM2612_SLOT* slotPtr, const LJ_YM_UINT8 egSSG, const LJ_YM_UINT32 debugFlags)
{
	/* SSG-EG: Bit 3 = Enabled, Bit 2 = Attack , Bit 1 = Invert, Bit 0 = hold */
	slotPtr->egSSG = egSSG;
	slotPtr->egSSGEnabled = ((egSSG >> 3) & 0x1);
	slotPtr->egSSGAttack = (LJ_YM_UINT8)((slotPtr->egSSGEnabled) & ((egSSG >> 2) & 0x1));
	slotPtr->egSSGInvert = (LJ_YM_UINT8)((slotPtr->egSSGEnabled) & ((egSSG >> 1) & 0x1));
	slotPtr->egSSGHold = (LJ_YM_UINT8)((slotPtr->egSSGEnabled) & ((egSSG >> 0) & 0x1));

	if (debugFlags & LJ_YM2612_DEBUG)
	{
		printf("SetEGSSGBits channel:%d slot:%d egSSG:0x%X enabled:%d attack:%d invert:%d hold:%d\n", slotPtr->chanId, slotPtr->id,
					slotPtr->egSSG, slotPtr->egSSGEnabled, slotPtr->egSSGAttack, slotPtr->egSSGInvert, slotPtr->egSSGHold );
	}
}

static void ym2612_slotSetSustainRate(LJ_YM2612_SLOT* const slotPtr, const LJ_YM_UINT8 AM_DR, const LJ_YM_UINT32 debugFlags)
{
	/* Sustain Rate = Bits 0-4 */
	const LJ_YM_UINT8 SR = ((AM_DR >> 0) & 0x1F);

	slotPtr->sustainRate = SR;
	slotPtr->omegaDirty = 1;

	if (debugFlags & LJ_YM2612_DEBUG)
	{
		printf("SetSustainRate channel:%d slot:%d SR:%d\n", slotPtr->chanId, slotPtr->id, SR);
	}
}

static void ym2612_slotSetSustainLevelReleaseRate(LJ_YM2612_SLOT* const slotPtr, const LJ_YM_UINT8 SL_RR, const LJ_YM_UINT32 debugFlags)
{
	/* Release rate = Bits 0-3 */
	const LJ_YM_UINT8 RR = ((SL_RR >> 0) & 0xF);

	/* Sustain Level = Bits 4-7 */
	const LJ_YM_UINT32 SL = ((SL_RR >> 4) & 0xF);

	/* Convert from 4-bits to usual 5-bits for rates */
	slotPtr->releaseRate = (LJ_YM_UINT8)((RR << 1) + 1);
	slotPtr->omegaDirty = 1;

	/* Convert from 5-bits to 10-bits used in EG attenuation */
	slotPtr->sustainLevelDB = (SL << (LJ_YM2612_EG_ATTENUATION_DB_NUM_BITS - 5U));

	if ((slotPtr->adsrState == LJ_YM2612_ADSR_DECAY) && (slotPtr->attenuationDB >= slotPtr->sustainLevelDB))
	{
		slotPtr->adsrState = LJ_YM2612_ADSR_SUSTAIN;
	}

	if (debugFlags & LJ_YM2612_DEBUG)
	{
		printf("SetSustainLevelReleaseRate channel:%d slot:%d SL:%d RR:%d\n", slotPtr->chanId, slotPtr->id, SL, RR);
	}
}

static void ym2612_slotSetTotalLevel(LJ_YM2612_SLOT* const slotPtr, const LJ_YM_UINT8 totalLevel, const LJ_YM_UINT32 debugFlags)
{
	/* Total Level = Bits 0-6 */
	const int TL = (totalLevel >> 0) & 0x7F;
	const int TL_EG_Shifted = (TL << LJ_YM2612_TL_EG_ATTENUATION_TABLE_SHIFT);
	const int TLscale = LJ_YM2612_EG_attenuationTable[TL_EG_Shifted & LJ_YM2612_EG_ATTENUATION_DB_TABLE_MASK];

	slotPtr->totalLevel = TLscale;

	if (debugFlags & LJ_YM2612_DEBUG)
	{
		printf("SetTotalLevel channel:%d slot:%d TL:%d scale:%f TLscale:%d\n", slotPtr->chanId,
					slotPtr->id, TL, ((float)(TLscale)/(float)(1<<LJ_YM2612_TL_SCALE_BITS)), TLscale);
	}
}

static void ym2612_slotSetDetuneMult(LJ_YM2612_SLOT* const slotPtr, const LJ_YM_UINT8 detuneMult, const LJ_YM_UINT32 debugFlags)
{
	/* Detune = Bits 4-6, Multiple = Bits 0-3 */
	const LJ_YM_UINT8 detune = (detuneMult >> 4) & 0x7;
	const LJ_YM_UINT8 multiple = (detuneMult >> 0) & 0xF;

	slotPtr->detune = detune;
	/* multiple = xN except N=0 then it is x1/2 store it as x2 */
	slotPtr->multiple = (LJ_YM_UINT8)(multiple ? (multiple << 1) : 1);

	if (debugFlags & LJ_YM2612_DEBUG)
	{
		printf("SetDetuneMult channel:%d slot:%d detune:%d mult:%d\n", slotPtr->chanId, slotPtr->id, detune, multiple);
	}
}

static void ym2612_computeBlockFnumKeyCode(int* const blockPtr, int* fnumPtr, int* keycodePtr, const int block_fnumMSB, const int fnumLSB)
{
	/* Block = Bits 3-5, Frequency Number MSB = Bits 0-2 (top 3-bits of frequency number) */
	const int block = (block_fnumMSB >> 3) & 0x7;
	const int fnumMSB = (block_fnumMSB >> 0) & 0x7;
	const int fnum = (fnumMSB << 8) + (fnumLSB & 0xFF);
	/* keycode = (block << 2) | (N4 << 1) | (N3 << 0) */
	/* N4 = Fnum Bit 11 */
	/* N3 = Fnum Bit 11 * (Bit 10 | Bit 9 | Bit 8) | !Bit 11 * Bit 10 * Bit 9 * Bit 8 */
	const int keycode = (block << 2) | LJ_YM2612_fnumKeycodeTable[fnum>>7];

	*blockPtr = block;
	*fnumPtr = fnum;
	*keycodePtr = keycode;
}

static void ym2612_channelSetFreqBlock(LJ_YM2612_CHANNEL* const channelPtr, const LJ_YM_UINT8 fnumLSB)
{
	const LJ_YM_UINT8 block_fnumMSB = channelPtr->block_fnumMSB;

	int block;
	int fnum;
	int keycode;
	ym2612_computeBlockFnumKeyCode(&block, &fnum, &keycode, block_fnumMSB, fnumLSB);

	channelPtr->block = block;
	channelPtr->fnum = fnum;
	channelPtr->keycode = (LJ_YM_UINT8)(keycode);
	channelPtr->omegaDirty = 1;

	if (channelPtr->debugFlags & LJ_YM2612_DEBUG)
	{
		printf("SetFreqBlock channel:%d block:%d fnum:%d 0x%X keycode:%d block_fnumMSB:0x%X fnumLSB:0x%X\n",
					 channelPtr->id, block, fnum, fnum, keycode, block_fnumMSB, fnumLSB);
	}
}

static void ym2612_channelSetBlockFnumMSB(LJ_YM2612_CHANNEL* const channelPtr, const LJ_YM_UINT8 data)
{
	channelPtr->block_fnumMSB = data;
}

static void ym2612_channelKeyOnOff(LJ_YM2612_CHANNEL* const channelPtr, LJ_YM2612* const ym2612Ptr, const LJ_YM_UINT8 slotOnOff)
{
	int i;
	int slotMask = 0x1;
	LJ_YM_UINT32 debugFlags = channelPtr->debugFlags;
	const LJ_YM_UINT8 csmKeyOn = ym2612Ptr->csmKeyOn;

	/* Set the start of wave */
	for (i = 0; i < LJ_YM2612_NUM_SLOTS_PER_CHANNEL; i++)
	{
		const int slot = i;
		LJ_YM2612_SLOT* const slotPtr = &(channelPtr->slot[slot]);

		if (slotOnOff & slotMask)
		{
			ym2612_slotKeyON(slotPtr, channelPtr, ym2612Ptr, debugFlags);
			slotPtr->normalKeyOn = 1;
		}
		else
		{
			/* Only key off if keyed on normally and CSM mode key off */
			if ((slotPtr->normalKeyOn == 1) && (csmKeyOn == 0))
			{
				ym2612_slotKeyOFF(slotPtr, debugFlags);
			}
			slotPtr->normalKeyOn = 0;
		}
		slotMask = slotMask << 1;
	}
}

static void ym2612_slotClear(LJ_YM2612_SLOT* const slotPtr)
{
	slotPtr->volume = 0;
	slotPtr->attenuationDB = 1023;

	slotPtr->detune = 0;
	slotPtr->multiple = 0;
	slotPtr->omega = 0;
	slotPtr->omegaDelta = 0;
	slotPtr->keycode = 0;

	slotPtr->totalLevel = 0;
	slotPtr->sustainLevelDB = 0;

	slotPtr->fmInputDelta = 0;
	slotPtr->modulationOutput[0] = NULL;
	slotPtr->modulationOutput[1] = NULL;
	slotPtr->modulationOutput[2] = NULL;

	slotPtr->adsrState = LJ_YM2612_ADSR_OFF;

	slotPtr->keyScale = 0;

	slotPtr->attackRate = 0;
	slotPtr->decayRate = 0;
	slotPtr->sustainRate = 0;
	slotPtr->releaseRate = 0;

	slotPtr->egAttackRateUpdateShift = 0;
	slotPtr->egDecayRateUpdateShift = 0;
	slotPtr->egSustainRateUpdateShift = 0;
	slotPtr->egReleaseRateUpdateShift = 0;

	slotPtr->egAttackRateUpdateMask = 0;
	slotPtr->egDecayRateUpdateMask = 0;
	slotPtr->egSustainRateUpdateMask = 0;
	slotPtr->egReleaseRateUpdateMask = 0;

	slotPtr->egSSG = 0;
	slotPtr->egSSGEnabled = 0;
	slotPtr->egSSGAttack = 0;
	slotPtr->egSSGInvert = 0;
	slotPtr->egSSGHold = 0;
	slotPtr->egSSGCycleEnded = 0;
	slotPtr->egSSGInvertOutput = 0;

	slotPtr->carrierOutputMask = 0x0;

	slotPtr->amEnable = 0;

	slotPtr->normalKeyOn = 0;

	slotPtr->omegaDirty = 1;

	slotPtr->chanId = 0;
	slotPtr->id = 0;
}

static void ym2612_channelClear(LJ_YM2612_CHANNEL* const channelPtr)
{
	int i;
	for (i = 0; i < LJ_YM2612_NUM_SLOTS_PER_CHANNEL; i++)
	{
		LJ_YM2612_SLOT* const slotPtr = &(channelPtr->slot[i]);
		ym2612_slotClear(slotPtr);
		slotPtr->id = i;
		slotPtr->chanId = channelPtr->id;
	}
	channelPtr->debugFlags = 0;
	channelPtr->feedback = 0;
	channelPtr->algorithm = 0;

	channelPtr->slot0Output[0] = 0;
	channelPtr->slot0Output[1] = 0;

	channelPtr->omegaDirty = 1;
	channelPtr->fnum = 0;
	channelPtr->block = 0;
	channelPtr->block_fnumMSB = 0;

	channelPtr->left = 0;
	channelPtr->right = 0;

	channelPtr->ams = 0;
	channelPtr->amsMaxdB = 0;

	channelPtr->pms = 0;
	channelPtr->pmsMaxFnumScale = 0;
	channelPtr->pmsFnumScale = 0;
}

static void ym2612_partClear(LJ_YM2612_PART* const partPtr)
{
	int i;
	for (i = 0; i < LJ_YM2612_NUM_REGISTERS; i++)
	{
		partPtr->reg[i] = 0x0;
	}
	for (i = 0; i < LJ_YM2612_NUM_CHANNELS_PER_PART; i++)
	{
		LJ_YM2612_CHANNEL* const channelPtr = &(partPtr->channel[i]);
		channelPtr->id = partPtr->id*LJ_YM2612_NUM_CHANNELS_PER_PART+i;
		ym2612_channelClear(channelPtr);
	}
}

static void ym2612_egDebugOutputDeltas(void)
{
	LJ_YM_UINT32 i;
	for (i = 0; i < 64; i++)
	{
		const LJ_YM_UINT32 egRate = i;
		const LJ_YM_UINT32 egRateUpdateShift = 0;
		LJ_YM_UINT32 cycle;
		for (cycle = 0; cycle < 8; cycle++)
		{
			const LJ_YM_UINT32 delta = ym2612_computeEGAttenuationDelta(egRate, cycle, egRateUpdateShift);
			printf("%d,", delta);
		}
		printf("    ");
		if ((i & 0x3) == 0x03)
		{
			printf("    %2d->%2d\n", (int)i&~0x3,i);
		}
	}
}

static void ym2612_egMakeData(LJ_YM2612_EG* const egPtr, LJ_YM2612* ym2612Ptr)
{
	int i;

	egPtr->timer.count = 0;

	/* FM output timer is (clockRate/144) but update code is every sampleRate times of that in the use of this code */
	/* This is a counter in the units of FM output samples : which is (clockRate/sampleRate)/144 = ym2612Ptr->baseFreqScale */
	egPtr->timer.addPerOutputSample = (int)((float)ym2612Ptr->baseFreqScale * (float)(1 << LJ_YM2612_EG_TIMER_NUM_BITS));

	egPtr->timerCountPerUpdate = (LJ_YM_UINT32)((float)LJ_YM2612_EG_TIMER_OUTPUT_PER_FM_SAMPLE * (float)(1 << LJ_YM2612_EG_TIMER_NUM_BITS));

	for (i = 0; i < LJ_YM2612_EG_ATTENUATION_TABLE_NUM_ENTRIES; i++)
	{
		const float dbScale = -(float)i * (1.0f / 64.0f);
		const float scale = (float)pow(2.0f, dbScale);
		const int intScale = (int)(scale * (float)(1 << LJ_YM2612_VOLUME_SCALE_BITS));
		LJ_YM2612_EG_attenuationTable[i] = intScale;
	}

	egPtr->counter = 0;

	/*printf("EG timerAdd:%d EG timerCountPerUpdate:%d\n", egPtr->timer.addPerOutputSample, egPtr->timerCountPerUpdate);*/

	/* Debug output of the EG deltas */
	if (0)
	{
		ym2612_egDebugOutputDeltas();
	}
	if (0)
	{
		LJ_YM_UINT32 att;
		for (att = 0; att <= LJ_YM2612_EG_ATTENUATION_DB_MAX; att++)
		{
			LJ_YM_UINT32 invertAttenuationDB = LJ_YM2612_EG_SSG_ATTENUATION_DB_MAX - att;
			invertAttenuationDB &= LJ_YM2612_EG_ATTENUATION_DB_MAX;
			printf("Invert:%d 0x%X -> %d 0x%X\n", att, att, invertAttenuationDB, invertAttenuationDB);
		}
	}
}

static void ym2612_timerClear(LJ_YM2612_TIMER* const timerPtr)
{
	timerPtr->count = 0;
	timerPtr->addPerOutputSample = 0;
}

static void ym2612_timerModeChanged(LJ_YM2612* const ym2612Ptr)
{
	const LJ_YM_UINT8 timerMode = ym2612Ptr->timerMode;
	const LJ_YM_UINT8 csmKeyOn = ym2612Ptr->csmKeyOn;
	const LJ_YM_UINT32 debugFlags = ym2612Ptr->debugFlags;
	/* Reset B = Bit 5, Reset A = Bit 4, Enable B = Bit 3, Enable A = Bit 2, Start B = Bit 1, Start A = Bit 0 */
	if (timerMode & 0x01)
	{
		/* See forum posts by Nemesis - only load register if it is stopped */
		if (ym2612Ptr->aTimer.count == 0)
		{
			ym2612Ptr->aTimer.count = ym2612Ptr->timerAstart;
			if (ym2612Ptr->debugFlags & LJ_YM2612_DEBUG)
			{
				printf("Start Timer A:%d Counter:%d Start:%d\n", ym2612Ptr->timerAvalue, ym2612Ptr->aTimer.count, ym2612Ptr->timerAstart);
			}
		}
	}
	else
	{
		ym2612Ptr->aTimer.count = 0;
	}
	if (timerMode & 0x02)
	{
		/* See forum posts by Nemesis - only load register if it is stopped */
		if (ym2612Ptr->bTimer.count == 0)
		{
			ym2612Ptr->bTimer.count = ym2612Ptr->timerBstart;
			if (ym2612Ptr->debugFlags & LJ_YM2612_DEBUG)
			{
				printf("Start Timer B:%d Counter:%d Start:%d\n", ym2612Ptr->timerBvalue, ym2612Ptr->bTimer.count, ym2612Ptr->timerBstart);
			}
		}
	}
	else
	{
		ym2612Ptr->bTimer.count = 0;
	}

	if (timerMode & 0x10)
	{
		ym2612Ptr->statusA = 0x0;
	}
	if (timerMode & 0x20)
	{
		ym2612Ptr->statusB = 0x0;
	}
	/* if CSM mode got turned off when they were on then key off */
	if (((ym2612Ptr->ch2Mode & 0x2) == 0x0) && (csmKeyOn == 1))
	{
		/* Key OFF via CSM */
		LJ_YM2612_CHANNEL* const ch2Ptr = ym2612Ptr->channels[2];
		int slot;
		for (slot = 0; slot < LJ_YM2612_NUM_SLOTS_PER_CHANNEL; slot++)
		{
			LJ_YM2612_SLOT* const slotPtr = &(ch2Ptr->slot[slot]);
			ym2612_slotCSMKeyOFF(slotPtr, debugFlags);
		}
    ym2612Ptr->csmKeyOn = 0;
	}
}

static void ym2612_lfoClear(LJ_YM2612_LFO* const lfoPtr, LJ_YM2612* ym2612Ptr)
{
	const LJ_YM_UINT8 lfoFreq = 0;
	const int freqSampleSteps = LJ_YM2612_LFO_freqSampleSteps[lfoFreq];

	ym2612_timerClear(&lfoPtr->timer);

	lfoPtr->counter = 0;
	lfoPtr->value = 0;
	lfoPtr->changed = 0;
	lfoPtr->enable = 0;

	lfoPtr->timerCountPerUpdate = (freqSampleSteps << LJ_YM2612_LFO_TIMER_NUM_BITS);
	lfoPtr->freq = lfoFreq;

	if (ym2612Ptr->debugFlags & LJ_YM2612_DEBUG)
	{
	}
}

static void ym2612_SetChannel2FreqBlock(LJ_YM2612* const ym2612Ptr, const LJ_YM_UINT8 slot, const LJ_YM_UINT8 fnumLSB)
{
	LJ_YM2612_CHANNEL* const channel2Ptr = &(ym2612Ptr->part[0].channel[2]);

	const LJ_YM_UINT8 block_fnumMSB = ym2612Ptr->channel2slotData[slot].block_fnumMSB;

	int block;
	int fnum;
	int keycode;
	ym2612_computeBlockFnumKeyCode(&block, &fnum, &keycode, block_fnumMSB, fnumLSB);

	ym2612Ptr->channel2slotData[slot].block = block;
	ym2612Ptr->channel2slotData[slot].fnum = fnum;
	ym2612Ptr->channel2slotData[slot].keycode = (LJ_YM_UINT8)(keycode);
	channel2Ptr->slot[slot].omegaDirty = 1;
	channel2Ptr->omegaDirty = 1;

	if (ym2612Ptr->debugFlags & LJ_YM2612_DEBUG)
	{
		printf("SetCh2FreqBlock channel:%d slot:%d block:%d fnum:%d 0x%X keycode:%d block_fnumMSB:0x%X fnumLSB:0x%X\n",
					 channel2Ptr->id, slot, block, fnum, fnum, keycode, block_fnumMSB, fnumLSB);
	}
}

static void ym2612_SetChannel2BlockFnumMSB(LJ_YM2612* const ym2612Ptr, const LJ_YM_UINT8 slot, const LJ_YM_UINT8 data)
{
	ym2612Ptr->channel2slotData[slot].block_fnumMSB = data;
}

static void ym2612_makeData(LJ_YM2612* const ym2612Ptr)
{
	const LJ_YM_UINT32 clock = ym2612Ptr->clockRate;
	const LJ_YM_UINT32 rate = ym2612Ptr->outputSampleRate;
	int i;
	float omegaScale;
	float freq;
	int intFreq;

	/* Fvalue = (144 * freq * 2^20 / masterClock) / 2^(B-1) */
	/* e.g. D = 293.7Hz F = 692.8 B=4 */
	/* freq = F * 2^(B-1) * baseFreqScale / (2^20) */
	/* freqScale = (chipClock/sampleRateOutput) / (144); */
	ym2612Ptr->baseFreqScale = ((float)clock / (float)rate) / 144.0f;

	for (i = 0; i < LJ_YM2612_FNUM_TABLE_NUM_ENTRIES; i++)
	{
		freq = ym2612Ptr->baseFreqScale * (float)i;
		/* Include the sin table size in the (1/2^20) */
		freq = freq * (float)(1 << (LJ_YM2612_FREQ_BITS - (20 - LJ_YM2612_SIN_TABLE_BITS)));

		intFreq = (int)(freq);
		LJ_YM2612_fnumTable[i] = intFreq;
	}
	/* From forums the internal register is 17-bits big so this is its overflow value */
	freq = ym2612Ptr->baseFreqScale * (float)(1<<17);
	freq = freq * (float)(1 << (LJ_YM2612_FREQ_BITS - (20 - LJ_YM2612_SIN_TABLE_BITS)));
	intFreq = (int)(freq);
	LJ_YM2612_fnumTable[LJ_YM2612_FNUM_TABLE_NUM_ENTRIES] = intFreq;

	omegaScale = (2.0f * M_PI / LJ_YM2612_SIN_TABLE_NUM_ENTRIES);
	for (i = 0; i < LJ_YM2612_SIN_TABLE_NUM_ENTRIES; i++)
	{
		const float omega = omegaScale * (float)i;
		const float sinValue = (float)sin(omega);
		const int scaledSin = (int)(sinValue * (float)(1 << LJ_YM2612_SIN_SCALE_BITS));
		LJ_YM2612_sinTable[i] = scaledSin;
	}

#if 0
	/* DT1 = -3 -> 3 */
	for (i = 0; i < 4; i++)
	{
		int keycode;
		for (keycode = 0; keycode < 32; keycode++)
		{
			int detuneScaleTableValue = LJ_YM2612_detuneScaleTable[i*32+keycode];
			float detuneMultiple = (float)detuneScaleTableValue * (0.529f/10.0f);
			int detFPValue = detuneScaleTableValue * ((529 * (1<<16) / 10000));
			float fpValue = (float)detFPValue / (float)(1<<16);
			printf("detuneMultiple FD=%d keycode=%d %f (%d) %f\n", i, keycode, detuneMultiple, detuneScaleTableValue,fpValue);
		}
	}
#endif /* #if 0 */
	for (i = 0; i < LJ_YM2612_NUM_FDS; i++)
	{
		int keycode;
		for (keycode = 0; keycode < LJ_YM2612_NUM_KEYCODES; keycode++)
		{
			const int detuneScaleTableValue = LJ_YM2612_detuneScaleTable[i*LJ_YM2612_NUM_KEYCODES+keycode];
			/* This should be including baseFreqScale to put in the same units as fnumTable - so it can be additive to it */
			const float detuneRate = ym2612Ptr->baseFreqScale * (float)detuneScaleTableValue;
			/* Include the sin table size in the (1/2^20) */
			const float detuneRateTableValue = detuneRate * (float)(1 << (LJ_YM2612_FREQ_BITS - (20 - LJ_YM2612_SIN_TABLE_BITS)));
			
			int detuneTableValue = (int)(detuneRateTableValue);
			LJ_YM2612_detuneTable[i*LJ_YM2612_NUM_KEYCODES+keycode] = detuneTableValue;
			LJ_YM2612_detuneTable[(i+LJ_YM2612_NUM_FDS)*LJ_YM2612_NUM_KEYCODES+keycode] = -detuneTableValue;
		}
	}
/*
	for (i = 3; i < 4; i++)
	{
		int keycode;
		for (keycode = 0; keycode < 32; keycode++)
		{
			const int detuneTable = LJ_YM2612_detuneTable[i*32+keycode];
			const int detuneScaleTableValue = LJ_YM2612_detuneScaleTable[i*32+keycode];
			printf("detuneTable FD=%d keycode=%d %d (%d)\n", i, keycode, detuneTable, detuneScaleTableValue);
		}
	}
*/

	ym2612_egMakeData(&ym2612Ptr->eg, ym2612Ptr);

	/* FM output timer is (clockRate/144) but update code is every sampleRate times of that in the use of this code */

	/* This is a counter in the units of FM output samples : which is (clockRate/sampleRate)/144 = ym2612Ptr->baseFreqScale */
	/* which is the base unit for Timer A and Timer B (but Timer B values get multiplied by 16) */
	ym2612Ptr->aTimer.addPerOutputSample = (int)((float)ym2612Ptr->baseFreqScale * (float)(1 << LJ_YM2612_TIMER_NUM_BITS));
	ym2612Ptr->bTimer.addPerOutputSample = (int)((float)ym2612Ptr->baseFreqScale * (float)(1 << LJ_YM2612_TIMER_NUM_BITS));
	/*printf("timerAdd:%d\n", ym2612Ptr->aTimer.addPerOutputSample);*/

	/* This is a counter in the units of FM output samples : which is (clockRate/sampleRate)/144 = ym2612Ptr->baseFreqScale */
	/* which is the base unit for the lfo timer */
	ym2612Ptr->lfo.timer.addPerOutputSample = (int)((float)ym2612Ptr->baseFreqScale * (float)(1 << LJ_YM2612_LFO_TIMER_NUM_BITS));
}

static void ym2612_clear(LJ_YM2612* const ym2612Ptr)
{
	int i;

	ym2612Ptr->dacValue = 0;
	ym2612Ptr->dacEnable = 0x0;
	ym2612Ptr->debugFlags = 0x0;
	ym2612Ptr->baseFreqScale = 0.0f;
	ym2612Ptr->regAddress = 0x0;	
	ym2612Ptr->slotWriteAddr = 0xFF;	
	ym2612Ptr->statusOutput = 0x0;
	ym2612Ptr->D07 = 0x0;
	ym2612Ptr->clockRate = 0;
	ym2612Ptr->outputSampleRate = 0;
	ym2612Ptr->sampleCount = 0;
	ym2612Ptr->csmKeyOn = 0;
	ym2612Ptr->ch2Mode = 0;
	ym2612Ptr->timerMode = 0;

	ym2612_timerClear(&ym2612Ptr->eg.timer);
	ym2612_timerClear(&ym2612Ptr->aTimer);
	ym2612_timerClear(&ym2612Ptr->bTimer);

	ym2612_lfoClear(&ym2612Ptr->lfo, ym2612Ptr);

	ym2612Ptr->statusA = 0;
	ym2612Ptr->timerAvalue = 0;
	ym2612Ptr->timerAstart = (LJ_YM_INT16)((1024 - ym2612Ptr->timerAvalue) << LJ_YM2612_TIMER_NUM_BITS);

	ym2612Ptr->statusB = 0;
	ym2612Ptr->timerBvalue = 0;
	ym2612Ptr->timerBstart = (LJ_YM_INT16)(((256 - ym2612Ptr->timerBvalue) << 4) << LJ_YM2612_TIMER_NUM_BITS);

	for (i = 0; i < LJ_YM2612_NUM_PARTS; i++)
	{
		LJ_YM2612_PART* const partPtr = &(ym2612Ptr->part[i]);
		ym2612_partClear(partPtr);
		partPtr->id = i;
	}
	for (i = 0; i < LJ_YM2612_NUM_PARTS; i++)
	{
		int channel;
		for (channel = 0; channel < LJ_YM2612_NUM_CHANNELS_PER_PART; channel++)
		{
			const int chan = (i * LJ_YM2612_NUM_CHANNELS_PER_PART) + channel;
			LJ_YM2612_CHANNEL* const channelPtr = &ym2612Ptr->part[i].channel[channel];
			ym2612Ptr->channels[chan] = channelPtr;
		}
	}
	for (i = 0; i < LJ_YM2612_NUM_SLOTS_PER_CHANNEL-1; i++)
	{
		ym2612Ptr->channel2slotData[i].fnum = 0;
		ym2612Ptr->channel2slotData[i].block = 0;
		ym2612Ptr->channel2slotData[i].block_fnumMSB = 0;
		ym2612Ptr->channel2slotData[i].keycode = 0;
	}
	for (i = 0; i < LJ_YM2612_NUM_REGISTERS; i++)
	{
		LJ_YM2612_validRegisters[i] = 0;
		LJ_YM2612_REGISTER_NAMES[i] = "UNKNOWN";
	}
	
	/* Known specific registers (only need explicit listing for the system type parameters which aren't per channel settings */
	LJ_YM2612_validRegisters[LJ_DAC] = 1;
	LJ_YM2612_REGISTER_NAMES[LJ_DAC] = "DAC";

	LJ_YM2612_validRegisters[LJ_DAC_EN] = 1;
	LJ_YM2612_REGISTER_NAMES[LJ_DAC_EN] = "DAC_EN";

	LJ_YM2612_validRegisters[LJ_KEY_ONOFF] = 1;
	LJ_YM2612_REGISTER_NAMES[LJ_KEY_ONOFF] = "KEY_ONOFF";

	LJ_YM2612_validRegisters[LJ_FREQLSB] = 1;
	LJ_YM2612_REGISTER_NAMES[LJ_FREQLSB] = "FREQ(LSB)";

	LJ_YM2612_validRegisters[LJ_BLOCK_FREQMSB] = 1;
	LJ_YM2612_REGISTER_NAMES[LJ_BLOCK_FREQMSB] = "BLOCK_FREQ(MSB)";

	LJ_YM2612_validRegisters[LJ_CH2_FREQLSB] = 1;
	LJ_YM2612_REGISTER_NAMES[LJ_CH2_FREQLSB] = "CH2_FREQ(LSB)";

	LJ_YM2612_validRegisters[LJ_CH2_BLOCK_FREQMSB] = 1;
	LJ_YM2612_REGISTER_NAMES[LJ_CH2_BLOCK_FREQMSB] = "CH2_BLOCK_FREQ(MSB)";

	LJ_YM2612_validRegisters[LJ_DETUNE_MULT] = 1;
	LJ_YM2612_REGISTER_NAMES[LJ_DETUNE_MULT] = "DETUNE_MULT";

	LJ_YM2612_validRegisters[LJ_LFO_EN_LFO_FREQ] = 1;
	LJ_YM2612_REGISTER_NAMES[LJ_LFO_EN_LFO_FREQ] = "LFO_END_LFO_FREQ";

	LJ_YM2612_validRegisters[LJ_TOTAL_LEVEL] = 1;
	LJ_YM2612_REGISTER_NAMES[LJ_TOTAL_LEVEL] = "TOTAL_LEVEL";

	LJ_YM2612_validRegisters[LJ_RATE_SCALE_ATTACK_RATE] = 1;
	LJ_YM2612_REGISTER_NAMES[LJ_RATE_SCALE_ATTACK_RATE] = "RATE_SCALE_ATTACK_RATE";

	LJ_YM2612_validRegisters[LJ_AM_DECAY_RATE] = 1;
	LJ_YM2612_REGISTER_NAMES[LJ_AM_DECAY_RATE] = "AM_DECAY_RATE";

	LJ_YM2612_validRegisters[LJ_SUSTAIN_RATE] = 1;
	LJ_YM2612_REGISTER_NAMES[LJ_SUSTAIN_RATE] = "SUSTAIN_RATE";

	LJ_YM2612_validRegisters[LJ_SUSTAIN_LEVEL_RELEASE_RATE] = 1;
	LJ_YM2612_REGISTER_NAMES[LJ_SUSTAIN_LEVEL_RELEASE_RATE] = "SUSTAIN_LEVEL_RELEASE_RATE";

	LJ_YM2612_validRegisters[LJ_EG_SSG_PARAMS] = 1;
	LJ_YM2612_REGISTER_NAMES[LJ_EG_SSG_PARAMS] = "EG_SSG_PARAMS";

	LJ_YM2612_validRegisters[LJ_FEEDBACK_ALGO] = 1;
	LJ_YM2612_REGISTER_NAMES[LJ_FEEDBACK_ALGO] = "FEEDBACK_ALGO";

	LJ_YM2612_validRegisters[LJ_LR_AMS_PMS] = 1;
	LJ_YM2612_REGISTER_NAMES[LJ_LR_AMS_PMS] = "LR_AMS_PMS";

	LJ_YM2612_validRegisters[LJ_TIMER_A_MSB] = 1;
	LJ_YM2612_REGISTER_NAMES[LJ_TIMER_A_MSB] = "TIMER_A_MSB";

	LJ_YM2612_validRegisters[LJ_TIMER_A_LSB] = 1;
	LJ_YM2612_REGISTER_NAMES[LJ_TIMER_A_LSB] = "TIMER_A_LSB";

	LJ_YM2612_validRegisters[LJ_TIMER_B] = 1;
	LJ_YM2612_REGISTER_NAMES[LJ_TIMER_B] = "TIMER_B";

	LJ_YM2612_validRegisters[LJ_CH2_MODE_TIMER_CONTROL] = 1;
	LJ_YM2612_REGISTER_NAMES[LJ_CH2_MODE_TIMER_CONTROL] = "CH2_MODE_TIMER_CONTROL";

	/* For parameters mark all the associated registers as valid */
	for (i = 0; i < LJ_YM2612_NUM_REGISTERS; i++)
	{
		if (LJ_YM2612_validRegisters[i] == 1)
		{
			/* Found a valid register */
			/* 0x20 = system which is specific set of registers */
			/* 0x30-0x90 = settings per channel per slot (channel = bottom 2 bits of reg, slot = reg / 4) */
			/* 0xA0-0xB6 = settings per channel (channel = bottom 2 bits of reg) */
			if ((i >= 0x20) && (i < 0x30))
			{
			}
			if ((i >= 0x30) && (i <= 0x90))
			{
				/* 0x30-0x90 = settings per channel per slot (channel = bottom 2 bits of reg, slot = reg / 4) */
				/* ignore registers which would be channel=3 they are invalid */
				int regBase = (i >> 4) << 4;
				int j;
				for (j = 0; j < 16; j++)
				{
					const char* const regBaseName = LJ_YM2612_REGISTER_NAMES[regBase];
					const int reg = regBase+j;
					if ((reg & 0x3) != 0x3)
					{
						LJ_YM2612_validRegisters[reg] = 1;
						LJ_YM2612_REGISTER_NAMES[reg] = regBaseName;
					}
				}
			}
			if ((i >= 0xA0) && (i <= 0xB6))
			{
				/* 0xA0-0xB0 = settings per channel (channel = bottom 2 bits of reg) */
				/* 0xA8->0xAE - settings per slot (slot = bottom 2 bits of reg) */
				/* 0xA9 = slot 0, 0xAA = slot 1, 0xA8 = slot 2, 0xA2 = slot 3 */
				/* 0xAD = slot 0, 0xAE = slot 1, 0xAC = slot 2, 0xA6 = slot 3 */
				/* ignore registers which would be channel=3 they are invalid */
				int regBase = (i >> 2) << 2;
				int j;
				for (j = 0; j < 3; j++)
				{
					const char* const regBaseName = LJ_YM2612_REGISTER_NAMES[regBase];
					const int reg = regBase+j;
					if ((reg & 0x3) != 0x3)
					{
						LJ_YM2612_validRegisters[reg] = 1;
						LJ_YM2612_REGISTER_NAMES[reg] = regBaseName;
					}
				}
			}
		}
	}
}

LJ_YM2612_RESULT ym2612_setRegister(LJ_YM2612* const ym2612Ptr, LJ_YM_UINT8 part, LJ_YM_UINT8 reg, LJ_YM_UINT8 data)
{
	LJ_YM_UINT32 debugFlags = 0;
	if (ym2612Ptr == NULL)
	{
		fprintf(stderr, "ym2612_setRegister:ym2612 is NULL\n");
		return LJ_YM2612_ERROR;
	}

	if (part >= LJ_YM2612_NUM_PARTS)
	{
		fprintf(stderr, "ym2612_setRegister:invalid part:%d max:%d \n",part, LJ_YM2612_NUM_PARTS);
		return LJ_YM2612_ERROR;
	}

	if (LJ_YM2612_validRegisters[reg] == 0)
	{
		fprintf(stderr, "ym2612_setRegister:unknown register:0x%X part:%d data:0x%X\n", reg, part, data);
		return LJ_YM2612_ERROR;
	}

	if ((reg < 0x30) && (part != 0))
	{
		fprintf(stderr, "ym2612_setRegister:unknown register:0x%X part:%d data:0x%X\n", reg, part, data);
		return LJ_YM2612_ERROR;
	}
	if (reg > 0xB6)
	{
		fprintf(stderr, "ym2612_setRegister:unknown register:0x%X part:%d data:0x%X\n", reg, part, data);
		return LJ_YM2612_ERROR;
	}
	debugFlags = ym2612Ptr->debugFlags;

	ym2612Ptr->part[part].reg[reg] = data;
	if (debugFlags & LJ_YM2612_DEBUG)
	{
		printf("ym2612:setRegister %s 0x%X\n", LJ_YM2612_REGISTER_NAMES[reg],data);
	}

	if (reg == LJ_LFO_EN_LFO_FREQ)
	{
		/* 0x22 Bit 3 = LFO enable, Bits 0-2 = LFO frequency */
		const LJ_YM_UINT8 lfoEnable = (data >> 3) & 0x1;
		const LJ_YM_UINT8 lfoFreq = (data >> 0) & 0x7;

		const int freqSampleSteps = LJ_YM2612_LFO_freqSampleSteps[lfoFreq];
		ym2612Ptr->lfo.timerCountPerUpdate = (freqSampleSteps << LJ_YM2612_LFO_TIMER_NUM_BITS);
		ym2612Ptr->lfo.freq = lfoFreq;

		ym2612Ptr->lfo.enable = lfoEnable;
		ym2612Ptr->lfo.changed = 1;

		return LJ_YM2612_OK;
	}
	else if (reg == LJ_TIMER_A_MSB)
	{
		/* 0x24 TIMER A MSB : Bits 0-7 = top 8-bits of timer A value */
		ym2612Ptr->timerAvalue = (LJ_YM_UINT16)(((LJ_YM_UINT16)data << 2) | (ym2612Ptr->timerAvalue & 0x3));
		ym2612Ptr->timerAstart = ((1024 - ym2612Ptr->timerAvalue) << LJ_YM2612_TIMER_NUM_BITS);
	}
	else if (reg == LJ_TIMER_A_LSB)
	{
		/* 0x25 TIMER A LSB : Bits 0-1 = bottom 2-bits of timer A value */
		ym2612Ptr->timerAvalue = (LJ_YM_UINT16)((ym2612Ptr->timerAvalue & ~0x3) | (data & 0x3));
		ym2612Ptr->timerAstart = ((1024 - ym2612Ptr->timerAvalue) << LJ_YM2612_TIMER_NUM_BITS);
	}
	else if (reg == LJ_TIMER_B)
	{
		/* 0x26 TIMER B : Bits 0-7 = timer B value */
		ym2612Ptr->timerBvalue  = data;
		ym2612Ptr->timerBstart = (((256 - ym2612Ptr->timerBvalue) << 4) << LJ_YM2612_TIMER_NUM_BITS);
	}
	else if (reg == LJ_CH2_MODE_TIMER_CONTROL)
	{
		/* 0x27 Ch2 Mode = Bits 6-7, */
		/* 0x27 Reset B = Bit 5, Reset A = Bit 4, Enable B = Bit 3, Enable A = Bit 2, Start B = Bit 1, Start A = Bit 0 */
		ym2612Ptr->ch2Mode = (data >> 6) & 0x3;
	
		ym2612Ptr->timerMode = (data & 0x3F);
		ym2612_timerModeChanged(ym2612Ptr);
		return LJ_YM2612_OK;
	}
	else if (reg == LJ_DAC_EN)
	{
		/* 0x2A DAC Bits 0-7 */
		ym2612Ptr->dacEnable = (int)(~0 * ((data & 0x80) >> 7));
		if (debugFlags & LJ_YM2612_DEBUG)
		{
			printf("dacEnable 0x%X\n", ym2612Ptr->dacEnable);
		}
		return LJ_YM2612_OK;
	}
	else if (reg == LJ_DAC)
	{
		/* 0x2B DAC en = Bit 7 */
#if LJ_YM2612_DAC_SHIFT >= 0
		ym2612Ptr->dacValue = ((int)(data - 0x80)) << LJ_YM2612_DAC_SHIFT;
#else /* #if LJ_YM2612_DAC_SHIFT >= 0 */
		ym2612Ptr->dacValue = ((int)(data - 0x80)) >> -LJ_YM2612_DAC_SHIFT;
#endif /* #if LJ_YM2612_DAC_SHIFT >= 0 */
		if (debugFlags & LJ_YM2612_DEBUG)
		{
			printf("dacValue: 0x%X -> 0x%X\n", data, ym2612Ptr->dacValue);
		}
		return LJ_YM2612_OK;
	}
	else if (reg == LJ_KEY_ONOFF)
	{
		/* 0x28 Slot = Bits 4-7, Channel ID = Bits 0-2, part 0 or 1 is Bit 3 */
		const int channel = data & 0x03;
		if (channel != 0x03)
		{
			const LJ_YM_UINT8 slotOnOff = (data >> 4);
			const int partParam = (data & 0x4) >> 2;
			
			LJ_YM2612_CHANNEL* const channelPtr = &ym2612Ptr->part[partParam].channel[channel];

			ym2612_channelKeyOnOff(channelPtr, ym2612Ptr, slotOnOff);
			if (debugFlags & LJ_YM2612_DEBUG)
			{
				printf("LJ_KEY_ONFF part:%d channel:%d slotOnOff:0x%X data:0x%X\n", partParam, channel, slotOnOff, data);
			}
		}
		return LJ_YM2612_OK;
	}
	else if ((reg >= 0x30) && (reg <= 0x9F))
	{
		/* 0x30-0x90 = settings per channel per slot (channel = bottom 2 bits of reg, slotReg = reg / 4) */
		/* Use a table to convert from slotReg -> internal slot */
		const int channel = reg & 0x3;
		const int slotReg = (reg >> 2) & 0x3;
		const int slot = LJ_YM2612_slotTable[slotReg];
		const int regParameter = reg & 0xF0;

		LJ_YM2612_CHANNEL* const channelPtr = &ym2612Ptr->part[part].channel[channel];
		LJ_YM2612_SLOT* const slotPtr = &channelPtr->slot[slot];

		if (channel == 0x3)
		{
			fprintf(stderr, "ym2612:setRegister 0x%X %s 0x%X invalid channel 0x3\n", reg, LJ_YM2612_REGISTER_NAMES[reg], data);
			return LJ_YM2612_ERROR;
		}

		if (regParameter == LJ_DETUNE_MULT)
		{
			ym2612_slotSetDetuneMult(slotPtr, data, debugFlags);
			if (debugFlags & LJ_YM2612_DEBUG)
			{
				printf("LJ_DETUNE_MULT part:%d channel:%d slot:%d slotReg:%d data:0x%X\n", part, channel, slot, slotReg, data);
			}
			return LJ_YM2612_OK;
		}
		else if (regParameter == LJ_TOTAL_LEVEL)
		{
			ym2612_slotSetTotalLevel(slotPtr, data, debugFlags);
			if (debugFlags & LJ_YM2612_DEBUG)
			{
				printf("LJ_TOTAL_LEVEL part:%d channel:%d slot:%d slotReg:%d data:0x%X\n", part, channel, slot, slotReg, data);
			}
			return LJ_YM2612_OK;
		}
		else if (regParameter == LJ_RATE_SCALE_ATTACK_RATE)
		{
			ym2612_slotSetKeyScaleAttackRate(slotPtr, data, debugFlags);
			if (debugFlags & LJ_YM2612_DEBUG)
			{
				printf("LJ_RATE_SCALE_ATTACK_RATE part:%d channel:%d slot:%d slotReg:%d data:0x%X\n", part, channel, slot, slotReg, data);
			}
			return LJ_YM2612_OK;
		}
		else if (regParameter == LJ_AM_DECAY_RATE)
		{
			ym2612_slotSetAMDecayRate(slotPtr, data, debugFlags);
			if (debugFlags & LJ_YM2612_DEBUG)
			{
				printf("LJ_AM_DECAY_RATE part:%d channel:%d slot:%d slotReg:%d data:0x%X\n", part, channel, slot, slotReg, data);
			}
			return LJ_YM2612_OK;
		}
		else if (regParameter == LJ_SUSTAIN_RATE)
		{
			ym2612_slotSetSustainRate(slotPtr, data, debugFlags);
			if (debugFlags & LJ_YM2612_DEBUG)
			{
				printf("LJ_SUSTAIN_RATE part:%d channel:%d slot:%d slotReg:%d data:0x%X\n", part, channel, slot, slotReg, data);
			}
			return LJ_YM2612_OK;
		}
		else if (regParameter == LJ_SUSTAIN_LEVEL_RELEASE_RATE)
		{
			ym2612_slotSetSustainLevelReleaseRate(slotPtr, data, debugFlags);
			if (debugFlags & LJ_YM2612_DEBUG)
			{
				printf("LJ_SUSTAIN_LEVEL_RELEASE_RATE part:%d channel:%d slot:%d slotReg:%d data:0x%X\n", part, channel, slot, slotReg, data);
			}
			return LJ_YM2612_OK;
		}
		else if (regParameter == LJ_EG_SSG_PARAMS)
		{
			ym2612_slotSetEGSSG(slotPtr, data, debugFlags);
			if (debugFlags & LJ_YM2612_DEBUG)
			{
				printf("LJ_EG_SSG_PARAMS part:%d channel:%d slot:%d slotReg:%d data:0x%X\n", part, channel, slot, slotReg, data);
			}
			return LJ_YM2612_OK;
		}
	}
	else if ((reg >= 0xA0) && (reg <= 0xB6))
	{
		/* 0xA0-0xB0 = settings per channel (channel = bottom 2 bits of reg) */
		const int channel = reg & 0x3;
		const int regParameter = reg & 0xFC;

		LJ_YM2612_CHANNEL* const channelPtr = &ym2612Ptr->part[part].channel[channel];

		if (channel == 0x3)
		{
			fprintf(stderr, "ym2612:setRegister 0x%X %s 0x%X invalid channel 0x3\n", reg, LJ_YM2612_REGISTER_NAMES[reg], data);
			return LJ_YM2612_ERROR;
		}

		if (regParameter == LJ_FREQLSB)
		{
			/* 0xA0-0xA2 Frequency number LSB = Bits 0-7 (bottom 8 bits of frequency number) */
			ym2612_channelSetFreqBlock(channelPtr, data);
			if (debugFlags & LJ_YM2612_DEBUG)
			{
				printf("LJ_FREQLSB part:%d channel:%d data:0x%X\n", part, channel, data);
			}
			return LJ_YM2612_OK;
		}
		else if (regParameter == LJ_BLOCK_FREQMSB)
		{
			/* 0xA4-0xA6 Block = Bits 5-3, Frequency Number MSB = Bits 0-2 (top 3-bits of frequency number) */
			ym2612_channelSetBlockFnumMSB(channelPtr, data);
			if (debugFlags & LJ_YM2612_DEBUG)
			{
				printf("LJ_BLOCK_FREQMSB part:%d channel:%d data:0x%X\n", part, channel, data);
			}
			return LJ_YM2612_OK;
		}
		else if (regParameter == LJ_CH2_FREQLSB)
		{
			/* 0xA8-0xAA Channel 2 Frequency number LSB = Bits 0-7 (bottom 8 bits of frequency number) */
			/* 0xA8-0xAA - settings per slot (slot = bottom 2 bits of reg) */
			/* 0xA9 = slot 0, 0xAA = slot 1, 0xA8 = slot 2, 0xA2 = slot 3 */
			/* Only on part 0 */
			if (part == 0)
			{
				/* It is only for part 0 and channel 2 so just store it globally */
				/* LJ_YM2612_slotTable is 0->0, 1->2, 2->1, 3->3 we want: 0->1, 1->0, 3->2 */
				/* which is LJ_YM2612_slotTable[slot+1]-1 : 0+1->2-1=1, 1+1->1-1=0, 2+1->3-1=2 */
				const int slotParameter = reg & 0x3;
				const LJ_YM_UINT8 slot = (LJ_YM_UINT8)(LJ_YM2612_slotTable[slotParameter+1] - 1U);

				ym2612_SetChannel2FreqBlock(ym2612Ptr, slot, data);
				if (debugFlags & LJ_YM2612_DEBUG)
				{
					printf("LJ_CH2_FREQLSB slot:%d slotParameter:%d data:0x%X\n", slot, slotParameter, data);
				}
			}
			return LJ_YM2612_OK;
		}
		else if (regParameter == LJ_CH2_BLOCK_FREQMSB)
		{
			/* 0xAC-0xAE Block = Bits 5-3, Channel 2 Frequency Number MSB = Bits 0-2 (top 3-bits of frequency number) */
			/* 0xAC-0xAE - settings per slot (slot = bottom 2 bits of reg) */
			/* 0xAD = slot 0, 0xAE = slot 1, 0xAC = slot 2, 0xA6 = slot 3 */
			/* Only on part 0 */
			if (part == 0)
			{
				/* It is only for part 0 and channel 2 so just store it globally */
				/* LJ_YM2612_slotTable is 0->0, 1->2, 2->1, 3->3 we want: 0->1, 1->0, 3->2 */
				/* which is LJ_YM2612_slotTable[slot+1]-1 : 0+1->2-1=1, 1+1->1-1=0, 2+1->3-1=2 */
				const int slotParameter = reg & 0x3;
				const LJ_YM_UINT8 slot = (LJ_YM_UINT8)(LJ_YM2612_slotTable[slotParameter+1] - 1U);

				ym2612_SetChannel2BlockFnumMSB(ym2612Ptr, slot, data);
				if (debugFlags & LJ_YM2612_DEBUG)
				{
					printf("LJ_CH2_BLOCK_FREQMSB slot:%d slotParameter:%d data:0x%X\n", slot, slotParameter, data);
				}
			}
			return LJ_YM2612_OK;
		}
		else if (regParameter == LJ_FEEDBACK_ALGO)
		{
			/* 0xB0-0xB2 Feedback = Bits 5-3, Algorithm = Bits 0-2 */
			ym2612_channelSetFeedbackAlgorithm(channelPtr, data);
			if (debugFlags & LJ_YM2612_DEBUG)
			{
				printf("LJ_FEEDBACK_ALGO part:%d channel:%d data:0x%X\n", part, channel, data);
			}
			return LJ_YM2612_OK;
		}
		else if (regParameter == LJ_LR_AMS_PMS)
		{
			/* 0xB4-0xB6 Feedback = Left= Bit 7, Right= Bit 6, AMS = Bits 3-5, PMS = Bits 0-1 */
			ym2612_channelSetLeftRightAmsPms(channelPtr, data);
			if (debugFlags & LJ_YM2612_DEBUG)
			{
				printf("LJ_LR_AMS_PMS part:%d channel:%d data:0x%X\n", part, channel, data);
			}
			return LJ_YM2612_OK;
		}
	}

	return LJ_YM2612_OK;
}

/*
* ////////////////////////////////////////////////////////////////////
* //
* // External data & functions
* //
* ////////////////////////////////////////////////////////////////////
*/

LJ_YM2612* LJ_YM2612_create(const int clockRate, const int outputSampleRate)
{
	LJ_YM2612* const ym2612Ptr = malloc(sizeof(LJ_YM2612));
	const LJ_YM_UINT8 panFull = (1<<7)|(1<<6);

	if (ym2612Ptr == NULL)
	{
		fprintf(stderr, "LJ_YM2612_create:malloc failed\n");
		return NULL;
	}

	ym2612_clear(ym2612Ptr);

	ym2612Ptr->clockRate = (LJ_YM_UINT32)(clockRate);
	ym2612Ptr->outputSampleRate = (LJ_YM_UINT32)(outputSampleRate);

	ym2612_makeData(ym2612Ptr);

	/* Set left, right bits to on by default at startup */

	/* Part 0 channel 0, 1, 2 */
	ym2612_setRegister(ym2612Ptr, 0, LJ_LR_AMS_PMS+0, panFull);
	ym2612_setRegister(ym2612Ptr, 0, LJ_LR_AMS_PMS+1, panFull);
	ym2612_setRegister(ym2612Ptr, 0, LJ_LR_AMS_PMS+2, panFull);

	/* Part 1 channel 0, 1, 2 */
	ym2612_setRegister(ym2612Ptr, 1, LJ_LR_AMS_PMS+0, panFull);
	ym2612_setRegister(ym2612Ptr, 1, LJ_LR_AMS_PMS+1, panFull);
	ym2612_setRegister(ym2612Ptr, 1, LJ_LR_AMS_PMS+2, panFull);

	return ym2612Ptr;
}

LJ_YM2612_RESULT LJ_YM2612_setFlags(LJ_YM2612* const ym2612Ptr, const unsigned int flags)
{
	int part;
	if (ym2612Ptr == NULL)
	{
		fprintf(stderr, "LJ_YM2612_setFlags:ym2612 is NULL\n");
		return LJ_YM2612_ERROR;
	}

	ym2612Ptr->debugFlags = flags;

	for (part = 0; part < LJ_YM2612_NUM_PARTS; part++)
	{
		int channel;
		for (channel = 0; channel < LJ_YM2612_NUM_CHANNELS_PER_PART; channel++)
		{
			LJ_YM2612_CHANNEL* const channelPtr = &ym2612Ptr->part[part].channel[channel];
			channelPtr->debugFlags = flags;
		}
	}
	return LJ_YM2612_OK;
}

LJ_YM2612_RESULT LJ_YM2612_destroy(LJ_YM2612* const ym2612Ptr)
{
	if (ym2612Ptr == NULL)
	{
		fprintf(stderr, "LJ_YM2612_destroy:ym2612 is NULL\n");
		return LJ_YM2612_ERROR;
	}
	ym2612_clear(ym2612Ptr);
	free(ym2612Ptr);

	return LJ_YM2612_OK;
}


LJ_YM2612_RESULT LJ_YM2612_generateOutput(LJ_YM2612* const ym2612Ptr, int numCycles, LJ_YM_INT16* output[2])
{
	LJ_YM_INT16* outputLeft = output[0];
	LJ_YM_INT16* outputRight = output[1];
	int dacValue = 0;
	int dacValueLeft = 0;
	int dacValueRight = 0;
	int sample;
	const LJ_YM_UINT32 debugFlags = ym2612Ptr->debugFlags;

	int channel;
	int slot;

	int outputChannelMask = 0xFF;
	int channelMask = 0x1;

	if (ym2612Ptr == NULL)
	{
		fprintf(stderr, "LJ_YM2612_generateOutput:ym2612 is NULL\n");
		return LJ_YM2612_ERROR;
	}

	/* Global state update - LFO, DAC, SSG */
	dacValue = (ym2612Ptr->dacValue & ym2612Ptr->dacEnable);
	if (debugFlags & LJ_YM2612_NODAC)
	{
		dacValue = 0x0;
	}
	dacValueLeft = (dacValue & ym2612Ptr->channels[5]->left);
	dacValueRight = (dacValue & ym2612Ptr->channels[5]->right);
	if (debugFlags & LJ_YM2612_DEBUG)
	{
		printf("dacValue 0x%X left 0x%X right 0x%X\n", dacValue, dacValueLeft, dacValueRight);
	}

	if (debugFlags & LJ_YM2612_ONECHANNEL)
	{
		/* channel starts at 0 */
		const int debugChannel =((debugFlags >> LJ_YM2612_ONECHANNEL_SHIFT) & LJ_YM2612_ONECHANNEL_MASK);
		outputChannelMask = 1 << debugChannel;
	}

	/* For each cycle */
	/* Loop over channels updating them, mix them, then output them into the buffer */
	for (sample = 0; sample < numCycles; sample++)
	{
		int ssgUpdate = 1;
		int mixedLeft = 0;
		int mixedRight = 0;
		short outL = 0;
		short outR = 0;

		LJ_YM_UINT8 csmKeyOff = ym2612Ptr->csmKeyOn;
	
		/* DAC enable blocks channel 5 */
		const int numChannelsToProcess = ym2612Ptr->dacEnable ? LJ_YM2612_NUM_CHANNELS_TOTAL-1 : LJ_YM2612_NUM_CHANNELS_TOTAL;

		/* LFO update */
		ym2612Ptr->lfo.timer.count += ym2612Ptr->lfo.timer.addPerOutputSample;
		while (ym2612Ptr->lfo.timer.count >= ym2612Ptr->lfo.timerCountPerUpdate)
		{
			/* Update the LFO counter */
			ym2612Ptr->lfo.timer.count -= ym2612Ptr->lfo.timerCountPerUpdate;
			ym2612Ptr->lfo.counter++;
			ym2612Ptr->lfo.changed = 1;
		}

		if (ym2612Ptr->lfo.enable == 1)
		{
			if (ym2612Ptr->lfo.changed == 1)
			{
				/* Using (1<<LJ_YM2612_LFO_PHI_NUM_BITS) steps for a full wave : chosen to be 128 */
				const LJ_YM_UINT32 lfoCounter = ym2612Ptr->lfo.counter & LJ_YM2612_LFO_PHI_MASK;
				const LJ_YM_UINT32 phi = (lfoCounter << (LJ_YM2612_SIN_TABLE_BITS-LJ_YM2612_LFO_PHI_NUM_BITS));

				const int scaledSin = LJ_YM2612_sinTable[phi & LJ_YM2612_SIN_TABLE_MASK];

				ym2612Ptr->lfo.value = scaledSin;
				ym2612Ptr->lfo.changed = 0;
			}
		}

		/* EG update */
		ym2612Ptr->eg.timer.count += ym2612Ptr->eg.timer.addPerOutputSample;
		while (ym2612Ptr->eg.timer.count >= ym2612Ptr->eg.timerCountPerUpdate)
		{
			/* Update the EG counter */
			ym2612Ptr->eg.timer.count -= ym2612Ptr->eg.timerCountPerUpdate;
			ym2612Ptr->eg.counter++;

			for (channel = 0; channel < LJ_YM2612_NUM_CHANNELS_TOTAL; channel++)
			{
				LJ_YM2612_CHANNEL* const channelPtr = ym2612Ptr->channels[channel];

				for (slot = 0; slot < LJ_YM2612_NUM_SLOTS_PER_CHANNEL; slot++)
				{
					LJ_YM2612_SLOT* const slotPtr = &(channelPtr->slot[slot]);

					ym2612_slotUpdateSSG(slotPtr, channelPtr, ym2612Ptr, debugFlags);
					ym2612_slotUpdateEG(slotPtr, ym2612Ptr->eg.counter, debugFlags);
					ssgUpdate = 0;

					slotPtr->volume = LJ_YM2612_CLAMP_VOLUME(slotPtr->volume);
					if (slotPtr->volume < 0)
					{
						slotPtr->volume = 0;
					}
				}
			}
		}
		/* SSG update */
		if (ssgUpdate == 1)
		{
			for (channel = 0; channel < LJ_YM2612_NUM_CHANNELS_TOTAL; channel++)
			{
				LJ_YM2612_CHANNEL* const channelPtr = ym2612Ptr->channels[channel];

				for (slot = 0; slot < LJ_YM2612_NUM_SLOTS_PER_CHANNEL; slot++)
				{
					LJ_YM2612_SLOT* const slotPtr = &(channelPtr->slot[slot]);
					ym2612_slotUpdateSSG(slotPtr, channelPtr, ym2612Ptr, debugFlags);
				}
			}
		}

		for (channel = 0; channel < numChannelsToProcess; channel++)
		{
			LJ_YM2612_CHANNEL* const channelPtr = ym2612Ptr->channels[channel];
			int channelOutput = 0;

			/* LFO value is in same units as scaledSin i.e. LJ_YM2612_SIN_SCALE_BITS */
			/* channel ams (0, 1.4, 5.9, 11.8 dB) -> 0, 2, 8, 16 in TL units = 2^(-x/8) */
			const int amsMaxdB = channelPtr->amsMaxdB;
			/* NOTE: on Kega & Mame output this frequency is twice - they appear to be half the spped of the docs */
			/* NOTE: to match Kega divide lfo.value by 2 or double lfo freq steps */
			/* NOTE: to match Kega might also need to adjust the lfo freq steps to get 100% accurate lfo freqeuncy */
			/* NOTE: on Kega & Mame output the AMS output looks like a triangle wave not a sine wave */
			/* TODO: measure it on real hardware */
			const int amsdB = abs((amsMaxdB * ym2612Ptr->lfo.value) >> LJ_YM2612_SIN_SCALE_BITS);
			const int amsVolumeScale = LJ_YM2612_EG_attenuationTable[amsdB & LJ_YM2612_EG_ATTENUATION_DB_TABLE_MASK];

			int channelOmegaDirty = channelPtr->omegaDirty;

			if ((channelMask & outputChannelMask) == 0)
			{
				channelMask = (channelMask << 1);
				continue;
			}

			/* LFO PMS fnum scale could force a recalc of wave params */
			if (ym2612Ptr->lfo.enable == 1)
			{
				/* LFO PMS settings and how it changes fnum value : a scaling to it e.g fnum * (1+scale) */
				const int pmsMaxFnumScale = channelPtr->pmsMaxFnumScale;
				int pmsFnumScale = ((pmsMaxFnumScale * ym2612Ptr->lfo.value) >> LJ_YM2612_SIN_SCALE_BITS);

				if (pmsMaxFnumScale != 0)
				{
					if (pmsFnumScale != channelPtr->pmsFnumScale)
					{
						channelOmegaDirty = 1;
						channelPtr->pmsFnumScale = pmsFnumScale;
					}
				}
			}

			for (slot = 0; slot < LJ_YM2612_NUM_SLOTS_PER_CHANNEL; slot++)
			{
				LJ_YM2612_SLOT* const slotPtr = &(channelPtr->slot[slot]);

				int carrierOutput = 0;

				int omegaDirty = 0;
				int deltaPhi;
				int phi;
				int scaledSin;
				int slotOutput;
				LJ_YM_UINT32 attenuationDB = slotPtr->attenuationDB;

				const int OMEGA = slotPtr->omega;
				const int slotPhi = (OMEGA >> LJ_YM2612_FREQ_BITS);
		
				/* For normal mode or channels that aren't channel 2 or for slot 3 */
				if ((ym2612Ptr->ch2Mode == 0) || (channel != 2) || (slot == 3))
				{
					omegaDirty = channelOmegaDirty;
				}

				/* Compute the omega delta value */
				if ((slotPtr->omegaDirty == 1) || (omegaDirty == 1))
				{
					ym2612_slotComputeWaveParams(slotPtr, channelPtr, ym2612Ptr, debugFlags);
				}
	
				/* Slot 0 feedback */
				if (slot == 0)
				{
					/* Average of the last 2 samples */
					slotPtr->fmInputDelta = (channelPtr->slot0Output[0] + channelPtr->slot0Output[1]);
					if (channelPtr->feedback != 0)
					{
						/* Average = >> 1 */
						/* From docs: Feedback is 0 = + 0, 1 = +PI/16, 2 = +PI/8, 3 = +PI/4, 4 = +PI/2, 5 = +PI, 6 = +2 PI, 7 = +4 PI */
						/* fmDelta is currently in -1->1 which is mapped to 0->2PI (and correct angle units) by the deltaPhi scaling */
						/* feedback 6 = +1 in these units, so feedback 6 maps to 2^0: << feedback then >> 6 */
						/* deltaPhi scaling has an implicit x4 so /4 here: >> 2 */

						/* Results in: >> (1+6+2-feedback) >> (9-feedback) */
						slotPtr->fmInputDelta = slotPtr->fmInputDelta >> (9 - channelPtr->feedback);
					}
					else
					{
						slotPtr->fmInputDelta = 0;
					}
				}
				/* Phi needs to have the fmInputDelta added to it to make algorithms work */
#if LJ_YM2612_DELTA_PHI_SCALE >= 0
				deltaPhi = (slotPtr->fmInputDelta >> LJ_YM2612_DELTA_PHI_SCALE);
#else /* #if LJ_YM2612_DELTA_PHI_SCALE >= 0 */
				deltaPhi = (slotPtr->fmInputDelta << -LJ_YM2612_DELTA_PHI_SCALE);
#endif /* #if LJ_YM2612_DELTA_PHI_SCALE >= 0 */

				phi = slotPhi + deltaPhi;

				scaledSin = LJ_YM2612_sinTable[phi & LJ_YM2612_SIN_TABLE_MASK];

				/* Handle SSG output inversion - convert attenuationDB -> volume */
				if ((slotPtr->egSSGEnabled == 1) &&
					((slotPtr->adsrState != LJ_YM2612_ADSR_RELEASE) && (slotPtr->adsrState != LJ_YM2612_ADSR_OFF)) &&
					(slotPtr->egSSGInvertOutput ^ slotPtr->egSSGAttack))
				{
					LJ_YM_UINT32 invertAttenuationDB = LJ_YM2612_EG_SSG_ATTENUATION_DB_MAX - attenuationDB;
					invertAttenuationDB &= LJ_YM2612_EG_ATTENUATION_DB_MAX;
					if (debugFlags & LJ_YM2612_DEBUG)
					{
						printf("Invert:%d -> %d\n", attenuationDB, invertAttenuationDB);
					}
					attenuationDB = invertAttenuationDB;
				}

				/* Convert attenuation DB into volume scale */
				slotPtr->volume = LJ_YM2612_EG_attenuationTable[attenuationDB & LJ_YM2612_EG_ATTENUATION_DB_TABLE_MASK];

				slotOutput = (slotPtr->volume * scaledSin) >> LJ_YM2612_SIN_SCALE_BITS;

				/* Scale by TL (total level = 0->1) */
				slotOutput = (slotOutput * slotPtr->totalLevel) >> LJ_YM2612_TL_SCALE_BITS;

				if (slotPtr->amEnable == 1)
				{
					if (ym2612Ptr->lfo.enable == 1)
					{
						/* Scale by LFO AM value */
						/* amsVolumeScale is in same units as TL i.e. LJ_YM2612_LFO_AM_SCALE_BITS */
						slotOutput = (slotOutput * amsVolumeScale) >> LJ_YM2612_LFO_AM_SCALE_BITS;
						/*printf("amsMaxdB %d amsdB %d amsVolumeScale %d\n", amsMaxdB, amsdB, amsVolumeScale);*/
					}
				}

				/* Add this output onto the input of the slot in the algorithm */
				if (slotPtr->modulationOutput[0] != NULL)
				{
					*slotPtr->modulationOutput[0] += slotOutput;
				}
				if (slotPtr->modulationOutput[1] != NULL)
				{
					*slotPtr->modulationOutput[1] += slotOutput;
				}
				if (slotPtr->modulationOutput[2] != NULL)
				{
					*slotPtr->modulationOutput[2] += slotOutput;
				}

				if (slot == 0)
				{
					/* Save the slot 0 output (the last 2 outputs are saved), used as input for the slot 0 feedback */
					channelPtr->slot0Output[0] = channelPtr->slot0Output[1];
					channelPtr->slot0Output[1] = slotOutput;
				}

				/* Keep within +/- 1 */
				slotOutput = LJ_YM2612_CLAMP_VOLUME(slotOutput);

				carrierOutput = slotOutput & slotPtr->carrierOutputMask;
				channelOutput += carrierOutput;

				if (debugFlags & LJ_YM2612_DEBUG)
				{
					if (slotOutput != 0)
					{
						printf("Channel:%d Slot:%d slotOutput:%d carrierOutput:%d\n", channelPtr->id, slot, slotOutput, carrierOutput);
					}
				}
				/* Zero out the fmInputDelta so it is ready to be used on the next update */
				slotPtr->fmInputDelta = 0;
			}
			if ((channelMask & outputChannelMask) == 0)
			{
				channelOutput = 0;
			}

			if (debugFlags & LJ_YM2612_DEBUG)
			{
				if (channelOutput != 0)
				{
					printf("Channel:%d channelOutput:%d\n", channelPtr->id, channelOutput);
				}
			}

			/* Keep within +/- 1 */
			channelOutput = LJ_YM2612_CLAMP_VOLUME(channelOutput);

			mixedLeft += channelOutput & channelPtr->left;
			mixedRight += channelOutput & channelPtr->right;

			channelPtr->omegaDirty = 0;

			channelMask = (channelMask << 1);
		}

		mixedLeft += dacValueLeft;
		mixedRight += dacValueRight;
		if (debugFlags & LJ_YM2612_DEBUG)
		{
			printf("mixedLeft 0x%X mixedRight 0x%X\n", mixedLeft, mixedRight);
		}

		/* Keep within +/- 1 */
		/* mixedLeft = LJ_YM2612_CLAMP_VOLUME(mixedLeft); */
		/* mixedRight = LJ_YM2612_CLAMP_VOLUME(mixedRight); */

#if LJ_YM2612_OUTPUT_SCALE >= 0
		mixedLeft = mixedLeft >> LJ_YM2612_OUTPUT_SCALE;
		mixedRight = mixedRight >> LJ_YM2612_OUTPUT_SCALE;
#else /* #if LJ_YM2612_OUTPUT_SCALE >= 0 */
		mixedLeft = mixedLeft << -LJ_YM2612_OUTPUT_SCALE;
		mixedRight = mixedRight << -LJ_YM2612_OUTPUT_SCALE;
#endif /* #if LJ_YM2612_OUTPUT_SCALE >= 0 */

		outL = (LJ_YM_INT16)mixedLeft;
		outR = (LJ_YM_INT16)mixedRight;

		outputLeft[sample] = outL;
		outputRight[sample] = outR;

		/* Update volumes and frequency position */
		for (channel = 0; channel < LJ_YM2612_NUM_CHANNELS_TOTAL; channel++)
		{
			LJ_YM2612_CHANNEL* const channelPtr = ym2612Ptr->channels[channel];

			for (slot = 0; slot < LJ_YM2612_NUM_SLOTS_PER_CHANNEL; slot++)
			{
				LJ_YM2612_SLOT* const slotPtr = &(channelPtr->slot[slot]);

				slotPtr->omega += slotPtr->omegaDelta;
			}
		}

		/* Timer A update */
		if (ym2612Ptr->aTimer.count > 0)
		{
			ym2612Ptr->aTimer.count -= ym2612Ptr->aTimer.addPerOutputSample;
			if (ym2612Ptr->aTimer.count < 0)
			{
				/* set timer A status flag : if timer A enable bit is set (0x4) */
				if (ym2612Ptr->timerMode & 0x04)
				{
					ym2612Ptr->statusA = 0x1;
				}
				/* See forum posts by Nemesis - timer A is at FM sample output (144 clock cycles) - docs are wrong */
				ym2612Ptr->aTimer.count += ym2612Ptr->timerAstart;
				while (ym2612Ptr->aTimer.count < 0)
				{
					ym2612Ptr->aTimer.count += ym2612Ptr->timerAstart;
				}

				/* CSM mode key-on : only active in 10 not in 11 */
      	if ((ym2612Ptr->ch2Mode & 0x3) == 0x2)
				{
					/* Key ON via CSM */
					LJ_YM2612_CHANNEL* const ch2Ptr = ym2612Ptr->channels[2];
					if (debugFlags & LJ_YM2612_DEBUG)
					{
						printf("%d CSM KeyON TimerACnt: %d Timer A:%d\n", ym2612Ptr->sampleCount, ym2612Ptr->aTimer.count, ym2612Ptr->timerAstart);
					}
					for (slot = 0; slot < LJ_YM2612_NUM_SLOTS_PER_CHANNEL; slot++)
					{
						LJ_YM2612_SLOT* const slotPtr = &(ch2Ptr->slot[slot]);
						ym2612_slotKeyON(slotPtr, ch2Ptr, ym2612Ptr, debugFlags);
					}
					ym2612Ptr->csmKeyOn = 1;
					csmKeyOff = 0;
				}
			}
		}

		/* If CSM key on was on before this timer A update then do CSM key off unless timer did overflow */
		if (csmKeyOff == 1)
		{
			/* Key OFF via CSM */
			LJ_YM2612_CHANNEL* const ch2Ptr = ym2612Ptr->channels[2];
			if (debugFlags & LJ_YM2612_DEBUG)
			{
				printf("%d CSM KeyOFF Timer A:%d\n", ym2612Ptr->sampleCount,ym2612Ptr->timerAstart);
			}
			for (slot = 0; slot < LJ_YM2612_NUM_SLOTS_PER_CHANNEL; slot++)
			{
				LJ_YM2612_SLOT* const slotPtr = &(ch2Ptr->slot[slot]);
				ym2612_slotCSMKeyOFF(slotPtr, debugFlags);
			}
      ym2612Ptr->csmKeyOn = 0;
		}

		/* Timer B update */
		if (ym2612Ptr->bTimer.count > 0)
		{
			ym2612Ptr->bTimer.count -= ym2612Ptr->bTimer.addPerOutputSample;
			if (ym2612Ptr->bTimer.count < 0)
			{
				/* set timer B status flag : if timer B enable bit is set (0x8) */
				if (ym2612Ptr->timerMode & 0x08)
				{
					ym2612Ptr->statusB = 0x1;
				}
				/* timer B is *16 compared to timer A which at FM sample output (144 clock cycles) */
				ym2612Ptr->bTimer.count += ym2612Ptr->timerBstart;
				while (ym2612Ptr->bTimer.count < 0)
				{
					ym2612Ptr->bTimer.count += ym2612Ptr->timerBstart;
				}
			}
		}

		ym2612Ptr->sampleCount++;
	}

	return LJ_YM2612_OK;
}

/* To set a value on the data pins D0-D7 - use for register address and register data value */
/* call setAddressPinsCSRDWRA1A0 to copy the data in D0-D7 to either the register address or register data setting */
LJ_YM2612_RESULT LJ_YM2612_setDataPinsD07(LJ_YM2612* const ym2612Ptr, LJ_YM_UINT8 data)
{
	if (ym2612Ptr == NULL)
	{
		fprintf(stderr, "LJ_YM2612_setDataPinsD07:ym2612 is NULL\n");
		return LJ_YM2612_ERROR;
	}
	ym2612Ptr->D07 = data;
	return LJ_YM2612_OK;
}

/* To read a value on the data pins D0-D7 - use to get back the status register */
/* call setAddressPinsCSRDWRA1A0 to put the status register output onto the pins */
LJ_YM2612_RESULT LJ_YM2612_getDataPinsD07(LJ_YM2612* const ym2612Ptr, LJ_YM_UINT8* const data)
{
	LJ_YM_UINT8 statusValue;
	if (ym2612Ptr == NULL)
	{
		fprintf(stderr, "LJ_YM2612_getDataPinsD07:ym2612 is NULL\n");
		return LJ_YM2612_ERROR;
	}

	if (data == NULL)
	{
		fprintf(stderr, "LJ_YM2612_getDataPinsD07:data is NULL\n");
		return LJ_YM2612_ERROR;
	}

	if (ym2612Ptr->statusOutput == 0)
	{
		fprintf(stderr, "LJ_YM2612_getDataPinsD07:ym2612 address pins not set to read mode\n");
		return LJ_YM2612_ERROR;
	}
	/* A1 = 0, A0 = 0 : D0-D7 will then contain status 0 value - timer A */
	/* A1 = 1, A0 = 0 : D0-D7 will then contain status 1 value - timer B */
	/* But Nemesis forum posts say any combination of A1/A0 work and just return the same status value */
	statusValue = 0;
	statusValue = (LJ_YM_UINT8)(statusValue | ((ym2612Ptr->statusA & 0x1) << 0));
	statusValue = (LJ_YM_UINT8)(statusValue | ((ym2612Ptr->statusB & 0x1) << 1));
	*data = statusValue;

	return LJ_YM2612_OK;
}

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
																						 				LJ_YM_UINT8 A1, LJ_YM_UINT8 A0)
{
	if (ym2612Ptr == NULL)
	{
		fprintf(stderr, "LJ_YM2612_setAddressPinsCSRDWRA1A0:ym2612 is NULL\n");
		return LJ_YM2612_ERROR;
	}
	if (notCS == 1)
	{
		/* !CS != 0 is inactive mode on chip e.g ignored */
		return LJ_YM2612_OK;
	}
	if ((notRD == 0) && (notWR == 1))
	{
		/* !RD = 0 !WR = 1 means a read */
		ym2612Ptr->statusOutput = 1;
		return LJ_YM2612_OK;
	}
	if ((notRD == 1) && (notWR == 0))
	{
		/* !RD = 1 !WR = 0 means a write */
		ym2612Ptr->statusOutput = 0;

		/* A0 = 0 means D0-7 is register address, A1 is part 0/1 */
		if ((A0 == 0) && ((A1 == 0) || (A1 == 1)))
		{
			ym2612Ptr->regAddress = ym2612Ptr->D07;
			ym2612Ptr->slotWriteAddr = A1;
			return LJ_YM2612_OK;
		}
		/* A0 = 1 means D0-7 is register data, A1 is part 0/1 */
		if ((A0 == 1) && ((A1 == 0) || (A1 == 1)))
		{
			if (ym2612Ptr->slotWriteAddr == A1)
			{
				const LJ_YM_UINT8 reg = ym2612Ptr->regAddress;
				const LJ_YM_UINT8 part = A1;
				const LJ_YM_UINT8 data = ym2612Ptr->D07;
				LJ_YM2612_RESULT result = ym2612_setRegister(ym2612Ptr, part, reg, data);
				/* Hmmmm - how does the real chip work */
				ym2612Ptr->slotWriteAddr = 0xFF;
				return result;
			}
		}
	}

	fprintf(stderr, "LJ_YM2612_setAddressPinsCSRDWRA1A0:unknown combination of pins !CS:%d !RD:%d !WR:%d A1:%d A0:%d\n",
					notCS, notRD, notWR, A1, A0);

	return LJ_YM2612_ERROR;
}

/*
Programmable Sound Generator (PSG)
(This information from a Genesis manual, but it's a standard
chip, so the info is probably public knowledge)

The PSG contains four sound channels, consisting of three tone
generators and a noise generator. Each of the four channels has
an independent volume control (attenuator). The PSG is
controlled through output port $7F.

TONE GENERATOR FREQUENCY

The frequency (pitch) of a tone generator is set by a 10-bit
value. This value is counted down until it reaches zero, at
which time the tone output toggles and the 10-bit value is
reloaded into the counter. Thus, higher 10-bit numbers produce
lower frequencies.

To load a new frequency value into one of the tone generators,
you write a pair of bytes to I/O location $7F according to the
following format:


First byte:     1   R2  R1  R0  d3  d2  d1  d0
Second byte:    0   0   d9  d8  d7  d6  d5  d4


The R2:R1:R0 field selects the tone channel as follows:


R2 R1 R0    Tone Chan.
----------------------
0  0  0     #1
0  1  0     #2
1  0  0     #3


10-bit data is (msb)    d9 d8 d7 d6 d5 d4 d3 d2 d1 d0   (lsb)


NOISE GENERATOR CONTROL

The noise generator uses three control bits to select the
"character" of the noise sound. A bit called "FB" (Feedback)
produces periodic noise of "white" noise:


FB  Noise type

0   Periodic (like low-frequency tone)
1   White (hiss)


The frequency of the noise is selected by two bits NF1:NF0
according to the following table:


NF1 NF0 Noise Generator Clock Source

0   0   Clock/2 [Higher pitch, "less coarse"]
0   1   Clock/4
1   0   Clock/8 [Lower pitch, "more coarse"]
1   1   Tone Generator #3


NOTE:   "Clock" is fixed in frequency. It is a crystal controlled
oscillator signal connected to the PSG.

When NF1:NF0 is 11, Tone Generator #3 supplies the noise clock
source. This allows the noise to be "swept" in frequency. This
effect might be used for a jet engine runup, for example.

To load these noise generator control bits, write the following
byte to I/O port $7F:

Out($7F): 1  1  1  0  0  FB  NF1  NF0


ATTENUATORS

Four attenuators adjust the volume of the three tone generators
and the noise channel. Four bits A3:A2:A1:A0 control the
attenuation as follows:


A3 A2 A1 A0     Attenuation
0  0  0  0       0 db   (maximum volume)
0  0  0  1       2 db   NOTE: a higher attenuation results
.  .  .  .       . .          in a quieter sound.
1  1  1  0      28 db
1  1  1  1      -Off-

The attenuators are set for the four channels by writing the
following bytes to I/O location $7F:


Tone generator #1:    1  0  0  1  A3  A2  A1  A0
Tone generator #2:    1  0  1  1  A3  A2  A1  A0
Tone generator #3:    1  1  0  1  A3  A2  A1  A0
Noise generator:      1  1  1  1  A3  A2  A1  A0


EXAMPLE

When the MK3 is powered on, the following code is executed:

        LD      HL,CLRTB        ;clear table
        LD      C,PSG_PRT       ;PSG port is $7F
        LD      B,4             ;load 4 bytes
        OTIR
        (etc.)

CLRTB   defb    $9F, $BF, $DF, $FF

This code turns the four sound channels off. It's a good idea to
also execute this code when the PAUSE button is pressed, so that
the sound does not stay on continuously for the pause interval.
*/
