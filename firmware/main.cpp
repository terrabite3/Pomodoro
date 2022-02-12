
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include <util/delay_basic.h>

/* Pin map

PB0     hack header
PB1     hack header
PB2     hack header
PB3     MOSI (JTAG header)
PB4     MISO (JTAG header)
PB5     SCK (JTAG header)
PB6     XTAL
PB7     XTAL

PC0     SEG0
PC1     SEG1
PC2     SEG2
PC3     SEG3
PC4     COLON1
PC5     COLON2
PC6     RESET (button)

PD0     DIGIT0
...     ...
PD7     DIGIT7

ADC6    pot
ADC7    hack header

*/

#define RESET_TIME 25 * 60

volatile int16_t countdown = RESET_TIME;

ISR(TIMER1_COMPA_vect)
{
    countdown--;
}

void setupTimer()
{
    // setup the timer to fire every 1 second
    
    // Timer count calculated with https://eleccelerator.com/avr-timer-calculator/
    // 12 MHz, 16-bit, Prescaler = Clk/256, Real Time = 1 second
    TCCR1A = 0;         // Nothing needed here
    TCCR1B =  1<<WGM12  // CTC mode (clear timer on compare match)
            | 1<<CS12;  // Clk/256 prescaler
    OCR1A = 46875;

    TIMSK = 1<<OCIE1A;  // Enable the interrupt for output compare A match

    sei(); // Enable global interrupt flag
}

uint8_t digitCode0 = 0;
uint8_t digitCode1 = 0;
uint8_t digitCode2 = 0;
uint8_t digitCode3 = 0;

void noDigit()
{
    PORTC = 0;
}

void nextDigit()
{
    static uint8_t digit = 0;

    digit = (digit + 1) % 4;

    // Reset segment drivers
    PORTC &= ~(1<<PC0 | 1<<PC1 | 1<<PC2 | 1<<PC3);

    switch (digit)
    {
    case 0:
        PORTC |= 1<<PC0;
        PORTD = digitCode0;
        break;
    case 1:
        PORTC |= 1<<PC1;
        PORTD = digitCode1;
        break;
    case 2:
        PORTC |= 1<<PC2;
        PORTD = digitCode2;
        break;
    case 3:
        PORTC |= 1<<PC3;
        PORTD = digitCode3;
        break;
    }
}

uint8_t numberToCode(uint8_t number)
{
    switch (number)
    {
    case 0:
        return 0b01111110;
    case 1:
        return 0b00001100;
    case 2:
        return 0b10110110;
    case 3:
        return 0b10011110;
    case 4:
        return 0b11001100;
    case 5:
        return 0b11011010;
    case 6:
        return 0b11111010;
    case 7:
        return 0b00001110;
    case 8:
        return 0b11111110;
    case 9:
        return 0b11011110;
    case 10: // All off
        return 0b00000000;
    default: // Light decimal point to indicate error
        return 0b00000001;
    }
}

void updateDigits()
{
    // Normal behavior
    if (countdown >= 0)
    {
        uint8_t minutes = countdown / 60;
        uint8_t minutesOnes = minutes % 10;
        uint8_t minutesTens = minutes / 10;

        uint8_t seconds = countdown % 60;
        uint8_t secondsOnes = seconds % 10;
        uint8_t secondsTens = seconds / 10;

        digitCode0 = numberToCode(minutesTens);
        digitCode1 = numberToCode(minutesOnes);
        digitCode2 = numberToCode(secondsTens);
        digitCode3 = numberToCode(secondsOnes);
    }
    // Countdown has underflowed, less than a minute ago
    // Blink 00:00
    else if (countdown >= -60) 
    {
        if (countdown % 2)
        {
            digitCode0 = numberToCode(10);
            digitCode1 = numberToCode(10);
            digitCode2 = numberToCode(10);
            digitCode3 = numberToCode(10);
        }
        else
        {
            digitCode0 = numberToCode(0);
            digitCode1 = numberToCode(0);
            digitCode2 = numberToCode(0);
            digitCode3 = numberToCode(0);
        }
    }
    else
    {
        // Go into low power mode until the reset button is pressed
        cli(); // Disable interrupts

        // Turn off all LEDs and set pins as inputs
        PORTC = 0;
        PORTD = 0;
        DDRC = 0;
        DDRD = 0;

        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_mode();
    }
}

int main(void)
{
    setupTimer();

    // AVCC reference, ADC6
    ADMUX = 1<<REFS0 | 6;
    // Enable the ADC, and set it to maximum clock division. We don't need it to be fast.
    ADCSRA = 1<<ADEN | 7;


    DDRC |= 1<<PC0 | 1<<PC1 | 1<<PC2 | 1<<PC3 | 1<<PC4 | 1<<PC5;
    DDRD = 0xFF;

    uint16_t adcValue = 1023;
    while (1)
    {
        // If there isn't currently an ADC conversion, read the value and start a new conversion
        if (!(ADCSRA & (1<<ADSC)))
        {
            adcValue = ADC;
            ADCSRA |= 1<<ADSC;
        }

        updateDigits();

        // If the brightness is turned down, turn it on for the first 10 seconds anyways,
        // dimming over 10 seconds
        if (countdown > RESET_TIME - 10)
        {
            int16_t secondsElapsed = RESET_TIME - countdown;
            int16_t minBrightness = 1023 - 100 * secondsElapsed;
            if (adcValue < minBrightness)
                adcValue = minBrightness;
        }
        // Blink anyways when the countdown ends
        else if (countdown <= 0)
        {
            adcValue = 1023;
        }

        if (adcValue == 0)
        {
            noDigit();
            _delay_loop_2(1024);
        }
        else if (adcValue >= 1023)
        {
            nextDigit();
            _delay_loop_2(1024);
        }
        else
        {
            nextDigit();
            // Keep the digit on for a time determined by the ADC
            _delay_loop_2(adcValue);

            noDigit();
            // Turn off for the inverse
            _delay_loop_2(1024 - adcValue - 1);
        }

    }
}
