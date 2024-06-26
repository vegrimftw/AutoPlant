#include <DHT.h>
#include <DHT_U.h>
#include <LiquidCrystal.h>
#include <elapsedMillis.h>

#define DHTPIN 9  // Temperature & Humidity sensor on digital input D9
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//..................User Adjustable Variables..........................................

  // Pump triggers
  int triggerStart     = 40;   // This % will trigger the pump
  int triggerStop      = 90;   // Pump will stop at this %

  // Temperature warning LED
  int tempHi           = 28;   // Yellow/red blink - Too warm
  int tempLo           = 22;   // Blue blink - Too cold

  // Soil scaling
  int mapLo            = 180;  // Lower limit raw (wet ~ 180-) [0]
  int mapHi            = 510;  // Upper limit raw (dry ~ 510+) [1023]

  // Time(r)
  int seconds          = 45;   // Pump stops after X seconds
  int longPress        = 3;    // Long press (seconds)
  int wateringDuration = 30;   // Manual watering duration [seconds]

//...................Inputs/Outputs & Variable...........................................

  elapsedMillis timeElapsed;
  unsigned int interval = 1000;  // Timer variable, don't touch

  // Input/Output pins

  const int rs = 3, en = 2, d4 = 4, d5 = 5, d6 = 6, d7 = 7;  // LCD
  const int relay         = 8;
  //        DHT Sensor    = 9;
  const int button        = 10;
  const int LED_Red       = 14;  // Arduino Uno: 11
  const int LED_Green     = 15;  // Arduino Uno: 12
  const int LED_Blue      = 16;  // Arduino Uno: 13
  const int soil1         = A0;
  const int buttonDown    = A1;
  const int soil2         = A2;
  const int buttonUp      = A3;
  const int toggleSwitch  = A4;  // UNO
  const int spare         = A5;  // UNO
  const int on  = HIGH;
  const int off = LOW;

  // Random variables
  int y;
  int click = 0;
  int totalClick;
  int clickStatus[12];
  int triggerstart;  // Used for serial input

  int prevSoil;
  int newSoil;
  double dxSoil;
  double deltaSoil;
  double prevSoilRaw;
  double newSoilRaw;
  double previousTime;
  double newTime;
  double previousTime2;
  double newTime2;
  bool buttonHeldLongEnough = false;

  void redLED(int blinkInterval = -1);
  void greenLED(int blinkInterval = -1);
  void blueLED(int blinkInterval = -1);
  void yellowLED(int blinkInterval = -1);
  void tealLED(int blinkInterval = -1);
  void purpleLED(int blinkInterval = -1);
  void whiteLED(int blinkInterval = -1);

//...................Average calculations................................................

  // numReadings is the amount of sensor data readings being averaged out

  const int numReadings1 = 32;  // Average Soil
  int soilReadings[numReadings1];
  int soilIndex = 0;
  int totalSoil = 0;

  const int numReadings2 = 32;  // Average Temperature
  float tempReadings[numReadings2];
  int tempIndex = 0;
  float totalTemp = 0;

  const int numReadings3 = 32;  // Average Humidity
  float humidReadings[numReadings3];
  int humidIndex = 0;
  float totalHumid = 0;

  const int numReadings4 = 32;  // Average Heat Index
  float hiReadings[numReadings4];
  int hiIndex = 0;
  float totalHi = 0;

  const int numReadings5 = 32;  // Average dx/dt
  float dxdtReadings[numReadings5];
  int dxdtIndex = 0;
  float totalDxdt = 0;

//...................LCD + Symbols.......................................................

  LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

  byte celcius[8] = {
    B01000,
    B10100,
    B01011,
    B00100,
    B00100,
    B00100,
    B00011,
    B00000,
  };

  byte topLeft[8] = {
    B00000,
    B00000,
    B00000,
    B00000,
    B00001,
    B00011,
    B00110,
    B01101,
  };

  byte topRight[8] = {
    B00000,
    B00000,
    B10000,
    B10000,
    B10000,
    B11000,
    B01100,
    B00110,
  };

  byte bottomLeft[8] = {
    B01101,
    B01100,
    B00110,
    B00011,
    B00000,
    B00000,
    B00000,
    B00000,
  };

  byte bottomRight[8] = {
    B10110,
    B00110,
    B01100,
    B11000,
    B00000,
    B00000,
    B00000,
    B00000,
  };


//...............SETUP.............................................................................

void setup() {

  //...................Pin Mode..........................................................

  Serial.begin(9600);  // This starts the serial port communication

  // Decleration of input/output

  pinMode(8, OUTPUT);   // Pump
  pinMode(9, INPUT);    // DHT Sensor
  pinMode(10, INPUT);   // Button
  pinMode(14, OUTPUT);  // LED Red
  pinMode(15, OUTPUT);  // LED Green
  pinMode(16, OUTPUT);  // LED Blue
  pinMode(A0, INPUT);   // Soil Sensor 1
  pinMode(A1, OUTPUT);  // Button Down
  pinMode(A2, INPUT);   // Soil sensor 2
  pinMode(A3, OUTPUT);  // Button Up
  pinMode(A4, INPUT);   // Toggle Button
  pinMode(A5, OUTPUT);  // Spare (Pump 2?)

  pump(off);            // Just because

  lcd.createChar(1, topLeft);
  lcd.createChar(2, topRight);
  lcd.createChar(3, bottomLeft);
  lcd.createChar(4, bottomRight);
  lcd.createChar(5, celcius);
  lcd.begin(16, 2);


  //...................Startup procedure.................................................

  Serial.println(F(" AutoPlant - For forgetful/lazy people"));
  Serial.println(F(" Initializing..."));
  lcd.print(" AutoWater for  ");
  lcd.setCursor(0, 1);
  lcd.print("Forgetful people");
  redLED();  // Test each LED color on startup
  delay(500);
  greenLED();
  delay(500);
  blueLED();
  delay(500);
  noLED();
  delay(250); 

  if (toggleSwitch == HIGH)  // One soil sensor - One white blink at startup
  {
    whiteLED();
    delay(1000);
    noLED();
  } else  // Two soil sensors - Two white blinks at startup
  {
    whiteLED();
    delay(500);
    noLED();
    delay(500);
    whiteLED();
    delay(500);
    noLED();
  }
  lcd.clear();
  Serial.println(F(" Initialized."));
}

//...............LOOP..............................................................................

void loop() {

  delay(100);  // limits the code cycle to ~10Hz

//...................More Variables......................................................

  int timer       = interval * seconds;    // Timer for disabling the pump if ON too long
  int yLoops      = longPress * 4;         // 4x because one for-loop = 250ms 

  // Limits
  int safetyLim   = triggerStart * 1.5;    // Pump cannot be activated above this %
  int lowerLim    = triggerStart - 10;     // Lower limit

  // Soil LED
  int soilGreen   = triggerStart * 1.45;   // Green above this
  int soilYellow  = triggerStart * 1.25;   // Yellow Above this
  int soilRed     = triggerStart * 1.1;    // Red above this

//...................Soil moisture & DHT sensor..........................................

  int soil1_raw = analogRead(A1);                    // Reads the analog value
  int soil1 = map(soil1_raw, mapLo, mapHi, 100, 0);  // Converts it to percentage

  int soil2_raw = analogRead(A2);
  int soil2 = map(soil2_raw, mapLo, mapHi, 100, 0);

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float hic = dht.computeHeatIndex(t, h);

//...................Math................................................................

  int analogAverage;
  int soil;

  if (toggleSwitch == HIGH)  // One soil sensor
  {
    analogAverage = analogRead(A1);
    soil = soil1;
  } else  // Two soil sensors
  {
    analogAverage = (analogRead(A1) + analogRead(A2)) / 2;
    soil = (soil1 + soil2) / 2;
  }

  // dx/dt    -   This derives the rate of change of input signal(s) to detect eratic values

  // Use the analogAverage and soil variables after they are assigned
  prevSoilRaw = newSoilRaw;  // Saves soil level read from previous cycle
  newSoilRaw = analogAverage;

  previousTime = newTime;  // Saves the time of the previous cycle
  newTime = millis();      // Filling up/down(+/-)  /  cycle runtime  -  (millis() / 100)

  float dxSoil = ((newSoilRaw - prevSoilRaw) / (newTime - previousTime));
  float dxdtSoil = (dxSoil * dxSoil);  // Avoids negative numbers

  // Average dx/dt

  // Store the current reading in the array
  dxdtReadings[dxdtIndex] = dxdtSoil;
  dxdtIndex = (dxdtIndex + 1) % numReadings5;  // Move to the next index, cycling through the array

  // Calculate the total of all readings
  totalDxdt = 0;
  for (int i = 0; i < numReadings5; i++) { totalDxdt += dxdtReadings[i]; }

  // Calculate the average Humidity value
  float avgDeltaSoil = totalDxdt / numReadings5;

  // Average Soil value

  // Store the current reading in the array
  soilReadings[soilIndex] = soil;
  soilIndex = (soilIndex + 1) % numReadings1;  // Move to the next index, cycling through the array

  // Calculate the total of all readings
  totalSoil = 0;
  for (int i = 0; i < numReadings1; i++) { totalSoil += soilReadings[i]; }

  // Calculate the average soil value
  int averageSoil = totalSoil / numReadings1;

  // Average Temp

  // Store the current reading in the array
  tempReadings[tempIndex] = t;
  tempIndex = (tempIndex + 1) % numReadings2;  // Move to the next index, cycling through the array

  // Calculate the total of all readings
  totalTemp = 0;
  for (int i = 0; i < numReadings2; i++) { totalTemp += tempReadings[i]; }

  // Calculate the average Temperature value
  float averageTemp = totalTemp / numReadings2;

  // Average Humidity

  // Store the current reading in the array
  humidReadings[humidIndex] = h;
  humidIndex = (humidIndex + 1) % numReadings3;  // Move to the next index, cycling through the array

  // Calculate the total of all readings
  totalHumid = 0;
  for (int i = 0; i < numReadings3; i++) { totalHumid += humidReadings[i]; }

  // Calculate the average Humidity value
  float averageHumid = totalHumid / numReadings3;

  // Average Heat Index

  // Store the current reading in the array
  hiReadings[hiIndex] = hic;
  hiIndex = (hiIndex + 1) % numReadings4;  // Move to the next index, cycling through the array

  // Calculate the total of all readings
  totalHi = 0;
  for (int i = 0; i < numReadings4; i++) { totalHi += hiReadings[i]; }

  // Calculate the average Humidity value
  float averageHI = totalHi / numReadings4;

  // dS/dt - detects changing value, avoids triggering when counting on startup

  int deltaSoil = averageSoil;  // int analogAverage = analogRead(A0); if only one sensor used !

  prevSoil = newSoil;  // Saves soil level read from previous cycle
  newSoil = deltaSoil;

  previousTime2 = newTime2;  // Saves the time of the previous cycle
  newTime2 = millis();       // Filling up/down(+/-)  /  cycle runtime  -  (millis() / 100)

  float dsdt = abs((newSoil - prevSoil) / (newTime2 - previousTime2));

//...................Button functions....................................................

  click = digitalRead(button);

  if (click == HIGH) {
    displayMessage(" Hold button to ", " start watering ");
    buttonHeldLongEnough = false;
    delay(1000);
    lcd.clear();

    // Read and display data while checking if the button is held down
    for (int i = 0; i < (40 - y); i++) {
      readAndDisplayData();

      if (digitalRead(button) == HIGH) {
        lcd.setCursor(7, 0);
        lcd.write(1);
        lcd.setCursor(8, 0);
        lcd.write(2);
        lcd.setCursor(7, 1);
        lcd.write(3);
        lcd.setCursor(8, 1);
        lcd.write(4);
        delay(50);

        for (y = 0; y < yLoops; y++) {
          readAndDisplayData();
          delay(240);

          if (y == (yLoops - 1)) {
            buttonHeldLongEnough = true;
            greenLED();
            delay(250);
            break;
          }

          if (digitalRead(button) == LOW) {
            buttonHeldLongEnough = false;
            lcd.setCursor(7, 0);
            lcd.print("  ");
            lcd.setCursor(7, 1);
            lcd.print("  ");
            redLED();
            delay(250);
            break;
          }
        }
      }
      delay(250);
    }
  }

  if ((buttonHeldLongEnough == true) && (soil <= safetyLim)) {
    startWatering();
  }
  buttonHeldLongEnough = false;  // Reset after watering starts

  //.................Adjust trigger value......................

  int up = digitalRead(buttonUp);
  int down = digitalRead(buttonDown);

  if (up == HIGH) {
    triggerStart = triggerStart + 1; 
    lcd.clear(); 
    lcd.setCursor(0, 0);
    lcd.print("Pump trigger:");
    lcd.setCursor(13, 0);
    lcd.print(triggerStart);
    lcd.setCursor(15, 1);
    lcd.print("%");
    delay (500);   // avoids multiple inputs 
  } 
  if (down == HIGH) {
    triggerStart = triggerStart - 1; 
    lcd.clear(); 
    lcd.setCursor(0, 0);
    lcd.print("Pump trigger:");
    lcd.setCursor(13, 0);
    lcd.print(triggerStart);
    lcd.setCursor(15, 1);
    lcd.print("%");
    delay (500); 
  }

//...................Serial Input........................................................

  // ...Timer edit...

  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();  // Trim whitespace and newline characters

    if (command.startsWith("timer ")) {
      String numberString = command.substring(6);  // Extract number part after "timer "
      int newSeconds = numberString.toInt();       // Convert to integer

      if (newSeconds > 0) {
        seconds = newSeconds;
        timer = interval * seconds;  // Reset timer with new seconds value
        Serial.print("Seconds set to: ");
        Serial.println(seconds);
      } else {
        Serial.println("Invalid seconds value.");
      }
    }

    // ...Trigger edit...

    if (command.startsWith("trigger ")) {
      String numberString = command.substring(8);  // Extract number part after "trigger "
      int newTrigger = numberString.toInt();       // Convert to integer

      if (newTrigger > 0) {
        triggerStart = newTrigger;
        Serial.print(F("Trigger value updated. Watering will start below "));
        Serial.print(triggerStart);
        Serial.println(F("%"));
      } else {
        Serial.println(F("Invalid seconds value."));
      }
    }

    // ...Manual Pump trigger...

    else if (command == "pump on") {
      pump(on); 
      Serial.println("Pump is ON");
    }

    else if (command == "pump off") {
      pump(off); 
      Serial.println("Pump is OFF");
    }
  }

//...................Pump triggers.......................................................

  // Pump will start if both sensors and the average value are below certain value (triggerStart)

  if ((soil1 <= (triggerStart + 5)) && (soil2 <= (triggerStart + 5)) && (averageSoil <= triggerStart) && (averageSoil > lowerLim) && (dsdt < 0.001) && (avgDeltaSoil < 0.01)) {
    pump(on); 
    delay(50);
  }

  // Stops pump if the average soil value exceeds trigger stop value OR both sensors exceeds trigger stop value, whatever comes first.

  if ((averageSoil < lowerLim) || (averageSoil > triggerStop) || ((soil1 > triggerStop) & (soil2 > triggerStop))) {
    pump(off); 
  }

  // If pump is ON for more than x seconds, pump turns OFF

  if (digitalRead(relay) == HIGH) {
    if (timeElapsed > timer)  // Timer starts counting
    {
      pump(off); 
      return;
    }
  } else {
    timeElapsed = 0;
  }  // Resets timer each cycle if pump is off

//...................LED.................................................................

  if (averageTemp < tempLo)  // If temp is below 22 degrees - blue blink (5Hz)
  {
    blueLED(100);
  }

  else if (averageTemp > tempHi)  // If temp is above 28 degrees - red blink (5Hz)
  {
    redLED(100);
  }

  else {  // Temperature is good. LED indicates soil moisture status

    if ((averageSoil > soilGreen) && !(avgDeltaSoil > 0.01))  // Green light
    {
      greenLED();
    }

    else if ((averageSoil > soilYellow) && !(avgDeltaSoil > 0.01))  //  Yellow light
    {
      yellowLED();
    }

    else if ((averageSoil >= soilRed) && !(avgDeltaSoil > 0.01))  //  Red light
    {
      redLED();
    }

    else if ((averageSoil < soilRed) && !(avgDeltaSoil > 0.01))  // Red blink (2Hz)
    {
      redLED(500);
    }

    else if ((averageSoil) < 0 || (averageSoil > 100))  // Calibration error, purple blink (1Hz)
    {
      purpleLED(1000);
    }
  }

//...................Sensor Error........................................................

  if (isnan(h) || isnan(t)) {
    Serial.println(F(" - DHT Sensor Error!"));

    if (avgDeltaSoil > 0.01)  // Soil Sensor Error
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
    } else  // Soil Sensor Working
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

      if (digitalRead(pump) == HIGH) {
        Serial.print(F("ON"));
      } else {
        Serial.print(F("OFF"));
      }
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
      } else {
        lcd.setCursor(11, 0);
        lcd.print("%    ");
      }
    }
    delay(25);
    return;
  }

//...................Print sensor data to LCD............................................

  if (digitalRead(pump) == HIGH) {
    lcd.clear();
    lcd.setCursor(0, 0);  //
    lcd.print("Watering now!");
    lcd.setCursor(14, 0);
    lcd.print(seconds - (timeElapsed / 1000));

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
    lcd.write(5);

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
    lcd.write(5);
  }

//...................Print sensor data to Serial Port....................................

  Serial.print(F(" T: "));
  Serial.print(t);  // average Temperature
  Serial.print(F("°C, "));

  Serial.print(F(" H: "));
  Serial.print(h);  // average Humidity
  Serial.print(F("%, -"));

  Serial.print(F(" Avg: "));
  Serial.print(averageSoil);  // Average soil sensor level
  Serial.print(F("%, "));

  Serial.print(F(" dx/dt: "));  // Average rate of change of soil sensor
  Serial.print(avgDeltaSoil);

  Serial.print(F("  Soil 1: "));
  Serial.print(soil1);  // Real-time soil (scaled) 1
  Serial.print(F("%, "));
  Serial.print(F("  Soil 2: "));
  Serial.print(soil2);  // Real-time soil (scaled) 2
  Serial.print(F("%, "));

  Serial.print(F(",  A0_Raw: "));  // Raw soil sensor data 1
  Serial.print(analogRead(A1));
  Serial.print(F(",  A2_Raw: "));  // Raw soil sensor data 2
  Serial.print(analogRead(A2));

  Serial.print(F(",  Trigger value: "));  // Trigger value [%]
  Serial.print(triggerStart);

  Serial.print(F(",  Timer: "));  // Emergency cut-off timer for pump
  Serial.print(seconds);

  Serial.print(F(", - Elapsed time: "));  // Cut-off counter for pump
  Serial.println(timeElapsed);            // ln = new line (enter)
}

//...............FUNCTIONS.........................................................................

  void pump(int state) {
  if (state == on) {
      digitalWrite(relay, HIGH); } 
    else if (state == off) {
      digitalWrite(relay,  LOW); }
  }

  
  void displayMessage(const char* line1, const char* line2) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
  }

  void readAndDisplayData() {
    if (buttonHeldLongEnough == true) {
      greenLED();
    } else {
      whiteLED();
    }

    int soil1 = map(analogRead(A0), mapLo, mapHi, 100, 0);
    int soil2 = map(analogRead(A2), mapLo, mapHi, 100, 0);
    int soil = (soil1 + soil2) / 2;

    lcd.setCursor(0, 0);
    lcd.print("S1: ");
    lcd.setCursor(4, 0);
    lcd.print(soil1);
    lcd.setCursor(6, 0);
    lcd.print("%");

    lcd.setCursor(9, 0);
    lcd.print("S2: ");
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
    lcd.write(5);
  }

  void startWatering() {
    const int checkInterval = 1000;  // interval to check button and update display in milliseconds
    const unsigned long endTime = millis() + (wateringDuration * 1000);

    tealLED();
    lcd.clear();
    Serial.println(F("Watering now!"));
    lcd.setCursor(1, 0);
    lcd.print("Watering now!");

    while (millis() < endTime) {
      int soilYo = (map(analogRead(A0), mapLo, mapHi, 100, 0) + map(analogRead(A2), mapLo, mapHi, 100, 0)) / 2;

      lcd.setCursor(0, 1);
      lcd.print("Soil:");
      lcd.setCursor(6, 1);
      lcd.print(soilYo);
      lcd.setCursor(8, 1);
      lcd.print("% ");
      lcd.setCursor(10, 1);

      unsigned long remainingTime = (endTime - millis()) / 1000;

      if (remainingTime < 10) {
        lcd.print("  t-");
        lcd.setCursor(14, 1);
      } else {
        lcd.print(" t-");
        lcd.setCursor(13, 1);
      }
      lcd.print(remainingTime);
      lcd.setCursor(15, 1);
      lcd.print("s");

      Serial.print(F("-Soil "));
      Serial.print(soilYo);
      Serial.print(F("%  - "));
      Serial.println(remainingTime);

      pump(on); 

      // Check if the button is pressed to stop watering early
      if (digitalRead(button) == HIGH) {
        pump(off); 
        redLED();
        delay(500);
        return;
      }

      delay(checkInterval);  // wait for checkInterval before updating again
    }

    pump(off); 
  }

//...................LED RGB.............................................................

void redLED(int blinkInterval = -1) {
  if (blinkInterval == -1) {
    digitalWrite(LED_Red, HIGH);
    digitalWrite(LED_Green, LOW);
    digitalWrite(LED_Blue, LOW);
  } else {
    digitalWrite(LED_Red, (millis() / blinkInterval) % 2);
    digitalWrite(LED_Green, LOW);
    digitalWrite(LED_Blue, LOW);
  }
}

void greenLED(int blinkInterval = -1) {
  if (blinkInterval == -1) {
    digitalWrite(LED_Red, LOW);
    digitalWrite(LED_Green, HIGH);
    digitalWrite(LED_Blue, LOW);
  } else {
    digitalWrite(LED_Red, LOW);
    digitalWrite(LED_Green, (millis() / blinkInterval) % 2);
    digitalWrite(LED_Blue, LOW);
  }
}

void blueLED(int blinkInterval = -1) {
  if (blinkInterval == -1) {
    digitalWrite(LED_Red, LOW);
    digitalWrite(LED_Green, LOW);
    digitalWrite(LED_Blue, HIGH);
  } else {
    digitalWrite(LED_Red, LOW);
    digitalWrite(LED_Green, LOW);
    digitalWrite(LED_Blue, (millis() / blinkInterval) % 2);
  }
}

void yellowLED(int blinkInterval = -1) {
  if (blinkInterval == -1) {
    digitalWrite(LED_Red, HIGH);
    digitalWrite(LED_Green, HIGH);
    digitalWrite(LED_Blue, LOW);
  } else {
    digitalWrite(LED_Red, (millis() / blinkInterval) % 2);
    digitalWrite(LED_Green, (millis() / blinkInterval) % 2);
    digitalWrite(LED_Blue, LOW);
  }
}

void tealLED(int blinkInterval = -1) {
  if (blinkInterval == -1) {
    digitalWrite(LED_Red, LOW);
    digitalWrite(LED_Green, HIGH);
    digitalWrite(LED_Blue, HIGH);
  } else {
    digitalWrite(LED_Red, LOW);
    digitalWrite(LED_Green, (millis() / blinkInterval) % 2);
    digitalWrite(LED_Blue, (millis() / blinkInterval) % 2);
  }
}

void purpleLED(int blinkInterval = -1) {
  if (blinkInterval == -1) {
    digitalWrite(LED_Red, HIGH);
    digitalWrite(LED_Green, HIGH);
    digitalWrite(LED_Blue, LOW);
  } else {
    digitalWrite(LED_Red, (millis() / blinkInterval) % 2);
    digitalWrite(LED_Green, LOW);
    digitalWrite(LED_Blue, (millis() / blinkInterval) % 2);
  }
}

void whiteLED(int blinkInterval = -1) {
  if (blinkInterval == -1) {
    digitalWrite(LED_Red, HIGH);
    digitalWrite(LED_Green, HIGH);
    digitalWrite(LED_Blue, HIGH);
  } else {
    digitalWrite(LED_Red, (millis() / blinkInterval) % 2);
    digitalWrite(LED_Green, (millis() / blinkInterval) % 2);
    digitalWrite(LED_Blue, (millis() / blinkInterval) % 2);
  }
}

void noLED() {
  digitalWrite(LED_Red, LOW);
  digitalWrite(LED_Green, LOW);
  digitalWrite(LED_Blue, LOW);
}

