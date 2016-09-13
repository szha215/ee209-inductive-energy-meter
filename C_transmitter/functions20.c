/* functions20.c
 *
 * Created 9/08/2015 1:22:34 p.m.
 * Author: szha215
 *
 * All function definitions
 *
 */

#include "prototype20.h"

#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


// Global cross file variable
extern char digits[4];

//////////////////////////////////////////////////////////////////////////
//			Initialisations

// pin_init() initialises pin configurations
void pin_init(void)
{
	// Set PB5 as output (LED)
	DDRB |= (1 << PB5);
		
	// Force clear PB7 to set as input (push button)
	DDRB &= ~(1 << PB7);
	
	// Force clear PC0 to set as input (ADC0, voltage)
	DDRC &= ~(1 << PC0);
	
	// Force clear PC5 to set as input (ADC5, current)
	DDRC &= ~(1 << PC5);
	
	// Force clear PD3 to set as input (vTrig)
	DDRD &= ~(1 << PD3);
	
	// Force clear PD2 to set as input (iTrig)
	DDRD &= ~(1 << PD2);
}

// UART_init() takes an 8bit unsigned clock division pre scalar
// to initialise the UART transmitter
void UART_init(uint8_t UBRR)
{
	// Set baud rate
	UBRR0H = (UBRR >> 8);
	UBRR0L = UBRR;
	
	// Disable parity bit
	UCSR0C |= (0 << UMSEL01) | (0 << UMSEL00);
	
	// Set number of stop bits to be 1
	UCSR0C |= (1 << USBS0);
	
	// Set data frame to be 8 bits
	UCSR0C |= (1 << UCSZ00) | (1 << UCSZ01);

	// Enable transmitter
	UCSR0B |= (1 << TXEN0);
}

// ADC_init() intialises registers for the A2D converter
void ADC_init(void)
{
	/* Set ADC reference
	   0 0 = AREF, internal Vref turned off
	   0 1 = AVcc with external capacitor at AREF pin
	   1 1 = Internal 1.1V with external capacitor at AREF pin*/
	ADMUX &= ~(1 << REFS1);		// AREF, 3.3V
	ADMUX &= ~(1 << REFS0);

	// Keep converted data to be right adjusted
	ADMUX &= ~(1 << ADLAR);

	// Force clear initial channel to 0
	ADMUX &= ~(1 << MUX3);
	ADMUX &= ~(1 << MUX2);
	ADMUX &= ~(1 << MUX1);
	ADMUX &= ~(1 << MUX0);

	// Enable ADC
	ADCSRA |= (1 << ADEN);

	// Force disable auto trigger mode
	ADCSRA &= ~(1 << ADATE);
	
	// Enable interrupt
	ADCSRA |= (1 << ADIE);

	/* Force clear ADC prescaler
	   1 1 1 = 128 (125kHz)
	   1 1 0 = 64  (250kHz)
	   1 0 1 = 32  (500kHz)	*/
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1);	// 64, 250kHz
	ADCSRA &= ~(1 << ADPS0);
}

// timer_init() intialises all 3 timers
void timer_init(void)
{
	/* Timer prescaler
	   0 0 0 = no clock
	   0 0 1 = 1	(16MHz)
	   0 1 0 = 8	(2MHz)
	   0 1 1 = 32	(125kHz)
	   1 0 0 = 64	(250kHz)
	   1 0 1 = 1024	(15.625kHz)
	   1 1 0 = external clock on T0 pin, falling edge
	   1 1 1 = external clock on T0 pin, rising edge */
	
	//				TIMER0 (8bit - 256) Phase
	//	---------------------------------------------------
	
	// 1 (16MHz)
	TCCR0B |= (0 << CS02) | (0 << CS01) | (1 << CS00);
	
	// Timer overflow interrupt enable
	TIMSK0 |= (1 << TOIE0);
	
	
	//				TIMER1 (16bit - 65536) EEPROM
	//	---------------------------------------------------
	
	// 1024 (15.625kHz)
	TCCR1B |= (1 << CS12) | (0 << CS11) | (1 << CS10);
	
	// Set compare reg to 15625 (1 second)
	OCR1A = 0x3D09;
	
	// Timer interrupt enable
	TIMSK1 |= (1 << OCIE1A);
	
	
	//				TIMER2 (8bit - 256) UART
	//	---------------------------------------------------
	
	// 64 (250kHz)
	TCCR2B |= (1 << CS22) | (0 << CS21) | (0 << CS20);
	
	// Set compare reg to 250 (1 ms)
	OCR2A = 0xFA;
	
	// Timer compare interrupt enable
	TIMSK2 |= (1 << OCIE2A);
}

// external_interrupt_init() sets up voltage and current zero crossing interrupts
// for phase and period calculations
void external_interrupt_init(void)
{
	// Voltage zero crossing rising edge
	EICRA |= (1 << ISC11) | (1 << ISC10);	// INT1
	
	// Current zero crossing rising edge
	EICRA |= (1 << ISC01) | (1 << ISC00);	// INT0
	
	// Interrupt enable
	EIMSK |= (1 << INT1) | (1 << INT0);
}


//////////////////////////////////////////////////////////////////////////
//					Transmission

// UART_transmit() takes an 8bit data and transmits it via UART
void UART_transmit(uint8_t data)
{
	// Wait for empty transmit buffer
	while ( !(UCSR0A & (1 << UDRE0)) );

	// Put data into buffer, sends the data
	UDR0 = data;
}

// create_package() takes a float, unit and update the global digits[] for transmission
// if the value is > 1000, a decimal place is added before the unit to represent *1000
void create_package(float value, char unit)
{
	int i;
	int DP = 0;
	int thousands = 0;
	char digit_str[10];
	
	// If value is greater than 1000, divide it by 1000 and keep record of it
	if (value >= 1000){
		thousands = 1;
		value = value/1000;
	}
	
	// Convert double (or float) to 3sf + 2dp char array
	dtostrf(value, 3, 2, digit_str);
	
	for (i = 0; i < 4; i++){
		
		// If a decimal place is detected, the previous digit should have a DP
		if (digit_str[i] == '.'){
			DP = 1;
			if (3-i+DP != 1){
				digits[3-i+DP] |= DP_BIT;
			}
			
		// Digit format on CPLD is unit as 0, most significant digit as 3, therefore 3-i
		} else {	
			digits[3-i+DP] = create_digit(digit_str[i] - '0', 3-i+DP);
		}
	}
	
	// If the value is greater than 1000, insert DP at the end of displayed representing *1000
	if (thousands != 0){
		digits[1] |= DP_BIT;
	}
	
	// The last digit on CPLD (digit 0) is always the unit
	digits[0] = create_unit(unit);
	
	// Check and insert parity bit for each byte
	for (i = 0; i < 4; i++){
		digits[i] = create_parity(digits[i]);
	}
}

// create_digit() takes a single unsigned int, position and create a 
// package for that digit suitable for transmission
char create_digit(uint8_t value, uint8_t pos)
{
	char byte = 0b00000000;
	
	switch(pos){
		case 1:
			byte |= POSITION_BIT0;
			break;
		case 2:
			byte |= POSITION_BIT1;
			break;
		case 3:
			byte |= POSITION_BIT1 | POSITION_BIT0;
			break;
	}
	
	switch(value){
		case 1:
			byte |= VALUE_BIT0;
			break;
		case 2:
			byte |= VALUE_BIT1;
			break;
		case 3:
			byte |= VALUE_BIT1 | VALUE_BIT0;
			break;
		case 4:
			byte |= VALUE_BIT2;
			break;
		case 5:
			byte |= VALUE_BIT2 | VALUE_BIT0;
			break;
		case 6:
			byte |= VALUE_BIT2 | VALUE_BIT1;
			break;
		case 7:
			byte |= VALUE_BIT2 | VALUE_BIT1 | VALUE_BIT0;
			break;
		case 8:
			byte |= VALUE_BIT3;
			break;
		case 9:
			byte |= VALUE_BIT3 | VALUE_BIT0;
			break;
	}
	
	return byte;	
}

// create_unit() takes the unit in char and create a package suitable
// for CPLD to display
char create_unit(char unit)
{
	switch(unit){
		case 'V':
		return UNIT_V;
		case 'I':
		return UNIT_I;
		case 'P':
		return UNIT_P;
		case 'E':
		return UNIT_J;
		case 'W':
		return UNIT_W;
	}
}

// create_parity() takes a byte, add all set bits and return sum%2
char create_parity(char byte)
{
	int i;
	uint8_t parity = 0;

	// Add up every bit in the byte
	for (i = 0; i < 8; i++){
		parity += ((byte & (1 << i)) >> i);
	}

	// If odd, insert parity bit
	if (parity%2 == 1){
		byte |= PARITY_BIT;
	}

	return byte;
}


//////////////////////////////////////////////////////////////////////////
//				Calculations

/* calculate_RMS_voltage() takes the peak voltage of the voltage divider,
   calculate and return the RMS voltage in V */
float calculate_RMS_voltage(float peak_voltage)
{
	// -0.06 = calibration
	// sqrt(2) = (sinosudial peak to RMS)
	// -1.1 = calibration
	return (peak_voltage-VOLTAGE_LEVEL_SHIFT-0.06)/(VOLTAGE_DIVIDER_RATIO*M_SQRT2) - 1.1;
}

/* calculate_peak_current() takes the amplified peak voltage representing
   the current, calculate and return the actual peak current in A */
float calculate_peak_current(float current_RMS)
{
	return current_RMS*M_SQRT2;
}

/* calculate_RMS_current() takes the amplified peak voltage representing
   the current, calculate and return the RMS current in A */
float calculate_RMS_current(float current_peak_reading)
{
	// sqrt(2) = sinosudial peak to RMS
	// -0.008 = calibration
	return (current_peak_reading-CURRENT_LEVEL_SHIFT)/(CURRENT_GAIN*SHUNT_RESISTACE*M_SQRT2) - 0.008;
}

/* calculate_phase_angle() takes the time difference between vTrig rising edge
   and iTrig rising edge and the period in seconds, calculate and return the 
   phase angle between voltage and current in radians */
float calculate_phase_angle(float td, float period)
{
	return (td/period)*2*M_PI;
}

/* calculate_real_power() takes the RMS voltage in V, RMS current in I and phase
   angle in radians, calculate and return the real power in W */
float calculate_real_power(float V, float I, float phase)
{
	return fabs(V*I*cos(phase));
}

/* calculate_joule() takes the RMS power in W, time in s, calculate and
   return the energy in joules*/
float calculate_joule(float P, float time)
{
	return P*time;
}

/* calculate_Wh() takes the energy in joules, calculate and return the
   energy in Wh */
float calculate_Wh(float energy)
{
	return energy/3600;	// 1Wh = 3.6kJ
}


//////////////////////////////////////////////////////////////////////////
//				Energy and EEPROM

// energy_mode_toggle() takes the current mode and toggle it to 1 or 0 (Wh or J)
uint8_t energy_mode_toggle(uint8_t current_mode)
{
	if (current_mode == 0) {
		return 1;	// Wh
	} else {
		return 0;	// J
	}
}

// EEPROM_read() reads and return a float in EEPROM address 0
float EEPROM_read()
{
	return eeprom_read_float(0);
}

// EEPROM_update() takes a float and update it in EEPROM address 0
void EEPROM_update(const float energy)
{
	LED_ON;
	
	cli();
	eeprom_write_float(0, energy);
	sei();
	
	LED_OFF;
}

// EEPROM_reset() will clear content of EEPROM address 0
void EEPROM_reset()
{
	LED_ON;
	
	cli();
	eeprom_write_float(0, 0.0);
	sei();
	
	LED_OFF;
}


//////////////////////////////////////////////////////////////////////////
//				Unused functions, but may be used

/*

// calculate_reactive_power() takes the RMS voltage in V, RMS current in I and phase
// angle in radians, calculate and return the reactive power in var 
float calculate_reactive_power(float V, float I, float phase)
{
	return fabs(V*I*sin(phase));
}

// calculate_apparent_power() takes the average real power in W, average
//   reactive power in var, calculate and return the apparent power in VA 
float calculate_apparent_power(float P, float phase)
{
	return fabs(P/cos(phase));
}


*/