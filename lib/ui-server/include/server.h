// TODO: Reduce dependence on lcui header files

#include <LCUI.h>
#include <app.h>
#include <ui.h>

LCUI_API int ui_server_disconnect(ui_widget_t *widget, app_window_t *window);
LCUI_API void ui_server_connect(ui_widget_t *widget, app_window_t *window);
LCUI_API size_t ui_server_render(void);
LCUI_API void ui_server_present(void);
LCUI_API ui_widget_t *ui_server_get_widget(app_window_t *window);
LCUI_API void ui_server_init(void);
LCUI_API void ui_server_set_threads(int threads);
LCUI_API void ui_server_set_paint_flashing_enabled(LCUI_BOOL enabled);
LCUI_API void ui_server_destroy(void);
