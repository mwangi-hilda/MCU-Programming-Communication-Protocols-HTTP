#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdio.h>

#define PHONE_NUMBER "+254712353402"

//usart0 function prototypes
void usart0_init(uint32_t baud_rate);
void usart0_send_char(char c);
void usart0_send_string(const char *str);

//usart0 global variables
char usart0_buffer[200];
volatile uint16_t usart0_index = 0;
volatile uint8_t new_message_flag = 0;

ISR(USART0_RX_vect){
	char c = UDR0;
	if (usart0_index < (sizeof(usart0_buffer)) - 1)
	{
		usart0_buffer[usart0_index++] = c;
		if (c == '\n')
		{
			usart0_buffer[usart0_index] = '\0';
			new_message_flag = 1;
		}
	}
}

//gsm helpers
void send_command(const char *cmd, uint16_t wait_ms);
void send_sms(const char *msg);

//hardware set-up
char prev_status[32] = "";

void set_devices(const char *status);

void fetch_process_status();

//timer1 interrupt service routine: to routinely fetch the status of the devices from the website (every 2 minutes)
void timer1_init(){
	TCCR1B |= (1<<WGM12); //CTC mode
	OCR1A = 15624; //compare value for 1HZ (1 second tick)
	TCCR1B |= (1<<CS12) | (1<<CS10); //pre-scaler 1024
	TIMSK1 |= (1<<OCIE1A); //enable timer1 compare interrupt
}

volatile uint16_t seconds_elapsed = 0;

ISR(TIMER1_COMPA_vect){
	seconds_elapsed++;
	if (seconds_elapsed >= 120)
	{
		seconds_elapsed = 0;
		fetch_process_status();
	}
}


int main(void)
{
    usart0_init(9600);
	timer1_init();
	sei();
	
	DDRB |= (1<<PB0) | (1<<PB1);
    while (1) 
    {
    }
}

void usart0_init(uint32_t baud_rate){
	uint16_t ubrr_value = (F_CPU / (8 * baud_rate)) - 1;
	UBRR0H = (unsigned char) (ubrr_value >> 8);
	UBRR0L = (unsigned char) ubrr_value;
	
	UCSR0A &= ~(1<<U2X0);
	UCSR0B |= (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0);
	UCSR0B &= ~(1<<UCSZ02);
	UCSR0C &= ~((1<<UMSEL01) | (1<<UMSEL00) | (1<<UPM01) | (1<<UPM00) | (1<<USBS0));
	UCSR0C |= (1<<UCSZ01) | (1<<UCSZ00);
}

void usart0_send_char(char c){
	while(!(UCSR0A & (1<<UDRE0)));
	UDR0 = c;
}

void usart0_send_string(const char *str){
	while(*str){
		usart0_send_char(*str++);
	}
}

void send_command(const char *cmd, uint16_t wait_ms){
	usart0_index = 0;
	new_message_flag = 0;
	usart0_send_string(cmd);
	usart0_send_string("\r");
	
	for (uint16_t i = 0; i < wait_ms; i++)
	{
		_delay_ms(1);
	}
}

void send_sms(const char *msg){
	send_command("AT+CMGF=1", 1000);
	usart0_send_string("AT+CMGS=\"");
	usart0_send_string(PHONE_NUMBER);
	usart0_send_string("\"\r");
	usart0_send_string(msg);
	usart0_send_char(26);
	_delay_ms(3000);	
}

void set_devices(const char *status){
	if (strcmp(status, "on_closed") == 0)
	{
		PORTB |= (1<<PB0);
		PORTB &= ~(1<<PB1);
		send_sms("Light is ON, Curtain is CLOSED");
	} else if (strcmp(status, "on_open") == 0)
	{
		PORTB |= (1<<PB0);
		PORTB |= (1<<PB1);
		send_sms("Light is ON, Curtain is OPEN");
	} else if (strcmp(status, "off_closed") == 0)
	{
		PORTB &= ~(1<<PB0);
		PORTB &= ~(1<<PB1);
		send_sms("Light is OFF, Curtain is CLOSED");
	} else if (strcmp(status, "off_open") == 0)
	{
		PORTB &= ~(1<<PB0);
		PORTB |= (1<<PB1);
		send_sms("Light is OFF, Curtain is OPEN");
	}
}

void fetch_process_status(){
	char status[32];
	
	send_command("AT", 1000);
	send_command("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", 1000); //setting the connection type
	send_command("AT+SAPBR=3,1,\"APN\"SAFARICOM\"", 2000); //setting the APN to be used
	send_command("AT+SAPBR=1,1", 3000); //initializing the mobile data (bearer) session
	send_command("AT+HTTPINIT", 1000); //initializing the http session
	send_command("AT+HTTPPARA=\"CID\",1", 1000);
	send_command("AT+HTTPPARA=\"URL\",\"https://homeauto.great-site.net/status.php\"", 1000); //setting the url that the http client will connect to
	send_command("AT+HTTPACTION=0", 5000); //setting our http action to GET
	send_command("AT+HTTPREAD", 2000); //read response
	
	while (!new_message_flag); //wait for response
	sscanf(usart0_buffer, "%*[^:]:%*d,%*d\r\n%[^\r\n]", status);
	new_message_flag = 0;
	
	send_command("AT+HTTPTERM", 1000); //terminating the http service
	send_command("AT+SAPBR=0,1", 2000); //close the bearer
	
	if (strcmp(status, prev_status) != 0) //check for change from previous status
	{
		strcpy(prev_status, status); //update previous status to the current status
		set_devices(status); //act and send SMS
	}
}