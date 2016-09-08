# Open bike locker  firmware. (Proof of concept)
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

defines for low level lock API (in uplink direction) 
 
CMD_GET_SERVER_TIME get current time from server
command = 1 byte
response from server: 6 bytes with datetime structure (to be defined)
 
CMD_INVOKE_DOWNLINK see if server has new data for us
command = 1 byte
response from server: 1 byte with command, x bytes with command parameters (eg. open lock in 1 minute, to be defined)   
 
CMD_SET_LOCKER_STATE sets state of the locker
command = 1 byte
param1 1 byte (required) state => maps 1:1 to veiligstallen; 0 = vrij, 1 = bezet, 2 = geblokkeerd, 3 = gereserveerd, 4 =  buiten werking 
param2 10 bytes (optional) mifareid
 
CMD_UPDATE_LOCKER_USER_ID sets the mifareid of the locker that is currently used (eg. anonymity expires after 24h)
command = 1 byte
param1 10 bytes (required) mifareid

## Known issues

Browse open issues and submit new ones related to the firmware in [Issues] (https://github.com/CommonBike/bike-locker-firmware/issues)

## License

OpenBikeLocker is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or(at your option) any later version.
OpenBikeLocker is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the [GNU Lesser General Public License] 
(http://www.gnu.org/licenses) for more details.


