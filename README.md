# Computer Technologies (3 semester) - Linux course
This repository is for the subject "Computer Technologies" (3 semester). The projects are mainly devoted to process synchronization primitives in Unix-like OS, in particular Linux. 

### Lecturer: Andrey Zhadchenko

## Tasks
* BIZZBUZZ - process an random file as follows: if there is a number divisible by 3, then we replace it with BIZZ, if it is divisible by 5, then by BUZZ, if it is divisible by both, then by BIZZBUZZ, otherwise, the string is not formatted. <br/>
Features:
    * Handling system сall errors.
    * The file is processed using a static buffer (no dynamic memory is allocated).
    * Overflow protection. The numbers can be as long as you want.
<br/>

* Pipe - implementation of the terminal operator | (conveyor). <br/>
Features:
    * Parser for terminal commands.
    * Implementation on fork(), pipe(), dup2(), execv().
    * No memory leaks.
* Signals - file transfer from the sender to the receiver only by sending signals between processes. <br/>
Features:
    * Protection against launching several senders.
    * When one of the parties dies, the second also dies.
    * Transfer rate 3 mb/s.

* Shared_stack - implementation of a stack on shared memory. <br/>
Features:
    * Critical section inside the stack (only one process can use the stack at one time).
    * When all processes are detached, the stack destructs.
    * Multiple stack operation modes: infinite element waiting, time-limited waiting or no waiting.

* Daemon - daemon-backuper. <br/>
Features: 
    * The daemon conforms to the UNIX standard.
    * User-friendly interface for communicating with a daemon.
    * Protection against the re-creation of the daemon.
    * System logging.
    * The option to change the source and destination directories.
    * 2 modes of operation: classic (creates a full copy once in a set interval), inotify (creates an incremental version when creating or deleting files). The second mode is implemented on hash tables for the ease of directory control.

* And also very easy class tasks


    
