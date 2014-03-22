#define SETTINGS_KEY 4

//Define window, textlayer(s), bitmap(s), and their layers
Window *window;
TextLayer *time_layer, *date_layer, *battery_text_layer, *weather_text_layer, *cover_layer, *update_at_a_glance;
GBitmap *battery_icon, *weather_icon, *bluetooth_icon, *bluetooth_connected_icon;
BitmapLayer *battery_icon_layer, *weather_icon_layer, *bluetooth_icon_layer, *bluetooth_connected_layer;

InverterLayer *invert_layer_half;

//Define font names
static GFont futuraBold, futuraLight, futuraExtended, futuraBold24;

//Define buffers for date and time
char timeBuffer[] = "00:00";
char timeBufferSeconds[] = "xx:xx:xx";
char dateBuffer[] = "Config Settings";
char temperature_buffer[32];

//Variables handling boot for clean animations
int animationNumber = 0;
bool booted = false;
bool bootedTime = 0;

//Variables for glance_this hold up
AppTimer *glanceTimer;
int holdUpSentence;
bool holdUpVibrate;
int holdUpVibrateNum;
int holdUpAnimationLength, intro, glanceTimerCalled;
bool holdUpFullNotify;
bool currentlyGlancing = 0;

GRect start, finish, finish1, finish2, finish3, finish4, finish5, finish6, finish7;

typedef struct persist {
	uint8_t tempPref;
	uint8_t batterySaveMode;
	uint8_t secondsEnabled;
	uint8_t dateFormat;
	uint8_t weatherIntervalValue;
	uint8_t bootAnimation;
	uint8_t previousTemp;
	uint8_t previousIcon;
	uint8_t disconnectWarn;
	uint8_t reconnectWarn; 
	uint8_t newVersion;
} __attribute__((__packed__)) persist;
	
persist settings = {
	.tempPref = 4,
	.batterySaveMode = 4,
	.secondsEnabled = 4,
	.dateFormat = 4,
	.weatherIntervalValue = 4,
	.bootAnimation = 4,
	.previousTemp = 1,
	.previousIcon = 1,
	.disconnectWarn = 5,
	.reconnectWarn = 5,
	.newVersion = 4,
};

int valueRead, valueWritten, valueRead2, valueWritten2;

//Key definitions 
enum {
  	WEATHER_ICON_KEY = 0x0,       
  	WEATHER_TEMPERATURE_KEY = 0x1,
  	KEY_TEMP_PREF = 0x2,
	KEY_BATTERY_SAVE = 0x3,
	KEY_WEATHER_INTERVAL = 0x4,
	KEY_DATE_FORMAT = 0x5,
	KEY_SECONDS = 0x6,
	KEY_BOOTANIMATION = 0x7,
	KEY_DISWARN = 0x8,
	KEY_REWARN = 0x9,
	KEY_WATCHAPPVER = 0x10,
};

//Other variables
AppTimer *timer;
int temperature;
int temperaturePreConvert;
int weatherInterval = 1800000;
int currentAppVer = 12;
bool newVersion, nightTime, warnedVersion;
bool versionChecked = 0;
int versionDiff, newAppVer;
int hours, minutes, seconds;

//Define weather icon resources in an array (in proper order. See JS code)
static const uint32_t WEATHER_ICONS[] = {
  RESOURCE_ID_IMAGE_THUNDER, //0
  RESOURCE_ID_IMAGE_SUN, //1
  RESOURCE_ID_IMAGE_RAIN, //2
  RESOURCE_ID_IMAGE_SNOW, //3
  RESOURCE_ID_IMAGE_FOG, //4
  RESOURCE_ID_IMAGE_SUN, //5
  RESOURCE_ID_IMAGE_TEST_WEATHER_ICON, //6
  RESOURCE_ID_IMAGE_CLOUD //7
};