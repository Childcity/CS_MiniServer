@echo off

:: Log lines have this form:
::
::     Lmmdd hh:mm:ss.uuuuuu threadid file:line] msg...
::
:: where the fields are defined as follows:
::
::   L                A single character, representing the log level
::                    (eg 'I' for INFO)
::   mm               The month (zero padded; ie May is '05')
::   dd               The day (zero padded)
::   hh:mm:ss.uuuuuu  Time in hours, minutes and fractional seconds
::   threadid         The space-padded thread ID as returned by GetTID()
::                    (this matches the PID on Linux)
::   file             The file name
::   line             The line number
::   msg              The user-supplied message
::
:: Example:
::
::   I1103 11:57:31.739339 24395 google.cc:2341] Command line: ./some_prog
::   I1103 11:57:31.739403 24395 google.cc:2342] Process id 24395
::
:: NOTE: although the microseconds are useful for comparing events on
:: a single machine, clocks on different machines may not be well
:: synchronized.  Hence, use caution when comparing the low bits of
:: timestamps from different machines.


::Log messages to stderr(console)	 instead of logfiles
set GLOG_logtostderr=false

::FATAL<-WARNINGS<-INFO
::Log message at or above this level. Default 0, that represent INFO and above
set GLOG_minloglevel=0

::Deep log for debuging. 0=off, 1=on
set GLOG_v=0

::Log folder. Default windows tmp directory
set GLOG_log_dir=logs

if not exist %GLOG_log_dir% mkdir %GLOG_log_dir%
if %errorlevel% NEQ 0 pause & exit /b 1

::Start server
start /wait /b CS_MiniServer.exe

exit /b 0