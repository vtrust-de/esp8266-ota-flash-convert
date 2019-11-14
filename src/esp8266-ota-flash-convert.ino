#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <eboot_command.h>
extern "C" uint8_t system_upgrade_userbin_check();
extern "C" void system_upgrade_flag_set(uint8 flag);
extern "C" void system_upgrade_reboot (void);

#define UPGRADE_FLAG_START 0x01
#define UPGRADE_FLAG_FINISH 0x02
#define SECTOR_SIZE 0x1000
#define MAX_FILE_SIZE 0x80000
#define SPI_FLASH_ADDR 0x40200000

#define FLASH_SUCCESS 0
#define FLASH_FAIL_BAD_URI 1
#define FLASH_FAIL_TOO_SMALL 2
#define FLASH_FAIL_TOO_BIG 3
#define FLASH_FAIL_WRONG_MAGIC 4
#define FLASH_FAIL_BAD_ERASE 5
#define FLASH_FAIL_BAD_WRITE 6
#define FLASH_FAIL_BOOTROM_ERASE 7
#define FLASH_FAIL_BOOTROM_WRITE 8
#define FLASH_FAIL_CONFIG_ERASE 9

const char * FLASH_ERROR_CODES[] = {
  "success",
  "bad URI",
  "too small",
  "too big",
  "wrong magic",
  "erase failed",
  "write failed",
  "bootrom erase failed (very bad)",
  "bootrom write failed (very, very bad)",
  "config erase failed"
};

#define VERSION "VTRUST-FLASH 1.3\n(c) VTRUST GMBH https://www.vtrust.de/35c3/"
#define WIFI_SSID "vtrust-flash"
#define WIFI_APSSID "vtrust-recovery"

IPAddress ip(10, 42, 42, 42);
IPAddress gateway(10, 42, 42, 42);   
IPAddress subnet(255,255,255,0);   

#define ROM_URL "http://10.42.42.1/files/thirdparty.bin"

ESP8266WebServer server(80);
HTTPClient client;

char buffer[SECTOR_SIZE] __attribute__((aligned(4))) = {0};
char bootrom[SECTOR_SIZE] __attribute__((aligned(4))) = {0};

uint8_t userspace = system_upgrade_userbin_check();

void handleRoot() {
  // print WiFi diagonistics to Serial
  WiFi.printDiag(Serial);

  // get flash info; size, mode, and speed
  uint32_t flashInfo;
  ESP.flashRead(0, &flashInfo, 4);

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

void handleFlash(){
  uint32_t flash_time = millis();
  String url = server.hasArg("url") ? server.arg("url") : ROM_URL;
  // we can't flash over the currently running program
  // determine where to place the new rom based on which userspace we're in
  int result = downloadRomToFlash(userspace ? 0x00000 : 0x80000 - 0x5000, url);
  // clean up the HTTPClient after use, regardless of result
  client.end();
  flash_time = millis() - flash_time;

  if (result) {
    if (result > FLASH_FAIL_CONFIG_ERASE || result < 0)
      sprintf(buffer, "Flashing %s failed after %dms, HTTP %d\n", url.c_str(), flash_time, result);
    else
      sprintf(buffer, "Flashing %s failed after %dms, %s\n", url.c_str(), flash_time, FLASH_ERROR_CODES[result]);
    server.send(200, "text/plain", buffer);
  } else {
    sprintf(buffer, "Flashed %s successfully in %dms, rebooting...\n", url.c_str(), flash_time);
    server.send(200, "text/plain", buffer);

    ESP.restart();
  }
}

void handleUndo(){
  sprintf(buffer, "Rebooting into userspace %d\n", 2 - userspace);
  server.send(200, "text/plain", buffer);

  system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
  system_upgrade_reboot();
}

void handleRead(){
  sprintf(buffer, "attachment; filename=\"firmware-%x.bin\"", ESP.getChipId());
  server.sendHeader("Content-Disposition", buffer);
  server.send_P(200, "application/octet-stream", (PGM_P) SPI_FLASH_ADDR, ESP.getFlashChipSize());
}

void setup_webserver(void){
  server.on("/", handleRoot);
  server.on("/flash", handleFlash);
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
int downloadRomToFlash(const uint32_t start_address, const String &url){
  if(!client.begin(url))
    return FLASH_FAIL_BAD_URI;

  //Response Code Check
  int httpCode = client.GET();
  if(httpCode != HTTP_CODE_OK)
    return httpCode;

  //Length Check (at least one sector)
  int fileSize = client.getSize();
  if(fileSize < SECTOR_SIZE)
    return FLASH_FAIL_TOO_SMALL;

  if(fileSize > MAX_FILE_SIZE)
    return FLASH_FAIL_TOO_BIG;

  //Confirm magic byte
  WiFiClient* stream = client.getStreamPtr();
  uint8_t magic;
  stream->peekBytes(&magic, 1);
  if(magic != 0xE9)
    return FLASH_FAIL_WRONG_MAGIC;

  Serial.printf("Downloading %d byte bootloader", sizeof(bootrom));
  while(stream->available() < sizeof(bootrom))
  {
    Serial.print("."); yield(); // reset watchdog
  }
  Serial.println();
  stream->readBytes(bootrom, sizeof(bootrom));
  
  // we do not want to overwrite the bootrom just yet
  // only write the first sector if start_address > 0
  if(start_address){
    if(!ESP.flashEraseSector(start_address >> 12))
      return FLASH_FAIL_BAD_ERASE;
    if(!ESP.flashWrite(start_address, (uint32_t*)bootrom, SECTOR_SIZE))
      return FLASH_FAIL_BAD_WRITE;
  }

  uint32_t write_address = start_address + SECTOR_SIZE;
  int remaining = fileSize - SECTOR_SIZE;
  
  while(remaining > 0)
  {
    size_t size = stream->available();
    if(size >= sizeof(buffer) || size == remaining) 
    {
      int c = stream->readBytes(buffer, size > sizeof(buffer) ? sizeof(buffer) : size);
      if(!ESP.flashEraseSector(write_address >> 12))
        return FLASH_FAIL_BAD_ERASE;
      if(!ESP.flashWrite(write_address, (uint32_t*)buffer, c))
        return FLASH_FAIL_BAD_WRITE;

      write_address += c; // increment next write address
      remaining -= c; // decrement remaining bytes
    }
    Serial.print("."); yield(); // reset watchdog
  }
  Serial.println();

  // clearing system param area helps avoid issues in your thirdparty firmware
  if(!ESP.eraseConfig())
    return FLASH_FAIL_CONFIG_ERASE;

  // now that we have gotten this far without failure we can write the bootrom
  // we do this at 0 regardless of start_address
  Serial.println("Writing bootloader");
  if(!ESP.flashEraseSector(0))
    return FLASH_FAIL_BOOTROM_ERASE;
  if(!ESP.flashWrite(0, (uint32_t*) bootrom, SECTOR_SIZE))
    return FLASH_FAIL_BOOTROM_WRITE;

  // if the firmware was written anywhere but 0, copy it back into place on next boot
  if(start_address) {
    eboot_command ebcmd;
    ebcmd.action = ACTION_COPY_RAW;
    ebcmd.args[0] = start_address;
    ebcmd.args[1] = 0x00000;
    ebcmd.args[2] = fileSize;
    eboot_command_write(&ebcmd);
  }

  return FLASH_SUCCESS;
}



void loop()
{
  server.handleClient();
}
