#include <MKL25Z4.h>
#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"

/* LCD Pin Definitions (PORTC) */
#define LCD_RS_PIN 0U
#define LCD_EN_PIN 1U
#define LCD_D4_PIN 2U
#define LCD_D5_PIN 3U
#define LCD_D6_PIN 4U
#define LCD_D7_PIN 5U

/* Keyboard Pin Definitions */
/* Rows as outputs (PORTE) */
#define KBD_ROW1_PIN 20U
#define KBD_ROW2_PIN 21U
#define KBD_ROW3_PIN 22U
#define KBD_ROW4_PIN 23U
/* Cols as inputs (PORTE/PORTB) */
#define KBD_COL1_PIN 29U // PORTE
#define KBD_COL2_PIN 30U // PORTE
#define KBD_COL3_PIN 0U  // PORTB
#define KBD_COL4_PIN 1U  // PORTB

/**
 *Software-based delay in milliseconds.
 * Uses a simple NOP loop calibrated for the KL25Z core clock.
 * ms Number of milliseconds to wait.
 */
void delay_ms(uint32_t ms) {
    uint32_t i;
    for (i = 0; i < ms * 7000; i++) {
        __NOP();
    }
}

/**
 * Sends a 4-bit to the LCD.
 * Sets the RS pin based on input and toggles the EN pin to latch data.
 * nibble The 4 bits of data to send (lower 4 bits).
 * rs Register Select: 0 for Command, 1 for Data.
 */
void LCD_Nibble(uint8_t nibble, uint8_t rs) {
    if (rs) {
        PTC->PSOR = (1U << LCD_RS_PIN);
    } else {
        PTC->PCOR = (1U << LCD_RS_PIN);
    }
    
    PTC->PCOR = (0x0F << LCD_D4_PIN); // Clear data pins
    PTC->PSOR = ((nibble & 0x0F) << LCD_D4_PIN); // Set data pins
    
    PTC->PSOR = (1U << LCD_EN_PIN);
    __NOP();
    PTC->PCOR = (1U << LCD_EN_PIN);
    delay_ms(1);
}

/**
 * Sends an 8-bit command to the LCD in 4-bit mode.
 * Splits the command into high and low nibbles.
 * cmd The command byte to send.
 */
void LCD_Command(uint8_t cmd) {
    LCD_Nibble(cmd >> 4, 0);
    LCD_Nibble(cmd & 0x0F, 0);
}

/**
 * Sends an 8-bit character data to the LCD.
 * Splits the data into high and low nibbles.
 * data The character byte to display.
 */
void LCD_Data(uint8_t data) {
    LCD_Nibble(data >> 4, 1);
    LCD_Nibble(data & 0x0F, 1);
}

/**
 * Initializes the LCD in 4-bit mode.
 * Configures PORTC pins as GPIO, sets data direction, and performs the
 * power-on initialization sequence for the HD44780 controller.
 */
void LCD_Init(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;
    PORTC->PCR[LCD_RS_PIN] = PORT_PCR_MUX(1);
    PORTC->PCR[LCD_EN_PIN] = PORT_PCR_MUX(1);
    PORTC->PCR[LCD_D4_PIN] = PORT_PCR_MUX(1);
    PORTC->PCR[LCD_D5_PIN] = PORT_PCR_MUX(1);
    PORTC->PCR[LCD_D6_PIN] = PORT_PCR_MUX(1);
    PORTC->PCR[LCD_D7_PIN] = PORT_PCR_MUX(1);
    
    PTC->PDDR |= (1U << LCD_RS_PIN) | (1U << LCD_EN_PIN) | (0x0F << LCD_D4_PIN);
    
    delay_ms(20);
    LCD_Nibble(0x03, 0);
    delay_ms(5);
    LCD_Nibble(0x03, 0);
    delay_ms(1);
    LCD_Nibble(0x03, 0);
    LCD_Nibble(0x02, 0); // 4-bit mode
    
    LCD_Command(0x28); // 2 lines, 5x7
    LCD_Command(0x0C); // Display ON, Cursor OFF
    LCD_Command(0x06); // Auto-increment
    LCD_Command(0x01); // Clear
    delay_ms(2);
}

/**
 * Displays a null-terminated string on the LCD.
 * str Pointer to the character array.
 */
void LCD_String(char* str) {
    while (*str) {
        LCD_Data(*str++);
    }
}

/**
 * Sets the cursor position on the LCD grid.
 * row Row index (0 or 1).
 * col Column index (0 to 15).
 */
void LCD_SetCursor(uint8_t row, uint8_t col) {
    uint8_t addr = (row == 0) ? 0x00 : 0x40;
    LCD_Command(0x80 | (addr + col));
}

/**
 * Initializes the 4x4 Matrix Keyboard.
 * Configures Rows as GPIO outputs (driving HIGH) and Columns as
 * GPIO inputs with internal pull-up resistors.
 */
void KBD_Init(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTE_MASK;
    
    // Rows as GPIO Output
    PORTE->PCR[KBD_ROW1_PIN] = PORT_PCR_MUX(1);
    PORTE->PCR[KBD_ROW2_PIN] = PORT_PCR_MUX(1);
    PORTE->PCR[KBD_ROW3_PIN] = PORT_PCR_MUX(1);
    PORTE->PCR[KBD_ROW4_PIN] = PORT_PCR_MUX(1);
    PTE->PDDR |= (1U << KBD_ROW1_PIN) | (1U << KBD_ROW2_PIN) | (1U << KBD_ROW3_PIN) | (1U << KBD_ROW4_PIN);
    PTE->PSOR = (1U << KBD_ROW1_PIN) | (1U << KBD_ROW2_PIN) | (1U << KBD_ROW3_PIN) | (1U << KBD_ROW4_PIN);

    // Cols as GPIO Input with Pull-up
    PORTE->PCR[KBD_COL1_PIN] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    PORTE->PCR[KBD_COL2_PIN] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    PORTB->PCR[KBD_COL3_PIN] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    PORTB->PCR[KBD_COL4_PIN] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    PTE->PDDR &= ~((1U << KBD_COL1_PIN) | (1U << KBD_COL2_PIN));
    PTB->PDDR &= ~((1U << KBD_COL3_PIN) | (1U << KBD_COL4_PIN));
}

/**
 * Scans the keyboard for a single key press.
 * Iteratively drives each row LOW and checks for LOW signals on columns.
 * return ASCII character of the pressed key, or 0 if no key is pressed.
 */
char KBD_GetKey(void) {
    const char keys[4][4] = {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}
    };
    uint32_t row_pins[4] = {KBD_ROW1_PIN, KBD_ROW2_PIN, KBD_ROW3_PIN, KBD_ROW4_PIN};
    
    for (int r = 0; r < 4; r++) {
        PTE->PCOR = (1U << row_pins[r]); // Drive row LOW
        
        delay_ms(5); // Debounce/Settle
        
        uint8_t c1 = !(PTE->PDIR & (1U << KBD_COL1_PIN));
        uint8_t c2 = !(PTE->PDIR & (1U << KBD_COL2_PIN));
        uint8_t c3 = !(PTB->PDIR & (1U << KBD_COL3_PIN));
        uint8_t c4 = !(PTB->PDIR & (1U << KBD_COL4_PIN));
        
        PTE->PSOR = (1U << row_pins[r]); // Drive row HIGH back
        
        if (c1) return keys[r][0];
        if (c2) return keys[r][1];
        if (c3) return keys[r][2];
        if (c4) return keys[r][3];
    }
    return 0;
}

/**
 * Blocking function that waits for a key press and release.
 * Includes a small debounce delay.
 * return ASCII character of the pressed key.
 */
char KBD_WaitKey(void) {
    char key = 0;
    while (!(key = KBD_GetKey()));
    delay_ms(200); // Debounce
    while (KBD_GetKey()); // Wait for release
    return key;
}

/**
 * Simple Integer to ASCII conversion.
 * Converts positive integers to a null-terminated string.
 * val Integer value to convert.
 * str Destination character buffer.
 */
void my_itoa(int val, char* str) {
    if (val == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    int i = 0;
    char tmp[10];
    while (val > 0) {
        tmp[i++] = (val % 10) + '0';
        val /= 10;
    }
    int j = 0;
    while (i > 0) {
        str[j++] = tmp[--i];
    }
    str[j] = '\0';
}

/**
 * Initializes TPM0 (Timer/PWM Module) for general timing.
 * Configures the TPM clock source and sets a prescaler of 128.
 */
void Timer_Init(void) {
    SIM->SCGC6 |= SIM_SCGC6_TPM0_MASK;
    SIM->SOPT2 |= SIM_SOPT2_TPMSRC(1); // MCGFLLCLK or MCGPLLCLK/2 (typically 48MHz)
    TPM0->SC = 0; // Disable
    TPM0->SC = TPM_SC_PS(7); // Prescaler 128 -> 48MHz / 128 = 375,000 Hz
}

/**
 * Main execution loop.
 * Orchestrates Part 1 (LED Menu) and Part 2 (Ascending Timer) logic.
 */
int main(void) {
    /* Initialize Board Hardware */
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();
    
    /* Initialize Components */
    LCD_Init();
    KBD_Init();
    Timer_Init();
    
    /* Initialize RGB LEDs */
    LED_RED_INIT(LOGIC_LED_OFF);
    LED_GREEN_INIT(LOGIC_LED_OFF);
    LED_BLUE_INIT(LOGIC_LED_OFF);

    PRINTF("System Initialized\r\n");

    /* Part 2 Requirement: Print hello and stay for 5 seconds */
    LCD_Command(0x01);
    LCD_String("hello");
    delay_ms(5000);

    while (1) {
        /* Part 1: Menu and Output Management */
        LCD_Command(0x01);
        LCD_SetCursor(0, 0);
        LCD_String("  PRESS BUTTON  ");
        LCD_SetCursor(1, 0);
        LCD_String("R: 1  B: 2  G: 3");

        char key = KBD_WaitKey();
        
        if (key == '1') {
            /* Handle Red LED selection */
            LCD_Command(0x01);
            LCD_String("RED");
            LCD_SetCursor(1, 0);
            LCD_String("LED IS ON!");
            LED_RED_ON();
            delay_ms(3000);
            LED_RED_OFF();
        } else if (key == '2') {
            /* Handle Blue LED selection */
            LCD_Command(0x01);
            LCD_String("BLUE");
            LCD_SetCursor(1, 0);
            LCD_String("LED IS ON!");
            LED_BLUE_ON();
            delay_ms(3000);
            LED_BLUE_OFF();
        } else if (key == '3') {
            /* Handle Green LED selection */
            LCD_Command(0x01);
            LCD_String("GREEN");
            LCD_SetCursor(1, 0);
            LCD_String("LED IS ON!");
            LED_GREEN_ON();
            delay_ms(3000);
            LED_GREEN_OFF();
        } else if (key == '*') {
            /* Part 2: Ascending Timer Implementation */
            LCD_Command(0x01);
            LCD_String("Seconds:");
            LCD_SetCursor(1, 0);
            
            uint32_t val = 0;
            char k;
            /* Capture user numeric input */
            while (1) {
                k = KBD_WaitKey();
                if (k >= '0' && k <= '9') {
                    LCD_Data(k);
                    val = val * 10 + (k - '0');
                } else {
                    // Stop input on non-numerical key (* or #)
                    break;
                }
            }
            
            LCD_Command(0x01);
            LCD_String("Counting");
            
            /* Perform countdown using TPM hardware */
            for (int i = val; i >= 0; i--) {
                LCD_SetCursor(1, 0);
                char buf[10];
                my_itoa(i, buf);
                LCD_String(buf);
                LCD_String("    "); // Clear trailing digits
                
                // Configure TPM for 1 second wait
                TPM0->CNT = 0;
                TPM0->MOD = 375000 - 1; // 1 second period at 375kHz
                TPM0->SC |= TPM_SC_CMOD(1); // Start LPTPM counter
                
                // Poll the Timer Overflow Flag (TOF)
                while (!(TPM0->SC & TPM_SC_TOF_MASK));
                TPM0->SC |= TPM_SC_TOF_MASK; // Clear the TOF flag
                TPM0->SC &= ~TPM_SC_CMOD(1); // Stop counter
            }
            
            /* Finish Countdown */
            LCD_SetCursor(1, 0);
            LCD_String("ZERO!");
            LED_RED_ON();
            LED_GREEN_ON();
            LED_BLUE_ON();
            delay_ms(3000);
            LED_RED_OFF();
            LED_GREEN_OFF();
            LED_BLUE_OFF();
        }
    }
    return 0;
}



