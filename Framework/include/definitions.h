#ifndef __DEFINITIONS_H
#define __DEFINITIONS_H

#define IEE_PORT 7910

#define UEE_HOSTNAME "localhost"
#define UEE_PORT 7911
#define UEE_BISEN_PORT 7899

// op codes
#define OP_UEE_INIT 'n'
#define OP_UEE_ADD 'o'
#define OP_UEE_SEARCH 'p'
#define OP_UEE_WRITE_MAP 'q'
#define OP_UEE_READ_MAP 'r'
#define OP_UEE_CLEAR 's'

#define OP_RBISEN '1'

#define OP_IEE_DUMP_BENCH 'v'
#define OP_UEE_DUMP_BENCH 'x'

#define OP_IEE_BISEN_BULK 'z'

// debug
#define DEBUG_PRINT_BISEN_CLIENT 0

// bisen params
#define BISEN_SCORING 1

#endif
