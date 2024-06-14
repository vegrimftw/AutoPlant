#include <DHT.h>
#include <DHT_U.h>
#include <LiquidCrystal.h>
#include <elapsedMillis.h>

#define DHTPIN 6        // Temperature & Humidity sensor on digital input D6
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//..................User Adjustable Variables..........................................

  // Pump triggers
  int triggerStart     = 40;              // This % will trigger the pump 
  int triggerStop      = 90;              // Pump will stop at this % 
  
  // Temperature warning LED 
  int tempHi           = 28;              // Yellow/red blink - Too warm
  int tempLo           = 22;              // Blue blink - Too cold 

  // Misc
  int hold             = 4;               // Long press (seconds) 
 
  // Soil scaling                                             
  int mapHi            = 500;             // Upper limit raw (dry ~ 510+) [1023]  
  int mapLo            = 180;             // Lower limit raw (wet ~ 180-) [0]    

  // Time(r) 
  int seconds          = 30;              // Pump stops after X seconds  
  int manual           = 15;              // Timer for manual start of pump 

//...................Inputs/Outputs & Variable...........................................

  elapsedMillis timeElapsed;
  unsigned int interval = 1000;    // Timer variable, don't touch

  // Input/Output pins

  const int rs = 3, en = 2, d4 = 4, d5 = 5, d6 = 6, d7 = 7;  // LCD 
  const int pump          = 8;
  //        DHT Sensor    = 9;
  const int button        = 10;      
  const int LED_Red       = 14;    // Arduino Uno: 11
  const int LED_Green     = 15;    // Arduino Uno: 12
  const int LED_Blue      = 16;    // Arduino Uno: 13
  const int soil1         = A0; 
  const int buttonDown    = A1;
  const int soil2         = A2; 
  const int buttonUp      = A3;    
  const int toggleSwitch  = A4;    // UNO
  const int spare         = A5;    // UNO
  
  // Random variables 

  int click = 0;
  int totalClick;
  int clickStatus[12];
  int triggerstart;                // Used for serial input

  int prevSoil;
  int newSoil;

  double dxSoil;
  double deltaSoil;
  double prevSoilRaw;
  double newSoilRaw;
  double previousTime;
  double newTime;


//...................Average for-loops...................................................

  // numReadings is the amount of sensor data readings being averaged out

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


//...............SETUP.............................................................................

void setup() {

  //...................Pin Mode..........................................................

    Serial.begin(9600);   // This starts the serial port communication 

    // Decleration of input/output 

    pinMode(8,  OUTPUT);  // Pump
    pinMode(9,   INPUT);  // DHT Sensor
    pinMode(10,  INPUT);  // Button
    pinMode(14, OUTPUT);  // LED Red
    pinMode(15, OUTPUT);  // LED Green
    pinMode(16, OUTPUT);  // LED Blue
    pinMode(A0,  INPUT);  // Soil Sensor
    pinMode(A1, OUTPUT);  // Button Down
    pinMode(A2,  INPUT);  // Soil sensor 2
    pinMode(A3, OUTPUT);  // Button Up

    digitalWrite(pump, LOW);  // Just because 

    lcd.createChar(1, celcius);
    lcd.createChar(2, smiley);
    lcd.begin(16, 2);


  //...................Startup procedure.................................................

    Serial.println(F(" AutoPlant - For forgetful/lazy people"));
    Serial.println(F(" Initializing..."));
    lcd.print(" AutoWater for  ");
    lcd.setCursor(0, 1);
    lcd.print("Forgetful people");
    digitalWrite(LED_Red,   HIGH);       // Test each LED color on startup 
    delay(1000);
    digitalWrite(LED_Red,    LOW);
    digitalWrite(LED_Green, HIGH);
    delay(1000);
    digitalWrite(LED_Green,  LOW);
    digitalWrite(LED_Blue,  HIGH);
    delay(1000);
    digitalWrite(LED_Blue,   LOW);
    delay(500);

    if (toggleSwitch == HIGH)            // One soil sensor - One white blink at startup
    {      
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,  HIGH);
      digitalWrite(LED_Red,   HIGH); 
      delay (1000);
      digitalWrite(LED_Green,  LOW);
      digitalWrite(LED_Blue,   LOW);
      digitalWrite(LED_Red,    LOW); 
    } 
    else                                 // Two soil sensors - Two white blinks at startup 
    {
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,  HIGH);
      digitalWrite(LED_Red,   HIGH); 
      delay (500);
      digitalWrite(LED_Green,  LOW);
      digitalWrite(LED_Blue,   LOW);
      digitalWrite(LED_Red,    LOW); 
      delay (500);
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,  HIGH);
      digitalWrite(LED_Red,   HIGH); 
      delay (500);
      digitalWrite(LED_Green,  LOW);
      digitalWrite(LED_Blue,   LOW);
      digitalWrite(LED_Red,    LOW); 
    }
    lcd.clear(); 
    Serial.println(F(" Initialized."));
}

//...............LOOP..............................................................................

void loop()     
{  

delay(100); // limits the code cycle to ~10Hz

//...................More Variables......................................................

  int timer      = interval * seconds;     // Timer for disabling the pump if ON too long

  // Limits 
  int safetyLim  = triggerStart * 1.5;     // Pump cannot be activated above this %
  int lowerLim   = triggerStart - 10;      // Lower limit 

  // Soil LED
  int soilGreen  = triggerStart * 1.45;    // Green above this   
  int soilYellow = triggerStart * 1.25;    // Yellow Above this  
  int soilRed    = triggerStart * 1.1;     // Red above this     

//...................Soil moisture & DHT sensor..........................................

  int soil1_raw = analogRead(A1);                     // Reads the analog value
  int soil1 = map(soil1_raw, mapLo, mapHi, 100, 0);   // Converts it to percentage 

  int soil2_raw = analogRead(A2);
  int soil2 = map(soil2_raw, mapLo, mapHi, 100, 0); 

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float hic = dht.computeHeatIndex(t, h);

//...................Math................................................................

  int analogAverage;
  int soil;

  if (toggleSwitch == HIGH)           // One soil sensor
  {      
    analogAverage = analogRead(A1); 
    soil = soil1; 
  } 
  else                                // Two soil sensors
  {
    analogAverage = (analogRead(A1) + analogRead(A2)) / 2; 
    soil = (soil1 + soil2) / 2; 
  }

  // dx/dt    -   This derives the rate of change of input signal(s) to prevent eratic values 

    // Use the analogAverage and soil variables after they are assigned
    prevSoilRaw = newSoilRaw;   // Saves soil level read from previous cycle
    newSoilRaw = analogAverage;

    previousTime = newTime;     // Saves the time of the previous cycle
    newTime = millis();         // Filling up/down(+/-)  /  cycle runtime  -  (millis() / 100)

    float dxSoil = ((newSoilRaw - prevSoilRaw) / (newTime - previousTime));
    float dxdtSoil = (dxSoil * dxSoil);   // Avoids negative numbers

  // Average dx/dt

    // Store the current reading in the array
    dxdtReadings[dxdtIndex] = dxdtSoil;
    dxdtIndex = (dxdtIndex + 1) % numReadings5; // Move to the next index, cycling through the array

    // Calculate the total of all readings
    totalDxdt = 0;
    for (int i = 0; i < numReadings5; i++) 
    { totalDxdt += dxdtReadings[i]; }

    // Calculate the average Humidity value
    float avgDeltaSoil = totalDxdt / numReadings5;

  // Average Soil value

    // Store the current reading in the array
    soilReadings[soilIndex] = soil;
    soilIndex = (soilIndex + 1) % numReadings1; // Move to the next index, cycling through the array

    // Calculate the total of all readings
    totalSoil = 0;
    for (int i = 0; i < numReadings1; i++) 
    { totalSoil += soilReadings[i]; }

    // Calculate the average soil value
    int averageSoil = totalSoil / numReadings1;

  // Average Temp

    // Store the current reading in the array
    tempReadings[tempIndex] = t;
    tempIndex = (tempIndex + 1) % numReadings2; // Move to the next index, cycling through the array

    // Calculate the total of all readings
    totalTemp = 0;
    for (int i = 0; i < numReadings2; i++) 
    { totalTemp += tempReadings[i]; }

    // Calculate the average Temperature value
    float averageTemp = totalTemp / numReadings2;

  // Average Humidity

    // Store the current reading in the array
    humidReadings[humidIndex] = h;
    humidIndex = (humidIndex + 1) % numReadings3; // Move to the next index, cycling through the array

    // Calculate the total of all readings
    totalHumid = 0;
    for (int i = 0; i < numReadings3; i++) 
    { totalHumid += humidReadings[i]; }

    // Calculate the average Humidity value
    float averageHumid = totalHumid / numReadings3;

  // Average Heat Index

    // Store the current reading in the array
    hiReadings[hiIndex] = hic;
    hiIndex = (hiIndex + 1) % numReadings4; // Move to the next index, cycling through the array

    // Calculate the total of all readings
    totalHi = 0;
    for (int i = 0; i < numReadings4; i++) 
    { totalHi += hiReadings[i]; }

    // Calculate the average Humidity value
    float averageHI = totalHi / numReadings4;


//...................Button functions....................................................

  click = digitalRead(button);

  if (click == HIGH) {
    displayMessage(" Hold button to ", " start watering ");
    delay(1000);
    lcd.clear();
    
    for (int i = 0; i < 10; i++) {
      readAndDisplayData();
      clickStatus[i] = digitalRead(button) == HIGH ? 1 : 0;
      delay(1000);
    }
    
    totalClick = 0;
    for (int i = 0; i < 12; i++) {
      totalClick += clickStatus[i];
    }
    
    soil = (soil1 + soil2) / 2;
    
    if (totalClick > hold && soil <= safetyLim) {
      startWatering();
    }
  }

  int up   = digitalRead(buttonUp);
  int down = digitalRead(buttonDown); 

  if (up == HIGH) {
    triggerStart = triggerStart + 1; 
    lcd.setCursor(0, 0);
    lcd.print("Pump trigger:");
    lcd.setCursor(13, 0);
    lcd.print(triggerStart);
    lcd.setCursor(15, 1);
    lcd.print("%");
    delay (750);   // avoids multiple inputs 
  } 
  if (down == HIGH) {
    triggerStart = triggerStart - 1; 
    lcd.setCursor(0, 0);
    lcd.print("Pump trigger:");
    lcd.setCursor(13, 0);
    lcd.print(triggerStart);
    lcd.setCursor(15, 1);
    lcd.print("%");
    delay (750); 
  }

//...................Serial Input........................................................

  // ...Timer edit...

    if (Serial.available() > 0) {
      String command = Serial.readStringUntil('\n');
      command.trim(); // Trim whitespace and newline characters

      if (command.startsWith("timer ")) {
        String numberString = command.substring(6); // Extract number part after "timer "
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

      if (command.startsWith("trigger ")) {
        String numberString = command.substring(8); // Extract number part after "trigger "
        int newManual = numberString.toInt(); // Convert to integer

        if (newManual > 0) 
        {
          manual = newManual;
          Serial.print(F("Manual pump timer updated. Pump will run for "));
          Serial.print(manual);
          Serial.println(F(" seconds when started."));
        } 
        else 
        {
          Serial.println(F("Invalid seconds value."));
        }
      } 

    // ...Trigger edit... 

      if (command.startsWith("manual ")) {
        String numberString = command.substring(7); // Extract number part after "manual "
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

      else if (command == "pump on") {
        digitalWrite(pump, HIGH);
        Serial.println("Pump is ON");
      } 

      else if (command == "pump off") {
        digitalWrite(pump, LOW);
        Serial.println("Pump is OFF");
      } 
    }

//...................Pump triggers.......................................................

  // Pump will start if both sensors and the average value are below certain value (triggerStart)

  if ((soil1 < (triggerStart + 5)) && (soil2 < (triggerStart + 5)) && (averageSoil <= triggerStart) && (averageSoil > lowerLim) && (avgDeltaSoil < 0.01))  
  {
    digitalWrite(pump, HIGH);  
    delay(50); 
  }

  // Stops pump if the average soil value exceeds trigger stop value OR both sensors exceeds trigger stop value, whatever comes first. 

  if ((averageSoil < lowerLim) || (averageSoil > triggerStop) || ((soil1 > triggerStop) & (soil2 > triggerStop)))           
  {
    digitalWrite(pump, LOW);
  }
    
  // If pump is ON for more than x seconds, pump turns OFF 

  if (digitalRead(pump) == HIGH) 
  {  
     if (timeElapsed > timer)   // Timer starts counting 
     {
      digitalWrite(pump, LOW); 
      return; 
     }
  }
  else { timeElapsed = 0; }     // Resets timer each cycle if pump is off

//...................LED.................................................................

  if (averageTemp < tempLo)                      // If temp is below 22 degrees - blue blink (2Hz)                                              
  {
    digitalWrite(LED_Green,  LOW);
    digitalWrite(LED_Blue,   (millis() / 500) % 2); 
    digitalWrite(LED_Red,    LOW);     
  }

  else if (averageTemp > tempHi)                 // If temp is above 28 degrees - red/yellow blink (2Hz)
  {
    digitalWrite(LED_Green,  (millis() / 500) % 2);  
    digitalWrite(LED_Blue,   LOW);    
    digitalWrite(LED_Red,   HIGH);     
  } 

  else {                                         // Temperature is good. LED indicates soil moisture status
  
    if ((averageSoil > soilGreen) && !(avgDeltaSoil > 0.01))           // Green light
    {
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,   LOW);
      digitalWrite(LED_Red,    LOW); 
    }

    else if ((averageSoil > soilYellow) && !(avgDeltaSoil > 0.01))     //  Yellow light
    {
      digitalWrite(LED_Green, HIGH);
      digitalWrite(LED_Blue,   LOW);
      digitalWrite(LED_Red,   HIGH);
    }

    else if ((averageSoil >= soilRed) && !(avgDeltaSoil > 0.01))       //  Red light
    {
      digitalWrite(LED_Green,  LOW);
      digitalWrite(LED_Blue,   LOW);
      digitalWrite(LED_Red,   HIGH);
    }

    else if ((averageSoil < soilRed) && !(avgDeltaSoil > 0.01))        // Red blink (3Hz)
    {
      digitalWrite(LED_Green,  LOW);
      digitalWrite(LED_Blue,   LOW);
      digitalWrite(LED_Red,    (millis() / 333) % 2);  
    }

    else if ((averageSoil) < 0 || (averageSoil > 100))                 // Sensor scaling is wrong, purple blink (2Hz)
    {
      digitalWrite(LED_Green,    LOW); 
      digitalWrite(LED_Blue,     (millis() / 500) % 2); 
      digitalWrite(LED_Red,      (millis() / 500) % 2); 
    } 
  }

//...................Print sensor data to LCD............................................
  
  if (digitalRead(pump) == HIGH) 
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
    lcd.write(1);

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
    lcd.write(1);    
  }

//...................Print sensor data to Serial Port....................................

  Serial.print(F(" T: "));         
  Serial.print(t);                        // average Temperature      
  Serial.print(F("°C, "));

  Serial.print(F(" H: "));     
  Serial.print(h);                        // average Humidity
  Serial.print(F("%, -"));

  Serial.print(F(" Avg: "));
  Serial.print(averageSoil);              // Average soil sensor level
  Serial.print(F("%, "));

  Serial.print(F(" dx/dt: "));            // Average rate of change of soil sensor
  Serial.print(avgDeltaSoil);

  Serial.print(F("  Soil 1: "));
  Serial.print(soil1);                    // Real-time soil (scaled) 1
  Serial.print(F("%, "));
  Serial.print(F("  Soil 2: "));
  Serial.print(soil2);                    // Real-time soil (scaled) 2
  Serial.print(F("%, "));

  Serial.print(F(",  A0_Raw: "));         // Raw soil sensor data 1
  Serial.print(analogRead(A1));
  Serial.print(F(",  A2_Raw: "));         // Raw soil sensor data 2
  Serial.print(analogRead(A2));

  Serial.print(F(",  Trigger value: "));  // Average rate of change of soil sensor
  Serial.print(triggerStart);

  Serial.print(F(",  Timer: "));          // Average rate of change of soil sensor
  Serial.print(seconds);

  Serial.print(F(", - Elapsed time: "));  // Cut-off counter for pump 
  Serial.println(timeElapsed);            // ln (line) is basically the enter-button

}

//...............FUNCTIONS.........................................................................

void displayMessage(const char* line1, const char* line2) 
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

void readAndDisplayData() 
{
  int soil1 = map(analogRead(A0), mapLo, mapHi, 100, 0);
  int soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0);

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

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  lcd.setCursor(0, 1);
  lcd.print("H:");
  lcd.setCursor(2, 1);
  lcd.print(h);
  lcd.setCursor(6, 1);
  lcd.print("%");

  lcd.setCursor(9, 1);
  lcd.print("T:");
  lcd.setCursor(11, 1);
  lcd.print(t);
  lcd.setCursor(15, 1);
  lcd.write(3);
}

void startWatering() 
{
  digitalWrite(LED_Red,    LOW);
  digitalWrite(LED_Blue,  HIGH);
  digitalWrite(LED_Green, HIGH);
  lcd.clear();
  Serial.println(F("Watering now!"));
  lcd.setCursor(1, 0);
  lcd.print("Watering now!");

  for (int i = 20; i > 9; i--) {
    int soiling = (map(analogRead(A0), mapLo, mapHi, 100, 0) + map(analogRead(A2), mapLo, mapHi, 100, 0)) / 2;
    lcd.setCursor(1, 1);
    lcd.print("Soil:");
    lcd.setCursor(7, 1);
    lcd.print(soiling);
    lcd.setCursor(9, 1);
    lcd.print("%");
    lcd.setCursor(11, 1);
    lcd.print(" ");
    lcd.setCursor(12, 1);
    lcd.print(i);

    Serial.print(F("-Soil "));
    Serial.print(soiling);
    Serial.print(F("%  - "));
    Serial.println(i);

    digitalWrite(pump, HIGH);

    if (digitalRead(button) == HIGH) 
    {
      digitalWrite(pump, LOW);
      return;
    }
  }
  digitalWrite(pump, LOW);
}

