# System-Programming-Project

Final project for the Systems Programming Course, A.A. 2017-2018.

Authors: [Antonio M.](https://github.com/antoniomuso) [Giacomo P.](https://github.com/Pg96)

Development Technique: pair programming.

## About The Project
C implementation of an HTTP server working either in multi-threaded or multi-process mode both on Linux and Windows.
The server supports the basic access authentication method:
![](https://www.iac.rm.cnr.it/~massimo/urlsyntax.png)

It runs as a normal process on Windows systems, and as a daemon process on Linux systems. It can be queried via any Internet browser or via programs like `curl` or `wget` and supports the HTTP 1.0 protocol (all the return codes comply with the HTTP standard), namely the `GET` and `PUT` primitives.

The `GET` primitive is meant to be used for file downloads (in case the connection port is 8081, the files will preemptively be encrypted) or command executions (bash commands, extra arguments are supported as well).
The `PUT` primitive is meant to be used for file uploads to the server.

## Compiling
Under Unix systems, it suffices to call the `make` command inside the folder where the Makefile is located.
Under Windows systems, the compilation can be performed via the `mingw32-make.exe` program.

## Running
The server can be run by executing the *main* file. You can optionally use the configuration file or specify the required parameters via command line.

## Further Information
Further information can be found in the `RELAZIONE.md` file (Italian only).
