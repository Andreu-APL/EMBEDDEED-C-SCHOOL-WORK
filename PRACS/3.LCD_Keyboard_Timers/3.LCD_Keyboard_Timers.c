/*
 * Prac3 Timer using Keypad, LCD and TPM0

 *
 * This program implements a timer where the user inputs a number
 * of seconds using a 4x4 keypad. The value is displayed on an LCD
 * and the timer starts when '*' is pressed.
 *
 * TPM0 is used to generate 1-second interrupts, incrementing a
 * counter that is shown on the LCD in real time.
 */

#include <MKL25Z4.h>
#include <stdio.h>

// --- Function Declarations ---
void delayMs(int n);
void delayUs(int n);

void keypad_init(void);
unsigned char keypad_getkey(void);

void LED_init(void);
void LED_set(int value);

void LCD_init(void);
void LCD_byte(unsigned char data);
void LCD_command(unsigned char command);
void LCD_data(unsigned char data);
void LCD_sendstring(char *str);

//TIMER
void TPM0_init(void);


#define RS 0x04
#define RW 0x10
#define EN 0x20

char buffer[5];
int index= 0;
int segundos= 0;
int contador= 0;
int running= 0;

int main(void)
{
    unsigned char key;
    unsigned char last_key = 255;

    //initialization
    keypad_init();
    LED_init();
    LCD_init();

    LCD_command(0x01); // Clear screen
    delayMs(2);
    LCD_command(0x80); // Move cursor to line 1
    LCD_sendstring("Hello");
    delayMs(5000);

    LCD_command(0x01);
    LCD_command(0x80);
    LCD_sendstring("Cuantos seg: ");
    LCD_command(0xC0); // Move cursor to line 2


    for(;;)
    {
        key = keypad_getkey();

        // If no key is pressed, reset the last_key state so we can press the same key twice
        if (key == 255)
        {
            last_key = 255;
        }
        // If a valid key is pressed and it's a new press (not held down)
        else if (key != last_key)
        {
            last_key = key;

            if (key <= 9)
            {
            	if (index < 4)
            	{
            		 buffer[index++] = key + '0';
            		 buffer[index] = '\0';
            		 LCD_command(0xC0);
            		 LCD_sendstring("                ");
            		 LCD_command(0xC0);
            		 LCD_sendstring(buffer);
            	}

            }
            // Confirmar con *
            else if (key == 14)
            {
               segundos = atoi(buffer);
               index = 0;
               contador = 0;
               running = 1;
               LCD_command(0x01);
               LCD_command(0x80);
               LCD_sendstring("Counting...");
             }
          }
        // Mostrar conteo
          if (running)
          {
              char temp[16];
              sprintf(temp, "t=%d/%d", contador, segundos);
              LCD_command(0xC0);
              LCD_sendstring("                ");
              LCD_command(0xC0);
              LCD_sendstring(temp);

              delayMs(200);

              if (contador >= segundos)
              {
                  running = 0;

                  LCD_command(0x01);
                  LCD_sendstring("FIN!");

                  LED_set(1);
                 }
              }
          }
      }



void keypad_init(void)
{
    SIM->SCGC5 |= 0x0800;
    PORTC->PCR[0] = 0x103;
    PORTC->PCR[1] = 0x103;
    PORTC->PCR[2] = 0x103;
    PORTC->PCR[3] = 0x103;
    PORTC->PCR[4] = 0x103;
    PORTC->PCR[5] = 0x103;
    PORTC->PCR[6] = 0x103;
    PORTC->PCR[7] = 0x103;
    PTC->PDDR = 0x0F;
}

unsigned char keypad_getkey(void)
{
    int row, col;
    const char row_select[] = {0x01, 0x02, 0x04, 0x08};

    const unsigned char keypad_map[4][4] = {
        {1,2,3,10},
        {4,5,6,11},
        {7,8,9,12},
        {14,0,15,13}
    };

    PTC->PDDR |= 0x0F;
    PTC->PCOR = 0x0F;
    delayUs(2);
    col = PTC->PDIR & 0xF0;
    PTC->PDDR = 0;
    if (col == 0xF0) return 255;

    for (row = 0; row < 4; row++)
    {
        PTC->PDDR = 0;
        PTC->PDDR |= row_select[row];
        PTC->PCOR = row_select[row];

        delayUs(2);
        col = PTC->PDIR & 0xF0;

        if (col != 0xF0) break;
    }

    PTC->PDDR = 0;

    if (row == 4) return 255;

    if (col == 0xE0) return keypad_map[row][0];
    if (col == 0xD0) return keypad_map[row][1];
    if (col == 0xB0) return keypad_map[row][2];
    if (col == 0x70) return keypad_map[row][3];

    return 255;
}

// ==================
// RGB LED FUNCTIONS
// ==================
void LED_init(void)
{
    SIM->SCGC5 |= 0x400;  // Port B
    SIM->SCGC5 |= 0x1000; // Port D

    PORTB->PCR[18] = 0x100;
    PTB->PDDR |= 0x40000;
    PTB->PSOR |= 0x40000;

    PORTB->PCR[19] = 0x100;
    PTB->PDDR |= 0x80000;
    PTB->PSOR |= 0x80000;

    PORTD->PCR[1] = 0x100;
    PTD->PDDR |= 0x02;
    PTD->PSOR |= 0x02;
}

void LED_set(int value)
{
    PTB->PSOR = 0x40000;
    PTB->PSOR = 0x80000;
    PTD->PSOR = 0x02;

    switch(value)
    {
        case 1: PTB->PCOR = 0x40000; break;
        case 2: PTB->PCOR = 0x80000; break;
        case 3: PTB->PCOR = 0x40000; PTB->PCOR = 0x80000; break;
        case 4: PTD->PCOR = 0x02; break;
        case 5: PTB->PCOR = 0x40000; PTD->PCOR = 0x02; break;
        case 6: PTB->PCOR = 0x80000; PTD->PCOR = 0x02; break;
        case 7: PTB->PCOR = 0x40000; PTB->PCOR = 0x80000; PTD->PCOR = 0x02; break;
        default: break;
    }
}
// ===============
// LCD FUNCTIONS (4-BIT MODE)
// ===============
void LCD_init(void)
{
    SIM->SCGC5 |= 0x1000; // Port D for Data (PTD4-PTD7)
    PORTD->PCR[0] = 0x100;
    PORTD->PCR[1] = 0x100;
    PORTD->PCR[2] = 0x100;
    PORTD->PCR[3] = 0x100;
    PORTD->PCR[4] = 0x100;
    PORTD->PCR[5] = 0x100;
    PORTD->PCR[6] = 0x100;
    PORTD->PCR[7] = 0x100;
    PTD->PDDR |= 0xFF; // all as outputs

    SIM->SCGC5 |= 0x0200; // Port A for Control (PTA2, PTA4, PTA5)
    PORTA->PCR[2] = 0x100;
    PORTA->PCR[4] = 0x100;
    PORTA->PCR[5] = 0x100;
    PTA->PDDR |= 0x34;

    // 4-Bit Initialization Sequence
    delayMs(30);
    PTA->PCOR = RS | RW;

    LCD_command(0x30);
    delayMs(5);
    LCD_command(0x30);
    delayUs(150);
    LCD_command(0x30);
    delayUs(150);


    LCD_command(0x38);
    LCD_command(0x0C);
    LCD_command(0x06);
    LCD_command(0x01);
    delayMs(2);
}

void LCD_byte(unsigned char data)
{
    // Clear the top 4 bits of Port D without affecting the rest (like PTD1)
    PTD->PCOR = 0xFF;
    // Set the 8 bits of Port D according to the data
    PTD->PSOR = (data);

    // Pulse EN
    PTA->PSOR = EN;
    delayUs(50);
    PTA->PCOR = EN;
    delayUs(50);
}

void LCD_command(unsigned char command)
{
    PTA->PCOR = RS | RW; // RS = 0, RW = 0
    LCD_byte(command);        // Send all


    if (command < 4) delayMs(2);
    else delayUs(50);
}

void LCD_data(unsigned char data)
{
    PTA->PSOR = RS; // RS = 1
    PTA->PCOR = RW; // RW = 0

    LCD_byte(data);        // Send all

    delayUs(50);
}

void LCD_sendstring(char *str)
{
    while (*str != '\0')
    {
        LCD_data(*str);
        str++;
    }
}

//TIMERS
void TPM0_init(void)
{
    SIM->SCGC6 |= 0x01000000;
    SIM->SOPT2 |= 0x01000000;

    TPM0->SC = 0;
    TPM0->SC = 0x07; // prescaler 128
    TPM0->MOD = 375000; // 1 segundo aprox

    TPM0->SC |= 0x40; // interrupcion
    NVIC_EnableIRQ(TPM0_IRQn);
}

void TPM0_IRQHandler(void)
{
    TPM0->SC |= 0x80;

    if (running)
    {
        contador++;
    }
}

void delayUs(int n) {
    volatile int i, j;
    for(i = 0; i < n; i++) {
        for(j = 0; j < 3; j++) {
            __asm("nop");
        }
    }
}

void delayMs(int n) {
    volatile int i, j;
    for(i = 0; i < n; i++) {
        for(j = 0; j < 5000; j++) {
            __asm("nop");
        }
    }
}
