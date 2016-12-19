#include <iostream>
#include <filewatch.h>

int main(int argc , char ** argv) {
    if (argc != 2) {
        std::cout << "Usage " << argv[0] << " /path/to/file/to/observ" << std::endl;
        return 0;
    }


    std::string file = argv[1];

    
    
    filewatch::FileWatcher fw(file);

    while (true) {



    }




    return 0;
}