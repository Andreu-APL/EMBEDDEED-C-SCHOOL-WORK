# Prac 4 — LCD, Keyboard, Timers & Interrupts


## First what is an Interrupt?

Normally a microcontroller runs code **line by line**, top to bottom. This is
called **thread mode** (or the main program flow).

An **interrupt** is a signal that tells the CPU: *"stop what you're doing right
now, run this special function (the ISR), then come back exactly where you were."*

```
Main program running...
  RED LED on  →  delay  →  RED LED off  →  delay  →  ...
                                  ↑
                     User presses button (PTA1)
                                  |
                     CPU jumps to PORTA_IRQHandler()
                          GREEN LED on → delay → GREEN LED off
                                  |
                     CPU returns to main program exactly where it left off
```

This is incredibly useful in robotics — a robot can keep doing its main task
(driving, counting, displaying) and only react to events (sensors, buttons,
encoders) when they actually happen, without constantly checking.

---

## Hardware Connections

| What          | Port  | Pins used                        |
|---------------|-------|----------------------------------|
| LCD (4-bit)   | PORTC | PTC0 (RS), PTC1 (EN), PTC2–PTC5 (D4–D7) |
| Keyboard rows | PORTE | PTE20–PTE23                      |
| Keyboard cols | PORTE/PORTB | PTE29, PTE30, PTB0, PTB1   |
| Button 1      | PORTA | PTA1                             |
| Button 2      | PORTA | PTA2                             |
| Red LED       | board | Built-in (PTB18)                 |
| Green LED     | board | Built-in (PTB19)                 |
| Blue LED      | board | Built-in (PTD1)                  |

---

## Key Concepts Used

### 1. `volatile` — telling the compiler "this can change at any time"

```c
volatile uint8_t paused = 0;
```

The `volatile` keyword tells the compiler: *"don't optimize this variable away —
it can be changed by the ISR at any moment, even between two lines of main code."*

Without `volatile`, the compiler might cache the value in a register and never
re-read it from memory, so main() would never see the change the ISR made.
**Always use `volatile` for variables shared between an ISR and main code.**

### 2. `PSOR`, `PCOR`, `PDDR` — GPIO registers

The KL25Z doesn't use simple `pin = HIGH` syntax. Instead it has registers:

| Register | Name                  | What it does                        |
|----------|-----------------------|-------------------------------------|
| `PDDR`   | Port Data Direction   | Set a bit = output, clear = input   |
| `PSOR`   | Port Set Output Register | Write 1 to set that pin HIGH     |
| `PCOR`   | Port Clear Output Register | Write 1 to set that pin LOW    |
| `PDIR`   | Port Data Input Register | Read the current state of a pin  |

Example from the code — turning green LED on:
```c
LED_GREEN_ON();   // internally does PTB->PCOR = (1 << 19) because green is active-low
```

### 3. Clock gating — `SIM->SCGC5`

The KL25Z saves power by **turning off clocks to peripherals that aren't in use**.
Before using any port or timer, the clock must be enabled:

```c
SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;   // turn on the clock for Port C
```

Without this line, writing to PORTC registers does nothing.

### 4. `PORT_PCR` — Pin Control Register

Every GPIO pin has a PCR register that configures its function:

```c
PORTA->PCR[BTN1_PIN] = PORT_PCR_MUX(1)      // MUX=1 means plain GPIO
                      | PORT_PCR_PE_MASK      // enable pull resistor
                      | PORT_PCR_PS_MASK      // pull-UP (not down)
                      | PORT_PCR_IRQC(0xA);   // 0xA = interrupt on falling edge
```

The **falling edge** is when the pin goes from HIGH to LOW — i.e., when the
button is pressed (it pulls the pin down to ground).

---

## The LCD — 4-bit Mode

The LCD has an 8-bit data bus, but we only wire up 4 of those wires (D4–D7)
to save pins. This means we send each byte in **two pieces (nibbles)**:

```
Sending the letter 'H' (ASCII 0x48 = 0100 1000):
  First nibble:  send 0100  (upper 4 bits)
  Second nibble: send 1000  (lower 4 bits)
```

The **EN (Enable) pin** works like a clock pulse — you put the data on D4–D7,
then pulse EN high→low to tell the LCD "latch this data now".

The **RS (Register Select) pin** tells the LCD what kind of data it's receiving:
- RS = 0 → Command (clear screen, set cursor position, etc.)
- RS = 1 → Character data (a letter or number to display)

```c
void LCD_Nibble(uint8_t nibble, uint8_t rs) {
    // set RS pin based on command or data
    // put 4 bits on D4-D7
    // pulse EN high then low  ← this is the "latch" moment
}
```

### LCD cursor address

The LCD memory is not continuous. Row 0 starts at address `0x00`, Row 1 starts
at `0x40`. To move the cursor:

```c
LCD_Command(0x80 | address);   // 0x80 = "set cursor" command
```

So `LCD_SetCursor(1, 3)` sends `0x80 | (0x40 + 3)` = `0xC3`.

---

## The 4x4 Keyboard — Row/Column Scanning

The keyboard has 16 keys but only 8 wires (4 rows + 4 columns). The trick:

1. Drive **one row LOW** (all others stay HIGH)
2. Check all 4 **column inputs**
3. If a column reads LOW → that key is pressed (it connected the LOW row to that column)
4. Move to the next row and repeat

```
      Col1  Col2  Col3  Col4
Row1   1     2     3     A
Row2   4     5     6     B
Row3   7     8     9     C
Row4   *     0     #     D
```

If Row2 is driven LOW and Col3 reads LOW → key '6' is pressed.

The columns use **internal pull-up resistors** (`PORT_PCR_PS_MASK`) so they
normally read HIGH. When a key connects them to a LOW row, they get pulled down.

---

## The Timer — TPM0

The KL25Z has Timer/PWM Modules (TPM). We use TPM0 to create an accurate 1-second delay.

**The math:**
- System clock: 48 MHz
- Prescaler of 128: 48,000,000 / 128 = **375,000 ticks per second**
- So setting `MOD = 374,999` makes the timer overflow exactly once per second

```c
TPM0->SC = TPM_SC_PS(7);       // prescaler = 2^7 = 128
TPM0->MOD = 375000 - 1;        // count from 0 to 374999
TPM0->SC |= TPM_SC_CMOD(1);    // start the counter
while (!(TPM0->SC & TPM_SC_TOF_MASK));  // wait for overflow flag
TPM0->SC |= TPM_SC_TOF_MASK;   // clear the flag (write 1 to clear)
```

**Why a hardware timer instead of `delay_ms`?**
With `delay_ms`, the CPU is stuck in a loop doing nothing. With TPM, the timer
counts independently in hardware, so in Part 3 we can also check `if (paused)`
inside the wait loop — letting the interrupt pause the counter mid-second.

---

## The ISR — `PORTA_IRQHandler`

Port A has **one interrupt** for all its pins. When any PTA pin fires, this single
function is called. The **ISFR (Interrupt Status Flag Register)** tells us exactly
which pin triggered it:

```c
void PORTA_IRQHandler(void) {
    uint32_t flags = PORTA->ISFR;   // read which pins fired
    PORTA->ISFR = 0xFFFFFFFF;       // clear ALL flags (CRITICAL!)
    ...
}
```

> **Why must we clear ISFR?**
> The CPU checks ISFR after every ISR return. If the flag is still set, it thinks
> the interrupt is still pending and immediately calls the ISR again — forever.
> Your program would get stuck in the ISR and never return to main.

We check which pin fired with a bitmask:
```c
if (flags & (1U << BTN1_PIN))  // was it PTA1 that fired?
if (flags & (1U << BTN2_PIN))  // was it PTA2 that fired?
```

This is how Part 2 tells two buttons apart even though they share one interrupt.

### Why disable the interrupt before init?

```c
NVIC_DisableIRQ(PORTA_IRQn);   // turn off interrupt
// ... configure pins ...
PORTA->ISFR = 0xFFFFFFFF;      // clear any noise that accumulated
NVIC_EnableIRQ(PORTA_IRQn);    // turn on interrupt
```

While setting up the PCR registers, the pin might glitch and accidentally
trigger an interrupt before ready. Disabling first prevents a phantom ISR
call during initialization.

---

## Program Flow

```
Power on
   |
   ├── Init: LCD, Keyboard, Timer, Buttons, LEDs
   |
   ├── PART 1 (~15 seconds)
   |     Main: RED LED blinks continuously
   |     ISR on PTA1: flash GREEN for 500ms, return to main
   |
   ├── PART 2 (~15 seconds)
   |     Main: RED LED blinks continuously
   |     ISR on PTA1: flash GREEN
   |     ISR on PTA2: flash BLUE  ← ISFR used to tell them apart
   |
   └── PART 3 (forever)
         Main: ascending counter on LCD, 1 tick per second
         ISR on PTA1: sets paused=1
         Main sees paused=1: shows "PAUSED", waits for * key
         * pressed: clears paused=0, counter resumes
```

---

## The `part` Global Variable

```c
volatile uint8_t part = 1;
```

Because there is only **one** `PORTA_IRQHandler` for all three parts, we use
`part` to tell the ISR what behavior is expected right now. Before each section
in main, we set `part = 1`, `part = 2`, or `part = 3`.

---

## Common Beginner Mistakes to Avoid

| Mistake | Why it breaks things |
|---------|----------------------|
| Forgetting `volatile` on ISR-shared variables | Compiler caches the value; main never sees the change |
| Not clearing `PORTA->ISFR` in the ISR | ISR fires forever, program freezes |
| Forgetting `SIM->SCGC5` clock enable | Writes to port registers silently do nothing |
| Not clearing `TPM_SC_TOF_MASK` after timer overflows | Timer never fires again on the next call |
| Enabling interrupt before init is done | Spurious ISR call with half-configured hardware |
