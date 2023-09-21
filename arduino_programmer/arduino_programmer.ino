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

void setAddress(int address)
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
        for (int pin = msb_pin; pin <= lsb_pin; pin++)
        {
            pinMode(pin, INPUT);
        }
        digitalWrite(nCE_PIN, LOW);
        digitalWrite(nOE_PIN, LOW);
    }
    else
    {
        for (int pin = msb_pin; pin <= lsb_pin; pin++)
        {
            pinMode(pin, OUTPUT);
        }

        switch (eprom_type)
        {
        case 16:
            digitalWrite(nOE_PIN, HIGH);
            break;

        case 32:
            digitalWrite(nCE_PIN, HIGH);
            break;

        case 64:
            break;

        case 128:
            break;

        case 256:
            break;

        case 512:
            break;
        }
    }
    delay(10); // wait for relay to firmly close
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
        digitalWrite(nCE_PIN, LOW);
        delay(50);
        digitalWrite(nCE_PIN, HIGH);
        break;

    case 64:
        break;

    case 128:
        break;

    case 256:
        break;

    case 512:
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

void readMemory(int eprom_type)
{
    changeAccessMode(read_mode, eprom_type);

    int memory_size = eprom_type * 128; // size in bytes = type * 1024 / 8

    Serial.print(start_of_message);
    for (int addr = 0; addr < memory_size; addr++)
    {
        setAddress(addr);
        byte data = 0;
        for (int pin = msb_pin; pin <= lsb_pin; pin++)
        {
            data = (data << 1) + digitalRead(pin);
        }
        Serial.println(data);
    }
    Serial.println(end_of_message);
}

void writeMemory(int eprom_type)
{
    changeAccessMode(write_mode, eprom_type);

    int chunk_index = 0;
    bool transfer_is_done = false;
    Serial.println("<WRITE-READY>");
    while (!transfer_is_done)
    {
        int rlen = Serial.readBytes(chunk_buffer, chunk_size);
        for (int chunk_addr = 0; chunk_addr < rlen; chunk_addr++)
        {
            setAddress(chunk_index + chunk_addr);
            for (int pin = lsb_pin; pin >= msb_pin; pin--)
            {
                digitalWrite(pin, chunk_buffer[chunk_addr] & 1);
                chunk_buffer[chunk_addr] >>= 1;
            }
            generateProgramPulse(eprom_type);
        }

        chunk_index += rlen;
        if (rlen < chunk_size)
        {
            transfer_is_done = true;
            Serial.println("<DONE>");
        }
        else
        {
            Serial.println("<CONTINUE>");
        }
    }
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
    digitalWrite(nPGM_PIN, LOW);

    while (!Serial)
    {
        delay(10);
    }

    Serial.println("<READY>");
}

void loop()
{
    String message = waitForSerialMessage();
    int idx = message.indexOf("|");
    if (idx != -1)
    {
        int eprom_type = message.substring(0, idx).toInt();
        String command = message.substring(idx + 1);

        if (command.equals("R"))
        {
            readMemory(eprom_type);
        }
        else if (command.equals("W"))
        {
            writeMemory(eprom_type);
        }
    }
}