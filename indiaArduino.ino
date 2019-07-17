#include <SoftwareSerial.h>
#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN,DHTTYPE);

#define DEBUG true

 //not pin num
#define TEMPER_ON_CODE 1     
#define TEMPER_OFF_CODE 2   
#define LIGHT_ON_CODE 3       
#define LIGHT_OFF_CODE 4    
#define MOVE_UP_CODE 5        
#define MOVE_DOWN_CODE 6       
#define TEMPER_UP_CODE 7
#define TEMPER_DOWN_CODE 8


#define TEMPER_RED_LED 5
#define TEMPER_GREEN_LED 6
#define TEMPER_BLUE_LED 7

#define LIGHT_SSR_ALG A0
#define LIGHT_LED 8

bool lightPinAct=true;
bool temperPinAct=true;
bool temperChanged= false;

int cur_red_light ,cur_blue_light;
int SEND_UP_CODE = 182012;
int SEND_DOWN_CODE =154783;

int lightValue;   //  조도센서에 입력된 빛의 세기
bool isVertical=false;  //침대 기울기 false: 수평

SoftwareSerial esp8266(2,3); // make RX Arduino line is pin 2, make TX Arduino line is pin 3.
                                        // This means that you need to connect the TX line from the esp to the Arduino's pin 2
                                        // and the RX line from the esp to the Arduino's pin 3
 
void setup() {
  Serial.begin(9600);
  esp8266.begin(9600); // your esp's baud rate might be different

  pinMode(LIGHT_LED, OUTPUT);
  digitalWrite(LIGHT_LED, LOW);
  
  pinMode(TEMPER_RED_LED,OUTPUT);
  pinMode(TEMPER_GREEN_LED,OUTPUT);
  pinMode(TEMPER_BLUE_LED,OUTPUT);

  sendData("AT+RST\r\n",2000,DEBUG); // reset module
  sendData("AT+CIOBAUD?\r\n",2000,DEBUG); // check baudrate (redundant)
  sendData("AT+CWMODE=3\r\n",1000,DEBUG); // configure as access point (working mode: AP+STA)
  sendData("AT+CWLAP\r\n",3000,DEBUG); // list available access points
  sendData("AT+CWJAP=\"cdi1\",\"Cdilab*1717\"\r\n",5000,DEBUG); // join the access point
  sendData("AT+CIFSR\r\n",1000,DEBUG); // get ip address
  sendData("AT+CIPMUX=1\r\n",1000,DEBUG); // configure for multiple connections
  sendData("AT+CIPSERVER=1,80\r\n",1000,DEBUG); // turn on server on port 80
}
 
void loop() {

  delay(3000);

  if(temperChanged==true){
    analogWrite(TEMPER_BLUE_LED,cur_blue_light);
    analogWrite(TEMPER_RED_LED,cur_red_light);
    analogWrite(TEMPER_GREEN_LED,0);
  }
  
  else if(temperPinAct==true){
    temperFunction();
  }

  
  else{
    analogWrite(TEMPER_BLUE_LED,0);
    analogWrite(TEMPER_RED_LED,0);
    analogWrite(TEMPER_GREEN_LED,0);
    //Serial.println("temperature function off");
  }


  
  if(lightPinAct==true){
    lightFunction();
  }

  else{
    digitalWrite(LIGHT_LED, LOW);
    //Serial.println("auto light function off");
  }

  
      
  if(esp8266.available()) { // check if the esp is sending a message
    if(esp8266.find("+IPD,")) {
      delay(1000); // wait for the serial buffer to fill up (read all the serial data)
      // get the connection id so that we can then disconnect
      int connectionId = esp8266.read()-48; // subtract 48 because the read() function returns 
                                           // the ASCII decimal value and 0 (the first decimal number) starts at 48
      //esp8266.find("pin="); // advance cursor to "pin="
      //int pinNumber = (esp8266.read()-48)*10; // get first number i.e. if the pin 13 then the 1st number is 1, then multiply to get 10
      //pinNumber += (esp8266.read()-48); // get second number, i.e. if the pin number is 13 then the 2nd number is 3, then add to the first number
      //digitalWrite(pinNumber, !digitalRead(pinNumber)); // toggle pin    

      esp8266.find("command=");
      int commandCode= (esp8266.read()-48);
       
      
      if(commandCode == TEMPER_ON_CODE) {
        
        if(temperPinAct==true){
         // Serial.println("temparture function is already on ");
        }
        
        else{
          temperPinAct=true;
         // Serial.println("temparture function turned on");
        }
      }

      else if(commandCode == TEMPER_OFF_CODE) {
   
        if(temperPinAct==true){
          temperPinAct=false;
          temperChanged=false;
         // Serial.println("temparture function turned off");
        }
        
        else{
          //Serial.println("temparture function is already off");
        }
        
      }

      else if(commandCode == TEMPER_UP_CODE){
        if(temperPinAct==true){
          temperControlFunction(true);               // true: 온도 높이기
          temperChanged=true;
        }
      }

      else if(commandCode ==TEMPER_DOWN_CODE){
        if(temperPinAct=true){
          temperControlFunction(false);        
          temperChanged=true;          
        }
      }
      
      else if(commandCode == LIGHT_ON_CODE){
        
        if(lightPinAct==true){
          //Serial.println("light function is already on");
        }
        
        else{
          lightPinAct=true;
          //Serial.println("light function turned on");
        }
      }

      else if(commandCode == LIGHT_OFF_CODE){

        if(lightPinAct==true){
          lightPinAct=false;
          //Serial.println("light function turned off");
        }
        else{
          //Serial.println("light function is already off");
        }
      }

      else if(commandCode == MOVE_UP_CODE){
        if(isVertical==false){
          sendBedMoveCode(true);
          isVertical=true;
          //Serial.println("bed was folded");
        }
        else{
          //Serial.println("bed is already folded");
        }
      }

      else if(commandCode == MOVE_DOWN_CODE){
        if(isVertical==true){
          sendBedMoveCode(false);
          isVertical=false;
          //Serial.println("bed was unfolded");
        }
        else{
          //Serial.println("bed is already unfolded");
        }
      }
      
      // make close command
      String closeCommand = "AT+CIPCLOSE="; 
      closeCommand+=connectionId; // append connection id
      closeCommand+="\r\n";
      sendData(closeCommand,1000,DEBUG); // close connection
    }
  }
}

void temperFunction(){
  
  delay(2000);
  int tmp = dht.readTemperature();
  Serial.print("temperature : ");
  Serial.println(tmp);

  if(tmp <= 28){
    analogWrite(TEMPER_BLUE_LED,255);
    analogWrite(TEMPER_RED_LED,0);
    analogWrite(TEMPER_GREEN_LED,0);
    cur_red_light=0;
    cur_blue_light=255;
  }
  else if(tmp <= 31){
    analogWrite(TEMPER_GREEN_LED,0);
    analogWrite(TEMPER_RED_LED,127);
    analogWrite(TEMPER_BLUE_LED,127);
    cur_red_light=127;
    cur_blue_light=127;
  }
  else{
    analogWrite(TEMPER_RED_LED,255);
    analogWrite(TEMPER_BLUE_LED,0);
    analogWrite(TEMPER_GREEN_LED,0);
    cur_red_light=255;
    cur_blue_light=0;
  }

}

void lightFunction(){
  Serial.print("light:");
  
  lightValue =  analogRead(LIGHT_SSR_ALG);
  Serial.println(lightValue);
  
  if(lightValue<700){
    digitalWrite(LIGHT_LED,LOW);
  }
  else{
    digitalWrite(LIGHT_LED,HIGH);
  }
}

void sendBedMoveCode(bool upOrDown){

  if(upOrDown == true)
    Serial.write(SEND_UP_CODE);      // 안되면  Serial.write 로 고칠것 
  else
    Serial.write(SEND_DOWN_CODE);
}


/*
* Name: sendData
* Description: Function used to send data to ESP8266.
* Params: command - the data/command to send; timeout - the time to wait for a response; debug - print to Serial window?(true = yes, false = no)
* Returns: The response from the esp8266 (if there is a reponse)
*/

String sendData(String command, const int timeout, boolean debug) {
    String response = "";
    esp8266.print(command); // send the read character to the esp8266
    long int time = millis();
    
    while( (time+timeout) > millis()) {
      while(esp8266.available()) {
        // The esp has data so display its output to the serial window 
        char c = esp8266.read(); // read the next character.
        response+=c;
      }
    }
    
    if(debug) {
      Serial.print(response);
    }
    return response;
}



void temperControlFunction(bool isGoHigh){
  int r,b;
  
  if(isGoHigh==true){
    for( r = cur_red_light, b= cur_blue_light  ; r < cur_red_light+100  ,b >  cur_blue_light-100 ; r++,b--){
      analogWrite(TEMPER_RED_LED, r);
      analogWrite(TEMPER_BLUE_LED, b);
      analogWrite(TEMPER_GREEN_LED,0);
    }
  }

  else{
    for( r=cur_red_light, b=cur_blue_light; r > cur_red_light-100 ,b < cur_blue_light+100 ; r--,b++){
      analogWrite(TEMPER_RED_LED, r);
      analogWrite(TEMPER_BLUE_LED, b);
      analogWrite(TEMPER_GREEN_LED,0);
    }
  }
   
   cur_red_light = r > 255 ? 255 : r;
   cur_red_light = r < 0 ? 0 : r;
   
   cur_blue_light= b > 255 ? 255 : b;
   cur_blue_light = b < 0 ? 0 : b;
}


