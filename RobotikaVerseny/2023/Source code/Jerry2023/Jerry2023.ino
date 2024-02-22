#include <MPU6050_light.h>
#include <SPI.h>
#include <MFRC522.h>
#include <string.h>
#include <stdio.h>
#include "Wire.h"

MPU6050 mpu(Wire);
unsigned long timer = 0;


#define TRIGGER_PIN_FRONT 2
#define ECHO_PIN_FRONT 3
#define TRIGGER_PIN_RIGHT 4
#define ECHO_PIN_RIGHT 5
#define TRIGGER_PIN_LEFT 6
#define ECHO_PIN_LEFT 7

#define ENA 11
#define IN1 29
#define IN2 27
#define IN3 25
#define IN4 23
#define ENB 10

#define RST_PIN 8
#define SS_PIN 9

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

void setup() {
  Serial.begin(9600);
  //gyro beállítása
  Wire.begin();
   byte status = mpu.begin();
  Serial.print(F("MPU6050 status: "));
  Serial.println(status);
  while(status!=0){ } // stop everything if could not connect to MPU6050
  
  Serial.println(F("Calculating offsets, do not move MPU6050"));
  delay(1000);
  mpu.calcOffsets(); // gyro and accelero
  Serial.println("Done!\n");

  // Ultrahangos távérzékelő pin-ek beállítása
  pinMode(TRIGGER_PIN_FRONT, OUTPUT);
  pinMode(ECHO_PIN_FRONT, INPUT);
  pinMode(TRIGGER_PIN_RIGHT, OUTPUT);
  pinMode(ECHO_PIN_RIGHT, INPUT);
  pinMode(TRIGGER_PIN_LEFT, OUTPUT);
  pinMode(ECHO_PIN_LEFT, INPUT);

  // Motorvezérlő pin-ek beállítása
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);

  // RFID kártyaolvasó inicializálása
  SPI.begin();
  mfrc522.PCD_Init();
  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
}
//TÁVOLSÁGMÉRÉS CM-BEN
float measureDistance(int triggerPin, int echoPin) {
  // Előkészítés
  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);

  // Trigger jel küldése
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);

  // Echo jel értelmezése
  long duration = pulseIn(echoPin, HIGH);
  // Távolság kiszámítása a hangsebesség alapján
  float distance = duration * 0.034 / 2; // A hangsebesség 34 cm/ms, a távolságot pedig két irányban kell elosztani
  
  //if(distance > 200){
    //distance = 0;
  //}
  return distance;
}
// Előre haladás
void forward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 80);  // Bal
  analogWrite(ENB, 80);  // Jobb
}

// Hátramenet
void backward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, 80);  // Sebesség beállítása 0-255 között
  analogWrite(ENB, 80);  // Sebesség beállítása 0-255 között
}

// Balra fordulás
void turnLeft() {
  mpu.update();
  float angle = mpu.getAngleZ();
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 30);  // Bal
  analogWrite(ENB, 160);  // Jobb
  while(mpu.getAngleZ() <= angle+90){ // lehet több vagy kevesebb a kívánt fok
    mpu.update();
  }
  backward();
  delay(400);

}

// Jobbra fordulás
void turnRight() {
  mpu.update();
  float angle = mpu.getAngleZ();
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, 160);  // Bal
  analogWrite(ENB, 30);  // Jobb
  while(mpu.getAngleZ() >= angle-90){ // lehet több vagy kevesebb a kívánt fok
    mpu.update();
  }
  backward();
  delay(400);


}

// Megállás
void stop() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}
void forwardWithAlignment() {
  // Távérzékelők mérése
  float rightDistance = measureDistance(TRIGGER_PIN_RIGHT, ECHO_PIN_RIGHT);
  delay(10);
  float leftDistance = measureDistance(TRIGGER_PIN_LEFT, ECHO_PIN_LEFT);
  if(leftDistance > 200){
    // jobra kanyar
      analogWrite(ENA, 255);  // bal oldal
      analogWrite(ENB, 30);  // jobb oldal
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
      leftDistance = 0;
      delay(100);
  }
  if(rightDistance > 200){
    // balra kanyar
      analogWrite(ENA, 30);  // Bal oldal
      analogWrite(ENB, 255);  // jobb oldal
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
      rightDistance = 0;
      delay(100);
  }
  Serial.print("Bal oldal:");
  Serial.print(leftDistance);Serial.print("Jobb oldal:");Serial.println(rightDistance);
  if (rightDistance >= -1 && rightDistance <= 15 && leftDistance >= -1 && leftDistance <= 15){ //van két fal
      // Középre igazítás
    if (rightDistance < leftDistance) {
      // balra kanyar
      analogWrite(ENA, 30);  // Bal oldal
      analogWrite(ENB, 125);  // jobb oldal
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
    } else if (leftDistance < rightDistance) {
      // jobra kanyar
      analogWrite(ENA, 125);  // bal oldal
      analogWrite(ENB, 30);  // jobb oldal
      digitalWrite(IN1, HIGH);
      digitalWrite(IN2, LOW);
      digitalWrite(IN3, HIGH);
      digitalWrite(IN4, LOW);
    } 
  }
  else {
    if(rightDistance >= 15 &&  leftDistance <= 15){ // bal oldalon van fal
      Serial.println("Bal oldalon van fal");
      if(leftDistance > 5){
        // balra kanyar
        analogWrite(ENA, 30);  // Bal oldal
        analogWrite(ENB, 125);  // jobb oldal
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
      } else {
        // jobra kanyar
        analogWrite(ENA, 125);  // bal oldal
        analogWrite(ENB, 30);  // jobb oldal
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
      }
      
    } else if (rightDistance <= 15 &&  leftDistance >= 15){ //jobb oldalon fal
      if(rightDistance > 5){
        // jobra kanyar
        analogWrite(ENA, 125);  // bal oldal
        analogWrite(ENB, 30);  // jobb oldal
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
            
      } else {
        // balra kanyar
        analogWrite(ENA, 30);  // Bal oldal
        analogWrite(ENB, 125);  // jobb oldal
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
      }
    } else {
      // Középre igazítás nincs szükség, egyenesen megy előre
      forward();
    }
      
  }
  
}



void loop() {
  // put your main code here, to run repeatedly:
  //gyro update
  mpu.update();
  // RFID kártyaolvasó ellenőrzése
  if (mfrc522.PICC_IsNewCardPresent()) {
    if (mfrc522.PICC_ReadCardSerial()) {
      // RFID kártya adatok kiolvasása
      String cardData = "";
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        cardData += (String)(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        cardData += String(mfrc522.uid.uidByte[i], HEX);
      }
      Serial.print("Sima: ");
      Serial.print(cardData);
      Serial.println();
      String vagott = cardData.substring (4,8);
      Serial.print("vagott: ");
      Serial.print(vagott);
      Serial.println();
      

      // Kártya adatok alapján műveletek végrehajtása
      if (cardData.substring (4,8) == "bc 0") {
        Serial.print("fordulas balra: ");
        forwardWithAlignment();
        
        delay(200);
        stop();
        delay(100);
        turnLeft();
        
      } else if (cardData.substring (4,8) == "bc f") {
        Serial.print("fordulas jobbra: ");
        forwardWithAlignment();
        
        delay(200);
        stop();
        delay(100);
        turnRight();
        
      } else if (cardData.substring (4,8) == "bc 5") {
        // Program kilépése
        stop();
        while (true) {
          // Végtelen ciklusban várakozás
        }
      }
    }
  }

  // Első távérzékelő mérése cm-ben
  float frontDistance = measureDistance(TRIGGER_PIN_FRONT, ECHO_PIN_FRONT);
  //Serial.println(frontDistance);
  
  if (frontDistance <= 5){
    stop();
    
    while(true){}
  }
  // Fal érzékelése az első távérzékelővel
  if (frontDistance <= 15) {
    
    
    // Fal érzékelése, fordulás az irányban, ahol több hely van
    if (measureDistance(TRIGGER_PIN_LEFT, ECHO_PIN_LEFT) > measureDistance(TRIGGER_PIN_RIGHT, ECHO_PIN_RIGHT)) {
      // Balra fordulás, mert több hely van jobbra
      backward();
      delay(500);
      turnLeft();
      
    } else {
      // Jobbra fordulás, mert több hely van balra
      backward();
      delay(500);
      turnRight();
      
    }
  } else {
    // Nincs fal az előtt, előre megyünk középre igazítással
    forwardWithAlignment();
  }
}
