/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "HF-Alicat-BASIS2 Driver", "index.html", [
    [ "hf-alicat-basis2-driver", "index.html", null ],
    [ "API reference", "md_docs_2api__reference.html", [
      [ "Driver<UartT> (alicat_basis2.hpp)", "md_docs_2api__reference.html#autotoc_md2", [
        [ "Construction", "md_docs_2api__reference.html#autotoc_md3", null ],
        [ "Live data", "md_docs_2api__reference.html#autotoc_md4", null ],
        [ "Setpoint / control (controllers only)", "md_docs_2api__reference.html#autotoc_md5", null ],
        [ "Configuration", "md_docs_2api__reference.html#autotoc_md6", null ],
        [ "Bus reconfiguration", "md_docs_2api__reference.html#autotoc_md7", null ],
        [ "Raw Modbus primitives", "md_docs_2api__reference.html#autotoc_md8", null ]
      ] ],
      [ "UartInterface<Derived> (alicat_basis2_uart_interface.hpp)", "md_docs_2api__reference.html#autotoc_md9", null ],
      [ "Result types (alicat_basis2_types.hpp)", "md_docs_2api__reference.html#autotoc_md10", null ],
      [ "Decoded data structures (alicat_basis2_types.hpp)", "md_docs_2api__reference.html#autotoc_md11", null ],
      [ "Constants and helpers", "md_docs_2api__reference.html#autotoc_md12", null ]
    ] ],
    [ "CMake integration", "md_docs_2cmake__integration.html", [
      [ "add_subdirectory()", "md_docs_2cmake__integration.html#autotoc_md14", null ],
      [ "Manual include of the build-settings file", "md_docs_2cmake__integration.html#autotoc_md15", null ],
      [ "ESP-IDF component wrapper", "md_docs_2cmake__integration.html#autotoc_md16", null ],
      [ "Variables exported by hf_alicat_basis2_build_settings.cmake", "md_docs_2cmake__integration.html#autotoc_md17", null ],
      [ "Generated header", "md_docs_2cmake__integration.html#autotoc_md18", null ]
    ] ],
    [ "Examples", "md_docs_2examples.html", [
      [ "Build with the project tools (recommended)", "md_docs_2examples.html#autotoc_md24", null ],
      [ "Build with idf.py directly", "md_docs_2examples.html#autotoc_md25", null ],
      [ "Layout", "md_docs_2examples.html#autotoc_md26", null ],
      [ "What the comprehensive example demonstrates", "md_docs_2examples.html#autotoc_md27", null ]
    ] ],
    [ "Hardware setup — RS-485 bus", "md_docs_2hardware__setup.html", [
      [ "Connector pinout", "md_docs_2hardware__setup.html#autotoc_md29", null ],
      [ "RS-485 multidrop topology", "md_docs_2hardware__setup.html#autotoc_md30", null ],
      [ "Transceiver wiring (ESP32-S3 reference)", "md_docs_2hardware__setup.html#autotoc_md31", null ],
      [ "First power-up — assigning addresses", "md_docs_2hardware__setup.html#autotoc_md32", null ],
      [ "Baud-rate change (datasheet warning)", "md_docs_2hardware__setup.html#autotoc_md33", null ]
    ] ],
    [ "HF-Alicat-BASIS2 documentation", "md_docs_2index.html", [
      [ "Documentation structure", "md_docs_2index.html#autotoc_md35", [
        [ "Getting started", "md_docs_2index.html#autotoc_md36", null ],
        [ "Hardware & integration", "md_docs_2index.html#autotoc_md37", null ],
        [ "Reference & examples", "md_docs_2index.html#autotoc_md38", null ],
        [ "Manufacturer", "md_docs_2index.html#autotoc_md39", null ]
      ] ],
      [ "Visual overview", "md_docs_2index.html#autotoc_md40", null ]
    ] ],
    [ "Installation", "md_docs_2installation.html", [
      [ "Requirements", "md_docs_2installation.html#autotoc_md42", null ],
      [ "Layout", "md_docs_2installation.html#autotoc_md43", null ],
      [ "Vendoring as a Git submodule (recommended)", "md_docs_2installation.html#autotoc_md44", null ],
      [ "Vendoring as an ESP-IDF component", "md_docs_2installation.html#autotoc_md45", null ],
      [ "Verifying the version header", "md_docs_2installation.html#autotoc_md46", null ]
    ] ],
    [ "Modbus-RTU protocol", "md_docs_2modbus__protocol.html", [
      [ "Frame format", "md_docs_2modbus__protocol.html#autotoc_md48", null ],
      [ "Register map highlights", "md_docs_2modbus__protocol.html#autotoc_md49", [
        [ "Instantaneous data (one Modbus burst, 10 registers)", "md_docs_2modbus__protocol.html#autotoc_md50", null ],
        [ "Identity (read once at boot)", "md_docs_2modbus__protocol.html#autotoc_md51", null ],
        [ "Setpoint (controllers only)", "md_docs_2modbus__protocol.html#autotoc_md52", null ],
        [ "Configuration", "md_docs_2modbus__protocol.html#autotoc_md53", null ]
      ] ],
      [ "Exception handling", "md_docs_2modbus__protocol.html#autotoc_md54", null ]
    ] ],
    [ "Quick start", "md_docs_2quickstart.html", [
      [ "Implement the CRTP adapter", "md_docs_2quickstart.html#autotoc_md56", null ],
      [ "Construct the driver", "md_docs_2quickstart.html#autotoc_md57", null ],
      [ "Read the identity once at boot", "md_docs_2quickstart.html#autotoc_md58", null ],
      [ "Periodically pull live data", "md_docs_2quickstart.html#autotoc_md59", null ],
      [ "Common control operations", "md_docs_2quickstart.html#autotoc_md60", null ],
      [ "Bus discovery (first power-up)", "md_docs_2quickstart.html#autotoc_md61", null ]
    ] ],
    [ "Troubleshooting", "md_docs_2troubleshooting.html", [
      [ "Extra logging", "md_docs_2troubleshooting.html#autotoc_md63", null ],
      [ "Filing an issue", "md_docs_2troubleshooting.html#autotoc_md64", null ]
    ] ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", null ],
        [ "Functions", "namespacemembers_func.html", null ],
        [ "Variables", "namespacemembers_vars.html", null ],
        [ "Enumerations", "namespacemembers_enum.html", null ]
      ] ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", null ],
        [ "Functions", "functions_func.html", null ],
        [ "Variables", "functions_vars.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", null ],
        [ "Functions", "globals_func.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"alicat__basis2_8hpp.html",
"md_docs_2api__reference.html#autotoc_md12",
"structalicat__basis2_1_1InstantaneousData.html#ad7cab1a7ce4d196313acb7e9a6068874"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';