```
rosrun <package> <rtcComp> -o manager.modules.load_path:`pkg-config execution_context_shmtime --variable prefix`/lib  -o manager.modules.preload:ShmTimePeriodicExecutionContext.so -o exec_cxt.periodic.type:ShmTimePeriodicExecutionContext -o exec_cxt.periodic.rate:10
```
