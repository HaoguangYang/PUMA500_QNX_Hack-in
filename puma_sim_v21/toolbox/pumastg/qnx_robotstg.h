/* --- ROBOTSTG.H
   --- This header file contains defined constants for use in both
   --- ROBOTSTG.C and ROBOTUSR.C when using the interface
   --- boards from Mark V Automation Corp. (Trident Robotics and Research, Inc.)
   ---
   --- Initial coding by Richard Voyles, Trident Robotics, Inc.
   ---
   --- (C) 1995,1997,1999,2000,2003 Mark V Automation Corp.
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
                INDEX_BIT,      /* S read an index mask bit */
                ALL_INDEX_BITS, /* R/W all index mask bits */
                CLR_INDEX_BIT,  /* C clear an index mask bit */
                CLR_INDEX_BITS, /* C clear all index mask bits */
                CLR_ADC_BIT,    /* C clear A/D done mask bit */
                INDEX_LVL_HI,   /* C set index pulse level hi */
                INDEX_LVL_LO,    /* C set index pulse level lo */
                AUX_STATUS
             } ioOptionsT;

/* --- Structure that holds the PUMA information */
typedef struct{ short   status;
                short   discrete;
                long    pos[6];
                unsigned int  base; //changed short->unsigned int for support of larger memory
              } PUMAstatusT;

/* --- Address offsets for functional groups of utilities. */
#ifdef STG_MODEL2

  #define ADC_MUX_SELECT        0x0401
  #define ADC_MUX_CMD           0x08
  #define BRDTEST               0x0403
  #define STATUS_REG            0x0405  /* Port D */
  #define IDLEN_REG             0x0409  /* Index Latch Enable */
  #define SELDI_REG             0x040B  /* Select Index */
  #define IDL_REG               0x040D  /* Index Latch value */
  #define TMRCMD                0x040E
  #define TIMER0                0x0408
  #define TIMER1                0x040A
  #define TIMER2                0x040C
  #define CNTRL1                0x040F
#else

  #define ADC_MUX_SELECT        0x0410
  #define STATUS_REG            0x0401  /* Port D */
#endif

#define AUXSTAT_REG             0x0402  /* Port B */
#define ENC_INDEX_BASE          0x0000
#define STATUS_CTRL             0x0407  /* byte register */
#define INTC_BIT_SR             0x0407  /* byte register */
#define ENC_COUNT_BASE          0x0000
#define ENC_CTRL_BASE           0x0002
#define ADC_VALUE               0x0410
#define ADC_START               0x0410
#define ENC_LOAD_BASE           0x0000
#define DISCRETE_REG            0x0404  /* Port C */
#define DISCRETE_CTRL           0x0406  /* byte register */
#define DAC_BASE                0x0010
/* Beklow this point are Model 1 offsets only */
#define INTR_ICW1               0x0409  /* byte register */
#define INTR_ICW2               0x040B  /* byte register */
#define INTR_OCW1               0x040B  /* byte register */
#define INTR_OCW3               0x0409  /* byte register - write */
#define INTR_IRR                0x0409  /* byte register - read */
#define INDEX_EVN_CLR           0x0403  /* byte register */
#define INDEX_ODD_CLR           0x0407  /* byte register */


/* --- Masks for the encoder control registers */
#define ENC_MCR_READ            0x0303
#define ENC_MCR_LOAD            0x0808
#define ENC_MCR_INIT            0x2323
#define ENC_MCR_RESET           0x0505
#define ENC_ICR_INIT            0x6868
#define ENC_OCR_INIT            0x8080
#define ENC_QCR_INIT            0xC3C3
#define IND_MCR_READ            0x0101

#ifdef STG_MODEL2
  #define CNTRL1_INIT           0x00
  #define TMR0_LSB              0x1A
  #define TMR0_MSB              0x2A
  #define TMR1_LSB              0x5A
  #define TMR1_MSB              0x6A
  #define TMR2_INIT             0x8A
#else
  /* --- Values for interrupt control - Model 1 only */
  #define ICW1_INIT               0x1A
  #define ICW2_INIT               0x00
  #define OCW1_MASK_ALL           0xFF
  #define OCW3_IRR_SEL            0x0A
#endif

/* --- Masks for the status register. */
/* --- 16-bit values are not used with Servo To Go board */
#define CALIB_MASK              0x02    /* Port B */
#define BR_SW_MASK              0x04    /* Brake Release Switch - Port B */
#define ADC_STAT_MASK           0x08    /* actually IRR on Model 1, BRDTEST on Model 1, not status reg */
#define WATCHDOG_MASK           0x80
#define POWER_ON_MASK           0x40    /* Port D */
#ifdef STG_MODEL2
  #define STATUS_INIT           0x8B    /* control byte */
#else
  #define STATUS_INIT           0x92    /* control byte */
#endif

/* --- Masks for the discrete register. */
#define POWER_BIT               0x80
#define HAND_OPEN_BIT           0x40
#define HAND_CLOSED_BIT         0x20
#define BRAKE_RELEASE           0x10
#define CALIB_BIT               0x02

/* --- 16-bit values are not used with Servo To Go board */
#ifdef STG_MODEL2
  #define JT0_INDEX_BIT         0x01
  #define JT1_INDEX_BIT         0x02
#else
  #define JT0_INDEX_BIT         0x20
  #define JT1_INDEX_BIT         0x10
#endif
#define CALIB_BIT               0x02
#define JT6_OVRFLO_BIT          0x01
#define ADC_MASK_BIT            0x4000
#define ALL_INDEX_BITS          0x3F00
#define DISCRETE_INIT           0x92    /* control byte */
#define CTRL_POWER_OFF          0x0F
#define CTRL_POWER_ON           0x0E
#define CTRL_EN_BRK_REL         0x09
#define CTRL_DIS_BRK_REL        0x08
#define CTRL_SET_CALIB          0x02	/* sets bit low, meaning calibrated */
#define CTRL_CLR_CALIB          0x03	/* sets bit hi, meaning uncalibrated */
#define CTRL_SET_CINIT          0x01	/* sets bit hi, meaning initialized */
#define CTRL_HANDOPEN_OFF       0x0D
#define CTRL_HANDOPEN_ON        0x0C
#define CTRL_HANDCLOSE_OFF      0x0B
#define CTRL_HANDCLOSE_ON       0x0A

/* --- INT.C bit set/reset control bytes */
/* --- These values are written to INT.C_BIT_SR to flip INT.C bits */
#define INTC_IXS0_0             0x08
#define INTC_IXS0_1             0x09
#define INTC_IXS1_0             0x0A
#define INTC_IXS1_1             0x0B
#define INTC_IXLVL_0            0x0C
#define INTC_IXLVL_1            0x0D

/* --- Reciprocal of DAC gain (gain is set by resistors) */
#define DAC_BITS_PER_VOLT       -409.6
#define DAC_MAX_CNT             0x1FFF
#define DAC_MIN_CNT             0x0000

#ifndef ROBOT_SRC

/* --- Global variables defined in ROBOTSTG.C (all globals end in "G") */
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
/* extern int pdg_init(processT *p_ptr, void *vptr); */

#endif
