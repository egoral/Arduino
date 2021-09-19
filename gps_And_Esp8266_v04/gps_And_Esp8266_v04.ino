#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#define _SS_MAX_RX_BUFF 64 // RX buffer size for TinyGPS
/*
   This sample sketch demonstrates the normal use of a TinyGPS++ (TinyGPSPlus) object.
   It requires the use of SoftwareSerial, and assumes that you have a
   4800-baud serial GPS device hooked up on pins 4(rx) and 3(tx).
*/
static const uint32_t baud = 9600;

const byte interruptPin = 2;

static const int espRXPin = 3, espTXPin = 4;
// static const uint32_t espBaud = Baud;

static const int gpsRXPin = 6 , gpsTXPin = 5;
// static const uint32_t gpsBaud = Baud;

// static const uint32_t serialBaud = Baud;

// EG 
String ag_ismi = "EG-CITS";
String ag_sifresi = "elbd5369";
#define TIMEOUT 5000 // mS
SoftwareSerial esp (espRXPin, espTXPin);
//EG

bool sendFlag = false;
long timePassed = 0;
bool wifiConnected = false;
float latit = 0.00f;
float longt = 0.00f;
float anchorLatit = 0.00f;
float anchorLongt = 0.00f;
bool printDetail = true;
long espLoopDuration = 6000; // in microseconds

String server = "www.bodeg.net"; // www.example.com &anchorLatit=41.017586&anchorLongt=29.126043

// String uriPost = "TrackerDatabase/php/insertLocationDataPost.php";// our example is bla_bla.php
String uriGet  = "TrackerDatabase/php/updateLocationData.php";// our example is bla_bla.php
String userName = "adir";
String deviceName = "BellaVita";

String data = "";

int resetCounter = 0;
int resetCounterValue = 10;
int resetPin = 12;

int eeAddress = 0; //EEPROM address to start reading from



// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial gps1(gpsRXPin, gpsTXPin);

// the controls for switch
const int toggleSwitch = 10; // pin of the switch
int switchValue = 1 ;



void setup()
{
  Serial.begin(baud);

  eeAddress = 0;
  EEPROM.get(eeAddress, anchorLatit);
  eeAddress += sizeof(float); //Move address to the next byte after float 'anchorLatit'.
  EEPROM.get(eeAddress, anchorLongt);
  Serial.println(anchorLatit);
  Serial.println(anchorLongt);


  esp.begin(baud);
  gps1.begin(baud);
  espReset();

  digitalWrite(resetPin, HIGH);
  pinMode(resetPin, OUTPUT);

  // attachInterrupt(digitalPinToInterrupt(interruptPin), getAnchorPosition, CHANGE);
  attachInterrupt(0, getAnchorPosition, FALLING);

  char incomingByte[] = "";


  //  analogReference(INTERNAL);

  Serial.println(F("DeviceExample.ino"));
  Serial.println(F("A simple demonstration of TinyGPS++ with an attached GPS module"));
  Serial.print(F("Testing TinyGPS++ library v. "));
  Serial.println(TinyGPSPlus::libraryVersion());
  Serial.println(F("by Mikal Hart"));
  Serial.println(F(""));
  timePassed = millis();
  //espLoop();
  //Serial.println(wifiConnected);

  // The switch
  pinMode(toggleSwitch, INPUT);

}

void loop()
{
  //  Serial.println(timePassed);
  gpsInfo();
  if (millis() - timePassed > espLoopDuration)
  {

    timePassed = millis();
    if ((sendFlag) & (latit != 0) & (latit > 1) & (longt > 1 ) )
    {

      Serial.print(F("Send -> "));
      Serial.print(F("Latit :"));
      Serial.print(latit, 6);
      Serial.print(F(" Longt :"));
      Serial.print(longt, 6);
      Serial.print(F(" timePassed :"));
      Serial.print(timePassed);
      Serial.print(F(" millis :"));
      Serial.println(millis());
      sendFlag = false;
      resetCounter = resetCounter + 1;
      Serial.print(F("reset counter : ")); Serial.println(resetCounter);
      if ( resetCounter > resetCounterValue ) digitalWrite(resetPin, LOW);
      espLoop();
      Serial.print(F("wifiConnected :"));
      Serial.println(wifiConnected);

    }
  }
}


void espLoop()
{
  //  if(wifiConnected == false)
  if (sendCommand("AT", "OK"))
  {
    //    esp.begin(espBaud);
    esp.listen();
    //    sendCommand("AT","OK");
    //ESP modülümüz ile bağlantı kurulup kurulmadığını kontrol ediyoruz.
    if (esp.available())
    {
      String inData = esp.readStringUntil('\n');
      Serial.print(F("ESP OK: "));
      //       Serial.println(inData);
      sendCommand("AT+CIFSR", "ok");
      //        Serial.println(ag_ismi);
      //        Serial.println(ag_sifresi);
    }
    else
    {
      Serial.println(F("No ESP: "));
    }
    if (sendCommand("AT", "OK")); //if(esp.find("OK"))
    //esp modülü ile bağlantıyı kurabilmişsek
    //modül "AT" komutuna "OK" komutu ile geri dönüş yapıyor.
    {
      sendCommand("AT+CWMODE=1", "OK");
      sendCommand("AT+CIPMUX=1", "OK");
      String baglantiKomutu = "AT+CWJAP=\"" + ag_ismi + "\",\"" + ag_sifresi + "\"";
      sendCommand(String("AT+CWJAP=\"" + ag_ismi + "\",\"" + ag_sifresi + "\""), "OK");
      //     sendCommand("AT+CWLAP","OK");      // List available SSiD
      sendCommand("AT+CIFSR", "OK");
      wifiConnected = true;
      delay(1000);
      Serial.println(F("GoTo -> httpGET..."));
      Serial.print(F("Data returned :"));
      Serial.println(data);
      // httpPost();
      httpGet(data);
    }
  }
  else
  {
    // String inData = esp.readStringUntil('\n');
    if (sendCommand("AT", "OK"))
    {
      esp.listen();
      Serial.println(F("Still ESP "));
      sendCommand("AT", "OK");
      Serial.println(sendCommand("AT+CIFSR", "ok"));
      delay(1000);
      //  httpPost();
      httpGet(data);
    }
    else
    {
      Serial.println(F("no wifi"));
      wifiConnected = false;
      espLoop();
    }
  }


}


void gpsInfo()
{
  // This sketch displays information every time a new sentence is correctly encoded.
  gps1.listen();
  if (gps1.available() > 0)
    if (gps.encode(gps1.read()))
      //Serial.println(gps.satellites.value());
      //Serial.println(gps.encode(ss.read()));
      displayInfo();

  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println(F("No GPS: check wiring."));
    while (true);
  }
}


void displayInfo()
{
  sendFlag = true;


  if (gps.location.isValid())
  {
    latit = gps.location.lat();
    longt = gps.location.lng();
    if (printDetail)
    {
      Serial.print(F("Location: "));
      Serial.print(latit, 6);
      Serial.print(F(" , "));
      Serial.print(longt, 6);
    }
  }
  else
  {
    Serial.print(F("INVALID"));
  }


  if (gps.date.isValid())
  {
    if (printDetail)
    {
      Serial.print(F("  Date/Time: "));
      Serial.print(gps.date.year());
      Serial.print(F("/"));
      Serial.print(gps.date.month());
      Serial.print(F("/"));
      Serial.print(gps.date.day());

    }
  }
  else
  {
    Serial.print(F("INVALID"));
  }


  if (gps.time.isValid())
  {
    if (printDetail)
    {
      data = "id=" + userName + "&device=" + deviceName;
      data += "&date=" + String(gps.date.year());
      if (gps.date.month() < 10) data += "0";
      data += String(gps.date.month());
      if (gps.date.day() < 10) data += "0";
      data += String(gps.date.day()) + "&time=";
      int gpsHour = gps.time.hour() + 3;
      if (gpsHour > 24) gpsHour = gpsHour - 24;
      int gpsMinute = gps.time.minute();
      int gpsSecond = gps.time.second();

      Serial.print(F(" "));
      if (gpsHour < 10)
      {
        Serial.print(F("0"));
        data += "0";
      }
      Serial.print(gpsHour);
      data += String(gpsHour);
      Serial.print(F(":"));
      if (gpsMinute < 10)
      {
        Serial.print(F("0"));
        data += "0";
      }
      Serial.print(gpsMinute);
      data += String(gpsMinute);
      Serial.print(F(":"));
      if (gpsSecond < 10)
      {
        Serial.print(F("0"));
        data += "0";
      }
      Serial.print(gpsSecond);
      data += String(gpsSecond);
      Serial.print(F("."));
      if (gps.time.centisecond() < 10) Serial.print(F("0"));
      Serial.print(gps.time.centisecond());
      data += "&lat=" + String(latit , 6) + "&long=" + String(longt , 6);
      data += "&anchorLatit=" + String(anchorLatit , 6) + "&anchorLongt=" + String(anchorLongt , 6);


      switchValue = digitalRead(toggleSwitch); 
      if (switchValue == HIGH)
        {
          data += "&sendTweet=" + String(1);
        }
        else
        {
          data += "&sendTweet=" + String(0);
        }
      
    }
  }
  else
  {
    Serial.print(F("INVALID"));
  }
  if (printDetail) Serial.println();
  //  Serial.print(F("Data prepared :"));
  //  Serial.println(data);
}

boolean sendCommand(String cmd, String ack)
{
  esp.println(cmd);         // Send "AT+" command to module
  if (!echoFind(ack))       // timed out waiting for ack string
    return true;              // ack blank or ack found
}

boolean echoFind(String keyword)
{
  byte current_char = 0;
  byte keyword_length = keyword.length();
  long deadline = millis() + TIMEOUT;
  while (millis() < deadline)
  {
    if (esp.available())
    {
      char ch = esp.read();
      Serial.write(ch);
      if (ch == keyword[current_char])
        if (++current_char == keyword_length)
        {
          Serial.println();
          return true;
        }
    }
  }
  return false; // Timed out
}






void httpGet(String getData)
{
  //  getData = "id=bora&device=doru&lat=19&long=19";
  Serial.print(F("httpGet: "));
  Serial.println(getData);
  Serial.println(server);
  //  Serial.print("AT+CIPSTART=1,\"TCP\",\"");
  //  Serial.print(server);
  //  Serial.println("\",80");
  esp.print(String("AT+CIPSTART=1,\"TCP\",\""));                     //thingspeak sunucusuna bağlanmak için bu kodu kullanıyoruz.
  esp.print(server);                                                 //AT+CIPSTART komutu ile sunucuya bağlanmak için sunucudan izin istiyoruz.
  esp.println("\",80");                                              //TCP burada yapacağımız bağlantı çeşidini gösteriyor. 80 ise bağlanacağımız portu gösteriyor

  delay(1000);
  if (esp.find((char*)"OK"))
  {
    Serial.println(F("web!"));
  }

  if (esp.find((char*)"Error"))                        //sunucuya bağlanamazsak ESP modülü bize "Error" komutu ile dönüyor.
  {
    Serial.println(F("AT+CIPSTART Error"));
    return;
  }

  // ESP modülümüz ile seri iletişim kurarken yazdığımız komutların modüle iletilebilmesi için Enter komutu yani
  delay(1000);                                                                                // /r/n komutu kullanmamız gerekiyor.



  int getDataLength = getData.length();
  Serial.println(getDataLength);
  getDataLength = 220;
  //  sendCommand("AT","OK");
  //  esp.println("AT");
  esp.print("AT+CIPSEND=1,");                 //veri yollayacağımız zaman bu komutu kullanıyoruz. Bu komut ile önce kaç tane karakter yollayacağımızı söylememiz gerekiyor.
  delay(500);
  esp.println(getDataLength);                 //getData değişkeninin kaç karakterden oluştuğunu .length() ile bulup yazırıyoruz.
  Serial.println(getDataLength);
  delay(500);

  if (esp.find((char*)">"))                          //eğer sunucu ile iletişim sağlayıp komut uzunluğunu gönderebilmişsek ESP modülü bize ">" işareti ile geri dönüyor.
  {                                           // arduino da ">" işaretini gördüğü anda  veriyi esp modülü ile sunucuya yolluyor.
    Serial.println(F("Sending now..."));

    esp.print("GET /");
    esp.print(uriGet);
    esp.print("?");
    esp.print(getData);
    esp.print(" HTTP/1.1\r\n");
    esp.print("Host: ");
    esp.print(server);
    esp.print("\r\n\r\n");
    Serial.println(F("NEW GET Statement : "));
    Serial.print(F("GET /"));
    Serial.print(uriGet);
    Serial.print(F("?"));
    Serial.print(getData);
    Serial.print(F(" HTTP/1.1\r\n"));
    Serial.print(F("Host: "));
    Serial.print(server);
    Serial.println(F("\r\n\r\n"));

    delay(1000);
    //    esp.print("\r\n\r\n");
    if (esp.find((char*)"Error"))                        //sunucuya bağlanamazsak ESP modülü bize "Error" komutu ile dönüyor.
    {
      Serial.println(F("get ile yollarken hata oldu..."));
    }
    else
    {
      Serial.println(F("Get ile data gönderildi..."));
    }
  }
  else
  {
    esp.println("AT+CIPCLOSE");
  }
}

void espReset()
{
  esp.println("AT+RST");
  delay(1000);
  if (esp.find((char*)"OK")) Serial.println(F("ESP Rst"));
}

void getAnchorPosition() {
  Serial.println(F("Int on"));
  //  gpsInfo();
  if ((latit != 0) & (latit > 1) & (longt > 1 ) )
  {
    anchorLatit = latit;
    anchorLongt = longt;
    eeAddress = 0;
    EEPROM.put(eeAddress, anchorLatit);
    eeAddress += sizeof(float); //Move address to the next byte after float 'f'.
    EEPROM.put(eeAddress, anchorLongt);
    eeAddress = 0;
    Serial.println( EEPROM.get( eeAddress, anchorLatit) );
    eeAddress += sizeof(float); //Move address to the next byte after float 'f'.
    Serial.println( EEPROM.get( eeAddress, anchorLongt) );
    //        Serial.print("Interrupt Send -> ");
    //        Serial.print("Latit :");
    //        Serial.print(latit, 6);
    //        Serial.print(" Longt :");
    //        Serial.print(longt, 6);
    //        Serial.print(" anchorLatit :");
    //        Serial.print(anchorLatit, 6);
    //        Serial.print(" anchorLongt :");
    //        Serial.print(anchorLongt, 6);
    //        Serial.print(" timePassed :");
    //        Serial.print(timePassed);
    //        Serial.print(" millis :");
    //        Serial.println(millis());
    //        delay(1000);
    //        espLoop();
    //        Serial.print("End of Interrupt : wifiConnected :");
    //        Serial.println(wifiConnected);
  }
}
