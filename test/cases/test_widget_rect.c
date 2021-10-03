#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <LCUI.h>
#include <LCUI/input.h>
#include <LCUI/ui.h>
#include "ctest.h"

void test_widget_rect(void)
{
	ui_widget_t* root;
	ui_widget_t* parent, child;
	app_event_t ev;
	pd_rect_t *rect;
	pd_rect_t expected_rect;
	list_t rects;

	lcui_init();
	root = ui_root();
	parent = ui_create_widget("button");
	child = ui_create_widget("textview");

	ui_widget_set_style(parent, key_box_sizing, SV_BORDER_BOX, style);
	ui_widget_resize(root, 200, 200);
	ui_widget_resize(parent, 100, 100);
	ui_widget_resize(child, 50, 50);
	ui_widget_append(parent, child);
	ui_widget_append(root, parent);
	ui_update();

	list_create(&rects);
	ui_widget_get_dirty_rects(root, &rects);
	list_destroy(&rects, free);

	ev.type = APP_EVENT_MOUSEMOVE;
	ev.motion.x = 150;
	ev.motion.y = 150;
	ev.motion.xrel = 0;
	ev.motion.yrel = 0;
	LCUI_TriggerEvent(&ev, NULL);
	ui_update();
	ui_widget_get_dirty_rects(root, &rects);
	it_b("app.trigger({ type: 'mousemove', x: 150, y: 150}), "
	     "root.getInvalidArea().length == 0",
	     rects.length == 0, TRUE);

	ev.motion.x = 80;
	ev.motion.y = 80;
	LCUI_TriggerEvent(&ev, NULL);
	ui_update();
	ui_widget_get_dirty_rects(root, &rects);
	rect = rects.head.next->data;
	it_b("app.trigger({ type: 'mousemove', x: 80, y: 80 }), "
	     "root.getInvalidArea().length == 1",
	     rects.length == 1, TRUE);

	expected_rect.x = 0;
	expected_rect.y = 0;
	expected_rect.width = 100;
	expected_rect.height = 100;
	it_rect("root.getInvalidArea()[0]", rect, &expected_rect);
	list_destroy(&rects, free);

	ev.motion.x = 40;
	ev.motion.y = 40;
	LCUI_TriggerEvent(&ev, NULL);
	ui_update();
	ui_widget_get_dirty_rects(root, &rects);
	it_b("app.trigger({ type: 'mousemove', x: 40, y: 40 }), "
	     "root.getInvalidArea().length == 0",
	     rects.length == 0, TRUE);

	ev.type = APP_EVENT_MOUSEDOWN;
	ev.mouse.x = 40;
	ev.mouse.y = 40;
	ev.mouse.button = MOUSE_BUTTON_LEFT;
	LCUI_TriggerEvent(&ev, NULL);
	ui_update();
	ui_widget_get_dirty_rects(root, &rects);
	it_b("app.trigger({ type: 'mousedown', x: 40, y: 40 }), "
	     "root.getInvalidArea().length == 1",
	     rects.length == 1, TRUE);
	if (rects.length == 1) {
		rect = rects.head.next->data;
		it_rect("root.getInvalidArea()[0]", rect, &expected_rect);
	}
	list_destroy(&rects, free);

	ev.type = APP_EVENT_MOUSEUP;
	LCUI_TriggerEvent(&ev, NULL);
	ui_update();
	ui_widget_get_dirty_rects(root, &rects);
	it_b("app.trigger({ type: 'mouseup', x: 40, y: 40 }), "
	     "root.getInvalidArea().length == 1",
	     rects.length == 1, TRUE);
	if (rects.length == 1) {
		rect = rects.head.next->data;
		it_rect("root.getInvalidArea()[0]", rect, &expected_rect);
	}
	list_destroy(&rects, free);

	ev.type = APP_EVENT_MOUSEMOVE;
	ev.motion.x = 80;
	ev.motion.y = 80;
	LCUI_TriggerEvent(&ev, NULL);
	ui_update();
	ui_widget_get_dirty_rects(root, &rects);
	it_b("app.trigger({ type: 'mousemove', x: 80, y: 80 }), "
	     "root.getInvalidArea().length == 0",
	     rects.length == 0, TRUE);

	ev.motion.x = 150;
	ev.motion.y = 150;
	ev.motion.xrel = 0;
	ev.motion.yrel = 0;
	LCUI_TriggerEvent(&ev, NULL);
	ui_update();
	ui_widget_get_dirty_rects(root, &rects);

	it_b("app.trigger({ type: 'mousemove', x: 150, y: 150 }), "
	     "root.getInvalidArea().length == 1",
	     rects.length == 1, TRUE);
	if (rects.length == 1) {
		rect = rects.head.next->data;
		it_rect("root.getInvalidArea()[0]", rect, &expected_rect);
	}
	list_destroy(&rects, free);

	expected_rect.x = 21;
	expected_rect.y = 11;
	expected_rect.width = 50;
	expected_rect.height = 50;
	ui_widget_remove(child);
	ui_update();
	ui_widget_get_dirty_rects(root, &rects);
	it_b("child.destroy(), root.getInvalidArea().length == 1",
	     rects.length == 1, TRUE);
	if (rects.length == 1) {
		rect = rects.head.next->data;
		it_rect("root.getInvalidArea()[0]", rect, &expected_rect);
	}
	list_destroy(&rects, free);

	expected_rect.x = 0;
	expected_rect.y = 0;
	expected_rect.width = 100;
	expected_rect.height = 100;
	ui_widget_remove(parent);
	ui_update();
	ui_widget_get_dirty_rects(root, &rects);
	it_b("parent.destroy(), root.getInvalidArea().length == 1",
	     rects.length == 1, TRUE);
	if (rects.length == 1) {
		rect = rects.head.next->data;
		it_rect("root.getInvalidArea()[0]", rect, &expected_rect);
	}
	list_destroy(&rects, free);

	lcui_destroy();
}
