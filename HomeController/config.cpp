#include "config.h"

char HOSTNAME[] = "HomeController";   //  hostname


char mqtt_host[64] = "";
char mqtt_port[6] = "";
char mqtt_user[32] = "";
char mqtt_pass[32] = "";
short qossub=0; // AMQTT can sub qos 0 or 1 or 2
bool isOffline = false;
bool isReboot = false;
bool isAPMode =false;
int accessory_type = 1;//homekit_accessory_category_other;


const char szfilesourceroot[] PROGMEM = "https://raw.githubusercontent.com/Yurik72/ESPHomeController/master/data";
//const char* filestodownload[] = { "/filebrowse.html",'\0' };
//String globlog;