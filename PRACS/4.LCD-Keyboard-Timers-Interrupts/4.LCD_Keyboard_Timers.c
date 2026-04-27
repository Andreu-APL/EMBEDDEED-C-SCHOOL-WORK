#include "MKL25Z4.h"
#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"

/* LCD pins (PORTC) */
#define LCD_RS_PIN 0U
#define LCD_EN_PIN 1U
#define LCD_D4_PIN 2U
#define LCD_D5_PIN 3U
#define LCD_D6_PIN 4U
#define LCD_D7_PIN 5U

/* Keyboard pins */
#define KBD_ROW1_PIN 20U   /* PORTE */
#define KBD_ROW2_PIN 21U
#define KBD_ROW3_PIN 22U
#define KBD_ROW4_PIN 23U
#define KBD_COL1_PIN 29U   /* PORTE */
#define KBD_COL2_PIN 30U
#define KBD_COL3_PIN  0U   /* PORTB */
#define KBD_COL4_PIN  1U

/* Button pins (PORTA) */
#define BTN1_PIN 1U   /* PTA1 */
#define BTN2_PIN 2U   /* PTA2 */

/* Tracks which part is running so the ISR knows what to do */
volatile uint8_t part   = 1;
volatile uint8_t paused = 0;   /* used in Part 3 */

/* ---- Utilities ---- */

void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms * 7000; i++) __NOP();
}

void my_itoa(int val, char *str) {
    if (val == 0) { str[0] = '0'; str[1] = '\0'; return; }
    char tmp[10]; int i = 0, j = 0;
    while (val > 0) { tmp[i++] = (val % 10) + '0'; val /= 10; }
    while (i > 0)   { str[j++] = tmp[--i]; }
    str[j] = '\0';
}

/* ---- LCD (4-bit mode) ---- */

void LCD_Nibble(uint8_t nibble, uint8_t rs) {
    if (rs) PTC->PSOR = (1U << LCD_RS_PIN);
    else     PTC->PCOR = (1U << LCD_RS_PIN);
    PTC->PCOR = (0x0F << LCD_D4_PIN);
    PTC->PSOR = ((nibble & 0x0F) << LCD_D4_PIN);
    PTC->PSOR = (1U << LCD_EN_PIN); __NOP();
    PTC->PCOR = (1U << LCD_EN_PIN);
    delay_ms(1);
}

void LCD_Command(uint8_t cmd)  { LCD_Nibble(cmd >> 4, 0); LCD_Nibble(cmd & 0x0F, 0); }
void LCD_Data(uint8_t data)    { LCD_Nibble(data >> 4, 1); LCD_Nibble(data & 0x0F, 1); }
void LCD_String(char *str)     { while (*str) LCD_Data(*str++); }
void LCD_SetCursor(uint8_t row, uint8_t col) {
    LCD_Command(0x80 | ((row ? 0x40 : 0x00) + col));
}

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
    LCD_Nibble(0x03, 0); delay_ms(5);
    LCD_Nibble(0x03, 0); delay_ms(1);
    LCD_Nibble(0x03, 0);
    LCD_Nibble(0x02, 0);  /* 4-bit mode */
    LCD_Command(0x28);    /* 2 lines, 5x7 */
    LCD_Command(0x0C);    /* display on, cursor off */
    LCD_Command(0x06);    /* auto-increment */
    LCD_Command(0x01);    /* clear */
    delay_ms(2);
}

/* ---- 4x4 Keyboard ---- */

void KBD_Init(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTE_MASK;
    PORTE->PCR[KBD_ROW1_PIN] = PORT_PCR_MUX(1);
    PORTE->PCR[KBD_ROW2_PIN] = PORT_PCR_MUX(1);
    PORTE->PCR[KBD_ROW3_PIN] = PORT_PCR_MUX(1);
    PORTE->PCR[KBD_ROW4_PIN] = PORT_PCR_MUX(1);
    PTE->PDDR |= (1U<<KBD_ROW1_PIN)|(1U<<KBD_ROW2_PIN)|(1U<<KBD_ROW3_PIN)|(1U<<KBD_ROW4_PIN);
    PTE->PSOR  = (1U<<KBD_ROW1_PIN)|(1U<<KBD_ROW2_PIN)|(1U<<KBD_ROW3_PIN)|(1U<<KBD_ROW4_PIN);
    PORTE->PCR[KBD_COL1_PIN] = PORT_PCR_MUX(1)|PORT_PCR_PE_MASK|PORT_PCR_PS_MASK;
    PORTE->PCR[KBD_COL2_PIN] = PORT_PCR_MUX(1)|PORT_PCR_PE_MASK|PORT_PCR_PS_MASK;
    PORTB->PCR[KBD_COL3_PIN] = PORT_PCR_MUX(1)|PORT_PCR_PE_MASK|PORT_PCR_PS_MASK;
    PORTB->PCR[KBD_COL4_PIN] = PORT_PCR_MUX(1)|PORT_PCR_PE_MASK|PORT_PCR_PS_MASK;
    PTE->PDDR &= ~((1U<<KBD_COL1_PIN)|(1U<<KBD_COL2_PIN));
    PTB->PDDR &= ~((1U<<KBD_COL3_PIN)|(1U<<KBD_COL4_PIN));
}

char KBD_GetKey(void) {
    const char keys[4][4] = {{'1','2','3','A'},{'4','5','6','B'},{'7','8','9','C'},{'*','0','#','D'}};
    uint32_t rows[4] = {KBD_ROW1_PIN, KBD_ROW2_PIN, KBD_ROW3_PIN, KBD_ROW4_PIN};
    for (int r = 0; r < 4; r++) {
        PTE->PCOR = (1U << rows[r]);
        delay_ms(5);
        uint8_t c1=!(PTE->PDIR&(1U<<KBD_COL1_PIN)), c2=!(PTE->PDIR&(1U<<KBD_COL2_PIN));
        uint8_t c3=!(PTB->PDIR&(1U<<KBD_COL3_PIN)), c4=!(PTB->PDIR&(1U<<KBD_COL4_PIN));
        PTE->PSOR = (1U << rows[r]);
        if (c1) return keys[r][0]; if (c2) return keys[r][1];
        if (c3) return keys[r][2]; if (c4) return keys[r][3];
    }
    return 0;
}

/* ---- TPM0 Timer (48MHz / 128 = 375kHz) ---- */

void Timer_Init(void) {
    SIM->SCGC6 |= SIM_SCGC6_TPM0_MASK;
    SIM->SOPT2 |= SIM_SOPT2_TPMSRC(1);
    TPM0->SC = TPM_SC_PS(7);  /* prescaler 128 */
}

/* Returns 1 after ~1 second, or 0 if paused flag was set mid-wait */
uint8_t Timer_Wait1s(void) {
    TPM0->CNT = 0;
    TPM0->MOD = 375000 - 1;
    TPM0->SC |= TPM_SC_CMOD(1);
    while (!(TPM0->SC & TPM_SC_TOF_MASK)) { if (paused) break; }
    TPM0->SC |= TPM_SC_TOF_MASK;
    TPM0->SC &= ~TPM_SC_CMOD(1);
    return !paused;
}

/* ---- Button / Interrupt Init ---- */

void BTN_Init(void) {
    NVIC_DisableIRQ(PORTA_IRQn);   /* disable before configuring */
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;
    /* falling-edge interrupt (IRQC = 1010b), internal pull-up */
    PORTA->PCR[BTN1_PIN] = PORT_PCR_MUX(1)|PORT_PCR_PE_MASK|PORT_PCR_PS_MASK|PORT_PCR_IRQC(0xA);
    PORTA->PCR[BTN2_PIN] = PORT_PCR_MUX(1)|PORT_PCR_PE_MASK|PORT_PCR_PS_MASK|PORT_PCR_IRQC(0xA);
    PTA->PDDR &= ~((1U<<BTN1_PIN)|(1U<<BTN2_PIN));
    PORTA->ISFR = 0xFFFFFFFF;      /* clear any spurious flags */
    NVIC_EnableIRQ(PORTA_IRQn);
}

/* ---- ISR — Port A ---- */

void PORTA_IRQHandler(void) {
    uint32_t flags = PORTA->ISFR;
    PORTA->ISFR = 0xFFFFFFFF;  /* MUST clear flags or ISR loops forever */

    if (part == 1) {
        /* Part 1: PTA1 -> flash green briefly */
        if (flags & (1U<<BTN1_PIN)) { LED_GREEN_ON(); delay_ms(500); LED_GREEN_OFF(); }

    } else if (part == 2) {
        /* Part 2: ISFR tells buttons apart — PTA1=green, PTA2=blue */
        if (flags & (1U<<BTN1_PIN)) { LED_GREEN_ON(); delay_ms(300); LED_GREEN_OFF(); }
        if (flags & (1U<<BTN2_PIN)) { LED_BLUE_ON();  delay_ms(300); LED_BLUE_OFF();  }

    } else {
        /* Part 3: PTA1 halts the ascending counter */
        if (flags & (1U<<BTN1_PIN)) paused = 1;
    }
}

/* ===========================================================
 * Main — parts run sequentially
 * =========================================================*/

int main(void) {
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();

    LCD_Init();
    KBD_Init();
    Timer_Init();
    BTN_Init();
    LED_RED_INIT(LOGIC_LED_OFF);
    LED_GREEN_INIT(LOGIC_LED_OFF);
    LED_BLUE_INIT(LOGIC_LED_OFF);

    /* ---- Part 1: simple GPIO interrupt ---- */
    part = 1;
    LCD_Command(0x01);
    LCD_SetCursor(0, 0); LCD_String("Part 1: ISR");
    LCD_SetCursor(1, 0); LCD_String("PTA1 -> Green");
    for (int i = 0; i < 15; i++) {   /* toggle red for ~15 seconds */
        LED_RED_ON();  delay_ms(500);
        LED_RED_OFF(); delay_ms(500);
    }
    LED_RED_OFF();

    /* ---- Part 2: two buttons, one port interrupt ---- */
    part = 2;
    LCD_Command(0x01);
    LCD_SetCursor(0, 0); LCD_String("Part 2: 2 Btns");
    LCD_SetCursor(1, 0); LCD_String("PTA1=G  PTA2=B");
    for (int i = 0; i < 15; i++) {
        LED_RED_ON();  delay_ms(500);
        LED_RED_OFF(); delay_ms(500);
    }
    LED_RED_OFF();

    /* ---- Part 3: event counter with pause/resume ---- */
    part = 3;
    paused = 0;
    int count = 0;
    char buf[10];

    LCD_Command(0x01);
    LCD_SetCursor(0, 0); LCD_String("Count: 0");
    LCD_SetCursor(1, 0); LCD_String("BTN1=Pause");

    while (1) {
        if (paused) {
            LCD_Command(0x01);
            LCD_SetCursor(0, 0); LCD_String("PAUSED");
            LCD_SetCursor(1, 0); LCD_String("* = Resume");

            char k = 0;
            while (k != '*') k = KBD_GetKey();
            paused = 0;

            LCD_Command(0x01);
            LCD_SetCursor(0, 0); LCD_String("Count: ");
            LCD_SetCursor(1, 0); LCD_String("BTN1=Pause");
        }

        LCD_SetCursor(0, 7);
        my_itoa(count, buf);
        LCD_String(buf); LCD_String("   ");

        if (Timer_Wait1s()) count++;
    }

    return 0;
}
