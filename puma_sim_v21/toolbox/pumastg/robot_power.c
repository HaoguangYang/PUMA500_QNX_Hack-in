/*
 * robot_power.c: turn on power for robot
 *
 */


/*
 * You must specify the S_FUNCTION_NAME as the name of your S-function
 * (i.e. replace sfuntmpl_basic with the name of your S-function).
 */

#define S_FUNCTION_NAME  robot_power
#define S_FUNCTION_LEVEL 2

/*
 * Need to include simstruc.h for the definition of the SimStruct and
 * its associated macro definitions.
 */
#include "simstruc.h"

#if defined(RT)
#include "qnx_robotstg.h"
#endif

/*====================*
 * S-function methods *
 *====================*/

/* Function: mdlInitializeSizes ===============================================
 * Abstract:
 *    The sizes information is used by Simulink to determine the S-function
 *    block's characteristics (number of inputs, outputs, states, etc.).
 */
static void mdlInitializeSizes(SimStruct *S)
{
  ssSetNumSFcnParams(S, 0);  /* Number of expected parameters */
  if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
	/* Return if number of expected != number of actual parameters */
	return;
  }
  
  ssSetNumContStates(S, 0);
  ssSetNumDiscStates(S, 0);
  
  if (!ssSetNumInputPorts(S, 0)) return;
  if (!ssSetNumOutputPorts(S, 0)) return;
  
  ssSetNumSampleTimes(S, 1);
  ssSetNumRWork(S, 0);
  ssSetNumIWork(S, 0);
  ssSetNumPWork(S, 0);
  ssSetNumModes(S, 0);
  ssSetNumNonsampledZCs(S, 0);
  
  ssSetOptions(S, 0);
  
  //do this here to guarentee it runs before the start functions
  // of any other block
#if defined(RT)
  robot_control(POWER,0,1);
  printf("\nrobot_power: turning power on\n");
#elif defined(MATLAB_MEX_FILE)
  /* during simulation, just print a message */
  if (ssGetSimMode(S) == SS_SIMMODE_NORMAL) {
	mexPrintf("\nrobot_power: turning power on\n");
  }
#endif
}

/* Function: mdlInitializeSampleTimes =========================================
 * Abstract:
 *    This function is used to specify the sample time(s) for your
 *    S-function. You must register the same number of sample times as
 *    specified in ssSetNumSampleTimes.
 */
static void mdlInitializeSampleTimes(SimStruct *S)
{
  // this module only needs to initialize the board
  // and does not need to be executed at any rate.
  // Thus, the sampletime is trivial.
  ssSetSampleTime(S, 0, 1.0);
  ssSetOffsetTime(S, 0, 0.0);  
}

/* Function: mdlOutputs =======================================================
 * Abstract:
 *    In this function, you compute the outputs of your S-function
 *    block. Generally outputs are placed in the output vector, ssGetY(S).
 */
static void mdlOutputs(SimStruct *S, int_T tid)
{ // do nothing
}

/* Function: mdlTerminate =====================================================
 * Abstract:
 *    In this function, you should perform any actions that are necessary
 *    at the termination of a simulation.  For example, if memory was
 *    allocated in mdlStart, this is the place to free it.
 */
static void mdlTerminate(SimStruct *S)
{
#if defined(RT)
  /* generated code calls function to initialize the servotogo driver */
  robot_control(POWER,0,0);
  printf("\nrobot_power: turning power off\n");
#elif defined(MATLAB_MEX_FILE)
  /* during simulation, just print a message */
  if (ssGetSimMode(S) == SS_SIMMODE_NORMAL) {
	mexPrintf("\nrobot_init: turning power off\n");
  }
#endif
}

/*======================================================*
 * See sfuntmpl_doc.c for the optional S-function methods *
 *======================================================*/

/*=============================*
 * Required S-function trailer *
 *=============================*/

#ifdef  MATLAB_MEX_FILE    /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif









