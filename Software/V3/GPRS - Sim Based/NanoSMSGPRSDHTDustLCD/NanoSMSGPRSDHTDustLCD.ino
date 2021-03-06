/*
Thanks to Chip McClelland, Kina Smith

Use instructions:
Search for "CHANGE" and modify those values as needed
This can send an sms and upload via gprs. Comment out what is needed.
*/

#include <SoftwareSerial.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

//define FONA pins
#define FONA_RX 7 //comms
#define FONA_TX 8 //comms
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX); //begin Software Serial

//For FONA
int ATtimeOut = 10000; //timeout for AT commands
String response; //globaly accessable response from AT commands (how do you make a function that returns a String?)
int keyTime = 2000; // Time needed to turn on/off the Fona
String url = "http://139.59.43.105:3000/update"; //CHANGE URL here. Usually will not change.
String key = "?key=HAMSS0C87AGJ3CXS&field1="; //CHANGE to API key of the necessary channel
String field2 = "&field2=";
String field3 = "&field3=";

//SMS Setup
char replybuffer[255];

//DHT Sensor
float h, t;
#define DHTPIN 6 //CHANGE if pin different
DHT dht(DHTPIN, DHT22,15);
String ts = String(t);
String hs = String(h);

//LCD Address
LiquidCrystal_I2C lcd(0x3F, 16, 2); //CHANGE if I2C driver address is different

//Dust Sensor
int measurePin = 0;
int ledPower = 5;
int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 9680;
float voMeasured=0.0;
float calcVoltage=0.0;
float dustDensity=0.0;

void setup() {
  //Setup of Arduino USB and GSM communications
  Serial.begin(9600);
  Serial.println("Serial started");
  fonaSS.begin(9600);
  Serial.println("GSM Communication started");

  //DHT Setup
  pinMode(DHTPIN, INPUT);
  dht.begin();
  Serial.println("DHT started");
  
  //LCD Setup
  lcd.init();   // initializing the LCD
  lcd.backlight();
  lcd.home(); lcd.clear();
  lcd.println("System Setup");
  
  //Dust setup
  pinMode(ledPower, OUTPUT);
  pinMode(measurePin, INPUT);
  
  //uncomment this to do a single GET request at start.
  /*
  turnOnFONA();
  Serial.println("Initializing please wait 10sec...");
  delay(10000);
  setupGPRS();
  makeRequest();
  Serial.println("Wait 10sec to power down....");
  delay(10000);
  Serial.println("Powering Down...");
  turnOffFONA();
  */
  Serial.println("Setup Done");
}

void loop() {
  temhum(); //Collects Temp and Humidity data
  dust();  //Collects Dust temperature 
    
    lcd.setCursor(0, 0);
    lcd.print("Cel ");
    lcd.print(t);
    lcd.print(" % ");
    lcd.print(h);
    lcd.setCursor(0, 1);
    lcd.print("DustPM2.5 ");
    lcd.print(dustDensity);

  //Select mode of communication. Comment one out with a "//" before the line to disable that mode
  sms();
  data();

}

void dust (){
  digitalWrite(ledPower, LOW); // power on the LED
  delayMicroseconds(samplingTime);

  voMeasured = analogRead(measurePin); // read the dust value

  delayMicroseconds(deltaTime);
  digitalWrite(ledPower, HIGH); // turn the LED off
  delayMicroseconds(sleepTime);

  // 0 - 5v mapped to 0 - 1023 integer values
  calcVoltage = voMeasured * (5.0 / 1024);

  // linear eqaution taken from http://www.howmuchsnow.com/arduino/airquality/
  // Chris Nafis (c) 2012
  dustDensity = (0.17 * calcVoltage) * 1000;

  Serial.print("Raw: ");
  Serial.print(voMeasured);

  Serial.print("; - Voltage: ");
  Serial.print(calcVoltage);

  Serial.print("; - Dust Density [ug/m3]: ");
  Serial.print(dustDensity);
  Serial.println(";");
}
void sms(){
      Serial.print("SMS sent values as");
      Serial.print(t);
      Serial.print(h);
      Serial.print(dustDensity);
      Serial.println();
      fonaSS.print("AT+CMGF=1\r");
      delay(1000); 
      fonaSS.print("AT+CMGS=\"8780973899\"\r"); //CHANGE number to one of your choice
      delay(1000);
     // fona.print(BinID); 
      fonaSS.print("temp in Cel is ");
      fonaSS.print(t);
      fonaSS.print(" & ");
      fonaSS.print("hum in % is ");
      fonaSS.print(h);
      fonaSS.print(" & ");
      fonaSS.print("Dust PM2.5: ");
      fonaSS.print(dustDensity);     
      fonaSS.print("\r"); 
      delay(1000); 
      fonaSS.println((char)26); 
      fonaSS.println();
      Serial.println("mssg sent");
      delay(10000);
      }
void data(){
    //turnOnFONA(); //turn on board
  Serial.println("Initializing please wait 10sec...");
  flushFONA(); //if there is stuff in the Serial buffer print it
  delay(10000); //delay for 10sec. NOTE: NEEDS to be longer than 3 seconds, 10 works great.
  setupGPRS(); //turn on GPRS, set APN, etc. 
  delay(3000); //just wait for a minute for that settle (probably not needed)
  makeRequest(); //Make Get request and shut down GPRS context.
  Serial.println("Wait 10sec to power down....");
  delay(10000); //This delay is also pretty important. Give it time to finish any operations BEFORE powering it down.
  Serial.println("Powering Down...");
  //turnOffFONA(); //turn off module
  flushFONA(); //clean out the Serial Port in prep for a new cycle.
  for(int i = 0; i < 6; i++) { //wait 1 minute with 10 second interval updates.
            Serial.print("wait til 6......");
            Serial.println(i);
    delay(10000);
  }
}
void temhum(){
   h = dht.readHumidity();
   t = dht.readTemperature();
   Serial.print (h);
   Serial.print (t);
}
void flushFONA() { //if there is anything is the fonaSS serial Buffer, clear it out and print it in the Serial Monitor.
  char inChar;
  while (fonaSS.available()){
    inChar = fonaSS.read();
    Serial.write(inChar);
    delay(20);
  }
}
void setupGPRS() { //all the commands to setup a GPRS context and get ready for HTTP command
  //the sendATCommand sends the command to the FONA and waits until the recieves a response before continueing on. 
  Serial.print("disable echo: ");
  if(sendATCommand("ATE0")) { //disable local echo
    Serial.println(response);
  }
  Serial.print("long errors: ");
  if(sendATCommand("AT+CMEE=2")){ //enable verbose errors
    Serial.println(response);
  }
  Serial.print("at+cmgf=1: ");
  if(sendATCommand("AT+CMGF=1")){ //sets SMS mode to TEXT mode....This MIGHT not be needed. But it doesn't break anything with it there. 
    Serial.println(response);
  }
  Serial.print("at+cgatt=1: ");
  if(sendATCommand("AT+CGATT=1")){ //Attach to GPRS service (1 - attach, 0 - disengage)
    Serial.println(response);
  }
  //AT+SAPBR - Bearer settings for applications based on IP
  Serial.print("Connection Type: GPRS: ");
  if(sendATCommand("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"")){ //3 - Set bearer perameters
    Serial.println(response);
  }
  Serial.print("Set APN: ");
  if(sendATCommand("AT+SAPBR=3,1,\"APN\",\"www\"")){ //sets APN for transaction
    Serial.println(response);
  }
  if(sendATCommand("AT+SAPBR=1,1")) { //Open Bearer
    if(response == "OK") {
      Serial.println("Engaged GPRS");
    } else {
      Serial.println("GPRS Already on");
    }
  }

}
void makeRequest() { //Make HTTP GET request and then close out GPRS connection
  Serial.println("HTTP BEGUN!");
  Serial.print("HTTPINIT: ");
  //this checks if it is on. If it is, it's turns it off then back on again. (This Is probably not needed. )
  if(sendATCommand("AT+HTTPINIT")){ //initialize HTTP service. If it's already on, this will throw an Error. 
    if(response != "OK") { //if you DO NOT respond OK (ie, you're already on)
      Serial.print("term: ");
      if(sendATCommand("AT+HTTPTERM")) { //TURN OFF
        Serial.print("init: ");
        if(sendATCommand("AT+HTTPINIT")) { //TURN ON
          Serial.println(response);
        }
      }
    } else {
      Serial.println(response);
    }
    Serial.println(response);
  }
  if(sendATCommand("AT+HTTPPARA=\"CID\",1")){ //Mandatory, Bearer profile identifier
    Serial.print("HTTPPARA, CID: ");
    Serial.println(response);
  }
  /*
  if(sendATCommand("AT+HTTPPARA=\"URL\",\"--url here--\"")){ //you can just use this command too if you want to input the whole command w/url
    Serial.print("URL: ");
    Serial.println(response);
  }
  */
  Serial.print("Set URL: "); 
  if(sendURL()){ //sets the URL for Sparkfun. Same result as the command above. Lots of other options, see the datasheet: sim800_series_at_command_manual
    Serial.println(response);
  }
  
  Serial.print("GET REQUEST: ");
  if(sendATCommand("AT+HTTPACTION=0")){ //make get request =0 - GET, =1 - POST, =2 - HEAD
    Serial.println(response);
  }
  Serial.println(">>> delay 2k");
  delay(2000); //wait for a bit for stuff to complete
  if(sendATCommand("AT")) { //clear out the serial Port
    Serial.println(response);
  }
  Serial.println(">>> delay 2k");
  delay(2000); //wait for a bit more (may not be needed)
  Serial.print("HTTP Read: ");
  if(sendATCommand("AT+HTTPREAD")){ //Read the HTTP response and print it out
    Serial.println(response);
  }
  Serial.println(">>> delay 2k");
  delay(2000);//wait some more

  if(sendATCommand("AT")) { // clear out the Serial port again
    Serial.println(response);
  } 
  Serial.println(">>> delay 2k");
  delay(2000); //hang out some more, make sure everything is done working
  Serial.print("HTTPTERM: ");
  if(sendATCommand("AT+HTTPTERM")){ //Terminate HTTP session. (You can make multiple HTTP requests while HTTPINIT is active. Maybe even to multiple URL's? I don't know)
    Serial.println(response);
  }
  Serial.print("Disengage GPRS: ");
  if(sendATCommand("AT+SAPBR=0,1")){ //disengages the GPRS context.
    Serial.println(response);
  }
}
boolean sendURL() { //builds url for Sparkfun GET Request, sends request and waits for reponse
  int complete = 0;
  char c;
  String content;
  unsigned long commandClock = millis();                      // Start the timeout clock

  fonaSS.print("AT+HTTPPARA=\"URL\",\"");
  fonaSS.print(url);
  fonaSS.print(key);
  fonaSS.print(t);
  fonaSS.print(field2);
  fonaSS.print(h);
  fonaSS.print(field3);
  fonaSS.print(dustDensity);  
  fonaSS.print("\"");
  fonaSS.println();

  while(!complete && commandClock <= millis() + ATtimeOut) {
    while(!fonaSS.available() && commandClock <= millis()+ATtimeOut);
    while(fonaSS.available()) {
      c = fonaSS.read();
      if(c == 0x0A || c == 0x0D);
      else content.concat(c);
    }
    response = content;
    complete = 1; 
  }
  if (complete ==1) return 1;
  else return 0;
}
boolean sendATCommand(char Command[]) { //Send an AT command and wait for a response
  int complete = 0; // have we collected the whole response?
  char c; //capture serial stream
  String content; //place to save serial stream
  unsigned long commandClock = millis(); //timeout Clock
  fonaSS.println(Command); //Print Command
  while(!complete && commandClock <= millis() + ATtimeOut) { //wait until the command is complete
    while(!fonaSS.available() && commandClock <= millis()+ATtimeOut); //wait until the Serial Port is opened
    while(fonaSS.available()) { //Collect the response
      c = fonaSS.read(); //capture it
      if(c == 0x0A || c == 0x0D); //disregard all new lines and carrige returns (makes the String matching eaiser to do)
      else content.concat(c); //concatonate the stream into a String
    }
    //Serial.println(content); //Debug
    response = content; //Save it out to a global Variable (How do you return a String from a Function?)
    complete = 1;  //Lable as Done.
  }
  if (complete ==1) return 1; //Is it done? return a 1
  else return 0; //otherwise don't (this will trigger if the command times out) 
  /*
    Note: This function may not work perfectly...but it works pretty well. I'm not totally sure how well the timeout function works. It'll be worth testing. 
    Another bug is that if you send a command that returns with two responses, an OK, and then something else, it will ignore the something else and just say DONE as soon as the first response happens. 
    For example, HTTPACTION=0, returns with an OK when it's intiialized, then a second response when the action is complete. OR HTTPREAD does the same. That is poorly handled here, hence all the delays up above. 
  */
}

