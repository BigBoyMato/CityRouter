CityRouter
=============

A back-end implementation of a transport routing system working with JSON-type requests. Program is two-staged and has 2 variants of input processing: first one is responsible for building a transport catalogue database and the second one does all the needed work to process requests and give any information on optimal routes, buses or stops.
Output is given as .json and .svg format.

**Usage:**
-------

0. Downloading, installation and setting of all needed program components
1. Variant presented in main.cpp and examples.txt
2. "make_base": create transport catalogue database
3. "process_requests": build optimal routes or get info


**System requirements:**
-------

1. C++20 (STL)
2. GCC (MinGW-w64) 11.2.0


**TO DO:**
-------

1. Add city sattellite-map layer
2. Test real city maps and coordinates
3. Create desktop application


**Tech stack:**
-------

1. CMake 3.22.0
2. [Protobuf-cpp 3.18.1](https://github.com/protocolbuffers/protobuf)

