/*
 * trajq.c: trajectory generator for generic robot. Uses 3rd degree
 *         polynomial.
 *
 *
 */


/*
 * You must specify the S_FUNCTION_NAME as the name of your S-function
 * (i.e. replace sfuntmpl_basic with the name of your S-function).
 */

#define S_FUNCTION_NAME trajq
#define S_FUNCTION_LEVEL 2

/*
 * Need to include simstruc.h for the definition of the SimStruct and
 * its associated macro definitions.
 */
#include "simstruc.h"

#define TIME_IDX 0
#define TIME_PARAM(S) ssGetSFcnParam(S,TIME_IDX)

#define JT_MIN_IDX 1
#define JT_MIN_PARAM(S) ssGetSFcnParam(S,JT_MIN_IDX)

#define JT_MAX_IDX 2
#define JT_MAX_PARAM(S) ssGetSFcnParam(S,JT_MAX_IDX)

#define FILE_IDX 3
#define FILE_PARAM(S) ssGetSFcnParam(S,FILE_IDX)

#define JTS_INCR_IDX 4
#define JTS_INCR_PARAM(S) ssGetSFcnParam(S,JTS_INCR_IDX)

#define MAX_JOINTS 6

#define pi 3.1415926535897931
#define A2 0.431800
#define A3 -0.020300
#define D3 0.150050
#define D4 0.433100
#define MAX_JTS 6
#define INDEX_NUM 14
#define AVERAGE_POT 3
#define TABLE_BUFFER 4
#define TABLE_START 103

#include "math.h"
#include "vtypes.h"
#if defined(RT)
/* include files for real time operation */
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
	ssSetNumSFcnParams(S, 5);	/* Number of expected parameters */
	if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
		/* Return if number of expected != number of actual parameters */
		return;
	}

	ssSetNumContStates(S, 0);
	ssSetNumDiscStates(S, 0);
	
	if (!ssSetNumInputPorts(S, 2)) return;
	//ssSetInputPortWidth(S, 0, 8);
	//ssSetInputPortMatrixDimensions(S, 0, 8, DYNAMICALLY_SIZED);
	if(!ssSetInputPortDimensionInfo( S, 0, DYNAMIC_DIMENSION)) return;
	ssSetInputPortWidth(S, 1, 1);
	ssSetInputPortDirectFeedThrough(S,0,1);
	ssSetInputPortDirectFeedThrough(S,1,1);

	if (!ssSetNumOutputPorts(S, 2)) return;
	ssSetOutputPortWidth(S, 0, 6);
	ssSetOutputPortWidth(S, 1, 1);

	ssSetNumSampleTimes(S, 1);
	ssSetNumRWork(S, 87);
	// RWork[0] = avgv
	// RWork[1] = time
	// RWork[2] = timeFinal
	// RWork[3-8] = calib jt increments
	// RWork[9-14] = last[0-5]
	// RWork[15-32] = coefs for traj
	ssSetNumIWork(S, 902);
	// IWork[0] = done
	// IWork[1] = current traj
	// IWork[2] = done calculating coefs
	// IWork[3] = calibration done
	// IWork[4] = traj type
	// IWork[5] = notdone
	// IWork[6] = calJoints
	// IWork[7-12] = doneJoints
	// IWork[13-96] = potVals[6][14]
	// IWork[97-102] = start index for table 97+i
	// IWork[103-902] = calib table
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
	real_T *RWork = ssGetRWork(S);
	int_T *IWork = ssGetIWork(S);
	int_T num_jts_incr, i;	
	int_T len;
	int_T numTot;
	int_T jt,num,idx,pot;
	char_T *fileName;
	double *jts;
	FILE *fptr;

	// get avg from parameters
	RWork[1] = 0.0;
	RWork[2] = 0.0;
	RWork[9] = 0.0; // initial last joint values
    RWork[10] = -pi/2;
    RWork[11] = pi/2;
	RWork[14] = RWork[12] = RWork[13] = 0.0;
	IWork[0] = 0; 
	IWork[1] = 0; // start with 1st trajectory(0 indexed)
	IWork[2] = 0; // start calculating coefficients
	IWork[3] = 0; // 0 = do calibration; 1 = skip calibration
	IWork[4] = 0; // trajectory type (0=jt, 1=cart)
	IWork[5] = 0; // notdone
	IWork[6] = 0; // local->calJoints
	IWork[7]=IWork[8]=IWork[9]=IWork[10]=IWork[11]=IWork[12]=INDEX_NUM; // doneJoints
	num_jts_incr = (int_T)mxGetNumberOfElements(JTS_INCR_PARAM(S));
	len = (int_T)mxGetNumberOfElements(FILE_PARAM(S));
	fileName = (char_T*)malloc(len+1);
	mxGetString(FILE_PARAM(S), fileName, len+1);

	printf("opening calibration file...\n");
	fptr = fopen(fileName,"r");
	if (fptr == NULL)
		printf("could not open file %s\n", fileName);

	// get joint increments from parameters
	jts = mxGetPr(JTS_INCR_PARAM(S));
	for (i=0;i<num_jts_incr;i++)
			RWork[i+3] = (real_T) jts[i];

	numTot=TABLE_START; // should be first index for pot table
	while ((fscanf(fptr, "%d %d",&jt,&num))!=EOF) {
		IWork[numTot]=jt;
		IWork[numTot+1]=num;
		IWork[TABLE_START-7+jt]=numTot;
		for (i=0;i<num;i++) {
			fscanf(fptr, "%d %d", &idx, &pot);
			IWork[numTot+(i+1)*2]=idx;
			IWork[numTot+(i+1)*2+1]=pot;
		}
		numTot+=2*num+2;
	}
	fclose(fptr);
#if defined (RT)
	/* Clear the index latch flags */
	robot_control(CLR_INDEX_BITS, 0, 0);
#endif
}
#endif // MDL_START

/* Function: mdlOutputs =======================================================
 * Abstract:
 *    In this function, you compute the outputs of your S-function
 *    block. Generally outputs are placed in the output vector, ssGetY(S).
 */
static void mdlOutputs(SimStruct *S, int_T tid)
{
	InputRealPtrsType posPtr = ssGetInputPortRealSignalPtrs(S,0);
	InputRealPtrsType enablePtr = ssGetInputPortRealSignalPtrs(S,1);
	real_T *y = ssGetOutputPortRealSignal(S,0);
	real_T *stop = ssGetOutputPortRealSignal(S,1);
	real_T posWidth = ssGetInputPortWidth(S,0);
	real_T pos[MAX_JOINTS];
	real_T maxPos;
	real_T *RWork = ssGetRWork(S);
	double *min_joints = mxGetPr(JT_MIN_PARAM(S));
	double *max_joints = mxGetPr(JT_MAX_PARAM(S));
	int_T i,j,k,t;
	int_T maxCnt;
	int_T numTraj;
	int_T potmax,potmin,pottmp;
	int_T magic[] = {70, -371, 332, -240, -222, -905};
	int_T *IWork = ssGetIWork(S);
	time_T time_step=ssGetSampleTime(S,0);
	real_T startPos[] = {0, -pi/2, pi/2, 0, 0, 0};
	real_T theta1[3];
	real_T theta2[3];
	real_T p1[3];
	real_T p2[3];
	real_T r, V114, Psi, num, den, tmp;
	real_T timeF,timeF2,timeF3;
	real_T c1,c2,c3,s1,s2,s3,c23,s23;
	real_T rkin,f11,d,e,w1,w2,t23;
	long longVal, tmpPos, indexVal, potSum;
	short shortVal;
	float tmpArray[6];
	long indPos[MAX_JTS], error, errMax;

	// check for correct number of values in trajectory input
	if ((int)posWidth%8!=0) {
		printf("incorrect number of values in trajectory matrix.\n");
		IWork[0]=IWork[3]=1;
	}
	numTraj=posWidth/8;
	if (IWork[1]==numTraj)
		stop[0]=1;

	// initialize the current pts with the last pts
	for (i=0;i<6;i++)
		y[i]=RWork[9+i];

	if (!IWork[3]&&(int_T)(*enablePtr[0])!=0) {
		// calibrate
#if defined (RT)
		if (IWork[6]==0) { // calJoints==0
			IWork[5] = 0;
			for (i=0;i<MAX_JTS;i++) {
				if (IWork[7+i]>0)
					IWork[5]++;
			}
			if (IWork[5] == 0){	// all jts are calibrated //
				// determine best match with the pot table for each joint //
				for (i=0; i<MAX_JTS; i++){
					// only try to match if the joint finished successfully //
					if (IWork[7+i] == 0){
						maxCnt = IWork[IWork[TABLE_START-6+i]+1];	// max size of this table //
						//pottmp = local->potVals[i][0];
						pottmp = IWork[13+i*14];
						j = 1;
						// Check for ascending pot table //
						//if (local->potVals[i][INDEX_NUM - 1] > pottmp){
						if (IWork[13+i*14+INDEX_NUM-1] > pottmp) {
							while (pottmp > IWork[IWork[TABLE_START-6+i]+2*j]) //TABLE(local->potTable[i],j,POT_VALUE)
								j++;
						} else {	// pot table is descending order //
							while (pottmp < IWork[IWork[TABLE_START-6+i]+2*j]) //TABLE(local->potTable[i],j,POT_VALUE)
								j++;
						} // endif //

						j = j - TABLE_BUFFER;	// search around the match //
						if (j < 1)			// make sure we're in the table //
							j = 1;

						errMax = 200000L;
						// find the best fit within a small region of the pot table //
						for (k = j; k < j + 2*TABLE_BUFFER; k++){
							// only calc the error if it fully fits in the table //
							if (k + INDEX_NUM - 1 <= maxCnt) {
								error = 0;
//#ifdef SKIP_INDEX
#if 1
								// sum the error over every other index found //
								for (t = 0; t < INDEX_NUM; t+=2)
									error += abs(IWork[13+i*14+t]-IWork[IWork[TABLE_START-6+i]+2*(k+t)]);
									//error += abs(local->potVals[i][t]-TABLE(local->potTable[i],k+t,POT_VALUE));
#else
								// sum the error over all indices found //
								for (t = 0; t < INDEX_NUM; t++)
									error += abs(IWork[13+i*14+t]-IWork[IWork[TABLE_START-6+i]+2*(k+t)]);
									//error += abs(local->potVals[i][t]-TABLE(local->potTable[i],k+t,POT_VALUE));
#endif
								//printf("Error[%d][%d] = %ld   ",i,k,error);

								if (error < errMax){
									errMax = error;
									indPos[i]=IWork[IWork[TABLE_START-6+i]+2*(k+INDEX_NUM-1)+1];
									//indPos[i] = TABLE(local->potTable[i],(k+INDEX_NUM-1),ENC_VALUE);
								} // endif //
							} // endif //
						} // endfor //
					} // endif - calib OK //
				} // endfor across joints //
#if 1
				// disable interrupts while updating encoder //
				//disable();
				for (i=0; i<MAX_JTS; i++){
					//if (local->doneJoints[i] == 0){
					if (IWork[7+i] == 0) {
						robot_longrd(INDEX, i, &indexVal);
						robot_longrd(POSITION, i, &tmpPos);
						shortVal = (short)(indPos[i] + tmpPos - indexVal);
						robot_shortwr(POSITION, i, &shortVal);
					} // endif //
				} // endfor //

				// Update reference values //
				robot_fltrd(POSITION6, 0, tmpArray);
				for (i=0;i<6;i++) {
					y[i]=RWork[9+i]=tmpArray[i];
				}
				//enable();
#else
				for (i=0; i<MAX_JTS; i++){
					//if (local->doneJoints[i] == 0){
					if (IWork[7+i] == 0) {
						robot_longrd(INDEX, i, &indexVal);
						robot_longrd(POSITION, i, &tmpPos);
						shortVal = (short)(indPos[i] + tmpPos - indexVal);
						printf("I:%ld, P: %ld, U: %d \n",indexVal, tmpPos, shortVal);
					}
				}
#endif
				robot_control(CALIB,0,1);
				//local->calJoints = -1;
				printf("Calibration complete\n");
				//return SBS_OFF;
				IWork[6]=-1;
				IWork[3]=1;
				IWork[1]=-1; // ???
			} // end if done calibration

			// calibrate all joints at once //
			for (i=0; i<MAX_JTS; i++){
				// Check if the pot table exists for this joint //
				//if (local->potTable[i] == NULL){
				//	local->doneJoints[i] = NO_TABLE;
				//} else if (local->doneJoints[i] > 0) {
				if (IWork[7+i]) {
					// Check the index found bit //
					robot_status(INDEX_BIT, i, &shortVal);
					if (shortVal == 0){
						// Index was found so clear flag and read the pot. //
						// Read index first for quicker service of STG Model 2 //
						robot_longrd(INDEX, i, &indexVal);
						robot_control(CLR_INDEX_BIT, i, 0);
						// average the pot values //
						potSum = 0;
						for (j=0; j<AVERAGE_POT; j++){
							robot_longrd(POT, i, &longVal);
							potSum += longVal;
						} // endfor //
						longVal = potSum/AVERAGE_POT;

						// save the current pot value for later processing //
						//local->potVals[i][INDEX_NUM - local->doneJoints[i]] = (int)longVal;
						IWork[13+i*14+INDEX_NUM-IWork[7+i]] = (int)longVal;
						//(local->doneJoints[i])--;
						IWork[7+i]--;
						//maxCnt = TABLE(local->potTable[i],0,1);
						maxCnt = IWork[IWork[TABLE_START-6+i]+1];

						//printf("pot %ld for joint %d\n",longVal,i+1);
						//if (i==4) printf("pot %ld for joint %d\n",longVal,i+1);

						// Check to make sure pot value is within range of pot table //
						// First, check to see which end of the table is lowest //
						//potmin = TABLE(local->potTable[i],1,POT_VALUE);
						potmin = IWork[IWork[TABLE_START-6+i]+2];
						potmax = IWork[IWork[TABLE_START-6+i]+2+2*(maxCnt-1)];
						//if (potmin > (potmax = TABLE(local->potTable[i],maxCnt,POT_VALUE))){
						if (potmin > potmax) {
							potmax = potmin;
							//potmin = TABLE(local->potTable[i],maxCnt,POT_VALUE);
							potmin = IWork[IWork[TABLE_START-6+i]+2+2*maxCnt];
						} // endif //

						// Now, compare the pot value to the max and min //
						if ((longVal < potmin) || (longVal > potmax)){
							//local->calJoints = -1;
							IWork[6]=-1;
							// save flag the joint for later status //
							//local->doneJoints[i] = TABLE_EXCEEDED;
							IWork[7+i]=-2;
							printf("Error: Joint %d out of range\n",i+1);
							//return SBS_OFF;
							stop[0]=1;
						} // endif //
					} // endif - index found //
				} // endif - table valid //
			} // endfor - loop through all joints //


			// update the reference positions if joint is not done //
			for (i=0; i<MAX_JTS; i++){
				//if (local->doneJoints[i] > 0){
				if (IWork[7+i] > 0) {
					//Q_RefG[i] += local->calInc[i];
					y[i] += RWork[3+i];
					RWork[9+i] += RWork[3+i];
				} // endif //
			} // endfor //

			// Assign the Magic Number to the next index pulse found //
		} else if (IWork[6] == -1){
			// Check to see if all joints are done. //
			//notDone = 0;
			IWork[5] = 0;
			for (i=0; i<MAX_JTS; i++){
				//if (local->doneJoints[i] > 0){
				if (IWork[7+i] > 0) {
					//notDone++;
					IWork[5]++;
				} // endif //
			} // endfor //
			//if (notDone == 0){	// Calibration is done //
			if (IWork[5] == 0) {
#if 1
				// disable interrupts while updating encoder //
				//disable();
				for (i=0; i<MAX_JTS; i++){
					//if (local->doneJoints[i] == 0){
					if (IWork[7+i] == 0) {
						robot_longrd(INDEX, i, &indexVal);
						robot_longrd(POSITION, i, &tmpPos);
						//shortVal = (short)(local->magicNum[i] + tmpPos - indexVal);
						shortVal = (short)(magic[i]+tmpPos-indexVal);
						robot_shortwr(POSITION, i, &shortVal);
					} // endif //
				} // endfor //

				// Update reference values //
				robot_fltrd(POSITION6, 0, tmpArray);
				for (i=0;i<6;i++)
					y[i]=RWork[9+i]=tmpArray[i];
				//enable();
#endif
				robot_control(CALIB,0,1);
				//local->calJoints = -1;
				printf("Calibration complete\n");
				IWork[6]=-1;
				IWork[3]=1;
				IWork[1]=-1;
				//return SBS_OFF;
			} // endif //

			// calibrate all joints at once //
			for (i=0; i<MAX_JTS; i++){
				// Check if the pot table exists for this joint //
				//if (local->potTable[i] == NULL){
					//local->doneJoints[i] = NO_TABLE;
				//} else if (local->doneJoints[i] > 0){
				if (IWork[7+i] > 0) {
					// Check the index found bit //
					robot_status(INDEX_BIT, i, &shortVal);
					if (shortVal == 0){
						// Index was found so clear flag and read the pot. //
						// Read index first for quicker service of STG Model 2 //
						robot_longrd(INDEX, i, &indexVal);
						robot_control(CLR_INDEX_BIT, i, 0);
						// no need to search this joint any further //
						//local->doneJoints[i] = 0;
						IWork[7+i]=0;
					} // endif - index found //
				} // endif - table valid //
			} // endfor - loop through all joints //

			// update the reference positions if joint is not done //
			for (i=0; i<MAX_JTS; i++){
				//if (local->doneJoints[i] > 0){
				if (IWork[7+i] > 0) {
					//Q_RefG[i] += local->calInc[i];
					y[i] += RWork[3+i];
					RWork[9+i] += RWork[3+i];
				} // endif //
			} // endfor //
		} // endif // */
#elif defined (MATLAB_MEX_FILE)
		RWork[9]=1;
		RWork[10]=.5;
		RWork[11]=.25;
		RWork[12]=.5435;
		RWork[13]=.5435;
		RWork[14]=.5435;
		IWork[3]=1;
		IWork[1]=-1;
#endif
	} // endif //

	if (!IWork[0]&&IWork[3]&&!IWork[2]&&(int_T)(*enablePtr[0])!=0) { // set up coefficients of polynomial
		maxPos = 0;
		if (IWork[1]==-1)
			IWork[4]=0;
		else
			IWork[4]=(int)*posPtr[IWork[1]*8+6];
		if (IWork[1]==-1)
			RWork[0] = 0.25;
		else if (IWork[4]!=2) // only if this isn't a wait period
			RWork[0]=*posPtr[IWork[1]*8+7]; // get avgV for this traj
		for (i=0;i<6;i++) { // get diff between current and final positions
			if (IWork[1]==-1)
				pos[i] = startPos[i] - RWork[i+9];
			else if (IWork[4]!=2) // only if this isn't a wait period
				pos[i] = *posPtr[IWork[1]*8+i]-RWork[i+9];
			if (IWork[4]!=2) {
				if (pos[i] > maxPos)
					maxPos = pos[i];
				else if (pos[i] < -1* maxPos)
					maxPos = -1 * pos[i];
			}
		}
		if (IWork[4]==2)
			RWork[2]=*posPtr[IWork[1]*8+7];
		else
			RWork[2] = maxPos / RWork[0]; // get time required for this traj
		if (RWork[2] == 0) // don't want to divide by zero
			RWork[2] = 0.001;
		if (IWork[4]==0) { // joint space motion
			for (i=0;i<6;i++) {
				if (IWork[1]>-1) {
					RWork[15+i] = RWork[i+9]; // last jt[i]
					RWork[21+i] = 3 * pos[i] / (RWork[2]*RWork[2]);
					RWork[27+i] = -2 * pos[i] / (RWork[2]*RWork[2]*RWork[2]);
				} else {
					RWork[15+i] = RWork[i+9]; // last jt[i]
					RWork[21+i] = 3 * pos[i] / (RWork[2]*RWork[2]);
					RWork[27+i] = -2 * pos[i] / (RWork[2]*RWork[2]*RWork[2]);
				}
			}
		} else if (IWork[4]==1) { //assuming cartesion motion
			for (i=0;i<3;i++) {
				theta2[i]=*posPtr[IWork[1]*8+i];
				theta1[i]=RWork[i+9];
			}
			c1=cos(theta1[0]);c2=cos(theta1[1]);c3=cos(theta1[2]);
			s1=sin(theta1[0]);s2=sin(theta1[1]);s3=sin(theta1[2]);
			c23=c2*c3-s2*s3;s23=s2*c3+c2*s3;
			p1[0]=c1*s23*D4-s1*D3+c1*c23*A3+c1*c2*A2;
			p1[1]=s1*s23*D4+c1*D3+s1*c23*A3+s1*c2*A2;
			p1[2]=c23*D4-s23*A3-s2*A2;
			// calculate ending cartesian pts
			c1=cos(theta2[0]);c2=cos(theta2[1]);c3=cos(theta2[2]);
			s1=sin(theta2[0]);s2=sin(theta2[1]);s3=sin(theta2[2]);
			c23=c2*c3-s2*s3;s23=s2*c3+c2*s3;
			p2[0]=c1*s23*D4-s1*D3+c1*c23*A3+c1*c2*A2;
			p2[1]=s1*s23*D4+c1*D3+s1*c23*A3+s1*c2*A2;
			p2[2]=c23*D4-s23*A3-s2*A2;
			for (i=0;i<3;i++) { //generate polynomials in cartesian space
				// don't need polynomials for wrist joint. Keep them the same throughout
				// traj
				RWork[15+i] = p1[i];
				RWork[21+i] = 3 * (p2[i]-p1[i])/(RWork[2]*RWork[2]);
				RWork[27+i] = -2 * (p2[i]-p1[i])/(RWork[2]*RWork[2]*RWork[2]);
			}
		}
		IWork[2] = 1;
		RWork[1] = 0.0; // reset clock for polynomials
	}

	if (!IWork[0]&&IWork[2]&&(int_T)(*enablePtr[0])!=0) {
		RWork[1] += (real_T) time_step;
		timeF = RWork[1];
		timeF2 = timeF*timeF;
		timeF3 = timeF2*timeF;

		stop[0] = 0;
		if (IWork[4]==0)  {// joint motion
			if (RWork[1] < RWork[2]) {
				for (i=0;i<6;i++) {
					y[i] = RWork[9+i] = RWork[15+i] + RWork[21+i] * timeF2 + RWork[27+i] * timeF3;
					if (y[i] > max_joints[i]) {
						stop[0] = 1;
						printf("Error: Max joint %d exceeeded %f %f\n",i,max_joints[i],y[i]);
					}
					if (y[i] < min_joints[i]) {
						stop[0] = 1;
						printf("Error: Min joint %d exceeeded %f %f \n", i, min_joints[i],y[i]);
					}
				}
			} else {
				IWork[2]=0;
				if (++IWork[1]==numTraj) {
					stop[0]=1;
				}
				for (i=0;i<6;i++) {
					y[i] = RWork[9+i];
				}
			}
		} else if (IWork[4]==1){ // assume cartesion traj
			if (RWork[1] < RWork[2]) {
				for (i=0;i<3;i++) {
					pos[i]=RWork[15+i] + RWork[21+i] * timeF2 + RWork[27+i] * timeF3;
				}
				r=pos[0]*pos[0]+pos[1]*pos[1];
				rkin=r-D3*D3;
				if (rkin<0.0)
					printf("unreachable position for joint 1\n");
				rkin=sqrt(rkin);
				tmp=atan2(pos[1],pos[0])-atan2(D3,rkin);
				y[0]=atan2(pos[1],pos[0])-atan2(D3,-rkin);
				if (fabs(tmp-RWork[9])<fabs(y[0]-RWork[9])) {
					y[0] = tmp;
					RWork[9] = tmp;
				} else
					RWork[9]=y[0];
				c1=cos(y[0]);s1=sin(y[0]);
				f11=c1*pos[0]+s1*pos[1];
				d=f11*f11+pos[2]*pos[2]-D4*D4-A3*A3-A2*A2;
				e=4*A2*A2*(A3*A3+D4*D4);
				w1=e-d*d;
				if (w1<0.0)
					printf("unreachable position for joint 3\n");
				rkin=sqrt(w1);
				tmp=atan2(d,rkin)-atan2(A3,D4);
				y[2]=atan2(d,-rkin)-atan2(A3,D4);
				if (fabs(tmp-RWork[11])<fabs(y[2]-RWork[11])) {
					y[2] = tmp;
					RWork[11] = tmp;
				} else
					RWork[11]=y[2];
				c3=cos(y[2]);s3=sin(y[2]);
				w1=A2*c3+A3;
				w2=D4+A2*s3;
				t23=atan2((w2*f11-w1*pos[2]),(w1*f11+w2*pos[2]));
				y[1]=RWork[10]=t23-y[2];
				for (i=0;i<3;i++) {
					if (y[i] > max_joints[i]) {
						stop[0] = 1;
						printf("Error: Max joint %d exceeeded %f %f\n",i,max_joints[i],y[i]);
					}
					if (y[i] < min_joints[i]) {
						stop[0] = 1;
						printf("Error: Min joint %d exceeeded %f %f \n", i, min_joints[i],y[i]);
					}
				}
			} else {
				IWork[2]=0;
				if (++IWork[1]==numTraj) {
					stop[0]=1;
				}
				for (i=0;i<6;i++) {
					y[i] = RWork[9+i];
				}
			}
		} else { // assume wait period
			// do nothing
			if (RWork[1]>=RWork[2]) {
				IWork[2]=0;
				if (++IWork[1]==numTraj) {
					stop[0]=1;
				}
			}
		}
	}
}

#if defined(MATLAB_MEX_FILE)
#define MDL_SET_INPUT_PORT_DIMENSION_INFO
/* Function: mdlSetInputPortDimensionInfo ====================================
 * Abstract:
 *    This routine is called with the candidate dimensions for an input port
 *    with unknown dimensions. If the proposed dimensions are acceptable, the 
 *    routine should go ahead and set the actual port dimensions.  
 *    If they are unacceptable an error should be generated via 
 *    ssSetErrorStatus.  
 *    Note that any other input or output ports whose dimensions are 
 *    implicitly defined by virtue of knowing the dimensions of the given port 
 *    can also have their dimensions set.
 */
static void mdlSetInputPortDimensionInfo(SimStruct        *S, 
                                         int_T            port,
                                         const DimsInfo_T *dimsInfo)
{
    int_T  uNumDims = dimsInfo->numDims;
    int_T  uWidth   = dimsInfo->width;
    int_T  *uDims   = dimsInfo->dims;

    /* Set input port dimension */
    if(!ssSetInputPortDimensionInfo(S, port, dimsInfo)) return;
}

# define MDL_SET_OUTPUT_PORT_DIMENSION_INFO
/* Function: mdlSetOutputPortDimensionInfo ===================================
 * Abstract:
 *    This routine is called with the candidate dimensions for an output port 
 *    with unknown dimensions. If the proposed dimensions are acceptable, the 
 *    routine should go ahead and set the actual port dimensions.  
 *    If they are unacceptable an error should be generated via 
 *    ssSetErrorStatus.  
 *    Note that any other input or output ports whose dimensions are  
 *    implicitly defined by virtue of knowing the dimensions of the given 
 *    port can also have their dimensions set.
 */
static void mdlSetOutputPortDimensionInfo(SimStruct        *S, 
                                          int_T            port,
                                          const DimsInfo_T *dimsInfo)
{
}

# define MDL_SET_DEFAULT_PORT_DIMENSION_INFO
/* Function: mdlSetDefaultPortDimensionInfo ====================================
 *    This routine is called when Simulink is not able to find dimension
 *    candidates for ports with unknown dimensions. This function must set the
 *    dimensions of all ports with unknown dimensions.
 */
static void mdlSetDefaultPortDimensionInfo(SimStruct *S)
{
}
#endif

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
	printf("\ntrajectory generation module: termination\n");
#elif defined(MATLAB_MEX_FILE)
	/* during simulation, just print a message */
	if (ssGetSimMode(S) == SS_SIMMODE_NORMAL) {
		mexPrintf("\ntraj: Simulated termination\n");
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
#include "simulink.c"     /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"      /* Code generation registration function */
#endif
