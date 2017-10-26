/*
 * flog.c: logs inputs to a file named as a parameter
 *
 */


/*
 * You must specify the S_FUNCTION_NAME as the name of your S-function
 * (i.e. replace sfuntmpl_basic with the name of your S-function).
 */

#define S_FUNCTION_NAME flog
#define S_FUNCTION_LEVEL 2

/*
 * Need to include simstruc.h for the definition of the SimStruct and
 * its associated macro definitions.
 */
#include "simstruc.h"

#define FILE_IDX 0
#define FILE_PARAM(S) ssGetSFcnParam(S,FILE_IDX)

#define TIME_IDX 1
#define TIME_PARAM(S) ssGetSFcnParam(S,TIME_IDX)

#if defined(RT)
/* include files for real time operation */

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
	ssSetNumSFcnParams(S, 2);	/* Number of expected parameters */
	if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
	/* Return if number of expected != number of actual parameters */
	return;
	}

	ssSetNumContStates(S, 0);
	ssSetNumDiscStates(S, 0);

	if (!ssSetNumInputPorts(S, 1)) return;
	ssSetInputPortWidth(S, 0, DYNAMICALLY_SIZED);

	if (!ssSetNumOutputPorts(S, 0)) return;

	ssSetNumSampleTimes(S, 1);
	ssSetNumRWork(S, 0);
	ssSetNumIWork(S, 0);
	ssSetNumPWork(S, 1);
	ssSetNumModes(S, 0);
	ssSetNumNonsampledZCs(S, 0);

	ssSetOptions(S, 0);
}

#define MDL_SET_INPUT_PORT_WIDTH
static void mdlSetInputPortWidth(SimStruct *S, int_T port, int_T inputPortWidth)
{
	ssSetInputPortWidth(S,port,inputPortWidth);
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
		ssSetSampleTime(S,0,INHERITED_SAMPLE_TIME);
	else
		ssSetSampleTime(S, 0, time);
	ssSetOffsetTime(S, 0, 0.0);
}

#define MDL_START
#if defined(MDL_START)
/* Function: mdlStart ===========================================================
 * Abstract:
 *    This function is called once at start of model execution. If you
 *    have states that should be initialized once, this is the place
 *    to do it.
 */
static void mdlStart(SimStruct *S)
{
	int_T len;
	char_T *fileName,fName[32];
	FILE *fptr;
	void **PWork = ssGetPWork(S);

	// get filename from parameters
	len = mxGetNumberOfElements(FILE_PARAM(S));
	fileName = (char_T*) malloc(len+1);
	mxGetString(FILE_PARAM(S), fileName, len+1);
	// append and open
	strcpy(fName,fileName);
	strcat(fName, ".txt");
	printf("opening log file %s...\n", fName);
	fptr = fopen(fName,"w");
	free(fileName);

	PWork[0] = fptr;
}
#endif // MDL_START

/* Function: mdlOutputs =======================================================
 * Abstract:
 *    In this function, you compute the outputs of your S-function
 *    block. Generally outputs are placed in the output vector, ssGetY(S).
 */
static void mdlOutputs(SimStruct *S, int_T tid)
{
	InputRealPtrsType var = ssGetInputPortRealSignalPtrs(S,0);
	int_T width = ssGetInputPortWidth(S,0);
	int_T i;
	FILE *fptr;

#if defined(RT)
	fptr = (FILE *) ssGetPWorkValue(S,0);
	for (i=0;i<width;i++)
		fprintf(fptr,"%9.4f ", (float) *var[i]);
	fprintf(fptr,"\n");
#elif defined(MATLAB_MEX_FILE)
	for (i=0;i<width;i++)
		printf("%9.4f ", (float) *var[i]);
	printf("\n");
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
	FILE *fptr;
#if defined(RT)
	/* generated code calls function to initialize the servotogo driver */
	fptr = (FILE *) ssGetPWorkValue(S,0);
	fclose(fptr);
	printf("\nfile logging module: termination\n");
	ssSetPWorkValue(S,0,NULL);
#elif defined(MATLAB_MEX_FILE)
	/* during simulation, just print a message */
	if (ssGetSimMode(S) == SS_SIMMODE_NORMAL) {
		mexPrintf("\nflog: Simulated termination\n");
	}
#endif
}


/*======================================================*
 * See sfuntmpl_doc.c for the optional S-function methods *
 *======================================================*/

/*=============================*
 * Required S-function trailer *
 *=============================*/

#ifdef MATLAB_MEX_FILE    /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif