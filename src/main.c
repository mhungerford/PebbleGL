#include <pebble.h>
#include "dither.h"
#include "model_stl.h"

static Window *window;
static Layer *render_layer;
static bool wireframe = false;

static uint8_t max_models = 0;
static uint8_t model_index = 0;
static uint8_t model_rotation = 0;

static bool reset_rotate = false;
static void register_timer(void* data);

//#define FPS 1

#define MAX(A,B) ((A>B) ? A : B)
#define MIN(A,B) ((A<B) ? A : B)

uint16_t frame = 0;
uint8_t* model_buf = NULL;

extern uint8_t framebuffer[FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT / 2];
static void register_timer(void* data);

static bool load_model_resource(int index) {
  if(model_buf) {
    free(model_buf);
    model_buf = NULL;
  }
  ResHandle handle = resource_get_handle(RESOURCE_ID_MODEL_1 + index);
  size_t len = resource_size(handle);
  uint8_t* buf = malloc(len);
  resource_load(handle, buf, len);
  model_buf = buf;
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  model_rotation = (model_rotation + 1) % 2;
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  wireframe = !wireframe;
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Increment the index (wrap around if necessary)
  model_index = (model_index + 1) % max_models;
  load_model_resource(model_index);
  reset_rotate = true;
  model_rotation = 0;
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

#ifdef FPS
//Currently about 3fps for filled, 4fps for wireframe
static uint16_t frame_count = 0;
#endif

static void render_opengl(Layer* layer, GContext *ctx) {
  GBitmap* bitmap = (GBitmap*)ctx;
  uint32_t* buf = (uint32_t*)bitmap->addr;

  gl_drawframe(model_buf, wireframe, model_rotation, reset_rotate);
  reset_rotate = false;

  floyd_steinberg_dither(framebuffer, (uint8_t*)buf, -1);
#ifdef FPS
  frame_count++;
#endif
  app_timer_register(10, register_timer, NULL);
}

static void register_timer(void* data) {
  layer_mark_dirty(render_layer);
}

#ifdef FPS
static void register_fps_timer(void* data) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "5s frame_count %d", frame_count);
  frame_count = 0;
  app_timer_register(5000, register_fps_timer, data);
}
#endif

static void window_load(Window *window) {
  Layer* window_layer = window_get_root_layer(window);
  render_layer = layer_create(layer_get_bounds(window_layer));
  layer_set_update_proc(render_layer, render_opengl);
  layer_add_child(window_layer, render_layer);
  register_timer(NULL);
#ifdef FPS
  register_fps_timer(NULL);
#endif
}

static void window_unload(Window *window) {}

static void init(void) {
  light_enable(true);  // Leave the backlight on

  //Discover how many images from base index
  while (resource_get_handle(RESOURCE_ID_MODEL_1 + max_models)) {
    max_models++;
  }

  window = window_create();
  //window_set_fullscreen(window, true);
  window_set_background_color(window, GColorBlack);
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true);

  load_model_resource(model_index);

  gl_init();
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  app_event_loop();
  deinit();
}
