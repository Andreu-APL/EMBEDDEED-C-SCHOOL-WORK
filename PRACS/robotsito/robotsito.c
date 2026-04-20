#include "MKL25Z4.h"
#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"

int main(void) {
    /* Hardware Initialization */
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();

    PRINTF("System Initialized: robotsito\r\n");

    /* Main Application Loop */
    while (1) {
        // TODO: Write your code here
        
    }
    return 0;
}
