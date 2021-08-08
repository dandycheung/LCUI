#include "config.h"
#include <app.h>

static struct lcui_t {
	int exit_code;
} lcui;

const char *lcui_get_version(void)
{
	return PACKAGE_VERSION;
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

void lcui_init_base(void)
{
#ifdef APP_PLATFORM_WIN_DESKTOP
	logger_set_handler(win32_logger_log_a);
	logger_set_handler_w(win32_logger_log_w);
#endif
	lcui.exit_code = 0;
	lcui_print_info();
	LCUI_InitFontLibrary();
	LCUI_InitTimer();
	LCUI_InitCursor();
	LCUI_InitWidget();
	LCUI_InitMetrics();
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
	LCUI_FreeTimer();
	app_destroy();
	return lcui.exit_code;
}

void lcui_quit(void)
{
	app_event_t e = { 0 };
	e.type = APP_EVENT_QUIT;
	app_post_event(&e);
}

void lcui_exit(int code)
{
	lcui.exit_code = code;
	lcui_quit();
}

int lcui_main(void)
{
	LCUI_BOOL active = TRUE;
	app_event_t e = { 0 };

	while (active) {
		while (lcui_poll_event(&e)) {
			if (lcui_process_event(&e)) {
				active = FALSE;
				break;
			}
			app_event_destroy(&e);
		}
	}
	return lcui_destroy();
}
