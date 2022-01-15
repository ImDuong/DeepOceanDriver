# DeepOceanDriver
## Overview
- This repo represents our School Project for Course "System Programming" in RHUST. Therefore, the main goal is to revise & share knowledge with system internals and kernel driver, specifically.
- In this project, we demonstrated a nice solution to monitor process creation in Windows. In details, we registered process creation/destruction notifications via `PsSetCreateProcessNotifyRoutineEx` in kernel mode. Therefore, we could view all informations about process creation & destruction in real-time. In user mode, we set up a client to print notifications that are from kernel driver to console. Nevertheless, we added up a feature that allow blocking program by path. Client in user mode can prevent any program from running by sending a blacklist to the driver in kernel mode. Then the driver will continuous check if any process creation starts from any path existing in the blacklist. There are 2 important things related to this feature. First, driver needs to compare paths insensitively. Second, driver should consider a mechanism to prevent the case the program changes its name. However, in this project, we only took the first case seriously and considered the second case as unpreventable.

## Architecture
- A driver set up in kernel mode for retrieving any process creation/destruction notifications and blocking any process creation if its path existed in blacklist.
- An executable set up in user mode for logging process creation/destruction notifications and sending blocking program path to our kernel driver.
	+ This Client logs process creation/destruction notifcations in console
	+ This Client also post a toast notification whenver a program is blocked from running.
- A client helper set up in user mode for posting toast notification containing information sent from client executable

## Project Structure

## Project Requirements

## Project Build

## Project Run

# Reference
- Windows Kernel Programming Book by Pavel Yosifovich