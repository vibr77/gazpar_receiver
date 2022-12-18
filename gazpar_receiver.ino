/*
$$\    $$\ $$$$$$\ $$$$$$$\  $$$$$$$\  
$$ |   $$ |\_$$  _|$$  __$$\ $$  __$$\ 
$$ |   $$ |  $$ |  $$ |  $$ |$$ |  $$ |
\$$\  $$  |  $$ |  $$$$$$$\ |$$$$$$$  |
 \$$\$$  /   $$ |  $$  __$$\ $$  __$$< 
  \$$$  /    $$ |  $$ |  $$ |$$ |  $$ |
   \$  /   $$$$$$\ $$$$$$$  |$$ |  $$ |
    \_/    \______|\_______/ \__|  \__|
                                   ©2022 
----------------------------------------
Author: Vincent BESSON
Project: gazpar_receiver
Version: 0.1
Date: 20221218
-----------------------------------------
*/

// NRF24L01 Lib
#include <SPI.h>
#include <RF24.h>

#include <printf.h>

#include <stdint.h>

#define pinCE   3            
#define pinCSN  2

uint8_t   server_address[5]       = {10, 20, 30, 40, 50};
uint8_t   node_address[5]         = {1, 20, 30, 40, 50};

/* -------CONFIG---------*/
#define LOGDEBUG  0

//DEBUG 
#if LOGDEBUG==1
#define DEBUG_PRINT(x)       Serial.print(x)
#define DEBUG_PRINTDEC(x)    Serial.print(x, DEC)
#define DEBUG_PRINTLN(x)     Serial.println(x)
#define DEBUG_RADIO()        radio.printDetails()
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTDEC(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_RADIO()
#endif

RF24 radio(pinCE, pinCSN);
char message[32]; 

void radioSetup(){

  radio.begin();  
  radio.setChannel(0); 
  radio.setPALevel(RF24_PA_MIN);
  //radio.openReadingPipe(0, adresse); 
  

  radio.openReadingPipe(1, node_address);
  //radio.openWritingPipe(server_address);
  radio.startListening();  
  DEBUG_RADIO();

}

void setup() {
  
  Serial.begin(115200, SERIAL_8N1);
  delay(2000);

  // Shutdown the builtin led
  pinMode(LED_BUILTIN, OUTPUT);
  radioSetup();
}
void loop() {

  delay(500);
  
  if (radio.available()) {
    radio.read(&message, sizeof(message));                        // Si un message vient d'arriver, on le charge dans la variable "message"
    Serial.print("Message reçu : "); Serial.println(message);     // … et on l'affiche sur le port série !
  }

}



