#include <SoftwareSerial.h>
#define DEBUG true


//flowers
#define FLOWER_SALON 7
#define BUTTON_MANUALLY_WATER_SALON 8
#define INTERVAL_WATER_FLOWERS 10800000 //3hours
#define TIME_TO_WATER_FLOWERS 100
unsigned long previous_time = 0;
unsigned long start_watering = 0;
boolean watering = false;
boolean send_request = false;
//flowers
//windows
#define WINDOW_SALON 4
#define INTERVAL_SEND_STATUS 15000
unsigned long window_previous_time = 0;
//windows
 
SoftwareSerial esp8266(5,6); // serial wiring


void setupFlowers()
{
  pinMode(FLOWER_SALON, OUTPUT);
  digitalWrite(FLOWER_SALON, HIGH);
  pinMode(BUTTON_MANUALLY_WATER_SALON, INPUT_PULLUP);
}

void setupWindows()
{
  pinMode(WINDOW_SALON, INPUT_PULLUP);
}

void setup()
{
   setupFlowers();
   setupWindows();
    
   Serial.begin(9600);
   esp8266.begin(9600);
   esp8266.setTimeout(3000);    
   sendData("AT+CWJAP=\"NAZWA_WIFI\",\"HASLO_WIFI\"",8000,DEBUG);
   sendData("AT+CWMODE=1",1000,DEBUG);
   sendData("AT+CIPMUX=0",1000,DEBUG);
   Serial.println("Started SmartKarol");
}

void startStopWatering(boolean watering, int flower_id) {
  if (watering) {
    digitalWrite(flower_id, LOW);
  } else {
    digitalWrite(flower_id, HIGH);
  }
}

void waterFlower(int flower_id, int time_to_water_flower)
{
  unsigned long current_time = millis();
  
  if ( (current_time - previous_time) >= INTERVAL_WATER_FLOWERS ) {
    previous_time = current_time;
    start_watering = current_time;
    watering = true;
  }

  if ( (current_time - start_watering) >= time_to_water_flower) {
    watering = false;
  }

  if (digitalRead(BUTTON_MANUALLY_WATER_SALON) == LOW) {
      Serial.println("wcisniety");
      startStopWatering(true, flower_id);
      watering = true;
  } else {
//      Serial.println("NIE");
      startStopWatering(watering, flower_id); 
  }  


  if (watering) {
     notifyWater(flower_id);
  } else {
    send_request = false;
  }

}

void notifyWindow(int window_id) {

  unsigned long current_time = millis();
  String window_state = "";
  
  if ( (current_time - window_previous_time) >= INTERVAL_SEND_STATUS ) {
    window_previous_time = current_time;
    if (digitalRead(window_id) == HIGH) {
      window_state = "1";
    } else {
      window_state = "0";
    }
    
    String string_window_id = String(window_id);
    sendData("AT+CIPSTART=\"TCP\",\"example.com\",80",500,DEBUG);
    delay(50);
    sendData("AT+CIPSEND=61",100,DEBUG); //68 is the length of bellow command
    //for example, for request GET http://example.com/?action=water&place=1 is 44chars + 4 = AT+CIPSEND=48 chars.
    sendData("GET http://example.com/?action=window&window_id="+string_window_id+"&state="+window_state,500,DEBUG);
    delay(50);
    sendData("AT+CIPCLOSE", 100, DEBUG);
  } 

}
void notifyWater(int flower_id)
{
  if (!send_request) {
    send_request = true;
    String string_flower_id = String(flower_id);
    sendData("AT+CIPSTART=\"TCP\",\"bialkowskikarol.pl\",80",500,DEBUG);
    delay(50);
    sendData("AT+CIPSEND=54",100,DEBUG);
    sendData("GET http://examepl.com/?action=watering&flower_id="+string_flower_id,500,DEBUG);
    delay(50);
    sendData("AT+CIPCLOSE", 100, DEBUG);
  }
}
 
void loop()
{
  waterFlower(FLOWER_SALON, TIME_TO_WATER_FLOWERS); 
  notifyWindow(WINDOW_SALON);
}


 
String sendData(String command, const int timeout, boolean debug)
{
 String response = "";
 
 esp8266.println(command);
 if (debug) {
   Serial.println("poszlo");
   Serial.println(command);
 }
 
 long int time = millis();
 
 while( (time+timeout) > millis())
 {
   while(esp8266.available())
   {
     char c = esp8266.read(); 
     response+=c;
   } 
 }
 
 if(debug) {
    Serial.println("#START#");
    Serial.print(response);
    Serial.println("#END");
 }

 if (response.indexOf("+IPD") > 0) {
  //on the response is prefix +IPD
      
      int length_ipd_prefix = 7; //+IPD,X:
      String parse_response = response.substring(response.indexOf("+IPD"), response.length());
      int length_response = parse_response.substring(5, 6).toInt() + length_ipd_prefix;
      parse_response = parse_response.substring(length_ipd_prefix, length_response);
      //get www response and pass to parse_response
      //for example: if response is: +IPD,7:D1=HIGH
      //below function run Action for D1 ( gpio1 digital ) and change state to HIGH
      
      if (parse_response[0] == 'D' || parse_response[0] == 'A')
      {
        if (parse_response[2] == '=') {
          runAction(parse_response);
        } 
      }
  }
 
 return response;
}

void runAction(String action)
{
  String state = action.substring(3, action.length());
//
//  if (state == "HIGH") {
//    digitalWrite(7, HIGH);
//    Serial.println("wysoki dla " + action.substring(0, 2));
//  }
//
//  if(state == "LOW") {
//    digitalWrite(7, LOW);
//    Serial.println("niski dla " + action.substring(0, 2));
//  }
}
