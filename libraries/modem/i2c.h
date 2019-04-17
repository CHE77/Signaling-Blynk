//**********************************************************
//************************** I2C ***************************
//**********************************************************
bool flag_esp8266 = false;

#if ESP8266_ENABLE

#define I2C_RESET {digitalWrite(A2,LOW);digitalWrite(A3,LOW);delay(1000);digitalWrite(A2,HIGH);digitalWrite(A3,HIGH);i2c_time=millis();flag_esp8266=false;}
#define I2C_OPROS if(millis()-i2c_time>30000)I2C_RESET

#include <Wire.h>

#define I2C_INIT i2c_init();i2c_rx_buf=text;i2c_tx_buf=email_buffer;i2c_enable=1;dtmf0=DTMF;I2C_RESET

TEXT* i2c_tx_buf;
TEXT* i2c_rx_buf;

uint8_t i2c_enable;
uint16_t* dtmf0;
uint32_t i2c_time = 0;

extern MY_SENS *sensors;

// function that executes whenever data is received from master
void receiveEvent(int numBytes)
{
  char buf[BUFFER_LENGTH];
  uint8_t i = 0;
  uint8_t count = 0;
  bool sens_flag = false;  
  i2c_cmd *cmd;

  while (0 <Wire.available() && i < BUFFER_LENGTH) {
    buf[i] = (uint8_t)Wire.read();
    //DEBUG_PRINT(' ');DEBUG_PRINT(buf[i]);DEBUG_PRINT('|');DEBUG_PRINT(buf[i],HEX);
    i++;
  }

  count = i;
  i = 0;

  while(i < count)
  {
    if(count - i >= sizeof(i2c_cmd))
    {
      cmd = (i2c_cmd*)&buf[i]; 

      if(cmd->type == I2C_DATA)
      {
        i+=sizeof(i2c_cmd);

        if(cmd->cmd==I2C_SENS_VALUE)
        {
          if(GET_FLAG(GUARD_ENABLE))
          {
            i2c_sens_value sens_value;

            sens_value.type = cmd->type;
            sens_value.index = cmd->index;
            sens_value.value = sensors->GetValueByIndex(cmd->index);
            uint8_t size = sizeof(i2c_sens_value);
            char* p = (char*)&sens_value;
            for(i = 0; i < size; i++)
            {
              i2c_tx_buf->AddChar(*(p+i));
            }
          } 
        }
        else
        if(cmd->cmd==I2C_SENS_INFO)
        {
          if(GET_FLAG(GUARD_ENABLE))
          {
            *dtmf0 = SENS_GET_NAMES;
          } 
        }
        else
        if(cmd->cmd==I2C_FLAG_NAMES)
        {
          i2c_tx_buf->AddText_P(PSTR(I2C_FLAG));            
          i2c_tx_buf->AddText_P(PSTR(" ALARM GUARD EMAIL"));              
#if MODEM_ENABLE
          i2c_tx_buf->AddText_P(PSTR(" GPRS SMS RING"));
#endif
          i2c_tx_buf->AddChar('\n');           
        }
        else
        if(cmd->cmd==I2C_FLAGS)
        {
          i+=sizeof(i2c_cmd);
          if(GET_FLAG(GUARD_ENABLE))
          {
            if(bitRead(cmd->index,GUARD_ENABLE)==0)
            {
              bitWrite(cmd->index,GUARD_ENABLE,HIGH);
              *dtmf0 = GUARD_OFF;
            } 
          }
          else
          if(bitRead(cmd->index,GUARD_ENABLE))
          {
            bitWrite(cmd->index,GUARD_ENABLE,LOW);
            *dtmf0 = GUARD_ON;            
          }
          flags = cmd->index;          
        }
        continue;
      }
    }
    i2c_rx_buf->AddChar(buf[i++]);
  }
}

// function that executes whenever data is requested from master
void requestEvent()
{
  uint8_t i;
  uint8_t k;
  uint8_t l;
  char buf[32];

  flag_esp8266 = true;

  i2c_time = millis();

  memset(buf, 0, 32);

  for(i = 0; i < 2; i++) buf[i] = START_BYTE;
  
  buf[i++] = flags;

  i+=i2c_tx_buf->GetBytes(buf+i, 27);

  for(k = 0; k < 2; k++) buf[i++] = END_BYTE;

  //memset(buf+i, 0, BUFFER_LENGTH-i);

  Wire.write(buf, 32);
}

void i2c_init()
{ 
  pinMode(A3,OUTPUT);
  digitalWrite(A3, HIGH); // Включаем ESP8266
  Wire.begin(ARDUINO_I2C_ADDR); /* join i2c bus with address */
  Wire.onReceive(receiveEvent); /* register receive event */
  Wire.onRequest(requestEvent); /* register request event */
}

#else

# define I2C_OPROS
# define I2C_INIT

#endif