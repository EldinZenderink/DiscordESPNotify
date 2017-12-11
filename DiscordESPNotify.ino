/*
 *  DiscordESPNotify
 *
 *  Learning project for ESP-8266
 *  Connects through HTTPS with Discord API
 *  Uses Discord BOT for api access
 *
 *  Uses 128x32 OLED display.
 *  
 *  Eldin Zenderink 12-11-2017 MIT License Applies
 */

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

const char* ssid     = "YOURSSID";
const char* password = "YOURPASSWORD";
const String botToken = "YOURDISCORDBOTTOKEN";
const String channelId = "YOURDISCORDCHANNELID";

const char* host = "discordapp.com";
const int httpsPort = 443; //ssl port

void apiRequest(String &response, String apiEndPoint){
    // Use WiFiClientSecure class to create TLS connection
    WiFiClientSecure client;
    client.setNoDelay(1);
    if (!client.connect(host, httpsPort)) {
      Serial.println("connection failed");
      return;
    }
    // Send get request to client
    client.print(String("GET ") + apiEndPoint + " HTTP/1.1\r\n" +
                  "Host: " + host + "\r\n" +
                  "Accept: */*\r\n" +
                  "Authorization:Bot " + botToken + "\r\n" +
                  "User-Agent:DiscordESPNotify (https://github.com/EldinZenderink/DiscordESPNotify, v0.1)\r\n" +
                  "Content-Type: application/json\r\n" + 
                  "Connection: close\r\n\r\n");
  
  
    //Receive anything but the headers
    uint8_t receivedHeaders = 0;
    String received = "";
    while(client.connected()){
      String line = client.readStringUntil('\r');
      if(receivedHeaders == 13){
        received = line;
        break;
      }
      receivedHeaders++;
    }
    response = received;
}

//start stripping until the search query has been found
char *stripStringTill(char c_to_search[], char searchIn[]){

    long int startTime = millis();
    int str_len = strlen(searchIn);
    int search_len = strlen(c_to_search);
    int str_pos = 0;
    int search_pos = 0;
    
    for(str_pos = 0; str_pos < (str_len - search_len); str_pos++){
       if(searchIn[str_pos] == c_to_search[search_pos]){
          for(search_pos = 0; search_pos < search_len; search_pos++){
            if(searchIn[str_pos + search_pos] != c_to_search[search_pos]){
              break;
            }
          }
          if(search_pos == search_len){
             char *toreturn = (char *) malloc(sizeof(char) * (str_len - (str_pos + search_pos)) + 1);
             strncpy(toreturn, searchIn + (str_pos + search_pos), str_len - (str_pos + search_pos) - 1);
             
             toreturn[(str_len - (str_pos + search_pos))] = '\0';
              Serial.print(millis()-startTime, DEC);
              Serial.println("MS - STRIP STRING TIME ");
             return toreturn;
          } 
       }
    }
   
    return "nothing";
}

//start stripping everything after the search query
char *getStringTill(char c_to_search[], char searchIn[]){

    long int startTime = millis();
    int str_len = strlen(searchIn);
    int search_len = strlen(c_to_search);
    int str_pos = 0;
    int search_pos = 0;
    
    for(str_pos = 0; str_pos < (str_len - search_len); str_pos++){
       if(searchIn[str_pos] == c_to_search[search_pos]){
          for(search_pos = 0; search_pos < search_len; search_pos++){
            if(searchIn[str_pos + search_pos] != c_to_search[search_pos]){
              break;
            }
          }
          if(search_pos == search_len){
             char *toreturn = (char *) malloc(sizeof(char) * str_pos + 1);
             strncpy(toreturn, searchIn , str_pos );
             toreturn[str_pos] = '\0';
             
              Serial.print(millis()-startTime, DEC);
              Serial.println("MS - GET STRING TIME ");
             return toreturn;
          }
       }

       

    }
   
    return "nothing";
}

//strip from the beginning and from the end of a string
char* removeStuff(char str[], int beginstrip, int endstrip){
    int str_len = strlen(str);
    int i;
    char *toreturn = (char *) malloc(sizeof(char) * (str_len - (beginstrip + endstrip)) + 1);
    strncpy(toreturn, str + beginstrip, str_len - (beginstrip + endstrip));
    toreturn[(str_len - (beginstrip + endstrip))] = '\0';
    return toreturn;
}

//gets the lastest message from the bot
void getLatestMessage(){

    //get channel info
    String dataReceived;
    String getchannelinfo = "/api/channels/" + channelId;
    apiRequest(dataReceived, getchannelinfo);
    String lastMessageId = dataReceived.substring(168,186);

    //get latest  message
    String getlastmessage = "/api/channels/" + channelId + "/messages/" + lastMessageId;
    apiRequest(dataReceived, getlastmessage);

    //parse the json
    char buf[dataReceived.length()];
    dataReceived.toCharArray(buf, dataReceived.length());
    
    char* usernamepart1 = stripStringTill("\"author\": {\"username\": \"", buf);
    char* username = getStringTill("\"", usernamepart1);     
    
    char* chatmessage1 = stripStringTill("content", buf);    
    char* chatmessage = removeStuff(getStringTill("channel_id", chatmessage1), 4, 4);

    //print the message depending on length
    int msglength = strlen(chatmessage);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    
    if(msglength > 21){ //21 characters max on one line
  
        //setup line buffer
        int i;
        int howmanylines = (int)((float) msglength / 21 + 0.5);
        char linebuffer[howmanylines][21];
        int charcount = msglength;
  
        //fill buffer
        for(i = 0; i < howmanylines; i++){
            int lengthtocpy = 21;
            if(charcount < 21){
              lengthtocpy = (msglength - i);
            }
            strncpy(linebuffer[i], chatmessage + (21 * i), lengthtocpy);
            charcount = charcount - msglength;
        }
        
        if(howmanylines > 3){ //max 3 lines
            for(i = 0; i < howmanylines; i++){
              delay(1000);
              display.clearDisplay();// text display tests  
              display.setCursor(0,0);
              display.print(username); display.print(":");    
              display.setCursor(0,8);
              display.println(linebuffer[i]); 
              if((i + 1) < howmanylines){         
                  display.setCursor(0,16);
                  display.println(linebuffer[1 + i]);    
              }   
              if((i + 2) < howmanylines){
                  display.setCursor(0,32);
                  display.println(linebuffer[2 + i]);                
              }         
              display.display();
              delay(1000);
            }        
        } else {        
            display.setCursor(0,0);
            display.print(username); display.print(":");    
            for(i = 0; i < howmanylines; i++){
              display.setCursor(0,(8 * (i + 1)));
              display.println(linebuffer[i]);         
            }        
            display.display();
        }
    } else {
        //single line of text      
        display.setCursor(0,0);
        display.print(username); display.print(":");    
        display.setCursor(0,8);
        display.println(chatmessage);         
        display.display(); 
    }   
    
    //shows in serial
    Serial.println("USERNAME:");
    Serial.println(username);   
    Serial.println("CHAT MESSAGE:");
    Serial.println(chatmessage); 
   
}


void setup() {
  //start i2c at gpio 0 = scl, gpio 2 = sda
  Wire.begin(0,2);        
  //starts serial
  Serial.begin(115200);
  //start display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)

  //start message
  display.clearDisplay();// text display tests  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("DiscordESPNotify");  
  display.display();
  
  delay(1000);

  //show wifi access point ssid
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Connecting:");  
  display.setCursor(0,8);
  display.println(ssid);
  
  Serial.println();
  Serial.print("connecting to ");
  Serial.println(ssid);
  
  //start wifi connection
  WiFi.begin(ssid, password);

  //show connecting status
  display.setCursor(0,8);

  
  int timeoutcount = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.write('.');
    display.display();

    if(timeoutcount > 10){
        //start wifi connection
        display.setCursor(0,16);
        display.println("Could not connect!");        
        display.setCursor(0,24);
        display.println("Retrying!");
        delay(1000);
        WiFi.begin(ssid, password);
        timeoutcount = 0;              
        display.clearDisplay();
    }
    timeoutcount++;
  }

  //show succes
  display.clearDisplay();// text display tests  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Connected:");  
  display.setCursor(0,8);
  display.println(WiFi.localIP());
  display.display();  
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  //get the latest message continuesly
  getLatestMessage();
}
