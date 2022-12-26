#include <SPI.h>
#include <RF24.h>

#include <Base64.h> // https://github.com/agdl/Base64
#include <AES.h>    // https://forum.arduino.cc/index.php?topic=88890.0
#include "pwd.h"
AES aes ;


#define pinCE   12             // On associe la broche "CE" du NRF24L01 à la sortie digitale D7 de l'arduino
#define pinCSN  11            // On associe la broche "CSN" du NRF24L01 à la sortie digitale D8 de l'arduino
#define tunnel  "D6E1A"       // On définit le "nom de tunnel" (5 caractères) à travers lequel on va recevoir les données de l'émetteur


#define LOGDEBUG  1
//DEBUG 
#if LOGDEBUG==1
#define DEBUG_PRINT(x)       Serial.print(x)
#define DEBUG_PRINTDEC(x)    Serial.print(x, DEC)
#define DEBUG_PRINTHEX(x)    Serial.print(x, HEX)
#define DEBUG_PRINTLN(x)     Serial.println(x)
#define DEBUG_RADIO()        radio.printDetails()
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTDEC(x)
#define DEBUG_PRINTHEX(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_RADIO()
#endif


RF24 radio(pinCE, pinCSN);    // Instanciation du NRF24L01

const uint16_t maxMessageSize = 16 * 16;               // message size + 1 NEEDS to be a multiple of N_BLOCK (16)
const uint16_t maxBase64MessageSize = maxMessageSize * 4 / 3; // 3 bytes become 4 bytes in Base64

uint8_t key128bits[] = AES_KEY;                       // ref in pwd.h ex #define AES_KEY "@cvQdSlWZersD7x!"
const uint8_t iv128bits[]  = AES_IV;                  // ref in pwd.h "ZbkssDaAaBbCcKwx9"

const byte adresse[6] = tunnel;                       // Mise au format "byte array" du nom du tunnel
char message[32];                                     // Avec cette librairie, on est "limité" à 32 caractères par message

const char* encode_128bits(const char* texteEnClair) {

  // static allocation of buffers to ensure the stick around when returning from the function until the next call
  static uint8_t message[maxMessageSize];
  static uint8_t cryptedMessage[maxMessageSize];
  static char base64Message[maxBase64MessageSize + 1]; // we want to print, so ensure we always have a trailing null
  base64Message[maxBase64MessageSize] = '\0';

  uint8_t iv[N_BLOCK]; // memory is modified during the call
  memcpy(iv, iv128bits, N_BLOCK);

  memset(message, 0, maxMessageSize); // padding with 0
  memset(cryptedMessage, 0, maxMessageSize); // padding with 0
  memset(base64Message, 0, maxBase64MessageSize); // padding with NULL char

  uint16_t tailleTexteEnClair = strlen(texteEnClair) + 1; // we grab the trailing NULL char for encoding
  memcpy(message, texteEnClair, tailleTexteEnClair);

  if ((tailleTexteEnClair % N_BLOCK) != 0) tailleTexteEnClair = N_BLOCK * ((tailleTexteEnClair / N_BLOCK) + 1);
  int n_block = tailleTexteEnClair / N_BLOCK;

  aes.set_key(key128bits, 128);
  aes.cbc_encrypt(message, cryptedMessage, n_block, iv); // iv will be modified
  aes.clean();

  Base64.encode(base64Message, (char*) cryptedMessage, tailleTexteEnClair);
  return base64Message;
}
void debughex(const char * message,int len){
  int c;
  for (int n = 0; n <= len; n++){
    c=message[n];
    DEBUG_PRINT("0x");
    DEBUG_PRINT(c < 16 ? "0" : "");
    DEBUG_PRINTHEX(c);
    DEBUG_PRINT(" ");
  }
   DEBUG_PRINTLN(" ");
}

const char* decode_128bits(const char* texteEnBase64,int len)
{
  // static allocation of buffers to ensure the stick around when returning from the function until the next call
  static uint8_t message[maxMessageSize];
  static uint8_t cryptedMessage[maxMessageSize];

  uint8_t iv[N_BLOCK]; // memory is modified during the call
  memcpy(iv, iv128bits, N_BLOCK);

  memset(message, 0, maxMessageSize); // padding with 0
  memset(cryptedMessage, 0, maxMessageSize); // padding with 0

  //Base64.decode((char*) cryptedMessage, (char*) texteEnBase64, strlen(texteEnBase64));
  //int n_block = Base64.decodedLength((char*) texteEnBase64, strlen(texteEnBase64)) / N_BLOCK;
  int n_block=len/ N_BLOCK;
  aes.set_key(key128bits, 128);
  aes.cbc_decrypt((byte *)texteEnBase64, message, n_block, iv); // iv will be modified
  aes.clean();

  return (char*) message;
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void setup() {
  // Initialisation du port série (pour afficher les infos reçues, sur le "Moniteur Série" de l'IDE Arduino)
  Serial.begin(9600);
  delay(2000);

  Serial.println("Récepteur NRF24L01");
  Serial.println("");

  // Partie NRF24
  radio.begin();                      // Initialisation du module NRF24
  radio.openReadingPipe(0, adresse);  // Ouverture du tunnel en LECTURE, avec le "nom" qu'on lui a donné
  radio.setPALevel(RF24_PA_HIGH);      // Sélection d'un niveau "MINIMAL" pour communiquer (pas besoin d'une forte puissance, pour nos essais)
  radio.startListening();             // Démarrage de l'écoute du NRF24 (signifiant qu'on va recevoir, et non émettre quoi que ce soit, ici)

  //test_encrypt_cbc();
}

void loop() {
  // On vérifie à chaque boucle si un message est arrivé
  if (radio.available()) {
    radio.read(&message, sizeof(message));                        // Si un message vient d'arriver, on le charge dans la variable "message"
    Serial.print("Message reçu cyphered: "); 
    debughex(message,strlen(message));
    //Serial.println(message); 
    
    Serial.print("Message uncrypted:"); 
 
    const char* decodedPtr = decode_128bits(message,strlen(message));
  
    Serial.println(decodedPtr);
    
    String bat = getValue(decodedPtr,',',1);
    String bat_val= bat.substring(2);
     Serial.println("batterie:"+bat_val);
     //  4,2V --> 100% et 3,2 --> 0%
    int bat_percent=(bat_val.toInt()-3200)/10;

    Serial.print("batterie %:");
    Serial.println(bat_percent);
    String indx = getValue(decodedPtr,',',2);
    
    Serial.println(indx);
    String indx_val= indx.substring(2);
     Serial.println(indx_val);
    //const char* encodedPtr = encode_128bits(message);
    //Serial.println(encodedPtr);
  }
  delay(5000);
  //Serial.println("here");

  //test_encrypt_cbc();
}