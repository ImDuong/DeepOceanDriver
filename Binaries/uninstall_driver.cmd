@echo off
sc stop deoc
timeout 3
sc delete deoc
timeout 3
del C:\Windows\System32\drivers\deocdrv.sys