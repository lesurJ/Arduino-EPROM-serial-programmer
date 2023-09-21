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


def saveMemoryContents(data, filename):
    data = data.split(sep="\r\n")
    with open(filename, "w") as f:
        for d in data:
            if d != "":
                f.write(f"{int(d):08b}\n")


if __name__ == "__main__":
    args = configureParser()

    folder = "/dev/"
    serialPort = [
        item for item in os.listdir(folder) if item.startswith("tty.usbserial")
    ]

    print("=== Arduino EPROM Serial Programmer ===")
    print("Make sure that a single EPROM is connected on the PCB.")
    print("Set the programming voltage (Vpp) as specified in the datasheet.")
    sc = SerialConnector(folder + serialPort[0])
    if sc.connect():
        print(f"-> arduino connected on serial port {serialPort[0]}.")
        # Wait for arduino to initialize
        sc.waitForCode("READY")

        # 1. Read EPROM
        print("> Reading EPROM.")
        sc.sendCode(f"{args.memory_type}|R")
        sc.waitForSerialData()
        saveMemoryContents(
            sc.payload,
            "epromContents_before.txt",
        )

        if args.filename is not None:
            chunkSize = 16
            programSize, programCode = getProgramCode(args.filename)
            print(f"> Located machine code. Program size: {programSize} bytes.")

            # 2. Write EPROM
            _ = input("Apply programming voltage (toggle appropriate switch) and press ENTER.")
            print("> Flashing EPROM,   0%", end="\r")
            t1 = time.time()
            sc.sendCode(f"{args.memory_type}|W")
            sc.waitForCode("WRITE-READY")

            chunkNumber = 0
            i = -1
            for i in range(programSize // chunkSize):
                print(
                    f"> Flashing EPROM, {100 * (i * chunkSize / programSize):3.0f}%, elapsed time: {time.time() - t1:6.2f} [s].",
                    end="\r",
                )
                sc.sendBytes(
                    bytearray(programCode[chunkSize * i : chunkSize * (i + 1)])
                )
                sc.waitForCode("CONTINUE")

            sc.sendBytes(bytearray(programCode[chunkSize * (i + 1) :]))
            sc.waitForCode("DONE")
            print(
                f"> Flashing EPROM, 100%. Total programming time: {time.time() - t1:5.2f} [s]."
            )

            # 3. Read EPROM
            _ = input("Remove programming voltage (toggle appropriate switch) and press ENTER.")
            print("> Reading EPROM.")
            sc.sendCode(f"{args.memory_type}|R")
            sc.waitForSerialData()
            saveMemoryContents(sc.payload, "epromContents_after.txt")

            # 4. Verify programming
            print("> Verifying program.")
            input_file = args.filename
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

    else:
        print("Unable to find/access selected serial port!")
