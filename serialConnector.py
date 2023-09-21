import serial


class SerialConnector(object):
    def __init__(self, serialPortName) -> None:
        self.ser = None

        self.serialPortNane = serialPortName
        self.timeout = None
        self.baudrate = 115200

        self.startOfMessage = "<"
        self.endOfMessage = ">"
        self.payload = ""

    def __del__(self):
        if self.ser:
            self.ser.close()

    def connect(self):
        connected = True
        try:
            self.ser = serial.Serial(
                self.serialPortNane, self.baudrate, timeout=self.timeout
            )
        except:
            connected = False
        return connected

    def waitForSerialData(self):
        receivedMessage = False
        currentChar = ""

        while not receivedMessage:
            currentChar = self.ser.read(1).decode("UTF-8")
            if currentChar == self.startOfMessage:
                self.payload = ""
            elif currentChar == self.endOfMessage:
                receivedMessage = True
            else:
                self.payload += currentChar

    def waitForCode(self, code):
        ready = False
        while not ready:
            self.waitForSerialData()
            ready = self.payload == code

    def sendCode(self, code):
        message = self.startOfMessage + code + self.endOfMessage
        self.ser.write(message.encode())

    def sendBytes(self, bb):
        self.ser.write(bb)
