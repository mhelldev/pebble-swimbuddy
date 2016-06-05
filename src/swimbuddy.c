#include <pebble.h>

static Window *window;
static Window *s_choices_window;
static Window *s_workout_window;
static Window *s_history_window;
static Window *s_save_window;
static Window *s_customsize_window;

ActionBarLayer *action_bar;
ActionBarLayer *action_bar_save;
ActionBarLayer *action_bar_customsize;

static TextLayer *text_layer;
static TextLayer *text_layer_title;
static TextLayer *text_layer_lapCounter;
static TextLayer *text_layer_timeCounter;
static TextLayer *text_layer_distance;
static TextLayer *text_layer_pace;
static TextLayer *text_layer_speed;
static TextLayer *text_layer_save;
static TextLayer *text_layer_custommsize;

static GBitmap *s_bitmap_main_logo;
static BitmapLayer *s_bitmap_main_logo_layer;
static GBitmap *s_bitmap_action_workout;
static GBitmap *s_bitmap_action_history;
static GBitmap *s_bitmap_action_information;
static GBitmap *s_bitmap_action_savenexit;
static GBitmap *s_bitmap_action_exit;
static GBitmap *s_bitmap_action_customsize_up;
static GBitmap *s_bitmap_action_customsize_down;

static int poolSize = 25;
static int lapCount = 0;
static int timeCount = 0;
static int numberOfWorkouts = 0;
static int customPoolSize = 0;

static char workout_date[100][16];
static int workout_distance[100];
static int workout_time[100];
static char menu_history_subtext[100][25];

/********************************
 *********** MENU ***************
 ********************************/
#define NUM_MENU_SECTIONS 1
#define NUM_FIRST_MENU_ITEMS 3
static SimpleMenuLayer *s_simple_menu_layer;
static SimpleMenuSection s_menu_sections[NUM_MENU_SECTIONS];
static SimpleMenuItem s_first_menu_items[NUM_FIRST_MENU_ITEMS];

#define NUM_HISTORY_MENU_SECTIONS 1
#define NUM_HISTORY_FIRST_MENU_ITEMS 100
static SimpleMenuLayer *s_history_menu_layer;
static SimpleMenuSection s_history_menu_sections[NUM_HISTORY_MENU_SECTIONS];
static SimpleMenuItem s_history_first_menu_items[NUM_HISTORY_FIRST_MENU_ITEMS];

/********************************
 *********** MENU END ***********
 ********************************/

/********************************
 *********** ACTIONS ************
 ********************************/
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
   window_stack_push(s_choices_window, true);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
   window_stack_push(s_history_window, true);
}

static void any_click_handler_workout(ClickRecognizerRef recognizer, void *context) {
   vibes_short_pulse();
   static char s_body_text[18];
   snprintf(s_body_text, sizeof(s_body_text), "%d", ++lapCount);
   text_layer_set_text(text_layer_lapCounter, s_body_text);
   static char s_body_text_distance[18];
   snprintf(s_body_text_distance, sizeof(s_body_text_distance), " %d m", lapCount * poolSize);
   text_layer_set_text(text_layer_distance, s_body_text_distance);
   static char s_body_text_speed[18];
   if(timeCount > 0) {  
     snprintf(s_body_text_speed, sizeof(s_body_text_speed), " %d kmh", (int)(lapCount * poolSize / 1000.0 / timeCount * 60.0 * 60.0));
   }
   text_layer_set_text(text_layer_speed, s_body_text_speed);
   static char s_body_text_pace[18];
   if(lapCount * poolSize > 0) {  
     snprintf(s_body_text_pace, sizeof(s_body_text_pace), " %d sec/100m", (int)((timeCount / ((float)lapCount * poolSize)) * 100.0));
   }
   text_layer_set_text(text_layer_pace, s_body_text_pace);
}

static void exit_click_handler_workout(ClickRecognizerRef recognizer, void *context) {
    window_stack_push(s_save_window, true);
}
static void up_click_handler_save(ClickRecognizerRef recognizer, void *context) {
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
  static char date_buffer[16];
  strftime(date_buffer, sizeof(date_buffer), "%a %d %b", tick_time);
  persist_write_string(3 * numberOfWorkouts, date_buffer);
  persist_write_int(3 * numberOfWorkouts + 1, lapCount * poolSize);
  persist_write_int(3 * numberOfWorkouts + 2, timeCount);
  persist_write_int(88888,++numberOfWorkouts);

  
  window_stack_pop(s_save_window);
  window_stack_pop(s_workout_window);
  window_stack_pop(s_choices_window);
}
static void down_click_handler_save(ClickRecognizerRef recognizer, void *context) {
    window_stack_pop(s_save_window);
    window_stack_pop(s_workout_window);
    window_stack_pop(s_choices_window);
}

static void refreshCustomPoolSize() {
    persist_write_int(99999, customPoolSize);
    static char s_body_text[26];
    snprintf(s_body_text, sizeof(s_body_text), "%d m", customPoolSize);  
    text_layer_set_text(text_layer_custommsize, s_body_text);
}

static void click_handler_customsize_up(ClickRecognizerRef recognizer, void *context) {
    customPoolSize++;
    refreshCustomPoolSize();
}

static void click_handler_customsize_down(ClickRecognizerRef recognizer, void *context) {
    customPoolSize--;
    refreshCustomPoolSize();
}

static void click_handler_customsize_select(ClickRecognizerRef recognizer, void *context) {
    poolSize = customPoolSize;
    window_stack_pop(s_customsize_window);
    window_stack_push(s_workout_window, true);
}



static void menu_select_callback(int index, void *ctx) {
   switch(index) {
     case 0:
       poolSize = 25;
     break;
     case 1:
       poolSize = 50;
     break;
     case 2:
       window_stack_push(s_customsize_window, true);
       return;
     default:
       poolSize = 25;
     break;
   }
   window_stack_push(s_workout_window, true);
}

static void click_config_provider_save(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler_save);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler_save);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void click_config_provider_workout(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, any_click_handler_workout);
  window_single_click_subscribe(BUTTON_ID_UP, any_click_handler_workout);
  window_single_click_subscribe(BUTTON_ID_DOWN, any_click_handler_workout);
  window_single_click_subscribe(BUTTON_ID_BACK, exit_click_handler_workout);
}

static void click_config_provider_customsize(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, click_handler_customsize_up);
  window_single_click_subscribe(BUTTON_ID_DOWN, click_handler_customsize_down);
  window_single_click_subscribe(BUTTON_ID_SELECT, click_handler_customsize_select);
}
/********************************
 *********** ACTIONS END ********
 ********************************/

/********************************
 *********** TIMER **************
 ********************************/
static void tick_handler(struct tm *tick_timer, TimeUnits units_changed) {
   static char s_body_text[26];
  ++timeCount;
   snprintf(s_body_text, sizeof(s_body_text), " %.2d:%.2d:%.2d",
            timeCount/3600, timeCount/60, timeCount%60);
   text_layer_set_text(text_layer_timeCounter, s_body_text);
}

static void worker_init() {
  // Use the TickTimer Service as a data source
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void worker_deinit() {
  // Stop using the TickTimerService
  tick_timer_service_unsubscribe();
}
/********************************
 *********** TIMER END **********
 ********************************/

/********************************
 *********** WINDOWS ************
 ********************************/
static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Initialize the action bar:
  action_bar = action_bar_layer_create();
  //action_bar_layer_set_background_color(action_bar, GColorWhite);
  // Associate the action bar with the window:
  action_bar_layer_add_to_window(action_bar, window);
  // Set the click config provider:
  action_bar_layer_set_click_config_provider(action_bar,
                                             click_config_provider);
  // logo
  s_bitmap_main_logo = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO);
  s_bitmap_main_logo_layer = bitmap_layer_create(GRect(6, 20, 100, 140));
  bitmap_layer_set_compositing_mode(s_bitmap_main_logo_layer, GCompOpSet);
  bitmap_layer_set_bitmap(s_bitmap_main_logo_layer, s_bitmap_main_logo);
  layer_add_child(window_get_root_layer(window),bitmap_layer_get_layer(s_bitmap_main_logo_layer));
  
  text_layer_title = text_layer_create(GRect(0, bounds.size.h - 40, bounds.size.w - 30, 40));  
  text_layer_set_text(text_layer_title, "Swim Buddy");
  text_layer_set_text_alignment(text_layer_title, GTextAlignmentCenter);
  text_layer_set_font(text_layer_title, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(text_layer_title));
  
  // action bar images
  s_bitmap_action_workout = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_WORKOUT);
  action_bar_layer_set_icon_animated(action_bar, BUTTON_ID_SELECT, s_bitmap_action_workout, true);
  s_bitmap_action_history = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HISTORY);
  action_bar_layer_set_icon_animated(action_bar, BUTTON_ID_UP, s_bitmap_action_history, true);
  s_bitmap_action_information = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_INFORMATION);
  
  #ifdef PBL_COLOR
    window_set_background_color(window, GColorPictonBlue);
  #else
    window_set_background_color(window, GColorWhite);
  #endif
   
  
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void workout_window_load(Window *window) {
  worker_init();
  timeCount = 0;
  lapCount = 0;
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  text_layer_lapCounter = text_layer_create(GRect(0, 58, bounds.size.w, 60));  
  text_layer_set_text(text_layer_lapCounter, "0");
  text_layer_set_font(text_layer_lapCounter, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(text_layer_lapCounter, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_lapCounter));
  
  text_layer_timeCounter = text_layer_create(GRect(0, 0, bounds.size.w, 30));  
  text_layer_set_text(text_layer_timeCounter, " 00:00:00");
  text_layer_set_font(text_layer_timeCounter, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(text_layer_timeCounter, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_timeCounter));
  
  text_layer_distance = text_layer_create(GRect(0, 30, bounds.size.w, 30));  
  text_layer_set_text(text_layer_distance, " 0 m");
  text_layer_set_font(text_layer_distance, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(text_layer_distance, GTextAlignmentLeft);
  text_layer_set_background_color(text_layer_distance, GColorBlack);
  text_layer_set_text_color(text_layer_distance, GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_distance));
  
  text_layer_speed = text_layer_create(GRect(0, bounds.size.h - 60, bounds.size.w, 30));  
  text_layer_set_text(text_layer_speed, " 0 kmh");
  text_layer_set_font(text_layer_speed, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(text_layer_speed, GTextAlignmentLeft);
  text_layer_set_background_color(text_layer_speed, GColorBlack);
  text_layer_set_text_color(text_layer_speed, GColorWhite);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_speed));
  
  text_layer_pace = text_layer_create(GRect(0, bounds.size.h - 30, bounds.size.w, 30));  
  text_layer_set_text(text_layer_pace, " 0 sec/100m");
  text_layer_set_font(text_layer_pace, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(text_layer_pace, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_pace));
  
  window_set_click_config_provider(s_workout_window, click_config_provider_workout);

}

static void workout_window_unload(Window *window) {
  worker_deinit();
}

static void choices_window_load(Window *window) {
  int num_a_items = 0;
  s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "25m",
    .callback = menu_select_callback,
  };
  s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "50m",
    .callback = menu_select_callback,
  };
  s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "other ...",
    .callback = menu_select_callback,
  };

  s_menu_sections[0] = (SimpleMenuSection) {
    .num_items = NUM_FIRST_MENU_ITEMS,
    .items = s_first_menu_items,
    .title = "Poolsize",
  };

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  s_simple_menu_layer = simple_menu_layer_create(
    bounds, window, s_menu_sections, NUM_MENU_SECTIONS, NULL);
  layer_add_child(window_layer, simple_menu_layer_get_layer(s_simple_menu_layer));
  
  #ifdef PBL_COLOR
    menu_layer_set_highlight_colors(simple_menu_layer_get_menu_layer(
      s_simple_menu_layer), GColorPictonBlue, GColorWhite);
  #else
    menu_layer_set_highlight_colors(simple_menu_layer_get_menu_layer(
      s_simple_menu_layer), GColorBlack, GColorWhite);
  #endif
   
}

static void choices_window_unload(Window *window) {
  
}

static void save_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  text_layer_save = text_layer_create(GRect(0, 20, bounds.size.w - 30, bounds.size.h));  
  text_layer_set_text_alignment(text_layer_save, GTextAlignmentCenter);
  text_layer_set_text(text_layer_save, "Do you want to save your workout?");
  layer_add_child(window_layer, text_layer_get_layer(text_layer_save));
  
  // Initialize the action bar:
  action_bar_save = action_bar_layer_create();
  // Associate the action bar with the window:
  action_bar_layer_add_to_window(action_bar_save, window);
  // Set the click config provider:
  action_bar_layer_set_click_config_provider(action_bar_save,
                                             click_config_provider_save);
  // action bar images
  s_bitmap_action_savenexit = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SAVE);
  action_bar_layer_set_icon_animated(action_bar_save, BUTTON_ID_UP, s_bitmap_action_savenexit, true);
  s_bitmap_action_exit = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DELETE);
  action_bar_layer_set_icon_animated(action_bar_save, BUTTON_ID_DOWN, s_bitmap_action_exit, true);
}

static void save_window_unload(Window *window) {
  
}

static void history_window_load(Window *window) {
  for (int i=0; i < numberOfWorkouts && i < NUM_HISTORY_FIRST_MENU_ITEMS; i++) {
      persist_read_string(3 * i, workout_date[i], 16);
      workout_distance[i] = persist_read_int(3 * i + 1);
      workout_time[i] = persist_read_int(3 * i + 2);
      /*int tempPace = 0;
      if(workout_distance[i] > 0) {  
         tempPace = (int)((timeCount / ((float)workout_distance[i])) * 100.0);
      }*/
    
      snprintf(menu_history_subtext[i], sizeof(menu_history_subtext[i]), "%d m   %.2d:%.2d:%.2d", workout_distance[i],
               workout_time[i]/3600, workout_time[i]/60, workout_time[i]%60);
    
      s_history_first_menu_items[(numberOfWorkouts-1) - i] = (SimpleMenuItem) {
      .title = workout_date[i],
      .subtitle = menu_history_subtext[i],
    };
  }

  s_history_menu_sections[0] = (SimpleMenuSection) {
    .num_items = NUM_HISTORY_FIRST_MENU_ITEMS,
    .items = s_history_first_menu_items,
  };

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  s_history_menu_layer = simple_menu_layer_create(bounds, window, s_history_menu_sections, NUM_HISTORY_MENU_SECTIONS, NULL);
  layer_add_child(window_layer, simple_menu_layer_get_layer(s_history_menu_layer));
  
   #ifdef PBL_COLOR
    menu_layer_set_highlight_colors(simple_menu_layer_get_menu_layer(
      s_history_menu_layer), GColorPictonBlue, GColorWhite);
  #else
    menu_layer_set_highlight_colors(simple_menu_layer_get_menu_layer(
      s_history_menu_layer), GColorBlack, GColorWhite);
  #endif
}

static void history_window_unload(Window *window) {
  
}

static void customsize_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  text_layer_custommsize = text_layer_create(GRect(0, 20, bounds.size.w - 30, bounds.size.h));  
  text_layer_set_text_alignment(text_layer_custommsize, GTextAlignmentCenter);
  text_layer_set_font(text_layer_custommsize, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  
  static char s_body_text[26];
  if (customPoolSize <= 0) {
      customPoolSize = 25;
  }
  snprintf(s_body_text, sizeof(s_body_text), "%d m", customPoolSize);  
  text_layer_set_text(text_layer_custommsize, s_body_text);
  layer_add_child(window_layer, text_layer_get_layer(text_layer_custommsize));
  
  // Initialize the action bar:
  action_bar_customsize = action_bar_layer_create();
  // Associate the action bar with the window:
  action_bar_layer_add_to_window(action_bar_customsize, window);
  // Set the click config provider:
  action_bar_layer_set_click_config_provider(action_bar_customsize,
                                             click_config_provider_customsize);
  // action bar images
  s_bitmap_action_customsize_up = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_UP);
  action_bar_layer_set_icon_animated(action_bar_customsize, BUTTON_ID_UP, s_bitmap_action_customsize_up, true);
  s_bitmap_action_customsize_down = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DOWN);
  action_bar_layer_set_icon_animated(action_bar_customsize, BUTTON_ID_DOWN, s_bitmap_action_customsize_down, true);
  s_bitmap_action_workout = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_WORKOUT);
  action_bar_layer_set_icon_animated(action_bar_customsize, BUTTON_ID_SELECT, s_bitmap_action_workout, true);
}

static void customsize_window_unload(Window *window) {
  
}

/********************************
 *********** WINDOWS END ********
 ********************************/

static void init(void) {
  numberOfWorkouts = persist_read_int(88888);
  customPoolSize = persist_read_int(99999);
  window = window_create();
  s_choices_window = window_create();
  s_workout_window = window_create();
  s_save_window = window_create(); 
  s_history_window = window_create();
  s_customsize_window = window_create();
  
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_set_window_handlers(s_choices_window, (WindowHandlers) {
    .load = choices_window_load,
    .unload = choices_window_unload,
  });
  window_set_window_handlers(s_workout_window, (WindowHandlers) {
    .load = workout_window_load,
    .unload = workout_window_unload,
  });
  window_set_window_handlers(s_save_window, (WindowHandlers) {
    .load = save_window_load,
    .unload = save_window_unload,
  });
  window_set_window_handlers(s_history_window, (WindowHandlers) {
    .load = history_window_load,
    .unload = history_window_unload,
  });
  window_set_window_handlers(s_customsize_window, (WindowHandlers) {
    .load = customsize_window_load,
    .unload = customsize_window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  window_destroy(window);
  window_destroy(s_choices_window);
  window_destroy(s_workout_window);
  window_destroy(s_save_window);
  window_destroy(s_history_window);
  window_destroy(s_customsize_window);
  
  gbitmap_destroy(s_bitmap_main_logo);
  gbitmap_destroy(s_bitmap_action_workout);
  gbitmap_destroy(s_bitmap_action_savenexit);
  gbitmap_destroy(s_bitmap_action_exit);
  gbitmap_destroy(s_bitmap_action_customsize_up);
  gbitmap_destroy(s_bitmap_action_customsize_down);
  bitmap_layer_destroy(s_bitmap_main_logo_layer);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}