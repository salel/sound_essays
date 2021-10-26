#pragma once

#include <functional>

// Describes an option for the program.
// long_name : name expected after two dashes (e.g. "--port", field contains "port")
// short_name : name expected after only one dash (e.g. "-p", field contains "p")
// desc : description of option for help page
// func : function executed if option is present in argument list ; function takes one or zero
//        arguments depending if the next command argument is to be taken into account
//        e.g : --port 1   -> func takes one argument
//              --help     -> func takes no argument
void register_arg(
    const char* long_name,
    const char* short_name,
    const char* description,
    std::function<void(const char* s)> func);

void register_arg(
    const char* long_name,
    const char* short_name,
    const char* description,
    std::function<void(void)> func);

// Processes options previously registered
void process_args(int argc, char ** argv);

