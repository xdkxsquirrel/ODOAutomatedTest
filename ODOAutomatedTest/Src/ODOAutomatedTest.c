/********************************************************************************
  * @file    ODOAutomatedTest.c
  * @author  Donovan Bidlack
  * @brief   c file that contains all of the API's needed for doing odo
           automated comparison test. All odo will be self contained in this 
           header file. 
********************************************************************************/

#include "ODOAutomatedTest.h"

// Global Variables
static uint16_t Novo_Value[8];
static uint8_t Timer_Fired;
static uint8_t ADC_Index;
static uint16_t Flash_Memory_Counter;

/**********************************************
  Name: ODO_Init
  Description: initializes the devices needed for test.
**********************************************/
void ODO_Init( void )
{
      ADC_Index = 0;
      Flash_Memory_Counter = 0;
      HAL_TIM_Base_Start_IT(&htim7);
      HAL_TIM_Base_Start_IT(&htim2);
      HAL_TIM_Base_Start_IT(&htim1);
      ODO_Erase_Flash_Memory( );

      HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);


      if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK)
      {
            Error_Handler();
      }

      if (HAL_ADC_Start_IT(&hadc1) != HAL_OK)
      {
            Error_Handler();
      }

      
      CAN_FilterTypeDef FilterConfig;
      FilterConfig.FilterIdHigh = 0xFFFF;
      FilterConfig.FilterIdLow = 0xFFFF;
      FilterConfig.FilterMaskIdHigh = 0x0000;
      FilterConfig.FilterMaskIdLow = 0x0000;
      FilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
      FilterConfig.FilterBank = 0;
      FilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
      FilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
      FilterConfig.FilterActivation = CAN_FILTER_ENABLE;
      if( HAL_CAN_ConfigFilter(&hcan1, &FilterConfig) != HAL_OK )
      {
        Error_Handler();
      }
      HAL_CAN_ActivateNotification( &hcan1, CAN_IT_RX_FIFO0_MSG_PENDING );
      HAL_CAN_Start( &hcan1 );
      
}

/**********************************************
  Name: ODO_Erase_Flash_Memory
  Description: Erases Flash Memory from user write address.
**********************************************/
void ODO_Erase_Flash_Memory( void )
{
  uint32_t PageEraseStatus = 0;
  uint32_t start = ODO_FLASH_WRITE_ADDRESS;
  uint8_t NbrOfPages = ((FLASH_START_ADDRESS + FLASH_SIZE) - start) / FLASH_PAGE_SIZE;
  uint8_t flashEraseLoopCounter = 0;
  FLASH_EraseInitTypeDef pEraseInit;
  pEraseInit.Banks = FLASH_BANK_1;
  pEraseInit.NbPages = NbrOfPages % FLASH_PAGE_NBPERBANK;
  pEraseInit.Page = FLASH_PAGE_NBPERBANK - pEraseInit.NbPages;
  pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
  HAL_FLASH_Unlock();    
  __disable_irq();
  while (PageEraseStatus != PAGE_ERASE_SUCCESS)
  {
    if( flashEraseLoopCounter > 10 )
    {
      Error_Handler();
    }
    HAL_FLASHEx_Erase( &pEraseInit, &PageEraseStatus );
    flashEraseLoopCounter ++;
  }
  __enable_irq();
  HAL_FLASH_Lock();
}

/**********************************************
  Name: ODO_Test_Process
  Description: runs the ODO Test Process for 
        collecting data from the two ODOs and
        the motor encoder data. Then send it to 
        the CPU for process in Python.  
**********************************************/
void ODO_Test_Process( void )
{
      uint16_t i;
      uint16_t j;
      uint16_t k;
      uint16_t l;
      uint8_t Every_4th_Counter = 0;
      uint16_t Filtered_Novo_Value;
      uint16_t Flash_Buffer[4];
      uint16_t number_of_samples_per_rotation;
      uint8_t *aTxBuffer;      
      
      // Number of Different Temperatures
      for(i = 0; i < NUMBER_OF_TEMPERATURE_TEST_CYCLES; i++)
      {
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
        // Wait for HASS Input
        while(HAL_GPIO_ReadPin(Temp_Set_GPIO_Port, Temp_Set_Pin) == GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
        
        // Number of Different Speeds
        for(j = 0; j < NUMBER_OF_SPEED_TESTS; j++)
        {
            switch(j)
            {
                  case(0):
                        ODO_Set_PWM(FIVEFPS_SPEED);
                        number_of_samples_per_rotation = FIVEFPS_ROTATIONS;
                        break;

                  case(1):
                        ODO_Set_PWM(TENFPS_SPEED);
                        number_of_samples_per_rotation = TENFPS_ROTATIONS;
                        break;

                  case(2):
                        ODO_Set_PWM(FIFTEENFPS_SPEED);
                        number_of_samples_per_rotation = FIFTEENFPS_ROTATIONS;
                        break;

                  default:
                        ODO_Set_PWM(0);
                        number_of_samples_per_rotation = 0;
                        break;
            }
            
            HAL_Delay(ODO_WAIT_TIME);
            
            // Send CAN message to sync start time
            aTxBuffer[0] = 1;
            IAP_CAN_Send( 0x600, CAN_ID_STD, aTxBuffer, 0 );

            // Number of Motor Rotations
            for(k = 0; k < NUMBER_OF_MOTOR_ROTATIONS; k++)
            {
                  // Number of samples per motor rotation (depends on speed of motor)
                  for(l = 0; l < number_of_samples_per_rotation; l++)
                  {
                        Timer_Fired = 0;
                        while(Timer_Fired == 0);
                        Calc_Novo_ADC( &Filtered_Novo_Value );
                        Flash_Buffer[Every_4th_Counter] = Read_Encoders( ) | Filtered_Novo_Value;
                        Flash_Buffer[Every_4th_Counter] = Read_Old_ODO( ) | Flash_Buffer[Every_4th_Counter];
                        Every_4th_Counter ++;
                        if(Every_4th_Counter > 3)
                        {
                              ODO_Write_Data_To_Flash( (uint64_t*) &Flash_Buffer );
                              Every_4th_Counter = 0;
                        }                       
                  }               
            }
            ODO_Set_PWM(0);
            Send_Data_To_CPU();
            Flash_Memory_Counter = 0;
            ODO_Erase_Flash_Memory( );
            HAL_Delay(ODO_WAIT_TIME);
      }
      
      ODO_Set_PWM(0);
   }
}

/**********************************************
  Name: Read_Novo_ADC
  Description: reads the ADC value and returns. 
**********************************************/
void Calc_Novo_ADC( uint16_t *Filtered_Novo_Value )
{
      *Filtered_Novo_Value = (Novo_Value[0]+Novo_Value[1]+Novo_Value[2]+Novo_Value[3]+Novo_Value[4]+Novo_Value[5]+Novo_Value[6]+Novo_Value[7] ) >> 3;
}

/**********************************************
  Name: HAL_ADC_ConvCpltCallback
  Description: updates the ADC when conversion is ready
**********************************************/
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
  
      //HAL_GPIO_TogglePin(Sample_Freq_GPIO_Port, Sample_Freq_Pin);
      Novo_Value[ ADC_Index ] = HAL_ADC_GetValue(hadc);
      if(ADC_Index > 6)
      {
            ADC_Index = 0;
      }
      else
      {
            ADC_Index++;
      }
}

/**********************************************
  Name: HAL_TIM_PeriodElapsedCallback
  Description: updates the ADC when conversion is ready.
**********************************************/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
      if(htim == &htim7)
      {
            //HAL_GPIO_TogglePin(Sample_Freq_GPIO_Port, Sample_Freq_Pin);
            Timer_Fired = 1;
      }
}

/**********************************************
  Name: ODO_Write_Data_To_Flash
  Description: reads the ADC value and returns. 
**********************************************/
void ODO_Write_Data_To_Flash( uint64_t *Pointer )
{
      HAL_StatusTypeDef status = HAL_ERROR; 
      uint32_t destination = ODO_FLASH_WRITE_ADDRESS + ((Flash_Memory_Counter) << 3);
      uint64_t Data = *Pointer;
      uint8_t flashWriteLoopCounter = 0;
      __disable_irq();
      while ( status != HAL_OK )
      {
            if( flashWriteLoopCounter > 10 )
            {
                  Error_Handler();
            }
            HAL_FLASH_Unlock();    
            status = HAL_FLASH_Program( FLASH_TYPEPROGRAM_DOUBLEWORD, destination, Data ); 
            HAL_FLASH_Lock();
            flashWriteLoopCounter ++;
      }
      __enable_irq();
      Flash_Memory_Counter ++;
}

/**********************************************
  Name: Send_Data_To_CPU
  Description: reads the ADC value and returns.  
**********************************************/
void Send_Data_To_CPU( void )
{
      uint8_t aTxBuffer[8];
      uint16_t i;
      uint8_t *pointer;

      for(i = 0; i < Flash_Memory_Counter; i++)
      {
            pointer = (uint8_t*) (ODO_FLASH_WRITE_ADDRESS + (i << 3));
            aTxBuffer[0] = *pointer;
            aTxBuffer[1] = *(pointer + 1);
            aTxBuffer[2] = *(pointer + 2);
            aTxBuffer[3] = *(pointer + 3);
            aTxBuffer[4] = *(pointer + 4);
            aTxBuffer[5] = *(pointer + 5);
            aTxBuffer[6] = *(pointer + 6);
            aTxBuffer[7] = *(pointer + 7);            
      
            IAP_CAN_Send( 0x600, CAN_ID_STD, aTxBuffer, 8 );
      }
}

/**********************************************
  Name: ODO_Set_PWM
  Description: Changes the speed value of the 
      PWM for motor control from 0 to 4095.
**********************************************/
void ODO_Set_PWM(uint16_t value)
{
      TIM_OC_InitTypeDef sConfigOC;
  
      sConfigOC.OCMode = TIM_OCMODE_PWM1;
      sConfigOC.Pulse = value;
      sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
      sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
      sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
      sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
      sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
      if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
      {
            Error_Handler();
      }
      HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
}

/**********************************************
  Name: Read_Encoders
  Description: Returns the current value for 
      the motor encoders.
**********************************************/
uint16_t Read_Encoders( void )
{
   return (uint16_t) ((HAL_GPIO_ReadPin(EnA_In_GPIO_Port, EnA_In_Pin) << ODO_MOTOR_ENA) | (HAL_GPIO_ReadPin(EnB_In_GPIO_Port, EnB_In_Pin) << ODO_MOTOR_ENB));
}

/**********************************************
  Name: Read_Old_ODO
  Description: Returns the current value for 
      the Old ODO.
**********************************************/
uint16_t Read_Old_ODO( void )
{
   return (uint16_t) (HAL_GPIO_ReadPin(Old_ODO_In_GPIO_Port, Old_ODO_In_Pin) << ODO_OLD);
}

/**********************************************
  Name: IAP_CAN_Send
  Description: Sends one CAN frame.
**********************************************/
void IAP_CAN_Send( uint16_t standardID, uint8_t ide, uint8_t *payload, uint8_t dlc )
{
  __disable_irq();
  uint32_t TxMailbox;
  CAN_TxHeaderTypeDef   TxHeader;
  TxHeader.StdId = standardID;
  TxHeader.ExtId = 0x01;
  TxHeader.RTR = CAN_RTR_DATA;
  TxHeader.IDE = CAN_ID_STD;
  TxHeader.DLC = dlc;
  TxHeader.TransmitGlobalTime = DISABLE;
  while( HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) == 0 )
  {
    // Waiting for Mailbox to become free.
  }
  if (HAL_CAN_AddTxMessage(&hcan1, &TxHeader, payload, &TxMailbox) != HAL_OK)
  {
    /* Transmission request Error */
    Error_Handler();
  }
  __enable_irq(); 
  HAL_Delay(150); // Komodo can only read frames this quickly.
}