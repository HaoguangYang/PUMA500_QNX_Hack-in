/*
 * Template.c
 *
 * Code generation for model "Template".
 *
 * Model version              : 1.47
 * Simulink Coder version : 8.9 (R2015b) 13-Aug-2015
 * C source code generated on : Thu Jan 18 11:45:09 2018
 *
 * Target selection: qnx_trc.tlc
 * Note: GRT includes extra infrastructure and instrumentation for prototyping
 * Embedded hardware selection: Altera->SoC (ARM Cortex A)
 * Code generation objectives: Unspecified
 * Validation result: Not run
 */

#include "Template.h"
#include "Template_private.h"

/* Real-time model */
RT_MODEL_Template_T Template_M_;
RT_MODEL_Template_T *const Template_M = &Template_M_;

//====================Declare any global variables here.=====================

static void rate_monotonic_scheduler(void);
time_T rt_SimUpdateDiscreteEvents(
  int_T rtmNumSampTimes, void *rtmTimingData, int_T *rtmSampleHitPtr, int_T
  *rtmPerTaskSampleHits )
{
  rtmSampleHitPtr[1] = rtmStepTask(Template_M, 1);
  UNUSED_PARAMETER(rtmNumSampTimes);
  UNUSED_PARAMETER(rtmTimingData);
  UNUSED_PARAMETER(rtmPerTaskSampleHits);
  return(-1);
}

/*
 *   This function updates active task flag for each subrate
 * and rate transition flags for tasks that exchange data.
 * The function assumes rate-monotonic multitasking scheduler.
 * The function must be called at model base rate so that
 * the generated code self-manages all its subrates and rate
 * transition flags.
 */
static void rate_monotonic_scheduler(void)
{
  /* Compute which subrates run during the next base time step.  Subrates
   * are an integer multiple of the base rate counter.  Therefore, the subtask
   * counter is reset when it reaches its limit (zero means run).
   */
  (Template_M->Timing.TaskCounters.TID[1])++;
  if ((Template_M->Timing.TaskCounters.TID[1]) > 24) {/* Sample time: [1.0s, 0.0s] */
    Template_M->Timing.TaskCounters.TID[1] = 0;
  }
}

/* Model output function for TID0 */
static void Template_output0(void)     /* Sample time: [0.04s, 0.0s] */
{
  {                                    /* Sample time: [0.04s, 0.0s] */
    rate_monotonic_scheduler();
  }
  //===============Put your RT periodic computation block here.====================
  
}

/* Model update function for TID0 */
static void Template_update0(void)     /* Sample time: [0.04s, 0.0s] */
{
  /* Update absolute time */
  /* The "clockTick0" counts the number of times the code of this task has
   * been executed. The absolute time is the multiplication of "clockTick0"
   * and "Timing.stepSize0". Size of "clockTick0" ensures timer will not
   * overflow during the application lifespan selected.
   * Timer of this task consists of two 32 bit unsigned integers.
   * The two integers represent the low bits Timing.clockTick0 and the high bits
   * Timing.clockTickH0. When the low bit overflows to 0, the high bits increment.
   */
  if (!(++Template_M->Timing.clockTick0)) {
    ++Template_M->Timing.clockTickH0;
  }

  Template_M->Timing.t[0] = Template_M->Timing.clockTick0 *
    Template_M->Timing.stepSize0 + Template_M->Timing.clockTickH0 *
    Template_M->Timing.stepSize0 * 4294967296.0;
}

/* Model output function for TID1 */
static void Template_output1(void)     /* Sample time: [1.0s, 0.0s] */
{
  /* Level2 S-Function Block: '<Root>/Initialization' (robot_init) */
  {
    SimStruct *rts = Template_M->childSfunctions[0];
    sfcnOutputs(rts, 1);
  }
}

/* Model update function for TID1 */
static void Template_update1(void)     /* Sample time: [1.0s, 0.0s] */
{
  /* Update absolute time */
  /* The "clockTick1" counts the number of times the code of this task has
   * been executed. The absolute time is the multiplication of "clockTick1"
   * and "Timing.stepSize1". Size of "clockTick1" ensures timer will not
   * overflow during the application lifespan selected.
   * Timer of this task consists of two 32 bit unsigned integers.
   * The two integers represent the low bits Timing.clockTick1 and the high bits
   * Timing.clockTickH1. When the low bit overflows to 0, the high bits increment.
   */
  if (!(++Template_M->Timing.clockTick1)) {
    ++Template_M->Timing.clockTickH1;
  }

  Template_M->Timing.t[1] = Template_M->Timing.clockTick1 *
    Template_M->Timing.stepSize1 + Template_M->Timing.clockTickH1 *
    Template_M->Timing.stepSize1 * 4294967296.0;
}

/* Model output wrapper function for compatibility with a static main program */
static void Template_output(int_T tid)
{
  switch (tid) {
   case 0 :
    Template_output0();
    break;

   case 1 :
    Template_output1();
    break;

   default :
    break;
  }
}

/* Model update wrapper function for compatibility with a static main program */
static void Template_update(int_T tid)
{
  switch (tid) {
   case 0 :
    Template_update0();
    break;

   case 1 :
    Template_update1();
    break;

   default :
    break;
  }
}

/* Model initialize function */
static void Template_initialize(void)
{
	//===========Put your initialization of matrices and vectors HERE.==============
	//Runs ONLY ONCE before entering the real-time loop.
}

/* Model terminate function */
static void Template_terminate(void)
{
  /* Level2 S-Function Block: '<Root>/Initialization' (robot_init) */
  {
    SimStruct *rts = Template_M->childSfunctions[0];
    sfcnTerminate(rts);
  }
}

/*========================================================================*
 * Start of Classic call interface                                        *
 *========================================================================*/
void MdlOutputs(int_T tid)
{
  Template_output(tid);
}

void MdlUpdate(int_T tid)
{
  Template_update(tid);
}

void MdlInitializeSizes(void)
{
}

void MdlInitializeSampleTimes(void)
{
}

void MdlInitialize(void)
{
}

void MdlStart(void)
{
  Template_initialize();
}

void MdlTerminate(void)
{
  Template_terminate();
}

/* Registration function */
RT_MODEL_Template_T *Template(void)
{
  /* Registration code */

  /* initialize non-finites */
  rt_InitInfAndNaN(sizeof(real_T));

  /* initialize real-time model */
  (void) memset((void *)Template_M, 0,
                sizeof(RT_MODEL_Template_T));
  rtsiSetSolverName(&Template_M->solverInfo,"FixedStepDiscrete");
  Template_M->solverInfoPtr = (&Template_M->solverInfo);

  /* Initialize timing info */
  {
    int_T *mdlTsMap = Template_M->Timing.sampleTimeTaskIDArray;
    mdlTsMap[0] = 0;
    mdlTsMap[1] = 1;
    Template_M->Timing.sampleTimeTaskIDPtr = (&mdlTsMap[0]);
    Template_M->Timing.sampleTimes = (&Template_M->Timing.sampleTimesArray[0]);
    Template_M->Timing.offsetTimes = (&Template_M->Timing.offsetTimesArray[0]);

    /* task periods */
    Template_M->Timing.sampleTimes[0] = (0.04);		//Period #1
    Template_M->Timing.sampleTimes[1] = (1.0);		//Base Period

    /* task offsets */
    Template_M->Timing.offsetTimes[0] = (0.0);
    Template_M->Timing.offsetTimes[1] = (0.0);
  }

  rtmSetTPtr(Template_M, &Template_M->Timing.tArray[0]);

  {
    int_T *mdlSampleHits = Template_M->Timing.sampleHitArray;
    int_T *mdlPerTaskSampleHits = Template_M->Timing.perTaskSampleHitsArray;
    Template_M->Timing.perTaskSampleHits = (&mdlPerTaskSampleHits[0]);
    mdlSampleHits[0] = 1;
    Template_M->Timing.sampleHits = (&mdlSampleHits[0]);
  }

  //Timing Parameters
  rtmSetTFinal(Template_M, 30.0);			//Total runtime of the program
  Template_M->Timing.stepSize0 = 0.04;		//Period #1
  Template_M->Timing.stepSize1 = 1.0;		//Base Period
  //Refer to timing section in Template.h for more details.

  /* Setup for data logging */
  {
    static RTWLogInfo rt_DataLoggingInfo;
    rt_DataLoggingInfo.loggingInterval = NULL;
    Template_M->rtwLogInfo = &rt_DataLoggingInfo;
  }

  /* Setup for data logging */
  {
    rtliSetLogXSignalInfo(Template_M->rtwLogInfo, (NULL));
    rtliSetLogXSignalPtrs(Template_M->rtwLogInfo, (NULL));
    rtliSetLogT(Template_M->rtwLogInfo, "");
    rtliSetLogX(Template_M->rtwLogInfo, "");
    rtliSetLogXFinal(Template_M->rtwLogInfo, "");
    rtliSetLogVarNameModifier(Template_M->rtwLogInfo, "rt_");
    rtliSetLogFormat(Template_M->rtwLogInfo, 4);
    rtliSetLogMaxRows(Template_M->rtwLogInfo, 1000);
    rtliSetLogDecimation(Template_M->rtwLogInfo, 1);
    rtliSetLogY(Template_M->rtwLogInfo, "");
    rtliSetLogYSignalInfo(Template_M->rtwLogInfo, (NULL));
    rtliSetLogYSignalPtrs(Template_M->rtwLogInfo, (NULL));
  }

  Template_M->solverInfoPtr = (&Template_M->solverInfo);
  Template_M->Timing.stepSize = (0.04);
  rtsiSetFixedStepSize(&Template_M->solverInfo, 0.04);
  rtsiSetSolverMode(&Template_M->solverInfo, SOLVER_MODE_MULTITASKING);

  /* parameters */
  Template_M->ModelData.defaultParam = ((real_T *)&Template_P);

  /* child S-Function registration */
  {
    RTWSfcnInfo *sfcnInfo = &Template_M->NonInlinedSFcns.sfcnInfo;
    Template_M->sfcnInfo = (sfcnInfo);
    rtssSetErrorStatusPtr(sfcnInfo, (&rtmGetErrorStatus(Template_M)));
    rtssSetNumRootSampTimesPtr(sfcnInfo, &Template_M->Sizes.numSampTimes);
    Template_M->NonInlinedSFcns.taskTimePtrs[0] = &(rtmGetTPtr(Template_M)[0]);
    Template_M->NonInlinedSFcns.taskTimePtrs[1] = &(rtmGetTPtr(Template_M)[1]);
    rtssSetTPtrPtr(sfcnInfo,Template_M->NonInlinedSFcns.taskTimePtrs);
    rtssSetTStartPtr(sfcnInfo, &rtmGetTStart(Template_M));
    rtssSetTFinalPtr(sfcnInfo, &rtmGetTFinal(Template_M));
    rtssSetTimeOfLastOutputPtr(sfcnInfo, &rtmGetTimeOfLastOutput(Template_M));
    rtssSetStepSizePtr(sfcnInfo, &Template_M->Timing.stepSize);
    rtssSetStopRequestedPtr(sfcnInfo, &rtmGetStopRequested(Template_M));
    rtssSetDerivCacheNeedsResetPtr(sfcnInfo,
      &Template_M->ModelData.derivCacheNeedsReset);
    rtssSetZCCacheNeedsResetPtr(sfcnInfo,
      &Template_M->ModelData.zCCacheNeedsReset);
    rtssSetBlkStateChangePtr(sfcnInfo, &Template_M->ModelData.blkStateChange);
    rtssSetSampleHitsPtr(sfcnInfo, &Template_M->Timing.sampleHits);
    rtssSetPerTaskSampleHitsPtr(sfcnInfo, &Template_M->Timing.perTaskSampleHits);
    rtssSetSimModePtr(sfcnInfo, &Template_M->simMode);
    rtssSetSolverInfoPtr(sfcnInfo, &Template_M->solverInfoPtr);
  }

  Template_M->Sizes.numSFcns = (1);

  /* register each child */
  {
    (void) memset((void *)&Template_M->NonInlinedSFcns.childSFunctions[0], 0,
                  1*sizeof(SimStruct));
    Template_M->childSfunctions =
      (&Template_M->NonInlinedSFcns.childSFunctionPtrs[0]);
    Template_M->childSfunctions[0] =
      (&Template_M->NonInlinedSFcns.childSFunctions[0]);

    /* Level2 S-Function Block: Template/<Root>/Initialization (robot_init) */
    {
      SimStruct *rts = Template_M->childSfunctions[0];

      /* timing info */
      time_T *sfcnPeriod = Template_M->NonInlinedSFcns.Sfcn0.sfcnPeriod;
      time_T *sfcnOffset = Template_M->NonInlinedSFcns.Sfcn0.sfcnOffset;
      int_T *sfcnTsMap = Template_M->NonInlinedSFcns.Sfcn0.sfcnTsMap;
      (void) memset((void*)sfcnPeriod, 0,
                    sizeof(time_T)*1);
      (void) memset((void*)sfcnOffset, 0,
                    sizeof(time_T)*1);
      ssSetSampleTimePtr(rts, &sfcnPeriod[0]);
      ssSetOffsetTimePtr(rts, &sfcnOffset[0]);
      ssSetSampleTimeTaskIDPtr(rts, sfcnTsMap);

      /* Set up the mdlInfo pointer */
      {
        ssSetBlkInfo2Ptr(rts, &Template_M->NonInlinedSFcns.blkInfo2[0]);
      }

      ssSetRTWSfcnInfo(rts, Template_M->sfcnInfo);

      /* Allocate memory of model methods 2 */
      {
        ssSetModelMethods2(rts, &Template_M->NonInlinedSFcns.methods2[0]);
      }

      /* Allocate memory of model methods 3 */
      {
        ssSetModelMethods3(rts, &Template_M->NonInlinedSFcns.methods3[0]);
      }

      /* Allocate memory for states auxilliary information */
      {
        ssSetStatesInfo2(rts, &Template_M->NonInlinedSFcns.statesInfo2[0]);
        ssSetPeriodicStatesInfo(rts,
          &Template_M->NonInlinedSFcns.periodicStatesInfo[0]);
      }

      /* path info */
      ssSetModelName(rts, "Initialization");
      ssSetPath(rts, "Template/Initialization");
      ssSetRTModel(rts,Template_M);
      ssSetParentSS(rts, (NULL));
      ssSetRootSS(rts, rts);
      ssSetVersion(rts, SIMSTRUCT_VERSION_LEVEL2);

      /* parameters */
      {
        mxArray **sfcnParams = (mxArray **)
          &Template_M->NonInlinedSFcns.Sfcn0.params;
        ssSetSFcnParamsCount(rts, 1);
        ssSetSFcnParamsPtr(rts, &sfcnParams[0]);
        ssSetSFcnParam(rts, 0, (mxArray*)Template_P.Initialization_P1_Size);
      }

      /* registration */
      robot_init(rts);
      sfcnInitializeSizes(rts);
      sfcnInitializeSampleTimes(rts);

      /* adjust sample time */
      ssSetSampleTime(rts, 0, 1.0);
      ssSetOffsetTime(rts, 0, 0.0);
      sfcnTsMap[0] = 1;

      /* set compiled values of dynamic vector attributes */
      ssSetNumNonsampledZCs(rts, 0);

      /* Update connectivity flags for each port */
      /* Update the BufferDstPort flags for each input port */
    }
  }

  /* Initialize Sizes */
  Template_M->Sizes.numContStates = (0);/* Number of continuous states */
  Template_M->Sizes.numY = (0);        /* Number of model outputs */
  Template_M->Sizes.numU = (0);        /* Number of model inputs */
  Template_M->Sizes.sysDirFeedThru = (0);/* The model is not direct feedthrough */
  Template_M->Sizes.numSampTimes = (2);/* Number of sample times */
  Template_M->Sizes.numBlocks = (1);   /* Number of blocks */
  Template_M->Sizes.numBlockPrms = (3);/* Sum of parameter "widths" */
  return Template_M;
}

/*========================================================================*
 * End of Classic call interface                                          *
 *========================================================================*/
