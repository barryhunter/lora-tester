/* Based on:
 * Heltec Automation Receive communication test example
 * https://github.com/HelTecAutomation/Heltec_ESP32/blob/master/examples/Factory_Test/WiFi_Kit_32_V3_FactoryTest/WiFi_Kit_32_V3_FactoryTest.ino
 * Requires the heltc library to be in stalled in Ardinuo (search for "Heltec ESP32 Dev-Boards" in the libraery managager) 
 * https://github.com/HelTecAutomation/Heltec_ESP32/
 *
 * But adapted to receive packets from a MicroPython Device using 'ulora.py' libary (so its default Lora Modem settings) 
 * eg a RFM95 device on a RP2040 based 'Lora Challenger device'
 * */

#include "LoRaWan_APP.h"
#include "Arduino.h"

#include <Wire.h>  
#include "HT_SSD1306Wire.h"

#define LED 35     // copied from WiFi_Kit_32_V3_FactoryTest.ino
#define BUTTON_PIN 0


///////////////////////////////////////////////////////////////////////////////////////
// 

//RF95_FREQ = 869.525   ##meshtastic
# define RF_FREQUENCY                               869525000 // Hz

#define TX_OUTPUT_POWER                             14        // dBm

// these defines setup to match LongFast on EU_868
#define LORA_BANDWIDTH                              1         // [0: 125 kHz,  1: 250 kHz,  2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR                       11         // [SF7..SF12] 6 / 64,  7 / 128,  8 / 256,  9 / 512,  10 / 1024,	11 / 2048,  12 / 4096
#define LORA_CODINGRATE                             1         // [1: 4/5,  2: 4/6,  3: 4/7,  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        16         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

///////////////////////////////////////////////////////////////////////////////////////


#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 255 // Define the payload size here

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;

int16_t txNumber;
int16_t rxCount;
unsigned long rxLast = 0;
unsigned long displayLast = 0;

//int16_t rssi,rxSize;

bool lora_idle = true;

byte displayRX = 1; //0-show message, 1-meter, 2-AB/TEst
int16_t best_rssi = -255;
int16_t worst_rssi = 255;

//for mode 2
#define LIST_SIZE                                 32 
int16_t listA[LIST_SIZE];
int16_t listB[LIST_SIZE];
int16_t lastA;
int16_t lastB;
byte countA=0; //counts how many slots filled (increment until full)
byte countB=0;
byte nextA=0; //marks the next one (increments and wraps)
byte nextB=0;


////////////////////////////////////////////////////////////////////////////////////////////////

//copied from  WIFI_LoRa_32 factory test code

SSD1306Wire  factory_display(0x3c, 500000, SDA_OLED, SCL_OLED, GEOMETRY_128_64, RST_OLED); // addr , freq , i2c group , resolution , rst

//////////////////////////////////////

void setup() {
    Serial.begin(115200);
    Mcu.begin();
    
    txNumber=0;
    rxCount=0;
    //rssi=0;
  
    RadioEvents.RxDone = OnRxDone;
    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );

    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                               LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                               LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                               0, true, 0, 0, LORA_IQ_INVERSION_ON, true );

    //Radio.SetSyncWord -- untested
    Radio.SetSyncWord(0x2b); //as we working on real meshtastic now!

    pinMode(LED, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);

    Serial.println("Starting (RX Mode)");   

    factory_display.init();                   
    factory_display.clear();
    factory_display.drawString(0, 0, "Listening...");
		factory_display.display();
}

//////////////////////////////////////
//main idle loop, renables lora, after reception

void loop()
{
  if(lora_idle)
  {
    lora_idle = false;
    //Serial.println("into RX mode");
    Radio.Rx(0);
  }
  Radio.IrqProcess( );

//////////////////////////////////////////////////////////////////////////
// display time since last contact (updates when nothing recieved)

  unsigned long myTime = millis(); //seems cant just use sleep, so have to do it ourselfs
  if (myTime - 1000 > displayLast && rxLast > 100) {
    float diff = (myTime - rxLast )/1000.0;

    if (diff > 1) {
      char buffer[40];
      if (diff < 100) {
        sprintf(buffer, "%.0f sec", diff); //can't just use %d to effectivly convert to int
      } else if (diff < 3600) {
        sprintf(buffer, "%0.1f Min", diff/60.0);
      } else {
        sprintf(buffer, "%0.1f Hr", diff/3600.0);
      }
      
      display_message(10,DISPLAY_HEIGHT -10, (String)buffer);
    }
    displayLast = myTime;
  }

//////////////////////////////////////////////////////////////////////////
//handle button presses

  int buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == LOW) { //is inverted, so "is pressed"
    usleep(200);
    buttonState = digitalRead(BUTTON_PIN);
    if (buttonState == LOW) { // still held
      usleep(200);
      buttonState = digitalRead(BUTTON_PIN);
      if (buttonState == HIGH) { //has been released
        displayRX = (displayRX+1) %3;

        display_message(10,10, " Mode = "+String(displayRX)+"  ");
        usleep(200); //further debounce
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////
// event when receive message

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    uint8_t header_from;
    uint8_t header_id;

    rxCount++;
    rxLast = millis();

    if (rssi > best_rssi) {
      best_rssi = rssi;
    } 
    if (rssi < worst_rssi) {
      worst_rssi = rssi;
    } 

    Radio.Sleep();
    digitalWrite(LED, HIGH);

    factory_display.clear();
    factory_display.setFont(ArialMT_Plain_10);
    factory_display.drawString(DISPLAY_WIDTH-50, DISPLAY_HEIGHT -10, (String)rxCount);

//////////////////////////////////////////////////////////////////////////

    if (displayRX == 2 && size == 5) {
      char ident = payload[4]; //the payload as 4 bytes of header, so message starts at [4]. COuld use header_id, for now use the body

      if (ident == 'A') {
        listA[nextA] = rssi+100; //add 100 to turn 0dp into 100 and -80 into 20 (so mostly percentage)
        if (countA < LIST_SIZE) { countA++; }
        nextA = (nextA+1)%LIST_SIZE;
        lastA = rssi+100; 
      } else if (ident == 'B') {
        listB[nextB] = rssi+100;
        if (countB < LIST_SIZE) { countB++; }
        nextB = (nextB+1)%LIST_SIZE;
        lastB = rssi+100; 
      }
      
      int16_t total;
      char buffer[80];
      float avgA = 0;
      float avgB = 0;

      factory_display.setFont(ArialMT_Plain_16);
      if (countA>0) {
        total = 0;
        for(int i=0;i<countA;i++) {
          total += listA[i];
        }
        avgA = (float)total/countA;
        sprintf(buffer, "A%d %3.1f %d", countA, avgA, lastA);
        factory_display.drawStringMaxWidth(0,8, DISPLAY_WIDTH, (String)buffer);
      }
      if (countB>0) {
        total = 0;
        for(int i=0;i<countB;i++) {
          total += listB[i]; 
        }
        avgB = (float)total/countB;
        sprintf(buffer, "B%d %3.1f %d", countB, avgB, lastB);
        factory_display.drawStringMaxWidth(0,24, DISPLAY_WIDTH, (String)buffer);
      }
      if (countA>2 && countB>2) {
        factory_display.setFont(ArialMT_Plain_24);
        if (avgA>avgB) {
          factory_display.drawString(DISPLAY_WIDTH-25, 8, "A");
        } else if (avgA<avgB) {
          factory_display.drawString(DISPLAY_WIDTH-25, 8, "B");
        }
      }
    }

//////////////////////////////////////////////////////////////////////////
// mode to display the current RSSI

    if (displayRX == 1) {
      long percent = map(rssi, worst_rssi, best_rssi, 0, 100);

      char buffer[80];
      sprintf(buffer, "RSSI: %d (%d%%)  %d>%d  S: %d", rssi+100, percent, worst_rssi+100, best_rssi+100,  snr);

      factory_display.setFont(ArialMT_Plain_16);
      factory_display.drawStringMaxWidth(0,5, DISPLAY_WIDTH, (String)buffer);
    }

//////////////////////////////////////////////////////////////////////////
// basic mode - just display last message recieved

    if (displayRX == 0) {
      //rssi=rssi;
      //rxSize=size;
      memcpy(rxpacket, payload, size );
      rxpacket[size]='\0';

      //ulora.py sends the first 4 bytes as 'header', and may contain 00, which works as null termination
      //        header = [header_to, self._this_address, header_id, header_flags]

      header_from = rxpacket[1];
      header_id = rxpacket[2];
      rxpacket[0] = 32; //header_to destination
      rxpacket[1] = 32; //header_from (sender)
      rxpacket[2] = 32; //header_id (incrementing)
      rxpacket[3] = 32; //header_flags (may be null)

      //[2024-03-13 21:15:54] From:7 ID:7 b'52.40971,2.707552 A:85.8m T:21.0C P:99951.2 I:27.0C [2024-03-13 21:15:51]' RSSI:-81.27 SNR:9.75 TX:14

      //estentially the same as the uloper/micropython version
      Serial.printf("From:%d ID:%d '%s' RSSI:%d SNR:%d\r\n", header_from,header_id, rxpacket, rssi,snr);

      //drawStringMaxWidth(int16_t x, int16_t y, uint16_t maxLineWidth, const String &text);
      factory_display.drawStringMaxWidth(0,0, DISPLAY_WIDTH, (String)rxpacket + " ID:" + String(header_id));
    }

    factory_display.display();
    digitalWrite(LED, LOW);
    lora_idle = true;
}


//////////////////////////////////////////////////////////////////////////
// just display a short mesage  (single line only!)
// ... doesnt clear screen, just blanks out the area first

void display_message(int x, int y, String buffer) {
  //factory_display.clear();
  factory_display.setFont(ArialMT_Plain_10);
  DISPLAY_COLOR color = factory_display.getColor(); //probally just WHITE/1!
  factory_display.setColor(BLACK);
  factory_display.fillRect(x,y, factory_display.getStringWidth(buffer), 10);
  factory_display.setColor(color);

  factory_display.drawString(x, y, buffer);
  factory_display.display();
}

//////////////////////////////////////////////////////////////////////////
