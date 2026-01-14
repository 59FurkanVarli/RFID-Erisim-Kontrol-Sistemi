import serial
import time
import csv
import os
from datetime import datetime

# ================= AYARLAR =================
# Bilgisayarınızdaki Arduino portunu buraya yazın (Örn: COM3, /dev/ttyUSB0)
ARDUINO_PORT = 'COM3'  
BAUD_RATE = 9600
DOSYA_ADI = "Giris_Kayitlari.csv" 

# ================= BAĞLANTI =================
print("------------------------------------------")
print(f"Baglanti araniyor: {ARDUINO_PORT}...")

try:
    ser = serial.Serial(ARDUINO_PORT, BAUD_RATE, timeout=1)
    time.sleep(2) # Arduino resetlendiği için biraz bekle
    print("Bağlantı BAŞARILI!")
    print("Log sistemi aktif. Çıkış için CTRL+C yapın.")
    print("------------------------------------------")
except serial.SerialException:
    print(f"HATA: Arduino {ARDUINO_PORT} portunda bulunamadı!")
    print("Lütfen kabloyu ve port numarasını kontrol edin.")
    exit()

# ================= DOSYA HAZIRLIĞI =================
# Dosya yoksa başlıkları oluştur
if not os.path.exists(DOSYA_ADI):
    with open(DOSYA_ADI, 'w', newline='', encoding='utf-8') as f:
        yazici = csv.writer(f)
        yazici.writerow(["Tarih", "Saat", "Kullanici", "Kart_ID"])

# ================= ANA DÖNGÜ =================
while True:
    try:
        # Arduino'dan veri gelmesini bekle
        if ser.in_waiting > 0:
            try:
                line = ser.readline().decode('utf-8').strip()
            except UnicodeDecodeError:
                continue # Hatalı karakter gelirse atla
            
            # Gelen veri "LOG" ile başlıyorsa işle
            if line.startswith("LOG"):
                data = line.split(",") 
                
                # Format: LOG,Kullanici_1,XX XX XX XX
                if len(data) >= 3:
                    kullanici = data[1]
                    kart_id = data[2]
                    
                    simdi = datetime.now()
                    tarih = simdi.strftime("%d.%m.%Y")
                    saat = simdi.strftime("%H:%M:%S")
                    
                    # CSV Dosyasına Ekle
                    with open(DOSYA_ADI, 'a', newline='', encoding='utf-8') as f:
                        yazici = csv.writer(f)
                        yazici.writerow([tarih, saat, kullanici, kart_id])
                    
                    print(f"[KAYIT EDILDI] {tarih} {saat} -> {kullanici}")
                
            elif line.startswith("ALARM"):
                 print(f"!!! GUVENLIK UYARISI !!! -> {line}")

    except KeyboardInterrupt:
        print("\nSistem kapatılıyor...")
        ser.close()
        break
    except Exception as e:
        print(f"Beklenmedik Hata: {e}")
        break
