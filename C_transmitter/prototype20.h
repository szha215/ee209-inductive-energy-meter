/* prototype20.c
 *
 * Created 9/08/2015 1:22:34 p.m.
 * Author: szha215
 *
 * All function declarations and defines.
 *
 */


#ifndef PROTOTYPE20_H
#define PROTOTYPE20_H

//				 Defines
//	---------------------------------------------------
#define F_CPU 16000000UL			// 16MHz CPU clock speed
#define BAUD 9600					// 9600bps baud rate
#define GROUP_ID 0b00010100			// Group ID (20)

#define START_CONVERSION ADCSRA |= (1 << ADSC)	// Start ADC conversion

#define LED_ON PORTB |= (1 << PB5)
#define LED_OFF PORTB &= ~(1 << PB5)
#define LED_TOGGLE PORTB ^= (1 << PB5)
#define BUTTON_PRESSED ~PINB & (1 << PB7)

#define ADC_RESOLUTION 1024
#define ADC_REFERENCE 3.3

#define LOWEST_PERIOD 0.00152		// Period of 540Hz signal (1.52ms)
#define HIGHEST_PERIOD 0.00185		// Period of 660Hz signal (1.85ms)
#define TIMER0_OVF_PERIOD 0.000016	// Period of one overflow in timer0
#define TIMER0_PERIOD 0.0000000625	// Period of each timer1 step

#define VOLTAGE_LEVEL_SHIFT 0.82
#define VOLTAGE_DIVIDER_RATIO 0.06822	// 8.2/(112+8.2)

#define CURRENT_LEVEL_SHIFT 1.65
#define CURRENT_GAIN 4.85				// 33/6.8
#define SHUNT_RESISTACE 0.2672			// 4x 1ohm in parallel

// Data format
// [--][----][-][-]
// POS-VALUE-DP-PARITY
#define POSITION_BIT1 (1 << 7)
#define POSITION_BIT0 (1 << 6)
#define VALUE_BIT3 (1 << 5)
#define VALUE_BIT2 (1 << 4)
#define VALUE_BIT1 (1 << 3)
#define VALUE_BIT0 (1 << 2)
#define	DP_BIT (1 << 1)
#define PARITY_BIT (1 << 0)

#define UNIT_V 0b00101000	// Voltage unit
#define UNIT_I 0b00101101	// Current unit
#define UNIT_P 0b00110000	// Power unit
#define UNIT_J 0b00110101	// Energy unit (Joules)
#define UNIT_W 0b00110110	// Energy unit (Wh)



//				 All libraries
//	---------------------------------------------------
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//				All function declarations
//	---------------------------------------------------

// Initialise pin configurations
void pin_init(void);

// Initialise UART transmitter
void UART_init(uint8_t UBRR);

// Initialise ADC
void ADC_init(void);

// Initiaise timers
void timer_init(void);

// Initialise external interrupts
void external_interrupt_init(void);

//	---------------------------------------------------

// Transmit data
void UART_transmit(uint8_t data);

// Updates global 4 bytes data for transmission
void create_package(float value, char unit);

// Creates the value byte for transmission
char create_digit(uint8_t value, uint8_t pos);

// Creates the unit byte for transmission
char create_unit(char unit);

// Creates the parity bit
char create_parity(char n);

//	---------------------------------------------------

// Calculates the RMS voltage given peak of divided voltage
float calculate_RMS_voltage(float peak_voltage);

// Calculates the peak current given the peak of amplified shunt voltage
float calculate_peak_current(float peak_current_voltage);

// Calculates the RMS current given the peak of amplified shunt voltage
float calculate_RMS_current(float peak_current_voltage);

// Calculates the phase angle in radians given the time difference and period
float calculate_phase_angle(float td, float period);

// Calculates the real power given RMS V, I and phase angle
float calculate_real_power(float V, float I, float phase);

// Calculates the energy in joules given the RMS power and time in s
float calculate_joule(float P, float time);

// Calculates the energy in Wh given the energy in joules
float calculate_Wh(float energy);

//	---------------------------------------------------

// Toggle between 1 and 0 (Wh and J)
uint8_t energy_mode_toggle(uint8_t current);

// Reads EEPROM for energy value stored
float EEPROM_read();

// Updates EEPROM with latest energy value
void EEPROM_update(float energy);

// Clear EEPROM to reset energy value stored
void EEPROM_reset();


#endif



//////////////////////////////////////////////////////////////////////////
//				Unused functions, but may be used

/*

// Calculates the reactive power given RMS V, I and phase angle
float calculate_reactive_power(float V, float I, float phase);

// Calculates the apparent power given RMS V, I and phase angle
float calculate_apparent_power(float P, float phase);

*/