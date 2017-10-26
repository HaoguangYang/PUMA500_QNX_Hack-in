/* --- ROBOT.C          Version 4.3
   --- This file contains the main routines for running the PUMA
   --- robot with the Trident Robotics and Research, Inc. interface
   --- boards.
   ---
   --- These are only demo routines and are not complete.
   --- For serious operation, your own interrupt routines must
   --- handle the system clock properly and properly save and
   --- restore state. You are free to use these routines, or any
   --- part thereof, AT YOUR OWN RISK, provided this entire comment
   --- block and the included copyright notice remain intact.
   ---
   --- Initial coding by Richard Voyles, Vigilant Technologies.
   --- Further modifications by Richard Voyles, Trident Robotics
   ---                      and Haoguang Yang, Tsinghua University.
   ---
   --- Copyright 1993-2000 Trident Robotics and Research, Inc.
   ---
   --- 12/02/93 RMV     Modified to work with TRC004 rev E.
   --- 02/21/95 RMV     Modified POT reading to perform byte reads.
   --- 11/08/95 RMV     Added POT6 and INDEX6 to robot_longrd().
   ---                  Added range checking to robot_fltwr(TORQUE6).
   --- 12/12/95 RMV     Added robot_potrd() to read A/D's with
   ---                  16-bit words. NOT RECOMMENDED FOR USERS!!
   --- 12/31/95 RMV     Changed motor and encoder scale factors.
   --- 02/02/96 RMV     Added robot_status(), robot_control(), and
   ---                  pdgControl(). Changed robot_intwr() to
   ---                  robot_shortwr().
   ---                  Added encoderOffsetG[] and changed the way
   ---                  encoders are read!!!             ROBOT 2.0
   --- 02/14/98 RMV     Added extra test to A/D read for fast CPUs.
   --- 09/08/98 RMV     Removed above test. No need for it.
   --- 10/03/99 RMV     Added compiler flag for TRC004 rev F AD7891.  v3.0
   --- 10/05/99 RMV     Provided averaging of the AD7891.
   --- 11/14/99 RMV     robot_control() no longer takes a pointer.
   ---                  **** This will impact user-written code *****
   ---                  Added CALIB and POWER options to robot_status().
   ---                  Added jointOffsetG[] and wristCoupling[] v4.0
   --- 03/26/00 RMV     Fixed sign error in wrist coupling factors. v4.2
   --- 06/19/00 RMV     Version 4.3 includes fix of Q_RefG update in calib.c
   --- 10/26/17 YHG     Updates interface of in*() and out*() functions for
   ---                  compatiblity with QNX 6.x API.
   --- */

#define ROBOT_SRC
#define TRC004F

#include <stdio.h>
#include <math.h>
#include "vtypes.h"
#include "qnx_robot.h"
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
uintptr_t TRC;

/* --- The following values are valid for a PUMA 560 */

/* --- The motor scale converts N-m at the joint to DAC quanta */
/* These are taken from 1986 ICRA paper by Armstrong, Khatib, Burdick,
   and are measured values.    COMMENTED OUT
float   fltmtrScaleG[6] = {-21.0, 11.0, -22.9, 84.6, 102., 96.2}; */

/* The above numbers confirm the following theoretical values which
   are multiplied by -1 to account for the inverting amplifiers. */
float   fltmtrScaleG[6] = {19.99, -10.97, 22.65, -84.80, -97.68, -95.00};

/* For PUMA 560 */
float   motorScaleG[6] = {19.99, -10.97, 22.65, -84.80, -97.68, -95.00};

/* For PUMA 550 Mark I */
/* float   motorScaleG[6] = {-19.99, 10.97, -22.65, -84.80, 0, 95.00}; */

/* --- The encoder scale converts encoder counts to radians. */
/* This set of values is for the TRC004 Rev D. */
/* float   encoderScaleG[6] = {0.00010035, -0.000073156, 0.000117,
                            -0.000082663, -0.000087376, -0.000081885}; */

/* This set of values is for the TRC004 Rev E and beyond. */
float   encoderScaleG[6] = {0.00010035, -0.000073156, 0.000117,
                            -0.000082663, -0.000087376, -0.00016377};

short   encoderOffsetG[6] = {0,0,0,8000,0,0};

/* --- Wrist joint coupling factors 4->5, 4->6, 5->6 (radians) */
/* --- For PUMA 560 */
float   wristCouplingG[3] = { 0.0139037, -0.0130401, 0.180556 };

/* --- Joint offsets to agree with VAL and robotics texts (radians) */
/* --- For PUMA 560 */
float   jointOffsetG[6] = {0.0, -1.5708, 1.5708, 0.0, 0.0, 0.0};

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
  int   i;
  int joint[]={0,0,0,0,0,0};

  //RFKM
  // initialize memory locations
  ThreadCtl(_NTO_TCTL_IO, 0);
  TRC = mmap_device_io(0x40, pumaVarsG.base);
  printf("TRC at 0x%04X\n", TRC);

  /* --- First initialize the PUMA structure. */
  /* --- Start by resetting everyting but the <Joint 6 Overflow> and
     --- <Calibration Status> bits and setting the discrete variable. */
  uintptr_t status_reg_port = mmap_device_io(0x0F, pumaVarsG.base + STATUS_REG);
  uintptr_t discrete_reg_port = mmap_device_io(0x0F, pumaVarsG.base + DISCRETE_REG);
  pumaVarsG.status = in16(status_reg_port);
  pumaVarsG.discrete = (pumaVarsG.status & 0xC000) >> 8;
  out16(discrete_reg_port, pumaVarsG.discrete);
  pumaVarsG.status = in16(status_reg_port);
  /* --- Now set position variables appropriately */
  if ((pumaVarsG.status & CALIB_MASK) == 0){
    /* --- Arm is NOT calibrated so set everything to zero. */
    for (i=0; i<6; i++){
      pumaVarsG.pos[i] = 0L;            /* reset the variable */
      robot_shortwr(POSITION, i, &joint[i]);    /* reset the encoder */
    } /* endfor */
    pumaVarsG.discrete = 0;           /* make sure all bits are cleared */
    out16(discrete_reg_port, pumaVarsG.discrete);
  } else {
    /* --- Arm IS calibrated */
    robot_longrd(POSITION6,0,&pumaVarsG.pos[0]);
  } /* endif */

  // return sched_init(freq);
  return 0;
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
    case TORQUE6:       /* Output to all DACs -- val is in N-m */
      for (i=0; i<6; i++){
        longVal = (long)(val[i] * motorScaleG[i]) + 0x0800;
        if (longVal > DAC_MAX_CNT)
          shortVal = DAC_MAX_CNT;
        else if (longVal < DAC_MIN_CNT)
          shortVal = DAC_MIN_CNT;
        else
          shortVal = (short)longVal;
        out16(mmap_device_io(0x0F, pumaVarsG.base + DAC_BASE + 2*i), shortVal);
      } /* endfor */
      return I_OK;
    case TORQUE:        /* Output to single DAC -- val is in N-m */
      longVal = (long)(*val * motorScaleG[chan]) + 0x0800;
      if (longVal > DAC_MAX_CNT)
        shortVal = DAC_MAX_CNT;
      else if (longVal < DAC_MIN_CNT)
        shortVal = DAC_MIN_CNT;
      else
        shortVal = (short)longVal;
      out16(mmap_device_io(0x0F, pumaVarsG.base + DAC_BASE + 2*chan), shortVal);
      return I_OK;
    case VOLTAGE:       /* Output to single DAC -- val is in volts */
      if (*val >= 10.0){
        intVal = 0;
      } else if (*val <= -9.995){
        intVal = 0x0FFF;
      } else {
        intVal = (int)(*val * DAC_BITS_PER_VOLT) + 0x0800;
      } /* endif */
      out16(mmap_device_io(0x0F, pumaVarsG.base + DAC_BASE + 2*chan), intVal);
      return I_OK;
    case POSITION6:     /* Preset all encoders -- val is in radians */
      for (i=0; i<6; i++){
        intVal = (int)(val[i] / encoderScaleG[i]) + encoderOffsetG[i];
        out16(mmap_device_io(0x0F, pumaVarsG.base + ENC_LOAD_BASE + 2*i), intVal);
      } /* endfor */
      return I_OK;
    case POSITION:      /* Preset a single encoder -- val is in radians */
      intVal = (int)(*val / encoderScaleG[chan]) + encoderOffsetG[chan];
      out16(mmap_device_io(0x0F, pumaVarsG.base + ENC_LOAD_BASE + 2*chan), intVal);
      return I_OK;
    default:
      return I_BAD_OPTION;
  } /* endswitch */
}

/* --- This function writes 1 or 6 integer values to the robot. The particular
   --- value is chosen with the OPTION parameter, which is one out of a
   --- list of enumerated ioOptionsT. CHAN ranges from 0 to 5 (corresponding
   --- to PUMA joints 1 - 6). It returns one of the enumerated ioErrorsT. */
short robot_shortwr(short option, short chan, short *val)
{
  short i;

  switch (option) {
    case TORQUE6:       /* Output to all DACs */
      for (i=0; i<6; i++){
        out16(mmap_device_io(0x0F, pumaVarsG.base + DAC_BASE + 2*i), val[i]);
      } /* endfor */
      return I_OK;
    case TORQUE:        /* Output to single DAC */
      out16(mmap_device_io(0x0F, pumaVarsG.base + DAC_BASE + 2*chan), *val);
      return I_OK;
    case POSITION6:     /* Preset all encoders */
      for (i=0; i<6; i++){
        out16(mmap_device_io(0x0F, pumaVarsG.base + ENC_LOAD_BASE + 2*i),
                        val[i] + encoderOffsetG[i]);
      } /* endfor */
      return I_OK;
    case POSITION:      /* Preset a single encoder */
      out16(mmap_device_io(0x0F, pumaVarsG.base + ENC_LOAD_BASE + 2*chan),
                        *val + encoderOffsetG[chan]);
      return I_OK;
    case STATUS:        /* Write the entire status word */
      out16(mmap_device_io(0x0F, pumaVarsG.base + DISCRETE_REG), *val);
      return I_OK;
    default:
      return I_BAD_OPTION;
  } /* endswitch */
 }

/* --- This function reads 1 or 6 float values from the robot. The particular
   --- value is chosen with the OPTION parameter, which is one out of a
   --- list of enumerated ioOptionsT. CHAN ranges from 0 to 5 (corresponding
   --- to PUMA joints 1 - 6). It returns one of the enumerated ioErrorsT. */
short robot_fltrd(short option, short chan, float *val)
{
  short tempPos,i;
  float jt4, jt5;

  switch (option) {
    case POSITION6:     /* Read all encoders -- val is in radians */
      for (i=0; i<6; i++){
        val[i] = encoderScaleG[i] * ((short)in16(mmap_device_io(0x0F, pumaVarsG.base +
                        ENC_COUNT_BASE + 2*i)) - encoderOffsetG[i]);
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
    case POSITION:      /* Read a single encoder */
      *val = encoderScaleG[chan]*((short)in16(mmap_device_io(0x0F, pumaVarsG.base +
                        ENC_COUNT_BASE + 2*chan)) - encoderOffsetG[chan]);
      switch(chan){
        case 1:
        case 2:
          *val += jointOffsetG[chan];
          break;
        case 4:
          jt4 = encoderScaleG[3]*((short)in16(mmap_device_io(0x0F, pumaVarsG.base +
                        ENC_COUNT_BASE + 6)) - encoderOffsetG[3]);
          *val -= jt4 * wristCouplingG[0];
          break;
        case 5:
          jt4 = encoderScaleG[3]*((short)in16(mmap_device_io(0x0F, pumaVarsG.base +
                        ENC_COUNT_BASE + 6)) - encoderOffsetG[3]);
          jt5 = encoderScaleG[4]*((short)in16(mmap_device_io(0x0F, pumaVarsG.base +
                        ENC_COUNT_BASE + 8)) - encoderOffsetG[4]);
          *val -= jt4 * wristCouplingG[1] + jt5 * wristCouplingG[2];
          break;
      } /* endswitch */
      return I_OK;
    case POT:           /* Read a single ADC */
      uintptr_t status_reg_port = mmap_device_io(0x0F, pumaVarsG.base + STATUS_REG);
#ifdef TRC004F
      /* --- Enable the ADC conversion complete bit. */
      pumaVarsG.discrete = pumaVarsG.discrete | ADC_MASK_BIT;
      out16(mmap_device_io(0x0F, pumaVarsG.base + DISCRETE_REG), pumaVarsG.discrete);
      /* Select the MUX channel and start conversion */
      out16(mmap_device_io(0x0F, pumaVarsG.base + ADC_MUX_SELECT), (chan << 7) | ADC_CMD_VAL);
      /* --- Wait for the conversion complete flag. */
      while ((in16(status_reg_port) & ADC_STAT_MASK) != 0);
      *val = 0.0003052 * (in16(mmap_device_io(0x0F, pumaVarsG.base + ADC_VALUE)) & 0xFFF0);
#else
      out8(mmap_device_io(0x07, pumaVarsG.base + ADC_MUX_SELECT), chan); /* select the MUX channel */
      in8(mmap_device_io(0x07, pumaVarsG.base + ADC_START));             /* trigger the conversion */
      /* --- Enable the ADC conversion complete bit. */
      pumaVarsG.discrete = pumaVarsG.discrete | ADC_MASK_BIT;
      out16(mmap_device_io(0x0F, pumaVarsG.base + DISCRETE_REG), pumaVarsG.discrete);
      /* --- Wait for the conversion to start. TEST */
      while ((in16(status_reg_port) & ADC_STAT_MASK) == 0);
      /* --- Wait for the conversion complete flag. */
      while ((in16(status_reg_port) & ADC_STAT_MASK) != 0);
      *val = 0.0196 * (in8(mmap_device_io(0x07, pumaVarsG.base + ADC_VALUE)) & 0x00FF);
#endif
      /* --- Disable and clear the ADC conversion complete bit. */
      pumaVarsG.discrete = pumaVarsG.discrete & (~ADC_MASK_BIT);
      out16(mmap_device_io(0x0F, pumaVarsG.base + DISCRETE_REG), pumaVarsG.discrete);
      return I_OK;
    default:
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
  long  tempPot;
  uintptr_t status_reg_port = mmap_device_io(0x0F, pumaVarsG.base + STATUS_REG);
  
  switch (option) {
    case POSITION6:     /* Read all encoders */
      for (i=0; i<6; i++){
        val[i] = (long)in16(mmap_device_io(0x0F, pumaVarsG.base + ENC_COUNT_BASE + 2*i)) -
                        encoderOffsetG[i];
      } /* endfor */
      return I_OK;
    case POSITION:      /* Read a single encoder */
      *val = (long)in16(mmap_device_io(0x0F, pumaVarsG.base + ENC_COUNT_BASE + 2*chan)) -
                        encoderOffsetG[chan];
      return I_OK;
    case STATUS:        /* Read the entire status word into long int */
      *val = (long)((unsigned short)in16(status_reg_port));
      return I_OK;
    case POT:           /* Read a single ADC */
#ifdef TRC004F
      tempPot = 0L;
      for (j=0; j<3; j++){
        /* --- Enable the ADC conversion complete bit. */
        pumaVarsG.discrete = pumaVarsG.discrete | ADC_MASK_BIT;
        out16(mmap_device_io(0x0F, pumaVarsG.base + DISCRETE_REG), pumaVarsG.discrete);
        /* Select the MUX channel and start conversion */
        out16(mmap_device_io(0x0F, pumaVarsG.base + ADC_MUX_SELECT), (chan << 7) | ADC_CMD_VAL);
        /* --- Wait for the conversion complete flag. */
        while ((in16(status_reg_port) & ADC_STAT_MASK) != 0);
        tempPot += (long)(in16(mmap_device_io(0x0F, pumaVarsG.base + ADC_VALUE)) & 0xFFF0);
        /* --- Disable and clear the ADC conversion complete bit. */
        pumaVarsG.discrete = pumaVarsG.discrete & (~ADC_MASK_BIT);
        out16(mmap_device_io(0x0F, pumaVarsG.base + DISCRETE_REG), pumaVarsG.discrete);
      } /* endfor */
      *val = (short)(tempPot / 3) & 0xFFF0;
#else
      out8(mmap_device_io(0x07, pumaVarsG.base + ADC_MUX_SELECT), chan); /* select the MUX channel */
      in8(mmap_device_io(0x07, pumaVarsG.base + ADC_START));             /* trigger the conversion */
      /* --- Enable the ADC conversion complete bit. */
      pumaVarsG.discrete = pumaVarsG.discrete | ADC_MASK_BIT;
      out16(mmap_device_io(0x0F, pumaVarsG.base + DISCRETE_REG), pumaVarsG.discrete);
      /* --- Wait for the conversion complete flag. */
      while ((in16(status_reg_port) & ADC_STAT_MASK) != 0);
      *val = (long)(in8(mmap_device_io(0x07, pumaVarsG.base + ADC_VALUE)) & 0x00FF);
      /* --- Disable and clear the ADC conversion complete bit. */
      pumaVarsG.discrete = pumaVarsG.discrete & (~ADC_MASK_BIT);
      out16(mmap_device_io(0x0F, pumaVarsG.base + DISCRETE_REG), pumaVarsG.discrete);
#endif
      return I_OK;
    case POT6:          /* Read all ADC's */
      for(i=0; i<6; i++){
#ifdef TRC004F
        tempPot = 0L;
        for(j=0; j<3; j++){
          /* --- Enable the ADC conversion complete bit. */
          pumaVarsG.discrete = pumaVarsG.discrete | ADC_MASK_BIT;
          out16(mmap_device_io(0x0F, pumaVarsG.base + DISCRETE_REG), pumaVarsG.discrete);
          /* Select the MUX channel and start conversion */
          out16(mmap_device_io(0x0F, pumaVarsG.base + ADC_MUX_SELECT), (i << 7) | ADC_CMD_VAL);
          /* --- Wait for the conversion complete flag. */
          while ((in16(status_reg_port) & ADC_STAT_MASK) != 0);
          tempPot += (long)(in16(mmmap_device_io(0x0F, pumaVarsG.base + ADC_VALUE)) & 0xFFF0);
          /* --- Disable and clear the ADC conversion complete bit. */
          pumaVarsG.discrete = pumaVarsG.discrete & (~ADC_MASK_BIT);
          out16(mmap_device_io(0x0F, pumaVarsG.base + DISCRETE_REG), pumaVarsG.discrete);
        } /* endfor */
        val[i] = (short)(tempPot / 3) & 0xFFF0;
#else
        /* Set the MUX */
        out8(mmap_device_io(0x07, pumaVarsG.base + ADC_MUX_SELECT), i); /* select the MUX channel */
        in8(mmap_device_io(0x07, pumaVarsG.base + ADC_START));          /* trigger the conversion */
        /* --- Enable the ADC conversion complete bit. */
        pumaVarsG.discrete = pumaVarsG.discrete | ADC_MASK_BIT;
        out16(mmap_device_io(0x0F, pumaVarsG.base + DISCRETE_REG), pumaVarsG.discrete);
        /* --- Wait for the conversion complete flag. */
        while ((in16(status_reg_port) & ADC_STAT_MASK) != 0);
        val[i] = (long)(in8(mmap_device_io(0x07, pumaVarsG.base + ADC_VALUE)) & 0x00FF);
        /* --- Disable and clear the ADC conversion complete bit. */
        pumaVarsG.discrete = pumaVarsG.discrete & (~ADC_MASK_BIT);
        out16(mmap_device_io(0x0F, pumaVarsG.base + DISCRETE_REG), pumaVarsG.discrete);
#endif
      } /* endfor */
      return I_OK;
    case INDEX:     /* Read one index register */
      *val = (long)in16(mmap_device_io(0x0F, pumaVarsG.base + 2*chan)) - encoderOffsetG[chan];
      return I_OK;
    case INDEX6:     /* Read all index registers */
      for (i=0; i<6; i++){
        val[i] = (long)in16(mmap_device_io(0x0F, pumaVarsG.base + 2*i)) - encoderOffsetG[i];
      } /* endfor */
      return I_OK;
    default:
      return I_BAD_OPTION;
  } /* endswitch */
}

/* --- This function writes 1 short value to the robot. The particular
   --- value is chosen with the OPTION parameter, which is one out of a
   --- list of enumerated ioOptionsT. CHAN ranges from 0 to 5 (corresponding
   --- to PUMA joints 1 - 6). It returns one of the enumerated ioErrorsT. */
short robot_control(short option, short chan, short val)
{
  short i, tmp;
  uintptr_t discrete_reg_port = mmap_device_io(0x0F, pumaVarsG.base + DISCRETE_REG);

  switch (option) {
    case CLR_INDEX_BIT:         /* Clear one index bit */
      tmp = JT1_INDEX_BIT << chan;
      /* Clear the index bit */
      out16(discrete_reg_port, pumaVarsG.discrete & ~tmp);
      tmp = pumaVarsG.discrete = pumaVarsG.discrete | tmp;
      /* Re-enable the index bit */
      out16(discrete_reg_port, tmp);
      return I_OK;
    case CLR_INDEX_BITS:        /* Clear all index bits */
      /* Clear all index bits */
      out16(discrete_reg_port, pumaVarsG.discrete & ~((short)ALL_INDEX_BITS));
      tmp = pumaVarsG.discrete = pumaVarsG.discrete | ALL_INDEX_BITS;
      /* Re-enable all index bits */
      out16(discrete_reg_port, tmp);
      return I_OK;
    case CLR_ADC_BIT:   /* Clear A/D status bit */
      out16(discrete_reg_port, pumaVarsG.discrete & ~((short)ADC_MASK_BIT));
      tmp = pumaVarsG.discrete = pumaVarsG.discrete | ADC_MASK_BIT;
      out16(discrete_reg_port, tmp);
      return I_OK;
    case POWER:         /* Enable/disable arm power */
      if (val != 0){
        tmp = pumaVarsG.discrete = pumaVarsG.discrete | POWER_BIT;
        out16(discrete_reg_port, tmp);
      } else {
        tmp = pumaVarsG.discrete = pumaVarsG.discrete & ~(POWER_BIT);
        out16(discrete_reg_port, tmp);
      } /* endif */
      return I_OK;
    case CALIB:         /* Set/Clear calibration status bit */
      if (val != 0){
        tmp = pumaVarsG.discrete = pumaVarsG.discrete | CALIB_BIT;
        out16(discrete_reg_port, tmp);
      } else {
        tmp = pumaVarsG.discrete = pumaVarsG.discrete & ~(CALIB_BIT);
        out16(discrete_reg_port, tmp);
      } /* endif */
      return I_OK;
    case STATUS:        /* Write the entire status word */
      out16(discrete_reg_port, val);
      return I_OK;
    case HAND:          /* Open/Close the pneumatic hand */
      if (val == 0){    /* 0 = close the hand */
        tmp = pumaVarsG.discrete = (pumaVarsG.discrete | HAND_OPEN_BIT) &
                                        ~(HAND_CLOSED_BIT);
        out16(discrete_reg_port, tmp);
      } else if (val == -1) {  /* -1 = relax the hand */
        tmp = pumaVarsG.discrete = pumaVarsG.discrete | HAND_CLOSED_BIT |
                                        HAND_OPEN_BIT;
        out16(discrete_reg_port, tmp);
      } else {          /* 1 = open the hand */
        tmp = pumaVarsG.discrete = (pumaVarsG.discrete | HAND_CLOSED_BIT) &
                                        ~(HAND_OPEN_BIT);
        out16(discrete_reg_port, tmp);
      } /* endif */
      return I_OK;
    default:
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
  uintptr_t status_reg_port = mmap_device_io(0x0F, pumaVarsG.base + STATUS_REG);

  switch (option) {
    case INDEX_BIT:             /* Read one index bit */
      *val = in16(status_reg_port);
      tmp = JT1_INDEX_BIT << chan;
      *val = *val & tmp;
      if (*val !=0)
        *val = 1;
      return I_OK;
    case ALL_INDEX_BITS:        /* Read all index bits */
      *val = in16(status_reg_port);
      *val = *val & ALL_INDEX_BITS;
      return I_OK;
    case STATUS:        /* Read the entire status word */
      *val = in16(status_reg_port);
      return I_OK;
    case POWER:         /* Read the power bit of the status word */
      *val = in16(status_reg_port) & POWER_ON_MASK;
      return I_OK;
    case CALIB:         /* Read the calibration status bit */
      *val = in16(status_reg_port) & CALIB_MASK;
      return I_OK;
    default:
      return I_BAD_OPTION;
  } /* endswitch */
}

/* --- This function reads the POTs using 16-bit words. It is similar
   --- to robot_longrd(). This is *NOT* the recommended interface to
   --- the A/D's and is provided for backward compatibility and testing
   --- only. It returns one of the enumerated ioErrorsT. */
short robot_potrd(short option, short chan, long *val)
{
  short i,tempPos;
  uintptr_t status_reg_port = mmap_device_io(0x0F, pumaVarsG.base + STATUS_REG);
  uintptr_t discrete_reg_port = mmap_device_io(0x0F, pumaVarsG.base + DISCRETE_REG);

  switch (option) {
    case POT:           /* Read a single ADC */
      out16(mmap_device_io(0x0F, pumaVarsG.base + ADC_MUX_SELECT), chan); /* select the MUX channel */
      in16(mmap_device_io(0x0F, pumaVarsG.base + ADC_START));             /* trigger the conversion */
      /* --- Enable the ADC conversion complete bit. */
      pumaVarsG.discrete = pumaVarsG.discrete | ADC_MASK_BIT;
      out16(discrete_reg_port, pumaVarsG.discrete);
      /* --- Wait for the conversion complete flag. */
      while ((in16(status_reg_port) & ADC_STAT_MASK) != 0);
      *val = (long)(in16(mmap_device_io(0x0F, pumaVarsG.base + ADC_VALUE)) & 0x00FF);
      /* --- Disable and clear the ADC conversion complete bit. */
      pumaVarsG.discrete = pumaVarsG.discrete & (~ADC_MASK_BIT);
      out16(discrete_reg_port, pumaVarsG.discrete);
      return I_OK;
    case POT6:          /* Read all ADC's */
      for(i=0; i<6; i++){
        out16(mmap_device_io(0x0F, pumaVarsG.base + ADC_MUX_SELECT), i); /* select the MUX channel */
        in16(mmap_device_io(0x0F, pumaVarsG.base + ADC_START));          /* trigger the conversion */
        /* --- Enable the ADC conversion complete bit. */
        pumaVarsG.discrete = pumaVarsG.discrete | ADC_MASK_BIT;
        out16(discrete_reg_port, pumaVarsG.discrete);
        /* --- Wait for the conversion complete flag. */
        while ((in16(status_reg_port) & ADC_STAT_MASK) != 0);
        val[i] = (long)(in16(mmap_device_io(0x0F, pumaVarsG.base + ADC_VALUE)) & 0x00FF);
        /* --- Disable and clear the ADC conversion complete bit. */
        pumaVarsG.discrete = pumaVarsG.discrete & (~ADC_MASK_BIT);
        out16(discrete_reg_port, pumaVarsG.discrete);
      } /* endfor */
      return I_OK;
    default:
      return I_BAD_OPTION;
  } /* endswitch */
}

#define TRC006D_ADC_MUX         0
#define TRC006D_ADC_VAL         0
#define TRC006D_ADC_CMD         0x0005

/* --- This function reads the 12-bit A/D converter on the TRC006 board.
   --- It has no effect on the TRC004.
   --- Like robot_longrd(), it returns one of the enumerated ioErrorsT. */
short trc006_longrd(short option, short chan, long *val)
{
  short i,tempPos;
  long  cnt;
  uintptr_t adc_mux_port = mmap_device_io(0x0F, pumaVarsG.base + TRC006D_ADC_MUX);
  uintptr_t adc_val_port = mmap_device_io(0x0F, pumaVarsG.base + TRC006D_ADC_VAL);

  switch (option) {
    case POT:           /* Read a single ADC */
      out16(adc_mux_port, (chan << 3) | TRC006D_ADC_CMD); /* select the MUX channel */
      /* --- Wait for the conversion complete flag. */
      for (cnt=0; cnt<1; cnt++);
      *val = (long)(in16(adc_val_port) << 4);
      return I_OK;
    case POT6:          /* Read all ADC's */
      for(i=0; i<6; i++){
        out16(adc_mux_port, (i << 3) | TRC006D_ADC_CMD); /* select the MUX channel */
        /* --- Wait for the conversion complete flag. */
        for (cnt=0; cnt<1; cnt++);
        val[i] = (long)(in16(adc_val_port) << 4);
      } /* endfor */
      return I_OK;
    default:
      return I_BAD_OPTION;
  } /* endswitch */
}
