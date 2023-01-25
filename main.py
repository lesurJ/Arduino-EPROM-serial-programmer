from serialConnector import SerialConnector


def getProgramCode():
    with open("machineCode.txt", "r") as f:
        lines = f.readlines()
    programCode = [int(item.strip(), 2) for item in lines]
    return len(programCode), programCode


def saveMemoryContents(data, filename):
    data = data.split(sep="\r\n")
    with open(filename, "w") as f:
        for d in data:
            if d != "":
                f.write(f"{int(d):08b}\n")


if __name__ == "__main__":
    chunkSize = 8
    programSize, programCode = getProgramCode()
    print(f"Program is {programSize} bytes")
    print(f"Program code is : {programCode}")

    sc = SerialConnector("/dev/tty.usbserial-1420")
    sc.waitForCode("READY")

    # 1. Read EPROM
    print("> Reading EPROM")
    sc.sendCode("rm")
    sc.waitForSerialData()
    saveMemoryContents(sc.payload, "epromContents_before.txt")

    # 2. Write EPROM
    _ = input("> Apply VPP and press Enter.")
    print("> Flashing EPROM,   0%", end="\r")
    sc.sendCode("wm")
    sc.waitForCode("WRITE-READY")

    chunkNumber = 0
    for i in range(programSize // chunkSize):
        print(
            f"> Flashing EPROM, {100 * (i * chunkSize / programSize):3.0f}%", end="\r"
        )
        sc.sendBytes(bytearray(programCode[chunkSize * i : chunkSize * (i + 1)]))
        sc.waitForCode("CONTINUE")

    sc.sendBytes(bytearray(programCode[chunkSize * (i + 1) :]))
    sc.waitForCode("DONE")
    print("> Flashing EPROM, 100%")

    # 3. Read EPROM
    _ = input("> Remove VPP and press Enter.")
    print("> Reading EPROM")
    sc.sendCode("rm")
    sc.waitForSerialData()
    saveMemoryContents(sc.payload, "epromContents_after.txt")
