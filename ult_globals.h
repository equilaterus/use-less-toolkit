#ifndef NO_INCLUDES
#include <stdint.h>
#endif

#define local_variable static
#define global_variable static
#define function static
#define internal static

#define Kilobytes(Value) ((Value)*1024)
#define Megabytes(Value) (Kilobytes(Value)*1024)
#define Gigabytes(Value) (Megabytes(Value)*1024)

typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;

const int PATH_SIZE = 256;
const int MAX_DIRS = 256;

const char DATA_DIR[] = "data";
const char APPLICATIONS_DIR[] = "data/applications";
const char SCRIPTS_DIR[] = "data/scripts";