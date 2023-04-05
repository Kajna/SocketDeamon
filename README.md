# SocketDeamon
C++ blueprint of Linux deamon

A daemon is a service process that runs in the background and supervises the system or provides functionality to other processes.

Have cmake 3.1+ and gcc 9.4.0+ installed on the system. Clone repository and then run:
```
cmake .
```

```
make
```

Start daemon
```
./SocketDeamon -c /path/to/conf.conf -l /path/to/log.log -p /path/to/socketdeamonpid.lock
```

Optionally check help:
```
./SocketDeamon -h
```

Check app log to see all is alright:
```
tail -f /path/to/log.log
```

To check system log:
```
grep SocketDeamon /var/log/syslog
```

To kill process find its PID first:
```
ps -aux | grep SocketDeamon
```

Then send SIGINT signal
```
kill -INT <pid>
```
