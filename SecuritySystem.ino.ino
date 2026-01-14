/*
  Proje: Arduino RFID & Keypad Security System (Gelişmiş Güvenlik Sistemi)
  Yazar: [59FurkanVarli]
  Tarih: 2026
  Açıklama: 
    Bu proje; RFID kart okuyucu, Keypad şifreleme, Servo motor kilit mekanizması 
    ve LCD ekran arayüzü içeren kapsamlı bir güvenlik sistemidir.
    EEPROM hafızası sayesinde şifreler güç kesilse bile saklanır.
  
  Özellikler:
    - Master Kart ile tam yetki (şifresiz giriş)
    - Kullanıcılar için RFID + Şifre (2FA benzeri)
    - Şifre değiştirme özelliği
    - Hatalı girişlerde sistem kilitleme (Brute-force koruması)
    - Tehdit altı şifresi (Panik kodu)
*/

#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Servo.h>
#include <EEPROM.h> 

// ================= PIN TANIMLARI =================
#define SS_PIN 10
#define RST_PIN 9
#define SERVO_PIN A3 

// LED VE BUZZER (Aktif Düşük/Yüksek durumuna göre bağlanmalı)
#define RED_BUZZER_PIN 4 
#define GREEN_LED_PIN A2 

// ================= NESNELER =================
MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2); 
Servo myServo;

// ================= ÖZEL İKONLAR =================
byte lockIcon[8] = {
  0b01110, 0b10001, 0b10001, 0b11111, 
  0b11011, 0b11011, 0b11111, 0b00000
}; 

byte checkIcon[8] = {
  0b00000, 0b00001, 0b00011, 0b10110, 
  0b11100, 0b01000, 0b00000, 0b00000
}; 

// ================= KEYPAD AYARLARI =================
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {8, 7, 6, 5}; 
byte colPins[COLS] = {3, 2, A0, A1}; 
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ================= KULLANICI YAPILANDIRMASI =================
const int userCount = 4; // Kac tane kart kullanacaksanız buradaki sayıyı ona göre güncelleyin.

String authorizedUIDs[userCount] = {
  "XX XX XX XX",  // [0] MASTER KART (Admin - Şifresiz)
  "YY YY YY YY",  // [1] Kullanıcı 1
  "ZZ ZZ ZZ ZZ",  // [2] Kullanıcı 2
  "QQ QQ QQ QQ"   // [3] Kullanıcı 3
};

// İlk kurulumda EEPROM'a yazılacak varsayılan şifreler
const String defaultPass[userCount] = {
  "",     // Master için şifre yok
  "1234", // Kullanıcı 1 Varsayılan
  "1234", // Kullanıcı 2 Varsayılan
  "1234"  // Kullanıcı 3 Varsayılan
};

// Çalışma zamanı şifreleri (RAM'de tutulur)
String authorizedPass[userCount];

// ================= GÜVENLİK DEĞİŞKENLERİ =================
int wrongAttempts = 0;          
unsigned long lockoutEndTime = 0; 
bool isLocked = false;          
String currentPass = "";

// ================= SETUP =================
void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();

  lcd.init();
  lcd.backlight(); 

  lcd.createChar(0, lockIcon);  
  lcd.createChar(1, checkIcon); 

  pinMode(RED_BUZZER_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  
  // Başlangıç durumu (Buzzer/Led lojik seviyesine göre ayarlayın)
  digitalWrite(RED_BUZZER_PIN, HIGH); 
  digitalWrite(GREEN_LED_PIN, HIGH);

  myServo.attach(SERVO_PIN);
  myServo.write(0); // Kilit kapalı pozisyon

  // --- EEPROM KONTROLÜ ---
  // Eğer sistem ilk kez çalışıyorsa varsayılan şifreleri yükle
  checkFirstRun(); 
  loadPasswords(); // Şifreleri hafızadan çek

  resetSystem(); 
}

// ================= LOOP =================
void loop() {
  
  // --- KİLİTLENME SÜRESİ KONTROLÜ ---
  if (isLocked) {
    long remainingTime = (lockoutEndTime - millis()) / 1000; 
    
    if (remainingTime <= 0) {
      isLocked = false;
      wrongAttempts = 0;
      resetSystem();
    } 
    else {
      lcd.setCursor(0,0);
      lcd.print("SISTEM KILITLI! ");
      lcd.setCursor(0,1);
      lcd.print("BEKLE: " + String(remainingTime) + " sn      ");
    }
    return; 
  }

  // --- KART OKUMA  ---
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {

    String readUID = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      if (rfid.uid.uidByte[i] < 0x10) readUID += " 0";
      else readUID += " ";
      readUID += String(rfid.uid.uidByte[i], HEX);
    }
    readUID.toUpperCase();
    readUID.trim();
    
    int userIndex = -1;
    for (int i = 0; i < userCount; i++) {
      if (readUID == authorizedUIDs[i]) {
        userIndex = i;
        break;
      }
    }

    // --- KART KONTROLLERİ ---
    if (userIndex != -1) {
      
      // 1. Durum: MASTER KART 
      if (userIndex == 0) {
        isLocked = false;   
        wrongAttempts = 0;  
        lockoutEndTime = 0; 

        beep(50); delay(50); beep(50); 
        
        lcd.clear();
        lcd.print("YONETICI MODU");
        lcd.setCursor(0,1);
        lcd.print("KILIT KALDIRILDI");
        delay(1500); 

        unlockDoor(0); 
      }
      // 2. Durum: NORMAL KULLANICI 
      else {
        beep(100); 
        askPassword(authorizedPass[userIndex], userIndex); 
      }
    } 
    else {
      // 3. Durum: TANIMSIZ KART
      lcd.clear();
      lcd.print("GECERSIZ KART");
      errorAlert(); 
      resetSystem();
    }
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
}

// ============== ŞİFRE İSTEME ==============
void askPassword(String truePass, int userIdx) {
  lcd.clear();
  lcd.print("SIFRE GIRIN:");
  lcd.setCursor(0,1);

  currentPass = "";
  unsigned long startTime = millis();

  while (true) {
    // Zaman aşımı kontrolü (15 sn)
    if (millis() - startTime > 15000) { 
      resetSystem();
      return;
    }

    char key = keypad.getKey();
    if (key) {
      startTime = millis(); 
      beep(50); 

      // --- KAPI AÇMA TUŞU (#) ---
      if (key == '#') { 
        if (currentPass == truePass) {
          wrongAttempts = 0; 
          unlockDoor(userIdx);
        } 
        else if (currentPass == "9999") { // TEHDİT KODU
           Serial.println("LOG,ALARM_TEHDIT," + authorizedUIDs[userIdx]); 
           unlockDoor(userIdx); // Kapıyı aç ama alarm ver
        }
        else {
           handleWrongPassword(); 
        }
        break; 
      }
      
      // --- ŞİFRE DEĞİŞTİRME TUŞU (A) ---
      else if (key == 'A') {
        if (currentPass == truePass) {
           changePassword(userIdx); 
           break;
        } else {
           handleWrongPassword();
           break;
        }
      }

      else if (key == '*') { 
        currentPass = "";
        lcd.setCursor(0,1);
        lcd.print("                "); 
        lcd.setCursor(0,1);
      }
      else { 
        currentPass += key;
        lcd.print("*");
      }
    }
  }
  
  if (!isLocked) resetSystem();
}

// ============== YENİ ŞİFRE BELİRLEME ==============
void changePassword(int userIdx) {
  lcd.clear();
  lcd.print("YENI SIFRE GIR:");
  lcd.setCursor(0, 1);
  
  String newPass = "";
  unsigned long startTime = millis();
  
  while (true) {
    if (millis() - startTime > 15000) {
      resetSystem();
      return;
    }

    char key = keypad.getKey();
    if (key) {
      startTime = millis();
      beep(50);

      if (key == '#') { // Onayla
        if (newPass.length() == 4) { 
          
          // 1. RAM'deki şifreyi güncelle
          authorizedPass[userIdx] = newPass; 
          
          // 2. EEPROM'a kalıcı olarak yaz
          writeStringToEEPROM(userIdx * 10, newPass); 

          lcd.clear();
          lcd.print("KAYDEDILIYOR...");
          delay(1000);
          
          lcd.clear();
          lcd.print("SIFRE DEGISTI!");
          lcd.setCursor(0,1);
          lcd.print("YENI: " + newPass);
          
          beep(100); delay(100); beep(100); 
          Serial.println("LOG,SIFRE_DEGISTI," + authorizedUIDs[userIdx]);
          delay(2000);
          resetSystem();
          return;
        } else {
          lcd.clear();
          lcd.print("4 HANE GIRINIZ!");
          delay(1500);
          lcd.clear();
          lcd.print("YENI SIFRE GIR:");
          lcd.setCursor(0,1);
          newPass = ""; 
        }
      }
      else if (key == '*') { 
        resetSystem();
        return;
      }
      else if (key == 'A' || key == 'B' || key == 'C' || key == 'D') {
        // Harf girişini engelle
      }
      else {
        if (newPass.length() < 4) {
          newPass += key;
          lcd.print(key); 
        }
      }
    }
  }
}

// ============== HATALI ŞİFRE YÖNETİMİ ==============
void handleWrongPassword() {
  wrongAttempts++; 
  lcd.clear();
  if (wrongAttempts >= 3) {
    lockoutEndTime = millis() + 60000; // 60 saniye kilit
    isLocked = true;
    lcd.print("COK HATA!");
    lcd.setCursor(0,1);
    lcd.print("SISTEM KILITLENDI");
    
    digitalWrite(RED_BUZZER_PIN, LOW); 
    delay(2000);
    digitalWrite(RED_BUZZER_PIN, HIGH);
    delay(2000); 
  } 
  else {
    lcd.print("YANLIS SIFRE!");
    lcd.setCursor(0,1);
    lcd.print("KALAN HAK: " + String(3 - wrongAttempts));
    errorAlert();
  }
}

// ============== KAPI AÇMA ==============
void unlockDoor(int userIndex) {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.write(byte(1)); 
  lcd.print(" KAPI ACILDI");
  
  lcd.setCursor(0,1);
  lcd.print("HOSGELDINIZ");

  Serial.print("LOG,Kullanici_");
  Serial.print(userIndex);
  Serial.print(",");
  Serial.println(authorizedUIDs[userIndex]);

  digitalWrite(GREEN_LED_PIN, LOW); 
  myServo.write(90); 
  delay(4000);       
  myServo.write(0);  
  digitalWrite(GREEN_LED_PIN, HIGH);
  
  resetSystem(); 
}

// ============== EEPROM YARDIMCI FONKSİYONLAR ==============
void writeStringToEEPROM(int addrOffset, const String &strToWrite) {
  byte len = strToWrite.length();
  EEPROM.write(addrOffset, len); 
  for (int i = 0; i < len; i++) {
    EEPROM.write(addrOffset + 1 + i, strToWrite[i]);
  }
}

String readStringFromEEPROM(int addrOffset) {
  int newStrLen = EEPROM.read(addrOffset);
  char data[newStrLen + 1];
  for (int i = 0; i < newStrLen; i++) {
    data[i] = EEPROM.read(addrOffset + 1 + i);
  }
  data[newStrLen] = '\0'; 
  return String(data);
}

// İlk kurulum kontrolü
void checkFirstRun() {
  // EEPROM'un 100. adresinde 99 sayısı yoksa bu ilk kurulumdur
  if (EEPROM.read(100) != 99) {
    lcd.clear();
    lcd.print("ILK KURULUM...");
    delay(1000);
    
    // Varsayılan şifreleri hafızaya yaz (Master hariç)
    for (int i = 1; i < userCount; i++) { 
      writeStringToEEPROM(i * 10, defaultPass[i]);
    }
    
    EEPROM.write(100, 99); // Bayrağı işaretle
    lcd.clear();
    lcd.print("HAFIZA HAZIR!");
    delay(1000);
  }
}

void loadPasswords() {
  authorizedPass[0] = "----"; 
  for (int i = 1; i < userCount; i++) {
    authorizedPass[i] = readStringFromEEPROM(i * 10);
  }
}

// ============== DİĞER YARDIMCI FONKSİYONLAR ==============
void resetSystem() {
  digitalWrite(RED_BUZZER_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, HIGH);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.write(byte(0)); 
  lcd.print(" KART OKUTUNUZ"); 
}

void errorAlert() {
  for(int i=0; i<3; i++){
    digitalWrite(RED_BUZZER_PIN, LOW);
    delay(600);
    digitalWrite(RED_BUZZER_PIN, HIGH);
    delay(600);
  }
}

void beep(int duration) {
  digitalWrite(RED_BUZZER_PIN, LOW);
  delay(duration);
  digitalWrite(RED_BUZZER_PIN, HIGH);
}