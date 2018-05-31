#include <stdio.h>


//#include "command_parser.c"

int main(int argc, char argv[]) {
    // TODO: Qui ci va un Parser per le opzioni.


#ifdef __unix__
    printf("Unix");
#elif _WIN32
    printf("Windows");
#endif

    return 0;
}
