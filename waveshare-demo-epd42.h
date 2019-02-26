/**
  ******************************************************************************
    @file    epd.h and edp4in2.h
    @author  Waveshare Team
    @version V1.0.0
    @date    23-January-2018
    @brief   This file provides e-Paper driver functions and initialisation of 4.2 and 4.2b e-Papers
              void EPD_SendCommand(byte command);
              void EPD_SendData(byte data);
              void EPD_WaitUntilIdle();
              void EPD_Send_1(byte c, byte v1);
              void EPD_Send_2(byte c, byte v1, byte v2);
              void EPD_Send_3(byte c, byte v1, byte v2, byte v3);
              void EPD_Send_4(byte c, byte v1, byte v2, byte v3, byte v4);
              void EPD_Send_5(byte c, byte v1, byte v2, byte v3, byte v4, byte v5);
              void EPD_Reset();
              void EPD_dispInit();

             varualbes:
              EPD_dispLoad;                - pointer on current loading function
              EPD_dispIndex;               - index of current e-Paper
              EPD_dispInfo EPD_dispMass[]; - array of e-Paper properties

  ******************************************************************************
*/

#include <SPI.h>

extern  ESP8266WebServer server;

/* SPI pin definition --------------------------------------------------------*/
// SPI pin definition
#define CS_PIN           15
#define RST_PIN          5
#define DC_PIN           4
#define BUSY_PIN         16

#define GPIO_PIN_SET   1
#define GPIO_PIN_RESET 0

/* The procedure of sending a byte to e-Paper by SPI -------------------------*/
void EpdSpiTransferCallback(byte data)
{
  digitalWrite(CS_PIN, GPIO_PIN_RESET);
  SPI.transfer(data);
  digitalWrite(CS_PIN, GPIO_PIN_SET);
}

/* Sending a byte as a command -----------------------------------------------*/
void EPD_SendCommand(byte command)
{
  digitalWrite(DC_PIN, LOW);
  EpdSpiTransferCallback(command);
}

/* Sending a byte as a data --------------------------------------------------*/
void EPD_SendData(byte data)
{
  digitalWrite(DC_PIN, HIGH);
  EpdSpiTransferCallback(data);
}

/* Waiting the e-Paper is ready for further instructions ---------------------*/
void EPD_WaitUntilIdle()
{
  //0: busy, 1: idle
  while (digitalRead(BUSY_PIN) == 0) delay(100);
}

/* Send a one-argument command -----------------------------------------------*/
void EPD_Send_1(byte c, byte v1)
{
  EPD_SendCommand(c);
  EPD_SendData(v1);
}

/* Send a two-arguments command ----------------------------------------------*/
void EPD_Send_2(byte c, byte v1, byte v2)
{
  EPD_SendCommand(c);
  EPD_SendData(v1);
  EPD_SendData(v2);
}

/* Send a three-arguments command --------------------------------------------*/
void EPD_Send_3(byte c, byte v1, byte v2, byte v3)
{
  EPD_SendCommand(c);
  EPD_SendData(v1);
  EPD_SendData(v2);
  EPD_SendData(v3);
}

/* Send a four-arguments command ---------------------------------------------*/
void EPD_Send_4(byte c, byte v1, byte v2, byte v3, byte v4)
{
  EPD_SendCommand(c);
  EPD_SendData(v1);
  EPD_SendData(v2);
  EPD_SendData(v3);
  EPD_SendData(v4);
}

/* Send a five-arguments command ---------------------------------------------*/
void EPD_Send_5(byte c, byte v1, byte v2, byte v3, byte v4, byte v5)
{
  EPD_SendCommand(c);
  EPD_SendData(v1);
  EPD_SendData(v2);
  EPD_SendData(v3);
  EPD_SendData(v4);
  EPD_SendData(v5);
}

/* Writting lut-data into the e-Paper ----------------------------------------*/
void EPD_lut(byte c, byte l, byte*p)
{
  // lut-data writting initialization
  EPD_SendCommand(c);

  // lut-data writting doing
  for (int i = 0; i < l; i++, p++) EPD_SendData(*p);
}

/* This function is used to 'wake up" the e-Paper from the deep sleep mode ---*/
void EPD_Reset()
{
  digitalWrite(RST_PIN, LOW);
  delay(200);

  digitalWrite(RST_PIN, HIGH);
  delay(200);
}

int  EPD_dispIndex;        // The index of the e-Paper's type
int  EPD_dispX, EPD_dispY; // Current pixel's coordinates (for 2.13 only)
void(*EPD_dispLoad)();     // Pointer on a image data writting function

/* Image data loading function for a-type e-Paper ----------------------------*/
void EPD_loadA()
{
  int index = 0;
  String p = server.arg(0);

  // Get the length of the image data begin
  int DataLength = p.length() - 8;

  // Enumerate all of image data bytes
  while (index < DataLength)
  {
    // Get current byte
    int value = ((int)p[index] - 'a') + (((int)p[index + 1] - 'a') << 4);

    // Write the byte into e-Paper's memory
    EPD_SendData((byte)value);

    // Increment the current byte index on 2 characters
    index += 2;
  }
}

/* Show image and turn to deep sleep mode (a-type, 4.2 and 2.7 e-Paper) ------*/
void EPD_showA()
{
  // Refresh
  EPD_Send_1(0x22, 0xC4);//DISPLAY_UPDATE_CONTROL_2
  EPD_SendCommand(0x20);//MASTER_ACTIVATION
  EPD_SendCommand(0xFF);//TERMINATE_FRAME_READ_WRITE
  EPD_WaitUntilIdle();

  // Sleep
  EPD_SendCommand(0x10);//DEEP_SLEEP_MODE
  EPD_WaitUntilIdle();
}

/* Show image and turn to deep sleep mode (b-type, e-Paper) ------------------*/
void EPD_showB()
{
  // Refresh
  EPD_SendCommand(0x12);//DISPLAY_REFRESH
  delay(100);
  EPD_WaitUntilIdle();

  // Sleep
  EPD_Send_1(0x50, 0x17);//VCOM_AND_DATA_INTERVAL_SETTING
  EPD_Send_1(0x82, 0x00);//VCM_DC_SETTING_REGISTER, to solve Vcom drop
  EPD_Send_4(0x01, 0x02, 0x00, 0x00, 0x00);//POWER_SETTING
  EPD_WaitUntilIdle();
  EPD_SendCommand(0x02);//POWER_OFF
}

/* The set of pointers on 'init', 'load' and 'show' functions, title and code */
struct EPD_dispInfo
{
  int(*init)(); // Initialization
  void(*chBk)();// Black channel loading
  int next;     // Change channel code
  void(*chRd)();// Red channel loading
  void(*show)();// Show and sleep
  char*title;   // Title of an e-Paper
};

unsigned char lut_dc_4in2[] = 
{
    0x00, 0x17, 0x00, 0x00, 0x00, 0x02, 0x00, 0x17, 0x17, 0x00, 0x00, 
    0x02, 0x00, 0x0A, 0x01, 0x00, 0x00, 0x01, 0x00, 0x0E, 0x0E, 0x00, 
    0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

//R21H
unsigned char lut_ww_4in2[] = 
{
    0x40, 0x17, 0x00, 0x00, 0x00, 0x02, 0x90, 0x17, 0x17, 0x00, 0x00, 0x02, 0x40, 0x0A, 
    0x01, 0x00, 0x00, 0x01, 0xA0, 0x0E, 0x0E, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

//R22H    r
unsigned char lut_bw_4in2[] =
{
    0x40, 0x17, 0x00, 0x00, 0x00, 0x02, 0x90, 0x17, 0x17, 0x00, 0x00, 0x02, 0x40, 0x0A, 
    0x01, 0x00, 0x00, 0x01, 0xA0, 0x0E, 0x0E, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

//R24H    b
unsigned char lut_bb_4in2[] =
{
    0x80, 0x17, 0x00, 0x00, 0x00, 0x02, 0x90, 0x17, 0x17, 0x00, 0x00, 0x02, 0x80, 0x0A, 
    0x01, 0x00, 0x00, 0x01, 0x50, 0x0E, 0x0E, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

//R23H    w
unsigned char lut_wb_4in2[] =
{
    0x80, 0x17, 0x00, 0x00, 0x00, 0x02, 0x90, 0x17, 0x17, 0x00, 0x00, 0x02, 0x80, 0x0A, 
    0x01, 0x00, 0x00, 0x01, 0x50, 0x0E, 0x0E, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};


int EPD_Init_4in2() 
{
    EPD_Reset();
    
    EPD_SendCommand(0x01);//POWER_SETTING
    EPD_SendData(0x03);   // VDS_EN, VDG_EN
    EPD_SendData(0x00);   // VCOM_HV, VGHL_LV[1], VGHL_LV[0]
    EPD_SendData(0x2F);   // VDH
    EPD_SendData(0x2F);   // VDL
    EPD_SendData(0xFF);   // VDHR
    
    EPD_Send_3(0x06, 0x17, 0x17, 0x17);//BOOSTER_SOFT_START
    EPD_SendCommand(0x04);//POWER_ON
    EPD_WaitUntilIdle();
    
    EPD_Send_2(0x00, 0xBF, 0x0B);//PANEL_SETTING: // KW-BF   KWR-AF  BWROTP 0f
    EPD_Send_1(0x30, 0x3C);//PLL_CONTROL: 3A 100HZ, 29 150Hz, 39 200HZ, 31 171HZ

    EPD_Send_4(0x61, 1, 144, 1, 44);// RESOLUTION_SETTING: HI(W), LO(W), HI(H), LO(H)  
    EPD_Send_1(0x82, 0x12);// VCM_DC_SETTING                   
    EPD_Send_1(0x50, 0x97);// VCOM_AND_DATA_INTERVAL_SETTING: VBDF 17|D7 VBDW 97  VBDB 57  VBDF F7  VBDW 77  VBDB 37  VBDR B7

    EPD_lut(0x20,44,&lut_dc_4in2[0]);// LUT_FOR_VCOM
    EPD_lut(0x21,42,&lut_ww_4in2[0]);// LUT_WHITE_TO_WHITE   
    EPD_lut(0x22,42,&lut_bw_4in2[0]);// LUT_BLACK_TO_WHITE
    EPD_lut(0x23,42,&lut_wb_4in2[0]);// LUT_WHITE_TO_BLACK
    EPD_lut(0x24,42,&lut_bb_4in2[0]);// LUT_BLACK_TO_BLACK

    EPD_SendCommand(0x10);//DATA_START_TRANSMISSION_1  
    delay(2);
    for(int i = 0; i < 400*300; i++)EPD_SendData(0xFF);//Red channel

    EPD_SendCommand(0x13);//DATA_START_TRANSMISSION_2
    delay(2);
    return 0;
    
}

unsigned char lut_dc_4in2b[] = 
{
    0x00, 0x17, 0x00, 0x00, 0x00, 0x02, 0x00, 0x17, 0x17, 0x00, 0x00, 
    0x02, 0x00, 0x0A, 0x01, 0x00, 0x00, 0x01, 0x00, 0x0E, 0x0E, 0x00, 
    0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

//R21H
unsigned char lut_ww_4in2b[] = 
{
    0x40, 0x17, 0x00, 0x00, 0x00, 0x02, 0x90, 0x17, 0x17, 0x00, 0x00, 0x02, 0x40, 0x0A, 
    0x01, 0x00, 0x00, 0x01, 0xA0, 0x0E, 0x0E, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

//R22H    r
unsigned char lut_bw_4in2b[] =
{
    0x40, 0x17, 0x00, 0x00, 0x00, 0x02, 0x90, 0x17, 0x17, 0x00, 0x00, 0x02, 0x40, 0x0A, 
    0x01, 0x00, 0x00, 0x01, 0xA0, 0x0E, 0x0E, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

//R24H    b
unsigned char lut_bb_4in2b[] =
{
    0x80, 0x17, 0x00, 0x00, 0x00, 0x02, 0x90, 0x17, 0x17, 0x00, 0x00, 0x02, 0x80, 0x0A, 
    0x01, 0x00, 0x00, 0x01, 0x50, 0x0E, 0x0E, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

//R23H    w
unsigned char lut_wb_4in2b[] =
{
    0x80, 0x17, 0x00, 0x00, 0x00, 0x02, 0x90, 0x17, 0x17, 0x00, 0x00, 0x02, 0x80, 0x0A, 
    0x01, 0x00, 0x00, 0x01, 0x50, 0x0E, 0x0E, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

int EPD_Init_4in2b() 
{
    EPD_Reset();
    EPD_Send_3(0x06,0x17,0x17,0x17);//BOOSTER_SOFT_START
    EPD_SendCommand(0x04);//POWER_ON
    EPD_WaitUntilIdle();
    EPD_Send_1(0x00, 0x0F);//PANEL_SETTING
    EPD_Send_1(0x50,0xF7);// VCOM_AND_DATA_INTERVAL_SETTING

    EPD_SendCommand(0x10);//DATA_START_TRANSMISSION_1  
    delay(2);
    return 0;
}

/* Array of sets describing the usage of e-Papers ----------------------------*/
EPD_dispInfo EPD_dispMass[] =
{
    { EPD_Init_4in2  , EPD_loadA, -1  , 0,         EPD_showB, "4.2 inch"    },// m 12 // IS THIS A TYPO IN THE ORIGINAL SOURCE? SHOULD IT BE EPD_showA??
    { EPD_Init_4in2b , EPD_loadA, 0x13, EPD_loadA, EPD_showB, "4.2 inch b"  },// n 13
    { EPD_Init_4in2b , EPD_loadA, 0x13, EPD_loadA, EPD_showB, "4.2 inch c"  },// o 14
};

/* Initialization of an e-Paper ----------------------------------------------*/
void EPD_dispInit()
{
  // Call initialization function
  EPD_dispMass[EPD_dispIndex].init();

  // Set loading function for black channel
  EPD_dispLoad = EPD_dispMass[EPD_dispIndex].chBk;

  // Set initial coordinates
  EPD_dispX = 0;
  EPD_dispY = 0;
}
