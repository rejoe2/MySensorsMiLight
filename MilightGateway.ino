#define MY_DEBUG 
//#define MY_RADIO_NRF24
#define MY_GATEWAY_FEATURE
#include <MyConfig.h>
#include <MySensors.h>

#include <SPI.h>
#define CE_PIN 9     //CE and CSN pin for nrf24 radio
#define CSN_PIN 10
#include <nRF_24L01.h>
#include <avr/power.h>
#include <printf.h>

#include "PL1167_nRF24.h"
#include "MiLightRadio.h"


//#define NODE_ID 109
#define mi_command_repeat 30   //# of times to resend the command to bulb


RF_24 mi_radio(CE_PIN, CSN_PIN);
PL1167_nRF24 prf(mi_radio);
MiLightRadio mlr(prf);
static uint8_t outgoingPacket_off[7] = { 0xB0, 0xF2, 0xEA, 0x04, 0x91, 0x04, 0x00};
uint8_t outgoingPacket[7];

#define sizeofincomingCommand 15 //command comes in text form of 7 uint8_t, with an additional NULL at end
char *incomingCommand;    //array for incoming command
 
boolean mi_radio_state = false;
#define CHILD_ID_IR 1

MyMessage msgIr(CHILD_ID_IR, V_IR_RECEIVE);


void presentation()  
{  
  //Send the sensor node sketch version information to the gateway
  sendSketchInfo("Milight Gateway", "0.1");
  present(CHILD_ID_IR, S_IR);	
  // Register the sensor als IR Node
}

void setup()
{  
mlr.begin();
}

void loop() {
    if (mlr.available()) {
      Serial.print("\n\r");
      uint8_t packet[7];
      size_t packet_length = sizeof(packet);
      mlr.read(packet, packet_length);
	    send(msgIr.set(packet));
      for (int i = 0; i < packet_length; i++) {
       printf("%02X ", packet[i]);
      }
    }
    
    int dupesReceived = mlr.dupesReceived();
    for (; dupesPrinted < dupesReceived; dupesPrinted++) {
      //Serial.print(".");
    }
	 if (My_Receiver.getResults(&My_Decoder)) //if IR data is ready to be decoded
  {
    //1) decode it
    My_Decoder.decode();
    Serial.println("decoding");
    Serial.print("Protocol:");
    Serial.print(Pnames(My_Decoder.decode_type));
    Serial.print(",");
    Serial.print(My_Decoder.decode_type);
    Serial.print(" ");
    Serial.print(My_Decoder.value, HEX);
    Serial.print(" ");
    Serial.println(My_Decoder.bits);
    //FOR EXTENSIVE OUTPUT:
    //My_Decoder.dumpResults();
    //filter out zeros for not recognized codes and NEC repeats
    const char rec_value = My_Decoder.value;
    if (rec_value != 0xffffffff && rec_value != 0x0) {
      char buffer[24];
      uint8_t IrBits = My_Decoder.bits;
      String IRType_string = Pnames(My_Decoder.decode_type);
      char IRType[IRType_string.length() + 1];
      IRType_string.toCharArray(IRType, IRType_string.length() + 1);
      sprintf(buffer, "%i 0x%08lX %i, %s", My_Decoder.decode_type, My_Decoder.value, IrBits, IRType);
      // Send ir result to gw
      send(msgIr.set(buffer));
#ifdef MY_DEBUG
      //2) print results
      //FOR BASIC OUTPUT ONLY:
      Serial.println(buffer);
#endif
}
	
	}


void receive(const MyMessage &message) {
  // The command will be transmitted in V_VAR1 type, in the following 7 byte format: ID1 ID2 ID3 COLOR BRIGHTNESS COMMAND.
  // see https://hackaday.io/project/5888-reverse-engineering-the-milight-on-air-protocol
  Serial.println(message.type);
  if (message.type == V_STATUS)
  {
    //Serial.println("triggered V_STATUS");
    mlr.begin();

    // Change light status
    Serial.println(message.getBool());
    if ( message.getBool())
    {
      Serial.println("here");
      // Serial.println("triggered ON");
      sendCommand (outgoingPacket_on, sizeof(outgoingPacket_on));
    }


    else {
      //  Serial.println("triggered off");
      sendCommand(outgoingPacket_off, sizeof(outgoingPacket_off));
    }
    _begin();
  }


  if (message.type == V_IRSEND)
  {
    // Serial.println("triggered V_IRSEND");
    
    incomingCommand = (char *)message.getCustom();
    Serial.println((char *)message.getCustom());
     //mlr.begin();
     Serial.println("#############");
    for (int i=0; i<sizeof(outgoingPacket); i++)
    { outgoingPacket[i] = 16*char_to_uint8_t(incomingCommand[i*2]) + char_to_uint8_t(incomingCommand[i*2+1]);
      Serial.println(outgoingPacket[i]);
      }
    Serial.println("#############");
    Serial.write(outgoingPacket, 7);
    sendCommand(outgoingPacket, sizeof(outgoingPacket));
    _begin();
  }
/* Copy of existing IR-Code
  if (message.type == V_IR_SEND) {
    irData = message.getString();
#ifdef MY_DEBUG
    Serial.println(F("Received IR send command..."));
    Serial.println(irData);
#endif

    int i = 0;
    char* arg[3];
    unsigned char protocol;
    unsigned long code;
    unsigned int bits;

    char* irString = strdup(irData);
    char* token = strtok(irString, " ");

    while (token != NULL) {
      arg[i] = token;
      token = strtok(NULL, " ");
      i++;
    }
#ifdef MY_DEBUG
    Serial.print("Protocol:"); Serial.print(arg[0]);
    Serial.print(" Code:"); Serial.print(arg[1]);
    Serial.print(" Bits:"); Serial.println(arg[2]);
#endif
    protocol = atoi(arg[0]);
    code = strtoul(arg[1], NULL, 0);
    bits = atoi(arg[2]);
    irsend.send(protocol, code, bits);
    free(irString);
}
*/



}

void sendCommand(uint8_t mi_Command[7], int sizeofcommand)
{ for (int i = 0; i < mi_command_repeat; i++)
  { mlr.write(mi_Command, sizeofcommand);
  }
}

uint8_t char_to_uint8_t(char c) 
{
	uint8_t i;
	if (c <= '9')
		i = c - '0';
	else if (c >= 'a')
		i = c - 'a' + 10;
	else
		i = c - 'A' + 10;
	return i;
}
