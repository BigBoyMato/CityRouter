# CityRouter
=
City router implementation
Transport router working with JSON-type requests as input is used to build optimal transport routing throw graph map.
Output is given as JSON+SVG format. The program is build made only with standart library (as well as JSON and SVG implementations).

Usage:
- variant presented in main
- "make_base": create json-database with protobuf3 
- "process_requests": deserialization and building output-requests 

Requirements:
- Protobuf 3
- CMake
- MinGW
- C++20
