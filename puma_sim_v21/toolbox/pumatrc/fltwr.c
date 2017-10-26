/*
 * fltwr.c: float read block for generic control board
 *
 */


/*
 * You must specify the S_FUNCTION_NAME as the name of your S-function
 * (i.e. replace sfuntmpl_basic with the name of your S-function).
 */

#define S_FUNCTION_NAME fltwr
#define S_FUNCTION_LEVEL 2

/*
 * Need to include simstruc.h for the definition of the SimStruct and
 * its associated macro definitions.
 */
#include "simstruc.h"

#define OPT_IDX 0
#define OPT_PARAM(S) ssGetSFcnParam(S, OPT_IDX)

#define CHAN_IDX 1
#define CHAN_PARAM(S) ssGetSFcnParam(S, CHAN_IDX)

#define TIME_IDX 2
#define TIME_PARAM(S) ssGetSFcnParam(S, TIME_IDX)

#if defined(RT)
/* include files for real time operation */
#include "qnx_robot.h"

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
	int_T option;

	ssSetNumSFcnParams(S, 3);	/* Number of expected parameters */
	if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
		/* Return if number of expected != number of actual parameters */
		return;
	}
	// get option from parameters
	option = (int_T) (mxGetScalar(OPT_PARAM(S))) - 1;

	ssSetNumContStates(S, 0);
	ssSetNumDiscStates(S, 0);

	if (!ssSetNumInputPorts(S, 1)) return;
#if defined(RT)
	switch (option) {
	case TORQUE6:
	case POSITION6:
	case POT6:
	case INDEX6:
		ssSetInputPortWidth(S, 0, 6);
		break;
	default:
		ssSetInputPortWidth(S, 0, 1);
	}
#elif defined(MATLAB_MEX_FILE)
	switch (option) {
	case 1:
	case 4:
	case 7:
	case 9:
		ssSetInputPortWidth(S, 0, 6);
		break;
	default:
		ssSetInputPortWidth(S, 0, 1);
	}
#endif
	if (!ssSetNumOutputPorts(S,0)) return;

	ssSetNumSampleTimes(S, 1);
	ssSetNumRWork(S, 0);
	ssSetNumIWork(S, 2);
	ssSetNumPWork(S, 0);
	ssSetNumModes(S, 0);
	ssSetNumNonsampledZCs(S, 0);

	ssSetOptions(S, 0);
}

/* Function: mdlInitializeSampleTimes =========================================
 * Abstract:
 *    This function is used to specify the sample time(s) for your
 *    S-function. You must register the same number of sample times as
 *    specified in ssSetNumSampleTimes.
 */
static void mdlInitializeSampleTimes(SimStruct *S)
{
	real_T time;

	// get sample time from parameters
	time = (real_T) (*mxGetPr(TIME_PARAM(S)));

	if (time == -1)
	ssSetSampleTime(S, 0, DYNAMICALLY_SIZED);
	else
		ssSetSampleTime(S, 0, time);
		ssSetOffsetTime(S, 0, 0.0);
}

#define MDL_START
#if defined(MDL_START)
/* Function: mdlStart ===========================================================
 * Abstract:
 *		This function is called once at start of model execution. If you
 *		have states that should be initialized once, this is the place
 *		to do it.
 */
static void mdlStart(SimStruct *S)
{
	int_T option, channel;
	int_T *IWork = ssGetIWork(S);

	// get option from parameters
	option = (int_T) (mxGetScalar(OPT_PARAM(S))) - 1;
	IWork[0] = option;

	// get channel from parameters
	channel = (int_T) (*mxGetPr(CHAN_PARAM(S)));
	IWork[1] = channel;
}
#endif // MDL_START

/* Function: mdlOutputs =======================================================
 * Abstract:
 *		In this function, you compute the outputs of your S-function
 *		block. Generally outputs are placed in the output vector, ssGetY(S).
 */
static void mdlOutputs(SimStruct *S, int_T tid)
{
	int_T i;
	int_T option = ssGetIWorkValue(S,0);
	int_T channel = ssGetIWorkValue(S,1);
	float tmpArray[6];
	float tmp;
	InputRealPtrsType val = ssGetInputPortRealSignalPtrs(S,0);

#if defined(RT)
	/* generated code calls function to initialize the servotogo driver */
	switch(option) {
	case TORQUE6: // TORQUE6 -- write all encoders
		for (i=0;i<6;i++)
			tmpArray[i] = (float) *val[i];
		robot_fltwr(TORQUE6, channel, tmpArray);
		break;
	case TORQUE: // TORQUE
		tmp = (float) *val[0];
		robot_fltwr(TORQUE, channel, &tmp);
		break;
	case VOLTAGE: // VOLTAGE
		tmp = (float) *val[0];
		robot_fltwr(VOLTAGE, channel, &tmp);
		break;
	default:
		printf("float write error: option failed\n");
	}

#elif defined(MATLAB_MEX_FILE)
	/* during simulation, just print a message */
	if (ssGetSimMode(S) == SS_SIMMODE_NORMAL) {
		mexPrintf("\nfloat write: mdlOutput\n");
	}
#endif
}

/* Function: mdlTerminate =====================================================
 * Abstract:
 *    In this function, you should perform any actions that are necessary
 *    at the termination of a simulation.	For example, if memory was
 *    allocated in mdlStart, this is the place to free it.
 */
static void mdlTerminate(SimStruct *S)
{
#if defined(RT)
	/* generated code calls function to initialize the servotogo driver */
	printf("\nfloat write module: termination\n");
	ssSetIWorkValue(S,0,NULL);
	ssSetIWorkValue(S,1,NULL);
#elif defined(MATLAB_MEX_FILE)
	/* during simulation, just print a message */
	if (ssGetSimMode(S) == SS_SIMMODE_NORMAL) {
		mexPrintf("\nfloat write: Simulated termination\n");
	}
#endif
}


/*======================================================*
 * See sfuntmpl_doc.c for the optional S-function methods *
 *======================================================*/

/*=============================*
 * Required S-function trailer *
 *=============================*/

#ifdef	MATLAB_MEX_FILE	   /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif