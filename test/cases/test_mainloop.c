#include <stdlib.h>
#include <LCUI.h>
#include <LCUI/ui.h>
#include <LCUI/gui/widget/button.h>
#include <LCUI/timer.h>
#include <LCUI/input.h>
#include <LCUI/display.h>
#include "ctest.h"

static void OnRefreshScreen(void *arg)
{
	LCUIDisplay_InvalidateArea(NULL);
}

static void OnQuit(void *arg)
{
	lcui_quit();
}

static void OnBtnClick(ui_widget_t* w, ui_event_t* e, void *arg)
{
	LCUI_MainLoop loop;

	loop = LCUIMainLoop_New();
	lcui_set_timeout(10, OnRefreshScreen, NULL);
	lcui_set_timeout(50, OnQuit, NULL);
	LCUIMainLoop_Run(loop);
}

static void OnTriggerBtnClick(void *arg)
{
	LCUI_SysEventRec e;

	e.type = APP_EVENT_MOUSEDOWN;
	e.mouse.button = MOUSE_BUTTON_LEFT;
	e.mouse.x = 5;
	e.mouse.y = 5;
	LCUI_TriggerEvent(&e, NULL);

	e.type = APP_EVENT_MOUSEUP;
	LCUI_TriggerEvent(&e, NULL);
}

static void ObserverThread(void *arg)
{
	int i;
	LCUI_BOOL *exited = arg;

	for (i = 0; i < 20 && !*exited; ++i) {
		sleep_ms(100);
	}
	it_b("main loop should exit within 2000ms", *exited, TRUE);
	if (!*exited) {
		exit(-print_test_result());
		return;
	}
	LCUIThread_Exit(NULL);
}

void test_mainloop(void)
{
	LCUI_Thread tid;
	ui_widget_t* root, btn;
	LCUI_BOOL exited = FALSE;

	lcui_init();
	btn = ui_create_widget("button");
	root = ui_root();
	Button_SetText(btn, "button");
	ui_widget_on(btn, "click", OnBtnClick, NULL, NULL);
	ui_widget_append(root, btn);
	/* Observe whether the main loop has exited in a new thread */
	LCUIThread_Create(&tid, ObserverThread, &exited);
	/* Trigger the click event after the first frame is updated */
	lcui_set_timeout(50, OnTriggerBtnClick, btn);
	lcui_main();
	exited = TRUE;
	LCUIThread_Join(tid, NULL);
}
