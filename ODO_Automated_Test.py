#
## ODO_Automated_Test.py
 # Author: Donovan Bidlack
 # Origin Date: 07/02/2019
 #
 # This program if for test automation of the Novoteknic ODO vs the Old ODO.
 #

import datetime as dt
import time
import pandas as pd
from array import array
import Komodo
from komodo_py import *

NUMBER_OF_TEMPERATURE_TEST_CYCLES               = 11
NUMBER_OF_SPEED_TESTS                           = 3
NUMBER_OF_MOTOR_ROTATIONS                       = 10

FIFTEENFPS_ROTATIONS                            = 216
TENFPS_ROTATIONS                                = 324
FIVEFPS_ROTATIONS                               = 692

col = ['Time (s:ms)', 'ENA', 'ENB', 'Old ODO', 'Novo ODO']
number_of_samples_per_rotation = [FIVEFPS_ROTATIONS/4, TENFPS_ROTATIONS/4, FIFTEENFPS_ROTATIONS/4]
sample_number = 0   

def RcvDataFromCAN(km, response_frame_number):
    counter = 0
    failed = False
    data = list()
    ret = km_disable(km)
    info      = km_can_info_t()
    data_in   = array('B', [0, 0, 0, 0, 0, 0, 0, 0])
    
    pkt       = km_can_packet_t()
    pkt.dlc   = 8
    pkt.id    = 0x600
    
    ret = km_enable(km)
    if ret != KM_OK:
        print('Unable to enable Komodo')
        data.insert(0, "NONERCVD")
        return data[0]
    
    for _ in range(10):
        for i in range(response_frame_number):
            ret = 0
            data_in   = array('B', [0]*8)
            while(ret == 0):
                (ret, info, pkt, data_in) = km_can_read(km, data_in)

                counter += 1
                if counter > 50:
                    data.insert(0, "NONERCVD")
                    failed = True
                    break
                current_message = ''
        
            if not failed :
                for j in range(8):
                    current_message = current_message + format(data_in[j], '02X')
                    data.insert(i, current_message)
        if (pkt.id != 0x600):
            data[0] = "NONERCVD"
        else:
            break
    return data[0]

def Get_Start_Time(km):
    rcvd = "NONERCVD"
    while rcvd == "NONERCVD":
        rcvd = RcvDataFromCAN(km, 1)
    return dt.datetime.now()

def Collect_Test_Data(km, speed, rotation, df, start_time): 
    for sample in range(number_of_samples_per_rotation[speed]):
        global sample_number
        rcvd = "NONERCVD"
        while rcvd == "NONERCVD":
            rcvd = RcvDataFromCAN(km, 1)
            
        for i in range(4):
            try:
                binary   = format(int((rcvd[(i*4)+2]+rcvd[(i*4)+3]+rcvd[i*4]+rcvd[(i*4)+1]), 16), '016b')
                encoderA = binary[2]
                encoderB = binary[1]
                Old_ODO  = binary[0]
                Novotek  = int(binary[3:16], 2)
                data     = [[(start_time + dt.timedelta(microseconds=(sample_number*400))).strftime('%S.%f'), encoderA, encoderB, Old_ODO, Novotek]]
            except TypeError:
                encoderA = "Python"
                encoderB = "Type"
                Old_ODO  = "Error"
                Novotek  = "Failure"
                data     = [[(start_time + dt.timedelta(microseconds=(sample_number*400))).strftime('%S.%f'), encoderA, encoderB, Old_ODO, Novotek]]
                print("Python Type Error: {}".format(rcvd))
            except ValueError:
                encoderA = "Python"
                encoderB = "Value"
                Old_ODO  = "Error"
                Novotek  = "Failure"
                data     = [[(start_time + dt.timedelta(microseconds=(sample_number*400))).strftime('%S.%f'), encoderA, encoderB, Old_ODO, Novotek]]
                print("Python Value Error: {}".format(rcvd))
                
            df       = df.append(pd.DataFrame(data, index = [sample_number], columns = col))
            sample_number += 1
                
    return df

     
def main(km):
    global sample_number
    print("Starting ODO Automated Test")
    
    for temperature_cycle in range(NUMBER_OF_TEMPERATURE_TEST_CYCLES):
        print(" ")
        print("{} degree C Temperature Test".format(temperature_cycle*10 -10))
        
        for speed in range(NUMBER_OF_SPEED_TESTS):
            df = pd.DataFrame(columns = col)
            sample_number = 0
            print(" ")
            print("{} Feet/Second Test".format((speed*5)+5))
            print("Waiting for Data from MCU")
            
            for rotation in range(NUMBER_OF_MOTOR_ROTATIONS):
                start_time = Get_Start_Time(km)
                df = Collect_Test_Data(km, speed, rotation, df, start_time)
                print("Completed Collecting rotation {}".format(rotation+1))
    
            try:
                df.to_csv("Data for " + str(temperature_cycle*10-10) + " Degrees C " + str((speed*5)+5) + " FPS.csv")
            except:
                    print("Close the csv file")

komodoReset = 0
km = Komodo.connect()
main(km)
Komodo.close(km)