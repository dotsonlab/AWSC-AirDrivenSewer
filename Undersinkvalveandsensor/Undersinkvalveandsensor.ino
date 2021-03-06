//code for operation of system under sinks, shower and laundry in Alaska Sewer and Water challenge UAA team Pilot study
int emptytime = 20000;//time to empty different per container
int wooshtime = 20000;

void setup() {
  // put your setup code here, to run once:
//Serial.begin(9600);
    pinMode(3, OUTPUT);//Drain
    pinMode(4, INPUT);//pinkdrain--feedback wires representing open and closed
    pinMode(5, INPUT);//graydrain
    pinMode(6, OUTPUT);//Vent
    pinMode(7, INPUT);//pinkvent
    pinMode(9, INPUT);//grayvent
    pinMode(14, OUTPUT);//Air
    pinMode(15, INPUT);//pinkair
    pinMode(16, INPUT);//grayair

    pinMode(17, INPUT);//sensor
}

void loop() {
  delay(500);
  int sensorValue = digitalRead(17);
  int pinkdrain=digitalRead(4);
  int graydrain=digitalRead(5);
  int pinkvent=digitalRead(7);
  int grayvent=digitalRead(9);
  int pinkair=digitalRead(15);
  int grayair=digitalRead(16);
  bool valveCheck = false;

  //status();//subroutine to read status of valves shown at bottom of code
//Serial.print(sensorValue);
//    digitalWrite(3, HIGH);//drain
//    digitalWrite(6, HIGH);//Vent
//    digitalWrite(14, HIGH);//air
sensorValue = digitalRead(17);
if (sensorValue ==0){
  //when sensor is dry and air is closed
     digitalWrite(3, LOW);//drain
     digitalWrite(6, LOW);//Vent
    digitalWrite(14, LOW);//air
}

else if(sensorValue ==1){  //empty cycle
  Serial.println("emptying");
    digitalWrite(3, HIGH);//drain
    digitalWrite(6, HIGH);//Vent
    while(valveCheck == false){ //wait for drain and vent valves to be closed
      pinkdrain=digitalRead(4);
      graydrain=digitalRead(5);
      pinkvent=digitalRead(7);
      grayvent=digitalRead(9);
      if ((graydrain==1) && (pinkdrain ==0) && (grayvent==1) && (pinkvent ==0)){
        valveCheck = true;
      }
    }
    valveCheck = false;
    
    digitalWrite(14, HIGH);//air
    delay(emptytime);//time to empty set at top of code
    delay(emptytime);
    delay(emptytime);
    delay(emptytime);
    delay(emptytime);
    delay(emptytime);
    delay(emptytime);//2min20sec
    
    digitalWrite(14, LOW);//air
    while(valveCheck == false){ //wait for air to be closed
      pinkair=digitalRead(15);
      grayair=digitalRead(16);
      if ((pinkair==1) && (grayair==0)){
        valveCheck = true;
      }
    }
    valveCheck = false;
    delay(wooshtime);//time for pressure to dissipate
    
    digitalWrite(6, LOW);//vent//when air is closed, open vent release pressrue
    while(valveCheck == false){ 
      pinkvent=digitalRead(7);
      grayvent=digitalRead(9);
      if ((pinkvent==1) && (grayvent==0)){
        valveCheck = true;
      }
    }
    valveCheck = false;
    
    digitalWrite(3, LOW);//drain
    while(valveCheck == false){
      pinkdrain=digitalRead(4);
      graydrain=digitalRead(5); 
      if ((pinkdrain==1) && (graydrain==0)){
        valveCheck = true;
      }
    }
    valveCheck = false;
  //}else{digitalWrite(4,LOW);}
}

}
/*
void status(){delay(500);
  int pinkdrain=digitalRead(4);
  int graydrain=digitalRead(5);
  int pinkvent=digitalRead(7);
  int grayvent=digitalRead(9);
  int pinkair=digitalRead(15);
  int grayair=digitalRead(16);

  
  Serial.print("Status -->  ");
  
  Serial.print("DRAIN");
  if ((pinkdrain==1) && (graydrain==0))
  {Serial.print(": open; ");
  }//Drain is open
  else if ((graydrain==1) && (pinkdrain ==0))
  {Serial.print(": closed; ");
  }//drain is closed
  else{
  Serial.print(": moving; ");//drain valve is moving
  }
  
  Serial.print("COMPRESSED AIR");
  if ((pinkair==1) && (grayair==0))
  {Serial.print(": closed; ");
  }//air is closed
  else if ((grayair==1) && (pinkair ==0))
  {Serial.print(": open; ");
  }//air is open
  else{
  Serial.print(": moving; ");//valve is moving
  }
  
  Serial.print("VENT");
  if ((pinkvent==1) && (grayvent==0))
  {Serial.print(": open; ");
  }//vent is open
  else if ((grayvent==1) && (pinkvent ==0))
  {Serial.print(": closed; ");
  }//vent is closed
  else{
  Serial.print(": moving; ");//valve is moving
  }
  
  Serial.println();
  }

*/

