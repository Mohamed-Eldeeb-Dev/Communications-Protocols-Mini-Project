#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>


// Initialize UART for serial communication
void UART_Init(unsigned long baud)
{
    // Calculate UBRR value for baudrate
    uint16_t ubrr_val = F_CPU / 16 / baud - 1;

    // Set baud rate registers
    UBRR0H = (uint8_t)(ubrr_val >> 8);
    UBRR0L = (uint8_t)ubrr_val;

    // Enable transmitter only
    UCSR0B = (1 << TXEN0);

    // 8-bit data, 1 stop bit
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

// Send one character over UART
void UART_SendChar(char c)
{
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

// Send a full string over UART
void UART_SendString(char *str)
{
    while (*str) UART_SendChar(*str++);
}




// Initialize TWI bus (I2C)
void TWI_Init(void)
{
    TWSR = 0x00;   // Prescaler = 1
    TWBR = 32;
}

// Send I2C Start condition
void TWI_Start(void)
{
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

// Send I2C Stop condition
void TWI_Stop(void)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO);
}

// Write one byte to I2C bus
void TWI_Write(uint8_t data)
{
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while (!(TWCR & (1 << TWINT)));
}

// Read one byte from I2C bus
uint8_t TWI_Read(uint8_t ack)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (ack << TWEA);
    while (!(TWCR & (1 << TWINT)));
    return TWDR;
}



// Read analog channel A0 from PCF8591 ADC module
uint8_t PCF8591_ReadA0(void)
{
    uint8_t val;

    // Select channel A0
    TWI_Start();
    TWI_Write(0x48 << 1);   // PCF8591 address + write
    TWI_Write(0x40);        // Control byte: analog input A0
    TWI_Stop();

    _delay_ms(10);

    // Dummy read (first read invalid)
    TWI_Start();
    TWI_Write((0x48 << 1) | 1);
    val = TWI_Read(0);
    TWI_Stop();

    // Actual ADC read
    TWI_Start();
    TWI_Write((0x48 << 1) | 1);
    val = TWI_Read(0);
    TWI_Stop();

    return val;
}



// Initialize MCU as SPI Master
void SPI_MasterInit(void)
{
    // MOSI(PB5), SCK(PB7), SS(PB4) as output, MISO(PB6) as input
    DDRB |= (1 << PB4) | (1 << PB5) | (1 << PB7);
    DDRB &= ~(1 << PB6);

    PORTB |= (1 << PB4); // SS high (inactive)

    // Enable SPI, Master mode, slow clock (Fosc/128)
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1) | (1 << SPR0);
}

// Send one byte over SPI
void SPI_MasterTransmit(uint8_t data)
{
    PORTB &= ~(1 << PB4);  // SS low
    SPDR = data;           // Load data
    while (!(SPSR & (1 << SPIF))); // Wait for transfer
    _delay_us(1);
    PORTB |= (1 << PB4);   // SS high
}




// Convert ADC value (0-255) to C assuming LM35 sensor
uint8_t ADC_ToTemperature(uint8_t adc)
{
    float voltage = adc * 5.0 / 255.0; // Convert ADC to voltage
    float temp = voltage * 100.0;      // LM35: 10mV per C → 100 x V
    return (uint8_t)(temp + 0.5);      // Round to nearest integer
}


int main(void)
{
    char buf[50];
    uint8_t adc_val, temp;

    _delay_ms(500);

    UART_Init(9600);
    TWI_Init();
    SPI_MasterInit();

    UART_SendString("Temperature Master Started\r\n");

    while (1)
    {
        // Read ADC value from PCF8591 channel A0
        adc_val = PCF8591_ReadA0();

        // Convert ADC → temperature
        temp = ADC_ToTemperature(adc_val);

        // Send temperature to SPI slave
        SPI_MasterTransmit(temp);

        // Print value on UART
        sprintf(buf, "ADC: %d -> Temp: %d C\r\n", adc_val, temp);
        UART_SendString(buf);

        _delay_ms(1000);
    }
}
