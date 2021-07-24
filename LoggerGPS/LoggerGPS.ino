#include <SoftwareSerial.h>
#include <avr/wdt.h>
#include <TinyGPS.h>
#include <SPI.h>
#include <SD.h>

TinyGPS gps;
SoftwareSerial ss(2, 3); 

static void smartdelay(unsigned long ms);
static void print_float(float val, float invalid, int len, int prec, bool ff);
static void print_int(unsigned long val, unsigned long invalid, int len, bool ff);
static void print_date(TinyGPS &gps, bool ff);
static void print_str(const char *str, int len, bool ff);
static void configFile();
static void reboot();

const int chipSelect = 4;
String wFile = "LOGGER";
File dataFile;

String labels = "SATS;HDOP;LAT;LNG;FIX;DATETIME;ALT;COURSE;SPEED;CARD"; 

void setup()
{
  Serial.begin(115200);
  Serial.println("");
  Serial.println("*** LOGGER GPS ***");
  Serial.println("");
  
  ss.begin(9600);
  
  configFile();
  
  Serial.println(labels);
}

void loop()
{
  digitalWrite(9, LOW);
  digitalWrite(8, LOW);

  bool newData = false;
  unsigned long chars;
  unsigned short sentences, failed;

  // For one second we parse GPS data and report some key values
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (ss.available())
    {
      char c = ss.read();
      // Serial.write(c); // uncomment this line if you want to see the GPS data flowing
      if (gps.encode(c)) // Did a new valid sentence come in?
        newData = true;
    }
  }

  if (newData)
  {
    digitalWrite(9, HIGH);

    float flat, flon;
    unsigned long age, date, time, chars = 0;
    unsigned short sentences = 0, failed = 0; 

    // Sats;
    print_int(gps.satellites(), TinyGPS::GPS_INVALID_SATELLITES, 5, false);
    Serial.print(" ");
    // HDOP;
    print_int(gps.hdop(), TinyGPS::GPS_INVALID_HDOP, 5, false);
    Serial.print(" ");
    gps.f_get_position(&flat, &flon, &age);
    // Latitude;
    print_float(flat, TinyGPS::GPS_INVALID_F_ANGLE, 10, 6, false);
    Serial.print(" ");
    // Longitude;
    print_float(flon, TinyGPS::GPS_INVALID_F_ANGLE, 11, 6, false);
    Serial.print(" ");
    // Fix;
    print_int(age, TinyGPS::GPS_INVALID_AGE, 5, false);
    Serial.print(" ");
    // DateTime;
    print_date(gps, false);
    Serial.print(" ");
    // Alt;
    print_float(gps.f_altitude(), TinyGPS::GPS_INVALID_F_ALTITUDE, 7, 2, false);
    Serial.print(" ");
    // Course;
    print_float(gps.f_course(), TinyGPS::GPS_INVALID_F_ANGLE, 7, 2, false);
    Serial.print(" ");
    // Speed;
    print_float(gps.f_speed_kmph(), TinyGPS::GPS_INVALID_F_SPEED, 6, 2, false);
    Serial.print(" ");
    // Card;
    print_str(gps.f_course() == TinyGPS::GPS_INVALID_F_ANGLE ? "*** " : TinyGPS::cardinal(gps.f_course()), 6, false);
    Serial.print(" ");

    Serial.println("");

    dataFile = SD.open((wFile+".csv"), FILE_WRITE); 
    if (dataFile) {

      // Sats;
      print_int(gps.satellites(), TinyGPS::GPS_INVALID_SATELLITES, 5, true);
      dataFile.print(";");
      
      // HDOP;
      print_int(gps.hdop(), TinyGPS::GPS_INVALID_HDOP, 5, true);
      dataFile.print(";");
      
      // Latitude;
      print_float(flat, TinyGPS::GPS_INVALID_F_ANGLE, 10, 6, true);
      dataFile.print(";");
      
      // Longitude;
      print_float(flon, TinyGPS::GPS_INVALID_F_ANGLE, 11, 6, true);
      dataFile.print(";");
      
      // Fix;
      print_int(age, TinyGPS::GPS_INVALID_AGE, 5, true);
      dataFile.print(";");
      
      // DateTime;
      print_date(gps, true);
      dataFile.print(";");
      
      // Alt;
      print_float(gps.f_altitude(), TinyGPS::GPS_INVALID_F_ALTITUDE, 7, 2, true);
      dataFile.print(";");
      
      // Course;
      print_float(gps.f_course(), TinyGPS::GPS_INVALID_F_ANGLE, 7, 2, true);
      dataFile.print(";");
      
      // Speed;
      print_float(gps.f_speed_kmph(), TinyGPS::GPS_INVALID_F_SPEED, 6, 2, true);
      dataFile.print(";");
      
      // Card;
      print_str(gps.f_course() == TinyGPS::GPS_INVALID_F_ANGLE ? "*** " : TinyGPS::cardinal(gps.f_course()), 6, true);
      dataFile.print(";");

      dataFile.println("");
      
      dataFile.close();
    }else { 
      digitalWrite(8, HIGH);
      
      Serial.println("error opening file..."); 
      Serial.println("");

      reboot();
    }
    
  } 
  else 
  {
    digitalWrite(8, HIGH);
    
    gps.stats(&chars, &sentences, &failed);
    Serial.print(" CHARS=");
    Serial.print(chars);
    Serial.print(" SENTENCES=");
    Serial.print(sentences);
    Serial.print(" CSUM ERR=");
    Serial.println(failed);
    if (chars == 0)
      Serial.println("** No characters received from GPS: check wiring **");
  }

  smartdelay(1000);
}

static void configFile() {
  // initSerial();
  Serial.println("card initializing...");
  if (!SD.begin(chipSelect)) {
    return;
  }
  else {
    bool file = false; //tracks when we found a file name we can use
    int count = 0; //used for naming the file
    while(!file){
      if(SD.exists(wFile + ".csv")) {
        count++; //up the count for naming the file
        wFile = "WDATA" + (String)count; //make a new name
      }
      else file = true; //we found a new name so we can exit loop
    } 
    dataFile = SD.open((wFile + ".csv"), FILE_WRITE); //open file to write labels to CSV
    if (dataFile) {
       dataFile.println(labels);
       dataFile.close(); //we are done with file so close it
    }
    else 
    { //if file doesn't open alert user
     Serial.println("configFile: error opening file..."); 
     Serial.println("");
     reboot();
    }
  }

}

void reboot() {
  // initSerial();
  Serial.println("reboot app..."); 
  Serial.println("");
  // endSerial();

  smartdelay(3000);
  wdt_disable();
  wdt_enable(WDTO_15MS);
  while (1) {}
}

static void smartdelay(unsigned long ms){
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

static void print_float(float val, float invalid, int len, int prec, bool ff) {
  if (val == invalid)
  {
    while (len-- > 1)
      Serial.print('*');
    Serial.print(' ');
  }
  else
  {
    if (!ff) {
      Serial.print(val, prec);
    } else {
      dataFile.print(val, prec);
    }
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;

    if (!ff) {
      for (int i=flen; i<len; ++i)
        Serial.print(' ');
    }
  }
  smartdelay(0);
}

static void print_int(unsigned long val, unsigned long invalid, int len, bool ff)
{
  char sz[32];
  if (val == invalid)
    strcpy(sz, "*******");
  else
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i=strlen(sz); i<len; ++i)
    sz[i] = ' ';
  if (len > 0) 
    sz[len-1] = ' ';

  if (ff) {
    dataFile.print(sz);  
  } else {
    Serial.print(sz);  
  }
  
  smartdelay(0);
}

static void print_date(TinyGPS &gps, bool ff)
{
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long age;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  if (age == TinyGPS::GPS_INVALID_AGE)
    Serial.print("********** ******** ");
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d %02d:%02d:%02d",
        month, day, year, hour, minute, second);
    if (ff) {
      dataFile.print(sz);
    } else {
      Serial.print(sz);
    }
  }
  smartdelay(0);
}

static void print_str(const char *str, int len, bool ff)
{
  int slen = strlen(str);
  if (ff) 
  {
    for (int i=0; i<len; ++i)
        dataFile.print(i<slen ? str[i] : ' ');
  } else 
  {
    for (int i=0; i<len; ++i)
        Serial.print(i<slen ? str[i] : ' ');
  }
  
  smartdelay(0);
}
