// #################################################################################################################
// This software, the ideas and concepts is Copyright (c) David Bird 2021 and beyond.
// All rights to this software are reserved.
// It is prohibited to redistribute or reproduce of any part or all of the software contents in any form other than the following:
// 1. You may print or download to a local hard disk extracts for your personal and non-commercial use only.
// 2. You may copy the content to individual third parties for their personal use, but
//    *** only if you acknowledge the author David Bird as the source of the material. ***
// 3. You may not, except with my express written permission, distribute or commercially exploit the content.
// 4. You may not transmit it or store it in any other website or other form of electronic retrieval system for commercial purposes.
// 5. You *** MUST *** include all of this copyright and permission notice ('as annotated') and this shall be included
// 6. IF YOU DON'T LIKE THESE LICENCE CONDITIONS DONT USE THE SOFTWARE
//    in all copies or substantial portions of the software and where the software use is visible to an end-user.
// THE SOFTWARE IS PROVIDED "AS IS" FOR PRIVATE USE ONLY, IT IS NOT FOR COMMERCIAL USE IN WHOLE OR PART OR CONCEPT.
// FOR PERSONAL USE IT IS SUPPLIED WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, OR FITNESS
// FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
// IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// #################################################################################################################

#include <WiFiClientSecure.h>  // In-built
WiFiClientSecure client;

#include <WiFi.h>              // In-built
#include <HTTPClient.h>        // In-built
#include <SPI.h>               // In-built
#include <time.h>              // In-built
#include <SPI.h>               // Built-in 
#include <GxEPD2_BW.h>         // GxEPD2 from Sketch, Include Library, Manage Libraries, search for GxEDP2
#include <U8g2_for_Adafruit_GFX.h>
#include "credentials.h"

#define  ENABLE_GxEPD2_display 0
#define  SCREEN_WIDTH  400
#define  SCREEN_HEIGHT 300

enum alignment {LEFT, RIGHT, CENTRE};

// Connections for e.g. LOLIN D32
static const uint8_t EPD_BUSY = 4;  // to EPD BUSY
static const uint8_t EPD_CS   = 5;  // to EPD CS
static const uint8_t EPD_RST  = 16; // to EPD RST
static const uint8_t EPD_DC   = 17; // to EPD DC
static const uint8_t EPD_SCK  = 18; // to EPD CLK
static const uint8_t EPD_MISO = 19; // Master-In Slave-Out not used, as no data from display
static const uint8_t EPD_MOSI = 23; // to EPD DIN

// Connections for e.g. Waveshare ESP32 e-Paper Driver Board
//static const uint8_t EPD_BUSY = 25;
//static const uint8_t EPD_CS   = 15;
//static const uint8_t EPD_RST  = 26;
//static const uint8_t EPD_DC   = 27;
//static const uint8_t EPD_SCK  = 13;
//static const uint8_t EPD_MISO = 12; // Master-In Slave-Out not used, as no data from display
//static const uint8_t EPD_MOSI = 14;

//GxEPD2_BW<GxEPD2_154_M09, GxEPD2_154_M09::HEIGHT> display(GxEPD2_154_M09(/*CS*/ EPD_CS, /*DC*/ EPD_DC, /*RST*/ EPD_RST, /*BUSY*/ EPD_BUSY)); // GDEW0154M09 200x200
GxEPD2_BW<GxEPD2_420, GxEPD2_420::HEIGHT> display(GxEPD2_420(/*CS=5*/ EPD_CS, /*DC=*/ EPD_DC, /*RST=*/ EPD_RST, /*BUSY=*/ EPD_BUSY)); // GDEW042T2

U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;  // Select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
// Using fonts:
// u8g2_font_helvB08_tf
// u8g2_font_helvB10_tf
// u8g2_font_helvB12_tf
// u8g2_font_helvB14_tf
// u8g2_font_helvB18_tf
// u8g2_font_helvB24_tf

typedef struct {
  String Metar         = "";
  String Station       = "";
  String StationMode   = "";
  String Date          = "";
  String Time          = "";
  String Description   = "";
  float  Temperature   = 0;
  float  DewPoint      = 0;
  float  WindChill     = 0;
  float  Humidity      = 0;
  int    Pressure      = 0;
  int    WindSpeed     = 0;
  int    WindDirection = 0;
  int    Gusting       = 0;
  String Units         = "";
  String Visibility    = "";
  String Veering       = "";
  int    StartV        = 0;
  int    EndV          = 0;
  String CloudsL1      = "";
  String CloudsL2      = "";
  String CloudsL3      = "";
  String CloudsL4      = "";
  String Conditions    = "";
  String Report1       = "";
  String Report2       = "";
} metar_type;

const int        maximumMETAR = 6; // (actually -1 !!)
metar_type METAR[maximumMETAR];

//################  VERSION  ##################################################
String version = "(v1.0)";  // for EPAPER Display Unit
//################ PROGRAM VARIABLES and OBJECTS ##########################################
long      SleepDuration = 30;    // Sleep time in minutes, aligned to the nearest minute boundary, so if 30 will always update at 00 or 30 past the hour
const int time_delay    = 20000; // 20-secs between METAR displays in decode mode
int       count         = 0;
int       displayLine   = 0;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("Starting...");
  StartWiFi();
  InitialiseSystem();
  Serial.println("Starting...");
  ReceiveMETAR();
  StopWiFi();
  if (!summaryMode) display.display(false); // Full screen update mode
  BeginSleep();
}

void loop() {
  // Nothing to do here
}

void ReceiveMETAR() {
  // Change these METAR Stations to suit your needs see: Use this URL address = ftp://tgftp.nws.noaa.gov/data/observations/metar/decoded/
  // to establish your list of sites to retrieve (you must know the 4-letter site dentification)
  display.fillScreen(GxEPD_WHITE);
  drawString(5, 10, "Receiving METAR data...", LEFT);
  display.display(false);
  GET_METAR("EGLL", "EGLL London Heathrow", true);
  GET_METAR("KIAD", "KIAD Washington, DC", true); // METAR Code, Description, Clear the screen (true) after display or not
  GET_METAR("LFPO", "LFPO Paris/Orly", true);
  GET_METAR("YSSY", "YSSY Sydney", true);
  GET_METAR("FACT", "FACT Cape Town", true);
  GET_METAR("GMMX", "GMMX Marakesh", false);
  if (summaryMode) {
    display.fillScreen(GxEPD_WHITE);
    u8g2Fonts.setFont(u8g2_font_helvB08_tf);   // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
    for (int displayMetar = 0; displayMetar < count; displayMetar++) DisplayMetarSummary(displayMetar);
    display.display(false);
  }
}

void GET_METAR(String Station, String Name, bool displayRefresh) { //client function to send/receive GET request data.
  String metar, raw_metar;
  // https://aviationweather.gov/adds/dataserver_current/httpparam?dataSource=metars&requestType=retrieve&format=xml&stationString=EGLL&hoursBeforeNow=1
  String url = "/cgi-bin/data/metar.php?dataSource=metars&requestType=retrieve&format=xml&ids=" + station + "&hoursBeforeNow=1";
  //String url = "/adds/dataserver_current/httpparam?datasource=metars&requestType=retrieve&format=xml&mostRecentForEachStation=constraint&hoursBeforeNow=1.25&stationString=" + Station;
  Serial.println("Connected, \nRequesting data for : " + Name);
  HTTPClient http;
  http.begin(host + url);
  int httpCode = http.GET(); //  Start connection and send HTTP header
  if (httpCode > 0) {        // HTTP header has been sent and Server response header has been handled
    if (httpCode == HTTP_CODE_OK) raw_metar = http.getString();
    http.end();
  }
  else
  {
    http.end();
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  metar = raw_metar.substring(raw_metar.indexOf("<raw_text>", 0) + 10, raw_metar.indexOf("</raw_text>", 0));
  Serial.println(metar);
  client.stop();
  if (metar.length() > 0) {
    if (!summaryMode) {
      display.fillScreen(GxEPD_WHITE);
      display_metar(metar, Name);
      display.display(false);
      delay(time_delay);
      if (displayRefresh) display.fillScreen(GxEPD_WHITE);
    }
    else
    {
      METAR[count].Metar = metar;
      count++;
    }
  }
  else
  {
    drawString(5, 0, Station + " - Station off-air", LEFT); // Now decode METAR
  }
}

void display_metar(String metar, String Station) {
  Serial.println("Displaying Metar : " + metar);
  int temperature   = 0;
  int dew_point     = 0;
  int wind_speedKTS = 0;
  char   str[130]   = "";
  char   *parameter;
  String conditions_start = " ";
  String temp_strA = "";
  String temp_strB = "";
  String conditions_test = "+-BDFGHIMPRSTUV"; // Test for light or heavy (+/-) BR - Mist, RA - Rain  SH-Showers VC - Vicinity, etc
  // Test metar exercises all decoder functions on screen, uncomment to invoke
  //metar = "EGDM 261550Z AUTO 26517G23KT 250V290 6999 R04/1200 4200SW -SH SN NCD SCT002TCU SCT090 FEW222/// 12/M15 Q1001 RERA NOSIG BLU";
  metar.toCharArray(str, 130);
  parameter = strtok(str, " "); // Find tokens that are seperated by SPACE
  while (parameter != NULL) {
    METAR[count].Metar = metar;
    u8g2Fonts.setFont(u8g2_font_helvB08_tf);   // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
    int line_length = 62;
    if (metar.length() > line_length) {
      drawString(5, 275, metar.substring(0, line_length), LEFT);
      drawString(5, 290, metar.substring(line_length), LEFT);
    }
    else drawString(5, 270, metar, LEFT);
    u8g2Fonts.setFont(u8g2_font_helvB12_tf);
    temp_strA = strtok(NULL, " ");
    METAR[count].Metar   = metar;
    METAR[count].Station = Station;
    METAR[count].Date    = temp_strA.substring(0, 2);
    METAR[count].Time    = temp_strA.substring(2, 6);
    drawString(5, 10, Station + "  Date:" + METAR[count].Date + " @ " + METAR[count].Time + "Hr", LEFT);
    temp_strA = strtok(NULL, " ");
    Serial.println(temp_strA);
    if (temp_strA == "AUTO") {
      METAR[count].StationMode = temp_strA;
      temp_strA = strtok(NULL, " ");
    }
    METAR[count].WindSpeed = temp_strA.substring(3, 5).toInt();
    if (temp_strA.endsWith("KT"))  METAR[count].Units = "kts";
    if (temp_strA.endsWith("MPS")) METAR[count].Units = "mps";
    if (temp_strA.endsWith("MPH")) METAR[count].Units = "mph";
    if (temp_strA.endsWith("KPH")) METAR[count].Units = "kph";
    int windDirection = 0;
    if  (temp_strA.indexOf('G') >= 0) {
      if (temp_strA.endsWith("KT"))  METAR[count].Gusting = temp_strA.substring(temp_strA.indexOf('G') + 1, temp_strA.indexOf("KT")).toInt();
      if (temp_strA.endsWith("MPS")) METAR[count].Gusting = temp_strA.substring(temp_strA.indexOf('G') + 1, temp_strA.indexOf("MPS")).toInt();
      if (temp_strA.endsWith("MPH")) METAR[count].Gusting = temp_strA.substring(temp_strA.indexOf('G') + 1, temp_strA.indexOf("MPH")).toInt();
      if (temp_strA.endsWith("KPH")) METAR[count].Gusting = temp_strA.substring(temp_strA.indexOf('G') + 1, temp_strA.indexOf("KPH")).toInt();
    }
    else temp_strB = temp_strA.substring(5);
    if (temp_strA.substring(0, 3) == "VRB") {
      METAR[count].Veering = "VRB";
      drawString(275, 30, "VRB", LEFT);
    }
    else {
      METAR[count].WindDirection = temp_strA.substring(0, 3).toInt();
      windDirection = temp_strA.substring(0, 3).toInt();
    }
    temp_strA = strtok(NULL, " ");
    int startV = -1;
    int endV   = -1;
    if (temp_strA.indexOf('V') >= 0 && temp_strA != "CAVOK") { // Check for variable wind direction
      startV = temp_strA.substring(0, 3).toInt();
      endV   = temp_strA.substring(4, 7).toInt();
      METAR[count].StartV = startV;
      METAR[count].EndV   = endV;
      // Minimum angle is either ABS(AngleA- AngleB) or (360-ABS(AngleA-AngleB))
      temp_strA = strtok(NULL, " ");    // Move to the next token/item
    }
    DisplayDisplayWindSection(330, 85, windDirection, METAR[count].WindSpeed, startV, endV, 50);
    if (temp_strA == "////") {
      temp_strB = "No Visibility Rep.";
      METAR[count].Visibility = temp_strB.toInt();
    }
    else {
      temp_strB = "";
      METAR[count].Visibility = temp_strB.toInt();
    }
    if (temp_strA == "CAVOK") {
      drawString(5, 33, "Visibility and Conditions good", LEFT);
      METAR[count].Conditions = "Visibility and Conditions good";
    }
    else {
      if (temp_strA != "////") {
        if (temp_strA == "9999") {
          temp_strB = "Visibility excellent";
          METAR[count].Conditions = "Visibility excellent";
        }
        else
        {
          String vis = temp_strA;
          while (vis.startsWith("0")) vis = vis.substring(1); // trim leading '0'
          if (vis.endsWith("SM")) temp_strB = vis.substring(0, vis.indexOf("SM")) + " Statute Miles";
          else temp_strB = vis + " Metres of visibility";
          METAR[count].Visibility = temp_strB;
        }
      }
    }
    drawString(5, 28, temp_strB, LEFT);
    temp_strA = strtok(NULL, " ");
    if ( temp_strA.startsWith("R") && !temp_strA.startsWith("RA")) { // Ignore an RA for Rain weather report for now
      temp_strA = strtok(NULL, " ");  // If there was a Variable report, move to the next item
    }
    if ( temp_strA.startsWith("R") && !temp_strA.startsWith("RA")) { // Ignore an RA for Rain weather report for now, there can be two
      temp_strA = strtok(NULL, " ");  // If there was a Variable report, move to the next item
    }
    if (temp_strA.length() >= 5 && temp_strA.substring(0, 1) != "+" && temp_strA.substring(0, 1) != "-" && (temp_strA.endsWith("N") || temp_strA.endsWith("S") || temp_strA.endsWith("E") || temp_strA.endsWith("W")) ) {
      conditions_start = temp_strA.substring(4);
      conditions_start = conditions_start.substring(0, 1);
      if (conditions_start == "N" || conditions_start == "S" || conditions_start == "E" || conditions_start == "W") {
        drawString(5, 45, "/" + temp_strA + " Mts Visibility", LEFT);
        METAR[count].Visibility += "/" + temp_strA + " Mts Visibility";
      }
      temp_strA = strtok(NULL, " ");
    }
    drawString(5, 60, "Additional Wx Reports:", LEFT);
    conditions_start = temp_strA.substring(0, 1);
    temp_strB = temp_strA.substring(0, 3);
    if ((temp_strA == "//" || conditions_test.indexOf(conditions_start) >= 0)
        && !(valid_cloud_report(temp_strB)) // Don't process a cloud report that starts with the same letter as a weather condition report
        && temp_strB != "NSC"
        && !temp_strA.startsWith("M0")) {
      temp_strB = "";
      if (conditions_start == "-" || conditions_start == "+") {
        if (conditions_start == "-") {
          temp_strB = "Light ";
        } else {
          temp_strB = "Heavy ";
        }
        temp_strA = temp_strA.substring(1); // Remove leading + or - and can't modify same variable recursively
      }
      if (temp_strA.length() <= 3) {
        drawString(5, 75, temp_strB + display_conditions(temp_strA), LEFT);
        METAR[count].Conditions = temp_strB + display_conditions(temp_strA);
      }
      else {
        drawString(5, 90, temp_strB + display_conditions(temp_strA.substring(0, 2)) + "/" + display_conditions(temp_strA.substring(2, 4)), LEFT);
        if (temp_strA.length() >= 6) { // sometimes up to three cateries are reported
          drawString(50, 115, "/" + display_conditions(temp_strA.substring(4, 6)), LEFT);
          METAR[count].Conditions += "/" + display_conditions(temp_strA.substring(4, 6));
        }
      }
      parameter = strtok(NULL, " ");
      temp_strA = parameter;
    }
    conditions_start = temp_strA.substring(0, 1);
    temp_strB = temp_strA.substring(0, 3);
    if ((temp_strA == "//" || conditions_test.indexOf(conditions_start) >= 0)
        && !(valid_cloud_report(temp_strB)) // Don't process a cloud report that starts with the same letter as a weather condition report
        && temp_strB != "NSC"
        && !temp_strA.startsWith("M0")) {
      temp_strB = "";
      if (conditions_start == "-" || conditions_start == "+") {
        if (conditions_start == "-") {
          temp_strB = "Light ";
        } else {
          temp_strB = "Heavy ";
        }
        temp_strA = temp_strA.substring(1); // Remove leading + or - and can't modify same variable recursively
      }
      if (temp_strA.length() == 2) {
        drawString(5, 90, temp_strB + display_conditions(temp_strA), LEFT);
        METAR[count].Conditions = temp_strB + display_conditions(temp_strA);
      }
      else {
        if (temp_strA.length() >= 5) { // sometimes up to three cateries are reported
          drawString(5, 105, "Poor Vert Visibility", LEFT);
          METAR[count].Conditions = "Poor Vert Visibility";
        }
        else {
          drawString(5, 105, temp_strB + display_conditions(temp_strA.substring(0, 2)), LEFT);
          METAR[count].Conditions = temp_strB + display_conditions(temp_strA.substring(0, 2));
        }
      }
      parameter = strtok(NULL, " ");
      temp_strA = parameter;
    }
    display.drawLine(5, 20, 400, 20, GxEPD_BLACK);
    if (temp_strA == "///" || temp_strA == "////" || temp_strA == "/////" || temp_strA == "//////")  {
      temp_strA = "No CC Rep.";
      METAR[count].CloudsL1 = "No Cloud Cover Report";
      temp_strA = strtok(NULL, " ");
    }
    else
    {
      if (valid_cloud_report(temp_strA) || temp_strA.startsWith("VV/")) {
        temp_strA = convert_clouds(temp_strA);
        drawString(5, 125, temp_strA, LEFT);
        METAR[count].CloudsL1 = temp_strA;
        temp_strA = strtok(NULL, " ");
      }
      if (valid_cloud_report(temp_strA)) {
        temp_strA = convert_clouds(temp_strA);
        drawString(5, 145, temp_strA, LEFT);
        METAR[count].CloudsL2 = temp_strA;
        temp_strA = strtok(NULL, " ");
      }
      if (valid_cloud_report(temp_strA)) {
        temp_strA = convert_clouds(temp_strA);
        drawString(5, 165, temp_strA, LEFT);
        METAR[count].CloudsL3 = temp_strA;
        temp_strA = strtok(NULL, " ");
      }
      if (valid_cloud_report(temp_strA)) {
        temp_strA = convert_clouds(temp_strA);
        drawString(5, 185, temp_strA, LEFT);
        METAR[count].CloudsL4 = temp_strA;
        temp_strA = strtok(NULL, " ");
      }
    }
    if (temp_strA.indexOf("/") <= 0) {
      temp_strA = "00/00";
    }
    String temp_sign = "";
    if (temp_strA.startsWith("M")) {
      temperature = 0 - temp_strA.substring(1, 3).toInt();
      if (temperature == 0) {
        temp_sign = "-"; // Reports of M00, meaning between -0.1 and -0.9C
      }
    }
    else {
      temperature = temp_strA.substring(0, 2).toInt();
      if (temperature >= 0 && temperature < 10) {
        temp_sign = " "; // Aligns for 2-digit temperatures e.g.  " 1'C" and "11'C"
      }
    }
    temp_strB = temp_strA.substring(temp_strA.indexOf('/') + 1);
    if (temp_strB.startsWith("M")) {
      dew_point = 0 - temp_strB.substring(1, 3).toInt();
    }
    else {
      dew_point = temp_strB.substring(0, 2).toInt();
    }
    METAR[count].DewPoint = dew_point;
    drawString(300, 225, "DewP " + String(METAR[count].DewPoint, 0) + "°", LEFT);
    u8g2Fonts.setFont(u8g2_font_helvB12_tf);   // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
    if (wind_speedKTS > 3 && temperature <= 14) {
      temp_sign = "";
      int wind_chill = int(calc_windchill(temperature, wind_speedKTS));
      if (wind_chill >= 0 && wind_chill < 10) {
        temp_sign = " "; // Aligns for 2-digit temperatures e.g.  " 1'C" and "11'C"
      }
      METAR[count].WindChill = wind_chill;
      drawString(300, 210, "WindC " + temp_sign + String(wind_chill) + "°", LEFT);
    }
    temp_sign = "";
    if (dew_point >= 0 && dew_point < 10) {
      temp_sign = " "; // Aligns for 2-digit temperatures e.g.  " 1'C" and "11'C"
    }
    int RH = calc_rh(temperature, dew_point);
    u8g2Fonts.setFont(u8g2_font_helvB18_tf);   // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
    METAR[count].Temperature = temperature;
    METAR[count].Humidity = RH;
    drawString(315, 175, temp_sign + String(temperature) + "°  " + String(RH) + "%", CENTRE);
    u8g2Fonts.setFont(u8g2_font_helvB12_tf);
    if (temperature >= 20) {
      float T = (temperature * 9 / 5) + 32;
      float RHx = RH;
      int tHI = (-42.379 + (2.04901523 * T) + (10.14333127 * RHx) - (0.22475541 * T * RHx) - (0.00683783 * T * T) - (0.05481717 * RHx * RHx)  + (0.00122874 * T * T * RHx) + (0.00085282 * T * RHx * RHx) - (0.00000199 * T * T * RHx * RHx) - 32 ) * 5 / 9;
      drawString(295, 240, "HeatX " + String(tHI) + "°", LEFT);
    }
    temp_strA = strtok(NULL, " ");
    temp_strA.trim();
    if (temp_strA.startsWith("Q")) {
      if (temp_strA.substring(1, 2) == "0") {
        temp_strA = " " + temp_strA.substring(2);
      }
      else {
        temp_strA = temp_strA.substring(1);
      }
      u8g2Fonts.setFont(u8g2_font_helvB14_tf);   // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
      METAR[count].Pressure = temp_strA.toInt();
      drawString(295, 195, String(METAR[count].Pressure) + " hPa", LEFT);
      u8g2Fonts.setFont(u8g2_font_helvB12_tf);   // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
    }
    else
    {
      temp_strA = temp_strA.substring(1);
      METAR[count].Pressure = temp_strA.toInt();
      u8g2Fonts.setFont(u8g2_font_helvB14_tf);   // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
      drawString(300, 195, String(METAR[count].Pressure / 100.0, 2) + " in", LEFT);
      u8g2Fonts.setFont(u8g2_font_helvB12_tf);   // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
    }
    temp_strA = strtok(NULL, " "); // Get last tokens, can be a secondary weather report
    METAR[count].Report1 = Process_secondary_reports(temp_strA);
    drawString(5, 210, METAR[count].Report1, LEFT);
    temp_strA = strtok(NULL, " "); // Get last tokens, can be a secondary weather report
    METAR[count].Report2 = Process_secondary_reports(temp_strA);
    drawString(5, 230, METAR[count].Report2, LEFT);
    parameter = NULL; // Reset the 'str' pointer, so end of report decoding
  }
  DisplayMetar();
  if (!summaryMode) ClearVariables();
  count++;
}


String convert_clouds(String source) {
  String height  = source.substring(3, 6);
  String cloud   = source.substring(0, 3);
  String warning = " ";
  while (height.startsWith("0")) height = height.substring(1);
  if (source.endsWith("TCU") || source.endsWith("CB")) {
    drawString(5, 105, "Warning - storm clouds detected", LEFT);
    warning = " (storm) ";
  }
  if (cloud != "SKC" && cloud != "CLR" && height != " ") {
    height = height + "00ft - ";
  } else height = "";
  if (source == "VV///") return "No cloud reported";
  if (cloud == "BKN")    return height + "Broken" + warning + "clouds";
  if (cloud == "SKC")    return "Clear skies";
  if (cloud == "FEW")    return height + "Few" + warning + "clouds";
  if (cloud == "NCD")    return "No clouds detected";
  if (cloud == "NSC")    return "No signficiant clouds";
  if (cloud == "OVC")    return height + "Overcast" + warning;
  if (cloud == "SCT")    return height + "Scattered" + warning + "clouds";
  return "";
}

int min_val(int num1, int num2) {
  if (num1 > num2) return num2;
  else return num1;
}

float calc_rh(int temp, int dewp) {
  return 100 * (exp((17.271 * dewp) / (237.7 + dewp))) / (exp((17.271 * temp) / (237.7 + temp))) + 0.5;
}

float calc_windchill(int temperature, int wind_speed) {
  float result;
  // Derived from wind_chill = 13.12 + 0.6215 * Tair - 11.37 * POWER(wind_speed,0.16)+0.3965 * Tair * POWER(wind_speed,0.16)
  wind_speed = wind_speed * 1.852; // Convert to Kph
  result = 13.12 + 0.6215 * temperature - 11.37 * pow(wind_speed, 0.16) + 0.3965 * temperature * pow(wind_speed, 0.16);
  if (result < 0 ) return result - 0.5;
  else return result + 0.5;
}

String Process_secondary_reports(String temp_strA) {
  temp_strA.trim();
  if (temp_strA == "NOSIG") return "No significant change expected";
  if (temp_strA == "TEMPO") return "Temporary conditions expected";
  if (temp_strA == "RADZ")  return "Recent Rain/Drizzle";
  if (temp_strA == "RERA")  return "Recent Moderate/Heavy Rain";
  if (temp_strA == "REDZ")  return "Recent Drizzle";
  if (temp_strA == "RESN")  return "Recent Moderate/Heavy Snow";
  if (temp_strA == "RASN")  return "Recent Moderate/Heavy Rain & Snow";
  if (temp_strA == "RESG")  return "Recent Moderate/Heavy Snow grains";
  if (temp_strA == "REGR")  return "Recent Moderate/Heavy Hail";
  if (temp_strA == "RETS")  return "Recent Thunder storms";
  return "";
}

String display_conditions(String WX_state) {
  if (WX_state == "//") return "No weather reported";
  if (WX_state == "VC") return "Vicinity has";
  if (WX_state == "BL") return "Blowing";
  if (WX_state == "SH") return "Showers";
  if (WX_state == "TS") return "Thunderstorms";
  if (WX_state == "FZ") return "Freezing";
  if (WX_state == "UP") return "Unknown";
  if (WX_state == "MI") return "Shallow";
  if (WX_state == "PR") return "Partial";
  if (WX_state == "BC") return "Patches";
  if (WX_state == "DR") return "Low drifting";
  if (WX_state == "IC") return "Ice crystals";
  if (WX_state == "PL") return "Ice pellets";
  if (WX_state == "GR") return "Hail";
  if (WX_state == "GS") return "Small hail";
  if (WX_state == "DZ") return "Drizzle";
  if (WX_state == "RA") return "Rain";
  if (WX_state == "SN") return "Snow";
  if (WX_state == "SG") return "Snow grains";
  if (WX_state == "DU") return "Widespread dust";
  if (WX_state == "SA") return "Sand";
  if (WX_state == "HZ") return "Haze";
  if (WX_state == "PY") return "Spray";
  if (WX_state == "BR") return "Mist";
  if (WX_state == "FG") return "Fog";
  if (WX_state == "FU") return "Smoke";
  if (WX_state == "VA") return "Volcanic ash";
  if (WX_state == "DS") return "Dust storm";
  if (WX_state == "PO") return "Well developed dust/sand swirls";
  if (WX_state == "SQ") return "Squalls";
  if (WX_state == "FC") return "Funnel clouds/Tornadoes";
  if (WX_state == "SS") return "Sandstorm";
  return "";
}

boolean valid_cloud_report(String temp_strA) {
  if (temp_strA.startsWith("BKN") ||
      temp_strA.startsWith("CLR") ||
      temp_strA.startsWith("FEW") ||
      temp_strA.startsWith("NCD") ||
      temp_strA.startsWith("NSC") ||
      temp_strA.startsWith("OVC") ||
      temp_strA.startsWith("SCT") ||
      temp_strA.startsWith("SKC") ||
      temp_strA.endsWith("CB")    ||
      temp_strA.endsWith("TCU")) {
    return true;
  }
  else return false;
}

void DisplayMetar() {
  Serial.println("         Metar = " + METAR[count].Metar);
  Serial.println("       Station = " + METAR[count].Station);
  Serial.println("  Station Mode = " + METAR[count].StationMode);
  Serial.println("          Date = " + METAR[count].Date);
  Serial.println("          Time = " + METAR[count].Time);
  Serial.println("   Description = " + METAR[count].Description);
  Serial.println("   Temperature = " + String(METAR[count].Temperature));
  Serial.println("      Dewpoint = " + String(METAR[count].DewPoint));
  Serial.println("     WindChill = " + String(METAR[count].WindChill));
  Serial.println("      Humidity = " + String(METAR[count].Humidity));
  Serial.println("      Pressure = " + String(METAR[count].Pressure));
  Serial.println("     WindSpeed = " + String(METAR[count].WindSpeed));
  Serial.println("Wind Direction = " + String(METAR[count].WindDirection));
  Serial.println("       Gusting = " + String(METAR[count].Gusting));
  Serial.println("    Visibility = " + String(METAR[count].Visibility));
  Serial.println("           VRB = " + METAR[count].Veering);
  Serial.println(" Veering Start = " + String(METAR[count].StartV));
  Serial.println("   Veering End = " + String(METAR[count].EndV));
  Serial.println(" Cloud Level-1 = " + METAR[count].CloudsL1);
  Serial.println(" Cloud Level-2 = " + METAR[count].CloudsL2);
  Serial.println(" Cloud Level-3 = " + METAR[count].CloudsL3);
  Serial.println(" Cloud Level-4 = " + METAR[count].CloudsL4);
  Serial.println("    Conditions = " + METAR[count].Conditions);
  Serial.println("      Report-1 = " + METAR[count].Report1);
  Serial.println("      Report-2 = " + METAR[count].Report2);
  Serial.println("---------------------------------------------");
}

void ClearVariables() {
  METAR[count].Metar         = "";
  METAR[count].Station       = "";
  METAR[count].StationMode   = "";
  METAR[count].Date          = "";
  METAR[count].Time          = "";
  METAR[count].Description   = "";
  METAR[count].Temperature   = 0;
  METAR[count].DewPoint      = 0;
  METAR[count].WindChill     = 0;
  METAR[count].Humidity      = 0;
  METAR[count].Pressure      = 0;
  METAR[count].WindSpeed     = 0;
  METAR[count].WindDirection = 0;
  METAR[count].Gusting       = 0;
  METAR[count].Visibility    = "";
  METAR[count].Veering       = "";
  METAR[count].StartV        = 0;
  METAR[count].EndV          = 0;
  METAR[count].CloudsL1      = "";
  METAR[count].CloudsL2      = "";
  METAR[count].CloudsL3      = "";
  METAR[count].CloudsL4      = "";
  METAR[count].Conditions    = "";
  METAR[count].Report1       = "";
  METAR[count].Report2       = "";
}

void DisplayMetarSummary(int count) {
  String Line1 = "", Line2 = "";
  int lineLength = 62;
  if (METAR[count].Metar.length() > lineLength) {
    Line1 = METAR[count].Metar.substring(0, lineLength);
    Line2 = METAR[count].Metar.substring(lineLength + 1);
  }
  else Line1 = METAR[count].Metar;
  drawString(5, 10 + displayLine * 18, Line1, LEFT);
  if (Line2 != "") {
    displayLine++;
    drawString(5, 10 + displayLine * 18, Line2, LEFT);
  }
  displayLine++;
  if (count < maximumMETAR - 1) drawString(5, 10 + displayLine * 18, "~~~~~~~~~", LEFT);
  displayLine++;
  Serial.println("Metar = " + METAR[count].Metar);
}

void drawString(int x, int y, String text, alignment align) {
  int16_t  x1, y1; //the bounds of x,y and w and h of the variable 'text' in pixels.
  uint16_t w, h;
  display.setTextWrap(false);
  display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
  if (align == RIGHT)  x = x - w;
  if (align == CENTRE) x = x - w / 2;
  u8g2Fonts.setCursor(x, y + h);
  u8g2Fonts.print(text);
}

void DisplayDisplayWindSection(int x, int y, float angle, float windspeed, int startV, int endV, int Cradius) {
  u8g2Fonts.setFont(u8g2_font_helvB08_tf);   // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
  arrow(x, y, Cradius - 22, angle, 15, 20);  // Awidth, Alength
  if (startV >= 0) arrow(x, y, Cradius - 24, startV, 8, 15); // Awidth, Alength Veering 1
  if (startV >= 0) arrow(x, y, Cradius - 24, endV, 8, 15);   // Awidth, Alength Veering 2
  int dxo, dyo, dxi, dyi;
  display.drawCircle(x, y, Cradius, GxEPD_BLACK);       // Draw compass circle
  display.drawCircle(x, y, Cradius + 1, GxEPD_BLACK);   // Draw compass circle
  display.drawCircle(x, y, Cradius * 0.7, GxEPD_BLACK); // Draw compass inner circle
  for (float a = 0; a < 360; a = a + 22.5) {
    dxo = Cradius * cos((a - 90) * PI / 180);
    dyo = Cradius * sin((a - 90) * PI / 180);
    if (a == 45)  drawString(dxo + x + 12, dyo + y - 5, "NE", CENTRE);
    if (a == 135) drawString(dxo + x + 5,  dyo + y + 5, "SE", CENTRE);
    if (a == 225) drawString(dxo + x - 12, dyo + y + 3, "SW", CENTRE);
    if (a == 315) drawString(dxo + x - 15, dyo + y - 5, "NW", CENTRE);
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    display.drawLine(dxo + x, dyo + y, dxi + x, dyi + y, GxEPD_BLACK);
    dxo = dxo * 0.7;
    dyo = dyo * 0.7;
    dxi = dxo * 0.9;
    dyi = dyo * 0.9;
    display.drawLine(dxo + x, dyo + y, dxi + x, dyi + y, GxEPD_BLACK);
  }
  drawString(x + 1, y - Cradius - 12, "N", CENTRE);
  drawString(x + 1, y + Cradius + 4,  "S", CENTRE);
  drawString(x - Cradius - 10, y - 3, "W", CENTRE);
  drawString(x + Cradius + 8, y - 3,  "E", CENTRE);
  u8g2Fonts.setFont(u8g2_font_helvB08_tf);
  drawString(x + 5, y + 20, String(angle, 0) + "°", CENTRE);
  drawString(x + 1, y + 9, METAR[count].Units, CENTRE);
  int veering = min_val(360 - abs(startV - endV), abs(startV - endV));
  if (veering > 0 ) drawString(x + 5, y + Cradius + 15, "Veering: " + String(veering) + "*", CENTRE);
  u8g2Fonts.setFont(u8g2_font_helvB12_tf);
  drawString(x - (WindDegToOrdinalDirection(angle).length() < 3 ? 5 : 10), y - 19, WindDegToOrdinalDirection(angle), CENTRE);
  drawString(x - 5, y  - 2, String(windspeed, (METAR[count].Gusting > 0 ? 0 : 1)) + (METAR[count].Gusting > 0 ? "/" + String(METAR[count].Gusting) : ""), CENTRE);
}

void arrow(int x, int y, int Asize, float Aangle, int Awidth, int Alength) {
  float dx = (Asize + 28) * cos((Aangle - 90) * PI / 180) + x; // calculate X position
  float dy = (Asize + 28) * sin((Aangle - 90) * PI / 180) + y; // calculate Y position
  float x1 = 0;           float y1 = Alength;
  float x2 = Awidth / 2;  float y2 = Awidth / 2;
  float x3 = -Awidth / 2; float y3 = Awidth / 2;
  float angle = Aangle * PI / 180;
  float xx1 = x1 * cos(angle) - y1 * sin(angle) + dx;
  float yy1 = y1 * cos(angle) + x1 * sin(angle) + dy;
  float xx2 = x2 * cos(angle) - y2 * sin(angle) + dx;
  float yy2 = y2 * cos(angle) + x2 * sin(angle) + dy;
  float xx3 = x3 * cos(angle) - y3 * sin(angle) + dx;
  float yy3 = y3 * cos(angle) + x3 * sin(angle) + dy;
  display.fillTriangle(xx1, yy1, xx2, yy2, xx3, yy3, GxEPD_BLACK);
}

String WindDegToOrdinalDirection(float winddirection) {
  if (winddirection >= 348.75 || winddirection < 11.25)  return "N";
  if (winddirection >=  11.25 && winddirection < 33.75)  return "NNE";
  if (winddirection >=  33.75 && winddirection < 56.25)  return "NE";
  if (winddirection >=  56.25 && winddirection < 78.75)  return "ENE";
  if (winddirection >=  78.75 && winddirection < 101.25) return "E";
  if (winddirection >= 101.25 && winddirection < 123.75) return "ESE";
  if (winddirection >= 123.75 && winddirection < 146.25) return "SE";
  if (winddirection >= 146.25 && winddirection < 168.75) return "SSE";
  if (winddirection >= 168.75 && winddirection < 191.25) return "S";
  if (winddirection >= 191.25 && winddirection < 213.75) return "SSW";
  if (winddirection >= 213.75 && winddirection < 236.25) return "SW";
  if (winddirection >= 236.25 && winddirection < 258.75) return "WSW";
  if (winddirection >= 258.75 && winddirection < 281.25) return "W";
  if (winddirection >= 281.25 && winddirection < 303.75) return "WNW";
  if (winddirection >= 303.75 && winddirection < 326.25) return "NW";
  if (winddirection >= 326.25 && winddirection < 348.75) return "NNW";
  return " ? ";
}

void BeginSleep() {
  esp_sleep_enable_timer_wakeup(SleepDuration * 60 * 1000000LL); // in Secs, 1000000LL converts to Secs
  Serial.println("Starting deep - sleep period...");
  esp_deep_sleep_start();  // Sleep for e.g. 30 minutes
}

void StartWiFi() {
  Serial.println("\r\nConnecting to : " + String(ssid));
  IPAddress dns(8, 8, 8, 8); // Use Google DNS
  WiFi.disconnect();
  WiFi.mode(WIFI_STA); // switch off AP
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("STA : Failed!\n");
    WiFi.disconnect(false);
    delay(10000);
    WiFi.reconnect();
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected at : " + WiFi.localIP().toString());
  }
  else Serial.println("WiFi connection *** FAILED ***");
}

void StopWiFi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi switched Off");
}

void InitialiseSystem() {
  Serial.begin(115200);
  while (!Serial);
  delay(1000);
  Serial.println(String(__FILE__) + "\nStarting...");
  display.init(115200, true, 2, false);
  //// display.init(); for older Waveshare HAT's
  SPI.end();
  SPI.begin(EPD_SCK, EPD_MISO, EPD_MOSI, EPD_CS);
  u8g2Fonts.begin(display);                  // connect u8g2 procedures to Adafruit GFX
  u8g2Fonts.setFontMode(1);                  // use u8g2 transparent mode (this is default)
  u8g2Fonts.setFontDirection(0);             // left to right (this is default)
  u8g2Fonts.setForegroundColor(GxEPD_BLACK); // apply Adafruit GFX color
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE); // apply Adafruit GFX color
  u8g2Fonts.setFont(u8g2_font_helvB10_tf);   // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
  display.setRotation(0);
  display.fillScreen(GxEPD_WHITE);
  display.setFullWindow();
}
//  775 lines of code
