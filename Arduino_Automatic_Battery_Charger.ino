int batteryCapacity = 2500;     //capacity rating of battery in mAh
float resistance = 7.5;     //measured resistance of the power resistor
int cutoffVoltage = 3100;     //maximum battery voltage (in mV) that should not be exceeded
int EmptyVoltage = 2400;     //maximum battery voltage (in mV) that should not be exceeded
int overVoltage = 5000;
long cutoffTime = 46800000;     //maximum charge time of 13 hours that should not be exceeded

int PwmPin = 9;     // Output signal wire connected to digital pin 9
int PwmValue = 255;     //value of PWM output signal 
int PwmValueShadow = 0;     //value of PWM output signal 

int analogPin1 = 0;     //first voltage probe connected to analog pin 1
int analogPin2 = 1;     //second voltage probe connected to analog pin 2
int analogPin3 = 2;     //second voltage probe connected to analog pin 2

float valueProbe1 = 0;     //variable to store the value of analogPin1
float valueProbe2 = 0;     //variable to store the value of analogPin2
float valueProbe3 = 0;     //variable to store the value of analogPin2

float voltageProbe1 = 0;     //calculated voltage at analogPin1
float voltageProbe2 = 0;     //calculated voltage at analogPin2
float voltageProbe3 = 0;     //calculated voltage at analogPin2

float batteryVoltage = 0;     //calculated voltage of battery
float current = 0;     //calculated current through the load (in mA)
float currentError = 0;     //difference between target current and actual current (in mA)
int CurrentLock = 0;
int CurrentLockVal = 0;

float targetCurrent = batteryCapacity / 10;     //target output current (in mA) set at C/10 or 1/10 of the battery capacity per hour
float BulkCurrent = batteryCapacity / 10;     //target output current (in mA) set at C/10 or 1/10 of the battery capacity per hour
float FloatCurrent = batteryCapacity / 40;     //float current (in mA) set at C/40 of the battery capacity per hour
float ChargePer = 0;

int ChargingLed = 7;     // Output signal wire connected to digital pin 9
int CurrentLockLed = 6;  // Output signal wire connected to digital pin 9
int FloatLed = 5;  // Output signal wire connected to digital pin 9
int DoneLed = 4;  // Output signal wire connected to digital pin 9

int pwm_update_delay = 10000;

int ChargeBulk = 0;
int ChargeFloat = 0;
int ChargeDone = 0 ;
int Disconnect = 0 ;

// ##########################################################
// ################### Init #################################
// ##########################################################

void setup()
{
  Serial.begin(9600);     //  setup serial
  pinMode(PwmPin, OUTPUT);     // sets the pin as output
  pinMode(ChargingLed, OUTPUT);     // sets the Led as output
  pinMode(CurrentLockLed, OUTPUT);     // sets the Led as output
  pinMode(DoneLed, OUTPUT);     // sets the Led as output
  pinMode(FloatLed, OUTPUT);     // sets the Led as output
 // reset led status
  digitalWrite(ChargingLed, LOW);   // turn the LED OFF
  digitalWrite(CurrentLockLed, LOW);   // turn the LED OFF
  digitalWrite(DoneLed, LOW);   // turn the LED OFF
  digitalWrite(FloatLed, LOW);   // turn the LED OFF
}

void loop()
{ 

// ##########################################################
// ################### VOLTAGE READ #########################
// ##########################################################

  PwmValueShadow = PwmValue; // save last PWM value
 
  analogWrite(PwmPin, 255);  // put PWM in DC=100% -> no pulses

  // read voltage when pulses is off
  for(int i=0;i<100;i++)
  {
    valueProbe1+=analogRead(analogPin1);  //read the input voltage one
    valueProbe2+=analogRead(analogPin2);  //read the input volrage two 
    valueProbe3+=analogRead(analogPin3);  //read the input voltage two 
    delayMicroseconds(50);              // pauses for 50 microseconds  
  }
  
  valueProbe1=valueProbe1/100; 
  valueProbe2=valueProbe2/100;
  valueProbe3=valueProbe3/100;

  voltageProbe1=0.97*(valueProbe1 * 5100 * 3.127)/1023;
  voltageProbe2=0.97*(valueProbe2 * 5100 * 3.127)/1023;
  voltageProbe3=0.97*(valueProbe3 * 5100 * 3.127)/1023;

  batteryVoltage = voltageProbe3 - voltageProbe1;     //calculate battery voltage

  //Serial.println(voltageProbe1);     
  //Serial.println(voltageProbe2);     
  //Serial.println(voltageProbe3);     

  ChargePer = (batteryVoltage / cutoffVoltage ) * 100 ;
  
  analogWrite(PwmPin, PwmValueShadow);  //Write output value to output pin

  // ##########################################################
  // ################### Charging Stage Control  #####################
  // ##########################################################

   if(batteryVoltage > overVoltage & Disconnect < 1) {    //stop charging if the battery voltage exceeds the safety threshold
      ChargeBulk  = 0;
      ChargeFloat = 0;
      ChargeDone  = 1;   
      Disconnect  = 1;
      Serial.println("Over voltage detected ..."); 
      Serial.println("Disconnecting....");
  }
  
   if(batteryVoltage < EmptyVoltage )     //start charging if the battery voltage exceeds the empty threshold
    {
      ChargeBulk  = 1;
      ChargeFloat = 0;
      ChargeDone  = 0;
      targetCurrent = BulkCurrent;
      Serial.println("Battery is Empty ...");
      Serial.println("Start Bulk stage....");
    }

   if(batteryVoltage < cutoffVoltage & Disconnect == 1)     //stop charging if the battery voltage exceeds the safety threshold
   {
      ChargeBulk  = 1;
      ChargeFloat = 0;
      ChargeDone  = 0;
      Disconnect  = 0;
      targetCurrent = BulkCurrent;
      digitalWrite(DoneLed, LOW);   // turn the Charge LED OFF
      Serial.println("Battery Connected ...");
      Serial.println("Start Bulk stage....");
   }

   if(batteryVoltage > cutoffVoltage & ChargeFloat < 1 & Disconnect < 1)     //stop charging if the battery voltage exceeds the safety threshold
   {
    ChargeBulk  = 0;
    ChargeFloat = 1;    
    ChargeDone  = 0;
    targetCurrent = FloatCurrent;
    CurrentLock = 0;
    Serial.println("Max Voltage Exceeded ...");
    Serial.println("Start Floating stage....");
  }  


   if(millis() > cutoffTime)     //stop charging if the charge time threshold
   {
    PwmValue = 255;
    ChargeBulk  = 0;
    ChargeFloat = 0;    
    ChargeDone  = 1;
    Serial.println("Max Charge Time Exceeded");
    Serial.println("Stop Charging....");
   }  
  
// ##########################################################
// ################### Current READ #########################
// ##########################################################

  // read voltage when charging is off
  for(int i=0;i<100;i++)
  {
    valueProbe1+=analogRead(analogPin1);  //read the input voltage one
    valueProbe2+=analogRead(analogPin2);  //read the input volrage two 
    delayMicroseconds(50);              // pauses for 50 microseconds  
  }
  
  valueProbe1=valueProbe1/100; 
  valueProbe2=valueProbe2/100;
  voltageProbe1=0.97*(valueProbe1 * 5100 * 3.127)/1023;
  voltageProbe2=0.97*(valueProbe2 * 5100 * 3.127)/1023;

  current = (voltageProbe1 - voltageProbe2) / resistance;     //calculate charge current
  currentError = targetCurrent - current;     //difference between target current and measured current

// ##########################################################
// ################### Current Control  #####################
// ##########################################################

if(ChargeDone == 0) {
 //if(CurrentLock == 0) {
    if(abs(currentError) > 10)    //if output error is large enough, adjust output
    {
      pwm_update_delay = 1000;
      PwmValue = PwmValue - currentError / 10;
      if(PwmValue < 1)   { PwmValue = 1;}
      if(PwmValue > 255) { PwmValue = 255;}
   } else {
      pwm_update_delay = 10000;
      CurrentLock = 1;
      CurrentLockVal = PwmValue;
      //Serial.println("Current locked !!!");     
  }
 //}
}

// ##########################################################
// ################### Led control  #########################
// ##########################################################

    // ChargeBulk  = 0;
    // ChargeFloat = 0;    
    // ChargeDone  = 1;
    
  if(ChargeDone == 0) {
    digitalWrite(ChargingLed, HIGH);   // turn the Charging LED ON
    if( ChargeFloat == 1) {
          digitalWrite(FloatLed, HIGH);   // turn the Done LED on   
    } else {
          digitalWrite(FloatLed, LOW);   // turn the Done LED on   
    }
    if(CurrentLock == 0) {
      digitalWrite(CurrentLockLed, HIGH);   // turn the Done LED on   
      delay (50);
      digitalWrite(CurrentLockLed, LOW);   // turn the Charge LED OFF
      delay (50);
      digitalWrite(CurrentLockLed, HIGH);   // turn the Charge LED OFF
      delay (50);
      digitalWrite(CurrentLockLed, LOW);   // turn the Charge LED OFF
    } else {
      digitalWrite(CurrentLockLed, HIGH);   // turn the Done LED on        
    }
  } else {
    digitalWrite(ChargingLed, LOW);   // turn the Charge LED OFF
    digitalWrite(CurrentLockLed, LOW);   // turn the Done LED on   
    digitalWrite(FloatLed, LOW);   // turn the Done LED on   
    digitalWrite(DoneLed, HIGH);   // turn the Charge LED OFF
  }
 
// ##########################################################
// ################### Print Measurments ####################
// ##########################################################

  Serial.print("PWM: ");     //display output values for monitoring with a computer
  Serial.print(PwmValue); 
  //Serial.print(" -- Supply [mV]: ");     //display battery voltage
  //Serial.print(voltageProbe3); 
  Serial.print(" -- Volt [mV]: ");     //display battery voltage
  Serial.print(batteryVoltage); 
  Serial.print(" -- Curr [mA]: ");     //display actual current
  Serial.print(current);  
  Serial.print(" -- State [%]: ");
  Serial.print(ChargePer);
  Serial.println();     //extra spaces to make debugging data easier to read
  
// ##########################################################
// ################### End Loop stage ####################
// ##########################################################

  analogWrite(PwmPin, PwmValue);  //Write output value to output pin
  delay(pwm_update_delay);     //delay 10 seconds before next iteration

}




