/* --- ROBOT.H
   --- This header file contains defined constants for use in both
   --- ROBOT.C and ROBOTUSR.C when using the interface boards from
   --- Trident Robotics and Research, Inc.
   ---
   --- Initial coding by Richard Voyles, Vigilant Technologies
   --- File maintenance by Richard Voyles, Trident Robotics, Inc.
   ---
   --- (C) 1995,1999,2000 Trident Robotics and Research, Inc.
   --- */

typedef enum {  TORQUE,         /* W torque to a single joint motor */
                TORQUE6,        /* W torques to all joint motors */
                VOLTAGE,        /* W voltage to a single joint motor */
                POSITION,       /* R/W position of a single joint */
                POSITION6,      /* R/W position of all joints */
                STATUS,         /* R/W discrete status/control bits */
                POT,            /* R ADC value */
                POT6,           /* R all ADC values */
                INDEX,          /* R one index register */
                INDEX6,         /* R all index registers */
                POWER,          /* R power status, W ARM POWER ENABLE bit */
                CALIB,          /* R/W the calibration status bit */
                HAND,           /* W the hand solenoid control bits */
                INDEX_BIT,      /* R/W an index mask bit */
                ALL_INDEX_BITS, /* R/W all index mask bits */
                CLR_INDEX_BIT,  /* clear an index mask bit */
                CLR_INDEX_BITS, /* clear all index mask bits */
                CLR_ADC_BIT     /* clear A/D done mask bit */
             } ioOptionsT;

/* --- Structure that holds the PUMA information */
typedef struct{ short   status;
                short   discrete;
                long    pos[6];
                unsigned int   base;    //changed short->unsigned int for support of larger memory
              } PUMAstatusT;

/* --- Address offsets for functional groups of utilities. */
#define ENC_INDEX_BASE          0x0000
#define STATUS_REG              0x000C
#define ENC_COUNT_BASE          0x0010
#define ADC_VALUE               0x001C
#define ADC_START               0x001E
#define ENC_LOAD_BASE           0x0020
#define ADC_MUX_SELECT          0x002C
#define DISCRETE_REG            0x002E
#define DAC_BASE                0x0030

/* --- Masks for the status register. */
#define CALIB_MASK              0x0040
#define ADC_STAT_MASK           0x4000
#define POWER_ON_MASK           0x8000

/* --- Masks for the discrete register. */
#define POWER_BIT               0x0001
#define HAND_OPEN_BIT           0x0002
#define HAND_CLOSED_BIT         0x0004
#define CALIB_BIT               0x0040
#define JT6_OVRFLO_BIT          0x0080
#define ADC_MASK_BIT            0x4000
#define JT1_INDEX_BIT           0x0100
#define ALL_INDEX_BITS          0x3F00

/* --- Reciprocal of DAC gain (gain is set by resistors) */
#define DAC_BITS_PER_VOLT       -204.8
#define DAC_MAX_CNT             0x0FFF
#define DAC_MIN_CNT             0x0000

/* --- Command value OR'ed onto start command for AD7891. */
#define ADC_CMD_VAL             0x0050

#ifndef ROBOT_SRC

/* --- Global variables defined in ROBOT.C (all globals end in "G") */
extern PUMAstatusT pumaVarsG;   /* structure containing PUMA variables */
extern float    motorScaleG[6]; /* motor scale factors; N-m ==> bits */
extern float    encoderScaleG[6]; /* encoder scale factors; bits ==> rad */
extern int      encoderOffsetG[6]; /* encoder offsets */
extern float    KPosG[6],       /* position error gain for PD cntrlr */
                KVelG[6];       /* velocity error gain for PD cntrlr */
extern float    Q_RefG[6],      /* joint reference positions */
                Qd_RefG[6];     /* joint reference velocity */
extern float    Q_MezG[6],      /* measured joint positions */
                Qd_MezG[6];     /* measured joint velocities */

/* --- This function sets up the robot structure */
extern float system_init(float freq);

/* --- These functions access the TRC006 */
extern short robot_fltwr(short option, short chan, float *val);
extern short robot_shortwr(short option, short chan, short *val);
extern short robot_fltrd(short option, short chan, float *val);
extern short robot_longrd(short option, short chan, long *val);
extern short robot_control(short option, short chan, short val);
extern short robot_status(short option, short chan, short *val);
extern short robot_potrd(short option, short chan, long *val);

/* --- Simple PD controller with gravity compensation on joints 2 and 3. */
extern int pdgControl();

#endif
