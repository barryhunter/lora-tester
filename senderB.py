# This is the B SENDER
# Currently is sending with Meshtastic inspired Modem Settings

# Requires ulora.py from https://github.com/martynwheeler/u-lora/
# or slighty fixed version from https://github.com/barryhunter/u-lora/tree/patch-1


from time import sleep
from ulora import LoRa, ModemConfig, SPIConfig, BROADCAST_ADDRESS, FLAGS_ACK
from machine import Pin
import machine
from machine import I2C
from random import random

#####################

CLIENT_ADDRESS = 12

SERVER_ADDRESS = 2

PAYLOAD = 'B'

RF95_FREQ = 868.0
RF95_POW = 20 #5-23
MODEM_SETTINGS = (0x82, 0xb4, 0x04) #meshtastic setttings, but without the syncword, and different frequency

led = Pin(24, Pin.OUT)   # gen purpose LED on the challenger board

###############################

#GPIO9 – GPIO12 is configured as SPI bus 1 (SPI1) and is connected to the SPI bus
#GPIO9 – LORA_CS
#GPIO10 – LORA_SCK
#GPIO11 – LORA_MOSI
#GPIO12 �� LORA_MISO
#GPIO13 is connected to the reset input of the RFM95W.
#GPIO14, GPIO15 and GPIO18 is connected to DIO0 – DIO2 respectively of the RFM95W.
#GPIO24 is connected to the general-purpose LED.

# Lora Parameters

RFM95_RST = 13

    # spi pin defs  (channel, sck, mosi, miso)

RFM95_SPIBUS = (1, 10, 11, 12)

RFM95_CS = 9
RFM95_INT = 14

# initialise radio

lora = LoRa(RFM95_SPIBUS, RFM95_INT, CLIENT_ADDRESS, RFM95_CS, modem_config=MODEM_SETTINGS,
            reset_pin=RFM95_RST, freq=RF95_FREQ, tx_power=RF95_POW, acks=False)

###############################
# loop and send data

last_rssi = None

while True:

    message = PAYLOAD;
    
    led.on()
    result = lora.send_to_wait(message, SERVER_ADDRESS)
    led.off()

    sleep(2+random()*2)

###############################


