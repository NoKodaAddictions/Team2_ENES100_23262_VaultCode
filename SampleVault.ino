/*
Krishay Iyer
Kevin Pei
Sudeep Abburu
Christian Garcia

ENES100-23262
Professor Mellini

Code to run the Elegoo/Arduino vault
Utilizes membrane keypad switch and RFID input
*/

//Importing Libraries
#include <LiquidCrystal.h> // For the Screen
#include <Keypad.h> // For the membrane keypad switch
#include <string.h>
#include <Servo.h> // Manages the servo
#include <SPI.h> // MFRC522 RFID Reader
#include <MFRC522.h>


//Membrane Keypad Switch Initialization

const byte ROWS = 4;
const byte COLS = 4;

char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte colPins[ROWS] = {30, 32, 34, 36};
byte rowPins[COLS] = {22, 24, 26, 28};

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

//LCD Display Initialization
//Referencing Pins
const int rs = 2, en = 4, d4 = 9, d5 = 10, d6 = 11, d7 = 12;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


//Passkey Initialization
String inputPasskey = "";
String correctPasskey = "YOUR_PASSKEY_HERE";
String rootPasskey = "YOUR_ADMIN_PASSKEY_HERE"; // Passkey to access settings
String newInputKey = "";

bool correct = false; // Giving the RFID reader a break, but doesn't seem to help.
bool root = false; // If this is activated, that means the user can change root-level settings like password, etc
bool passKeyMode = false; //Change passkey
bool rfidReadMode = false; //Read rfid
bool rfidRegMode = false; // Add RFID to accepted array
bool rfidDeRegMode = false; // Remove RFID from accepted array


int numTries = 0;
int numTriesLimit = 5;
int timeOut = 30; //In Seconds

//Servo Initialization
Servo myservo;  // create servo object to control a servo
int servoPin = 40;
int unlockPos = 180;
int lockPos = 90;

//LED Initialization
int ledPin = 13; //LED

//MFRC522 RFID Reader Initialization
int rstPin = 8;
int ssPin = 53;
MFRC522 mfrc522(ssPin, rstPin);
const int rfidArrLength = 1;
String rfidTags[rfidArrLength] = {};
String tagID;

void clearScreen(){ // This function is redundant :/
  lcd.clear();
}

//Function to unlock the vault and perform all the necessary unlock functions
void ledUnlock(String statement){
  digitalWrite(ledPin, LOW);
  myservo.write(unlockPos);
  clearScreen();
  lcd.setCursor(0,0);
  lcd.print(statement);
  lcd.setCursor(0,1);
  lcd.print("Vault Unlocked");
  correct = true;
}

//Function to lock the vault and perform all the necessary lock functions
void ledLock(String statement){
  digitalWrite(ledPin, HIGH);
  myservo.write(lockPos);
  clearScreen();
  lcd.setCursor(0,0);
  inputPasskey="";
  lcd.print(statement);
  lcd.setCursor(0, 1);
  lcd.print("Vault Locked");
  correct = false;
}

String readTag() 
{
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return "";
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return "";
  }
  //Show UID on serial monitor
  Serial.print("UID tag :");
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  content.toUpperCase();
  return content.substring(1);
} 

bool checkTag(String tag){
  if (tag.equals("")){
    return false;
  }
  else{
    for (int i=0; i < rfidArrLength; i++){
      if (tag.equals(rfidTags[i])){
        return true;
      }
    }
  }
  return false;
}


void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("Enter Passkey");
  //Starting serial port output (not used)
  Serial.begin(9600);
  //Referencing the servo's control pin
  myservo.attach(servoPin);
  //Setting the servo to its lock position

  pinMode(ledPin, OUTPUT); //This is to give another indication of the vault being locked or unlocked.
  ledLock("");

  SPI.begin();
  mfrc522.PCD_Init();
  delay(4);
  mfrc522.PCD_DumpVersionToSerial();
}

void loop() {

  if (numTries >= numTriesLimit){ // Lock the user out if the attempts reach the limit
    correct = false;
    ledLock("");
    for (int i = 0; i <= timeOut; i++){
      clearScreen();
      lcd.setCursor(0,0);
      lcd.print("Lock Timeout");
      lcd.setCursor(0, 1);
      lcd.print(String(timeOut - i));
      delay(1000);
    }
    numTries = 0;
    clearScreen();
    lcd.setCursor(0,0);
    lcd.print("Enter Passkey");
  }

  else if (root){ //ROOT MODE
    char button = customKeypad.getKey(); // Retreive button input

    if (button) {
      Serial.println(button);
      if (button == '#'){ // Locking/Unlocking Vault
        if (correct){ // Already correct? lock it
          Serial.println("Vault Locked");
          Serial.println("Goodbye Admin");
          
          inputPasskey=""; //Erasing input passkey
          root = false;
          ledLock("Goodbye Admin");
        }
      }

      else if (button == 'A'){
        clearScreen();
        lcd.setCursor(0,0);
        lcd.print("Change Passkey");
        newInputKey = "";
        passKeyMode = true;

        while (passKeyMode){
          char button = customKeypad.getKey(); // Retreive button input
          if (button){
            if (button == '#'){
              if (newInputKey.length() != 0) {
                correctPasskey = newInputKey.substring(0, newInputKey.length());
                passKeyMode = false;
                ledLock("Pskey Changed");
                root = false;
                delay(1000);
                clearScreen();
                lcd.setCursor(0,0);
                lcd.print("Enter Passkey");
                lcd.setCursor(0,1);
                Serial.println(inputPasskey);
                lcd.print(inputPasskey);
              }
              else{
                clearScreen();
                lcd.setCursor(0,0);
                lcd.print("No Blank Pskey");
              }
            }
            else if (button == '*'){
              newInputKey.remove(newInputKey.length()-1);
              clearScreen();
              lcd.setCursor(0,0);
              lcd.print("Change Passwd");
              lcd.setCursor(0, 1);
              lcd.print(newInputKey);
            }
            else{
              newInputKey += button;
              Serial.println(newInputKey);
              clearScreen();
              lcd.setCursor(0,0);
              lcd.print("Change Passwd");
              lcd.setCursor(0, 1);
              lcd.print(newInputKey);
            }
          }
        }
      }

      else if (button == 'B'){ //Reads serial off of RFID (serial is used to authenticate)
        clearScreen();
        lcd.setCursor(0,0);
        lcd.print("Read RFID");
        rfidReadMode = true;

        while (rfidReadMode){
          char button = customKeypad.getKey(); // Retreive button input
          if(!readTag().equals("")){
            lcd.setCursor(0,1);
            lcd.print(readTag());
          }
          if (button){
            if (button == '*'){
              clearScreen();
              lcd.setCursor(0,0);
              lcd.print("Welcome Admin");
              lcd.setCursor(0,1);
              lcd.print("Use Letter Btn");
              rfidReadMode=false;
            }
          }
        }
      }


      else if (button == 'D'){ // Displays current passcode
        clearScreen();
        lcd.setCursor(0,0);
        lcd.print("Current Pass");
        lcd.setCursor(0,1);
        Serial.println(correctPasskey);
        lcd.print(correctPasskey);
      }
    }
  }

  else if (!root){
    // Serial.println(correct);
    if (!correct){

      tagID = readTag();
      Serial.println(tagID);

      if (checkTag(tagID) == true){
        if (inputPasskey.equals("AAA")){
          root = true;
          clearScreen();
          lcd.setCursor(0,0);
          lcd.print("Welcome Admin");
          lcd.setCursor(0,1);
          lcd.print("Use Letter Btn");
        }

        ledUnlock("Correct Card");
      }

      else if (!tagID.equals("")){
        clearScreen();
        lcd.setCursor(0,0);
        numTries++;
        lcd.print("Incorrect Card " + String(numTries));
        inputPasskey = "";
        delay(1000);
      }
    }

    char button = customKeypad.getKey(); // Retreive button input
      
    lcd.setCursor(0, 1);

    if (button) {
      if (button == '#'){ // Locking/Unlocking Vault
        if (correct){ // Already correct? lock it
          
          ledLock("Goodbye");
        }

        else if (inputPasskey.equals(correctPasskey)){ // Check if it is correct, if not already correct
          ledUnlock("Correct Passkey");
        }

        else if (inputPasskey.equals(rootPasskey)){ //Admin
          ledUnlock("Welcome Admin");
          root = true;

          clearScreen();
          lcd.setCursor(0,0);
          lcd.print("Welcome Admin");
          lcd.setCursor(0,1);
          lcd.print("Use Letter Btn");
        }

        else{ // Increase attempt by 1 if wrong
          // Serial.println("Wrong Input!");
          clearScreen();
          lcd.setCursor(0,0);
          numTries++;
          lcd.print("Incorrect Pass " + String(numTries));
          inputPasskey = "";
        }
      }

      else if (button == '*'){ // Removing the last character 
        if (inputPasskey.length() != 0){
          inputPasskey.remove(inputPasskey.length()-1);
          Serial.println(inputPasskey);
          clearScreen();
          lcd.setCursor(0,0);
          lcd.print("Enter Passkey");
          lcd.setCursor(0,1);
          lcd.print(inputPasskey);
        }
      }

      else{ //Adding input to end of string,
        inputPasskey += button;
        clearScreen();
        lcd.setCursor(0,0);
        lcd.print("Enter Passkey");
        lcd.setCursor(0,1);
        Serial.println(inputPasskey);
        lcd.print(inputPasskey);
      }
    }
  }
}