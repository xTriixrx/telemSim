# TelemSim

<p align="center">

TelemSim is comprised of two different utilities with one acting as a server to process incoming spacecraft telemetry data while the other is a client service dumpign data onto the connected socket. The server utility mdp has the ability to start up as either an IPV4 or IPV6 server depending on whether or not one would like to limit the range of addresses. The client utility sim is a time simulated process and produces major frames that are dumped onto the socket connection for the mdp server process to consume.
Please feel free to give feedback or suggest enhancements to this project.

</p>

## Building

### mdp

gcc mdp.c -o mdp

### sim

gcc simulator.c -o sim

## CMD Options

### mdp

The mdp server utility requires the PORT and PROTOCOL cmd arguments.

1. ./mdp 8080 --INET
	- This is for allowing IPV4 addresses only.
2. ./mdp 8080 --INET6
	- This allows IPV6 & IPV4 addresses.
3. ./mdp 8080 --INET --debug
	- A debug option is allowed for extra standard output.

### sim

The sim client utility requires the HOST, PORT, and SEC cmd arguments.

1. ./sim 127.0.0.1 8080 30
	- Can connect to IVP4 mdp server execution only.
2. ./sim ::1 8080 30
	- Can connect to IPV4 and IPV6 mdp server execution.
3. ./sim 127.0.0.1 8080 45 --debug
	- A debug option is allowed for extra standard output.
