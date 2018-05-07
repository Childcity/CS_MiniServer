@echo off

::Stop server
sc stop CS_MiniServerSvc
sc delete CS_MiniServerSvc
