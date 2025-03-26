#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
    #define DEBUG_PRINT(x) std::cout << x << std::endl
    #define DEBUG_PRINT2(x, y) std::cout << x << " " << y << std::endl
    #define DEBUG_PRINT3(x, y, z) std::cout << x << " " << y << " " << std::endl
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINT2(x, y)
    #define DEBUG_PRINT3(x, y, z)
#endif

#endif