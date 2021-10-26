#include "process_args.h"
#include <iostream>
#include <cstring>
#include <vector>

using namespace std;

struct arg {
    const char* long_name;
    const char* short_name;
    const char* desc;
    function<void(const char* s)> func;
    bool no_args = false;
};

vector<arg> args;

void register_arg(
    const char* long_name,
    const char* short_name,
    const char* description,
    std::function<void(const char* s)> func,
    bool no_args) {
    args.push_back({long_name, short_name, description, func, no_args});
}

void register_arg(
    const char* long_name,
    const char* short_name,
    const char* description,
    std::function<void(const char* s)> func) {
    register_arg(long_name, short_name, description, func, false);
}

void register_arg(
    const char* long_name,
    const char* short_name,
    const char* description,
    std::function<void(void)> func) {
    register_arg(long_name, short_name, description, [&](const char* s){func();}, true);
}

void process_args(int argc, char ** argv) {
    // Cool helper page listing options
    auto print_help = [=]() {
        cout << "Usage : " << argv[0] << " [options]" << endl;
        cout << endl;
        cout << "Options : " << endl;
        for (auto ar : args) {
            cout << "\t--" << ar.long_name << " , " << "-" << ar.short_name << " : " << ar.desc << endl;
        }
        cout << endl;
        exit(0);
    };

    // no duplicates!
    vector<bool> used(args.size(), false);

    for (int i=1;i<argc;i++) {
        char* a = argv[i];
        if (strcmp(a, "--help")==0 || strcmp(a, "-h") == 0) {
            print_help();
        }
        // identify option
        if (a[0] == '-') {
            // identify short or long name
            bool long_arg = (a[1] == '-');

            // iterate through registered options
            bool found = false;
            for (size_t j=0;j<args.size();j++) {
                auto ar = args[j];
                // name matching
                if (!used[j] && ((long_arg && strcmp(&(a[2]), ar.long_name) == 0) ||
                    (!long_arg && strcmp(&(a[1]), ar.short_name) == 0 ))) {
                    if (ar.no_args) ar.func(nullptr);
                    else {
                        // get next arg and pass as argument to lambda
                        if (i == argc-1) print_help();
                        else {
                            i += 1;
                            ar.func(argv[i]);
                        }
                    }
                    found = true;
                    used[j] = true;
                    break;
                }
            }
            if (!found) {
                print_help();
            }
        } else {
            print_help();
        }
    }
}