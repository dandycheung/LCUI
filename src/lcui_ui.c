#include <math.h>
#include <LCUI.h>
#include <ui/server.h>
#include <LCUI/gui/widget/textview.h>
#include <LCUI/gui/widget/button.h>
#include <LCUI/gui/widget/anchor.h>
#include <LCUI/gui/widget/canvas.h>
#include <LCUI/gui/widget/sidebar.h>
#include <LCUI/gui/widget/scrollbar.h>
#include <LCUI/gui/widget/textedit.h>
#include <LCUI/gui/widget/textcaret.h>

static struct lcui_ui_t {
	lcui_display_mode_t mode;
	ui_mutation_observer_t *observer;
	list_t windows;
} lcui_ui;

static void lcui_dispatch_ui_mouse_event(ui_event_type_t type,
					 app_event_t *app_evt)
{
	ui_event_t e = { 0 };
	float scale = ui_get_scale();

	e.type = type;
	e.mouse.y = (float)round(app_evt->mouse.y / scale);
	e.mouse.x = (float)round(app_evt->mouse.x / scale);
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

static void lcui_dispatch_ui_touch_event(app_touch_event_t *touch)
{
	size_t i;
	float scale;
	ui_event_t e = { 0 };

	scale = ui_get_scale();
	e.type = UI_EVENT_TOUCH;
	e.touch.n_points = touch->n_points;
	e.touch.points = malloc(sizeof(ui_touch_point_t) * e.touch.n_points);
	for (i = 0; i < e.touch.n_points; ++i) {
		switch (touch->points[i].state) {
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
		e.touch.points[i].x = (float)round(touch->points[i].x / scale);
		e.touch.points[i].y = (float)round(touch->points[i].y / scale);
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

void lcui_dispatch_ui_event(app_event_t *app_event)
{
	switch (app_event->type) {
	case APP_EVENT_KEYDOWN:
		lcui_dispatch_ui_keyboard_event(UI_EVENT_KEYDOWN, app_event);
		break;
	case APP_EVENT_KEYUP:
		lcui_dispatch_ui_keyboard_event(UI_EVENT_KEYUP, app_event);
		break;
	case APP_EVENT_KEYPRESS:
		lcui_dispatch_ui_keyboard_event(UI_EVENT_KEYPRESS, app_event);
		break;
	case APP_EVENT_MOUSEDOWN:
		lcui_dispatch_ui_mouse_event(UI_EVENT_MOUSEDOWN, app_event);
		break;
	case APP_EVENT_MOUSEUP:
		lcui_dispatch_ui_mouse_event(UI_EVENT_MOUSEUP, app_event);
		break;
	case APP_EVENT_MOUSEMOVE:
		lcui_dispatch_ui_mouse_event(UI_EVENT_MOUSEMOVE, app_event);
		break;
	case APP_EVENT_TOUCH:
		lcui_dispatch_ui_touch_event(&app_event->touch);
		break;
	case APP_EVENT_WHEEL:
		lcui_dispatch_ui_wheel_event(&app_event->wheel);
		break;
	case APP_EVENT_COMPOSITION:
		lcui_dispatch_ui_textinput_event(app_event);
		break;
	default:
		break;
	}
}

size_t lcui_render_ui(void)
{
	return ui_server_render();
}

void lcui_update_ui(void)
{
	ui_process_events();
	ui_update();
}

static app_window_t *lcui_create_window_for_widget(ui_widget_t *w)
{
	app_window_t *wnd;
	pd_rect_t rect;

	ui_compute_rect_actual(&rect, &w->box.canvas);
	wnd = app_window_create(w->title, rect.x, rect.y, rect.width,
				rect.height, NULL);
	list_append(&lcui_ui.windows, wnd);
	return wnd;
}

static void lcui_process_ui_mutation(ui_mutation_record_t *mutation)
{
	list_node_t *node;

	if (mutation->type != UI_MUTATION_RECORD_TYPE_CHILD_LIST ||
	    lcui_ui.mode != LCUI_DISPLAY_MODE_SEAMLESS) {
		return;
	}
	for (list_each(node, &mutation->removed_widgets)) {
		ui_server_disconnect(node->data, NULL);
	}
	for (list_each(node, &mutation->added_widgets)) {
		ui_server_connect(node->data, NULL);
	}
}

static void lcui_on_ui_mutation(ui_mutation_list_t *mutation_list,
				ui_mutation_observer_t *observer, void *arg)
{
	list_node_t *node;

	for (list_each(node, mutation_list)) {
		lcui_process_ui_mutation(node->data);
	}
}

void lcui_set_ui_display_mode(lcui_display_mode_t mode)
{
	app_window_t *wnd;
	ui_mutation_observer_init_t options = { 0 };

	if (mode == lcui_ui.mode) {
		return;
	}
	list_destroy(&lcui_ui.windows, app_window_close);
	if (lcui_ui.observer) {
		ui_mutation_observer_disconnect(lcui_ui.observer);
		lcui_ui.observer = NULL;
	}
	switch (mode) {
	case LCUI_DISPLAY_MODE_FULLSCREEN:
		wnd = lcui_create_window_for_widget(ui_root());
		app_window_set_size(wnd, app_get_screen_width(),
				    app_get_screen_height());
		app_window_activate(wnd);
		break;
	case LCUI_DISPLAY_MODE_SEAMLESS:
		options.child_list = TRUE;
		lcui_ui.observer =
		    ui_mutation_observer_create(lcui_on_ui_mutation, NULL);
		ui_mutation_observer_observe(lcui_ui.observer, ui_root(),
					     options);
		break;
	case LCUI_DISPLAY_MODE_WINDOWED:
		wnd = lcui_create_window_for_widget(ui_root());
		app_window_activate(wnd);
	default:
		break;
	}
	lcui_ui.mode = mode;
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

void lcui_init_ui(void)
{
	ui_init();
	ui_server_init();
	list_create(&lcui_ui.windows);
	lcui_set_ui_display_mode(LCUI_DISPLAY_MODE_WINDOWED);
	lcui_init_ui_preset_widgets();
}

void lcui_destroy_ui(void)
{
	list_destroy(&lcui_ui.windows, app_window_close);
	if (lcui_ui.observer) {
		ui_mutation_observer_disconnect(lcui_ui.observer);
		ui_mutation_observer_destroy(lcui_ui.observer);
		lcui_ui.observer = NULL;
	}
	ui_server_destroy();
	ui_destroy();
}
