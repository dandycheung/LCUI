/* main.c -- The main functions for the LCUI normal work
 *
 * Copyright (c) 2018, Liu chao <lc-soft@live.cn> All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of LCUI nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#define LCUI_MAIN_C
#include "config.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <LCUI.h>
#include <LCUI/app.h>
#include <LCUI/thread.h>
#include <LCUI/worker.h>
#include <LCUI/timer.h>
#include <LCUI/cursor.h>
#include <LCUI/display.h>
#include <LCUI/settings.h>
#ifdef LCUI_EVENTS_H
#include LCUI_EVENTS_H
#endif
#ifdef LCUI_MOUSE_H
#include LCUI_MOUSE_H
#endif
#ifdef LCUI_KEYBOARD_H
#include LCUI_KEYBOARD_H
#endif
#ifdef LCUI_DISPLAY_H
#include LCUI_DISPLAY_H
#endif
#include <LCUI/font.h>

#define STATE_ACTIVE 1
#define STATE_KILLED 0

typedef struct LCUI_FrameProfileRec_ {
	size_t timers_count;
	clock_t timers_time;

	size_t events_count;
	clock_t events_time;

	size_t render_count;
	clock_t render_time;
	clock_t present_time;

	ui_profile_t ui_profile;
} LCUI_FrameProfileRec, *LCUI_FrameProfile;

typedef struct LCUI_ProfileRec_ {
	clock_t start_time;
	clock_t end_time;
	unsigned frames_count;
	LCUI_FrameProfileRec frames[LCUI_MAX_FRAMES_PER_SEC];
} LCUI_ProfileRec, *LCUI_Profile;

typedef struct LCUI_MainLoopRec_ {
	int state;       /**< 主循环的状态 */
	LCUI_Thread tid; /**< 当前运行该主循环的线程的ID */
} LCUI_MainLoopRec;

/** 主循环的状态 */
enum LCUI_MainLoopState { STATE_PAUSED, STATE_RUNNING, STATE_EXITED };

/* clang-format off */

#define LCUI_WORKER_NUM 4

/** LCUI 应用程序数据 */
static struct lcui_app_t {
	int exit_code;
	step_timer_t timer;
	LCUI_Worker main_worker;		/**< 主工作线程 */
	LCUI_Worker workers[LCUI_WORKER_NUM];	/**< 普通工作线程 */
	int worker_next;			/**< 下一个工作线程编号 */
	LCUI_SettingsRec settings;
	LCUI_ProfileRec profile;
	LCUI_FrameProfile frame;
	int settings_change_handler_id;
} MainApp;

/* clang-format on */
static void LCUIProfile_Init(LCUI_Profile profile)
{
	memset(profile, 0, sizeof(LCUI_ProfileRec));
	profile->start_time = clock();
}

static void LCUIProfile_Print(LCUI_Profile profile)
{
	unsigned i;
	LCUI_FrameProfile frame;

	logger_debug("\nframes_count: %zu, time: %ld\n", profile->frames_count,
		     profile->end_time - profile->start_time);
	for (i = 0; i < profile->frames_count; ++i) {
		frame = &profile->frames[i];
		logger_debug("=== frame [%u/%u] ===\n", i + 1,
			     profile->frames_count);
		logger_debug("timers.count: %zu\ntimers.time: %ldms\n",
			     frame->timers_count, frame->timers_time);
		logger_debug("events.count: %zu\nevents.time: %ldms\n",
			     frame->events_count, frame->events_time);
		logger_debug("ui_profile.time: %ldms\n"
			     "ui_profile.update_count: %u\n"
			     "ui_profile.refresh_count: %u\n"
			     "ui_profile.layout_count: %u\n"
			     "ui_profile.user_task_count: %u\n"
			     "ui_profile.destroy_count: %u\n"
			     "ui_profile.destroy_time: %ldms\n",
			     frame->ui_profile.time,
			     frame->ui_profile.update_count,
			     frame->ui_profile.refresh_count,
			     frame->ui_profile.layout_count,
			     frame->ui_profile.user_task_count,
			     frame->ui_profile.destroy_count,
			     frame->ui_profile.destroy_time);
		logger_debug("render: %zu, %ldms, %ldms\n", frame->render_count,
			     frame->render_time, frame->present_time);
	}
}

static LCUI_FrameProfile LCUIProfile_BeginFrame(LCUI_Profile profile,
						LCUI_Settings settings)
{
	LCUI_FrameProfile frame;

	frame = &profile->frames[profile->frames_count];
	if (profile->frames_count > (unsigned)settings->frame_rate_cap) {
		profile->frames_count = 0;
	}
	memset(frame, 0, sizeof(LCUI_FrameProfileRec));
	return frame;
}

static void LCUIProfile_EndFrame(LCUI_Profile profile, LCUI_Settings settings)
{
	profile->frames_count += 1;
	profile->end_time = clock();
	if (profile->end_time - profile->start_time >= CLOCKS_PER_SEC) {
		if (profile->frames_count < (unsigned)settings->frame_rate_cap / 4) {
			LCUIProfile_Print(profile);
		}
		profile->frames_count = 0;
		profile->start_time = profile->end_time;
	}
}

static void OnSettingsChangeEvent(LCUI_SysEvent e, void *arg)
{
	Settings_Init(&MainApp.settings);
	StepTimer_SetFrameLimit(MainApp.timer, MainApp.settings.frame_rate_cap);
}

/* TODO: refactor event loop

static void lcui_convert_ui_event()
{
	LCUICursor_GetPos(&pos);
	scale = ui_get_scale();
	pos.x = y_iround(pos.x / scale);
	pos.y = y_iround(pos.y / scale);
	// keyboard
	switch (e->type) {
	case LCUI_KEYDOWN:
		e.type = UI_EVENT_KEYDOWN;
		break;
	case LCUI_KEYUP:
		e.type = UI_EVENT_KEYUP;
		break;
	case LCUI_KEYPRESS:
		e.type = UI_EVENT_KEYPRESS;
		break;
	default:
		return;
	}
	// textinput
	e.type = UI_EVENT_TEXTINPUT;
	e.text.length = e->text.length;
	e.text.text = NEW(wchar_t, e->text.length + 1);
	if (!e.text.text) {
		return;
	}
	wcsncpy(e.text.text, e->text.text, e->text.length + 1);
	ui_widget_emit_event(e.target, &e, NULL);
	free(e.text.text);
	e.text.text = NULL;
	e.text.length = 0;
	ui_event_destroy(&e);
}
*/

void LCUI_RunFrameWithProfile(LCUI_FrameProfile profile)
{
	profile->timers_time = clock();
	profile->timers_count = lcui_process_timers();
	profile->timers_time = clock() - profile->timers_time;

	profile->events_time = clock();
	profile->events_count = LCUI_ProcessEvents();
	profile->events_time = clock() - profile->events_time;

	LCUICursor_Update();
	ui_update_with_profile(&profile->ui_profile);

	profile->render_time = clock();
	LCUIDisplay_Update();
	profile->render_count = LCUIDisplay_Render();
	profile->render_time = clock() - profile->render_time;

	profile->present_time = clock();
	LCUIDisplay_Present();
	profile->present_time = clock() - profile->present_time;
}

void LCUI_RunFrame(void)
{
	lcui_process_timers();
	LCUI_ProcessEvents();
	LCUICursor_Update();
	ui_update();
	LCUIDisplay_Update();
	LCUIDisplay_Render();
	LCUIDisplay_Present();
}

static void LCUI_InitEvent(void)
{
	LCUIMutex_Init(&System.event.mutex);
	System.event.trigger = EventTrigger();
}

static void LCUI_FreeEvent(void)
{
	LCUIMutex_Destroy(&System.event.mutex);
	EventTrigger_Destroy(System.event.trigger);
	System.event.trigger = NULL;
}

static void OnEvent(LCUI_Event e, void *arg)
{
	SysEventHandler handler = e->data;
	SysEventPack pack = arg;
	pack->event->type = e->type;
	pack->event->data = handler->data;
	handler->func(pack->event, pack->arg);
}

static void DestroySysEventHandler(void *arg)
{
	SysEventHandler handler = arg;
	if (handler->data && handler->destroy_data) {
		handler->destroy_data(handler->data);
	}
	handler->data = NULL;
	free(arg);
}

int LCUI_BindEvent(int id, LCUI_SysEventFunc func, void *data,
		   void (*destroy_data)(void *))
{
	int ret;
	SysEventHandler handler;
	if (System.state != STATE_ACTIVE) {
		return -1;
	}
	handler = NEW(SysEventHandlerRec, 1);
	handler->func = func;
	handler->data = data;
	handler->destroy_data = destroy_data;
	LCUIMutex_Lock(&System.event.mutex);
	ret = EventTrigger_Bind(System.event.trigger, id, OnEvent, handler,
				DestroySysEventHandler);
	LCUIMutex_Unlock(&System.event.mutex);
	return ret;
}

int LCUI_UnbindEvent(int handler_id)
{
	int ret;
	if (System.state != STATE_ACTIVE) {
		return -1;
	}
	LCUIMutex_Lock(&System.event.mutex);
	ret = EventTrigger_Unbind2(System.event.trigger, handler_id);
	LCUIMutex_Unlock(&System.event.mutex);
	return ret;
}

int LCUI_TriggerEvent(LCUI_SysEvent e, void *arg)
{
	if (System.state != STATE_ACTIVE) {
		return -1;
	}
	int ret;
	SysEventPackRec pack;
	pack.arg = arg;
	pack.event = e;
	LCUIMutex_Lock(&System.event.mutex);
	ret = EventTrigger_Trigger(System.event.trigger, e->type, &pack);
	LCUIMutex_Unlock(&System.event.mutex);
	return ret;
}

	LCUI_SettingsRec settings;
} lcui_app;

static void on_settings_change(app_event_t *e, void *arg)
{
	Settings_Init(&lcui_app.settings);
	lcui_app.timer.target_elapsed_time = 1000.f / lcui_app.settings.frame_rate_cap;
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
	LCUI_ResetSettings();
	lcui_app.settings_change_handler_id = LCUI_BindEvent(
	    LCUI_SETTINGS_CHANGE, on_settings_change, NULL, NULL);
	Settings_Init(&lcui_app.settings);
	lcui_app.main_worker = LCUIWorker_New();
	for (i = 0; i < LCUI_WORKER_NUM; ++i) {
		lcui_app.workers[i] = LCUIWorker_New();
		LCUIWorker_RunAsync(lcui_app.workers[i]);
	}
	lcui_app.timer.target_elapsed_time = 1000.f / lcui_app.settings.frame_rate_cap;
}

static void lcui_destroy_app(void)
{
	int i;
	LCUI_MainLoop loop;
	list_node_t *node;

	LCUI_UnbindEvent(lcui_app.settings_change_handler_id);
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

#ifdef LCUI_BUILD_IN_WIN32

static void Win32Logger_LogA(const char *str)
{
	OutputDebugStringA(str);
}

static void Win32Logger_LogW(const wchar_t *wcs)
{
	OutputDebugStringW(wcs);
}

#endif

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
#ifdef LCUI_BUILD_IN_WIN32
	logger_set_handler(Win32Logger_LogA);
	logger_set_handler_w(Win32Logger_LogW);
#endif
	lcui_app.exit_code = 0;
	lcui_print_info();
	LCUI_InitFontLibrary();
	lcui_init_timers();
	LCUI_InitCursor();
	ui_init();
	lcui_init_ui_preset_widgets();
}

void lcui_init(void)
{
	lcui_init_base();
	lcui_init_app();
	app_init(L"LCUI Application");
	app_init_events();
	app_init_ime();
	switch (app_get_id()) {
	case APP_ID_LINUX_X11:
	case APP_ID_UWP:
	case APP_ID_WIN32:
		LCUICursor_Hide();
		break;
	default:
		break;
	}
}

const char *LCUI_GetVersion(void)
{
	return PACKAGE_VERSION;
}

int lcui_destroy(void)
{
	lcui_destroy_app();
	app_destroy_ime();
	LCUI_FreeKeyboard();
	ui_destroy();
	LCUI_FreeCursor();
	LCUI_FreeFontLibrary();
	lcui_destroy_timers();
	LCUI_FreeEvent();
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

static void lcui_app_on_tick(step_timer_t *timer, void *data)
{
	LCUIDisplay_Update();
	LCUIDisplay_Render();
	LCUIDisplay_Present();
}

int lcui_poll_event(app_event_t *e)
{
	LCUI_ProcessTimers();
	app_process_native_events();
	while (LCUIWorker_RunTask(lcui_app.main_worker));
	return app_poll_event(e);
}

int lcui_process_event(app_event_t *e)
{
	app_process_event(&e);
	LCUIWidget_Update();
	step_timer_tick(&lcui_app.timer, lcui_app_on_tick, NULL);
}

int lcui_main(void)
{
	LCUI_BOOL active = TRUE;
	app_event_t e = { 0 };

	while (active) {
		while (lcui_poll_event(&e)) {
			lcui_process_event(&e);
			if (e.type = APP_EVENT_QUIT) {
				active = FALSE;
				break;
			}
			app_event_destroy(&e);
		}
	}
	return lcui_destroy();
}
