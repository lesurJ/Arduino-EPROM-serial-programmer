import argparse
import os
import time
import difflib

from serialConnector import SerialConnector

def configureParser():
    parser = argparse.ArgumentParser(
        prog="main.py",
        description="This program read and write vintage EPROMs with the help of Arduino.",
    )
    parser.add_argument(
        "-t",
        "--memory_type",
        type=int,
        required=True,
        choices=[16, 32, 64, 128, 256, 512],
        help="the memory type code (e.g 32 for 27x32 memories)",
    )
    parser.add_argument(
        "-f",
        "--filename",
        type=str,
        help="the name of the machine code to flash on EPROM (.txt); all bytes must be on different lines.",
    )
    return parser.parse_args()

def getProgramCode(machine_code_file):
    with open(f"{machine_code_file}", "r") as f:
        lines = f.readlines()
    programCode = [int(item.strip(), 2) for item in lines]
    return (
        len(programCode),
        programCode,
    )

def saveDataToFile(data, filename):
    data = data.split(sep="\r\n")
    with open(filename, "w") as f:
        for d in data:
            if d != "":
                f.write(f"{int(d):08b}\n")

def readMemory(memoryType, filename):
    print("> Reading EPROM:   0%", end="\r")

    memorySize = memoryType * 128 # size in bytes = type * 1024 / 8
    sc.sendCode(f"{memoryType}|R{memorySize}")
    t1 = time.time()

    epromContents = ""
    readIsDone = False
    readBytes = 0
    while not readIsDone:
        data = sc.waitForSerialData()
        if data == "DONE":
            readIsDone = True
            print(f"> Reading EPROM: 100%, elapsed time: {time.time() - t1:6.2f}")
        else:
            epromContents += data
            readBytes += chunkSize
        progress = 100 * readBytes / memorySize
        print(f"> Reading EPROM: {progress:3.0f}%, elapsed time: {time.time() - t1:6.2f}[s].", end="\r")

    saveDataToFile(epromContents, filename)

def writeMemory(memoryType, programSize, programCode):
    print("> Programming EPROM:   0%", end="\r")
    sc.sendCode(f"{memoryType}|W{programSize}")
    t1 = time.time()

    sc.waitForCode("WRITE-READY")
    i = -1
    for i in range(programSize // chunkSize):
        progress = 100 * (i * chunkSize / programSize)
        print(f"> Programming EPROM: {progress:3.0f}%, elapsed time: {time.time() - t1:6.2f}[s].", end="\r")
        sc.sendBytes(bytearray(programCode[chunkSize * i : chunkSize * (i + 1)]))
        sc.waitForCode("CONTINUE")

    sc.sendBytes(bytearray(programCode[chunkSize * (i + 1) :]))
    sc.waitForCode("DONE")
    print(f"> Programming EPROM: 100%, elapsed time: {time.time() - t1:6.2f}[s].")

def verifyProgramming(input_file):
    print("> Verifying program.")
    output_file = "epromContents_after.txt"

    with open(input_file, "r") as f:
        input_lines = f.readlines()

    with open(output_file, "r") as f:
        output_lines = f.readlines()[: len(input_lines)]
        output_lines[-1] = output_lines[-1].replace("\n", "")

    error = False
    for l in difflib.unified_diff(
        input_lines, output_lines, fromfile=input_file, tofile=output_file
    ):
        print(l)
        error = True

    if not error:
        print("> Programming success!")
    else:
        print("> Programming failure!")

if __name__ == "__main__":
    args = configureParser()
    chunkSize = 16 # how many bytes to handle at once (read or write)

    print("=== Arduino EPROM Serial Programmer ===")
    print("Make sure that a single EPROM is connected on the PCB.")
    print("Set the programming voltage (Vpp) as specified in the datasheet.")

    folder = "/dev/"
    serialPort = [
        item for item in os.listdir(folder) if item.startswith("tty.usbserial")
    ]
    if len(serialPort) == 0:
        print("No serial port is a tty.usbserial; exiting...")
        exit()

    if args.filename is not None:
        programSize, programCode = getProgramCode(args.filename)
        print(f"-> located machine code. Program size: {programSize} bytes.")

    sc = SerialConnector(folder + serialPort[0])
    if sc.connect():
        print(f"-> arduino connected on serial port {serialPort[0]}.")
        sc.waitForCode("INIT") # wait for arduino to initialize

        # 1. Read EPROM
        readMemory(args.memory_type, "epromContents_before.txt")

        # 2. Write EPROM
        if args.filename is not None:
            _ = input("> Apply programming voltage (toggle appropriate switch) and press ENTER.")
            writeMemory(args.memory_type, programSize, programCode)

            # 3. Read EPROM
            _ = input("> Remove programming voltage (toggle appropriate switch) and press ENTER.")
            readMemory(args.memory_type, "epromContents_after.txt")

            # 4. Verify programming
            verifyProgramming(args.filename)

    else:
        print("Unable to find/access selected serial port!")
