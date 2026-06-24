#ifndef __GPS_PARSER_H
#define __GPS_PARSER_H

#include "main.h"

typedef struct {
  float latitude;   // Latitude in decimal degrees
  float longitude;  // Longitude in decimal degrees
  float altitude;   // GPS altitude in meters
  uint8_t fix_type; // 0=No fix, 1=GPS fix, 2=DGPS, etc.
  uint8_t satellites; // Number of satellites tracked
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t updated;  // Set to 1 when a new valid fix sentence is parsed
} GPS_Data_t;

void GPS_Parser_Init(void);
void GPS_Parser_FeedChar(uint8_t c);
GPS_Data_t* GPS_Parser_GetData(void);

#endif /* __GPS_PARSER_H */
