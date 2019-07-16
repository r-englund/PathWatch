#pragma once

#ifdef PW_SHARED_BUILD
    #ifdef PW_EXPORTS
        #ifdef _WIN32
            #define PW_API __declspec(dllexport)
        #else //UNIX (GCC)
            #define PW_API __attribute__ ((visibility ("default")))
        #endif
    #else
        #ifdef _WIN32
            #define PW_API __declspec(dllimport)
        #else
            #define PW_API 
    #endif
#endif
#else
    #define PW_API 
#endif

