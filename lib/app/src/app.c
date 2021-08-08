#include <app.h>

int app_init(const wchar_t *name)
{
	if (app_init_engine(name) != 0) {
		return -1;
	}
	app_init_ime();
	app_init_events();
	switch (app_get_id()) {
	case APP_ID_LINUX_X11:
	case APP_ID_UWP:
	case APP_ID_WIN32:
		LCUICursor_Hide();
		break;
	default:
		break;
	}
	return 0;
}

void app_destory(void)
{
	app_destroy_events();
	app_destroy_ime();
	app_destroy_engine();
}
