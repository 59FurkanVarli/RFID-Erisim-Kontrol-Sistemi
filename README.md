# Arduino RFID & Keypad Güvenlik Sistemi 🔒

Bu proje, Arduino tabanlı, RFID kart ve tuş takımı (Keypad) entegrasyonuna sahip gelişmiş bir kapı kilit sistemidir. Sistem, yetkili kullanıcıların kart okuttuktan sonra şifre girmesini gerektirir (2 Faktörlü Kimlik Doğrulama benzeri) ve tüm verileri EEPROM üzerinde güvenli bir şekilde saklar.

## 🌟 Özellikler

* **Çift Aşamalı Doğrulama:** RFID Kart + Şifre.
* **Master Kart:** Yönetici kartı okutulduğunda şifre sormadan kapı açılır ve sistem kilidi sıfırlanır.
* **Şifre Değiştirme:** Kullanıcılar 'A' tuşuna basarak kendi şifrelerini tuş takımı üzerinden değiştirebilir.
* **Kalıcı Hafıza (EEPROM):** Elektrik kesilse bile kullanıcıların değiştirdiği şifreler silinmez.
* **Brute-Force Koruması:** Üst üste 3 kez yanlış şifre girildiğinde sistem kendini 1 dakika kilitler.
* **Tehdit Kodu:** '9999' şifresi girilirse kapı açılır ancak Seri Port üzerinden "TEHDIT" alarmı gönderilir.

## 🛠 Donanım Gereksinimleri

* Arduino UNO (veya muadili)
* MFRC522 RFID Okuyucu Modülü
* 4x4 Membran Keypad
* 16x2 LCD Ekran (I2C Modüllü)
* Servo Motor (SG90 vb.)
* Buzzer ve LED
* Jumper Kablolar

## 🔌 Pin Bağlantıları

| Bileşen | Arduino Pin |
| :--- | :--- |
| **MFRC522 RST** | 9 |
| **MFRC522 SDA(SS)** | 10 |
| **MFRC522 MOSI** | 11 |
| **MFRC522 MISO** | 12 |
| **MFRC522 SCK** | 13 |
| **Servo** | A3 |
| **Buzzer** | 4 |
| **Yeşil LED** | A2 |
| **Keypad Satırlar** | 8, 7, 6, 5 |
| **Keypad Sütunlar** | 3, 2, A0, A1 |

## 📚 Kütüphaneler

Bu projeyi derlemek için Arduino IDE üzerinden şu kütüphaneleri kurmalısınız:
1.  `MFRC522` (Miguel Balboa)
2.  `LiquidCrystal_I2C` (Frank de Brabander)
3.  `Keypad` (Mark Stanley)
4.  `Servo` (Arduino Built-in)

## 🚀 Kurulum

1.  Devreyi şemaya uygun kurun.
2.  `SecuritySystem.ino` dosyasını açın.
3.  `authorizedUIDs` dizisine kendi RFID kartlarınızın ID'lerini yazın.
4.  Kodu Arduino'ya yükleyin.
5.  İlk kullanımda şifreler "1234" olarak ayarlanacaktır.

## ⚠️ Uyarı

Bu kod eğitim amaçlıdır. Gerçek bir güvenlik sisteminde kullanmadan önce röle bağlantılarını ve güç kaynağını kontrol ediniz.
