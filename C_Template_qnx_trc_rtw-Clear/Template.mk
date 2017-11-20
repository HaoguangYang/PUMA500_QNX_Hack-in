# Copyright 1994-2002 The MathWorks, Inc.
#
# File    : qnx_unix.tmf   $Revision: 1.81 $
# 10/26/17  Modified by Haoguang Yang for compatibility with QNX 6.x and newer version of MATLAB.
#
# Abstract:
#	Real-Time Workshop template makefile for building a UNIX-based
#	stand-alone generic real-time version of Simulink model using
#	generated C code.
#
# 	This makefile attempts to conform to the guidelines specified in the
# 	IEEE Std 1003.2-1992 (POSIX) standard. It is designed to be used
#       with GNU Make which is located in matlabroot/rtw/bin.
#
# 	Note that this template is automatically customized by the Real-Time
#	Workshop build procedure to create "<model>.mk"
#
#       The following defines can be used to modify the behavior of the
#	build:
#	  OPT_OPTS       - Optimization options. Default is -O. To enable 
#                          debugging specify as OPT_OPTS=-g. 
#                          Because of optimization problems in IBM_RS, 
#                          default is no-optimization. 
#	  OPTS           - User specific compile options.
#	  USER_SRCS      - Additional user sources, such as files needed by
#			   S-functions.
#	  USER_INCLUDES  - Additional include paths
#			   (i.e. USER_INCLUDES="-Iwhere-ever -Iwhere-ever2")
#
#       This template makefile is designed to be used with a system target
#       file that contains 'rtwgensettings.BuildDirSuffix' see qnx.tlc

#---------------------- USAGE WITH QNX MOMENTICS IDE ---------------------------
#
# File -> New -> Makefile Project with Existing Code
# Navigate to the source folder (/..._stg or /..._trc)
# Toolchain: Multi-Toolchain
# Finish
# On Project Explorer, right click the project imported, choose Properties
# Under C/C++ Build Tab, make sure Automatic Makefile Generation in Builder Settings is NOT SELECTED
# In Behavior, under Build, type in the following:
# 		-f "$(MODEL).mk" MODELLIB="$(MODEL)lib.lib" RELATIVE_PATH_TO_ANCHOR=.. MODELREF_TARGET_TYPE=NONE ISPROTECTINGMODEL=NOTPROTECTING MATLAB=C:\PROGRA~1\MATLAB\R2015b
# Please change the first $(MODEL) to the model name you have.
# (You may need to change MATLAB Path to your actual installation path and version.)
# Under Clean, similiarly type in:
#		-f "$(MODEL).mk" clean
# OR
# 		-f "$(MODEL).mk" purge
# (Sorry to my Windows Fellows, but it uses command rm ...)
# You should now build your generated code just as building under command line.

#------------------------ Macros read by make_rtw ------------------------------
#
# The following macros are read by the Real-Time Workshop build procedure:
#
#  MAKECMD         - This is the command used to invoke the make utility
#  HOST            - What platform this template makefile is targeted for
#                    (i.e. PC or UNIX)
#  BUILD           - Invoke make from the Real-Time Workshop build procedure
#                    (yes/no)?
#  SYS_TARGET_FILE - Name of system target file.

MAKECMD         = make
HOST            = UNIX
BUILD           = yes
SYS_TARGET_FILE = qnx_trc.tlc

#---------------------- Tokens expanded by make_rtw ----------------------------
#
# The following tokens, when wrapped with "|>" and "|<" are expanded by the
# Real-Time Workshop build procedure.
#
#  MODEL_NAME          - Name of the Simulink block diagram
#  MODEL_MODULES       - Any additional generated source modules
#  MAKEFILE_NAME       - Name of makefile created from template makefile <model>.mk
#  MATLAB_ROOT         - Path to were MATLAB is installed.
#  S_FUNCTIONS         - List of S-functions.
#  S_FUNCTIONS_LIB     - List of S-functions libraries to link. 
#  SOLVER              - Solver source file name
#  NUMST               - Number of sample times
#  TID01EQ             - yes (1) or no (0): Are sampling rates of continuous task
#                        (tid=0) and 1st discrete task equal.
#  NCSTATES            - Number of continuous states
#  COMPUTER            - Computer type. See the MATLAB computer command.
#  BUILDARGS           - Options passed in at the command line.
#  MULTITASKING        - yes (1) or no (0): Is solver mode multitasking
#  EXT_MODE            - yes (1) or no (0): Build for external mode
#  TMW_EXTMODE_TESTING - yes (1) or no (0): Build ext_test.c for external mode
#                        testing.

MODEL                = Template
MODULES              = Template_data.c rtGetInf.c rtGetNaN.c rt_logging.c rt_matrx.c rt_printf.c 
MAKEFILE             = Template.mk
#--------------------------
#NOTICE: May need to modify MATLAB_ROOT to eliminate spaces. e.g. C:/PROGRA~1/...
MATLAB_ROOT          = C:\PROGRA~1\MATLAB\R2015b
#--------------------------
S_FUNCTIONS          = robot_init.c robot_power.c
S_FUNCTIONS_LIB      = 
SOLVER               = 
NUMST                = 2
TID01EQ              = 0
NCSTATES             = 0
COMPUTER             = GLNX86
BUILDARGS            =  MODELLIB=Templatelib.lib RELATIVE_PATH_TO_ANCHOR=.. MODELREF_TARGET_TYPE=NONE ISPROTECTINGMODEL=NOTPROTECTING
MULTITASKING         = 1
EXT_MODE             = 0
TMW_EXTMODE_TESTING  = 0

#--------------------------- Tool Specifications -------------------------------

include $(MATLAB_ROOT)/rtw/c/tools/unixtools.mk

#------------------------------ Include Path -----------------------------------

MATLAB_INCLUDES = \
	-I$(MATLAB_ROOT)/simulink/include \
	-I$(MATLAB_ROOT)/extern/include \
	-I$(MATLAB_ROOT)/rtw/c/src \
	-I$(MATLAB_ROOT)/rtw/c/libsrc \
	-I$(MATLAB_ROOT)/rtw/c/src/ext_mode/common \
	-I$(MATLAB_ROOT)/toolbox/pumatrc

# Additional file include paths
ADD_INCLUDES = \
	-I/matlab/rtw/c/libsrc \
	-I/matlab/rtw/c/libsrc \


INCLUDES = -I. -I.. $(MATLAB_INCLUDES) $(ADD_INCLUDES) $(USER_INCLUDES) \
	$(INSTRUMENT_INCLUDES)

#----------------------------- External mode -----------------------------------
# Uncomment -DVERBOSE to have information printed to stdout (or specify
# OPTS=-DVERBOSE).
ifeq ($(EXT_MODE),1)
  EXT_SRC     = ext_svr.c updown.c ext_svr_transport.c ext_work.c
  EXT_CC_OPTS = -DEXT_MODE -D$(COMPUTER) #-DVERBOSE
  EXT_LIB     =
  ifeq ($(TMW_EXTMODE_TESTING),1)
    EXT_SRC     += ext_test.c
    EXT_CC_OPTS += -DTMW_EXTMODE_TESTING
  endif
  ifeq ($(COMPUTER),SOL2)
    EXT_LIB = -lsocket -lnsl
  endif
endif

#----------------------------- Real-Time Model ---------------------------------
RTM_CC_OPTS = -DUSE_RTMODEL

#-------------------------------- C Flags --------------------------------------

#--------------------------------
#CHANGE YOUR ARCHITECTURE HERE!

CC = qcc -V4.7.3,gcc_ntoarmv7le

# Optimization Options
ifndef OPT_OPTS
OPT_OPTS = $(DEFAULT_OPT_OPTS) -Bstatic -static -fno-stack-protector
#OPT_OPTS = -Ofast -Bstatic -static -fno-stack-protector
endif

# General User Options
OPTS = -g -V4.7.3,gcc_ntoarmv7le -Bstatic -static -fno-stack-protector
#--------------------------------

# Compiler options, etc:
CC_OPTS = $(OPT_OPTS) $(OPTS) $(ANSI_OPTS) $(EXT_CC_OPTS) $(RTM_CC_OPTS)

CPP_REQ_DEFINES = -DMODEL=$(MODEL) -DRT -DNUMST=$(NUMST) \
                  -DTID01EQ=$(TID01EQ) -DNCSTATES=$(NCSTATES) -DUNIX \
                  -DMT=$(MULTITASKING) -DHAVESTDIO

CFLAGS = $(CC_OPTS) $(CPP_REQ_DEFINES) $(INCLUDES)

LDFLAGS =

#-------------------------- Additional Libraries ------------------------------

SYSLIBS = $(EXT_LIB) -lm -lsocket

LIBS =
 
LIBS += $(S_FUNCTIONS_LIB) $(INSTRUMENT_LIBS)

#----------------------------- Source Files ------------------------------------

REQ_SRCS  = $(MODEL).c $(MODULES) qnx_trc_main.c rt_sim.c rt_nonfinite.c \
            $(EXT_SRC)
USER_SRCS = qnx_robot.c

USER_OBJS       = $(USER_SRCS:.c=.o)
LOCAL_USER_OBJS = $(notdir $(USER_OBJS))

SRCS      = $(REQ_SRCS) $(S_FUNCTIONS) $(SOLVER)
OBJS      = $(SRCS:.c=.o) $(USER_OBJS)
LINK_OBJS = $(SRCS:.c=.o) $(LOCAL_USER_OBJS)

# PROGRAM = ../$(MODEL)

#------------- Test Compile using gcc -Wall to look for warnings ---------------
#
# DO_GCC_TEST=1 runs gcc with compiler warning flags on all the source files
# used in this build. This includes the generated code, and any user source
# files needed for the build and placed in this directory.
#
# WARN_ON_GLNX=1 runs the linux compiler with warnings flags. On hand-written
# code we use the max amount of flags availabe while on the generated code, a
# few less.
#
# See rtw/c/tools/unixtools.mk for the definition of GCC_WARN_OPTS
GCC_TEST_CMD  := echo
GCC_TEST_OUT  := > /dev/null
ifeq ($(DO_GCC_TEST), 1)
  GCC_TEST := gcc -c -o /dev/null $(GCC_WARN_OPTS_MAX) $(CPP_REQ_DEFINES) \
                                     $(INCLUDES)
  GCC_TEST_CMD := echo; echo "\#\#\# GCC_TEST $(GCC_TEST) $<"; $(GCC_TEST)
  GCC_TEST_OUT := ; echo
endif

GCC_WALL_FLAG     :=
GCC_WALL_FLAG_MAX :=
ifeq ($(COMPUTER), GLNX86)
ifeq ($(WARN_ON_GLNX), 1)
  GCC_WALL_FLAG     := $(GCC_WARN_OPTS)
  GCC_WALL_FLAG_MAX := $(GCC_WARN_OPTS_MAX)
endif
endif


#----------------------------- Lint (sol2 only) --------------------------------

LINT_SRCS = $(MATLAB_ROOT)/rtw/c/qnx_trc/qnx_trc_main.c \
	    $(MATLAB_ROOT)/rtw/c/src/rt_sim.c
ifneq ($(SOLVER), )
  LINT_SRCS += $(MATLAB_ROOT)/rtw/c/src/$(SOLVER)
endif
ifneq ($(EXT_SRC), )
  LINT_SRCS += $(MATLAB_ROOT)/rtw/c/src/$(EXT_SRC)
endif
LINT_SRCS += $(MODEL).c $(MODULES) $(USER_SRCS) $(S_FUNCTIONS)
LINTOPTSFILE = $(MODEL).lintopts

LINT_ERROFF1 = E_NAME_DEF_NOT_USED2,E_NAME_DECL_NOT_USED_DEF2
LINT_ERROFF2 = $(LINT_ERROFF1),E_FUNC_ARG_UNUSED
LINT_ERROFF  = $(LINT_ERROFF2),E_INDISTING_FROM_TRUNC2,E_NAME_USED_NOT_DEF2

#--------------------------------- Rules ---------------------------------------

$(MODEL) : $(OBJS) $(LIBS)
	$(LD) $(LDFLAGS) $(INCLUDES) -o $@ $(LINK_OBJS) $(LIBS) $(SYSLIBS)
	@echo "### Created executable: $(MODEL)"

%.o : %.c
	@$(GCC_TEST_CMD) $< $(GCC_TEST_OUT)
	$(CC) -c -o $(@F) $(CFLAGS) $(GCC_WALL_FLAG) $<

%.o : $(MATLAB_ROOT)/rtw/c/qnx_trc/%.c
	@$(GCC_TEST_CMD) $< $(GCC_TEST_OUT)
	$(CC) -c $(CFLAGS) $(GCC_WALL_FLAG_MAX) $<

%.o : $(MATLAB_ROOT)/rtw/c/src/%.c
	@$(GCC_TEST_CMD) $< $(GCC_TEST_OUT)
	$(CC) -c $(CFLAGS) $(GCC_WALL_FLAG_MAX) $<

%.o : $(MATLAB_ROOT)/rtw/c/libsrc/%.c
	@$(GCC_TEST_CMD) $< $(GCC_TEST_OUT)
	$(CC) -c $(CFLAGS) $(GCC_WALL_FLAG_MAX) $<

%.o : $(MATLAB_ROOT)/rtw/c/%.cpp
	@$(GCC_TEST_CMD) $< $(GCC_TEST_OUT)
	$(CPP) -c $(CFLAGS) $<

%.o : $(MATLAB_ROOT)/simulink/src/%.c
	@$(GCC_TEST_CMD) $< $(GCC_TEST_OUT)
	$(CC) -c $(CFLAGS) $(GCC_WALL_FLAG_MAX) $<

%.o : $(MATLAB_ROOT)/simulink/src/%.cpp
	@$(GCC_TEST_CMD) $< $(GCC_TEST_OUT)
	$(CPP) -c $(CFLAGS) $<

%.o : $(MATLAB_ROOT)/toolbox/pumatrc/%.c
	@$(GCC_TEST_CMD) $< $(GCC_TEST_OUT)
	$(CC) -c $(CFLAGS) $(GCC_WALL_FLAG_MAX) $<

%.o : ../%.c
	@$(GCC_TEST_CMD) $< $(GCC_TEST_OUT)
	$(CC) -c $(CFLAGS) $(GCC_WALL_FLAG) $<

%.o : ../%.cpp
	@$(GCC_TEST_CMD) $< $(GCC_TEST_OUT)
	$(CPP) -c $(CFLAGS) $<

#------------------------------- Libraries -------------------------------------





#----------------------------- Dependencies ------------------------------------

$(OBJS) : $(MAKEFILE) rtw_proj.tmw

#--------- Miscellaneous rules to purge, clean and lint (sol2 only) ------------

purge : clean
	@echo "### Deleting the generated source code for $(MODEL)"
	@\rm -f $(MODEL).c $(MODEL).h $(MODEL)_types.h $(MODEL)_data.c \
	        $(MODEL)_private.h $(MODEL).rtw $(MODULES) rtw_proj.tmw $(MAKEFILE)

clean :
	@echo "### Deleting the objects and $(MODEL)"
	@\rm -f $(LINK_OBJS) $(MODEL)

lint  : rtwlib.ln
	@lint -errchk -errhdr=%user -errtags=yes -F -L. -lrtwlib -x -Xc \
	      -erroff=$(LINT_ERROFF) \
	      -D_POSIX_C_SOURCE $(CFLAGS) $(LINT_SRCS)
	@\rm -f $(LINTOPTSFILE)
	@echo
	@echo "### Created lint output only, no executable"
	@echo

rtwlib.ln : $(MAKEFILE) rtw_proj.tmw
	@echo
	@echo "### Linting ..."
	@echo
	@\rm -f llib-lrtwlib.ln $(LINTOPTSFILE)
	@echo "-dirout=. -errchk -errhdr=%user " >> $(LINTOPTSFILE)
	@echo "-errtags -F -ortwlib -x -Xc " >> $(LINTOPTSFILE)
	@echo "-erroff=$(LINT_ERROFF) " >> $(LINTOPTSFILE)
	@echo "-D_POSIX_C_SOURCE $(CFLAGS) " >> $(LINTOPTSFILE)
	@for file in $(MATLAB_ROOT)/rtw/c/libsrc/*.c; do \
	  echo "$$file " >> $(LINTOPTSFILE); \
	done
	lint -flagsrc=$(LINTOPTSFILE)

# EOF: qnx_unix.tmf





