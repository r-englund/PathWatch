#include <iostream>
#include <pathwatch.h>

int main(int argc , char ** argv) {
    if (argc != 2) {
        std::cout << "Usage " << argv[0] << " /path/to/file/to/observ" << std::endl;
        return 0;
    }


    std::string file = argv[1];

   
    auto fileWatcherManager = pathwatch::FileWatcherManager::getManager();
    auto watch = fileWatcherManager->watchFile(file);
    

    watch->onChange([]() {
        std::cout << "File changed" << std::endl;
    });
   

    while (true) {



    }




    return 0;
}