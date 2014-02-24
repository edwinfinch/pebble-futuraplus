/*
Futura Plus
Written by Edwin Finch (http://www.github.com/edwinfinch/futuraplus)
Under MIT license

Credits:
Weather icons: http://maxcdn.webappers.com/img/2012/05/weather-icons.jpg
Most of the JS file for fetching weather was taken from: https://github.com/pebble/pebble-sdk-examples/tree/master/pebblekit-js/weather
OpenWeatherMap API used for weather :)
*/

#include <pebble.h>
	
#define NUM_WEATHER_TEMP_KEY 21
#define NUM_WEATHER_TEMP_DEFAULT 0
	
#define NUM_ICON_KEY 1
#define NUM_ICON_DEFAULT 0

//Define window, textlayer(s), bitmap(s), and their layers
Window *window;
TextLayer *time_layer, *date_layer, *battery_text_layer, *weather_text_layer, *cover_layer, *update_at_a_glance;
GBitmap *battery_icon, *weather_icon, *bluetooth_icon, *bluetooth_connected_icon;
BitmapLayer *battery_icon_layer, *weather_icon_layer, *bluetooth_icon_layer, *bluetooth_connected_layer;

InverterLayer *invert_layer_half;

//Define font names
static GFont futuraBold, futuraLight, futuraExtended;

//Define buffers for date and time
char timeBuffer[] = "00:00";
char dateBuffer[] = "February 31st";
char temperature_buffer[32];
char textBuffer[32];

//Variables handling boot for clean animations
int animationNumber = 0;
int weatherUpdateCount = 0;
bool booted = false;
bool bootedTime = 0;

//Define variables for persistent storage of weather
int previousTemp = NUM_WEATHER_TEMP_DEFAULT;
int previousIcon = NUM_ICON_DEFAULT;

//Weather key definitions 
enum WeatherKey {
  WEATHER_ICON_KEY = 0x0,         // TUPLE_INT
  WEATHER_TEMPERATURE_KEY = 0x1,  // TUPLE_CSTRING
  WEATHER_CITY_KEY = 0x2,         // TUPLE_CSTRING
};

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

//Define glance_this function early
void glance_this(const char *updateText, bool vibrate, int vibrateNum, int animationLength);

//static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
void process_tuple(Tuple *t)
{
	//Get key
	int key = t->key;

	//Get integer value
	int value = t->value->int32;

	//Get string value
	char string_value[32];
	strcpy(string_value, t->value->cstring);
	APP_LOG(APP_LOG_LEVEL_INFO, "Extracting values");
	
  switch (key) {
	  //If the icon has been extracted
    case WEATHER_ICON_KEY:
	  //Reset icon
      if (weather_icon) {
        gbitmap_destroy(weather_icon);
      }
	  //Set icon and layer
	  APP_LOG(APP_LOG_LEVEL_INFO, "Setting icon");
      weather_icon = gbitmap_create_with_resource(WEATHER_ICONS[t->value->uint8]);
      bitmap_layer_set_bitmap(weather_icon_layer, weather_icon);
	  previousIcon = t->value->uint8;
      break;
	  //If it's the temp.
    case WEATHER_TEMPERATURE_KEY:
	  //Set textlayer accordingly and write temp for previousTemp variable
	  snprintf(temperature_buffer, sizeof("-XX"), "%d", value);
	  APP_LOG(APP_LOG_LEVEL_INFO, "Setting weather");
      text_layer_set_text(weather_text_layer, (char*) &temperature_buffer);
	  previousTemp = value;
      break;
	  //This is here just in case we need to debug
    case WEATHER_CITY_KEY:
	  APP_LOG(APP_LOG_LEVEL_INFO, "Setting city");
      break;
  }
	//If the watchface has booted tell the user the weather has updated
	if(booted == true && weatherUpdateCount >= 1){
		glance_this("Weather updated.", 1, 2, 5000);
	}
	weatherUpdateCount++;
}

//When we get a message from the phone
static void in_received_handler(DictionaryIterator *iter, void *context) 
{
	(void) context;

	//Get data
	Tuple *t = dict_read_first(iter);
	if(t)
	{
		process_tuple(t);
	}

	//Get next
	while(t != NULL)
	{
		t = dict_read_next(iter);
		if(t)
		{
			//Process that data
			process_tuple(t);
		}
	}
}

//Send random data to JavaScript
static void send_cmd(void) {
  Tuplet value = TupletInteger(1, 1);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    return;
  }

  dict_write_tuplet(iter, &value);
  dict_write_end(iter);

  app_message_outbox_send();
}

//Define textlayerinit function, which handles init of text layers
//Saves lots of code and time
static TextLayer* textLayerInit(GRect location, GColor colour, GColor background, GTextAlignment alignment)
{
	TextLayer *layer = text_layer_create(location);
	text_layer_set_text_color(layer, colour);
	text_layer_set_background_color(layer, background);
	text_layer_set_text_alignment(layer, alignment);

	return layer;
}

//When an animation has stopped
void on_animation_stopped(Animation *anim, bool finished, void *context)
{
    //Free the memoery used by the Animation
    property_animation_destroy((PropertyAnimation*) anim);
	animationNumber++;
	
	//If the watchface time has hit it's location, set it as booted time to 
	//prevent minute change animation from firing wrongly
	if(animationNumber > 1 && bootedTime == 0){
		APP_LOG(APP_LOG_LEVEL_INFO, "Booted time.");
		bootedTime = 1;
	}
	//When all watchface elements have booted, set variables as true
	else if(animationNumber > 7 && booted == 0){
		APP_LOG(APP_LOG_LEVEL_INFO, "Booted.");
		booted = 1;
		//Debug
		//glance_this("Booted.", 0, 0, 2000);
		animationNumber = 0;
	}
}
 
void animate_layer(Layer *layer, GRect *start, GRect *finish, int duration, int delay)
{
    //Declare animation
    PropertyAnimation *anim = property_animation_create_layer_frame(layer, start, finish);
     
    //Set characteristics
    animation_set_duration((Animation*) anim, duration);
    animation_set_delay((Animation*) anim, delay);
     
    //Set stopped handler to free memory
    AnimationHandlers handlers = {
        //The reference to the stopped handler is the only one in the array
        .stopped = (AnimationStoppedHandler) on_animation_stopped
    };
    animation_set_handlers((Animation*) anim, handlers, NULL);
     
    //Start animation
    animation_schedule((Animation*) anim);
}

void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
    //Format the Time buffer string using tick_time as the time source
    strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", tick_time);
	strftime(dateBuffer, sizeof(dateBuffer), "%B %e", tick_time);
	
	//12 and 24 hour support
	if(clock_is_24h_style()){
      strftime(timeBuffer, sizeof(timeBuffer), "%H:%M", tick_time);
    }
    else{
      strftime(timeBuffer,sizeof(timeBuffer),"%I:%M",tick_time);
    }
	//Take seconds from tick_time();
	int seconds = tick_time->tm_sec;
	int minutes = tick_time->tm_min;
	
	//if the seconds is close to time-change, but not hour change
	if(seconds == 59 && minutes < 59){
		//Animate layer
	    GRect start = GRect(180, 10, 144, 35);
	    GRect finish = GRect(80, 10, 144, 35);
	    animate_layer(text_layer_get_layer(cover_layer), &start, &finish, 1000, 10);
		GRect start1 = GRect(80, 10, 144, 35);
	    GRect finish1 = GRect(180, 10, 144, 35);
	    animate_layer(text_layer_get_layer(cover_layer), &start1, &finish1, 1000, 1010);
	}
	//different hour change animation
	else if(seconds == 58 && minutes == 59){
		GRect start = GRect(180, 10, 144, 35);
	    GRect finish = GRect(0, 10, 144, 35);
	    animate_layer(text_layer_get_layer(cover_layer), &start, &finish, 2000, 10);
		GRect start1 = GRect(0, 10, 144, 35);
	    GRect finish1 = GRect(180, 10, 144, 35);
	    animate_layer(text_layer_get_layer(cover_layer), &start1, &finish1, 1000, 2010);
	}
	//If it's 45 minutes on the hour, push a request for weather to be updated
	if(minutes == 45 && seconds == 2){
		send_cmd();
	}
	
    //Change the TextLayer(s) text to show the new time and date
  	text_layer_set_text(time_layer, timeBuffer);
	text_layer_set_text(date_layer, dateBuffer);
}

void handle_bt(bool connected){
	//If the watch has disconnected
	if(connected == 0){
		//Update icon and glance_this alert
		bluetooth_connected_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DISCONNECTED_ICON);
		glance_this("Bluetooth disconnected", 1, 3, 4000);
  }
  	if(connected == 1){
		//If it's been reconnected, update icon and glance_this alert
  	    bluetooth_connected_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CONNECTED_ICON);
		glance_this("Bluetooth connected.", 1, 2, 3000);
		send_cmd();
  }
	//Set icon
	bitmap_layer_set_bitmap(bluetooth_connected_layer, bluetooth_connected_icon);
}

//Battery section
void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "100%";
//If the watchface is charging
  if (charge_state.is_charging) {
	  //Set icon to charge icon
      battery_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY_CHARGE);
	  snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
	  bitmap_layer_set_bitmap(battery_icon_layer, battery_icon);
    } else {
	  //Otherwise set icon to standard battery image
	    battery_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY);
        snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
	    bitmap_layer_set_bitmap(battery_icon_layer, battery_icon);
      }
	//Update battery text
  text_layer_set_text(battery_text_layer, battery_text);
}

//Section for initializing all layers and resources
void window_load(Window *window){
	Layer *window_layer = window_get_root_layer(window);
	futuraBold = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FUTURA_BOLD_35));
	futuraLight = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FUTURA_LIGHT_29));
	futuraExtended = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FUTURA_EXTNEDED_21));
	
	time_layer = textLayerInit(GRect(0, -300, 140, 168), GColorBlack, GColorClear, GTextAlignmentCenter);
	text_layer_set_font(time_layer, futuraBold);
  	layer_add_child(window_layer, (Layer*) time_layer);
	
	date_layer = textLayerInit(GRect(0, -300, 140, 168), GColorBlack, GColorClear, GTextAlignmentCenter);
	text_layer_set_font(date_layer, futuraLight);
	layer_add_child(window_layer, (Layer*) date_layer);
	
	cover_layer = textLayerInit(GRect(180,0,144,84), GColorBlack, GColorWhite, GTextAlignmentCenter);
	layer_add_child(window_layer, (Layer*) cover_layer);
	
	update_at_a_glance = textLayerInit(GRect(0, 300, 144, 168), GColorBlack, GColorClear, GTextAlignmentCenter);
	text_layer_set_font(update_at_a_glance, fonts_get_system_font(FONT_KEY_GOTHIC_18));
    layer_add_child(window_layer, (Layer*) update_at_a_glance);
	
	invert_layer_half = inverter_layer_create(GRect(0,0,144,84));
	layer_add_child(window_layer, (Layer*) invert_layer_half);
	
	battery_text_layer = textLayerInit(GRect(0, -300, 140, 168), GColorBlack, GColorClear, GTextAlignmentCenter);
	text_layer_set_font(battery_text_layer, futuraExtended);
	layer_add_child(window_layer, (Layer *) battery_text_layer);
	text_layer_set_text(battery_text_layer, "0");
	
	weather_text_layer = textLayerInit(GRect (0, -300, 140, 168), GColorBlack, GColorClear, GTextAlignmentCenter);
	text_layer_set_font(weather_text_layer, futuraExtended);
	layer_add_child(window_layer, (Layer *) weather_text_layer);
	snprintf(temperature_buffer, sizeof("-XX"), "%d", previousTemp);
	text_layer_set_text(weather_text_layer, temperature_buffer);
	
	bluetooth_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH_ICON);
	bluetooth_icon_layer = bitmap_layer_create(GRect(0, -300, 140, 168));
	bitmap_layer_set_background_color(bluetooth_icon_layer, GColorClear);
	bitmap_layer_set_bitmap(bluetooth_icon_layer, bluetooth_icon);
	layer_add_child(window_layer, (Layer *) bluetooth_icon_layer);
	
	battery_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATTERY);
	battery_icon_layer = bitmap_layer_create(GRect(0, -300, 140, 168));
	bitmap_layer_set_background_color(battery_icon_layer, GColorClear);
	bitmap_layer_set_bitmap(battery_icon_layer, battery_icon);
	layer_add_child(window_layer, bitmap_layer_get_layer(battery_icon_layer));
	
	weather_icon = gbitmap_create_with_resource(WEATHER_ICONS[previousIcon]);
	weather_icon_layer = bitmap_layer_create(GRect(0, -300, 140, 168));
	bitmap_layer_set_background_color(weather_icon_layer, GColorClear);
	bitmap_layer_set_bitmap(weather_icon_layer, weather_icon);
	layer_add_child(window_layer, bitmap_layer_get_layer(weather_icon_layer));
	
	bluetooth_connected_icon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DISCONNECTED_ICON);
	bluetooth_connected_layer = bitmap_layer_create(GRect(0, -300, 140, 168));
	bitmap_layer_set_background_color(bluetooth_connected_layer, GColorClear);
	layer_add_child(window_layer, (Layer *) bluetooth_connected_layer);
	
	struct tm *t;
  	time_t temp;        
  	temp = time(NULL);        
  	t = localtime(&temp);
	
	BatteryChargeState btchg = battery_state_service_peek();
    handle_battery(btchg);
	
	bool connected = bluetooth_connection_service_peek();
  	handle_bt(connected);
	
	tick_handler(t, MINUTE_UNIT);
}
//Unload resources to save memory
void window_unload(Window *window){
	text_layer_destroy(time_layer);
	text_layer_destroy(date_layer);
	text_layer_destroy(battery_text_layer);
	text_layer_destroy(weather_text_layer);
	text_layer_destroy(cover_layer);
	text_layer_destroy(update_at_a_glance);
	inverter_layer_destroy(invert_layer_half);
	bitmap_layer_destroy(battery_icon_layer);
	bitmap_layer_destroy(weather_icon_layer);
	bitmap_layer_destroy(bluetooth_icon_layer);
	bitmap_layer_destroy(bluetooth_connected_layer);
	gbitmap_destroy(battery_icon);
	gbitmap_destroy(weather_icon);
	gbitmap_destroy(bluetooth_icon);
	gbitmap_destroy(bluetooth_connected_icon);
}
//Initialize watchface
void handle_init(void) {
	window = window_create();
	//Set handlers
	window_set_window_handlers(window, (WindowHandlers) {
  		.load = window_load,
  		.unload = window_unload,
    });
	//Open app message port and it's function "in_received_handler"		 
	app_message_register_inbox_received(in_received_handler);					 
	app_message_open(512, 512);		//Large input and output buffer sizes
	send_cmd();
	
	//Set tick_handler for seconds for more accurate animations
	tick_timer_service_subscribe(SECOND_UNIT, (TickHandler) tick_handler);

	//Load previous temperature sync'd if there is one
	previousTemp = persist_exists(NUM_WEATHER_TEMP_KEY) ? persist_read_int(NUM_WEATHER_TEMP_KEY) : NUM_WEATHER_TEMP_DEFAULT;
	previousIcon = persist_exists(NUM_ICON_KEY) ? persist_read_int(NUM_ICON_KEY) : NUM_ICON_DEFAULT;
	
	//Push window
	window_stack_push(window, true);
	
	//Subscribe to check bluetooth connection
	bluetooth_connection_service_subscribe(&handle_bt);
	
	//Subscribe to watch battery status
	battery_state_service_subscribe(&handle_battery);
	
	//Animate all layers
	//Looks pretty epic
	GRect start = GRect(0, 400, 144, 168);
  	GRect finish = GRect(0, 10, 144, 168);
	GRect finish1 = GRect(-45, 126, 144, 168);
	GRect finish2 = GRect(0, 45, 140, 168);
	GRect finish3 = GRect(45,126,144,168);
	GRect finish4 = GRect(45, 25, 144, 168);
	GRect finish5 = GRect(-45, 25, 144, 168);
	GRect finish6 = GRect(0, 25, 144, 168);
	GRect finish7 = GRect(3,55,144,168);
	animate_layer(text_layer_get_layer(time_layer), &start, &finish, 1200, 210);
	animate_layer(text_layer_get_layer(date_layer), &start, &finish2, 1200, 400);
	animate_layer(bitmap_layer_get_layer(weather_icon_layer), &start, &finish4, 1200, 600);
	animate_layer(bitmap_layer_get_layer(bluetooth_icon_layer), &start, &finish6, 1200, 800);
	animate_layer(bitmap_layer_get_layer(battery_icon_layer), &start, &finish5, 1200, 1000);
	animate_layer(text_layer_get_layer(weather_text_layer), &start, &finish3, 1200, 1200);
	animate_layer(bitmap_layer_get_layer(bluetooth_connected_layer), &start, &finish7, 1200, 1400);
	animate_layer(text_layer_get_layer(battery_text_layer), &start, &finish1, 1200, 1600);
}

//Deinit animations and subscribed elements ie. bluetooth
void handle_deinit(void) {
	persist_write_int(NUM_WEATHER_TEMP_KEY, previousTemp);
	persist_write_int(NUM_ICON_KEY, previousIcon);
	animation_unschedule_all();
	bluetooth_connection_service_unsubscribe();
	tick_timer_service_unsubscribe();
	battery_state_service_unsubscribe();
	window_destroy(window);
}

int main(void) {
	handle_init();
	app_event_loop();
	handle_deinit();
}

//Function taken from my other watchface, Clean and Simple
//Takes simple info and provides wearer with some text and optional vibration
void glance_this(const char *updateText, bool vibrate, int vibrateNum, int animationLength){
	//Update the text layer to the char provided by function call
	APP_LOG(APP_LOG_LEVEL_INFO, "glance_this(); called");
	if(booted == true){
		APP_LOG(APP_LOG_LEVEL_INFO, "Watchface is booted");
		//if there's a vibration request,
		if(vibrate == true){
			APP_LOG(APP_LOG_LEVEL_INFO, "Vibration number %d", vibrateNum);
			//check what number it is and fufill accordingly.
			if(vibrateNum == 1){
				vibes_short_pulse();
		    }
			else if(vibrateNum == 2){
				vibes_double_pulse();
		    }
			else if(vibrateNum == 3){
				vibes_long_pulse();
		    }
		}
		APP_LOG(APP_LOG_LEVEL_INFO, "Animating");
		//Update text and animate update_at_a_glance layer for fancy effect
		text_layer_set_text(update_at_a_glance, updateText);
		GRect start01 = GRect(0, 300, 144, 168);
		GRect finish01 = GRect(0, 145, 144, 168);
		GRect start02 = GRect(0, 145, 144, 168);
		GRect finish02 = GRect(0, 300, 144, 168);
		animate_layer(text_layer_get_layer(update_at_a_glance), &start01, &finish01, 1000, 0);
		animate_layer(text_layer_get_layer(update_at_a_glance), &start02, &finish02, 1000, animationLength);
	}
}