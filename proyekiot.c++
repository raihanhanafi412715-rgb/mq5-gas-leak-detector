#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>

hd44780_I2Cexp lcd;

// --- PENGATURAN PIN (ARDUINO UNO) ---
const int PIN_MQ5       = A0; // Pin AO sensor MQ-5
const int PIN_LED_HIJAU = 7;  // LED Hijau (+ resistor 220-330 Ohm)
const int PIN_BUZZER    = 8;  // Buzzer 5V

// --- PENGATURAN SENSITIVITAS ---
const bool PAKAI_BUZZER    = true; // Ubah ke false jika tanpa buzzer
const bool BUZZER_AKTIF    = true; // true = Buzzer Aktif 5V, false = Buzzer Pasif
const int  SELISIH_BAHAYA  = 80;   // Masuk BAHAYA jika angka naik +80 dari udara normal
const int  SELISIH_AMAN    = 45;   // Kembali AMAN jika turun ke +45 dari udara normal
const int  DETIK_KALIBRASI = 10;   // Waktu kalibrasi awal (detik)

// --- VARIABEL INTERNAL ---
int  nilaiUdaraNormal     = 300;
bool statusBahaya         = false;
int  nilaiGasTerakhir     = -1;
int  statusTerakhir       = -1;
bool bunyiBuzzer          = false;
unsigned long waktuBaca   = 0;
unsigned long waktuBuzzer = 0;

int bacaSensorMQ5() {
  long total = 0;
  for (int i = 0; i < 10; i++) {
    total += analogRead(PIN_MQ5);
    delay(5);
  }
  return (int)(total / 10);
}

void matikanBuzzer() {
  if (!PAKAI_BUZZER) return;
  if (BUZZER_AKTIF) {
    digitalWrite(PIN_BUZZER, LOW);
  } else {
    noTone(PIN_BUZZER);
  }
  bunyiBuzzer = false;
}

void nyalakanBuzzer() {
  if (!PAKAI_BUZZER) return;
  if (BUZZER_AKTIF) {
    digitalWrite(PIN_BUZZER, HIGH);
  } else {
    tone(PIN_BUZZER, 1000);
  }
  bunyiBuzzer = true;
}

void setup() {
  Serial.begin(9600);

  pinMode(PIN_MQ5, INPUT);
  pinMode(PIN_LED_HIJAU, OUTPUT);
  if (PAKAI_BUZZER) {
    pinMode(PIN_BUZZER, OUTPUT);
  }

  // Mulai langsung dalam kondisi AMAN
  digitalWrite(PIN_LED_HIJAU, HIGH);
  matikanBuzzer();

  lcd.begin(16, 2);
  Wire.setClock(50000);
  lcd.backlight();

  // Pemanasan singkat & kalibrasi udara ruangan otomatis
  lcd.setCursor(0, 0);
  lcd.print("Kalibrasi Udara ");
  for (int detik = DETIK_KALIBRASI; detik > 0; detik--) {
    lcd.setCursor(0, 1);
    lcd.print("Tunggu: ");
    if (detik < 10) lcd.print(" ");
    lcd.print(detik);
    lcd.print(" dtk  ");
    delay(1000);
  }

  // Rekam nilai udara bersih saat ini sebagai patokan normal
  nilaiUdaraNormal = bacaSensorMQ5();
  if (nilaiUdaraNormal > 900) {
    nilaiUdaraNormal = 900;
  }

  lcd.clear();
  Serial.print(F("Patokan Udara Normal: "));
  Serial.println(nilaiUdaraNormal);
}

void loop() {
  unsigned long sekarang = millis();

  if (sekarang - waktuBaca >= 500) {
    waktuBaca = sekarang;

    int nilaiGas = bacaSensorMQ5();

    // Jika sensor makin hangat dan angka udara bersih turun, ikuti perlahan
    if (!statusBahaya && nilaiGas > 50 && nilaiGas < nilaiUdaraNormal) {
      nilaiUdaraNormal--;
    }

    int batasBahaya = nilaiUdaraNormal + SELISIH_BAHAYA;
    int batasAman   = nilaiUdaraNormal + SELISIH_AMAN;
    if (batasBahaya > 1000) batasBahaya = 1000;
    if (batasAman > 960)    batasAman   = 960;

    // Cek apakah terjadi lonjakan gas
    if (!statusBahaya && nilaiGas >= batasBahaya) {
      statusBahaya = true;
    } else if (statusBahaya && nilaiGas <= batasAman) {
      statusBahaya = false;
    }

    // Tampilkan info lengkap di Serial Monitor
    Serial.print(F("Gas: "));
    Serial.print(nilaiGas);
    Serial.print(F(" | Batas: "));
    Serial.print(batasBahaya);
    Serial.print(F(" | "));
    Serial.println(statusBahaya ? F("BAHAYA") : F("AMAN"));

    // Update baris 1 LCD
    if (nilaiGas != nilaiGasTerakhir) {
      lcd.setCursor(0, 0);
      lcd.print("Gas MQ5: ");
      lcd.print(nilaiGas);
      if (nilaiGas < 10)        lcd.print("      ");
      else if (nilaiGas < 100)  lcd.print("     ");
      else if (nilaiGas < 1000) lcd.print("    ");
      else                      lcd.print("   ");
      nilaiGasTerakhir = nilaiGas;
    }

    // Update baris 2 LCD & LED Hijau
    if ((int)statusBahaya != statusTerakhir) {
      lcd.setCursor(0, 1);
      if (!statusBahaya) {
        digitalWrite(PIN_LED_HIJAU, HIGH);
        matikanBuzzer();
        lcd.print("Status: AMAN    ");
      } else {
        digitalWrite(PIN_LED_HIJAU, LOW);
        lcd.print("AWAS! GAS BOCOR!");
      }
      statusTerakhir = (int)statusBahaya;
    }
  }

  // Alarm bip-bip saat BAHAYA
  if (statusBahaya && PAKAI_BUZZER) {
    if (sekarang - waktuBuzzer >= 300) {
      waktuBuzzer = sekarang;
      if (bunyiBuzzer) {
        matikanBuzzer();
      } else {
        nyalakanBuzzer();
      }
    }
  }
}