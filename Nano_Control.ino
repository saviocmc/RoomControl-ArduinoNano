#define VERSION "0.1"

#include "CapacitiveSensor.h"
#include "SoftwareSerial.h"
#include "EEPROM.h"

#define relay 11
#define pushbutton 3
#define DARK 400
#define BRIGHT 300

byte rState;
CapacitiveSensor capSense1(5,12);
SoftwareSerial Bluetooth(4,2);

void setup(){
	pinMode(relay, OUTPUT);
	digitalWrite(relay, EEPROM.read(0));
	Serial.begin(9600);
	Bluetooth.begin(9600);
	pinMode(pushbutton,INPUT_PULLUP);
	capSense1.set_CS_AutocaL_Millis(5000);
}

void loop(){
	checkPushbutton();
	checkCapSense();
	checkLuminosity();
	bluetoothEvent();
}

void serialEvent(){
	char received = Serial.read();
	if(received=='C') serialCommand();
	else if(rState==received-48)
		switchRelay();
}

void bluetoothEvent(){
	while(Bluetooth.available()){
		char received = Bluetooth.read();
		if(rState==received-48)
			switchRelay();
		else if(received=='C') bluetoothCommand();
	}
}

void switchRelay(){
	if(!(rState && analogRead(A7) < BRIGHT))
	digitalWrite(relay, rState=!rState);
	EEPROM.write(0,rState);
	Bluetooth.write(rState);
	Serial.write(rState);
}

void checkPushbutton(){
	static unsigned long pbLastTime=0;
	if(!digitalRead(pushbutton) & 
		millis()-pbLastTime>500){
   		pbLastTime = millis();
   		switchRelay();
   	}
}

void checkCapSense(){
	static unsigned long csLastTime = 0;
	long capSense1_time = capSense1.capacitiveSensor(30);
	if(capSense1_time>1500 & millis()-csLastTime>500){
		csLastTime = millis();
		switchRelay();
		capSense1.reset_CS_AutoCal();
	}
}

void checkLuminosity(){
	int luminosity = analogRead(A7);
	static byte flag = 0;
	flag = (luminosity<BRIGHT|flag);
	if(luminosity<BRIGHT & !rState) switchRelay();
	else if(luminosity>DARK & rState & flag)
		switchRelay(),flag=0;
}

void serialCommand(){
	while(!Serial.available());
	char command = Serial.read();
	switch(command){
	    case '?':
	      Serial.print("Relay State: ");
		  Serial.println(rState?"OFF":"ON");
	      break;
	    case 'L':
	      Serial.print("Luminosity State: ");
		  Serial.println(analogRead(A7));
	      break;
	    default:
	      Serial.println("Invalid Command");
	}
}

void bluetoothCommand(){
	while(!Bluetooth.available());
	char command = Bluetooth.read();
	switch(command){
	    case '?':
	      Bluetooth.print("Relay State: ");
		  Bluetooth.println(rState?"OFF":"ON");
	      break;
	    case 'L':
	      Bluetooth.print("Luminosity State: ");
		  Bluetooth.println(analogRead(A7));
	      break;
	    default:
	      Bluetooth.println("Invalid Command");
	}
}