#include "CapacitiveSensor.h"
#include "SoftwareSerial.h"
#include "Photosensor.h"
#include "Relay.h"
#include "IRremote.h"
#include "IRremoteInt.h"

#define IR_VOL_DOWN_BUTTON 0x538AC7
#define IR_VOL_UP_BUTTON 0x518AE7
#define IR_ON_BUTTON 0x514AEB

#define S4_ON_BUTTON 0x20DF10EF
#define S4_VOL_UP_BUTTON 0x20DF40BF
#define S4_VOL_DOWN_BUTTON 0x20DFC03F

CapacitiveSensor capSenseVolDown(5,9);
CapacitiveSensor capSenseVolUp(5,10);
CapacitiveSensor capSenseLamp(5,12);
SoftwareSerial Bluetooth(4,2);
Photosensor photosensor(A7, 300, 500);
const int pushbutton = 3;
decode_results IRsignal;
IRrecv IRsensor(A5);
Relay relay(11);

void setup(){
	Serial.begin(9600);
	Bluetooth.begin(9600);
	pinMode(pushbutton,INPUT_PULLUP);
	IRsensor.enableIRIn();
	IRsensor.blink13(true);
}

void loop(){
	bluetoothEvent();
	checkPushbutton();
	checkLuminosity();
	checkCapSenseLamp();
	checkCapSenseVol();
	checkIR();
	echoLuminosity(&Bluetooth,1000);
	//echoLuminosity(&Serial,1000);
}

void serialEvent(){
	char received = Serial.read();
	if(received=='C') serialInterfaceCommand(&Serial);
	else if(received=='R') relay.setState(1),relay.echoState(&Serial);
	else if(received=='r') relay.setState(0),relay.echoState(&Serial);
}

void bluetoothEvent(){
	while(Bluetooth.available()){
		char received = Bluetooth.read();
		if(received=='C') serialInterfaceCommand(&Bluetooth);
		else if(received=='R') relay.setState(1),relay.echoState(&Bluetooth);
		else if(received=='r') relay.setState(0),relay.echoState(&Bluetooth);
		else if(received=='*'){
			String command = "";
			while(!(received == '#')){
				received = Bluetooth.read();
				command += received;
			}
			if(command == "acender#") relay.setState(1),relay.echoState(&Bluetooth);
			else if(command == "apagar#") relay.setState(0),relay.echoState(&Bluetooth);
		}
	}
}

void checkPushbutton(){
	static unsigned long pbLastTime=0;
	if(!digitalRead(pushbutton) & millis()-pbLastTime>500){
   		pbLastTime = millis();
   		relay.switchState();
   		relay.echoState(&Bluetooth);
   	}
}

void checkCapSenseLamp(){
	static unsigned long cslLastTime = 0;
	long capSenseLamp_time = capSenseLamp.capacitiveSensor(30);
	if(capSenseLamp_time>4000 & millis()-cslLastTime>500){
		cslLastTime = millis();
		relay.switchState();
		relay.echoState(&Bluetooth);
		//capSenseLamp.reset_CS_AutoCal();
	}
}

void checkCapSenseVol(){
	static unsigned long csvLastTime = 0;
	long capSenseVolUp_time = capSenseVolUp.capacitiveSensor(30);
	if(capSenseVolUp_time>4000 & millis()-csvLastTime>100){
		csvLastTime = millis();
		Bluetooth.print("VolUp#");
		//capSenseVolUp.reset_CS_AutoCal();
	}
	long capSenseVolDown_time = capSenseVolDown.capacitiveSensor(30);
	if(capSenseVolDown_time>4000 & millis()-csvLastTime>100){
		csvLastTime = millis();
		Bluetooth.print("VolDown#");
		//capSenseVolDown.reset_CS_AutoCal();
	}
}

/*void checkLuminosity(){
	if(photosensor.getState()==DARK & photosensor.getLastState()==BRIGHT)
		relay.setState(1),relay.echoState(&Bluetooth);

	if(photosensor.getState()==BRIGHT & !photosensor.getLastState()==DARK)
		relay.setState(0),relay.echoState(&Bluetooth);
}*/

void checkLuminosity(){
	static byte wasBright = 0;
	static byte wasDark = 0;
	wasBright = (photosensor.getState()==BRIGHT | wasBright);
	wasDark = (photosensor.getState()==DARK | wasDark);

	if(photosensor.getState()==BRIGHT){
		relay.setSwitchableToOn(false);
		if(relay.getState()==ON && wasDark){
			relay.setState(OFF);
			relay.echoState(&Bluetooth);
			wasDark=0;
		}
	}
		
	else if(photosensor.getState()==DARK){
		relay.setSwitchableToOn(true);
		if(relay.getState()==OFF & wasBright){
			relay.setState(ON);
			relay.echoState(&Bluetooth);
			wasBright=0;
		}
	}
}

void checkIR(){
	static unsigned long IRLastTime = 0;
 	if(IRsensor.decode(&IRsignal)) {
 		if(millis()-IRLastTime>300) {
 			switch (IRsignal.value) {
 				case IR_ON_BUTTON:
 				case S4_ON_BUTTON:
 					IRLastTime = millis();
 					relay.switchState();
 					IRsensor.resume();
 					relay.echoState(&Bluetooth);
 				break;
 				case IR_VOL_UP_BUTTON:
 				case S4_VOL_UP_BUTTON:
 					IRLastTime = millis();
 					IRsensor.resume();
 					Bluetooth.print("VolUp#");
 					break;
 				case IR_VOL_DOWN_BUTTON:
 				case S4_VOL_DOWN_BUTTON:
 					IRLastTime = millis();
 					IRsensor.resume();
 					Bluetooth.print("VolDown#");
 					break;
 				default:
 					IRsensor.resume();
 			}
 			
 		}
 		else IRsensor.resume();
 	}
}

void echoLuminosity(Stream *serialPort, int interval){
	static unsigned long echoLastTime = 0;
	if(millis()-echoLastTime>interval){
		echoLastTime = millis();

		serialPort->write("LumLevel");
		serialPort->print(photosensor.getLuminosity());
		serialPort->print("#");
	}
}

void serialInterfaceCommand(Stream *serialPort){
	while(!serialPort->available());
	char command = serialPort->read();
	switch(command){
	    	case 'R':
	      		relay.echoState(serialPort);
	      	break;
	    	case 'L':
	      		serialPort->write("LumLevel");
			serialPort->print(photosensor.getLuminosity());
			serialPort->print("#");
	      	break;
	    	case '?':
	      		serialPort->print("OK#");
	      	break;
	    	case 'V':
	      		serialPort->println("Version: ");
	      		serialPort->println(VERSION);
	      		serialPort->print("#");
	      	break;
	    	default:
	      		serialPort->println("Invalid Command#");
	}
}