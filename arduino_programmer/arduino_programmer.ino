const int nSRCLR_pin = 2;
const int SRCLK_pin = 3;
const int SERIAL_pin = 4;
const int RCLK_pin = 5;
const int msb_pin = 6;
const int lsb_pin = 13;

const int nCE_PIN = A0;
const int nPGM_PIN = A1;
const int nOE_PIN = A2;

const unsigned long baudrate = 115200;
const char start_of_message = '<';
const char end_of_message = '>';
const int read_mode = 0;
const int write_mode = 1;

const int chunk_size = 16;
byte chunk_buffer[chunk_size];

// === FUNCTIONS ===

void setAddress(long address)
{
    digitalWrite(RCLK_pin, LOW);
    shiftOut(SERIAL_pin, SRCLK_pin, LSBFIRST, (byte)address);
    shiftOut(SERIAL_pin, SRCLK_pin, LSBFIRST, address >> 8);
    digitalWrite(RCLK_pin, HIGH);
}

void changeAccessMode(int mode, int eprom_type)
{
    if (mode == read_mode)
    {
        digitalWrite(nCE_PIN, LOW);
        digitalWrite(nOE_PIN, LOW);
        digitalWrite(nPGM_PIN, HIGH);
        for (int pin = msb_pin; pin <= lsb_pin; pin++)
        {
            pinMode(pin, INPUT);
        }
    }
    else
    {
        switch (eprom_type)
        {
        case 16:
            digitalWrite(nOE_PIN, HIGH);
            digitalWrite(nCE_PIN, LOW);
            break;

        case 32:
            digitalWrite(nCE_PIN, HIGH);
            break;

        case 64:
            digitalWrite(nCE_PIN, LOW);
            digitalWrite(nOE_PIN, HIGH);
            digitalWrite(nPGM_PIN, HIGH);
            break;

        case 128:
            digitalWrite(nCE_PIN, LOW);
            digitalWrite(nOE_PIN, HIGH);
            digitalWrite(nPGM_PIN, HIGH);
            break;

        case 256:
            digitalWrite(nCE_PIN, HIGH);
            digitalWrite(nOE_PIN, HIGH);
            break;

        case 512:
            digitalWrite(nCE_PIN, HIGH);
            break;
        }
        for (int pin = msb_pin; pin <= lsb_pin; pin++)
        {
            pinMode(pin, OUTPUT);
        }
    }
}

void generateProgramPulse(int eprom_type)
{
    switch (eprom_type)
    {
    case 16:
        digitalWrite(nCE_PIN, HIGH);
        delay(50);
        digitalWrite(nCE_PIN, LOW);
        break;

    case 32:
        // digitalWrite(nCE_PIN, LOW);
        delay(50);
        // digitalWrite(nCE_PIN, HIGH);
        break;

    case 64:
        // implement custom algo
        // digitalWrite(nPGM_PIN, LOW);
        delay(50);
        // digitalWrite(nPGM_PIN, HIGH);
        break;

    case 128:
        // implement custom algo
        // digitalWrite(nPGM_PIN, LOW);
        delay(50);
        // digitalWrite(nPGM_PIN, HIGH);
        break;

    case 256:
        // implement custom algo
        // digitalWrite(nCE_PIN, LOW);
        delay(50);
        // digitalWrite(nCE_PIN, HIGH);
        break;

    case 512:
        // implement custom algo
        // digitalWrite(nCE_PIN, LOW);
        delay(50);
        // digitalWrite(nCE_PIN, HIGH);
        break;
    }
}

String waitForSerialMessage(void)
{
    String message = "";
    char current_char = ' ';
    bool received_message = false;

    while (!received_message)
    {
        while (Serial.available() > 0)
        {
            current_char = Serial.read();
            if (current_char == start_of_message)
            {
                message = "";
            }
            else if (current_char == end_of_message)
            {
                received_message = true;
                break;
            }
            else
            {
                message += String(current_char);
            }
        }
    }
    return message;
}

void readMemory(int eprom_type, long memory_size)
{
    changeAccessMode(read_mode, eprom_type);

    long bytes_read = 0;
    bool read_is_done = false;
    while (!read_is_done)
    {
        Serial.print(start_of_message);
        for (int addr = 0; addr < chunk_size; addr++)
        {
            setAddress(bytes_read + addr);
            delayMicroseconds(1); // typical access time is 200ns for EPROM
            byte data = 0;
            for (int pin = msb_pin; pin <= lsb_pin; pin++)
            {
                data = (data << 1) + digitalRead(pin);
            }
            Serial.println(data);
        }
        Serial.println(end_of_message);

        bytes_read += chunk_size;
        read_is_done = (bytes_read == memory_size);
    }
    Serial.println("<DONE>");
}

void writeMemory(int eprom_type, long program_size)
{
    changeAccessMode(write_mode, eprom_type);
    Serial.println("<WRITE-READY>");

    long data_index = 0;
    bool transfer_is_done = false;
    while (!transfer_is_done)
    {
        int rlen = Serial.readBytes(chunk_buffer, chunk_size);
        for (int data_addr = 0; data_addr < rlen; data_addr++)
        {
            setAddress(data_index + data_addr);
            delayMicroseconds(2); // typical adress and data setup time
            for (int pin = lsb_pin; pin >= msb_pin; pin--)
            {
                digitalWrite(pin, chunk_buffer[data_addr] & 1);
                chunk_buffer[data_addr] >>= 1;
            }
            generateProgramPulse(eprom_type);
        }

        data_index += rlen;
        transfer_is_done = (data_index == program_size);
        Serial.println("<CONTINUE>");
    }
    Serial.println("<DONE>");
}

// === END OF FUNCTIONS ===

void setup()
{
    Serial.begin(baudrate);

    pinMode(nSRCLR_pin, OUTPUT);
    pinMode(SRCLK_pin, OUTPUT);
    pinMode(SERIAL_pin, OUTPUT);
    pinMode(RCLK_pin, OUTPUT);
    pinMode(nCE_PIN, OUTPUT);
    pinMode(nPGM_PIN, OUTPUT);
    pinMode(nOE_PIN, OUTPUT);

    digitalWrite(nSRCLR_pin, HIGH);
    digitalWrite(nCE_PIN, LOW);
    digitalWrite(nOE_PIN, LOW);
    digitalWrite(nPGM_PIN, HIGH);

    while (!Serial)
    {
        delay(10);
    }

    Serial.println("<INIT>");
}

void loop()
{
    String message = waitForSerialMessage();
    int idx = message.indexOf("|");
    if (idx != -1)
    {
        int eprom_type = message.substring(0, idx).toInt();
        String command = message.substring(idx + 1);

        if (command.startsWith("R"))
        {
            long memory_size = command.substring(1).toInt();
            readMemory(eprom_type, memory_size);
        }
        else if (command.startsWith("W"))
        {
            long program_size = command.substring(1).toInt();
            writeMemory(eprom_type, program_size);
        }
    }
}