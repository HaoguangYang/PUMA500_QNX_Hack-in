/* $Revision: 1.68 $
 * Copyright 1994-2002 The MathWorks, Inc.
 *
 * File    : qnx_main.c
 *
 * Abstract:
 *      A Generic "Real-Time (single tasking or pseudo-multitasking,
 *      statically allocated data)" main that runs under most
 *      operating systems.
 *
 *      This file may be a useful starting point when targeting a new
 *      processor or microcontroller.
 *
 * 3Oct04 -- added timer_delete to make sure timer is turned off; added
 *           ClockPeriod for finer clock resolution; allowed for base
 *           time samples to be >= 1.0 sec. RFKM
 *
 * Compiler specified defines:
 *	RT              - Required.
 *	MODEL=modelname - Required.
 *	NUMST=#         - Required. Number of sample times.
 *	NCSTATES=#      - Required. Number of continuous states.
 *	TID01EQ=1 or 0  - Optional. Only define to 1 if sample time task
 *                    id's 0 and 1 have equal rates.
 *	MULTITASKING    - Optional. (use MT for a synonym).
 *	SAVEFILE        - Optional (non-quoted) name of .mat file to create.
 *		          	  Default is <MODEL>.mat
 *	BORLAND         - Required if using Borland C/C++
 */

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* START WATCHDOG */
#include <sys/types.h>
#include <sys/neutrino.h>
#include <pthread.h>
/* END WATCHDOG */

#include "tmwtypes.h"
#include "rtmodel.h"
#include "rt_sim.h"
#include "rt_logging.h"
#include "rt_nonfinite.h"

/* Signal Handler header */
#ifdef BORLAND
#include <signal.h>
#include <float.h>
#endif

#include "ext_work.h"

/*=========*
 * Defines *
 *=========*/

#ifndef TRUE
#define FALSE (0)
#define TRUE  (1)
#endif

#ifndef EXIT_FAILURE
#define EXIT_FAILURE  1
#endif
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS  0
#endif

#define QUOTE1(name) #name
#define QUOTE(name) QUOTE1(name)    /* need to expand name    */

#ifndef RT
# error "must define RT"
#endif

#ifndef MODEL
# error "must define MODEL"
#endif

#ifndef NUMST
# error "must define number of sample times, NUMST"
#endif

#ifndef NCSTATES
# error "must define NCSTATES"
#endif

#ifndef SAVEFILE
# define MATFILE2(file) #file ".mat"
# define MATFILE1(file) MATFILE2(file)
# define MATFILE MATFILE1(MODEL)
#else
# define MATFILE QUOTE(SAVEFILE)
#endif

#define RUN_FOREVER -1.0

#define EXPAND_CONCAT(name1,name2) name1 ## name2
#define CONCAT(name1,name2) EXPAND_CONCAT(name1,name2)
#define RT_MODEL            CONCAT(MODEL,_rtModel)

#define NSEC_PER_SEC 1000000000

/*====================*
 * External functions *
 *====================*/
extern RT_MODEL *MODEL(void);

extern void MdlInitializeSizes(void);
extern void MdlInitializeSampleTimes(void);
extern void MdlStart(void);
extern void MdlOutputs(int_T tid);
extern void MdlUpdate(int_T tid);
extern void MdlTerminate(void);

#if NCSTATES > 0
  extern void rt_ODECreateIntegrationData(RTWSolverInfo *si);
  extern void rt_ODEUpdateContinuousStates(RTWSolverInfo *si);

# define rt_CreateIntegrationData(S) \
    rt_ODECreateIntegrationData(rtmGetRTWSolverInfo(S));
# define rt_UpdateContinuousStates(S) \
    rt_ODEUpdateContinuousStates(rtmGetRTWSolverInfo(S));
# else
# define rt_CreateIntegrationData(S)  \
      rtsiSetSolverName(rtmGetRTWSolverInfo(S),"FixedStepDiscrete");
# define rt_UpdateContinuousStates(S) \
      rtmSetT(S, rtsiGetSolverStopTime(rtmGetRTWSolverInfo(S)));
#endif


/*==================================*
 * Global data local to this module *
 *==================================*/

static struct {
  int_T    stopExecutionFlag;
  int_T    isrOverrun;
  int_T    overrunFlags[NUMST];
  const char_T *errmsg;
} GBLbuf;

/* START WATCHDOG */
pthread_mutex_t wd_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t wd_cond = PTHREAD_COND_INITIALIZER;
struct sigevent event;
int timer_chid;
timer_t timer_id;
struct itimerspec itime;
#define TIMER_PULSE_CODE _PULSE_CODE_MINAVAIL
/* END WATCHDOG */

#ifdef EXT_MODE
#  define rtExtModeSingleTaskUpload(S)    \
   {                                      \
        int stIdx;                        \
        rtExtModeUploadCheckTrigger();    \
        for (stIdx=0; stIdx<NUMST; stIdx++) {                   \
            if (rtmIsSampleHit(S, stIdx, 0 /*unused*/)) {       \
                rtExtModeUpload(stIdx,rtmGetTaskTime(S,stIdx)); \
            }                                                   \
        }                                                       \
   }
#else
#  define rtExtModeSingleTaskUpload(S) /* Do nothing */
#endif

/*=================*
 * Local functions *
 *=================*/

#ifdef BORLAND
/* Implemented for BC++ only*/

typedef void (*fptr)(int, int);

/* Function: divideByZero =====================================================
 *
 * Abstract: Traps the error Division by zero and prints a warning
 *           Also catches other FP errors, but does not identify them
 *           specifically.
 */
void divideByZero(int sigName, int sigType)
{
    signal(SIGFPE, (fptr)divideByZero);
    if ((sigType == FPE_ZERODIVIDE)||(sigType == FPE_INTDIV0)){
        printf("*** Warning: Division by zero\n\n");
        return;
    }
    else{
        printf("*** Warning: Floating Point error\n\n");
        return;
    }
} /* end divideByZero */

#endif /* BORLAND */

#if !defined(MULTITASKING)  /* SINGLETASKING */

/* Function: rtOneStep ========================================================
 *
 * Abstract:
 *      Perform one step of the model. This function is modeled such that
 *      it could be called from an interrupt service routine (ISR) with minor
 *      modifications.
 */
static void rt_OneStep(RT_MODEL *S)
{
    real_T tnext;
 
    /* START WATCHDOG */
    int pid;
    struct _pulse msg;
    /* END WATCHDOG */

	/* START WATCHDOG */
	for (;;) {
		pid = MsgReceive(timer_chid, &msg, sizeof(msg), NULL);
		if (pid == 0) {
			if (msg.code == TIMER_PULSE_CODE) {

				/***********************************************
				 * Check and see if error status has been set  *
				 ***********************************************/

			    if (rtmGetErrorStatus(S) != NULL) {
			        GBLbuf.stopExecutionFlag = 1;
			        return;
			    }

			    /***********************************************
				* Check and see if base step time is too fast *
				***********************************************/

				if (GBLbuf.isrOverrun++) {
					GBLbuf.stopExecutionFlag = 1;
					return;
				}

				rtExtModeOneStep(rtmGetRTWExtModeInfo(S),
					         rtmGetNumSampleTimes(S),
						 (boolean_T *)&rtmGetStopRequested(S));

			    tnext = rt_SimGetNextSampleHit();
				rtsiSetSolverStopTime(rtmGetRTWSolverInfo(S),tnext);

			    MdlOutputs(0);

			    rtExtModeSingleTaskUpload(S);

			    GBLbuf.errmsg = rt_UpdateTXYLogVars(rtmGetRTWLogInfo(S),
                                        rtmGetTPtr(S));
				if (GBLbuf.errmsg != NULL) {
    					GBLbuf.stopExecutionFlag = 1;
    					return;
				}

			    MdlUpdate(0);

				rt_SimUpdateDiscreteTaskSampleHits(rtmGetNumSampleTimes(S),
                                       rtmGetTimingData(S),
                                       rtmGetSampleHitPtr(S),
                                       rtmGetTPtr(S));

			    if (rtmGetSampleTime(S,0) == CONTINUOUS_SAMPLE_TIME) {
					rt_UpdateContinuousStates(S);
				}

			    GBLbuf.isrOverrun--;

			    rtExtModeCheckEndTrigger();
				pthread_cond_signal(&wd_cond);
			/* START WATCHDOG */
			}  /* end of if loop */
		}  /* end of if loop */
	}  /* end of continuous for loop */
	/* END WATCHDOG */

} /* end rtOneStep */

#else /* MULTITASKING */

# if TID01EQ == 1
#  define FIRST_TID 1
# else
#  define FIRST_TID 0
# endif

/* Function: rtOneStep ========================================================
 *
 * Abstract:
 *      Perform one step of the model. This function is modeled such that
 *      it could be called from an interrupt service routine (ISR) with minor
 *      modifications.
 *
 *      This routine is modeled for use in a multitasking environment and
 *		therefore needs to be fully re-entrant when it is called from an
 *		interrupt service routine.
 *
 * Note:
 *      Error checking is provided which will only be used if this routine
 *      is attached to an interrupt.
 *
 */
static void rt_OneStep(RT_MODEL *S)
{    
    int_T  eventFlags[NUMST];
    int_T  i;
    real_T tnext;
    int_T *sampleHit = rtmGetSampleHitPtr(S);

    /* START WATCHDOG */
    int pid;
    struct _pulse msg;
    /* END WATCHDOG */

    /* enable interrupts here */
	/* START WATCHDOG */
	for (;;) {
		pid = MsgReceive(timer_chid, &msg, sizeof(msg), NULL);
		if (pid == 0) {
			if (msg.code == TIMER_PULSE_CODE) {

			    /***********************************************
  				 * Check and see if base step time is too fast *
     			***********************************************/

			    if (GBLbuf.isrOverrun++) {
					GBLbuf.stopExecutionFlag = 1;
					return;
				}

			    /***********************************************
			     * Check and see if error status has been set  *
 			    ***********************************************/

			    if (rtmGetErrorStatus(S) != NULL) {
			        GBLbuf.stopExecutionFlag = 1;
			        return;
			    }

			    rtExtModeOneStep(rtmGetRTWExtModeInfo(S), rtGetNumSampleTimes(S),
                     (boolean_T *)&rtmGetStopRequested(S));

			    /************************************************************************
			     * Update discrete events and buffer event flags locally so that ISR is *
			     * re-entrant.                                                          *
			     ************************************************************************/

			    tnext = rt_SimUpdateDiscreteEvents(rtmGetNumSampleTimes(S),
                                       rtmGetTimingData(S),
                                       rtmGetSampleHitPtr(S),
                                       rtmGetPerTaskSampleHitsPtr(S));
			    rtsiSetSolverStopTime(rtmGetRTWSolverInfo(S),tnext);
			    for (i=FIRST_TID+1; i < NUMST; i++) {
					eventFlags[i] = sampleHit[i];
				}

			    /*******************************************
			     * Step the model for the base sample time *
			     *******************************************/

			    MdlOutputs(FIRST_TID);

			    rtExtModeUploadCheckTrigger(rtmGetNumSampleTimes(S));
			    rtExtModeUpload(FIRST_TID,rtmGetTaskTime(S, FIRST_TID));

			    GBLbuf.errmsg = rt_UpdateTXYLogVars(rtmGetRTWLogInfo(S),
                                        rtmGetTPtr(S));
			    if (GBLbuf.errmsg != NULL) {
			        GBLbuf.stopExecutionFlag = 1;
			        return;
			    }

			    MdlUpdate(FIRST_TID);

			    if (rtmGetSampleTime(S,0) == CONTINUOUS_SAMPLE_TIME) {
			        rt_UpdateContinuousStates(S);
			    }
			    else {
			        rt_SimUpdateDiscreteTaskTime(rtmGetTPtr(S), 
                                     rtmGetTimingData(S), 0);
			    }

#if FIRST_TID == 1
			    rt_SimUpdateDiscreteTaskTime(rtmGetTPtr(S), 
                                 rtmGetTimingData(S),1);
#endif

				GBLbuf.isrOverrun--;

				/*********************************************
				 * Step the model for any other sample times *
				 *********************************************/

				for (i=FIRST_TID+1; i<NUMST; i++) {
					if (eventFlags[i]) {
						if (GBLbuf.overrunFlags[i]++) {  /* Are we sampling too fast for */
							GBLbuf.stopExecutionFlag=1;  /*   sample time "i"?           */
							return;
						}

						MdlOutputs(i);

						rtExtModeUpload(i, rtmGetTaskTime(S,i));

						MdlUpdate(i);

						rt_SimUpdateDiscreteTaskTime(rtmGetTPtr(S), rtmGetTimingData(S),i);

						/* Indicate task complete for sample time "i" */
						GBLbuf.overrunFlags[i]--;
					}
				}

				rtExtModeCheckEndTrigger();    
			/* START WATCHDOG */
			pthread_cond_signal(&wd_cond);
			}  /* end of if loop */
		}  /* end of if loop */
	}  /* end of continuous for loop */
	/* END WATCHDOG */
	
} /* end rtOneStep */

#endif /* MULTITASKING */


static void displayUsage (void)
{
    (void) printf("usage: %s -tf <finaltime> -w -port <TCPport>\n",QUOTE(MODEL));
    (void) printf("arguments:\n");
    (void) printf("  -tf <finaltime> - overrides final time specified in "
                  "Simulink (inf for no limit).\n");
    (void) printf("  -w              - waits for Simulink to start model "
                  "in External Mode.\n");
    (void) printf("  -port <TCPport> - overrides 17725 default port in "
                  "External Mode, valid range 256 to 65535.\n");
}

/*===================*
 * Visible functions *
 *===================*/

/* Function: main =============================================================
 *
 * Abstract:
 *      Execute model on a generic target such as a workstation.
 */
int_T main(int_T argc, const char_T *argv[])
{
    RT_MODEL  *S;
    const char *status;
    real_T     finaltime = -2.0;

    int_T  oldStyle_argc;
    const char_T *oldStyle_argv[5];

	/* START WATCHDOG */
	double sec;
	long nsec;
	pthread_attr_t attrib;
	struct sched_param param;
	struct sched_param our_param;
	pthread_t tid;
	int i;
    struct timespec res;
    struct _clockperiod *new;
	/* END WATCHDOG */

    /******************************
     * MathError Handling for BC++ *
     ******************************/
#ifdef BORLAND
    signal(SIGFPE, (fptr)divideByZero);
#endif

    /*******************
     * Parse arguments *
     *******************/

    if ((argc > 1) && (argv[1][0] != '-')) {
        /* old style */
        if ( argc > 3 ) {
            displayUsage();
            exit(EXIT_FAILURE);
        }

        oldStyle_argc    = 1;
        oldStyle_argv[0] = argv[0];
    
        if (argc >= 2) {
            oldStyle_argc = 3;

            oldStyle_argv[1] = "-tf";
            oldStyle_argv[2] = argv[1];
        }

        if (argc == 3) {
            oldStyle_argc = 5;

            oldStyle_argv[3] = "-port";
            oldStyle_argv[4] = argv[2];

        }

        argc = oldStyle_argc;
        argv = oldStyle_argv;

    }

	{
        /* new style: */
        double    tmpDouble;
        char_T tmpStr2[200];
        int_T  count      = 1;
        int_T  parseError = FALSE;

        /*
         * Parse the standard RTW parameters.  Let all unrecognized parameters
         * pass through to external mode for parsing.  NULL out all args handled
         * so that the external mode parsing can ignore them.
         */
        while(count < argc) {
            const char_T *option = argv[count++];
            
            /* final time */
            if ((strcmp(option, "-tf") == 0) && (count != argc)) {
                const char_T *tfStr = argv[count++];
                
                sscanf(tfStr, "%200s", tmpStr2);
                if (strcmp(tmpStr2, "inf") == 0) {
                    tmpDouble = RUN_FOREVER;
                } else {
                    char_T tmpstr[2];

                    if ( (sscanf(tmpStr2,"%lf%1s", &tmpDouble, tmpstr) != 1) ||
                         (tmpDouble < 0.0) ) {
                        (void)printf("finaltime must be a positive, real value or inf\n");
                        parseError = TRUE;
                        break;
                    }
                }
                finaltime = (real_T) tmpDouble;

                argv[count-2] = NULL;
                argv[count-1] = NULL;
            }
        }

        if (parseError) {
            (void)printf("\nUsage: %s -option1 val1 -option2 val2 -option3 "
                         "...\n\n", QUOTE(MODEL));
            (void)printf("\t-tf 20 - sets final time to 20 seconds\n");

            exit(EXIT_FAILURE);
        }

        rtExtModeParseArgs(argc, argv, NULL);

        /*
         * Check for unprocessed ("unhandled") args.
         */
        {
            int i;
            for (i=1; i<argc; i++) {
                if (argv[i] != NULL) {
                    printf("Unexpected command line argument: %s\n",argv[i]);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    /****************************
     * Initialize global memory *
     ****************************/
    (void)memset(&GBLbuf, 0, sizeof(GBLbuf));

    /************************
     * Initialize the model *
     ************************/
    rt_InitInfAndNaN(sizeof(real_T));

    S = MODEL();
    if (rtmGetErrorStatus(S) != NULL) {
        (void)fprintf(stderr,"Error during model registration: %s\n",
                      rtmGetErrorStatus(S));
        exit(EXIT_FAILURE);
    }
    if (finaltime >= 0.0 || finaltime == RUN_FOREVER) rtmSetTFinal(S,finaltime);

    MdlInitializeSizes();

    MdlInitializeSampleTimes();
    
    status = rt_SimInitTimingEngine(rtmGetNumSampleTimes(S),
                                    rtmGetStepSize(S),
                                    rtmGetSampleTimePtr(S),
                                    rtmGetOffsetTimePtr(S),
                                    rtmGetSampleHitPtr(S),
                                    rtmGetSampleTimeTaskIDPtr(S),
                                    rtmGetTStart(S),
                                    &rtmGetSimTimeStep(S),
                                    &rtmGetTimingData(S));

    if (status != NULL) {
        (void)fprintf(stderr,
                "Failed to initialize sample time engine: %s\n", status);
        exit(EXIT_FAILURE);
    }
    rt_CreateIntegrationData(S);

    GBLbuf.errmsg = rt_StartDataLogging(rtmGetRTWLogInfo(S),
                                        rtmGetTFinal(S),
                                        rtmGetStepSize(S),
                                        &rtmGetErrorStatus(S));
    if (GBLbuf.errmsg != NULL) {
        (void)fprintf(stderr,"Error starting data logging: %s\n",GBLbuf.errmsg);
        return(EXIT_FAILURE);
    }

    rtExtModeCheckInit(rtGetNumSampleTimes(S));
    rtExtModeWaitForStartPkt(rtmGetRTWExtModeInfo(S), rtmGetNumSampleTimes(S),
                             (boolean_T *)&rtmGetStopRequested(S));

    (void)printf("\n** starting the model **\n");

    MdlStart();
    if (rtmGetErrorStatus(S) != NULL) {
      GBLbuf.stopExecutionFlag = 1;
    }

    /*************************************************************************
     * Execute the model.  You may attach rtOneStep to an ISR, if so replace *
     * the call to rtOneStep (below) with a call to a background task        *
     * application.                                                          *
     *************************************************************************/

    if (rtmGetTFinal(S) == RUN_FOREVER) {
        printf ("\n**May run forever. Model stop time set to infinity.**\n");
    }

	/* START WATCHDOG */
	new = (struct _clockperiod *)malloc(sizeof(struct _clockperiod));
	new->nsec = 100000L;
	new->fract = 0L;
	ClockPeriod(CLOCK_REALTIME,new,NULL,0);
	free(new);
    if ( clock_getres( CLOCK_REALTIME, &res) == -1 ) {
      perror( "clock get resolution" );
      return EXIT_FAILURE;
      }
    printf( "Clock resolution is %ld microseconds.\n",res.tv_nsec/1000);
	printf("sample time: %6.4f\n", (float) rtmGetSampleTime(S,TID01EQ));
	sec = (double) floorf( (float) rtmGetSampleTime(S,TID01EQ));
	nsec = fmod(rtmGetSampleTime(S,TID01EQ),1.0) * NSEC_PER_SEC;
	printf("seconds for task %d: %d\n", TID01EQ, (int) sec);
	printf("nanoseconds for task %d: %d\n", TID01EQ, nsec);
    timer_chid = ChannelCreate(0);
	event.sigev_notify = SIGEV_PULSE;
	event.sigev_coid = ConnectAttach(0,0,timer_chid,_NTO_SIDE_CHANNEL,0);
	event.sigev_priority = getprio(0);
	event.sigev_code = TIMER_PULSE_CODE;
	timer_create(CLOCK_REALTIME, &event, &timer_id);

	/* set timer */
	itime.it_value.tv_sec = sec;
	itime.it_value.tv_nsec = nsec;

	/* set to non-zero for cyclical timer */
	itime.it_interval.tv_sec = sec;
	itime.it_interval.tv_nsec = nsec;

	/* start timer */
	timer_settime(timer_id, 0, &itime, NULL);
	
	pthread_attr_init(&attrib);
	pthread_attr_setinheritsched(&attrib,PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attrib,SCHED_RR);
	sched_getparam(0,&our_param);
	param.sched_priority = our_param.sched_priority;
	pthread_attr_setschedparam(&attrib, &param);

	if (pthread_create(&tid, &attrib, rt_OneStep, (void *) S) < 0) {
		printf("rt_OneStep routine spawn failed\n");
	}

	while (!GBLbuf.stopExecutionFlag && (rtmGetTFinal(S) == RUN_FOREVER || rtmGetTFinal(S)-rtmGetT(S) > rtmGetT(S)*DBL_EPSILON)) {

		rtExtModePauseIfNeeded(rtmGetRTWExtModeInfo(S), rtmGetNumSampleTimes(S), (boolean_T *)&rtmGetStopRequested(S));
		if (rtmGetStopRequested(S)) break;

		pthread_mutex_lock(&wd_mutex);
		pthread_cond_wait(&wd_cond,&wd_mutex);
		pthread_mutex_unlock(&wd_mutex);
	}

    if (!GBLbuf.stopExecutionFlag && !rtmGetStopRequested(S)) {    
        /* Execute model last time step */
		pthread_mutex_lock(&wd_mutex);
		pthread_cond_wait(&wd_cond,&wd_mutex);
		pthread_mutex_unlock(&wd_mutex);
    }

	timer_delete(timer_id);
	printf("done\n");
	
    /********************
     * Cleanup and exit *
     ********************/

    rt_StopDataLogging(MATFILE,rtmGetRTWLogInfo(S));

    rtExtModeShutdown(rtmGetNumSampleTimes(S));

    if (GBLbuf.errmsg) {
        (void)fprintf(stderr,"%s\n",GBLbuf.errmsg);
        exit(EXIT_FAILURE);
    }

    if (GBLbuf.isrOverrun) {
        (void)fprintf(stderr,
                      "%s: ISR overrun - base sampling rate is too fast\n",
                      QUOTE(MODEL));
        exit(EXIT_FAILURE);
    }

    if (rtmGetErrorStatus(S) != NULL) {
        (void)fprintf(stderr,"%s\n", rtmGetErrorStatus(S));
        exit(EXIT_FAILURE);
    }
#ifdef MULTITASKING
    else {
        int_T i;
        for (i=1; i<NUMST; i++) {
            if (GBLbuf.overrunFlags[i]) {
                (void)fprintf(stderr,
                        "%s ISR overrun - sampling rate is too fast for "
                        "sample time index %d\n", QUOTE(MODEL), i);
                exit(EXIT_FAILURE);
            }
        }
    }
#endif

    MdlTerminate();
    return(EXIT_SUCCESS);

} /* end main */



/* EOF: qnx_main.c */
