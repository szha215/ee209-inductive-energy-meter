/*
 * group20.c
 *
 * Created: 9/08/2015 1:21:52 p.m.
 * Author: szha215
 *
 * Variables ending with "_value" are related to raw readings such as
 * ADC, TNCT values.
 */ 


//				 Includes
//	---------------------------------------------------
#include "prototype20.h"	// All libraries in here


//		 Global volatile variables
//	---------------------------------------------------
volatile uint16_t ADC_voltage_value = 0;
volatile uint16_t ADC_current_value = 0;
volatile uint16_t ADC_read_counter = 0;

volatile uint8_t period_value = 0;		// Number of timer OVF
volatile uint8_t period_value2 = 0;		// Remainder of timer in TCNT
volatile uint8_t phase_value = 0;		// Number of timer OVF
volatile uint8_t phase_value2 = 0;		// Remainder of timer in TCNT

volatile uint16_t timer0_overflow_counter = 0;
volatile uint16_t timer1_sec_counter = 0;
volatile uint16_t timer2_ms_counter = 0;

//		Global non volatile variables
//	---------------------------------------------------
char digits[4];			// Bytes to be transmitted


//			 ISR, sorted by priority
//	---------------------------------------------------

// The current zero crossing is used to find the period,
// phase = (period - the time difference between current and voltage)

// Current zero crossing rising edge for period
ISR(INT0_vect)
{	
	// Retrieve the timer counter then reset it
	period_value2 = TCNT0;
	TCNT0 = 0;
	
	// Store number of overflows and reset it
	period_value = timer0_overflow_counter;
	timer0_overflow_counter = 0;
}

// Voltage zero crossing rising edge for phase
ISR(INT1_vect)
{
	phase_value2 = TCNT0;					// Remainder of overflows
	phase_value = timer0_overflow_counter;	// Full number of overflows
}


// Milliseconds counter
ISR(TIMER2_COMPA_vect)
{
	TCNT2 = 0;
	timer2_ms_counter++;
}

// Seconds counter
ISR(TIMER1_COMPA_vect)
{
	TCNT1 = 0;
	timer1_sec_counter++;
}

// Number of timer0 overflow counter
ISR(TIMER0_OVF_vect)
{
	timer0_overflow_counter++;
}

// Store ADC result to variable respective to the channel and toggle channel
ISR(ADC_vect)
{	
	// If current channel is 5 (current), store ADC result in ADC_current_value, else store in ADC_voltage_value
	if (ADMUX & (1 << MUX0)){
		ADC_current_value = ADC;
		ADMUX &= 0b11111000;	// Clear last 3 bits to set channel to 0 (voltage)
	} else {
		ADC_voltage_value = ADC;
		ADMUX |= (1 << MUX2) | (1 << MUX0);		// Set channel to 5 (current)
	}
	
	// Start the next conversion
	START_CONVERSION;
	
	// Increment number of readings
	ADC_read_counter++;
}


//			 Main()
//	---------------------------------------------------
int main(void)
{
	uint16_t UBRR = F_CPU/(16UL*BAUD) - 1;
	
	float td = 0;			// Time difference between V and I
	float period = 0;
	float phase_angle = 0;	// Phase angle in radians
	
	uint16_t ADC_voltage_peak_value = 0;	// Raw ADC voltage reading
	uint16_t ADC_current_peak_value = 0;	// Raw ADC voltage reading representing current
	
	float voltage_peak_reading = 0;
	float voltage_RMS = 0;
	
	float current_peak_reading = 0;
	float current_RMS = 0;
	float current_peak = 0;
		
	float power_real = 0;
	
	float energy_joule = 0;
	uint8_t energy_mode = 0;				// 0 = J, 1 = Wh
	
	uint8_t byte_sent_counter = 0;			// A byte is one digit on CPLD
	uint8_t package_sent_counter = 0;		// A package a set of 4 digits of CPLD, 4 bytes
	uint8_t package_update_counter = 0;		// Number of times packages updated
	char unit = 'V';						// Current display mode
	
	uint16_t button_press_length = 0;		// How long the button has been pressed
	
	
	//////////////////////////////////////////////////////////////////////////
	//			Initialisations
	
	pin_init();
	UART_init(UBRR);
	ADC_init();
	timer_init();
	external_interrupt_init();
	energy_joule = EEPROM_read();

	// Start first conversion
	START_CONVERSION;

	// Global interrupt enable
	sei();
	
	while (1){
		
		//////////////////////////////////////////////////////////////////////////
		//			Phase and period calculations
		
		period = period_value*TIMER0_OVF_PERIOD + period_value2*TIMER0_PERIOD;
		td = period - (phase_value*TIMER0_OVF_PERIOD + phase_value2*TIMER0_PERIOD);
		phase_angle = calculate_phase_angle(td, period);
		
		
		//////////////////////////////////////////////////////////////////////////
		//			Voltage and current calculations
		
		// Update peak voltage reading
		if ((ADC_voltage_value > ADC_voltage_peak_value)){
			ADC_voltage_peak_value = ADC_voltage_value;
		}
		
		// Update peak current reading
		if ((ADC_current_value > ADC_current_peak_value)){
			ADC_current_peak_value = ADC_current_value;
		}
		
		// If over 200 readings have been checked, convert ADC reading to voltage
		if (ADC_read_counter >= 200){
			
			voltage_peak_reading = ((float)ADC_voltage_peak_value)/ADC_RESOLUTION * ADC_REFERENCE;
			current_peak_reading = ((float)ADC_current_peak_value)/ADC_RESOLUTION * ADC_REFERENCE;
			
			// Calculate RMS
			voltage_RMS = calculate_RMS_voltage(voltage_peak_reading);
			current_RMS = calculate_RMS_current(current_peak_reading);
			current_peak = calculate_peak_current(current_RMS);
			
			// RMS voltage should not be below 10V
			if (voltage_RMS < 10){
				voltage_RMS = 0;
				current_RMS = 0;
				current_peak = 0;
			}
			
			// RMS current should not be below 50mA
			if (current_RMS < 0.05){
				current_RMS = 0;
				current_peak = 0;
			}
			
			// Reset peaks
			ADC_read_counter = 0;
			ADC_voltage_peak_value = 0;
			ADC_current_peak_value = 0;
		}
		
		
		//////////////////////////////////////////////////////////////////////////
		//				Transmission
		
		// Transmit a digit every 5ms
		if (timer2_ms_counter >= 5){
			timer2_ms_counter = 0;
			
			// Encrypt data by XOR'ing with the group ID and transit
			UART_transmit(digits[byte_sent_counter++] ^ GROUP_ID);

			// If all 4 bytes have been sent, reset counter
			if (byte_sent_counter == 4){
				byte_sent_counter = 0;
				package_sent_counter++;
			}
		}


		//////////////////////////////////////////////////////////////////////////
		//				Power, energy calculations, display switch
		
		// Update display value every ~500ms (actual time = 512ms)
		if (package_sent_counter >= 25){
			package_sent_counter = 0;

			// Calculate power and update energy
			power_real = calculate_real_power(voltage_RMS, current_RMS, phase_angle);
			energy_joule += calculate_joule(power_real, 0.512);
			
			switch (unit){
				case 'V':
					create_package(voltage_RMS, 'V');
					break;
				case 'I':
					create_package(current_peak*1000, 'I');
					break;
				case 'P':
					create_package(power_real, 'P');
					break;
				case 'E':
					if (energy_mode == 1){
						create_package(calculate_Wh(energy_joule), 'W');
					} else {
						create_package(energy_joule, 'E');
					}
					break;
			}
			
			package_update_counter++;
		}
		
		// Switch display after ~3 seconds
		if (package_update_counter >= 6){
			package_update_counter = 0;

			switch(unit){
				case 'V':
					unit = 'I';
					break;
				case 'I':
					unit = 'P';
					break;
				case 'P':
				
					// Automatically change energy mode if value gets too high to display
					if (energy_joule >= 1000000 && energy_mode == 0){
						energy_mode = energy_mode_toggle(energy_mode);
					}
				
					unit = 'E';
					break;
				case 'E':
					unit = 'V';
					break;
			}
		}
		
		//////////////////////////////////////////////////////////////////////////
		//					Press button and EEPROM reset
		
		// Increment button pressed counter if button is pressed
		if (BUTTON_PRESSED){
			button_press_length++;
		} else {
			
			// If button has been pressed, check if it's long or short
			if (button_press_length > 5){
				
				// Short button press will toggle between energy mode (0.5s)
				if (button_press_length < 4000){
					energy_mode = energy_mode_toggle(energy_mode);
					
				// Long button press will clear EEPROM content
				} else {
					EEPROM_reset();
					energy_joule = 0;
				}
				
				// Reset button pressed counter
				button_press_length = 0;
			}
		}
		
		
		//////////////////////////////////////////////////////////////////////////
		//					Auto EEPROM update
		
		// If 28 seconds has passed, turn on LED to warn user
		if (timer1_sec_counter >= 28){
			LED_ON; 
			
			// If 30 seconds has passed, reset EEPROM
			if (timer1_sec_counter >= 30){
				timer1_sec_counter = 0;
				EEPROM_update(energy_joule);
			}
		}
		
		
		//////////////////////////////////////////////////////////////////////////
	}
		
	return 0;
}