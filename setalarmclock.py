import serial
import sys

if __name__ == "__main__":
    ser = serial.Serial(sys.argv[1], 9600)
    ser.write('ALARM 3 2149\n')
