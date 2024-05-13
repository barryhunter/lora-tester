# lora-tester

This repository contains some basic scripts I created for my own use, for testing Lora on microcontroller devices. 

Use for testing communication between deviuces

------------------------

I started with Lora-Challenger devices
https://thepihut.com/products/challenger-rp2040-lora-868mhz

senderB.py
--
senderB.py is a MicroPython script to run on a Challenger device, that just send periodic pings. 
It uses some settings inspired from LongFast modem preset, but just using 868Mhz, not the actual Meshtastic Frequency

Also have micropython reciver, which will upload later

I would actully run two - A+B versions on two different scripts to compare different antennas. The receiver can show which one it receiveing stronger!
Just change the PAYLOAD variabvle on the second device. 

------------------------

Then got a Heltec Lora V3 device - to test out Meshtastic, but it was useful for testing communication with the Lora-Challenger device, as it has a nice screen onboard. 

uLoraReceiver.ino 
--
is an ardino project testing receipt of messages from senderB.py above. (and optionaly a senderA too!) 

Works in three modes - press the button for 1/2 second to switch mode. Only has a rudimentry debounce script, so takes a bit of practice to get the right length of keyporess. 

0) Shows the raw text of the last message, not very interesting with the senderA, but can be useful if sending more elaborate messages
1) Displays the RSSI and SNR of the last message, as well as a average.
2) Shows the RSSI and SNR from two transmittors (used when have a A+B), to see which one received strongest

-------------------------

meshtasticReceiver.ino
--
finally, this one is an adoptation of the above script, but listens on the normal Meshtastic EU-868 region, and LongFast settings (never transmits) 

Can be useful to showing the signal strength (use mode 1) of received packets. Currently can't decode the actual payload, so just displays the receivedd strenght, no more. 


------------

Both the .ino files, are just Ardunio projects. should be directly flashable to Heltec Lora V3 devices (not tested with anything else!) 
Will need to install the Heltec library, as well as make sure select the right board in the Board Manager

If the device prompts about needing a licence, (on the serial monitor) got
http://www.heltec.cn/search/
and enter your serial number. 

The Licence which is needed to send via (example arduino monitor ) is

AT+CHKEY=F9F9F9F9F9F9F9F9F9F9F9F9F9F9F9F9

Which is the key from the heltec results page, with the commas removed

I found this out from
http://community.heltec.cn/t/ht-m00-please-provide-a-correct-license-or-license-activation/5303/13

After flashing my code to the Heltec device, have been able to use https://flasher.meshtastic.org/ to return to normal meshtastic software!

