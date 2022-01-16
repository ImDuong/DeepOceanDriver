# DeepOceanDriver
## Overview
- This repo represents our School Project for Course "System Programming" in RHUST. Therefore, the main goal is to revise & share knowledge with system internals and kernel driver, specifically.
- The project is named after the idea of digging inside the Windows internal system. The default Windows screen is blue, the death screen is also blue and what we do is also diving in this deep Windows ocean. On the other hand, the project name also represents the deep work of students who trying their best to achieve success and be better their "yesterdays". 
- In this project, we demonstrated a nice solution to monitor process creation in Windows. In details, we registered process creation/destruction notifications via `PsSetCreateProcessNotifyRoutineEx` in kernel mode. Therefore, we could view all informations about process creation & destruction in real-time. In user mode, we set up a client to print notifications that are from kernel driver to console. Nevertheless, we added up a feature that allow blocking program by path. Client in user mode can prevent any program from running by sending a blacklist to the driver in kernel mode. Then the driver will continuous check if any process creation starts from any path existing in the blacklist. There are 2 important things related to this feature. First, driver needs to compare paths insensitively. Second, driver should consider a mechanism to prevent the case the program changes its name. However, in this project, we only took the first case seriously and considered the second case as unpreventable. Overall, the contributions in this project are as follows:
	+ Kernel Driver regists for process creation/destruction notifications
	+ User mode Client communicate with kernel driver for logging purpose and send list of blocking program path
	+ User mode Helper helps user mode client in posting toast notification

## Architecture
- A driver (deocdrv.sys) set up in kernel mode for retrieving any process creation/destruction notifications and blocking any process creation if its path existed in blacklist.
	+ The driver is written in Cpp and targets Windows 10. Its main idea is for debugging purpose, but not production!
- An executable (DOClient.exe) set up in user mode for logging process creation/destruction notifications and sending blocking program path to our kernel driver. The client is written in Cpp.
	+ This Client logs process creation/destruction notifcations in console
	+ This Client also post a toast notification whenever a program is blocked from running.
- A client helper (DOClientHelper.exe) set up in user mode for posting toast notification containing information sent from client executable
	+ This client helper is quickly written in Golang.
- Both of these modules are targetting x64 architecture of Windows.

## Project Structure
- Basically, Deep Ocean Solution is divided in 3 main parts: DODriver, DOClient and DOClientHelper. More details about code has been written inside each individual modules. Therefore, we just highlights flows and architecture of this solution.
- The root of repository includes a solution file `Deep Ocean Driver.sln` which helps monitoring multiple projects (in this case is DOClient and DODriver).

### DODriver
- The driver regists `PsSetCreateProcessNotifyRoutineEx` for process creation/destruction notifications with function `OnProcessNotify`. This makes `OnProcessNotify` as a additional routine that system needs to run whenever there is a process creation/destruction notification. In other words, `OnProcessNotify` will always receive every process creation/destruction notification sequentially. 
- Inside `OnProcessNotify`, we set up 2 cases corresponding to process creation and process destruction notification.
	+ With process destruction notification, we simply added the notification to a linked list (the head is `ProcNotiItemsHead` and the node type is `ProcessExitInfo`), which later be sent to client mode
	+ With process creation notification, we compared the process path (if there is) with all process paths in blacklist. If the process path exists in the blacklist linked list (the head is `ProgItemsHead` and the node type is `ProcessMonitorInfo`), the driver simply adds the notification to the linked list (the head is `ProcNotiItemsHead` and the node type is `ProcessCreateInfo`) and marks this notification as blocked
- Thanks to the great power of inheritance for struct in Cpp, `ProcessCreateInfo` and `ProcessExitInfo` can inherits a same parent `ItemHeader`. Therefore, we could simply added both process creation and destruction notification to a same linkedlist, whose head is `ProcNotiItemsHead`.
- To communicate with client user mode, we use 2 main methods: Direct I/O and Buffered I/O.
	+ We use direct I/O for quickly writing notifications to the same memory that client provides
	+ The buffered I/O is used for receiving commands from cient
		+ Driver receives `IOCTL_DO_PROGRAM_BLOCK` as blocking the program by path. Whenever this IOCTL comes, the driver will store it inside the above blacklist linked list. 
		+ For `IOCTL_DO_PROGRAM_UNBLOCK`, driver will remove the requested program path from the blacklist

## Project Requirements

## Project Build

## Project Run

# Reference
- Windows Kernel Programming Book by Pavel Yosifovich