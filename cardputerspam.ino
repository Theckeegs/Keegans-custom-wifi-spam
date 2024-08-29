#include <M5Cardputer.h>
#include <WiFi.h>
#include "esp_wifi.h"

String ssid_base = "keegs";
int num_networks = 30;
bool update_ssid = false;
bool update_num_networks = false;
bool update_display = true;
bool wifi_spam_active = false;
bool charging_mode_active = false;
bool wifi_scan_active = false;

int menu_position = 0;
int color_menu_position = 0;
bool in_color_menu = false;
uint16_t text_color = WHITE;
uint16_t bg_color = BLACK;

bool key_up_pressed = false;
bool key_down_pressed = false;
bool key_left_pressed = false;
bool key_right_pressed = false;
bool key_enter_pressed = false;

unsigned long last_enter_press = 0;
const unsigned long debounce_delay = 300; // 300ms debounce time

unsigned long last_battery_update = 0;
const unsigned long battery_display_duration = 10000; // 10 seconds
const unsigned long battery_update_interval = 60000; // 1 minute in milliseconds

int current_page = 0;
const int ITEMS_PER_PAGE = 3;
const int TOTAL_PAGES = 2;

const char* menu_items[] = {
    "Toggle WiFi Spam",
    "Change SSID Name",
    "Set Network Count",
    "Change Color",
    "Charging Mode",
    "WiFi Scanner"
};
const int menu_items_count = sizeof(menu_items) / sizeof(menu_items[0]);

const char* color_menu_items[] = {
    "White", "Red", "Green", "Blue", "Yellow", 
    "Cyan", "Magenta", "Orange", "Pink", "Purple"
};
const int color_menu_items_count = sizeof(color_menu_items) / sizeof(color_menu_items[0]);

const uint16_t color_values[] = {
    WHITE, RED, GREEN, BLUE, YELLOW, 
    CYAN, MAGENTA, ORANGE, PINK, PURPLE
};

// Beacon Packet buffer
uint8_t packet[128] = { 0x80, 0x00, 0x00, 0x00, 
                /*4*/   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
                /*10*/  0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                /*16*/  0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 
                /*22*/  0xc0, 0x6c, 
                /*24*/  0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00, 
                /*32*/  0x64, 0x00, 
                /*34*/  0x01, 0x04, 
                /* SSID */
                /*36*/  0x00, 0x06, 0x72, 0x72, 0x72, 0x72, 0x72, 0x72,
                        0x01, 0x08, 0x82, 0x84,
                        0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, 0x03, 0x01, 
                /*56*/  0x04 };

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg);
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.fillScreen(bg_color);
  M5Cardputer.Display.setTextColor(text_color);
  M5Cardputer.Display.setTextSize(1.7);
  M5Cardputer.Display.setTextScroll(1.7f);  // Set text size to 1.5

  displayWelcomeScreen();
  
  WiFi.mode(WIFI_MODE_STA);
  esp_log_level_set("wifi", ESP_LOG_NONE);

  drawMenu();
}

void displayWelcomeScreen() {
  M5Cardputer.Display.fillScreen(bg_color);
  M5Cardputer.Display.setTextSize(2.5);
  
  // Calculate the width of each text line
  int16_t textWidth1 = M5Cardputer.Display.textWidth("Keegans Custom");
  int16_t textWidth2 = M5Cardputer.Display.textWidth("   Wifi Spam");

  // Center X positions
  int16_t xPosition1 = (M5Cardputer.Display.width() - textWidth1) / 2;
  int16_t xPosition2 = (M5Cardputer.Display.width() - textWidth2) / 2;

  // Set Y positions
  int16_t yPosition1 = M5Cardputer.Display.height() / 2 - 40; // Adjust Y position as needed
  int16_t yPosition2 = yPosition1 + 40; // Adjust the spacing between lines as needed

  // Set text color
  M5Cardputer.Display.setTextColor(text_color);

  // Print centered text
  M5Cardputer.Display.setCursor(xPosition1, yPosition1);
  M5Cardputer.Display.println("Keegans Custom");

  M5Cardputer.Display.setCursor(xPosition2, yPosition2);
  M5Cardputer.Display.println("  Wifi Spam");

  M5Cardputer.Display.setTextSize(1.7);

  delay(5000);  // Wait for 5 seconds
}

void drawMenu() {
  M5Cardputer.Display.fillScreen(bg_color);
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.setTextColor(text_color);
  
  // Display battery percentage at the top
  M5Cardputer.Display.printf("Batt: %d%%\n", M5Cardputer.Power.getBatteryLevel());
  
  int start_index = current_page * ITEMS_PER_PAGE;
  int end_index = min(start_index + ITEMS_PER_PAGE, menu_items_count);
  
  for (int i = start_index; i < end_index; i++) {
    if (i == menu_position) {
      M5Cardputer.Display.print(">");
    } else {
      M5Cardputer.Display.print(" ");
    }
    M5Cardputer.Display.println(menu_items[i]);
  }

  // Display WiFi spam status at the bottom
  M5Cardputer.Display.setCursor(0, M5Cardputer.Display.height() - 40);
  M5Cardputer.Display.printf("Name: %s\n", ssid_base.c_str());
  M5Cardputer.Display.printf("Networks: %d\n", num_networks);
  M5Cardputer.Display.printf("Status: %s", wifi_spam_active ? "Active" : "Inactive");

  // Display page number in the bottom right corner
  M5Cardputer.Display.setCursor(M5Cardputer.Display.width() - 30, M5Cardputer.Display.height() - 20);
  M5Cardputer.Display.printf("%d/%d", current_page + 1, TOTAL_PAGES);
}

void handleMenu() {
  bool menu_updated = false;

  if (M5Cardputer.Keyboard.isKeyPressed(';')) {
    if (!key_up_pressed) {
      menu_position = (menu_position - 1 + menu_items_count) % menu_items_count;
      current_page = menu_position / ITEMS_PER_PAGE;
      menu_updated = true;
      key_up_pressed = true;
    }
  } else {
    key_up_pressed = false;
  }

  if (M5Cardputer.Keyboard.isKeyPressed('.')) {
    if (!key_down_pressed) {
      menu_position = (menu_position + 1) % menu_items_count;
      current_page = menu_position / ITEMS_PER_PAGE;
      menu_updated = true;
      key_down_pressed = true;
    }
  } else {
    key_down_pressed = false;
  }

  if (M5Cardputer.Keyboard.isKeyPressed(',')) {
    if (!key_left_pressed) {
      current_page = (current_page - 1 + TOTAL_PAGES) % TOTAL_PAGES;
      menu_position = current_page * ITEMS_PER_PAGE;
      menu_updated = true;
      key_left_pressed = true;
    }
  } else {
    key_left_pressed = false;
  }

  if (M5Cardputer.Keyboard.isKeyPressed('/')) {
    if (!key_right_pressed) {
      current_page = (current_page + 1) % TOTAL_PAGES;
      menu_position = current_page * ITEMS_PER_PAGE;
      menu_updated = true;
      key_right_pressed = true;
    }
  } else {
    key_right_pressed = false;
  }

  if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
    unsigned long current_time = millis();
    if (!key_enter_pressed && (current_time - last_enter_press > debounce_delay)) {
      key_enter_pressed = true;
      last_enter_press = current_time;
      switch (menu_position) {
        case 0:  // Begin/Stop WiFi Spam
          wifi_spam_active = !wifi_spam_active;
          menu_updated = true;
          break;
        case 1:  // Change SSID Name
          update_ssid = true;
          displaySSIDInput();
          return;
        case 2:  // Set Network Count
          update_num_networks = true;
          displayNetworkCountInput();
          return;
        case 3:  // Change Color
          in_color_menu = true;
          color_menu_position = 0;
          drawColorMenu();
          return;
        case 4:  // Charging Mode
          enterChargingMode();
          return;
        case 5:  // WiFi Scanner
          wifi_scan_active = true;
          scanWiFiNetworks();
          return;
      }
    }
  } else {
    key_enter_pressed = false;
  }
  
  if (menu_updated) {
    drawMenu();
  }
}

void drawColorMenu() {
  M5Cardputer.Display.fillScreen(bg_color);
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.setTextColor(text_color);
  M5Cardputer.Display.println("Select Color:");
  for (int i = 0; i < color_menu_items_count; i++) {
    M5Cardputer.Display.setCursor((i % 2) * 120, 20 + (i / 2) * 20);
    if (i == color_menu_position) {
      M5Cardputer.Display.print(">");
    } else {
      M5Cardputer.Display.print(" ");
    }
    M5Cardputer.Display.print(color_menu_items[i]);
  }
}

void handleColorMenu() {
  bool menu_updated = false;

  if (M5Cardputer.Keyboard.isKeyPressed(';')) {
    if (!key_up_pressed) {
      color_menu_position = (color_menu_position - 2 + color_menu_items_count) % color_menu_items_count;
      menu_updated = true;
      key_up_pressed = true;
    }
  } else {
    key_up_pressed = false;
  }

  if (M5Cardputer.Keyboard.isKeyPressed('.')) {
    if (!key_down_pressed) {
      color_menu_position = (color_menu_position + 2) % color_menu_items_count;
      menu_updated = true;
      key_down_pressed = true;
    }
  } else {
    key_down_pressed = false;
  }

  if (M5Cardputer.Keyboard.isKeyPressed(',')) {
    if (!key_left_pressed) {
      color_menu_position = (color_menu_position - 1 + color_menu_items_count) % color_menu_items_count;
      menu_updated = true;
      key_left_pressed = true;
    }
  } else {
    key_left_pressed = false;
  }

  if (M5Cardputer.Keyboard.isKeyPressed('/')) {
    if (!key_right_pressed) {
      color_menu_position = (color_menu_position + 1) % color_menu_items_count;
      menu_updated = true;
      key_right_pressed = true;
    }
  } else {
    key_right_pressed = false;
  }

  if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
    unsigned long current_time = millis();
    if (!key_enter_pressed && (current_time - last_enter_press > debounce_delay)) {
      key_enter_pressed = true;
      last_enter_press = current_time;
      text_color = color_values[color_menu_position];
      in_color_menu = false;
      drawMenu();
    }
  } else {
    key_enter_pressed = false;
  }

  if (M5Cardputer.Keyboard.isKeyPressed('`')) {  // Backtick to exit
    in_color_menu = false;
    drawMenu();
  }
  
  if (menu_updated) {
    drawColorMenu();
  }
}

void displaySSIDInput() {
  M5Cardputer.Display.fillScreen(bg_color);
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.setTextColor(text_color);
  M5Cardputer.Display.println("Enter new SSID name:");
  M5Cardputer.Display.println(ssid_base);
  M5Cardputer.Display.println("ENTER to confirm");
  M5Cardputer.Display.println("ESC to cancel");
}

void displayNetworkCountInput() {
  M5Cardputer.Display.fillScreen(bg_color);
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.setTextColor(text_color);
  M5Cardputer.Display.println("Enter network count:");
  M5Cardputer.Display.println(num_networks);
  M5Cardputer.Display.println("ENTER to confirm");
  M5Cardputer.Display.println("ESC to cancel");
}

void enterChargingMode() {
  charging_mode_active = true;
  last_battery_update = millis(); // Start the cycle
  displayBatteryPercentage();
}

void displayBatteryPercentage() {
  M5Cardputer.Display.fillScreen(bg_color);
  M5Cardputer.Display.setTextColor(text_color);
  M5Cardputer.Display.setTextSize(3);
  
  String batteryText = String(M5Cardputer.Power.getBatteryLevel()) + "%";
  int16_t textWidth = M5Cardputer.Display.textWidth(batteryText.c_str());
  int16_t textHeight = M5Cardputer.Display.fontHeight();
  
  int16_t x = (M5Cardputer.Display.width() - textWidth) / 2;
  int16_t y = (M5Cardputer.Display.height() - textHeight) / 2;
  
  M5Cardputer.Display.setCursor(x, y);
  M5Cardputer.Display.print(batteryText);
  
  M5Cardputer.Display.setTextSize(1.7); // Reset text size
}

void scanWiFiNetworks() {
  M5Cardputer.Display.fillScreen(bg_color);
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.setTextColor(text_color);
  M5Cardputer.Display.println("Scanning WiFi networks...");

  int n = WiFi.scanNetworks();
  int current_page = 0;
  const int NETWORKS_PER_PAGE = 2;
  int total_pages = (n + NETWORKS_PER_PAGE - 1) / NETWORKS_PER_PAGE;

  bool left_key_pressed = false;
  bool right_key_pressed = false;
  unsigned long last_key_press_time = 0;
  const unsigned long debounce_delay = 200; // 200ms debounce time

  bool need_refresh = true;

  while (true) {
    if (need_refresh) {
      M5Cardputer.Display.fillScreen(bg_color);
      M5Cardputer.Display.setCursor(0, 0);

      if (n == 0) {
        M5Cardputer.Display.println("No networks found");
      } else {
        M5Cardputer.Display.printf("%d networks found\n\n", n);
        int start_index = current_page * NETWORKS_PER_PAGE;
        int end_index = min(start_index + NETWORKS_PER_PAGE, n);

        for (int i = start_index; i < end_index; ++i) {
          M5Cardputer.Display.printf("%d: %s\n   (%d)\n\n", i + 1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
        }

        // Display page number in the bottom right corner
        M5Cardputer.Display.setCursor(M5Cardputer.Display.width() - 30, M5Cardputer.Display.height() - 20);
        M5Cardputer.Display.printf("%d/%d", current_page + 1, total_pages);
      }

      need_refresh = false;
    }

    unsigned long current_time = millis();
    M5Cardputer.update();

    if (M5Cardputer.Keyboard.isKeyPressed(',')) {
      if (!left_key_pressed && (current_time - last_key_press_time > debounce_delay)) {
        if (current_page > 0) {
          current_page--;
          last_key_press_time = current_time;
          need_refresh = true;
        }
        left_key_pressed = true;
      }
    } else {
      left_key_pressed = false;
    }

    if (M5Cardputer.Keyboard.isKeyPressed('/')) {
      if (!right_key_pressed && (current_time - last_key_press_time > debounce_delay)) {
        if (current_page < total_pages - 1) {
          current_page++;
          last_key_press_time = current_time;
          need_refresh = true;
        }
        right_key_pressed = true;
      }
    } else {
      right_key_pressed = false;
    }

    if (M5Cardputer.Keyboard.isKeyPressed('`')) {
      wifi_scan_active = false;
      drawMenu();
      return;
    }

    delay(10); // Small delay to prevent excessive CPU usage
  }
}

void loop() {
  M5Cardputer.update();
  
  if (charging_mode_active) {
    unsigned long current_time = millis();
    unsigned long time_in_cycle = (current_time - last_battery_update) % battery_update_interval;
    
    if (time_in_cycle < battery_display_duration) {
      // Display battery percentage for the first 10 seconds of each minute
      if (time_in_cycle % 1000 == 0) { // Update every second
        displayBatteryPercentage();
      }
    } else if (time_in_cycle == battery_display_duration) {
      // Clear the screen after 10 seconds
      M5Cardputer.Display.fillScreen(bg_color);
    }
    
    if (M5Cardputer.Keyboard.isKeyPressed('`')) {
      charging_mode_active = false;
      drawMenu();
    }
  } else if (in_color_menu) {
    handleColorMenu();
  } else if (update_ssid) {
    if (M5Cardputer.Keyboard.isChange()) {
      if (M5Cardputer.Keyboard.isPressed()) {
        unsigned long current_time = millis();
        if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) && (current_time - last_enter_press > debounce_delay)) {
          last_enter_press = current_time;
          update_ssid = false;
          drawMenu();
        } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
          if (ssid_base.length() > 0) {
            ssid_base.remove(ssid_base.length() - 1);
          }
          displaySSIDInput();
        } else if (M5Cardputer.Keyboard.isKeyPressed('`')) {
          update_ssid = false;
          drawMenu();
        } else {
          bool updated = false;
          for (char c = 32; c <= 126; c++) {
            if (M5Cardputer.Keyboard.isKeyPressed(c) && ssid_base.length() < 20) {
              ssid_base += c;
              updated = true;
              break;
            }
          }
          if (updated) {
            displaySSIDInput();
          }
        }
      }
    }
  } else if (update_num_networks) {
    if (M5Cardputer.Keyboard.isChange()) {
      if (M5Cardputer.Keyboard.isPressed()) {
        unsigned long current_time = millis();
        if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) && (current_time - last_enter_press > debounce_delay)) {
          last_enter_press = current_time;
          update_num_networks = false;
          drawMenu();
        } else if (M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
          num_networks /= 10;
          displayNetworkCountInput();
        } else if (M5Cardputer.Keyboard.isKeyPressed('`')) {
          update_num_networks = false;
          drawMenu();
        } else {
          for (char c = '0'; c <= '9'; c++) {
            if (M5Cardputer.Keyboard.isKeyPressed(c)) {
              int digit = c - '0';
              if (num_networks < 100) {
                num_networks = num_networks * 10 + digit;
              }
              displayNetworkCountInput();
              break;
            }
          }
        }
      }
    }
  } else if (wifi_scan_active) {
    // WiFi scanning is handled in the scanWiFiNetworks function
    // This state is just to prevent other actions while scanning
  } else {
    handleMenu();
    
    if (wifi_spam_active) {
      for (int i = 0; i < num_networks; i++) {
        char ssid[32];
        snprintf(ssid, sizeof(ssid), "%s_%02d", ssid_base.c_str(), i + 1);
        
        packet[10] = packet[16] = i & 0xFF;
        
        int ssid_len = strlen(ssid);
        packet[37] = ssid_len;
        memcpy(&packet[38], ssid, ssid_len);
        
        packet[56] = (i % 13) + 1;

        esp_wifi_80211_tx(WIFI_IF_STA, packet, sizeof(packet), false);
      }
    }
  }
}