// TODO: Reduce dependence on lcui header files

#include <LCUI.h>
#include <LCUI/graph.h>
#include <app.h>
#include <ui.h>

LCUI_API void ui_cursor_refresh(void);
LCUI_API LCUI_BOOL ui_cursor_is_visible(void);
LCUI_API void ui_cursor_show(void);
LCUI_API void ui_cursor_hide(void);
LCUI_API void ui_cursor_set_position(int x, int y);
LCUI_API int ui_cursor_set_image(pd_canvas_t *image);
LCUI_API void ui_cursor_get_position(int *x, int *y);
LCUI_API int ui_cursor_paint(app_window_t *w, app_window_paint_t* paint);
LCUI_API void ui_cursor_init(void);
LCUI_API void ui_cursor_destroy(void);
