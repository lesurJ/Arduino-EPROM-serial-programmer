import argparse
from serialConnector import SerialConnector


def getProgramCode(machineCode):
    with open(f"programs/{machineCode}", "r") as f:
        lines = f.readlines()
    programCode = [int(item.strip(), 2) for item in lines]
    return (
        len(programCode),
        programCode,
    )


def saveMemoryContents(data, filename):
    data = data.split(sep="\r\n")
    with open(filename, "w") as f:
        for d in data:
            if d != "":
                f.write(f"{int(d):08b}\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog="main.py",
        description="This program read and write vintage EPROMs with the help of Arduino.",
    )
    parser.add_argument(
        "-f",
        "--filename",
        type=str,
        help="the name of the machine code to flash on EPROM (.txt); all bytes must be on different lines.",
    )
    args = parser.parse_args()

    sc = SerialConnector("/dev/tty.usbserial-1420")
    if sc.connect():
        # Wait for arduino to initialize
        sc.waitForCode("READY")

        # 1. Read EPROM
        print("> Reading EPROM")
        sc.sendCode("rm")
        sc.waitForSerialData()
        saveMemoryContents(
            sc.payload,
            "epromContents_before.txt",
        )

        if args.filename is not None:
            print("> Preparation for EPROM flashing")
            chunkSize = 8
            programSize, programCode = getProgramCode(args.filename)
            print(f"Program is {programSize} bytes")
            print(f"Program code is : {programCode}")

            # 2. Write EPROM
            _ = input("> Apply VPP and press Enter.")
            print(
                "> Flashing EPROM,   0%",
                end="\r",
            )
            sc.sendCode("wm")
            sc.waitForCode("WRITE-READY")

            chunkNumber = 0
            i = -1
            for i in range(programSize // chunkSize):
                print(
                    f"> Flashing EPROM, {100 * (i * chunkSize / programSize):3.0f}%",
                    end="\r",
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
    else:
        print("Unable to find/access selected serial port!")
