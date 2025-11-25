#include <USB.h>
#include <USBHIDKeyboard.h>

USBHIDKeyboard Keyboard;

void setup() {
  USB.begin();
  Keyboard.begin();
  delay(5000);

  // Menekan tombol 'Windows' dan 'R' bersamaan
  Keyboard.press(KEY_LEFT_GUI); // KEY_LEFT_GUI adalah tombol Windows/Command
  Keyboard.press('r');
  delay(100); // Tahan sebentar
  Keyboard.releaseAll(); // Lepas semua tombol
  
  delay(1000); // Tunggu Run Dialog terbuka
  
  // Mengetik "notepad" dan menekan Enter
  Keyboard.println("https://www.youtube.com/watch?v=mN9vQHkbdmI");
    // 1. Tekan tombol Volume Up
  Keyboard.press(HID_KEY_VOLUME_UP);
  
  // 2. Tahan sebentar (simulasi penekanan selama 3000 milidetik)
  delay(3000); 
  Keyboard.releaseAll();
  // 3. Lepas tombol Volume Up
  //Keyboard.release(KEY_MEDIA_VOLUME_UP);
  
  // Tunggu 5 detik sebelum mengulanginya lagi
  delay(5000); 
  // put your main code here, to run repeatedly:
  // Menekan tombol 'Windows' dan 'R' bersamaan
  Keyboard.press(KEY_LEFT_GUI); // KEY_LEFT_GUI adalah tombol Windows/Command
  Keyboard.press('r');
  delay(100); // Tahan sebentar
  Keyboard.releaseAll(); // Lepas semua tombol
  
  delay(1000); // Tunggu Run Dialog terbuka
  
  // Mengetik "notepad" dan menekan Enter
  Keyboard.print("powershell");
  // Tekan dan tahan tombol Ctrl
  Keyboard.press(KEY_LEFT_CTRL);
  // Tekan dan tahan tombol Shift
  Keyboard.press(KEY_LEFT_SHIFT);
  // Tekan tombol Enter
  Keyboard.press(KEY_RETURN);
  
  // Tahan sebentar agar sistem operasi mendeteksinya sebagai satu kombinasi
  delay(100);
  
  // Lepas SEMUA tombol yang ditekan sekaligus
  Keyboard.releaseAll();
  delay(2000);
  Keyboard.press(KEY_LEFT_ARROW);
  delay(100);
  Keyboard.releaseAll();
  delay(100);
  Keyboard.press(KEY_RETURN);
  delay(50);
  Keyboard.releaseAll();
  delay(1000);
  Keyboard.println("ls");
  delay(2000);

}

void loop() {
  Keyboard.press(KEY_LEFT_GUI);
  delay(100); 
  Keyboard.releaseAll();
  delay(100); 
  Keyboard.print("Sound mixer options");
  delay(3000);
  Keyboard.press(KEY_RETURN);
  delay(100);
  Keyboard.releaseAll();
  delay(3000);
  Keyboard.press(KEY_RIGHT_ARROW);
  delay(3000);
  Keyboard.releaseAll();
  delay(3000);
  Keyboard.press(KEY_LEFT_ALT);
  Keyboard.press(KEY_F4);
  delay(100);
  Keyboard.releaseAll();
  delay(300000);
}
