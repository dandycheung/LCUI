#include "config.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <LCUI.h>
#include <LCUI/app.h>
#include <LCUI/ui.h>
#include <LCUI/thread.h>
#include <LCUI/worker.h>
#include <LCUI/timer.h>
#include <LCUI/cursor.h>
#include <LCUI/font.h>

#define LCUI_WORKER_NUM 4

/** LCUI 应用程序数据 */
static struct lcui_app_t {
	int exit_code;
	step_timer_t timer;
	LCUI_Worker main_worker;
	LCUI_Worker workers[LCUI_WORKER_NUM];
	int worker_next;
} lcui_app;

#ifdef APP_PLATFORM_WIN_DESKTOP

static void win32_logger_log_a(const char *str)
{
	OutputDebugStringA(str);
}

static void win32_logger_log_w(const wchar_t *wcs)
{
	OutputDebugStringW(wcs);
}

#endif

const char *lcui_get_version(void)
{
	return PACKAGE_VERSION;
}

LCUI_BOOL lcui_post_task(LCUI_Task task)
{
	if (!lcui_app.main_worker) {
		return FALSE;
	}
	LCUIWorker_PostTask(lcui_app.main_worker, task);
	return TRUE;
}

void lcui_post_async_task(LCUI_Task task, int worker_id)
{
	if (worker_id < 0) {
		if (lcui_app.worker_next >= LCUI_WORKER_NUM) {
			lcui_app.worker_next = 0;
		}
		worker_id = lcui_app.worker_next;
		lcui_app.worker_next += 1;
	}
	if (worker_id >= LCUI_WORKER_NUM) {
		worker_id = 0;
	}
	LCUIWorker_PostTask(lcui_app.workers[worker_id], task);
}

int lcui_get_fps(void)
{
	return lcui_app.timer.frames_this_second;
}

void lcui_init_app(void)
{
	int i;
	step_timer_init(&lcui_app.timer);
	lcui_app.main_worker = LCUIWorker_New();
	for (i = 0; i < LCUI_WORKER_NUM; ++i) {
		lcui_app.workers[i] = LCUIWorker_New();
		LCUIWorker_RunAsync(lcui_app.workers[i]);
	}
	lcui_app.timer.target_elapsed_time = 0;
}

void lcui_set_frame_rate_cap(float rate_cap)
{
	lcui_app.timer.target_elapsed_time =
	    rate_cap > 0 ? 1000.f / rate_cap : 0;
}

static void lcui_destroy_app(void)
{
	int i;
	list_node_t *node;

	for (i = 0; i < LCUI_WORKER_NUM; ++i) {
		LCUIWorker_Destroy(lcui_app.workers[i]);
		lcui_app.workers[i] = NULL;
	}
	LCUIWorker_Destroy(lcui_app.main_worker);
	lcui_app.main_worker = NULL;
}

static void lcui_print_info(void)
{
	logger_log(LOGGER_LEVEL_INFO,
		   "LCUI (LC's UI) version " PACKAGE_VERSION "\n"
#ifdef _MSC_VER
		   "Build tool: "
#if (_MSC_VER > 1912)
		   "MS VC++ (higher version)"
#elif (_MSC_VER >= 1910 && _MSC_VER <= 1912)
		   "MS VC++ 14.1 (VisualStudio 2017)"
#elif (_MSC_VER == 1900)
		   "MS VC++ 14.0 (VisualStudio 2015)"
#elif (_MSC_VER == 1800)
		   "MS VC++ 12.0 (VisualStudio 2013)"
#elif (_MSC_VER == 1700)
		   "MS VC++ 11.0 (VisualStudio 2012)"
#elif (_MSC_VER == 1600)
		   "MS VC++ 10.0 (VisualStudio 2010)"
#else
		   "MS VC++ (older version)"
#endif
		   "\n"
#endif
		   "Build at "__DATE__
		   " - "__TIME__
		   "\n"
		   "Copyright (C) 2012-2021 Liu Chao <root@lc-soft.io>.\n"
		   "This is open source software, licensed under MIT. \n"
		   "See source distribution for detailed copyright notices.\n"
		   "To learn more, visit http://www.lcui.org.\n\n");
}

static void lcui_app_on_tick(step_timer_t *timer, void *data)
{
	LCUIDisplay_Update();
	LCUIDisplay_Render();
	LCUIDisplay_Present();
}

int lcui_poll_event(app_event_t *e)
{
	lcui_process_timers();
	app_process_native_events();
	while (LCUIWorker_RunTask(lcui_app.main_worker))
		;
	return app_poll_event(e);
}

static void lcui_dispatch_ui_mouse_event(ui_event_type_t type,
					 app_event_t *app_evt)
{
	ui_event_t e = { 0 };
	float scale = ui_get_scale();

	e.type = type;
	e.mouse.y = y_iround(app_evt->mouse.y / scale);
	e.mouse.x = y_iround(app_evt->mouse.x / scale);
	ui_dispatch_event(&e);
}

static void lcui_dispatch_ui_keyboard_event(ui_event_type_t type,
					    app_event_t *app_evt)
{
	ui_event_t e = { 0 };

	e.type = type;
	e.key.code = app_evt->key.code;
	e.key.is_composing = app_evt->key.is_composing;
	e.key.alt_key = app_evt->key.alt_key;
	e.key.shift_key = app_evt->key.shift_key;
	e.key.ctrl_key = app_evt->key.ctrl_key;
	e.key.meta_key = app_evt->key.meta_key;
	ui_dispatch_event(&e);
}

static void lcui_dispatch_ui_touch_event(app_touch_event_t *touch_ev)
{
	size_t i;
	float scale;
	ui_event_t e = { 0 };

	scale = ui_get_scale();
	e.type = UI_EVENT_TOUCH;
	e.touch.n_points = touch_ev->n_points;
	e.touch.points = malloc(sizeof(ui_touch_point_t) * e.touch.n_points);
	for (i = 0; i < e.touch.n_points; ++i) {
		switch (touch_ev->points[i].state) {
		case APP_EVENT_TOUCHDOWN:
			e.touch.points[i].state = UI_EVENT_TOUCHDOWN;
			break;
		case APP_EVENT_TOUCHUP:
			e.touch.points[i].state = UI_EVENT_TOUCHUP;
			break;
		case APP_EVENT_TOUCHMOVE:
			e.touch.points[i].state = UI_EVENT_TOUCHMOVE;
			break;
		default:
			break;
		}
		e.touch.points[i].x = y_iround(touch_ev->points[i].x / scale);
		e.touch.points[i].y = y_iround(touch_ev->points[i].y / scale);
	}
	ui_dispatch_event(&e);
	ui_event_destroy(&e);
}

static void lcui_dispatch_ui_textinput_event(app_event_t *app_evt)
{
	ui_event_t e = { 0 };

	e.type = UI_EVENT_TEXTINPUT;
	e.text.length = app_evt->text.length;
	e.text.text = wcsdup2(app_evt->text.text);
	ui_dispatch_event(&e);
	ui_event_destroy(&e);
}

static void lcui_dispatch_ui_wheel_event(app_wheel_event_t *wheel)
{
	ui_event_t e = { 0 };

	// TODO:
	e.type = UI_EVENT_WHEEL;
	e.wheel.delta_mode = UI_WHEEL_DELTA_PIXEL;
	e.wheel.delta_y = wheel->delta_y;
	ui_dispatch_event(&e);
}

static void lcui_dispatch_ui_event(app_event_t *app_event)
{
	// keyboard
	switch (app_event->type) {
	case APP_EVENT_KEYDOWN:
		lcui_dispatch_ui_keyboard_event(UI_EVENT_KEYDOWN, app_event);
		return;
	case APP_EVENT_KEYUP:
		lcui_dispatch_ui_keyboard_event(UI_EVENT_KEYUP, app_event);
		return;
	case APP_EVENT_KEYPRESS:
		lcui_dispatch_ui_keyboard_event(UI_EVENT_KEYPRESS, app_event);
		return;
	case APP_EVENT_MOUSEDOWN:
		lcui_dispatch_ui_mouse_event(UI_EVENT_MOUSEDOWN, app_event);
		return;
	case APP_EVENT_MOUSEUP:
		lcui_dispatch_ui_mouse_event(UI_EVENT_MOUSEUP, app_event);
		return;
	case APP_EVENT_MOUSEMOVE:
		lcui_dispatch_ui_mouse_event(UI_EVENT_MOUSEMOVE, app_event);
		return;
	case APP_EVENT_TOUCH:
		lcui_dispatch_ui_touch_event(&app_event->touch);
		return;
	case APP_EVENT_WHEEL:
		lcui_dispatch_ui_wheel_event(&app_event->wheel);
	case APP_EVENT_COMPOSITION:
		lcui_dispatch_ui_textinput_event(app_event);
	default:
		break;
	}
}

int lcui_process_event(app_event_t *e)
{
	app_process_event(e);
	if (e->type = APP_EVENT_QUIT) {
		return -1;
	}
	lcui_dispatch_ui_event(e);
	ui_process_events();
	step_timer_tick(&lcui_app.timer, lcui_app_on_tick, NULL);
	return 0;
}

void lcui_init_ui_preset_widgets(void)
{
	LCUIWidget_AddTextView();
	LCUIWidget_AddCanvas();
	LCUIWidget_AddAnchor();
	LCUIWidget_AddButton();
	LCUIWidget_AddSideBar();
	LCUIWidget_AddTScrollBar();
	LCUIWidget_AddTextCaret();
	LCUIWidget_AddTextEdit();
}

void lcui_init_base(void)
{
#ifdef APP_PLATFORM_WIN_DESKTOP
	logger_set_handler(win32_logger_log_a);
	logger_set_handler_w(win32_logger_log_w);
#endif
	lcui_app.exit_code = 0;
	lcui_print_info();
	LCUI_InitFontLibrary();
	lcui_init_timers();
	LCUI_InitCursor();
	LCUI_InitWidget();
	lcui_init_ui_preset_widgets();
	lcui_reset_settings();
}

void lcui_init(void)
{
	lcui_init_base();
	lcui_init_app();
	app_init(L"LCUI Application");
}

int lcui_destroy(void)
{
	lcui_destroy_app();
	LCUI_FreeWidget();
	LCUI_FreeFontLibrary();
	lcui_destroy_timers();
	app_destroy();
	return lcui_app.exit_code;
}

void lcui_quit(void)
{
	app_event_t e = { 0 };
	e.type = APP_EVENT_QUIT;
	app_post_event(&e);
}

void lcui_exit(int code)
{
	lcui_app.exit_code = code;
	lcui_quit();
}

int lcui_main(void)
{
	LCUI_BOOL active = TRUE;
	app_event_t e = { 0 };

	while (active) {
		while (lcui_poll_event(&e)) {
			if (lcui_process_event(&e) != 0) {
				active = FALSE;
				break;
			}
			app_event_destroy(&e);
		}
	}
	return lcui_destroy();
}
