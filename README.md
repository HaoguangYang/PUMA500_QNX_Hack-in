Update of project on the [following page](http://www-users.cs.umn.edu/~martin/PUMA/):

[http://www-users.cs.umn.edu/~martin/PUMA/](http://www-users.cs.umn.edu/~martin/PUMA/)

## Installation and Compilation
 - Prerequisites
    
    MATLAB 2015b (tested) and above with SIMULINK and Real Time Workshop;
    
    QNX IDE 6.5.0 assumed to be installed under default path.
    
 - Add `puma_sim_v21/rtw/c/qnx_*` and `puma_sim_v21/toolbox/puma*` folders to MATLAB path;
 
 - Run `puma_sim_v21/set_qnx_env_Win.bat` everytime before compiling any code modified from provided template in commandline.
 
 - Or compile generated C code in QNX IDE:
 
    File -> New -> Makefile Project with Existing Code
    
    Navigate to the source folder (`/..._stg` or `/..._trc`)
    
    Toolchain: `Multi-Toolchain`, then click `Finish`.
    
    On Project Explorer, right click the project imported, choose Properties
    
    Under C/C++ Build Tab, make sure `Automatic Makefile Generation` in Builder Settings is NOT SELECTED
    
    In Behavior, under Build, type in the following:
    
    ```
    -f "$(MODEL).mk" MODELLIB="$(MODEL)lib.lib" RELATIVE_PATH_TO_ANCHOR=.. MODELREF_TARGET_TYPE=NONE ISPROTECTINGMODEL=NOTPROTECTING MATLAB=C:\PROGRA~1\MATLAB\R2015b
    ```
    
    Please change the first `$(MODEL)` to the model name you have. (You may need to change MATLAB Path to your actual installation path and version.)
    
    Under Clean, similiarly type in: `-f "$(MODEL).mk" clean` OR `-f "$(MODEL).mk" purge` (Sorry to my Windows Fellows, but it uses command rm ...)
    
    You should now build your generated code just as building under command line.
