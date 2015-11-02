#include <pebble.h>
  
//-------------------------------//
#define BACKGROUND_DARK true //true: black background; false: white background
#define FILL_COLOR GColorOrange //the fill color of a cell; only if colors are possible, else inverted to background
#define BORDER_COLOR GColorLightGray //the border color of a cell
#define RELATIVE_CORNER_RADIUS 0.5 //border radius in ralation to half of the cell's dimension [0.0 -> rect; 1.0 -> circle]
#define BORDER_WIDTH 2 //border width in pixels
#define BORDER_PADDING 1 //border padding to cell in pixels
#define VERTICAL_SPACE 3 //the vertical space between two cells in pixels
#define HORIZONTAL_SPACE 10 //the horizontal space between two cells in pixels
#define HAS_BORDER true //defines if a border is drawn around a single cell (overritten by activated IS_BORDER_DATE)
#define IS_BORDER_DATE true //defines if the border represents the current date (if activated overrides HAS_BORDER)
//-------------------------------// 
 
#define WINDOW_WIDTH 144
#define WINDOW_HEIGHT 168
  
#define DATE_TYPE 0
#define TIME_TYPE 1
  
#define MAX_ROWS 6
#define YEAR_MAX_ROWS 6
#define MONTH_MAX_ROWS 4
#define DAY_MAX_ROWS 6
#define HOURS_MAX_ROWS 5
#define MINUTES_MAX_ROWS 6
#define SECONDS_MAX_ROWS 6

#define COLS 3
#define YEAR_COL 2
#define MONTH_COL 1
#define DAY_COL 0
#define HOURS_COL 0
#define MINUTES_COL 1
#define SECONDS_COL 2

#define DATE_CELL_SIZE (WINDOW_HEIGHT - ((MAX_ROWS - 1) * VERTICAL_SPACE)) / MAX_ROWS
#define TIME_CELL_SIZE DATE_CELL_SIZE - (2 * (BORDER_WIDTH + BORDER_PADDING))
#define HORIZONTAL_SIDE_PADDING (WINDOW_WIDTH - (COLS * DATE_CELL_SIZE + (COLS - 1) * HORIZONTAL_SPACE)) / 2

static Window *s_main_window;
static Layer *s_display_layer;

static void draw_date_cell(GContext *ctx, GPoint center, bool filled) {
  if (filled) {
    #ifdef PBL_COLOR
      graphics_context_set_fill_color(ctx, BORDER_COLOR);
    #else
      graphics_context_set_fill_color(ctx, BACKGROUND_DARK ? GColorWhite : GColorBlack);
    #endif
    int d = ((int) DATE_CELL_SIZE)/2;
    graphics_fill_rect(ctx, GRect(center.x - d, center.y - d, DATE_CELL_SIZE, DATE_CELL_SIZE), d * RELATIVE_CORNER_RADIUS, GCornersAll);
  
		graphics_context_set_fill_color(ctx, BACKGROUND_DARK ? GColorBlack : GColorWhite);
		d = ((int) DATE_CELL_SIZE)/2 - BORDER_WIDTH;
    graphics_fill_rect(ctx, GRect(center.x - d, center.y - d, DATE_CELL_SIZE - (BORDER_WIDTH * 2), DATE_CELL_SIZE - (BORDER_WIDTH * 2)), d * RELATIVE_CORNER_RADIUS, GCornersAll);
	}
}

static void draw_time_cell(GContext *ctx, GPoint center, bool filled) {
  if (!IS_BORDER_DATE && HAS_BORDER) {
    draw_date_cell(ctx, center, true);
  }
  if (filled) {
    #ifdef PBL_COLOR
      graphics_context_set_fill_color(ctx, FILL_COLOR);
    #else
      graphics_context_set_fill_color(ctx, BACKGROUND_DARK ? GColorWhite : GColorBlack);
    #endif
  } else {
    graphics_context_set_fill_color(ctx, BACKGROUND_DARK ? GColorBlack : GColorWhite);
  }
  int d = ((int) TIME_CELL_SIZE)/2;
  graphics_fill_rect(ctx, GRect(center.x - d, center.y - d, TIME_CELL_SIZE, TIME_CELL_SIZE), d * RELATIVE_CORNER_RADIUS, GCornersAll);
}

static GPoint get_center_point_from_cell_location(unsigned short i, unsigned short j) {
  uint8_t x = HORIZONTAL_SIDE_PADDING + i * (DATE_CELL_SIZE + HORIZONTAL_SPACE) + DATE_CELL_SIZE/2;
  uint8_t y = WINDOW_HEIGHT - ((j+0.5) * (DATE_CELL_SIZE + VERTICAL_SPACE));
  return GPoint(x, y);
}

static void draw_col(GContext *ctx, unsigned short digit, unsigned short type, unsigned short col, unsigned short max_rows) {
  for (int row = 0; row < max_rows; row++) {
    (type == DATE_TYPE) ? 
      draw_date_cell(ctx, get_center_point_from_cell_location(col, row), (digit >> row) & 0x1) :
      draw_time_cell(ctx, get_center_point_from_cell_location(col, row), (digit >> row) & 0x1);
  }
}

static unsigned short get_display_hour(unsigned short hour) {
  if (clock_is_24h_style()) {
    return hour;
  }
  unsigned short display_hour = hour % 12;
  return display_hour ? display_hour : 12;
}

static void display_layer_update_callback(Layer *layer, GContext *ctx) { 
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  
  draw_col(ctx, t->tm_year%100, DATE_TYPE, YEAR_COL, YEAR_MAX_ROWS);
  draw_col(ctx, t->tm_mon + 1, DATE_TYPE, MONTH_COL, MONTH_MAX_ROWS);
  draw_col(ctx, t->tm_mday, DATE_TYPE, DAY_COL, DAY_MAX_ROWS);

  draw_col(ctx, get_display_hour(t->tm_hour), TIME_TYPE, HOURS_COL, HOURS_MAX_ROWS);
  draw_col(ctx, t->tm_min, TIME_TYPE, MINUTES_COL, MINUTES_MAX_ROWS);
  draw_col(ctx, t->tm_sec, TIME_TYPE, SECONDS_COL, SECONDS_MAX_ROWS);
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_display_layer);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);

  s_display_layer = layer_create(bounds);
  layer_set_update_proc(s_display_layer, display_layer_update_callback);
  layer_add_child(window_layer, s_display_layer);

  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
}

static void main_window_unload(Window *window) {
  layer_destroy(s_display_layer);
}

static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, BACKGROUND_DARK ? GColorBlack : GColorWhite);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}