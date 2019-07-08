# OdoAutomatedTest

## Contents
[ODO Test MCU Library](https://github.com/xdkxsquirrel/OdoAutomatedTest/blob/master/ODOAutomatedTest/Src/ODOAutomatedTest.c "C file that contains all required libraries for the MCU to run the ODO Automated Test.")

[ODO Test MCU Header](https://github.com/xdkxsquirrel/OdoAutomatedTest/blob/master/ODOAutomatedTest/Inc/ODOAutomatedTest.h "H file that contains all required header information for the MCU to run the ODO Automated Test.")

[ODO Test Python File](https://github.com/xdkxsquirrel/OdoAutomatedTest/blob/master/ODO_Automated_Test.py "Python script that recods data from the ODO Automated Test sent from the MCU")

## Purpose
This software was used to build a test platform for comparing two odometers during a temperature stress test. The PWM controls a Pololu motor that spins a shaft that simulates a wheel that would turn in a real scenario. These two odometers then measure the distance that the shaft moves to know how far the device has moved.

The MCU starts up, waits for a signal from the temperature chamber control unit, and sends a CAN message when the measurements start. The motor is then spun at three speeds. Five, ten, and fifteen feet per second. The data captured is saved to flash memory so there is no lag time to ensure data capture rates of 2.5 KHz. After 10 rotations of the motor shaft, the data is transfered from flash memory over CAN. This process is the longest because the CAN Komodo Solo is used, which can only read a CAN frame every milisecond. Once all three speeds have been performed, the MCU waits for the next signal from the temperature chamber control unit.
