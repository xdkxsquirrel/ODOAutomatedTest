/********************************************************************************
  * @file    ODOAutomatedTest.h
  * @author  Donovan Bidlack
  * @brief   header file that contains all of the API's needed for doing odo
           automated comparison test. All odo will be self contained in this 
           header file. 
********************************************************************************/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ODOAUTOMATEDTEST_H__
#define __ODOAUTOMATEDTEST_H__

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "tim.h"
#include "can.h"
#include "gpio.h"
#include "stm32l4xx_hal.h"

/* ODO DEFINES*/
#define NUMBER_OF_TEMPERATURE_TEST_CYCLES                       11
#define NUMBER_OF_SPEED_TESTS                                   3
#define NUMBER_OF_MOTOR_ROTATIONS                               10

#define FIFTEENFPS_SPEED                                        2762
#define FIFTEENFPS_ROTATIONS                                    216
#define TENFPS_SPEED                                            1847
#define TENFPS_ROTATIONS                                        324
#define FIVEFPS_SPEED                                           915
#define FIVEFPS_ROTATIONS                                       692

#define WAIT_TIME 10000
#define ODO_MOTOR_ENA                                           13
#define ODO_MOTOR_ENB                                           14
#define ODO_OLD                                                 15 
#define ODO_WAIT_TIME                                           2000

#define ODO_FLASH_WRITE_ADDRESS                                 0x08008000
#define FLASH_START_ADDRESS                                     0x08000000
#define FLASH_PAGE_NBPERBANK                                    256
#define FLASH_BANK_NUMBER                                       2
#define PAGE_ERASE_SUCCESS                                      0xFFFFFFFF                     

/* ODO Types -----------------------------------------------------------------*/


/* Function Prototypes  ------------------------------------------------------*/

/**********************************************
  Name: ODO_Init
  Description: initializes the devices needed for test.
**********************************************/
void ODO_Init( void );

/**********************************************
  Name: ODO_Erase_Flash_Memory
  Description: Erases Flash Memory from user write address.
**********************************************/
void ODO_Erase_Flash_Memory( void );

/**********************************************
  Name: ODO_Test_Process
  Description: runs the ODO Test Process for 
        collecting data from the two ODOs and
        the motor encoder data. Then send it to 
        the CPU for process in Python.  
**********************************************/
void ODO_Test_Process( void );

/**********************************************
  Name: Calc_Novo_ADC
  Description: reads the ADC value and averages 8 of them.  
**********************************************/
void Calc_Novo_ADC( uint16_t *Filtered_Novo_Value );

/**********************************************
  Name: ODO_Write_Data_To_Flash
  Description: reads the ADC value and returns. 
**********************************************/
void ODO_Write_Data_To_Flash( uint64_t *Pointer );

/**********************************************
  Name: Send_Data_To_CPU
  Description: reads the ADC value and returns.  
**********************************************/
void Send_Data_To_CPU( void );

/**********************************************
  Name: ODO_Set_PWM
  Description: Changes the speed value of the 
      PWM for motor control from 0 to 4095.
**********************************************/
void ODO_Set_PWM(uint16_t value);

/**********************************************
  Name: Read_Encoders
  Description: Returns the current value for 
      the motor encoders.
**********************************************/
uint16_t Read_Encoders( void );

/**********************************************
  Name: Read_Old_ODO
  Description: Returns the current value for 
      the Old ODO.
**********************************************/
uint16_t Read_Old_ODO( void );

/**********************************************
  Name: IAP_CAN_Send
  Description: Sends one CAN frame.
**********************************************/
void IAP_CAN_Send( uint16_t standardID, uint8_t ide, uint8_t *payload, uint8_t dlc );


#endif /* __ODOAUTOMATEDTEST_H__ */
