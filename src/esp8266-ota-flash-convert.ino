#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
extern "C" uint8_t system_upgrade_userbin_check();
extern "C" void system_upgrade_flag_set(uint8 flag);
extern "C" void system_upgrade_reboot (void);

#define MAGIC_V1 0xE9
#define MAGIC_V2 0xEA
#define UPGRADE_FLAG_START 0x01
#define UPGRADE_FLAG_FINISH 0x02
#define SECTOR_SIZE 4096
#define BUFFER_SIZE SECTOR_SIZE
#define SPI_FLASH_ADDR 0x40200000

#define FLASH_SUCCESS 0
#define FLASH_FAIL_BAD_URI 1
#define FLASH_FAIL_TOO_SMALL 2
#define FLASH_FAIL_TOO_BIG 3
#define FLASH_FAIL_WRONG_MAGIC 4

#define VERSION "VTRUST-FLASH 1.2\n(c) VTRUST GMBH https://www.vtrust.de/35c3/"
#define WIFI_SSID "vtrust-flash"
#define WIFI_APSSID "vtrust-recovery"

IPAddress ip(10, 42, 42, 42);
IPAddress gateway(10, 42, 42, 42);   
IPAddress subnet(255,255,255,0);   

#define URL_ROM_2 "http://10.42.42.1/files/user2.bin"
#define URL_ROM_3 "http://10.42.42.1/files/thirdparty.bin"

ESP8266WebServer server(80);
HTTPClient client;

char buffer[BUFFER_SIZE] __attribute__((aligned(4))) = {0};
char bootrom[SECTOR_SIZE] __attribute__((aligned(4))) = {0};

uint8_t userspace = system_upgrade_userbin_check();

void handleRoot() {
  // print WiFi diagonistics to Serial
  WiFi.printDiag(Serial);

  // get flash info; size, mode, and speed
  uint32_t flashInfo;
  ESP.flashRead(0, (uint32_t *)&flashInfo, 4);

  const char * FlashSize = "";
  switch((flashInfo >> 28) & 0xF){
    case 0x0:  FlashSize = "512K";  break;
    case 0x1:  FlashSize = "256K";  break;
    case 0x2:  FlashSize = "1M";  break;
    case 0x3:  FlashSize = "2M";  break;
    case 0x4:  FlashSize = "4M";  break;
    case 0x8:  FlashSize = "8M";  break;
    case 0x9:  FlashSize = "16M";  break;
  }

  const char * FlashMode = "";
  switch((flashInfo >> 16) & 0xF){
    case 0:  FlashMode = "QIO";  break;
    case 1:  FlashMode = "QOUT";  break;
    case 2:  FlashMode = "DIO";  break;
    case 3:  FlashMode = "DOUT";  break;
  }

  const char * FlashSpeed = "";
  switch((flashInfo >> 24) & 0xF){
    case 0x0:  FlashSpeed = "40";  break;
    case 0x1:  FlashSpeed = "26";  break;
    case 0x2:  FlashSpeed = "20";  break;
    case 0xF:  FlashSpeed = "80";  break;
  }

  sprintf(buffer,
    VERSION "\n"                               // 61
    "READ FLASH: http://%s/backup" "\n"        // 27
    "ChipID: %x" "\n"                          // 9
    "MAC: %s" "\n"                             // 6
    "BootVersion: %d" "\n"                     // 14
    "BootMode: %s" "\n"                        // 11
    "FlashMode: %s %s @ %sMHz" "\n"            // 19
    "FlashChipId: %x" "\n"                     // 14
    "FlashChipRealSize: %dK" "\n"              // 21
    "Active Userspace: user%s000" "\n",        // 26
    WiFi.localIP().toString().c_str(),         // max 15
    ESP.getChipId(),                           // 6
    WiFi.macAddress().c_str(),                 // 17
    ESP.getBootVersion(),                      // 1
    ESP.getBootMode() ? "normal" : "enhanced", // max 8
    FlashSize, FlashMode, FlashSpeed,          // max 10
    ESP.getFlashChipId(),                      // 6
    ESP.getFlashChipRealSize() / 1024,         // max 4
    userspace ? "2 0x81" : "1 0x01");          // 6

  server.send(200, "text/plain", buffer);
}

void handleFlash2(){
  if (userspace)
  {
    server.send(200, "text/plain", "Device is already booting from userspace 2 (0x81000)\n");
    return;
  }
  else
  {
    const char * url = URL_ROM_2;
    String customUrl;
    if (server.hasArg("url")) {
      customUrl = server.arg("url");
      url = customUrl.c_str();
    }

    uint32_t flash_time = millis();
    int result = downloadRomToFlash(
      false,            //Bootloader is not being updated
      0xEA,             //V2 Espressif Magic
      0x081000,         //Not replacing bootloader
      0x100000,         //Stop before end of ram
      128,              //From middle of flash
      256,              //End of flash
      url               //URL
    );
    flash_time = millis() - flash_time;

    if(result) {
      sprintf(buffer, "Flashing %s to userspace %d failed after %dms, error code %d\n", url, 2 - userspace, flash_time, result);
      server.send(200, "text/plain", buffer);
    } else {
      sprintf(buffer, "Flashed %s to userspace %d successfully in %dms, rebooting...\n", url, 2 - userspace, flash_time);
      server.send(200, "text/plain", buffer);

      system_upgrade_reboot();
    }
  }
}

void handleUndo(){
  sprintf(buffer, "Rebooting into userspace %d\n", 2 - userspace);
  server.send(200, "text/plain", buffer);

  system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
  system_upgrade_reboot();
}

void handleFlash3(){
  if (userspace)
  {
    const char * url = URL_ROM_3;
    String customUrl;
    if (server.hasArg("url")) {
      customUrl = server.arg("url");
      url = customUrl.c_str();
    }

    uint32_t flash_time = millis();
    int result = downloadRomToFlash(
      true,             //Bootloader is being updated
      0xE9,             //Standard Arduino Magic
      0x00000,          //Write to 0x0 since we are replacing the bootloader
      0x80000,          //Stop before 0x80000
      1,                //Erase Sector from 1 to
      128,              //Sector 128 (not inclusive)
      url               //URL
    );
    flash_time = millis() - flash_time;

    if(result) {
      sprintf(buffer, "Flashing %s to userspace %d failed after %dms, error code %d\n", url, 2 - userspace, flash_time, result);
      server.send(200, "text/plain", buffer);
    } else {
      sprintf(buffer, "Flashed %s to userspace %d successfully in %dms, rebooting...\n", url, 2 - userspace, flash_time);
      server.send(200, "text/plain", buffer);

      system_upgrade_reboot();
    }
  }
  else
  {
    server.send(200, "text/plain", "Device is booting from userspace 1 (0x01000) Please flash it to boot from userspace 2 first!\n");
    return;
  }
}


void handleRead(){
  sprintf(buffer, "attachment; filename=\"firmware-%x.bin\"", ESP.getChipId());
  server.sendHeader("Content-Disposition", buffer);
  server.send_P(200, "application/octet-stream", (PGM_P) SPI_FLASH_ADDR, ESP.getFlashChipSize());
}

void setup_webserver(void){
  server.on("/", handleRoot);
  server.on("/flash2", handleFlash2);
  server.on("/flash3", handleFlash3);
  server.on("/undo", handleUndo);
  server.on("/backup", handleRead);
  server.begin();
  Serial.println("HTTP server started");
}

void setup()
{
  // using esp boot loader default baud rate so we don't have to keep switching baud rates to get all the messages
  Serial.begin(74880);
  Serial.println();
  Serial.println(VERSION);
  connectToWiFiBlocking();
  setup_webserver();
}


void softAPsetup()
{
  Serial.println("Setting up SoftAP...");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(ip, gateway , subnet);   // subnet FF FF FF 00
  WiFi.softAP(WIFI_APSSID);
  delay(1000); 
}

void connectToWiFiBlocking()
{
  int x = 0;

  Serial.print("Connecting to Wifi");
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);

  WiFi.begin(WIFI_SSID);
  WiFi.config(ip, gateway , subnet);   // subnet FF FF FF 00

  while (WiFi.status() != WL_CONNECTED)
  {
    x++;
    delay(200);
    Serial.print("."); yield(); // reset watchdog
    if(x>100)
    {
      Serial.println("\nFailed to connect");
      softAPsetup();
      return;
    }
  }
  Serial.printf("\nConnected to %s as %s\n", WIFI_SSID, WiFi.localIP().toString().c_str());
}

//Assumes bootloader must be in first SECTOR_SIZE bytes.
int downloadRomToFlash(bool bootloader, char magic, uint32_t start_address, uint32_t end_address, uint16_t erase_start, uint16_t erase_end, const char * url)
{
  system_upgrade_flag_set(UPGRADE_FLAG_START);

  if(!client.begin(url))
  {
    return FLASH_FAIL_BAD_URI;
  }

  //Response Code Check
  int httpCode = client.GET();
  Serial.printf("HTTP response Code: %d\n", httpCode);
  if(httpCode != HTTP_CODE_OK)
  {
    Serial.println("Invalid response Code");
    return httpCode;
  }

  //Length Check (at least one sector)
  int len = client.getSize();
  Serial.printf("HTTP response length: %d\n", len);
  if(len < SECTOR_SIZE)
  {
    Serial.println("Length too short");
    return FLASH_FAIL_TOO_SMALL;
  }

  if(len > (end_address-start_address))
  {
    Serial.println("Length exceeds flash size");
    return FLASH_FAIL_TOO_BIG;
  }

  //Confirm magic byte
  WiFiClient* stream = client.getStreamPtr();
  char header;
  stream->peekBytes(&header, 1);
  Serial.printf("Magic byte from stream: 0x%02X\n", header);
  if(header != magic)
  {
    Serial.println("Invalid magic byte");
    return FLASH_FAIL_WRONG_MAGIC;
  }

  if(bootloader)
  { 
    Serial.printf("Downloading %d byte bootloader", sizeof(bootrom));
    while(stream->available() < sizeof(bootrom))
    {
      Serial.print("."); yield(); // reset watchdog
    }
    int c = stream->readBytes(bootrom, sizeof(bootrom));
    start_address += c; // increment next write address
    len -= c; // decrement remaining bytes
    Serial.println("Done");
  }

  Serial.printf("Erasing flash sectors %d-%d", erase_start, erase_end);
  for (uint16_t i = erase_start; i < erase_end; i++)
  {
    ESP.flashEraseSector(i);
    Serial.print("."); yield(); // reset watchdog
  }  
  Serial.println("Done");
  
  Serial.printf("Downloading rom to 0x%06X-0x%06X in %d byte blocks", start_address, start_address+len, sizeof(buffer));
  while(len > 0)
  {
    size_t size = stream->available();
    if(size >= sizeof(buffer) || size == len) 
    {
      int c = stream->readBytes(buffer, size > sizeof(buffer) ? sizeof(buffer) : size);
      ESP.flashWrite(start_address, (uint32_t*)buffer, c);
      start_address += c; // increment next write address
      len -= c; // decrement remaining bytes
    }
    Serial.print("."); yield(); // reset watchdog
  }
  client.end();
  Serial.println("Done");

  if(bootloader)
  {
    Serial.println("Erasing bootloader sector 0");
    ESP.flashEraseSector(0);
    Serial.println("Done");
    
    Serial.printf("Writing bootloader to 0x%06X-0x%06X", 0, SECTOR_SIZE);
    ESP.flashWrite(0, (uint32_t*)bootrom, SECTOR_SIZE);
    Serial.println("Done");

    // erase tasmota param area [244-253) and system params [253-256)
    for (uint16_t i = 244; i < 256; i++)
    {
      ESP.flashEraseSector(i);
      yield(); // reset watchdog
    }
  }

  system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
  return FLASH_SUCCESS;
}



void loop()
{
  server.handleClient();
}
