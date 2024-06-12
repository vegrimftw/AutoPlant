#include <DHT.h>
#include <DHT_U.h>
#include <LiquidCrystal.h>
#include <elapsedMillis.h>

#define DHTPIN 9   
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//...................Adjustable Variables................................................

  // Pump triggers
  int triggerStart     = 40;              // This % will trigger the pump 
  int triggerStop      = 99;              // Pump will stop at this % 
  
  // Temperature warning LED 
  int tempHi           = 28;              // Yellow/red blink - Too warm
  int tempLo           = 22;              // Blue blink - Too cold 
 
  // Misc
  int clicks           = 5;               // How long to long press button 

  // Soil scaling                                             
  int mapLo            = 180;             // Lower limit raw (wet ~ 220-) [0]     (200)
  int mapHi            = 500;             // Upper limit raw (dry ~ 515+) [1023]  (520)

  // Time(r) 
  int Hz               = 5;               // Code will cycle 5 times per second 
  int seconds          = 30;              // Pump stops after X seconds  

//...................Variables & Constants...............................................

  elapsedMillis timeElapsed;
  unsigned int interval = 1000; 

  int triggerstart;                       // Used for serial input

  const int rs = 3, en = 2, d4 = 4, d5 = 5, d6 = 6, d7 = 7;  // LCD 
  const int Pump        = 8;
  const int Button      = 10;      
  const int LED_Red     = 14;     
  const int LED_Green   = 15;   
  const int LED_Blue    = 16;   
  const int soil1       = A0; 
  const int buzzer      = A1;
  const int soil2       = A2; 
  const int spare       = A3; 
  const int buttonDown  = A4;      // To be added*
  const int buttonUp    = A5;      // To be added* 
  
  int click = 0;
  int click1; 
  int click2; 
  int click3; 
  int click4; 
  int click5; 
  int click6; 
  int click7; 
  int click8; 
  int click9; 
  int click10; 
  int click11;   
  int click12; 

  int prevSoil;
  int newSoil;

  double dxSoil;
  double deltaSoil;
  double prevSoilRaw;
  double newSoilRaw;
  double previousTime;
  double newTime;


//...................Average for-loops...................................................

  const int numReadings1 = 32;     // Average Soil
  int soilReadings[numReadings1];
  int soilIndex = 0;
  int totalSoil = 0;

  const int numReadings2 = 32;      // Average Temperature
  float tempReadings[numReadings2];
  int   tempIndex = 0;
  float totalTemp = 0;

  const int numReadings3 = 32;      // Average Humidity
  float humidReadings[numReadings3];
  int   humidIndex = 0;
  float totalHumid = 0;

  const int numReadings4 = 32;      // Average Heat Index
  float hiReadings[numReadings4];
  int   hiIndex = 0;
  float totalHi = 0;

  const int numReadings5 = 32;      // Average dx/dt 
  float dxdtReadings[numReadings5];
  int   dxdtIndex = 0;
  float totalDxdt = 0;


//...................LCD + Symbols.......................................................

  LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

  byte arrowUp[8] = 
  {
    B00000,
    B00000,
    B00000,
    B11110,
    B00110,
    B01010,
    B10010,
    B00000,
  };

  byte sad[8] = 
  {
    B00000,
    B01010,
    B01010,
    B00000,
    B00000,
    B01110,
    B10001,
    B00000,
  };

  byte celcius[8] = 
  {
    B01000,
    B10100,
    B01011,
    B00100,
    B00100,
    B00100,
    B00011,
    B00000,
  };

  byte drop[8] = 
  {
    B00100,
    B01110,
    B01110,
    B11111,
    B11001,
    B11011,
    B01110,
    B00000,
  };

  byte smiley[8] = 
  {
    B00000,
    B01010,
    B01010,
    B00000,
    B10001,
    B01110,
    B00000,
    B00000,
  };

  byte delta[8] = 
  {
    B00000,
    B00000,
    B00000,
    B00000,
    B00100,
    B01010,
    B11111,
    B00000,
  };

//...............STARTUP...........................................................................

void setup() 
{
dht.begin();

//...................Pin Mode............................................................

  Serial.begin(9600);

  pinMode(8,  OUTPUT);  // Pump
  pinMode(9,   INPUT);  // DHT Sensor
  pinMode(10,  INPUT);  // Button
  pinMode(14, OUTPUT);  // LED Red
  pinMode(15, OUTPUT);  // LED Green
  pinMode(16, OUTPUT);  // LED Blue
  pinMode(A0,  INPUT);  // Soil Sensor
  //pinMode(A1, OUTPUT);  // Spare - buzzer
  pinMode(A2,  INPUT);  // Soil sensor 2
  //pinMode(A3, OUTPUT);  // Spare
  //pinMode(A4,  INPUT);  // Button down
  //pinMode(A5,  INPUT);  // Button Up 

  digitalWrite(Pump, LOW);

//...................LCD splash-screen...................................................
  
  lcd.createChar(1, arrowUp);
  lcd.createChar(2, sad);
  lcd.createChar(3, celcius);
  lcd.createChar(4, drop);
  lcd.createChar(5, smiley);
  lcd.createChar(6, delta);

  lcd.begin(16, 2);

  lcd.print(" AutoWater  ");
  Serial.println(F(" AutoWater - For forgetful people"));
  delay(500);
  
  lcd.setCursor(12, 0);
  lcd.print("by");
  delay(500);

  Serial.println(F("        by Adrian Vegrim"));
  lcd.setCursor(1, 1);
  lcd.print("Adrian Vegrim");
  delay(1500);

//...................Loading screen......................................................

  Serial.println(F("Initializing..."));

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  Starting up!  ");
  lcd.setCursor(0, 1);
  lcd.print("*");
  digitalWrite(LED_Red, HIGH);
  delay(25);
  lcd.setCursor(1, 1);
  lcd.print("*");
  delay(25);
  lcd.setCursor(2, 1);
  lcd.print("*");
  delay(25);
  lcd.setCursor(3, 1);
  lcd.print("*");
  delay(25);
  lcd.setCursor(4, 1);
  lcd.print("*");
  delay(25);
  lcd.setCursor(5, 1);
  lcd.print("*");
  delay(25);
  lcd.setCursor(6, 1);
  lcd.print("*");
  delay(25);
  lcd.setCursor(7, 1);
  lcd.print("*");
  digitalWrite(LED_Red, LOW);
  digitalWrite(LED_Green, HIGH);
  delay(25);
  lcd.setCursor(8, 1);
  lcd.print("*");
  delay(25);
  lcd.setCursor(9, 1);
  lcd.print("*");
  delay(25);
  lcd.setCursor(10, 1);
  lcd.print("*");
  delay(25);
  lcd.setCursor(11, 1);
  lcd.print("*");
  delay(25);
  lcd.setCursor(12, 1);
  lcd.print("*");
  delay(25);
  lcd.setCursor(13, 1);
  lcd.print("*");
  delay(25);
  lcd.setCursor(14, 1);
  lcd.print("*");
  delay(25);
  digitalWrite(LED_Green, LOW);
  digitalWrite(LED_Blue, HIGH);
  lcd.setCursor(15, 1);
  lcd.print("*");
  delay(400);
  digitalWrite(LED_Blue,  LOW);
  lcd.clear(); 
  lcd.setCursor(2, 0);
  lcd.print("Fuck you");
  delay(8);
  lcd.setCursor(6, 1);
  lcd.print("Asshole!");
  delay(32);
  lcd.clear();

}

//...............PROGRAM...........................................................................

void loop() 
{  

//...................Calculated Variables................................................

  int timer      = interval * seconds;     // Timer for disabling the pump if ON too long

  // Limits 
  int safetyLim  = triggerStart * 1.5;     // Pump cannot be activated above this %
  int lowerLim   = triggerStart - 10;      // Lower limit 
  //int upperLim   = triggerStart + 5;     // Upper limit 

  // Soil LED
  int soilGreen  = triggerStart * 1.45;    // Green above this   60% above trigger
  int soilYellow = triggerStart * 1.25;    // Yellow Above this  25% above trigger
  int soilRed    = triggerStart * 1.1;     // Red above this     10% above trigger


//...................Soil moisture sensor................................................

  int soil1_raw = analogRead(A0);
  int soil1 = map(soil1_raw, mapLo, mapHi, 100, 0); 

  int soil2_raw = analogRead(A2);
  int soil2 = map(soil2_raw, mapLo, mapHi, 100, 0); 

  delay(1000/Hz);

//...................DHT Temperature & humidity sensor...................................  

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float hic = dht.computeHeatIndex(t, h);

//...................dx/dt + Average.....................................................

  // dx/dt

    int analogAverage =  ((  analogRead(A0)  +  (analogRead(A2)  )/2)); // This could fuck up, but we'll see

    prevSoilRaw = newSoilRaw;  // Saves soil level read from previous cycle
    newSoilRaw = analogAverage;

    previousTime = newTime;    // Saves the time of the previous cycle
    newTime = millis();        // Filling up/down(+/-)  /  cycle runtime  -  (millis() / 100)

    float dxSoil = ((newSoilRaw - prevSoilRaw) / (newTime - previousTime));
    float dxdtSoil = (dxSoil * dxSoil);   // Avoids negative numbers
    
  // Average Soil 

    int soil = ((soil1 + soil2)/2); 

    // Store the current reading in the array
    soilReadings[soilIndex] = soil;
    soilIndex = (soilIndex + 1) % numReadings1; // Move to the next index, cycling through the array

    // Calculate the total of all readings
    totalSoil = 0;
    for (int i = 0; i < numReadings1; i++) 
    {
      totalSoil += soilReadings[i];  
    }

    // Calculate the average soil value
    int averageSoil = totalSoil / numReadings1;

  // Average Temp

    // Store the current reading in the array
    tempReadings[tempIndex] = t;
    tempIndex = (tempIndex + 1) % numReadings2; // Move to the next index, cycling through the array

    // Calculate the total of all readings
    totalTemp = 0;
    for (int i = 0; i < numReadings2; i++) 
    {
      totalTemp += tempReadings[i];  
    }

    // Calculate the average Temperature value
    float averageTemp = totalTemp / numReadings2;

  // Average Humidity

    // Store the current reading in the array
    humidReadings[humidIndex] = h;
    humidIndex = (humidIndex + 1) % numReadings3; // Move to the next index, cycling through the array

    // Calculate the total of all readings
    totalHumid = 0;
    for (int i = 0; i < numReadings3; i++) 
    {
      totalHumid += humidReadings[i];  
    }

    // Calculate the average Humidity value
    float averageHumid = totalHumid / numReadings3;

  // Average Heat Index

    // Store the current reading in the array
    hiReadings[hiIndex] = hic;
    hiIndex = (hiIndex + 1) % numReadings4; // Move to the next index, cycling through the array

    // Calculate the total of all readings
    totalHi = 0;
    for (int i = 0; i < numReadings4; i++) 
    {
      totalHi += hiReadings[i];  
    }

    // Calculate the average Humidity value
    float averageHI = totalHi / numReadings4;

  // Average dx/dt

    // Store the current reading in the array
    dxdtReadings[dxdtIndex] = dxdtSoil;
    dxdtIndex = (dxdtIndex + 1) % numReadings5; // Move to the next index, cycling through the array

    // Calculate the total of all readings
    totalDxdt = 0;
    for (int i = 0; i < numReadings5; i++) 
    {
      totalDxdt += dxdtReadings[i];  
    }

    // Calculate the average Humidity value
    float avgDeltaSoil = totalDxdt / numReadings5;

//...................Button triggered pump & More values.................................

  click = digitalRead(Button);

  if (click == HIGH) {

    digitalWrite(Pump, LOW); 
    tone(buzzer, 1000);
    delay (150); 
    noTone(buzzer); 
    lcd.clear(); 
    lcd.setCursor(0, 0);
    lcd.print(" Hold button to ");
    lcd.setCursor(0, 1);
    lcd.print(" start watering ");
    delay (1000); 
    lcd.clear();
    
    // Data view

      soil1 = map(analogRead(A0), mapLo, mapHi, 100, 0); 
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0); 

      lcd.setCursor(0, 0);
      lcd.print("S1:");
      lcd.setCursor(4, 0);
      lcd.print(soil1);
      lcd.setCursor(6, 0);  
      lcd.print("%");
      
      lcd.setCursor(9, 0);  
      lcd.print("S2:");
      lcd.setCursor(13, 0);
      lcd.print(soil2); 
      lcd.setCursor(15, 0);  
      lcd.print("%");

      h = dht.readHumidity();
      lcd.setCursor(0, 1);  // LCD 1st row R - Humidity 
      lcd.print("H:"); 
      lcd.setCursor(2, 1);
      lcd.print(h);
      lcd.setCursor(6, 1);
      lcd.print("%");

      t = dht.readTemperature();
      lcd.setCursor(9, 1);  // LCD 2nd row R - Temperature Raw
      lcd.print("T:");
      lcd.setCursor(11, 1);
      lcd.print(t);
      lcd.setCursor(15, 1);
      lcd.write(3);    

      click = digitalRead(Button);
      if (click == HIGH) {
        click1 = 1; 
      } else {
        click1 = 0; }
      digitalWrite(LED_Green,    HIGH);
      digitalWrite(LED_Blue,     HIGH);
      digitalWrite(LED_Red,      HIGH);
      delay(750); 

      soil1 = map(analogRead(A0), mapLo, mapHi, 100, 0); 
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0); 

      lcd.setCursor(4, 0);
      lcd.print(soil1);
      lcd.setCursor(13, 0);
      lcd.print(soil2); 
      h = dht.readHumidity();
      lcd.setCursor(2, 1);
      lcd.print(h);
      lcd.setCursor(6, 1);
      lcd.print("%");
      t = dht.readTemperature();
      lcd.setCursor(11, 1);
      lcd.print(t);
      lcd.setCursor(15, 1);
      lcd.write(3); 

      click = digitalRead(Button);
      if (click == HIGH) {
        click2 = 1; 
      } else {
        click2 = 0; }
      delay(750); 

      soil1 = map(analogRead(A0), mapLo, mapHi, 100, 0); 
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0); 

      lcd.setCursor(4, 0);
      lcd.print(soil1);
      lcd.setCursor(13, 0);
      lcd.print(soil2); 
      h = dht.readHumidity();
      lcd.setCursor(2, 1);
      lcd.print(h);
      lcd.setCursor(6, 1);
      lcd.print("%");
      t = dht.readTemperature();
      lcd.setCursor(11, 1);
      lcd.print(t);
      lcd.setCursor(15, 1);
      lcd.write(3); 

      click = digitalRead(Button);
      if (click == HIGH) {
        click3 = 1; 
      } else {
        click3 = 0; }
      delay(750); 
      
      soil1 = map(analogRead(A0), mapLo, mapHi, 100, 0); 
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0); 

      lcd.setCursor(4, 0);
      lcd.print(soil1);
      lcd.setCursor(13, 0);
      lcd.print(soil2); 
      h = dht.readHumidity();
      lcd.setCursor(2, 1);
      lcd.print(h);
      lcd.setCursor(6, 1);
      lcd.print("%");
      t = dht.readTemperature();
      lcd.setCursor(11, 1);
      lcd.print(t);
      lcd.setCursor(15, 1);
      lcd.write(3); 

      click = digitalRead(Button);
      if (click == HIGH) {
        click4 = 1; 
      } else {
        click4 = 0; }
      delay(750); 

      soil1 = map(analogRead(A0), mapLo, mapHi, 100, 0); 
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0); 

      lcd.setCursor(4, 0);
      lcd.print(soil1);
      lcd.setCursor(13, 0);
      lcd.print(soil2); 
      h = dht.readHumidity();
      lcd.setCursor(2, 1);
      lcd.print(h);
      lcd.setCursor(6, 1);
      lcd.print("%");
      t = dht.readTemperature();
      lcd.setCursor(11, 1);
      lcd.print(t);
      lcd.setCursor(15, 1);
      lcd.write(3); 

      click = digitalRead(Button);
      if (click == HIGH) {
        click5 = 1; 
      } else {
        click5 = 0; }
      delay(750); 

      soil1 = map(analogRead(A0), mapLo, mapHi, 100, 0); 
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0); 

      lcd.setCursor(4, 0);
      lcd.print(soil1);
      lcd.setCursor(13, 0);
      lcd.print(soil2); 
      h = dht.readHumidity();
      lcd.setCursor(2, 1);
      lcd.print(h);
      lcd.setCursor(6, 1);
      lcd.print("%");
      t = dht.readTemperature();
      lcd.setCursor(11, 1);
      lcd.print(t);
      lcd.setCursor(15, 1);
      lcd.write(3); 

      click = digitalRead(Button);
      if (click == HIGH) {
        click6 = 1; 
      } else {
        click6 = 0; }
      delay(750); 

      soil1 = map(analogRead(A0), mapLo, mapHi, 100, 0); 
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0); 

      lcd.setCursor(4, 0);
      lcd.print(soil1);
      lcd.setCursor(13, 0);
      lcd.print(soil2); 
      h = dht.readHumidity();
      lcd.setCursor(2, 1);
      lcd.print(h);
      lcd.setCursor(6, 1);
      lcd.print("%");
      t = dht.readTemperature();
      lcd.setCursor(11, 1);
      lcd.print(t);
      lcd.setCursor(15, 1);
      lcd.write(3); 

      click = digitalRead(Button);
      if (click == HIGH) {
        click7 = 1; 
      } else {
        click7 = 0; }
      delay(750); 

      soil1 = map(analogRead(A0), mapLo, mapHi, 100, 0); 
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0); 

      lcd.setCursor(4, 0);
      lcd.print(soil1);
      lcd.setCursor(13, 0);
      lcd.print(soil2); 
      h = dht.readHumidity();
      lcd.setCursor(2, 1);
      lcd.print(h);
      lcd.setCursor(6, 1);
      lcd.print("%");
      t = dht.readTemperature();
      lcd.setCursor(11, 1);
      lcd.print(t);
      lcd.setCursor(15, 1);
      lcd.write(3); 

      click = digitalRead(Button);
      if (click == HIGH) {
        click8 = 1; 
      } else {
        click8 = 0; }
      delay(750); 

      soil1 = map(analogRead(A0), mapLo, mapHi, 100, 0); 
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0); 

      lcd.setCursor(4, 0);
      lcd.print(soil1);
      lcd.setCursor(13, 0);
      lcd.print(soil2); 
      h = dht.readHumidity();
      lcd.setCursor(2, 1);
      lcd.print(h);
      lcd.setCursor(6, 1);
      lcd.print("%");
      t = dht.readTemperature();
      lcd.setCursor(11, 1);
      lcd.print(t);
      lcd.setCursor(15, 1);
      lcd.write(3); 

      click = digitalRead(Button);
      if (click == HIGH) {
        click9 = 1; 
      } else {
        click9 = 0; }
      delay(750); 

      soil1 = map(analogRead(A0), mapLo, mapHi, 100, 0); 
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0); 

      lcd.setCursor(4, 0);
      lcd.print(soil1);
      lcd.setCursor(13, 0);
      lcd.print(soil2); 
      h = dht.readHumidity();
      lcd.setCursor(2, 1);
      lcd.print(h);
      lcd.setCursor(6, 1);
      lcd.print("%");
      t = dht.readTemperature();
      lcd.setCursor(11, 1);
      lcd.print(t);
      lcd.setCursor(15, 1);
      lcd.write(3); 

      click = digitalRead(Button);
      if (click == HIGH) {
        click10 = 1; 
      } else {
        click10 = 0; }
      delay(750); 

      soil1 = map(analogRead(A0), mapLo, mapHi, 100, 0); 
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0); 

      lcd.setCursor(4, 0);
      lcd.print(soil1);
      lcd.setCursor(13, 0);
      lcd.print(soil2); 
      h = dht.readHumidity();
      lcd.setCursor(2, 1);
      lcd.print(h);
      lcd.setCursor(6, 1);
      lcd.print("%");
      t = dht.readTemperature();
      lcd.setCursor(11, 1);
      lcd.print(t);
      lcd.setCursor(15, 1);
      lcd.write(3); 

      click = digitalRead(Button);
      if (click == HIGH) {
        click11 = 1; 
      } else {
        click11 = 0; }
      delay(750); 

      soil1 = map(analogRead(A0), mapLo, mapHi, 100, 0); 
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0); 

      lcd.setCursor(4, 0);
      lcd.print(soil1);
      lcd.setCursor(13, 0);
      lcd.print(soil2); 
      h = dht.readHumidity();
      lcd.setCursor(2, 1);
      lcd.print(h);
      lcd.setCursor(6, 1);
      lcd.print("%");
      t = dht.readTemperature();
      lcd.setCursor(11, 1);
      lcd.print(t);
      lcd.setCursor(15, 1);
      lcd.write(3); 

      click = digitalRead(Button);
      if (click == HIGH) {
        click12 = 1; 
      } else {
        click12 = 0; }
      delay(750); 

      int totalClick = (click1 + click2 + click3 + click4 + click5 + click6 + click7 + click8 + click9 + click10 + click11 + click12);
      tone(buzzer, 1000);
      delay (100); 
      noTone(buzzer); 

    if ((totalClick > clicks) && (averageSoil <= safetyLim)) {

    // Start watering

      tone(buzzer, 1000);
      delay (200); 
      noTone(buzzer); 
      digitalWrite(LED_Red,   LOW);
      digitalWrite(LED_Blue,  LOW);
      digitalWrite(LED_Green, LOW);

      lcd.clear();
      Serial.println(F("Watering now!"));      // 26
      
      lcd.setCursor(1, 0);
      lcd.print("Watering now!");
      lcd.setCursor(1, 1);
      lcd.print("Soil:");
      lcd.setCursor(7, 1);
      lcd.print(soil);
      lcd.setCursor(9, 1);
      lcd.print("%");
    
      lcd.setCursor(11, 1);
      lcd.write(1);  //arrow
      lcd.setCursor(12, 1);
      lcd.print("20");

      Serial.print(F("-Soil "));
      Serial.print(soil);
      Serial.println(F("%  - 20"));
  
      digitalWrite(Pump,      HIGH);
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,  HIGH);
      delay(1000);

      digitalWrite(LED_Green, LOW);
      digitalWrite(LED_Blue,  LOW);
      delay(1000);      // 500
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      soil1 = map(analogRead(soil1), mapLo, mapHi, 100, 0);    // 19
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0);    
      soil  = ((soil1 + soil2)/2);

      lcd.setCursor(7, 1);
      Serial.print(F("-Soil "));
      Serial.print(soil);
      lcd.print(soil);
      Serial.println(F("%  - 19"));
      
      lcd.setCursor(11, 1);
      lcd.print(" ");  //empty
      lcd.setCursor(12, 1);
      lcd.print("19");
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,  HIGH);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      digitalWrite(LED_Green, LOW);
      digitalWrite(LED_Blue,  LOW);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      soil1 = map(analogRead(soil1), mapLo, mapHi, 100, 0);    // 18
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0);    
      soil  = ((soil1 + soil2)/2);

      lcd.setCursor(7, 1);
      Serial.print(F("-Soil "));
      Serial.print(soil);
      lcd.print(soil);
      Serial.println(F("%  - 18"));
      
      lcd.setCursor(11, 1);
      lcd.print(" ");  //empty
      lcd.setCursor(12, 1);
      lcd.print("18");
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,  HIGH);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      digitalWrite(LED_Green, LOW);
      digitalWrite(LED_Blue,  LOW);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      soil1 = map(analogRead(soil1), mapLo, mapHi, 100, 0);    // 17
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0);    
      soil  = ((soil1 + soil2)/2);

      lcd.setCursor(7, 1);
      Serial.print(F("-Soil "));
      Serial.print(soil);
      lcd.print(soil);
      Serial.println(F("%  - 17"));
      
      lcd.setCursor(11, 1);
      lcd.print(" ");  //empty
      lcd.setCursor(12, 1);
      lcd.print("17");
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,  HIGH);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      digitalWrite(LED_Green, LOW);
      digitalWrite(LED_Blue,  LOW);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      soil1 = map(analogRead(soil1), mapLo, mapHi, 100, 0);    // 16
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0);    
      soil  = ((soil1 + soil2)/2);

      lcd.setCursor(7, 1);
      Serial.print(F("-Soil "));
      Serial.print(soil);
      lcd.print(soil);
      Serial.println(F("%  - 16"));
      
      lcd.setCursor(11, 1);
      lcd.print(" ");  //empty
      lcd.setCursor(12, 1);
      lcd.print("16");
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,  HIGH);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      digitalWrite(LED_Green, LOW);
      digitalWrite(LED_Blue,  LOW);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      soil1 = map(analogRead(soil1), mapLo, mapHi, 100, 0);    // 15
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0);    
      soil  = ((soil1 + soil2)/2);

      lcd.setCursor(7, 1);
      Serial.print(F("-Soil "));
      Serial.print(soil);
      lcd.print(soil);
      Serial.println(F("%  - 15"));
      
      lcd.setCursor(11, 1);
      lcd.print(" ");  //empty
      lcd.setCursor(12, 1);
      lcd.print("15");
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,  HIGH);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      digitalWrite(LED_Green, LOW);
      digitalWrite(LED_Blue,  LOW);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      soil1 = map(analogRead(soil1), mapLo, mapHi, 100, 0);    // 14
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0);    
      soil  = ((soil1 + soil2)/2);

      lcd.setCursor(7, 1);
      Serial.print(F("-Soil "));
      Serial.print(soil);
      lcd.print(soil);
      Serial.println(F("%  - 14"));
      
      lcd.setCursor(11, 1);
      lcd.print(" ");  //empty
      lcd.setCursor(12, 1);
      lcd.print("14");
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,  HIGH);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      digitalWrite(LED_Green, LOW);
      digitalWrite(LED_Blue,  LOW);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      soil1 = map(analogRead(soil1), mapLo, mapHi, 100, 0);    // 13
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0);    
      soil  = ((soil1 + soil2)/2);

      lcd.setCursor(7, 1);
      Serial.print(F("-Soil "));
      Serial.print(soil);
      lcd.print(soil);
      Serial.println(F("%  - 13"));
      
      lcd.setCursor(11, 1);
      lcd.print(" ");  //empty
      lcd.setCursor(12, 1);
      lcd.print("13");
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,  HIGH);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      digitalWrite(LED_Green, LOW);
      digitalWrite(LED_Blue,  LOW);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      soil1 = map(analogRead(soil1), mapLo, mapHi, 100, 0);    // 12
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0);    
      soil  = ((soil1 + soil2)/2);

      lcd.setCursor(7, 1);
      Serial.print(F("-Soil "));
      Serial.print(soil);
      lcd.print(soil);
      Serial.println(F("%  - 12"));
      
      lcd.setCursor(11, 1);
      lcd.print(" ");  //empty
      lcd.setCursor(12, 1);
      lcd.print("12");
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,  HIGH);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      digitalWrite(LED_Green, LOW);
      digitalWrite(LED_Blue,  LOW);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      soil1 = map(analogRead(soil1), mapLo, mapHi, 100, 0);    // 11
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0);    
      soil  = ((soil1 + soil2)/2);

      lcd.setCursor(7, 1);
      Serial.print(F("-Soil "));
      Serial.print(soil);
      lcd.print(soil);
      Serial.println(F("%  - 11"));
      
      lcd.setCursor(11, 1);
      lcd.print(" ");  //empty
      lcd.setCursor(12, 1);
      lcd.print("11");
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,  HIGH);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      digitalWrite(LED_Green, LOW);
      digitalWrite(LED_Blue,  LOW);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      soil1 = map(analogRead(soil1), mapLo, mapHi, 100, 0);    // 10
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0);    
      soil  = ((soil1 + soil2)/2);

      lcd.setCursor(7, 1);
      Serial.print(F("-Soil "));
      Serial.print(soil);
      lcd.print(soil);
      Serial.println(F("%  - 10"));
      
      lcd.setCursor(11, 1);
      lcd.print(" ");  //empty
      lcd.setCursor(12, 1);
      lcd.print("10");
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,  HIGH);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      digitalWrite(LED_Green, LOW);
      digitalWrite(LED_Blue,  LOW);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }      

      soil1 = map(analogRead(soil1), mapLo, mapHi, 100, 0);    // 9
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0);    
      soil  = ((soil1 + soil2)/2);

      lcd.setCursor(7, 1);
      Serial.print(F("-Soil "));
      Serial.print(soil);
      lcd.print(soil);
      Serial.println(F("%  - 9"));
      
      lcd.setCursor(11, 1);
      lcd.print("  ");  //empty
      lcd.setCursor(13, 1);
      lcd.print("8");
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,  HIGH);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      digitalWrite(LED_Green, LOW);
      digitalWrite(LED_Blue,  LOW);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      soil1 = map(analogRead(soil1), mapLo, mapHi, 100, 0);    // 8
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0);    
      soil  = ((soil1 + soil2)/2);

      lcd.setCursor(7, 1);
      Serial.print(F("-Soil "));
      Serial.print(soil);
      lcd.print(soil);
      Serial.println(F("%  - 8"));
      
      lcd.setCursor(11, 1);
      lcd.print(" ");  //empty
      lcd.setCursor(13, 1);
      lcd.print("8");
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,  HIGH);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      digitalWrite(LED_Green, LOW);
      digitalWrite(LED_Blue,  LOW);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      soil1 = map(analogRead(soil1), mapLo, mapHi, 100, 0);    // 7
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0);    
      soil  = ((soil1 + soil2)/2);

      lcd.setCursor(7, 1);
      Serial.print(F("-Soil "));
      Serial.print(soil);
      lcd.print(soil);
      Serial.println(F("%  - 7"));
      
      lcd.setCursor(11, 1);
      lcd.write(1);  //arrow
      lcd.setCursor(13, 1);
      lcd.print("7");
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,  HIGH);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      digitalWrite(LED_Green, LOW);
      digitalWrite(LED_Blue,  LOW);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      soil1 = map(analogRead(soil1), mapLo, mapHi, 100, 0);    // 6
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0);    
      soil  = ((soil1 + soil2)/2);

      lcd.setCursor(7, 1);
      Serial.print(F("-Soil "));
      Serial.print(soil);
      lcd.print(soil);
      Serial.println(F("%  - 6"));
      
      lcd.setCursor(11, 1);
      lcd.print(" ");  //empty
      lcd.setCursor(13, 1);
      lcd.print("6");
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,  HIGH);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      digitalWrite(LED_Green, LOW);
      digitalWrite(LED_Blue,  LOW);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      soil1 = map(analogRead(soil1), mapLo, mapHi, 100, 0);    // 5
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0);    
      soil  = ((soil1 + soil2)/2);

      lcd.setCursor(7, 1);
      Serial.print(F("-Soil "));
      Serial.print(soil);
      lcd.print(soil);
      Serial.println(F("%  - 5"));
      
      lcd.setCursor(11, 1);
      lcd.write(1);  //arrow
      lcd.setCursor(13, 1);
      lcd.print("5");
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,  HIGH);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      digitalWrite(LED_Green, LOW);
      digitalWrite(LED_Blue,  LOW);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      soil1 = map(analogRead(soil1), mapLo, mapHi, 100, 0);    // 4
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0);    
      soil  = ((soil1 + soil2)/2);

      lcd.setCursor(7, 1);
      Serial.print(F("-Soil "));
      Serial.print(soil);
      lcd.print(soil);
      Serial.println(F("%  - 5"));
      
      lcd.setCursor(11, 1);
      lcd.print(" ");  //empty
      lcd.setCursor(13, 1);
      lcd.print("4");
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,  HIGH);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      digitalWrite(LED_Green, LOW);
      digitalWrite(LED_Blue,  LOW);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      soil1 = map(analogRead(soil1), mapLo, mapHi, 100, 0);    // 3
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0);    
      soil  = ((soil1 + soil2)/2);

      lcd.setCursor(7, 1);
      Serial.print(F("-Soil "));
      Serial.print(soil);
      lcd.print(soil);
      Serial.println(F("%  - 3"));

      lcd.setCursor(11, 1);
      lcd.write(1);  //arrow
      lcd.setCursor(13, 1);
      lcd.print("3");
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,  HIGH);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      digitalWrite(LED_Green, LOW);
      digitalWrite(LED_Blue,  LOW);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      soil1 = map(analogRead(soil1), mapLo, mapHi, 100, 0);    // 2
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0);    
      soil  = ((soil1 + soil2)/2);

      lcd.setCursor(7, 1);
      Serial.print(F("-Soil "));
      Serial.print(soil);
      lcd.print(soil);
      Serial.println(F("%  - 2"));

      lcd.setCursor(11, 1);
      lcd.print(" ");  //empty
      lcd.setCursor(13, 1);
      lcd.print("2");
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,  HIGH);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      digitalWrite(LED_Green, LOW);
      digitalWrite(LED_Blue,  LOW);
      delay(500);
      if (digitalRead(Button) == HIGH) {
      digitalWrite(Pump, LOW); return; }

      soil1 = map(analogRead(soil1), mapLo, mapHi, 100, 0);    // 1
      soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0);    
      soil  = ((soil1 + soil2)/2);

      lcd.setCursor(7, 1);
      Serial.print(F("-Soil "));
      Serial.print(soil);
      lcd.print(soil);
      Serial.println(F("%  - 1"));

      lcd.setCursor(11, 1);
      lcd.write(1);  //arrow
      lcd.setCursor(13, 1);
      lcd.print("1");
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,  HIGH);
      delay(500);
      digitalWrite(Pump, LOW);
      digitalWrite(LED_Green, LOW);
      digitalWrite(LED_Blue,  LOW);
      delay(500);
      lcd.clear();
    }
  }
    
//...................Serial Input........................................................

  // ...Timer edit...

  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim(); // Trim whitespace and newline characters

    if (command.startsWith("timer ")) 
    {
      String numberString = command.substring(6); // Extract number part after "set seconds "
      int newSeconds = numberString.toInt(); // Convert to integer

      if (newSeconds > 0) 
      {
        seconds = newSeconds;
        timer = interval * seconds; // Reset timer with new seconds value
        Serial.print("Seconds set to: ");
        Serial.println(seconds);
      } 
      else 
      {
        Serial.println("Invalid seconds value.");
      }
    } 

    // ...Trigger edit... 

    if (command.startsWith("trigger ")) 
    {
      String numberString = command.substring(8); // Extract number part after "set trigger "
      int newTrigger = numberString.toInt(); // Convert to integer

      if (newTrigger > 0) 
      {
        triggerStart = newTrigger;
        Serial.print(F("Trigger value updated. Watering will start below "));
        Serial.print(triggerStart);
        Serial.println(F("%"));
      } 
      else 
      {
        Serial.println(F("Invalid seconds value."));
      }
    } 
    
    // ...Manual Pump trigger...

    else if (command == "pump on") 
    {
      digitalWrite(Pump, HIGH);
      Serial.println("Pump is ON");
    } 

    else if (command == "pump off") 
    {
      digitalWrite(Pump, LOW);
      Serial.println("Pump is OFF");
    } 
  }

//...................Pump triggers.......................................................
  
  if ((soil1 < (triggerStart + 5)) && (soil2 < (triggerStart + 10)) && (averageSoil < triggerStart) && (averageSoil > lowerLim) && (avgDeltaSoil < 0.01))  
  {
    digitalWrite(Pump, HIGH);  
    delay(50); 
  }

  // Stops pump if
  if ((averageSoil < lowerLim) || (averageSoil > triggerStop) || ((soil1 > triggerStop) & (soil2 > triggerStop)))           
  {
    digitalWrite(Pump, LOW);
  }
    
  // If pump is ON for more than x seconds, pump turns OFF 

  if (digitalRead(Pump) == HIGH) 
  {  
     if (timeElapsed > timer) 
     {
      digitalWrite(Pump, LOW); 
      return; 
     }
  }
  else { timeElapsed = 0; }

//...................LED.................................................................

  if (averageTemp < tempLo)                                 // If temp is below 22 degrees - blue blink (2Hz)
    {                                              
    digitalWrite(LED_Green,  LOW);
    digitalWrite(LED_Blue,  (millis() / 500) % 2); 
    digitalWrite(LED_Red,    LOW);     
    }
  else if (averageTemp >= tempHi)                           // If temp is above 29 degrees - red/yellow blink (2Hz)
    {
    digitalWrite(LED_Green, (millis() / 500) % 2);  
    digitalWrite(LED_Blue,   LOW);    
    digitalWrite(LED_Red,   HIGH);     
    } 
  else {                                              // Temp is good. LED indicates soil % or if water pump is ON
  
    if ((averageSoil < soilRed) && !(avgDeltaSoil > 0.01))            //   Red warning blink [4Hz]
      {
        if (digitalRead(Pump) == HIGH)
        {
          digitalWrite(LED_Green, (millis() / 500) % 2);
          digitalWrite(LED_Blue,  (millis() / 500) % 2);
          digitalWrite(LED_Red,    LOW);
        }
        else {
          digitalWrite(LED_Green,  LOW);
          digitalWrite(LED_Blue,   LOW);
          digitalWrite(LED_Red,   (millis() / 250) % 2);
        }
      }

    else if ((averageSoil < soilYellow) && !(avgDeltaSoil > 0.01))    //  Red
      {
        if (digitalRead(Pump) == HIGH)
        {
          digitalWrite(LED_Green, (millis() / 500) % 2);
          digitalWrite(LED_Blue,  (millis() / 500) % 2);
          digitalWrite(LED_Red,    LOW);
        }
        else {
          digitalWrite(LED_Green,  LOW);
          digitalWrite(LED_Blue,   LOW);
          digitalWrite(LED_Red,   HIGH);
        }
      }

    else if ((averageSoil < soilGreen) && !(avgDeltaSoil > 0.01))     //  Orange
      {
        if (digitalRead(Pump) == HIGH)
        {
          digitalWrite(LED_Green, (millis() / 500) % 2);
          digitalWrite(LED_Blue,  (millis() / 500) % 2);
          digitalWrite(LED_Red,    LOW);
        }
        else {
          digitalWrite(LED_Green, HIGH);
          digitalWrite(LED_Blue,   LOW);
          digitalWrite(LED_Red,   HIGH);
        }
      }

    else if ((averageSoil < 100) && !(avgDeltaSoil > 0.01))  // Green
      {
        if (digitalRead(Pump) == HIGH)
        {
          digitalWrite(LED_Green, (millis() / 500) % 2);
          digitalWrite(LED_Blue,  (millis() / 500) % 2);
          digitalWrite(LED_Red,    LOW);
        }
        else {
          digitalWrite(LED_Green, HIGH);
          digitalWrite(LED_Blue,   LOW);
          digitalWrite(LED_Red,    LOW);
        }
      }
    else                                               // Something's fucked!
      {
      digitalWrite(LED_Green,    LOW);
      digitalWrite(LED_Blue,     LOW);
      digitalWrite(LED_Red,      LOW);
      } 
    }

//...................DHT Sensor Error....................................................
  
  if (isnan(h) || isnan(t)) 
  {
    Serial.println(F(" - DHT Sensor Error!"));

    if (avgDeltaSoil > 0.01)       // Soil Sensor Error
    {
      Serial.print(F(" -Soil~ "));
      Serial.print(averageSoil);
      Serial.print(F("%! "));
      Serial.print(F(" -dx/dt: "));
      Serial.print(avgDeltaSoil);
      Serial.print(F("  -A0_Raw: "));
      Serial.print(analogRead(A0));
      
      lcd.setCursor(0, 0); 
      lcd.print("No sensors found");
      lcd.setCursor(0, 1);
      lcd.print(" *Manual Mode*  ");

      digitalWrite(LED_Green,    LOW);
      digitalWrite(LED_Blue,     (millis() / 2000) % 2);
      digitalWrite(LED_Red,      (millis() / 2000) % 2);
    } 
    else                                                   // Soil Sensor Working
    { 
      Serial.print(F("  Soil: "));
      Serial.print(soil); 
      Serial.print(F("%,  Avg: "));
      Serial.print(averageSoil);
      Serial.print(F("%,  dx/dt: "));
      Serial.print(avgDeltaSoil);       
      Serial.print(F(",  A0_Raw: "));  // Raw sensor data
      Serial.print(analogRead(A0));
      Serial.print(F(",  A2_Raw: "));  // Raw sensor data
      Serial.print(analogRead(A2));
      //
      Serial.print(F("  -Pump: "));

      if (digitalRead(Pump) == HIGH) {
      Serial.print(F("ON"));}
      else {
      Serial.print(F("OFF")); }
      Serial.print(F(",  Elapsed time: "));  // Raw sensor data
      Serial.print(timeElapsed);
      //
      lcd.setCursor(0, 1);
      lcd.print("DHT Sensor Error");
      lcd.setCursor(0, 0);
      lcd.print("    Soil:");
      lcd.setCursor(9, 0);    
      lcd.print(averageSoil); 
      if (averageSoil < 10) { 
        lcd.setCursor(10, 0);
        lcd.print("%     ");     
       } 
      else {
        lcd.setCursor(11, 0);
        lcd.print("%    ");  
      }   
    }
    delay(25);
  return;
  }

//...................Print sensor data to LCD............................................
  
  if (digitalRead(Pump) == HIGH) 
  {
    lcd.clear(); 
    lcd.setCursor(0, 0);  // 
    lcd.print("Watering now!");
    lcd.setCursor(14, 0);
    lcd.print(seconds - (timeElapsed/1000));

    lcd.setCursor(0, 1);  // LCD 2nd row L - Soil
    lcd.print("Soil:");
    lcd.setCursor(5, 1);
    lcd.print(averageSoil); 
    lcd.setCursor(7, 1);  
    lcd.print("% ");
    
    lcd.setCursor(9, 1);  // LCD 2nd row R - Temperature
    lcd.print("Raw:");
    lcd.setCursor(13, 1);
    lcd.print(soil);
    lcd.setCursor(15, 1);
    lcd.print("%");   
  }

  else {
    lcd.setCursor(0, 0);  // LCD 1st row L - Relative Humidity 
    lcd.print("HI:");
    lcd.setCursor(3, 0);
    lcd.print(averageHI);
    lcd.setCursor(7, 0);
    lcd.write(3);

    lcd.setCursor(8, 0);  // LCD 1st row R - Humidity 
    lcd.print(" H:"); 
    lcd.setCursor(11, 0);
    lcd.print(averageHumid);
    lcd.setCursor(15, 0);
    lcd.print("%");
    
    lcd.setCursor(0, 1);  // LCD 2nd row L - Soil
    lcd.print("Soil:");
    lcd.setCursor(5, 1);
    lcd.print(averageSoil); 
    lcd.setCursor(7, 1);  
    lcd.print("% ");
    
    lcd.setCursor(9, 1);  // LCD 2nd row R - Temperature
    lcd.print("T:");
    lcd.setCursor(11, 1);
    lcd.print(averageTemp);
    lcd.setCursor(15, 1);
    lcd.write(3);    
  }

//...................Print sensor data to PC.............................................

  Serial.print(F(" T: "));         
  Serial.print(t);                        // averageTemp      
  Serial.print(F("°C, "));

  Serial.print(F(" H: "));     
  Serial.print(h);                        // averageHumid
  Serial.print(F("%, -"));

  Serial.print(F(" Avg: "));
  Serial.print(averageSoil);              // Average soil sensor level
  Serial.print(F("%, "));

  Serial.print(F(" dx/dt: "));            // Average rate of change of soil sensor
  Serial.print(avgDeltaSoil);

  Serial.print(F("  Soil 1: "));
  Serial.print(soil1);                    // Real-time soil scaled 1
  Serial.print(F("%, "));
  Serial.print(F("  Soil 2: "));
  Serial.print(soil2);                    // Real-time soil scaled 2
  Serial.print(F("%, "));

  Serial.print(F(",  A0_Raw: "));         // Raw soil sensor data 1
  Serial.print(analogRead(A0));
  Serial.print(F(",  A2_Raw: "));         // Raw soil sensor data 2
  Serial.print(analogRead(A2));

  Serial.print(F(",  Trigger value: "));            // Average rate of change of soil sensor
  Serial.print(triggerStart);

  Serial.print(F(",  Timer: "));            // Average rate of change of soil sensor
  Serial.print(seconds);

  Serial.print(F(", - Elapsed time: "));  // Cut-off counter for pump 
  Serial.println(timeElapsed);

//...................Debug...............................................................

  /* 
  
  Serial.print(" -HI= ");   
  Serial.print(averageHI);
  Serial.print("°C ");

  Serial.print(F(" -dx/dt: "));
  Serial.println(dxdtSoil);

  Serial.print(F(" -Average dx/dt: "));
  Serial.print(averageDxdt);
  
  Serial.print(F(" -average: "));
  Serial.print(averageSoil);
  
  Serial.print(F("  -Pump: "));

  if (digitalRead(Pump) == HIGH) {
    Serial.println(F("ON"));}
  else {
    Serial.println(F("OFF")); }
  
  Serial.print(F(" -Btn "));   // Button input 
  Serial.print(click);

  */
  
//...................Misc................................................................
}

void watering() 
{

}

void button()
{

}

void manualWatering() 
{

}