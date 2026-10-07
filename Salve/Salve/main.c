#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>


// Initialize UART for serial communication
void UART_Init(unsigned long baud)
{
    // Calculate UBRR value for selected baud rate
    uint16_t ubrr_val = F_CPU / 16 / baud - 1;

    UBRR0H = (uint8_t)(ubrr_val >> 8);
    UBRR0L = (uint8_t)ubrr_val;

	// Enable UART transmitter
    UCSR0B = (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // Set frame: 8 data bits, 1 stop bit
}

// Send one character through UART
void UART_SendChar(char c)
{
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

// Send full string through UART
void UART_SendString(char *str)
{
    while (*str) UART_SendChar(*str++);  // Send characters one by one
}



// Initialize SPI in Slave mode
void SPI_SlaveInit(void)
{
    // SPI pins direction for SLAVE:
    DDRB &= ~(1 << PB4);   // SS as input
    DDRB &= ~(1 << PB5);   // MOSI as input
    DDRB |=  (1 << PB6);   // MISO as output
    DDRB &= ~(1 << PB7);   // SCK as input

    SPCR = (1 << SPE);     // Enable SPI in slave mode
}

// Receive 1 byte from SPI master
uint8_t SPI_SlaveReceive(void)
{
    while (!(SPSR & (1 << SPIF)));
    return SPDR;
}



int main(void)
{
    char uart_buf[40];
    uint8_t temp_val;

	// Initialize serial communication
    UART_Init(9600);
	// Prepare SPI slave
    SPI_SlaveInit();

    while (1)
    {
        // Receive temperature value sent from SPI Master
        temp_val = SPI_SlaveReceive();

        // Print temperature on UART
        sprintf(uart_buf, "Temperature: %d C\r\n", temp_val);
        UART_SendString(uart_buf);

        _delay_ms(500);    // Small delay to avoid flooding
    }
}
