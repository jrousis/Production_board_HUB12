// Visual Micro is in vMicro>General>Tutorial Mode
// 
/*
    Name:       web_GUI_messager_HUB12.ino
    Created:	27/8/2024 8:26:37 μμ
    Author:     ROUSIS_FACTORY\user

    // Refer the file "cricket_64X64.ino" for the complete code
    // This is the main file for the project
*/

#include <EEPROM.h>
#include <DNSServer.h>
#include <ESPUI.h>
#include <WiFi.h>
#include "time.h"
#include <Ds1302.h>
#include <DMD32_B.h>        //
#include "fonts/SystemFont5x7.h"
#include "fonts/scoreboard_12.h"
#include "fonts/Big_font.h"

//Fire up the DMD library as dmd
//#define DISPLAYS_ACROSS 1
//#define DISPLAYS_DOWN 2
#define DISPLAYS_ACROSS 4
#define DISPLAYS_DOWN 2
DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN, PROTOCOL_QIANGLI);
#define PIXELS_X (DISPLAYS_ACROSS * 32)
#define PIXELS_Y (DISPLAYS_DOWN * 16)

//EEPROM data ADDRESSER
#define EEP_WIFI_SSID 0            //32 bytes for WiFi SSID
#define EEP_WIFI_PASS 32           //32 bytes for WiFi Password
#define EEP_USER_LOGIN 128         //32 bytes for User name
#define EEP_USER_PASS 160          //32 bytes for User password
#define EEP_SENSOR 196             //1 byte for photo sensor on/off
#define EEP_DEFAULT_LOGIN 197      //1 byte for default login on/off
#define EEP_DEFAULT_WiFi 198       //1 byte for default WiFi on/off
#define EEP_PRIGHTNESS 199         //1 byte for brightness
#define EEP_TOTAL_PRICES 200       //1 byte for total prices
#define EEP_ADDRESS 201            //1 byte for address
#define EEP_SELECTED_FUNCTION 202      //1 byte for selected function
#define PRODUCT_LABEL_1A 203 //10 bytes
#define PRODUCT_LABEL_1B 213 //10 bytes
#define PRODUCT_LABEL_2A 223 //10 bytes
#define PRODUCT_LABEL_2B 233 //10 bytes
#define PRODUCT_LABEL_3A 243 //10 bytes
#define PRODUCT_LABEL_3B 253 //10 bytes
#define PRODUCT_PRICE_1A 263 //2 bytes
#define PRODUCT_PRICE_1B 265 //2 bytes

#define SAMPLES_MAX 60

#define LED_PIN 15
#define OPEN_HOT_SPOT 36
//--------------------------------------------------------------------------------------------
uint8_t Sensor_on;
uint8_t Default_login;
int8_t samples_metter = -1;
uint16_t Timer_devide_photo = 0;
uint8_t Brightness;
uint8_t Total_prices;
uint8_t IR_toggle;

uint16_t photosensor;
uint16_t button1;
uint16_t status;
uint16_t WiFiStatus;
uint16_t mainTime;

volatile bool allowTimeSet = false;
static uint32_t last_update = 0;

const char* soft_ID = "Rousis Systems LTD\nProduction_board_V2.1";
//--------------------------------------------------------------------------------------------
// DS1302 RTC instance
Ds1302 rtc(4, 32, 2);
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0; // 7200;
const int   daylightOffset_sec = 0; // 3600;

hw_timer_t* scan_timer = NULL;
bool Scan = false;
portMUX_TYPE Scan_timerMux = portMUX_INITIALIZER_UNLOCKED;
void IRAM_ATTR triggerScan()
{
    Scan = true;
    portENTER_CRITICAL_ISR(&Scan_timerMux);

    dmd.scanDisplayBySPI();

    portEXIT_CRITICAL_ISR(&Scan_timerMux);
    Scan = false;
}
//---------------------------------------------------------------------------------------------
// GUI settings 
//----------------------------------------------------------------------------------
#define SAMPLES_MAX 60
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;

const char* ssid = "Rousis_Hotspot";
const char* password = "jea899hphcbk";
const char* hostname = "espui";

uint16_t password_text, user_text;
uint16_t wifi_ssid_text, wifi_pass_text;

String stored_user, stored_password;
String stored_ssid, stored_pass;
uint8_t photo_samples[60];
//----------------------------------------------------------------------------------------
// Potentiometer is connected to GPIO 34 (Analog ADC1_CH6) 
const int photoPin = 36;

// variable for storing the potentiometer value
int photoValue = 0;

//----------------------------------------------------------------------------------------
void writeString(char add, String data, uint8_t length)
{
    int _size = length; // data.length();
    int i;
    for (i = 0; i < _size; i++)
    {
        EEPROM.write(add + i, data[i]);
    }
    EEPROM.write(add + _size, '\0');   //Add termination null character for String Data
    EEPROM.commit();
}

String read_String(char add, uint8_t length)
{
    int i;
    char data[100]; //Max 100 Bytes
    int len = 0;
    unsigned char k;
    k = EEPROM.read(add);
    while (k != '\0' && len < length)   //Read until null character
    {
        k = EEPROM.read(add + len);
        data[len] = k;
        len++;
    }
    data[len] = '\0';
    return String(data);
}
//----------------------------------------------------------------------------------------
void numberCall(Control* sender, int type) {
    Serial.println("Instruction Nn:");
    Serial.println(sender->label);
    Serial.println("Type:");
    Serial.println(type);
    timerAlarmDisable(scan_timer);

    if (sender->label == "Price address") {

    }
    //Total+
    timerAlarmEnable(scan_timer);
}

void textCall(Control* sender, int type) {
    Serial.print("Text: ID: ");
    Serial.print(sender->id);
    Serial.print(", Value: ");
    Serial.println(sender->value);
}

void slider(Control* sender, int type) {
    Serial.print("Slider: ID: ");
    Serial.print(sender->id);
    Serial.print(", Value: ");
    Serial.println(sender->value);
    Brightness = (sender->value.toInt()) & 0xff;
    dmd.setBrightness(Brightness);
    timerAlarmDisable(scan_timer);
    EEPROM.write(EEP_PRIGHTNESS, Brightness);
    EEPROM.commit();

    timerAlarmEnable(scan_timer);
}

void ButtonsScoreCallback(Control* sender, int type) {

    switch (type) {
    case B_DOWN:
        break;

    case B_UP:
        //.println("Button UP");
        break;
    }
}

void ClearCallback(Control* sender, int type) {
    switch (type) {
    case B_DOWN:
        Serial.println("Button DOWN");

        break;
    case B_UP:
        Serial.println("Button UP");
        break;
    }
}


void enterWifiDetailsCallback(Control* sender, int type) {
    if (type == B_UP) {
        if (sender->label == "WiFi")
        {
            Serial.println("Saving credentials to EPROM...");
            Serial.println(ESPUI.getControl(wifi_ssid_text)->value);
            Serial.println(ESPUI.getControl(wifi_pass_text)->value);
            unsigned int i;
            timerAlarmDisable(scan_timer);
            for (i = 0; i < ESPUI.getControl(wifi_ssid_text)->value.length(); i++) {
                EEPROM.write(i, ESPUI.getControl(wifi_ssid_text)->value.charAt(i));
                if (i == 30) break; //Even though we provided a max length, user input should never be trusted
            }
            EEPROM.write(i, '\0');

            for (i = 0; i < ESPUI.getControl(wifi_pass_text)->value.length(); i++) {
                EEPROM.write(i + EEP_WIFI_PASS, ESPUI.getControl(wifi_pass_text)->value.charAt(i));
                if (i == 94) break; //Even though we provided a max length, user input should never be trusted
            }
            EEPROM.write(i + EEP_WIFI_PASS, '\0');

            ESPUI.getControl(status)->value = "Saved credentials to EEPROM<br>SSID: "
                + ESPUI.getControl(wifi_ssid_text)->value
                + "<br>Password: " + ESPUI.getControl(wifi_pass_text)->value;
            ESPUI.updateControl(status);

        }
        else if (sender->label == "User") {
            Serial.println("Saving User to EPROM...");
            Serial.println(ESPUI.getControl(user_text)->value);
            Serial.println(ESPUI.getControl(password_text)->value);
            unsigned int i;
            for (i = 0; i < ESPUI.getControl(user_text)->value.length(); i++) {
                EEPROM.write(i + EEP_USER_LOGIN, ESPUI.getControl(user_text)->value.charAt(i));
                if (i == 30) break; //Even though we provided a max length, user input should never be trusted
            }
            EEPROM.write(i + EEP_USER_LOGIN, '\0');

            for (i = 0; i < ESPUI.getControl(password_text)->value.length(); i++) {
                EEPROM.write(i + EEP_USER_PASS, ESPUI.getControl(password_text)->value.charAt(i));
                if (i == 30) break; //Even though we provided a max length, user input should never be trusted
            }
            EEPROM.write(i + EEP_USER_PASS, '\0');

            ESPUI.getControl(status)->value = "Saved login details to EEPROM<br>User name: "
                + ESPUI.getControl(user_text)->value
                + "<br>Password: " + ESPUI.getControl(password_text)->value;
            ESPUI.updateControl(status);

            Default_login = 0;
            EEPROM.write(EEP_DEFAULT_LOGIN, Default_login);
        }
        EEPROM.commit();
        //timerAlarmEnable(scan_timer);
        /*Serial.println("Reset..");
        ESP.restart();*/
    }
}


void ReadWiFiCrententials(void) {
    uint8_t DefaultWiFi = EEPROM.read(EEP_DEFAULT_WiFi);
    if (DefaultWiFi) {
        Serial.println("Load default WiFi Crententials..");
        unsigned int i;
        stored_ssid = "Rousis";
        stored_pass = "rousis074520198";
        for (i = 0; i < stored_ssid.length(); i++) {
            EEPROM.write(i, stored_ssid.charAt(i));
            if (i == 30) break; //Even though we provided a max length, user input should never be trusted
        }
        EEPROM.write(i, '\0');

        for (i = 0; i < stored_pass.length(); i++) {
            EEPROM.write(i + EEP_WIFI_PASS, stored_pass.charAt(i));
            if (i == 94) break; //Even though we provided a max length, user input should never be trusted
        }
        EEPROM.write(i + EEP_WIFI_PASS, '\0');
        EEPROM.write(EEP_DEFAULT_WiFi, 0); //Erase the EEP_DEFAULT_WiFi
    }
    else {
        stored_ssid = read_String(EEP_WIFI_SSID, 32);
        stored_pass = read_String(EEP_WIFI_PASS, 32);
    }
}

void Readuserdetails(void) {
    if (Default_login) {
        stored_user = "espboard";
        stored_password = "12345678";
    }
    else {
        stored_user = read_String(EEP_USER_LOGIN, 32);
        stored_password = read_String(EEP_USER_PASS, 32);
    }

    /*readStringFromEEPROM(stored_user, 128, 32);
    readStringFromEEPROM(stored_password, 160, 32);*/
}

void ReadSettingEeprom() {
    Serial.println("Rad Settings EEPROM: ");
    Sensor_on = EEPROM.read(EEP_SENSOR);
    if (Sensor_on > 1) { Sensor_on = 1; }
    Serial.print("Photo sensor= ");
    Serial.println(Sensor_on, HEX);
    Default_login = EEPROM.read(EEP_DEFAULT_LOGIN);
    Serial.print("Default login= ");
    Serial.println(Default_login, HEX);
    Brightness = EEPROM.read(EEP_PRIGHTNESS);
    Serial.print("Brightness= ");
    Serial.println(Brightness);
    Serial.println("-------------------------------");
}

void ResetCallback(Control* sender, int type) {
    if (type == B_UP) {
        Serial.println("Reset..");
        ESP.restart();
    }
}

void textCallback(Control* sender, int type) {
    //This callback is needed to handle the changed values, even though it doesn't do anything itself.
}

//Most elements in this test UI are assigned this generic callback which prints some
//basic information. Event types are defined in ESPUI.h
void generalCallback(Control* sender, int type) {
    // Handle time control first and exit early
    if (sender->label == "Set time") {
        if (!allowTimeSet) return;  // Ignore auto updates
        allowTimeSet = false;       // Consume one update

        Serial.println("Trying to fix the local time - Set the local RTC..");
        timerAlarmDisable(scan_timer);

        int year, month, day, hour, minute, second;
        float milliseconds;
        sscanf(sender->value.c_str(), "%d-%d-%dT%d:%d:%d.%fZ",
            &year, &month, &day, &hour, &minute, &second, &milliseconds);

        setTimezone("GMT0");
        setTime(year, month, day, hour, minute, second, 1);
        SetExtRTC();
        setTimezone("EET-2EEST,M3.5.0/3,M10.5.0/4");

        timerAlarmEnable(scan_timer);
        return;
    }

    Serial.print("CB: id(");
    Serial.print(sender->id);
    Serial.print(") Type(");
    Serial.print(type);
    Serial.print(") '");
    Serial.print(sender->label);
    Serial.print("' = ");
    Serial.println(sender->value);G:\My Drive\C_Projects\arduino\esp32\Rousis LTD projects\P10_HUB12_APPS\Production_board_HUB12\README.md
    
    if (EEPROM.read(EEP_SELECTED_FUNCTION) != 0) {
        if (sender->label == "Total")
        {
            int a = sender->value.toInt();
            char buf[16] = { 0 };
            // convert integer to string format "0.000.000"
            sprintf(buf, "%d", a);
            int len = strlen(buf);
            int insertPos = len;

            // Προσθήκη τελείας κάθε 3 ψηφία, εκτός από το τέλος
            for (int i = len; i > 0; i -= 3) {
                if (i != len) {
                    memmove(buf + i + 1, buf + i, insertPos - i + 1);
                    buf[i] = '.';
                    insertPos++;
                }
            }

            // Αφαίρεση της τελευταίας τελείας αν υπάρχει
            if (buf[insertPos - 1] == '.') {
                buf[insertPos - 1] = '\0';
            }

            dmd.selectFont(scoreboard_12);
            dmd.drawString(36, 2, "          ", 10, GRAPHICS_NORMAL, 1);
            dmd.drawString(36, 2, buf, 10, GRAPHICS_NORMAL, 1);
        }
        else if (sender->label == "Label1A")
        {
            dmd.selectFont(SystemFont5x7);
            dmd.drawString(0, 0, "      ", 6, GRAPHICS_NORMAL, 1);
            dmd.drawString(0, 0, sender->value.c_str(), 6, GRAPHICS_NORMAL, 1);
            //save the value to EEPROM
            //stop scan timer while saving to epprom always
            timerAlarmDisable(scan_timer);
            writeString(PRODUCT_LABEL_1A, sender->value, 8);
            timerAlarmEnable(scan_timer);

        }
        else if (sender->label == "Label1B")
        {
            dmd.selectFont(SystemFont5x7);
            dmd.drawString(0, 8, "      ", 6, GRAPHICS_NORMAL, 1);
            dmd.drawString(0, 8, sender->value.c_str(), 6, GRAPHICS_NORMAL, 1);
            //save the value to EEPROM
            //stop scan timer while saving to epprom always
            timerAlarmDisable(scan_timer);
            writeString(PRODUCT_LABEL_1B, sender->value, 8);
            timerAlarmEnable(scan_timer);

        }
    }
	
    if (sender->label == "Auto Sensor Brigntness")
    {
        Sensor_on = sender->value.toInt();
        timerAlarmDisable(scan_timer);
        EEPROM.write(EEP_SENSOR, Sensor_on);
        EEPROM.commit();
        timerAlarmEnable(scan_timer);
        samples_metter = -1;
    }
    else if (sender->label == "Function")
    {
        EEPROM.write(EEP_SELECTED_FUNCTION, sender->value.toInt());
        timerAlarmDisable(scan_timer);
        EEPROM.commit();
		timerAlarmEnable(scan_timer);
    }  
  //  else if (sender->label == "Set time") {
  //      if (!allowTimeSet) return;  // Ignore auto updates
  //      allowTimeSet = false;       // Consume one update
  //      // Ανάλυση του ISO 8601 string

		//Serial.println("Trying to fix the local time - Set the local RTC..");
  //      timerAlarmDisable(scan_timer);
  //      int year, month, day, hour, minute, second;
  //      float milliseconds;
  //      sscanf(sender->value.c_str(), "%d-%d-%dT%d:%d:%d.%fZ", &year, &month, &day, &hour, &minute, &second, &milliseconds);

  //      struct tm tm;
  //      tm.tm_year = year - 1900; // Το έτος πρέπει να είναι έτος από 1900.
  //      tm.tm_mon = month - 1; // Οι μήνες ξεκινούν από το 0 (Ιανουάριος).
  //      tm.tm_mday = day;
  //      tm.tm_hour = hour;
  //      tm.tm_min = minute;
  //      tm.tm_sec = second;
  //      // Ρύθμιση του χρόνου και της ημερομηνίας
  //      Serial.println("Trying to fix the local time - Set the local RTC..");
  //      Serial.println(&tm, "%H:%M:%S");
  //      //setTime(2021, 3, 28, 1, 59, 50, 1);
  //      setTimezone("GMT0");
  //      setTime(year, month, day, hour, minute, second, 1);
  //      SetExtRTC();
  //      setTimezone("EET-2EEST,M3.5.0/3,M10.5.0/4");
  //      timerAlarmEnable(scan_timer);
  //  }
}

void getTimeCallback(Control* sender, int type) {
    //if (type == B_UP) {
    //    //reset the mainTime callback
    //    Serial.println("Button pressed - syncing time with system");

    //    ESPUI.getControl(mainTime)->callback = generalCallback;
    //    //update the mainTime callback

    //    ESPUI.updateTime(mainTime);
		
    //}

    if (type == B_UP) {
        Serial.println("Button pressed - syncing time with system");
        allowTimeSet = true;
        ESPUI.updateTime(mainTime);
    }
}

void readStringFromEEPROM(String& buf, int baseaddress, int size) {
    buf.reserve(size);
    for (int i = baseaddress; i < baseaddress + size; i++) {
        char c = EEPROM.read(i);
        buf += c;
        if (!c) break;
    }
}

void IPdisplay() {

    char Displaybuf[7];
    IPAddress localIp;
    char strLocalIp[20] = {0};

    if (WiFi.getMode() == WIFI_AP) {
        memcpy(Displaybuf, "AP Display:", 11);
        localIp = WiFi.softAPIP();
    }
    else {
        memcpy(Displaybuf, "IP Display:", 11);
        localIp = WiFi.localIP();        
    }
    //IPAddress to char array
    sprintf(strLocalIp, "%d.%d.%d.%d", localIp[0], localIp[1], localIp[2], localIp[3]);


    dmd.selectFont(SystemFont5x7);
    dmd.clearScreen(true);
    dmd.drawString(0, 0, Displaybuf, 11, GRAPHICS_NORMAL, 1);
    //display IP address
    Serial.print("IP Address to display: ");
    Serial.println(strLocalIp);
    dmd.drawString(0, 8, strLocalIp, 15, GRAPHICS_NORMAL, 1);
        
    
    /*myLED.print(Displaybuf, INVERT_DISPLAY);*/
    delay(3000);
}

void photo_sample(void) {
    // Reading potentiometer value
    if (!Sensor_on) { return; }

    photoValue = analogRead(photoPin);
    if (photoValue < 200) { photoValue = 1; }
    photoValue = (4095 - photoValue);
    photoValue = 0.06 * photoValue; //(255 / 4095) = 0.06
    if (photoValue > 240) { photoValue = 255; }
    Brightness = photoValue;
    if (Brightness < 20) { Brightness = 20; }

    if (samples_metter == -1) {
        samples_metter = 0;
        //myLED.displayBrightness(Brightness);
        Serial.print("First Sensor value = ");
        Serial.println(Brightness);
    }
    photo_samples[samples_metter] = Brightness;
    samples_metter++;

    /*Serial.print("Sample metter nummber = ");
    Serial.println(samples_metter);
    Serial.print("Sensor value =  ");
    Serial.println(Brightness);*/

    if (samples_metter >= SAMPLES_MAX)
    {
        uint16_t A = 0;
        uint8_t i;
        for (i = 0; i < SAMPLES_MAX; i++) { A += photo_samples[i]; }
        samples_metter = 0;
        Brightness = A / SAMPLES_MAX;
        //uint8_t digits = eeprom_read_byte((uint8_t*)FAV_eep + DISPLAY_DIGITS);
        //TLC_config_byte(sram_brigt,digits);
        //myLED.displayBrightness(Brightness);
        Serial.print("Average Sensor value to brightness = ");
        Serial.println(Brightness);
    }
}

// The setup() function runs once each time the micro-controller starts
void setup()
{
    // Initialize the serial port
    Serial.begin(115200);
    delay(50);
	while (!Serial) {
		; // wait for serial port to connect. Needed for native USB port only
	}
	//Print the file name and the date and time of compilation
	Serial.println();
	Serial.println("File: " __FILE__);
	Serial.println("Compiled on: " __DATE__ " at " __TIME__);
	Serial.println(soft_ID); 
    Serial.println();

    pinMode(OPEN_HOT_SPOT, INPUT_PULLUP);
    pinMode(OPEN_HOT_SPOT, INPUT);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    rtc.init();

    
    EEPROM.begin(256);
    ReadSettingEeprom();

    //read the stored brightness value
    Brightness = EEPROM.read(EEP_PRIGHTNESS);

    // return the clock speed of the CPU
    uint8_t cpuClock = ESP.getCpuFreqMHz();
    scan_timer = timerBegin(0, cpuClock, true);
    timerAttachInterrupt(scan_timer, &triggerScan, true);
    timerAlarmWrite(scan_timer, 300, true);
    timerAlarmDisable(scan_timer);
    //-----------------------------------------------------------------------
    // GUI interface setup
    WiFi.setHostname(hostname);
    ReadWiFiCrententials();

    Serial.println("Stored SSID:");
    Serial.println(stored_ssid);
    Serial.println("Stored password:");
    Serial.println(stored_pass);
    // try to connect to existing network

    WiFi.begin(stored_ssid.c_str(), stored_pass.c_str());
    //WiFi.begin(ssid, password);
    Serial.print("\n\nTry to connect to existing network");

    uint8_t timeout = 10;

    // Wait for connection, 5s timeout
    do {
        delay(500);
        Serial.print(".");
        timeout--;
    } while (timeout && WiFi.status() != WL_CONNECTED);

    // not connected -> create hotspot
    if (WiFi.status() != WL_CONNECTED || digitalRead(OPEN_HOT_SPOT) == 0) {
        Serial.print("\n\nCreating hotspot");

        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
        WiFi.softAP(ssid);
        timeout = 5;

        do {
            delay(500);
            Serial.print(".");
            timeout--;
        } while (timeout);
    }

    dnsServer.start(DNS_PORT, "*", apIP);

    Serial.println("\n\nWiFi parameters:");
    Serial.print("Mode: ");
    Serial.println(WiFi.getMode() == WIFI_AP ? "Station" : "Client");
    Serial.print("IP address: ");
    Serial.println(WiFi.getMode() == WIFI_AP ? WiFi.softAPIP() : WiFi.localIP());

    Serial.println("WiFi mode:");
    Serial.println(WiFi.getMode());

    timerAlarmEnable(scan_timer);
    //timerAlarmEnable(timer);

    IPdisplay();

    //---------------------------------------------------------------------------------------------
        //}
    uint16_t tab1 = ESPUI.addControl(ControlType::Tab, "Main Display", "Main Display");
    uint16_t tab2 = ESPUI.addControl(ControlType::Tab, "User Settings", "User");
    uint16_t tab3 = ESPUI.addControl(ControlType::Tab, "WiFi Settings", "WiFi");

	// Make a Select control with 2 options, and assign the general callback to it. The callback will print the selected value to the serial monitor
	uint8_t selected_func = EEPROM.read(EEP_SELECTED_FUNCTION);
	uint16_t select_function = ESPUI.addControl(Select, "Function", "0", None, tab1, generalCallback); 
	if (selected_func == 0) {
		ESPUI.getControl(select_function)->value = "0";
	}
	else if (selected_func > 0) {
		ESPUI.getControl(select_function)->value = "1";
	}
	ESPUI.addControl(Option, "LOT", "0", None, select_function);
	ESPUI.addControl(Option, "Production", "1", None, select_function);


    String strbuf = read_String(PRODUCT_LABEL_1A, 6);
    //check if the product label is empty or not ascii and then set "-" to the label
    if (strbuf[0] < 32 || strbuf[0] > 126) { strbuf = "-"; }
    uint16_t maintext = ESPUI.addControl(Text, "Label1A", strbuf, Sunflower, tab1, generalCallback);
    ESPUI.addControl(Max, "", "8", None, maintext);
    strbuf = read_String(PRODUCT_LABEL_1B, 6);
    if (strbuf[0] < 32 || strbuf[0] > 126) { strbuf = "-"; }
    maintext = ESPUI.addControl(Text, "Label1B", strbuf, Sunflower, tab1, generalCallback);
    ESPUI.addControl(Max, "", "8", None, maintext);


    uint16_t mainNumber = ESPUI.addControl(Number, "Total", "0", Emerald, tab1, generalCallback);
    ESPUI.addControl(Min, "0", "0", None, mainNumber);
    ESPUI.addControl(Max, "9999999", "9999999", None, mainNumber);

    mainNumber = ESPUI.addControl(Number, "Product 1", "0", Sunflower, tab1, generalCallback);
    ESPUI.addControl(Min, "0", "0", None, mainNumber);
    ESPUI.addControl(Max, "10", "99", None, mainNumber);

    mainNumber = ESPUI.addControl(Number, "Product 2", "0", Sunflower, tab1, generalCallback);
    ESPUI.addControl(Min, "0", "0", None, mainNumber);
    ESPUI.addControl(Max, "99", "99", None, mainNumber);

    mainNumber = ESPUI.addControl(Number, "Product 3", "0", Sunflower, tab1, generalCallback);
    ESPUI.addControl(Min, "0", "0", None, mainNumber);
    ESPUI.addControl(Max, "999", "999", None, mainNumber);

    ESPUI.addControl(Separator, "Other Options", "", None, tab1);

    photosensor = ESPUI.addControl(Switcher, "Auto Sensor Brigntness", "0", Dark, tab1, generalCallback);
    ESPUI.getControl(photosensor)->value = String(Sensor_on);

    button1 = ESPUI.addControl(ControlType::Button, "Push Button", "Clear All", ControlColor::Carrot, tab1, &ClearCallback);

    ESPUI.addControl(Button, "Synchronize Clock with your time", "Set", Carrot, tab1, getTimeCallback);
    // Add this near your other controls in setup()
    mainTime = ESPUI.addControl(Time, "Set time", "Set_Time", None, tab1, generalCallback);


    uint16_t mainSlider = ESPUI.addControl(ControlType::Slider, "Brightness", "255", ControlColor::Alizarin, tab1, &slider);
    ESPUI.addControl(Min, "", "0", None, mainSlider);
    ESPUI.addControl(Max, "", "255", None, mainSlider);
    ESPUI.getControl(mainSlider)->value = String(Brightness);

    Readuserdetails();
    user_text = ESPUI.addControl(Text, "User name", stored_user, Alizarin, tab2, textCallback); // stored_user
    ESPUI.addControl(Max, "", "32", None, user_text);
    password_text = ESPUI.addControl(Text, "Password", stored_password, Alizarin, tab2, textCallback); //stored_password
    ESPUI.addControl(Max, "", "32", None, password_text);
    ESPUI.setInputType(password_text, "password");
    ESPUI.addControl(Button, "User", "Save", Peterriver, tab2, enterWifiDetailsCallback);

    wifi_ssid_text = ESPUI.addControl(Text, "SSID", stored_ssid, Alizarin, tab3, textCallback); // stored_ssid
    ESPUI.addControl(Max, "", "32", None, wifi_ssid_text);
    wifi_pass_text = ESPUI.addControl(Text, "Password", stored_pass, Alizarin, tab3, textCallback); //stored_pass
    ESPUI.addControl(Max, "", "64", None, wifi_pass_text);
    ESPUI.addControl(Button, "WiFi", "Save", Peterriver, tab3, enterWifiDetailsCallback);

    ESPUI.addControl(ControlType::Button, "Reset device", "Reset", ControlColor::Emerald, tab3, &ResetCallback);

    //WiFiStatus = ESPUI.addControl(Label, "WiFi Status", "Wifi Status ok", ControlColor::None, tab3);

    photoValue = analogRead(photoPin);
    status = ESPUI.addControl(Label, "Status", "System status: OK", Wetasphalt, Control::noParent);
    ESPUI.getControl(status)->value = "Photo sensor = " + String(photoValue);
    ESPUI.addControl(Separator, soft_ID, "", Peterriver, tab1);

    //-------------------------------------------------- 
        //clear/init the DMD pixels held in RAM
       //printPinsStatus();
        //dmd.setBrightness(160);
    dmd.setBrightness(Brightness);
    dmd.clearScreen(true);
    dmd.selectFont(System5x7);
    dmd.drawString(0, 0, "Rousis LTD", 10, GRAPHICS_NORMAL, 1);
    dmd.drawString(0, 8, "Production board", 16, GRAPHICS_NORMAL, 1);
	dmd.drawString(0, 16, "Version 2.1", 11, GRAPHICS_NORMAL, 1);

    delay(2000);
    if (EEPROM.read(EEP_SELECTED_FUNCTION) > 0) {
        dmd.clearScreen(true);
        //dmd.drawString(0, 2, "TOTAL", 5, GRAPHICS_NORMAL, 1);
        //display the PRODUCT_LABEL_1A and PRODUCT_LABEL_1B
        String buf = read_String(PRODUCT_LABEL_1A, 10);
        dmd.drawString(0, 0, buf.c_str(), 10, GRAPHICS_NORMAL, 1);
        buf = read_String(PRODUCT_LABEL_1B, 10);
        dmd.drawString(0, 8, buf.c_str(), 10, GRAPHICS_NORMAL, 1);

        /*dmd.drawString(0, 18, "WICKETS", 7, GRAPHICS_NORMAL, 1);
        dmd.drawString(0, 34, "OVERS", 5, GRAPHICS_NORMAL, 1);
        dmd.drawString(0, 50, "1'INNS", 6, GRAPHICS_NORMAL, 1);*/

        dmd.selectFont(scoreboard_12);
        dmd.drawString(36, 2, "0", 1, GRAPHICS_NORMAL, 1);
        /*dmd.drawString(47, 16, "00", 2, GRAPHICS_NORMAL, 1);
        dmd.drawString(47, 32, "00", 2, GRAPHICS_NORMAL, 1);
        dmd.drawString(40, 48, "000", 3, GRAPHICS_NORMAL, 1);*/
    }

    ESPUI.begin("- Production Board", stored_user.c_str(), stored_password.c_str()); //"espboard", "12345678"

	configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    setTimezone("EET-2EEST,M3.5.0/3,M10.5.0/4");
    printLocalTime();
    SetExtRTC();

    last_update = millis();
}

// Add the main program code into the continuous loop() function
void loop()
{
    dnsServer.processNextRequest();
	  
	if (millis() - last_update > 3000) {
		last_update = millis();
        
        if (EEPROM.read(EEP_SELECTED_FUNCTION) == 0) {
            struct tm timeinfo;
            if (getLocalTime(&timeinfo)) {
                dmd.selectFont(Big_font);
                char timeStr[20];
				// Fisrt display the date to the upper part of the screen and then the time to the lower part of the screen
				dmd.clearScreen(true);
                strftime(timeStr, sizeof(timeStr), "%d-%m-%Y", &timeinfo);
                dmd.drawString(11, -1, timeStr, 10, GRAPHICS_NORMAL, 2);

                strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);   //"%H:%M:%S"               
                dmd.drawString(0, 17, timeStr, 5, GRAPHICS_NORMAL, 2);

                // Calculate and display the Day Of the Year
                char dayOfYearStr[20]= {0};
                strftime(dayOfYearStr, sizeof(dayOfYearStr), "%j", &timeinfo);

				dmd.drawString(60, 17, "LOT", 3, GRAPHICS_NORMAL, 2);
                dmd.drawString(94, 17, dayOfYearStr, 5, GRAPHICS_NORMAL, 2);
                /*Serial.println("dayOfYearStr: ");
                Serial.println(dayOfYearStr);*/
            }
            else {
                Serial.println();
				Serial.println("Failed to obtain time");
            }
        }
        
        // invert the value of the LED pin 
        //digitalWrite(LED_PIN, !digitalRead(LED_PIN));

		digitalWrite(LED_PIN, LOW);
		delay(100);
		digitalWrite(LED_PIN, HIGH);
       
	}

}

void printLocalTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void SetExtRTC() {

    Serial.println("Setting the external RTC");
    setTimezone("GMT0");
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        get_time_externalRTC();
        return;
    }
    setTimezone("EET-2EEST,M3.5.0/3,M10.5.0/4");
    /*
        Serial.print("Time to set DS1302: ");
        Serial.print(timeinfo.tm_year); Serial.print(" ");
        Serial.print(timeinfo.tm_mon); Serial.print(" ");
        Serial.print(timeinfo.tm_mday); Serial.print(" ");
        Serial.print(timeinfo.tm_hour); Serial.print(" ");
        Serial.print(timeinfo.tm_min); Serial.print(" ");
        Serial.println(timeinfo.tm_sec);
        Serial.println("--------------------------------------");*/

    Ds1302::DateTime dt = {
           .year = timeinfo.tm_year,
           .month = timeinfo.tm_mon + 1,
           .day = timeinfo.tm_mday,
           .hour = timeinfo.tm_hour,
           .minute = timeinfo.tm_min,
           .second = timeinfo.tm_sec,
           .dow = timeinfo.tm_wday
    };
    rtc.setDateTime(&dt);

    Serial.println(&timeinfo, "%A, %d %B %Y %H:%M:%S");
    Serial.println("----------------------------------------");
}

void setTimezone(String timezone) {
    Serial.printf("  Setting Timezone to %s\n", timezone.c_str());
    setenv("TZ", timezone.c_str(), 1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
    tzset();
}

void get_time_externalRTC() {
    // get the current time
    Ds1302::DateTime now;
    rtc.getDateTime(&now);

    Serial.println();
    Serial.println("Read and set the external Time from DS1302:");

    Serial.print("20");
    Serial.print(now.year);    // 00-99
    Serial.print('-');
    if (now.month < 10) Serial.print('0');
    Serial.print(now.month);   // 01-12
    Serial.print('-');
    if (now.day < 10) Serial.print('0');
    Serial.print(now.day);     // 01-31
    Serial.print(' ');
    Serial.print(now.dow); // 1-7
    Serial.print(' ');
    if (now.hour < 10) Serial.print('0');
    Serial.print(now.hour);    // 00-23
    Serial.print(':');
    if (now.minute < 10) Serial.print('0');
    Serial.print(now.minute);  // 00-59
    Serial.print(':');
    if (now.second < 10) Serial.print('0');
    Serial.print(now.second);  // 00-59
    Serial.println();
    Serial.println("---------------------------------------------");

    setTimezone("GMT0");
    setTime(now.year + 2000, now.month, now.day, now.hour, now.minute, now.second, 1);
    setTimezone("EET-2EEST,M3.5.0/3,M10.5.0/4");
}

void setTime(int yr, int month, int mday, int hr, int minute, int sec, int isDst) {
    struct tm tm;

    tm.tm_year = yr - 1900;   // Set date
    tm.tm_mon = month - 1;
    tm.tm_mday = mday;
    tm.tm_hour = hr;      // Set time
    tm.tm_min = minute;
    tm.tm_sec = sec;
    tm.tm_isdst = isDaylightSavingTime(&tm);  // 1 or 0
    time_t t = mktime(&tm);
    Serial.printf("Setting time: %s", asctime(&tm));
    struct timeval now = { .tv_sec = t };
    settimeofday(&now, NULL);
}

bool isDaylightSavingTime(struct tm* timeinfo) {
    // Implement your logic to check if DST is in effect based on your timezone's rules
    // For example, you might check if the current month and day fall within the DST period.

    int month = timeinfo->tm_mon + 1; // Month is 0-11, so add 1
    int day = timeinfo->tm_mday;

    // Add your DST logic here
    // For example, assuming DST is from April to October
    if ((month > 3) && (month < 11)) {
        return true;
    }

    return false;
}