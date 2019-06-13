/*
 * TurboStick.c
 *
 * Created: 17.3.2019
 * Author : Olli Ahtola
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <math.h>

#include "usbdrv.h" // V-USB libarary

void hwInit(void);
void getStick(void);
int8_t rescale_analog(uint16_t val, uint16_t scale, int16_t offset);

// Joystick HID report descriptor for 3 full axis and 6 buttons
const char PROGMEM usbHidReportDescriptor[50] = {
	0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
	0x09, 0x04,                    // USAGE (Joystick)
	0xa1, 0x01,                    // COLLECTION (Application)
	0x09, 0x01,                    //   USAGE (Pointer)
	0xa1, 0x00,                    //   COLLECTION (Physical)
	0x09, 0x30,                    //     USAGE (X)
	0x09, 0x31,                    //     USAGE (Y)
	0x09, 0x32,                    //     USAGE (Z)
	0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
	0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
	0x75, 0x08,                    //     REPORT_SIZE (8)
	0x95, 0x03,                    //     REPORT_COUNT (3)
	0x81, 0x02,                    //     INPUT (Data,Var,Abs)
	0xc0,                          //   END_COLLECTION
	0x05, 0x09,                    //   USAGE_PAGE (Button)
	0x19, 0x01,                    //   USAGE_MINIMUM (Button 1)
	0x29, 0x06,                    //   USAGE_MAXIMUM (Button 6)
	0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
	0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
	0x95, 0x06,                    //   REPORT_COUNT (6)
	0x75, 0x01,                    //   REPORT_SIZE (1)
	0x81, 0x02,                    //   INPUT (Data,Var,Abs)
	0x95, 0x02,                    //   REPORT_COUNT (2)
	0x75, 0x01,                    //   REPORT_SIZE (1)
	0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
	0xc0                           // END_COLLECTION
};

uint8_t    idleRate;
uint8_t reportHIDData[4];

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
	usbRequest_t    *rq = (void *)data;

	if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* class request type */

		if(rq->bRequest == USBRQ_HID_GET_REPORT)
		{  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
			/* we only have one report type, so don't look at wValue */
			usbMsgPtr = (void *)&reportHIDData;
			return sizeof(reportHIDData);
		}
		else if(rq->bRequest == USBRQ_HID_GET_IDLE)
		{
			usbMsgPtr = &idleRate;
			return 1;
		}
		else if(rq->bRequest == USBRQ_HID_SET_IDLE)
		idleRate = rq->wValue.bytes[1];
	}
	else;
	/* no vendor specific requests implemented */	
	
	return 0;   /* default for not implemented requests: return no data back to host */
}

int main(void)
{
	hwInit();
	uint16_t cc;
	
	cli();
	
	usbInit();
	usbDeviceDisconnect();	/* enforce re-enumeration, do this while interrupts are disabled! */
	for(cc=0; cc <= 100; cc++)
	{
		_delay_ms(5);
	}
	usbDeviceConnect();
	sei();
	
	while (1)
	{
		getStick(); // Read analog joystick values
		reportHIDData[3] = ~((PINB & 0x1F) | ((PIND & 0x80)>>2)); // Read button values
		usbPoll();
		if(usbInterruptIsReady())
		{
			// called after every poll of the interrupt endpoint
			{
				usbSetInterrupt( reportHIDData, sizeof reportHIDData );
			}
		}
		//		_delay_us(100); // Refresh gamepad data only at 10 kHz rate
	}
}

void hwInit(void)
{
	// Setup analog data collection
	ADMUX |= (1<<REFS0); // Set REFS0 = 1
	ADMUX &= ~((1<<REFS1) | (1<<ADLAR)); // Set REFS1 = 0 and ADLAR = 0 (Use 5V ref and left adjusted results)
	ADCSRA |= (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0); // Enable ADC with 128 factor on prescaler
		
	DDRC = 0x00; // Enable analog inputs on A0-A2;
	PORTC = 0x07; // Pull-ups for A0-A2;
	
	// Digital inputs for buttons
	DDRB &= ~((1<<DDB0) | (1<<DDB1) | (1<<DDB2) | (1<<DDB3) | (1<<DDB4)); // Set PB0-PB4 as inputs
	PORTB |= ((1<<PORTB0) | (1<<PORTB1) | (1<<PORTB2) | (1<<PORTB3) | (1<<PORTB4)); // Pull-ups for PB0-PB4
	DDRD &= ~(1<<DDD7); // Set PD7 as input
	PORTD |= (1<<PORTD7); // Pull-up for PD7
}

void getStick(void)
{
	uint16_t readADC;
	
	ADMUX &= 0xF0; // Set MUX channel to A0
	ADCSRA |= (1<<ADSC); // Get converting
	while(!(ADCSRA & (1<<ADIF))); // Wait for ADC to finish
	ADCSRA |= (1 << ADIF); // Clear operating flag
	readADC = ADC;
	reportHIDData[1] = rescale_analog(readADC, 220, 25); // Report Y-axis data

	ADMUX |= 0x01; // Set MUX channel to A1
	ADCSRA |= (1<<ADSC); // Get converting
	while(!(ADCSRA & (1<<ADIF)));
	ADCSRA |= (1 << ADIF);
	readADC = ADC;
	reportHIDData[0] = rescale_analog(readADC, 210, 23) * -1; // Report X-axis data
	
	ADMUX = (ADMUX & 0xF0) | 0x02; // Set MUX channel to A2
	ADCSRA |= (1<<ADSC); // Get converting
	while(!(ADCSRA & (1<<ADIF)));
	ADCSRA |= (1 << ADIF);
	readADC = ADC;
	reportHIDData[2] = rescale_analog(readADC, 230, 31) *-1; // Report Z-axis data
}

int8_t rescale_analog(uint16_t val, uint16_t scale, int16_t offset)
{
	int16_t temp_val;
	if(val > (500 + offset) && val < (524 + offset))
		return 0;
	else
	{
		temp_val = val - (512 + offset);
		if(temp_val > 0)
		{
			temp_val -= 12;
		}
		else
		{
			temp_val += 12;
		}
		temp_val = (int) (round(temp_val * (254.0/scale)));
		if(temp_val > 127)
			return 127;
		else if(temp_val < -127)
			return -127;
		else
			return temp_val;
	}

}