@echo off

copy ..\DOClientHelper\Build\DOClientHelper.exe DOClientHelper.exe

copy ..\x64\Debug\deocdrv.sys deocdrv.sys

copy ..\x64\Debug\DOClient.exe DOClient.exe

copy "..\Driver Scripts\install_driver.cmd" install_driver.cmd
copy "..\Driver Scripts\uninstall_driver.cmd" uninstall_driver.cmd