# Config_Tool

Config_Tool is a configuration management and version migration tool


## Basic Functionality
- execution file generation: 
```C++
make / make clean
```
- print meta data info: 
```C++
./bin/conf_test -m -i cfg/test.cfg
```
- load and build input cfg file without version migration (output location can be changed in main.c)
```C++
./bin/conf_test -i cfg/test.cfg

user input 0
```
- load and build input cfg file with version migration
```C++
./bin/conf_test -i cfg/test.cfg

user input 1 / 2
```

## Modifications could be done
- output cfg file location can be changed in main.c
- more migration version can be added in shim.h
- more setting with different traits can be added in shim.h
