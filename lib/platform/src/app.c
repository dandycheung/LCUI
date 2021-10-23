#include "platform.h"

int app_init(const wchar_t *name)
{
	if (app_init_engine(name) != 0) {
		return -1;
	}
	app_init_ime();
	app_init_events();
	return 0;
}

void app_destroy(void)
{
	app_destroy_events();
	app_destroy_ime();
	app_destroy_engine();
}
