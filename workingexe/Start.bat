@echo off

::Start server as service
sc create CS_MiniServerSvc binpath=%~dp0CS_MiniServer.exe
sc description CS_MiniServerSvc "Service for CS_MiniServer"
sc start CS_MiniServerSvc
pause