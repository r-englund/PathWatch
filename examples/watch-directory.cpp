#include <pathwatch.h>

#include <iostream>


int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage " << argv[0] << " /path/to/dir/to/observ" << std::endl;
        return 0;
    }

    std::string file = argv[1];

    pathwatch::PathWatcher pathWatcher;

    pathWatcher.watch(file, [&](auto what) { 
        std::cout << what << std::endl;
    });

    std::cout << "Waiting for a change" << std::endl;
    std::cout << "Press any key to exit" << std::endl;
    std::cin.get();
    return 0;
} 