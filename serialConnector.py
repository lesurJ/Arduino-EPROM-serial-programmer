import serial


class SerialConnector(object):
    def __init__(self, serialPortName) -> None:
        self.ser = None

        self.serialPortNane = serialPortName
        self.timeout = None
        self.baudrate = 115200

        self.startOfMessage = "<"
        self.endOfMessage = ">"

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
        data = ""

        while not receivedMessage:
            currentChar = self.ser.read(1).decode("UTF-8")
            if currentChar == self.startOfMessage:
                data = ""
            elif currentChar == self.endOfMessage:
                receivedMessage = True
            else:
                data += currentChar
        return data

    def waitForCode(self, code):
        ready = False
        while not ready:
            data = self.waitForSerialData()
            ready = data == code

    def sendCode(self, code):
        message = self.startOfMessage + code + self.endOfMessage
        self.ser.write(message.encode())

    def sendBytes(self, bb):
        self.ser.write(bb)
