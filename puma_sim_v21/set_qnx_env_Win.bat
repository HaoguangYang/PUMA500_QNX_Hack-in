set QNX_TARGET=C:\qnx660\target\qnx6
set QNX_TARGET=%QNX_TARGET:\=/%
set QNX_HOST=C:\qnx660\host\win32\x86
set QNX_HOST=%QNX_HOST:\=/%
set QNX_CONFIGURATION=C:\qnx660\.qnx
set MAKEFLAGS=-IC:\qnx660\target\qnx6\usr\include
set MAKEFLAGS=%MAKEFLAGS:\=/%
set PATH=C:\qnx660\host\win32\x86\usr\bin;C:\qnx660\.qnx\bin;C:\qnx660\jre\bin;%PATH%
set qnxCarDeployment=C:\qnx660\deployment\qnx-car
if exist %qnxCarDeployment%\qnxcar-env.bat %qnxCarDeployment%\qnxcar-env.bat
