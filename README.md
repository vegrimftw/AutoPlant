# AutoPlant
Takes care of plants or whatever for people who can't maintain one by themselves (like me).  


  Everything inside "Adjustable Variables" can be changed as pleased.
  Especially triggerStart as that will vary depending on the plant. 

  1. Install libraries (DHT11/22, LCD and elapsedMillis). 

  2. Calibration is required and should be started by commenting out the pump trigger IF statement (line 485-488) 
     This will avoid accidental triggering of the pump. Or just not have the pump connected through the relay yet. 

  3. mapHi and mapLo is the sensor calibration borders. The analog input range is 0-1023, but the soil sensor
     does not use the entire range (approx 510-180). It should be good, but check and make changes if needed. 
  
  4. -The "seconds" variable is the failsafe timer. Default is 45s, but should be changed based on pump effect.
     -The "wateringDuration variable is how long the pump should run if manually activated. Change it to own preference. 

  5. The serial input section of the code is for writing instructions to the Arduino while it's running.* 
     To change the:

     * timer value,   write this: timer xy
     
     * trigger value, write this: trigger xy

     ..then press ENTER button to send the command. For example: "trigger 45"

     The pump can also be started the same way, simply write this:  
     * "pump on" or "pump off"

     Serial input only works if the Arduino is battery powered or powered by something else than USB. 

Inputs/Outputs is entirely up to you, this is just how I ended up doing this. I encrourage you to clean up the io's and code a bit. 
I used two sensors as just one proved to be somewhat unreliable at times. 

I've used this project for learning and have moved on to new projects, so this will rarely be updated. 
