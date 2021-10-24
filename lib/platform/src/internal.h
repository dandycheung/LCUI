#include "config.h"
#include <LCUI/util.h>
#include "../include/platform.h"

#ifdef LCUI_PLATFORM_WIN_DESKTOP

int ime_add_win32(void);

#endif

#ifdef LCUI_PLATFORM_LINUX

void fb_app_driver_init(app_driver_t *driver);
void fb_app_window_driver_init(app_window_driver_t *driver);

int linux_mouse_init(void);
int linux_mouse_destroy(void);
int linux_keyboard_init(void);
int linux_keyboard_destroy(void);
int ime_add_linux(void);

#ifdef USE_LIBX11

void x11_app_driver_init(app_driver_t *driver);
void x11_app_window_driver_init(app_window_driver_t *driver);

#endif

#endif
