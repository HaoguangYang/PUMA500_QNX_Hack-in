/* --- ROBOTSTG.C
   --- This file contains the main routines for running the PUMA
   --- robot with the Mark V Automation Corp. amplifiers
   --- and Servo To Go, Inc. interface board.
   ---
   --- These are only demo routines and are not complete.
   --- For serious operation, your own interrupt routines must
   --- handle the system clock properly and properly save and
   --- restore state. You are free to use these routines, or any
   --- part thereof, AT YOUR OWN RISK, provided this entire comment
   --- block and the included copyright notice remain intact.
   ---
   --- Initial coding by Richard Voyles, Vigilant Technologies.
   --- Further modifications by Richard Voyles, Trident Robotics,
   ---					 and Haoguang Yang, Tsinghua University.
   ---
   --- Copyright 1997-2003 Mark V Automation Corp. (formerly Trident Robotics and Research, Inc.)
   ---
   --- 12/17/97 RMV     Copied from ROBOT.C v 2.0 for the TRC004.
   --- 02/18/99 RMV     Added index pulse level setting to robot_control()
   ---                  Inverted output of robot_status(INDEX_BIT)
   ---                  Added explicit level set to system_init()
   ---                  Fixed neg bug in robot_shortwr(POSITIONx)
   --- 11/10/99 RMV     Added WORD_STATUS, BYTE_STATUS, AUX_STATUS commands
   --- 11/14/99 RMV     Completed conversion to Model 2 hardware.
   ---                  Turned on VAL offsets and wrist coupling. v4.0
   --- 12/01/99 RMV     Better management of pumaVarsG.discrete.
   --- 03/27/00 RMV     Fixed sign error in coupling compensation. v4.2
   --- 07/23/03 RMV     Updated to sched2. Added cal init. Changed gains. v6.0
   --- 10/24/17 YHG     Updated parameter formats for compatibility with QNX 6.x API on the following:
   --- 					include stdint.h and sys/mman.h
   --- 					uintptr_t mmap_device_io(length, address);
   --- 					in8(uintptr_t); out(uintptr_t, value).
   --- */

#define ROBOT_SRC

#define STG_MODEL2

#include <stdio.h>
#include <math.h>
#include "vtypes.h"
#include "qnx_robotstg.h"
// for QNX hardware IO
#include <hw/inout.h>
#include <sys/neutrino.h>
#include <sys/iofunc.h>
//Include specific integer and pointer headers
#include <stdint.h>
#include <sys/mman.h>

/* --- Global variables (all globals end in "G") */
PUMAstatusT     pumaVarsG;      /* structure containing PUMA variables */
float           KPosG[6],       /* position error gain for PD cntrlr */
                KVelG[6];       /* velocity error gain for PD cntrlr */
float           Q_RefG[6],      /* joint reference positions */
                Qd_RefG[6];     /* joint reference velocity */
float           Q_MezG[6],      /* measured joint positions */
                Qd_MezG[6];     /* measured joint velocities */

//RFKM
// global vars for memory
uintptr_t SERVOTOGO_LO;
uintptr_t SERVOTOGO_HI;

/* --- The motor scale converts N-m at the joint to DAC quanta (13-bit resolution) */
/* For PUMA 550 Mark I */
/* float   motorScaleG[6] = {-39.98, 21.94, -45.30, -169.60, 0, 190.00}; */

/* For PUMA 560 Mark II */
//float   motorScaleG[6] = {-39.98, 21.94, -45.30, 169.60, 195.36, 190.00};

/* For PUMA 560 Mark II  w/ Servo-To-Go */
float   motorScaleG[6] = {39.98, -21.94, 45.30, -169.60, -195.36, -190.00};

/* --- The encoder scale converts encoder counts to radians. */
/* This set of values is for the TRC004 Rev D and Servo To Go Boards. */
float   encoderScaleG[6] = {0.00010035, -0.000073156, 0.000117,
                            -0.000082663, -0.000087376, -0.000081885};

short   encoderOffsetG[6] = {0,0,0,8000,0,0};

/* --- Wrist joint coupling factors 4->5, 4->6, 5->6 (radians) */
/* --- For PUMA 560 */
float   wristCouplingG[3] = { 0.0139037, -0.0130401, 0.180556 };

/* --- Joint offsets to agree with VAL and robotics texts (radians) */
/* --- For PUMA 560 */
float   jointOffsetG[6] = {0.0, -1.5708, 1.5708, 0.0, 0.0, 0.0};

/* --- Joint limits (radians) */
/* --- For PUMA 760 */
float   jointMaxG[6] = {2.0, 0.2, 3.2, 3.5, 1.7, 4.0};
float   jointMinG[6] = {-2.0, -3.2, -0.2, -3.5, -1.7, -4.0};

/* --- Joint error limits (radians) */
/* --- For PUMA 760 */
float   jointErrMaxG[6] = {0.5, 0.5, 0.5, 0.5, 0.5, 0.5};
float   jointErrMinG[6] = {-0.5, -0.5, -0.5, -0.5, -0.5, -0.5};


/* ************ Function Prototypes ************** */

short robot_fltwr(short option, short chan, float *val);
short robot_shortwr(short option, short chan, short *val);
short robot_fltrd(short option, short chan, float *val);
short robot_longrd(short option, short chan, long *val);


/* --- This function initializes the system for robot control. It installs
   --- the interrupt handler and sets the desired interrupt frequency, in
   --- the proper order. It also initializes variables.
   --- It returns the acutal interrupt frequency, in hertz. */
float system_init(float freq)
{
  int   i, stat, disc;

        //RFKM
        // initialize memory locations
        ThreadCtl(_NTO_TCTL_IO, 0);
        SERVOTOGO_LO = mmap_device_io(0x1F, pumaVarsG.base);
        printf("SERVOTOGO_LO at 0x%04X\n", SERVOTOGO_LO);
        SERVOTOGO_HI = mmap_device_io(0x1F, pumaVarsG.base+0x400);
        printf("SERVOTOGO_HI at 0x%04X\n", SERVOTOGO_HI);


  /* --- First initialize the PUMA structure. */
  pumaVarsG.status = 0;

  //Change from QNX2.X: in() and out() pass parameter type of uintptr_t
  /* Get the calibration bit from the aux status register */
  stat = in8(mmap_device_io(0x07, pumaVarsG.base+AUXSTAT_REG)) & 0x02;
  /* And the calibration and init bits from the discrete register */
  disc = in8(mmap_device_io(0x07, pumaVarsG.base+DISCRETE_REG)) & 0x03;

  uintptr_t discrete_ctrl_port = mmap_device_io(0x07, pumaVarsG.base + DISCRETE_CTRL);
  if ((stat == 0) && (disc == 0x01)){
    printf("Calibrated!\n");
    /* initialize the STG DIO */
    out8(mmap_device_io(0x07, pumaVarsG.base+STATUS_CTRL), STATUS_INIT);       /* for input */
    
	out8(discrete_ctrl_port, DISCRETE_INIT);   /* for output */
    out8(discrete_ctrl_port, CTRL_POWER_OFF);
    out8(discrete_ctrl_port, CTRL_DIS_BRK_REL);
    out8(discrete_ctrl_port, CTRL_SET_CALIB);
    out8(discrete_ctrl_port, CTRL_SET_CINIT);
    robot_longrd(POSITION6,0,&pumaVarsG.pos[0]);

  } else {	/* Not calibrated */

    /* --- Arm is NOT calibrated so set everything to zero. */
    for (i=0; i<6; i++){
      pumaVarsG.pos[i] = 0L;            /* reset the variable */
      robot_shortwr(POSITION, i, 0);    /* reset the encoder */
    } /* endfor */
    /* initialize the encoders on the STG board */
    for (i=0; i<6; i+=2){
	  uintptr_t enc_ctrl_port = mmap_device_io(0x0F, pumaVarsG.base + ENC_CTRL_BASE + 2*i);
      out16(enc_ctrl_port, ENC_MCR_INIT);
      out16(enc_ctrl_port, ENC_ICR_INIT);
      out16(enc_ctrl_port, ENC_OCR_INIT);
      out16(enc_ctrl_port, ENC_QCR_INIT);
    } /* endfor */
    /* initialize the STG DIO */
    out8(mmap_device_io(0x07, pumaVarsG.base+STATUS_CTRL), STATUS_INIT);	/* for input */
    out8(discrete_ctrl_port, DISCRETE_INIT);   /* for output */
    out8(discrete_ctrl_port, CTRL_POWER_OFF);
    out8(discrete_ctrl_port, CTRL_DIS_BRK_REL);
    out8(discrete_ctrl_port, CTRL_CLR_CALIB);
    out8(discrete_ctrl_port, CTRL_SET_CINIT);
  } /* endif */

#ifdef STG_MODEL2
  uintptr_t timer_cmd_port = mmap_device_io(0x07, pumaVarsG.base + TMRCMD);
  out8(mmap_device_io(0x07, pumaVarsG.base+CNTRL1), CNTRL1_INIT);
  uintptr_t timer_addr_port = mmap_device_io(0x07, pumaVarsG.base + TIMER0);
  //Timer 0
  out8(timer_cmd_port, TMR0_LSB);
  out8(timer_addr_port, 0xFF);
  out8(timer_cmd_port, TMR0_MSB);
  out8(timer_addr_port, 0xFF);
  //Timer 1
  timer_addr_port = mmap_device_io(0x07, pumaVarsG.base + TIMER1);
  out8(timer_cmd_port, TMR1_LSB);
  out8(timer_addr_port, 0xFF);
  out8(timer_cmd_port, TMR1_MSB);
  out8(timer_addr_port, 0xFF);
  //Timer 2
  timer_addr_port = mmap_device_io(0x07, pumaVarsG.base + TIMER2);
  out8(timer_cmd_port, TMR2_INIT);
  out8(timer_addr_port, 0);
#else
  out8(mmap_device_io(0x07,pumaVarsG.base+INTR_ICW1), ICW1_INIT);
  out8(mmap_device_io(0x07,pumaVarsG.base+INTR_ICW2), ICW2_INIT);
  out8(mmap_device_io(0x07,pumaVarsG.base+INTR_OCW1), OCW1_MASK_ALL);
#endif

#ifdef STG_MODEL2
  /* Enable all index latches */
  out8(mmap_device_io(0x07,pumaVarsG.base + IDLEN_REG), 0xFF);
  /* Select the index pulse as the latch signal for all */
  out8(mmap_device_io(0x07,pumaVarsG.base + SELDI_REG), 0x00);
  /* Clear all index latches */
  out8(mmap_device_io(0x07,pumaVarsG.base + IDL_REG), 0x00);
#else
  /* Set the index pulse level */
  out8(mmap_device_io(0x07,pumaVarsG.base + INTC_BIT_SR), INTC_IXLVL_1);
#endif

  pumaVarsG.discrete = in8(mmap_device_io(0x07,pumaVarsG.base + DISCRETE_REG));

   /* return sched_init(freq); */
   return (0);
}

/* --- This function writes 1 or 6 float values to the robot. The particular
   --- value is chosen with the OPTION parameter, which is one out of a
   --- list of enumerated ioOptionsT. CHAN ranges from 0 to 5 (corresponding
   --- to PUMA joints 1 - 6). It returns one of the enumerated ioErrorsT. */
short robot_fltwr(short option, short chan, float *val)
{
  short intVal, shortVal,i;
  long  longVal;

  switch (option) {
    case TORQUE6:       /* Output to all DACs -- val is in N-m - STG */
      for (i=0; i<6; i++){
        longVal = (long)(val[i] * motorScaleG[i]) + 0x1000;
        if (longVal > DAC_MAX_CNT)
          shortVal = DAC_MAX_CNT;
        else if (longVal < DAC_MIN_CNT)
          shortVal = DAC_MIN_CNT;
        else
          shortVal = (short)longVal;
        out16(mmap_device_io(0x0F,pumaVarsG.base + DAC_BASE + 2*i), shortVal);
      } /* endfor */
      return I_OK;
    case TORQUE:        /* Output to single DAC -- val is in N-m - STG */
      longVal = (long)(*val * motorScaleG[chan]) + 0x1000;
      if (longVal > DAC_MAX_CNT)
        shortVal = DAC_MAX_CNT;
      else if (longVal < DAC_MIN_CNT)
        shortVal = DAC_MIN_CNT;
      else
        shortVal = (short)longVal;
      out16(mmap_device_io(0x0F,pumaVarsG.base + DAC_BASE + 2*chan), shortVal);
      return I_OK;
    case VOLTAGE:       /* Output to single DAC -- val is in volts - STG */
      if (*val >= 10.0){
        intVal = 0x1FFF;
      } else if (*val <= -9.995){
        intVal = 0;
      } else {
        intVal = (int)(*val * DAC_BITS_PER_VOLT) + 0x1000;
      } /* endif */
      out16(mmap_device_io(0x0F,pumaVarsG.base + DAC_BASE + 2*chan), intVal);
      return I_OK;
    case POSITION6:     /* Preset all encoders -- val is in radians */
    case POSITION:      /* Preset a single encoder -- val is in radians */
    default:
      printf("ERROR: robot_fltwr() option failed\n");
      return I_BAD_OPTION;
  } /* endswitch */
}

/* --- This function writes 1 or 6 integer values to the robot. The particular
   --- value is chosen with the OPTION parameter, which is one out of a
   --- list of enumerated ioOptionsT. CHAN ranges from 0 to 5 (corresponding
   --- to PUMA joints 1 - 6). It returns one of the enumerated ioErrorsT. */
short robot_shortwr(short option, short chan, short *val)
{
  short i,j,tempEnc;

  switch (option) {
    case TORQUE6:       /* Output to all DACs - STG */
      for (i=0; i<6; i++){
        out16(mmap_device_io(0x0F,pumaVarsG.base + DAC_BASE + 2*i), val[i]);
      } /* endfor */
      return I_OK;
    case TORQUE:        /* Output to single DAC - STG */
      out16(mmap_device_io(0x0F,pumaVarsG.base + DAC_BASE + 2*chan), *val);
      return I_OK;
    case POSITION6:     /* Preset all encoders - STG */
      for (i=0; i<6; i+=2){
        /* Reset the address pointer and transfer to output latch */
		uintptr_t enc_load_port = mmap_device_io(0x0F,pumaVarsG.base + ENC_LOAD_BASE + 2*i);
		uintptr_t enc_ctrl_port = mmap_device_io(0x0F,pumaVarsG.base + ENC_CTRL_BASE + 2*i);
        out16(enc_ctrl_port, ENC_MCR_READ);
        
		/* Ready the low byte of two adjacent channels */
        tempEnc = (val[i] + encoderOffsetG[i]) & 0x00FF;
        tempEnc |= ((val[i+1] + encoderOffsetG[i+1]) & 0x00FF) << 8;
        out16(enc_load_port, tempEnc);

        /* Ready the middle byte of two adjacent channels */
        tempEnc = ((val[i] + encoderOffsetG[i]) >> 8) & 0x00FF;
        tempEnc |= (val[i+1] + encoderOffsetG[i+1]) & 0x0FF00;
        out16(enc_load_port, tempEnc);

        /* Ready the high byte of two adjacent channels */
        tempEnc = (short)((((int)val[i] + (int)encoderOffsetG[i]) >> 16) & 0x00FF);
        tempEnc |= (short)((((int)val[i+1] + (int)encoderOffsetG[i+1]) >> 8) & 0x0FF00);
        out16(enc_load_port, tempEnc);

        /* Transfer the preload to the counter */
        out16(enc_ctrl_port, ENC_MCR_LOAD);
      } /* endfor */
      return I_OK;
    case POSITION:      /* Preset a single encoder - STG */
      i = chan & 0x7FFE;
      j = chan & 0x0001;
	  uintptr_t enc_load_port = mmap_device_io(0x07,pumaVarsG.base + ENC_LOAD_BASE + 2*i+j);
      /* Reset the address pointer and transfer to output latch */
      out16(mmap_device_io(0x0F,pumaVarsG.base+ENC_CTRL_BASE+2*i), ENC_MCR_READ);
      /* Ready the low byte of one channel */
      //tempEnc = (*val + encoderOffsetG[chan]) & 0x00FF;
      tempEnc = 0;
      out8(enc_load_port, (char)tempEnc);
 
      /* Ready the middle byte of one channel */
      //tempEnc = ((*val + encoderOffsetG[chan]) >> 8) & 0x00FF;
      tempEnc = 0;
      out8(enc_load_port, tempEnc);

      /* Ready the high byte of one channel */
      //tempEnc = (short)((((int)*val + (int)encoderOffsetG[chan]) >> 16) & 0x00FF);
      tempEnc = 0;
      out8(enc_load_port, tempEnc);

      /* Transfer the preload to the counter */
      out8(mmap_device_io(0x07,pumaVarsG.base+ENC_CTRL_BASE+2*i+j), (char)(ENC_MCR_LOAD));
	  //--> Double Check <-- this part.
      return I_OK;
    case STATUS:        /* Write the entire status word */
    default:
      printf("ERROR: robot_shortwr() option failed\n");
      return I_BAD_OPTION;
  } /* endswitch */
}

/* --- This function reads 1 or 6 float values from the robot. The particular
   --- value is chosen with the OPTION parameter, which is one out of a
   --- list of enumerated ioOptionsT. CHAN ranges from 0 to 5 (corresponding
   --- to PUMA joints 1 - 6). It returns one of the enumerated ioErrorsT. */
short robot_fltrd(short option, short chan, float *val)
{
  short tempPos,i,j;
  unsigned char  tempEnc[2], eoc;
  long  tempLong[2];

  switch (option) {
    case POSITION6:     /* Read all encoders -- val is in radians - STG */
      for (i=0; i<6; i+=2){
        /* Reset the address pointer and transfer to output latch */
        out16(mmap_device_io(0x0F,pumaVarsG.base + ENC_CTRL_BASE + 2*i), ENC_MCR_READ);
        /* Read the first byte of a pair of channels */
		uintptr_t enc_count_port = mmap_device_io(0x0F,pumaVarsG.base+ENC_COUNT_BASE+2*i);
        *((short *)tempEnc) = in16(enc_count_port);
        /* Shift everything up 8 bits for sign extension */
        tempLong[0] = (long)tempEnc[0] << 8;
        tempLong[1] = (long)tempEnc[1] << 8;

        /* Read the middle byte of 2 adjacent channels */
        *((short *)tempEnc) = in16(enc_count_port);
        tempLong[0] |= (long)tempEnc[0] << 16;
        tempLong[1] |= (long)tempEnc[1] << 16;

        /* Read the high byte of two adjacent channels */
        *((short *)tempEnc) = in16(enc_count_port);
        tempLong[0] |= (long)tempEnc[0] << 24;
        tempLong[1] |= (long)tempEnc[1] << 24;
        tempLong[0] /= 256;
        tempLong[1] /= 256;
        val[i] = encoderScaleG[i] * (tempLong[0] - encoderOffsetG[i]);
        val[i+1] = encoderScaleG[i+1] * (tempLong[1] - encoderOffsetG[i+1]);
      } /* endfor */

      /* --- Now adjust joints 6 and 5 for coupling factors */
#if 1
      val[5] -= val[3] * wristCouplingG[1] + val[4] * wristCouplingG[2];
      val[4] -= val[3] * wristCouplingG[0];

      /* --- Include joint offsets to agree with VAL */
      val[1] += jointOffsetG[1];
      val[2] += jointOffsetG[2];
#endif
      return I_OK;
    /* DO NOT USE THIS OPTION FOR WRIST JOINTS. THEY ARE WRONG */
    case POSITION:     /* Read 1 encoder -- val is in radians - STG */
      i = chan >> 1;
      i = i << 1;
      /* Reset the address pointer and transfer to output latch */
      out16(mmap_device_io(0x0F,pumaVarsG.base + ENC_CTRL_BASE + 2*i), ENC_MCR_READ);
      /* Read the first byte of a pair of channels */
	  uintptr_t enc_count_port = mmap_device_io(0x0F,pumaVarsG.base+ENC_COUNT_BASE+2*i);
      *((short *)tempEnc) = in16(enc_count_port);
      /* Shift everything up 8 bits for sign extension */
      tempLong[0] = (long)tempEnc[0] << 8;
      tempLong[1] = (long)tempEnc[1] << 8;

      /* Read the middle byte of 2 adjacent channels */
      *((short *)tempEnc) = in16(enc_count_port);
      tempLong[0] |= (long)tempEnc[0] << 16;
      tempLong[1] |= (long)tempEnc[1] << 16;

      /* Read the high byte of two adjacent channels */
      *((short *)tempEnc) = in16(enc_count_port);
      tempLong[0] |= (long)tempEnc[0] << 24;
      tempLong[1] |= (long)tempEnc[1] << 24;
      tempLong[0] /= 256;
      tempLong[1] /= 256;
      if (i == chan)
        *val = encoderScaleG[chan] * (tempLong[0] - encoderOffsetG[chan]) +
                   jointOffsetG[chan];
      else
        *val = encoderScaleG[chan] * (tempLong[1] - encoderOffsetG[chan]) +
                   jointOffsetG[chan];

      /* --- Now adjust joints 6 and 5 for coupling factors */
#if 0
      val[5] -= val[3] * wristCouplingG[1] + val[4] * wristCouplingG[2];
      val[4] -= val[3] * wristCouplingG[0];

      /* --- Include joint offsets to agree with VAL */
      val[1] += jointOffsetG[1];
      val[2] += jointOffsetG[2];
#endif
      return I_OK;
    case POT:           /* Read a single ADC -- val is in volts - STG */
#ifdef STG_MODEL2
      out8(mmap_device_io(0x07,pumaVarsG.base+ADC_MUX_SELECT), (chan << 4) | ADC_MUX_CMD);
      /* Delay for MUX settling */
      eoc = in8(mmap_device_io(0x07,pumaVarsG.base + BRDTEST)) & ADC_STAT_MASK;

      /* Start conversion */
      out16(mmap_device_io(0x0F,pumaVarsG.base + ADC_START), 0);

      /* --- Wait for the polling delay. */
      eoc = 0;
	  uintptr_t brdtest_port = mmap_device_io(0x07,pumaVarsG.base + BRDTEST);
      while (eoc == 0){
        eoc = in8(brdtest_port) & ADC_STAT_MASK;
      } /* endwhile */

      /* --- Wait for conversion complete. /EOC goes low */
      while (eoc != 0){
        eoc = in8(brdtest_port) & ADC_STAT_MASK;
      } /* endwhile */

      tempPos = (in16(mmap_device_io(0x0F,pumaVarsG.base + ADC_VALUE + 2*chan))) << 3;
      *val = 0.000152 * tempPos;        /* +/- 5V range JP2 out */
#else
      in16(mmap_device_io(0x0F,pumaVarsG.base+ADC_MUX_SELECT+2*chan)); /* select the MUX channel */
      out16(mmap_device_io(0x07,pumaVarsG.base+ADC_START+2*chan),0);   /* trigger the conversion */

      /* --- Wait for the polling delay. */
      eoc = 1;
	  uintptr_t intr_ocw3_port = mmap_device_io(0x07,pumaVarsG.base + INTR_OCW3);
	  uintptr_t intr_irr_port = mmap_device_io(0x07,pumaVarsG.base + INTR_IRR);
      while (eoc != 0){
        out8(intr_ocw3_port, OCW3_IRR_SEL);
        eoc = in8(intr_irr_port) & ADC_STAT_MASK;
      } /* endwhile */

      /* --- Wait for conversion complete. */
      while (eoc == 0){
        out8(intr_ocw3_port, OCW3_IRR_SEL);
        eoc = in8(intr_irr_port) & ADC_STAT_MASK;
      } /* endwhile */

      tempPos = (in16(mmap_device_io(0x0F,pumaVarsG.base+ADC_VALUE+2*chan))) << 3;
      *val = 0.000152 * tempPos;        /* +/- 5V range JP2 out */
#endif
      return I_OK;
    default:
      printf("ERROR: robot_fltrd() option failed\n");
      return I_BAD_OPTION;
  } /* endswitch */
}

/* --- This function reads 1 or 6 LONG values to the robot. The particular
   --- value is chosen with the OPTION parameter, which is one out of a
   --- list of enumerated ioOptionsT. CHAN ranges from 0 to 5 (corresponding
   --- to PUMA joints 1 - 6). It returns one of the enumerated ioErrorsT. */
short robot_longrd(short option, short chan, long *val)
{
  short i,j,tempPos;
  unsigned char  tempEnc[2], eoc;
  long  tempLong[2];
  uintptr_t enc_count_port;

  switch (option) {
    case INDEX6:        /* STG Has no separate index registers */
    case POSITION6:     /* Read all encoders - STG */
      for (i=0; i<6; i+=2){
        /* Reset the address pointer and transfer to output latch */
		out16(mmap_device_io(0x0F,pumaVarsG.base+ENC_CTRL_BASE+2*i), ENC_MCR_READ);
        enc_count_port = mmap_device_io(0x0F,pumaVarsG.base+ENC_COUNT_BASE+2*i);
		/* Read the first byte of a pair of channels */
        *((short *)tempEnc) = in16(enc_count_port);
        /* Shift everything up 8 bits for sign extension */
        tempLong[0] = (long)tempEnc[0] << 8;
        tempLong[1] = (long)tempEnc[1] << 8;

        /* Read the middle byte of 2 adjacent channels */
        *((short *)tempEnc) = in16(enc_count_port);
        tempLong[0] |= (long)tempEnc[0] << 16;
        tempLong[1] |= (long)tempEnc[1] << 16;

        /* Read the high byte of two adjacent channels */
        *((short *)tempEnc) = in16(enc_count_port);
        tempLong[0] |= (long)tempEnc[0] << 24;
        tempLong[1] |= (long)tempEnc[1] << 24;
        tempLong[0] /= 256;
        tempLong[1] /= 256;
        val[i] = tempLong[0] - encoderOffsetG[i];
        val[i+1] = tempLong[1] - encoderOffsetG[i+1];
      } /* endfor */
      return I_OK;
    case POSITION:     /* Read one encoder - STG */
      i = chan >> 1;
      i = i << 1;

      /* Reset the address pointer and transfer to output latch */
	  enc_count_port = mmap_device_io(0x0F,pumaVarsG.base+ENC_COUNT_BASE+2*i);
      out16(mmap_device_io(0x0F, pumaVarsG.base + ENC_CTRL_BASE + 2*i), ENC_MCR_READ);
      /* Read the first byte of a pair of channels */
      *((short *)tempEnc) = in16(enc_count_port);
      /* Shift everything up 8 bits for sign extension */
      tempLong[0] = (long)tempEnc[0] << 8;
      tempLong[1] = (long)tempEnc[1] << 8;

      /* Read the middle byte of 2 adjacent channels */
      *((short *)tempEnc) = in16(enc_count_port);
      tempLong[0] |= (long)tempEnc[0] << 16;
      tempLong[1] |= (long)tempEnc[1] << 16;

      /* Read the high byte of two adjacent channels */
      *((short *)tempEnc) = in16(enc_count_port);
      tempLong[0] |= (long)tempEnc[0] << 24;
      tempLong[1] |= (long)tempEnc[1] << 24;
      tempLong[0] /= 256;
      tempLong[1] /= 256;
      if (i == chan)
        *val = tempLong[0] - encoderOffsetG[chan];
      else
        *val = tempLong[1] - encoderOffsetG[chan];
      return I_OK;
    case INDEX:         /* STG has no separate index registers */
      /* Read two for speed (word is faster than byte) and discard one */
      i = chan & 0xFFFE;        /* Force i to be even: 0, 2, 4 */
      /* Index pulse should have transferred value to output latch */
      /* Only reset the address pointer */
	  enc_count_port = mmap_device_io(0x0F,pumaVarsG.base+ENC_COUNT_BASE+2*i);
      out8(mmap_device_io(0x07, pumaVarsG.base + ENC_CTRL_BASE + 2*i), IND_MCR_READ);
      /* Read the first byte of a pair of channels */
      *((short *)tempEnc) = in16(enc_count_port);
      /* Of the 2 channels read, select only the channel requested */
      if (i == chan){
        /* Shift everything up 8 bits for sign extension */
        tempLong[0] = (long)tempEnc[0] << 8;

        /* Read the middle byte of 2 adjacent channels */
        *((short *)tempEnc) = in16(enc_count_port);
        tempLong[0] |= (long)tempEnc[0] << 16;

        /* Read the high byte of two adjacent channels */
        *((short *)tempEnc) = in16(enc_count_port);
        tempLong[0] |= (long)tempEnc[0] << 24;
        tempLong[0] /= 256;
        *val = tempLong[0] - encoderOffsetG[chan];
      } else {
        /* Shift everything up 8 bits for sign extension */
        tempLong[1] = (long)tempEnc[1] << 8;

        /* Read the middle byte of 2 adjacent channels */
        *((short *)tempEnc) = in16(enc_count_port);
        tempLong[1] |= (long)tempEnc[1] << 16;

        /* Read the high byte of two adjacent channels */
        *((short *)tempEnc) = in16(enc_count_port);
        tempLong[1] |= (long)tempEnc[1] << 24;
        tempLong[1] /= 256;
        *val = tempLong[1] - encoderOffsetG[chan];
      } /* endif */
      return I_OK;
    case STATUS:        /* Read the entire status word into long int - STG */
      *val = (long)((unsigned char)in8(mmap_device_io(0x07, pumaVarsG.base + STATUS_REG)));
      return I_OK;
    case POT:           /* Read a single ADC - STG2 */
#ifdef STG_MODEL2
      out8(mmap_device_io(0x07, pumaVarsG.base+ADC_MUX_SELECT), (chan << 4) | ADC_MUX_CMD);
      /* Delay for MUX settling - dummy read */
      uintptr_t brdtest_port = mmap_device_io(0x07, pumaVarsG.base + BRDTEST);
	  eoc = in8(brdtest_port) & ADC_STAT_MASK;

      /* Start conversion */
      out16(mmap_device_io(0x0F, pumaVarsG.base + ADC_START), 0);

      /* --- Wait for the polling delay. */
      eoc = 0;
      while (eoc == 0){
        eoc = in8(brdtest_port) & ADC_STAT_MASK;
      } /* endwhile */

      /* --- Wait for conversion complete. /EOC goes low */
      while (eoc != 0){
        eoc = in8(brdtest_port) & ADC_STAT_MASK;
      } /* endwhile */

      *val = (short)((in16(mmap_device_io(0x0F, pumaVarsG.base + ADC_VALUE))) << 3);
      *val = *val >> 3;
#else
      in16(mmap_device_io(0x0F, pumaVarsG.base + ADC_MUX_SELECT + 2*chan)); /* select the MUX channel */
      out16(mmap_device_io(0x0F, pumaVarsG.base + ADC_START + 2*chan),0);   /* trigger the conversion */

      /* --- Wait for the polling delay. */
      eoc = 1;
	  uintptr_t intr_irr_port = mmap_device_io(0x07, pumaVarsG.base + INTR_IRR);
	  uintptr_t intr_ocw3_port = mmap_device_io(0x07, pumaVarsG.base + INTR_OCW3);
	  
      while (eoc != 0){
        out8(intr_ocw3_port, OCW3_IRR_SEL);
        eoc = in8(intr_irr_port) & ADC_STAT_MASK;
      } /* endwhile */

      /* --- Wait for conversion complete. */
      while (eoc == 0){
        out8(intr_ocw3_port, OCW3_IRR_SEL);
        eoc = in8(intr_irr_port) & ADC_STAT_MASK;
      } /* endwhile */

      *val = (short)((in16(mmap_device_io(0x0F,pumaVarsG.base + ADC_VALUE + 2*chan))) << 3);
      *val = *val >> 3;
#endif
      return I_OK;
    case POT6:          /* Read all ADC's - STG2 */
      for(i=0; i<6; i++){
#ifdef STG_MODEL2
        out8(mmap_device_io(0x07, pumaVarsG.base + ADC_MUX_SELECT), (i << 4) | ADC_MUX_CMD);
		uintptr_t brdtest_port = mmap_device_io(0x07, pumaVarsG.base + BRDTEST);
        /* Delay for MUX settling - dummy read */
		
        eoc = in8(brdtest_port) & ADC_STAT_MASK;

        /* Start conversion */
        out16(mmap_device_io(0x0F, pumaVarsG.base + ADC_START), 0);

        /* --- Wait for the polling delay. */
        eoc = 0;
        while (eoc == 0){
          eoc = in8(brdtest_port) & ADC_STAT_MASK;
        } /* endwhile */

        /* --- Wait for conversion complete. /EOC goes low */
        while (eoc != 0){
          eoc = in8(brdtest_port) & ADC_STAT_MASK;
        } /* endwhile */

        val[i] = (short)((in16(mmap_device_io(0x0F, pumaVarsG.base + ADC_VALUE))) << 3);
        val[i] = val[i] >> 3;
#else
        in16(mmap_device_io(0x0F, pumaVarsG.base + ADC_MUX_SELECT + 2*i)); /* select the MUX channel */
        out16(mmap_device_io(0x0F, pumaVarsG.base + ADC_START + 2*i),0);   /* trigger the conversion */

        /* --- Wait for the polling delay. */
		uintptr_t intr_ocw3_port = mmap_device_io(0x07, pumaVarsG.base + INTR_OCW3);
		uintptr_t intr_irr_port = mmap_device_io(0x07, pumaVarsG.base + INTR_IRR)
        eoc = 1;
        while (eoc != 0){
          out8(intr_ocw3_port, OCW3_IRR_SEL);
          eoc = in8(intr_irr_port) & ADC_STAT_MASK;
        } /* endwhile */

        /* --- Wait for conversion complete. */
        while (eoc == 0){
          out8(intr_ocw3_port, OCW3_IRR_SEL);
          eoc = in8(intr_irr_port) & ADC_STAT_MASK;
        } /* endwhile */

        val[i] = (short)((in16(mmap_device_io(0x0F, pumaVarsG.base+ADC_VALUE+2*i))) << 3);
        val[i] = val[i] >> 3;
#endif
      } /* endfor */
      return I_OK;
    default:
      return I_BAD_OPTION;
  } /* endswitch */
}

/* --- This function writes 1 short value to the robot. The particular
   --- value is chosen with the OPTION parameter, which is one out of a
   --- list of enumerated ioOptionsT. CHAN ranges from 0 to 7 (corresponding
   --- to PUMA joints 1 - 6 and 2 extra). It returns one of the enumerated ioErrorsT. */
short robot_control(short option, short chan, short val)
{
  short i, tmp;
  volatile char tmpc;
  uintptr_t discrete_ctrl_port;
  
  switch (option) {
    case CLR_INDEX_BIT:         /* Clear one index bit - STG2 */
#ifdef STG_MODEL2
      tmp = ~(JT0_INDEX_BIT << chan);
      out8(mmap_device_io(0x07, pumaVarsG.base + IDL_REG), tmp);
#else
      /* Index bits are paired. Select the proper pair, then odd/even */
	  uintptr_t intc_bit_port = mmap_device_io(0x07, pumaVarsG.base + INTC_BIT_SR);
	  uintptr_t ind_evn_clr_port = mmap_device_io(0x07, pumaVarsG.base + INDEX_EVN_CLR);
	  uintptr_t ind_odd_clr_port = mmap_device_io(0x07, pumaVarsG.base + INDEX_ODD_CLR);
      switch (chan) {
        case 0:
        case 1:
          /* Set up the INT.C register for the correct pair of index bits */
          out8(intc_bit_port, INTC_IXS0_0);
          out8(intc_bit_port, INTC_IXS1_0);
          /* Clear either the even or odd bit */
          if (chan == 0)
            tmpc = in8(ind_evn_clr_port);
          else
            tmpc = in8(ind_odd_clr_port);
          break;
        case 2:
        case 3:
          out8(intc_bit_port, INTC_IXS0_1);
          out8(intc_bit_port, INTC_IXS1_0);
          if (chan == 2)
            tmpc = in8(ind_evn_clr_port);
          else
            tmpc = in8(ind_odd_clr_port);
          break;
        case 4:
        case 5:
          out8(intc_bit_port, INTC_IXS0_0);
          out8(intc_bit_port, INTC_IXS1_1);
          if (chan == 4)
            tmpc = in8(ind_evn_clr_port);
          else
            tmpc = in8(ind_odd_clr_port);
          break;
      } /* endswitch */
#endif
      return I_OK;
    case CLR_INDEX_BITS:        /* Clear all index bits - STG2 */
#ifdef STG_MODEL2
      out8(mmap_device_io(0x07, pumaVarsG.base + IDL_REG), 0);
#else
      /* Index bits are paired. */
	  uintptr_t intc_bit_port = mmap_device_io(0x07, pumaVarsG.base + INTC_BIT_SR);
	  uintptr_t ind_evn_clr_port = mmap_device_io(0x07, pumaVarsG.base + INDEX_EVN_CLR);
	  uintptr_t ind_odd_clr_port = mmap_device_io(0x07, pumaVarsG.base + INDEX_ODD_CLR);
      /* Set up the INT.C register for the first pair (0 and 1) */
      out8(intc_bit_port, INTC_IXS0_0);
      out8(intc_bit_port, INTC_IXS1_0);
      /* Clear the even and odd bits */
      tmpc = in8(ind_evn_clr_port);
      tmpc = in8(ind_odd_clr_port);

      /* Set up INT.C for second pair (2 and 3) */
      out8(intc_bit_port, INTC_IXS0_1);
      out8(intc_bit_port, INTC_IXS1_0);
      /* Clear the even and odd bits */
      tmpc = in8(ind_evn_clr_port);
      tmpc = in8(ind_odd_clr_port);

      /* Set up INT.C for third pair (4 and 5) */
      out8(intc_bit_port, INTC_IXS0_0);
      out8(intc_bit_port, INTC_IXS1_1);
      tmpc = in8(ind_evn_clr_port);
      tmpc = in8(ind_odd_clr_port);
#endif
      return I_OK;
    case INDEX_LVL_HI:          /* STG Model 1 only */
      out8(mmap_device_io(0x07, pumaVarsG.base + INTC_BIT_SR), INTC_IXLVL_1);
      return I_OK;
    case INDEX_LVL_LO:          /* STG Model 1 only */
      out8(mmap_device_io(0x07, pumaVarsG.base + INTC_BIT_SR), INTC_IXLVL_0);
      return I_OK;
    case POWER:         /* Enable/disable arm power - STG */
      if (val != 0){
        out8(mmap_device_io(0x07, pumaVarsG.base + DISCRETE_CTRL), CTRL_POWER_ON);
      } else {
        out8(mmap_device_io(0x07, pumaVarsG.base + DISCRETE_CTRL), CTRL_POWER_OFF);
      } /* endif */
      pumaVarsG.discrete = in8(mmap_device_io(0x07, pumaVarsG.base + DISCRETE_REG));
      return I_OK;
    case CALIB:         /* Set/Clear Calibration Status bit - STG */
      /* The meaning of this bit is inverted (lo = calibrated) */
      if (val != 0){
        out8(mmap_device_io(0x07, pumaVarsG.base + DISCRETE_CTRL), CTRL_SET_CALIB);
      } else {
        out8(mmap_device_io(0x07, pumaVarsG.base + DISCRETE_CTRL), CTRL_CLR_CALIB);
      } /* endif */
      pumaVarsG.discrete = in8(mmap_device_io(0x07, pumaVarsG.base + DISCRETE_REG));
      return I_OK;
    case STATUS:        /* Write the entire status word */
      out16(mmap_device_io(0x0F, pumaVarsG.base + DISCRETE_REG), val);
      pumaVarsG.discrete = in8(mmap_device_io(0x07, pumaVarsG.base + DISCRETE_REG));
      return I_OK;
    case HAND:          /* Open/Close the pneumatic hand */
	  discrete_ctrl_port = mmap_device_io(0x07, pumaVarsG.base + DISCRETE_CTRL);
      if (val == 0){    /* 0 = close the hand */
        out8(discrete_ctrl_port, CTRL_HANDOPEN_OFF);
        out8(discrete_ctrl_port, CTRL_HANDCLOSE_ON);
      } else if (val == -1) {  /* -1 = relax the hand */
        out8(discrete_ctrl_port, CTRL_HANDOPEN_OFF);
        out8(discrete_ctrl_port, CTRL_HANDCLOSE_OFF);
      } else {          /* 1 = open the hand */
        out8(discrete_ctrl_port, CTRL_HANDCLOSE_OFF);
        out8(discrete_ctrl_port, CTRL_HANDOPEN_ON);
      } /* endif */
      pumaVarsG.discrete = in8(mmap_device_io(0x07,pumaVarsG.base + DISCRETE_REG));
      return I_OK;
    case CLR_ADC_BIT:   /* Clear A/D status bit */
    default:
      printf("ERROR: robot_control() option failed\n");
      return I_BAD_OPTION;
  } /* endswitch */
}

/* --- This function reads 1 short value of the robot. The particular
   --- value is chosen with the OPTION parameter, which is one out of a
   --- list of enumerated ioOptionsT. CHAN ranges from 0 to 5 (corresponding
   --- to PUMA joints 1 - 6). It returns one of the enumerated ioErrorsT. */
short robot_status(short option, short chan, short *val)
{
  short i, tmp;

  switch (option) {
    case INDEX_BIT:             /* Read one index bit - STG2 */
      /* Return value of 0 means index found, 1 means no index pulse */
#ifdef STG_MODEL2
      tmp = (short)((unsigned char)(~in8(mmap_device_io(0x07, pumaVarsG.base + IDL_REG))));
      *val = tmp & (unsigned char)(JT0_INDEX_BIT << chan);
#else
      /* Index bits are paired. Select the proper pair and read IRR */
	  uintptr_t intc_bit_port = mmap_device_io(0x07, pumaVarsG.base + INTC_BIT_SR);
	  uintptr_t intr_ocw3_port = mmap_device_io(0x07,pumaVarsG.base + INTR_OCW3);
	  uintptr_t intr_irr_port = mmap_device_io(0x07,pumaVarsG.base + INTR_IRR);
      switch (chan) {
        case 0:
        case 1:
          /* Set up the INT.C register for the correct pair of index bits */
          out8(intc_bit_port, INTC_IXS0_0);
          out8(intc_bit_port, INTC_IXS1_0);
          /* Tell the 82C59 to return the IRR bits */
          out8(intr_ocw3_port, OCW3_IRR_SEL);
          /* Read IRR and mask off all but the even or odd index bit */
          if (chan == 0)
            *val = (short)(in8(intr_irr_port) &
                      JT0_INDEX_BIT);
          else
            *val = (short)(in8(intr_irr_port) &
                      JT1_INDEX_BIT);
          break;
        case 2:
        case 3:
          /* Set up the INT.C register for the correct pair of index bits */
          out8(intc_bit_port, INTC_IXS0_1);
          out8(intc_bit_port, INTC_IXS1_0);
          /* Tell the 82C59 to return the IRR bits */
          out8(intr_ocw3_port, OCW3_IRR_SEL);
          /* Read IRR and mask off all but the even or odd index bit */
          if (chan == 2)
            *val = (short)(in8(intr_irr_port) &
                      JT0_INDEX_BIT);
          else
            *val = (short)(in8(intr_irr_port) &
                      JT1_INDEX_BIT);
          break;
        case 4:
        case 5:
          /* Set up the INT.C register for the correct pair of index bits */
          out8(intc_bit_port, INTC_IXS0_0);
          out8(intc_bit_port, INTC_IXS1_1);
          /* Tell the 82C59 to return the IRR bits */
          out8(intr_ocw3_port, OCW3_IRR_SEL);
          /* Read IRR and mask off all but the even or odd index bit */
          if (chan == 4)
            *val = (short)(in8(intr_irr_port) &
                      JT0_INDEX_BIT);
          else
            *val = (short)(in8(intr_irr_port) &
                      JT1_INDEX_BIT);
          break;
        default:
          *val = 0;
      } /* endswitch */
      /* Invert the index bit to be consistent with TRC004 and robot.c */
      if (*val !=0)
        *val = 0;
      else
        *val = 1;
#endif
      return I_OK;

/* This code is not used but provides an alternative for STATUS if
   Port B is used for other purposes. */
#if 0
    case STATUS:        /* Read the main status byte, Port D - STG2 */
      *val = (short)((unsigned char)in8(mmap_device_io(0x07, pumaVarsG.base + STATUS_REG)));
      return I_OK;
    case AUX_STATUS:    /* Read the auxilliary status byte, Port B - STG */
/*      *val = (short)(in8(pumaVarsG.base + AUXSTAT_REG) << 8);*/
      *val = (short)(in8(mmap_device_io(0x07, pumaVarsG.base + AUXSTAT_REG)));
      return I_OK;
#endif

    case STATUS:        /* Read both status bytes, Ports B & D - STG2 */
      *val = ((short)in8(mmap_device_io(0x07, pumaVarsG.base + AUXSTAT_REG))) << 8;
      *val = *val | ((unsigned char)in8(mmap_device_io(0x07, pumaVarsG.base + STATUS_REG)));
      return I_OK;
    case POWER:         /* Read Port D - STG2 */
      *val = (short)(in8(mmap_device_io(0x07, pumaVarsG.base + STATUS_REG)) & POWER_ON_MASK);
      return I_OK;
    case CALIB:         /* Read Port B - STG2 */
      /* hi means it is calibrated */
      *val = (short)(~in8(mmap_device_io(0x07, pumaVarsG.base + AUXSTAT_REG)) & CALIB_MASK);
      return I_OK;
    case HAND:          /* Read the hand status bits - Open is bit 0, closed is bit 1 */
      *val = (short)(~in8(mmap_device_io(0x07, pumaVarsG.base + DISCRETE_REG)) & HAND_OPEN_BIT) >> 6;
      *val = *val | ((short)(~in8(mmap_device_io(0x07, pumaVarsG.base + DISCRETE_REG)) & HAND_CLOSED_BIT) >> 4);
      return I_OK;
    case ALL_INDEX_BITS:        /* Read all index bits */
#ifdef STG_MODEL2
      /* Return value of 0 means index found, 1 means no index pulse */
      *val = (short)((unsigned char)(~in8(mmap_device_io(0x07, pumaVarsG.base + IDL_REG))));
      return I_OK;
#endif
    default:
      printf("ERROR: robot_status() option failed\n");
      return I_BAD_OPTION;
  } /* endswitch */
}

/* --- This function reads the POTs using 16-bit words. It is similar
   --- to robot_longrd(). This is *NOT* the recommended interface to
   --- the A/D's and is provided for backward compatibility and testing
   --- only. It returns one of the enumerated ioErrorsT. */
short robot_potrd(short option, short chan, long *val)
{
  short i,
        tempPos;

  printf("WARNING: potrd() called\n");
  switch (option) {
    case POT:           /* Read a single ADC */
      out16(mmap_device_io(0x0F, pumaVarsG.base + ADC_MUX_SELECT), chan); /* select the MUX channel */
      in16(mmap_device_io(0x0F, pumaVarsG.base + ADC_START));             /* trigger the conversion */
      /* --- Enable the ADC conversion complete bit. */
      pumaVarsG.discrete = pumaVarsG.discrete | ADC_MASK_BIT;
      out16(mmap_device_io(0x0F, pumaVarsG.base + DISCRETE_REG), pumaVarsG.discrete);
      /* --- Wait for the conversion complete flag. */
      while ((in16(mmap_device_io(0x0F, pumaVarsG.base + STATUS_REG)) & ADC_STAT_MASK) != 0);
      *val = (long)(in16(pumaVarsG.base + ADC_VALUE) & 0x00FF);
      /* --- Disable and clear the ADC conversion complete bit. */
      pumaVarsG.discrete = pumaVarsG.discrete & (~ADC_MASK_BIT);
      out16(mmap_device_io(0x0F, pumaVarsG.base + DISCRETE_REG), pumaVarsG.discrete);
      return I_OK;
    case POT6:          /* Read all ADC's */
      for(i=0; i<6; i++){
        out16(mmap_device_io(0x0F, pumaVarsG.base + ADC_MUX_SELECT), i); /* select the MUX channel */
        in16(mmap_device_io(0x0F, pumaVarsG.base + ADC_START));             /* trigger the conversion */
        /* --- Enable the ADC conversion complete bit. */
        pumaVarsG.discrete = pumaVarsG.discrete | ADC_MASK_BIT;
        out16(mmap_device_io(0x0F, pumaVarsG.base + DISCRETE_REG), pumaVarsG.discrete);
        /* --- Wait for the conversion complete flag. */
        while ((in16(mmap_device_io(0x0F, pumaVarsG.base + STATUS_REG)) & ADC_STAT_MASK) != 0);
        val[i] = (long)(in16(mmap_device_io(0x0F, pumaVarsG.base + ADC_VALUE)) & 0x00FF);
        /* --- Disable and clear the ADC conversion complete bit. */
        pumaVarsG.discrete = pumaVarsG.discrete & (~ADC_MASK_BIT);
        out16(mmap_device_io(0x0F, pumaVarsG.base + DISCRETE_REG), pumaVarsG.discrete);
      } /* endfor */
      return I_OK;
    default:
      return I_BAD_OPTION;
  } /* endswitch */
}
