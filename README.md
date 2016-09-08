Open bike locker  firmware. (Proof of concept)

More info on [commonbike.com](http://commonbike.com).

### Contents
- [Functionality](#Functionality)
- [Hardware](#Hardware)
- [Documentation](#documentation)
- [API](#API)
- [Known issues](#known-issues)
- [License](#License)

## Functionality

The firmware  supports the following use case(s) (version 0.1):
1) Open empty locker with Mifare card
- Hold mifare card (eg. OV Chipkaart) in front of card reader
- Locker opens (servo)
- Red LED goes on to signal locker is in use
- Green LED goes off to signal locker is in use
- Data is send to The Things Network (TTN)

2) Open used locker with Mifare card
- Hold mifare card (eg. OV Chipkaart) in front of card reader (use same card as in use case 1)
- Locker opens (servo)
- Red LED goes off to signal locker is free
- Green LED goes on to signal locker is free
- Data is send to TTN

3) Try to open used locker with other Mifare card
- Hold mifare card (eg. OV Chipkaart) in front of card reader (other card as in use case 1)
- Red LED blinks to signal locker is in use by another card

4) Press hardware button to open locker from backend 
- Press hardware button
- Data is send to TTN
- Response is received from TTN
- Check response for 'open lock command'
- Clear in use status of the locker (see use case 2)

## Hardware
Hardware used:
- SODAQ Autonomo 
  http://shop.sodaq.com/nl/sodaq-autonomo.html
- SODAQ LoRaBEE module
  http://shop.sodaq.com/nl/sodaq-lorabee-rn2483.html
- MFRC522 Mifare card reader 
  For example: http://www.ebay.nl/itm/NFC-Mifare-RC522-Antenna-Module-RFID-Reader-Tags-IC-Card-Read-and-Write-/262432406729
- Simple servo SG90 to simulate lock open/close
  For example http://www.ebay.nl/itm/1PCS-9G-SG90-Mini-Micro-Servo-For-RC-Robot-Helicopter-Airplane-Car-Boat-C9-/262494744056
- Micro switch 
- Red LED
- Green LED 
- 2x resistors for LED's (+/- 220 Ohm) 
	
Arduino IDE is used for compiling + uploading code for Autonomo. Follow instructions from Sodaq for setting up Arduino IDE for Autonomo.

Hardware connections
*  Autonomo D0 <=> MFRC522 RESET
*  Autonomo D5 <=> MFRC522 MISO
*  Autonomo D6 <=> MFRC522 SS
*  Autonomo D7 <=> MFRC522 MOSI
*  Autonomo D8 <=> MFRC522 CLK
*
*  Autonomo D1 <=> RED LED
*  Autonomo D2 <=> GREEN LED
*  
*  Autonomo D3 <=> SWITCH
*  
*  Autonomo D4 <=> SERVO

Don't forget to allo make power and GND connections:)



## Todo:
Lots ...

- It is probably also possible run the firmware code on the TTN Uno. Will look into 'porting' the code for this. Unfortunately we are just out of reach of a TTN gateway when using the TTN Uno. 
- Add Real Time Clock (RTC) functionality to support use cases based on usage time (eg. 24h anonymity)

## Documentation

More detailed general documentation about the whole project is located in ....

## API

API info ....

## Known issues

Browse open issues and submit new ones related to the firmware in [Issuess] (https://github.com/CommonBike/bike-locker-firmware/issues)

## License

This is open source under the ..... license. 


