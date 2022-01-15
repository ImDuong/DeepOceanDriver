@echo off

copy deocdrv.sys C:\Windows\System32\drivers\deocdrv.sys

sc create deoc type= kernel binPath= "C:\Windows\System32\drivers\deocdrv.sys"

sc start deoc