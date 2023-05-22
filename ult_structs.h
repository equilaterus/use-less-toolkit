#ifndef NO_INCLUDES
#include "ult_memory.h"
#endif

struct ult_state {
    struct arena Arena;
    struct subdirectories* Scripts;
    struct subdirectories* Applications;
};