#include "gps_parser.h"
#include <string.h>
#include <stdlib.h>

#define NMEA_MAX_LEN 120

static GPS_Data_t gps_data;
static char nmea_buffer[NMEA_MAX_LEN];
static uint16_t nmea_index = 0;

void GPS_Parser_Init(void)
{
  memset(&gps_data, 0, sizeof(GPS_Data_t));
  memset(nmea_buffer, 0, NMEA_MAX_LEN);
  nmea_index = 0;
}

GPS_Data_t* GPS_Parser_GetData(void)
{
  return &gps_data;
}

// Convert NMEA lat/long format (DDMM.MMMMM) to decimal degrees (DD.DDDDDD)
static float ConvertDegreeMinToDecimal(const char *coord, const char *dir)
{
  if (strlen(coord) == 0) return 0.0f;
  
  float raw_val = atof(coord);
  int degrees = (int)(raw_val / 100.0f);
  float minutes = raw_val - (float)(degrees * 100.0f);
  float decimal = (float)degrees + (minutes / 60.0f);
  
  if (strcmp(dir, "S") == 0 || strcmp(dir, "W") == 0)
  {
    decimal = -decimal;
  }
  return decimal;
}

static void ParseGNGGA(char *sentence)
{
  // $GNGGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
  // Fields:
  // 0: Sentence ID ($GNGGA)
  // 1: UTC Time
  // 2: Latitude
  // 3: Latitude direction (N/S)
  // 4: Longitude
  // 5: Longitude direction (E/W)
  // 6: Fix quality (0=no fix, 1=GPS, 2=DGPS)
  // 7: Number of satellites
  // 8: HDOP
  // 9: Altitude
  // 10: Altitude Unit (M)
  
  char *token;
  char *fields[20];
  int field_idx = 0;
  
  // Split sentence by comma
  token = strtok(sentence, ",");
  while (token != NULL && field_idx < 20)
  {
    fields[field_idx++] = token;
    token = strtok(NULL, ",");
  }

  if (field_idx < 11) return; // Incomplete sentence

  // 1. Time (Field 1)
  if (strlen(fields[1]) >= 6)
  {
    char hour_str[3] = { fields[1][0], fields[1][1], '\0' };
    char min_str[3]  = { fields[1][2], fields[1][3], '\0' };
    char sec_str[3]  = { fields[1][4], fields[1][5], '\0' };
    gps_data.hour = atoi(hour_str);
    gps_data.minute = atoi(min_str);
    gps_data.second = atoi(sec_str);
  }

  // 2. Fix Quality (Field 6)
  gps_data.fix_type = atoi(fields[6]);

  // 3. Satellites Tracked (Field 7)
  gps_data.satellites = atoi(fields[7]);

  if (gps_data.fix_type > 0)
  {
    // 4. Latitude (Field 2, 3)
    gps_data.latitude = ConvertDegreeMinToDecimal(fields[2], fields[3]);
    
    // 5. Longitude (Field 4, 5)
    gps_data.longitude = ConvertDegreeMinToDecimal(fields[4], fields[5]);
    
    // 6. Altitude (Field 9)
    gps_data.altitude = atof(fields[9]);
    
    gps_data.updated = 1; // Mark as updated with new valid fix
  }
}

static void ParseSentence(char *sentence)
{
  if (strncmp(sentence, "$GNGGA", 6) == 0 || strncmp(sentence, "$GPGGA", 6) == 0)
  {
    ParseGNGGA(sentence);
  }
}

void GPS_Parser_FeedChar(uint8_t c)
{
  if (c == '$')
  {
    nmea_buffer[nmea_index] = '\0';
    ParseSentence(nmea_buffer);
    nmea_index = 0;
    nmea_buffer[nmea_index++] = c;
  }
  else if (c == '\r' || c == '\n')
  {
    if (nmea_index > 0)
    {
      nmea_buffer[nmea_index] = '\0';
      ParseSentence(nmea_buffer);
      nmea_index = 0;
    }
  }
  else if (nmea_index < NMEA_MAX_LEN - 1)
  {
    nmea_buffer[nmea_index++] = c;
  }
  else
  {
    // Buffer overflow protection, reset index
    nmea_index = 0;
  }
}
