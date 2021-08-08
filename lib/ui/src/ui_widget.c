﻿#include <string.h>
#include <assert.h>
#include <LCUI.h>
#include "../include/ui.h"
#include "private.h"

static list_t ui_trash;

static void ui_widget_destroy_children(ui_widget_t* w);

static void ui_widget_init(ui_widget_t* w)
{
	memset(w, 0, sizeof(ui_widget_t));
	w->state = LCUI_WSTATE_CREATED;
	w->style = StyleSheet();
	w->computed_style.opacity = 1.0;
	w->computed_style.visible = TRUE;
	w->computed_style.focusable = FALSE;
	w->computed_style.display = SV_BLOCK;
	w->computed_style.position = SV_STATIC;
	w->computed_style.pointer_events = SV_INHERIT;
	w->computed_style.box_sizing = SV_CONTENT_BOX;
	list_create(&w->children);
	list_create(&w->stacking_context);
	w->node.data = w;
	w->node_show.data = w;
	w->node.next = w->node.prev = NULL;
	w->node_show.next = w->node_show.prev = NULL;
	ui_widget_init_background(w);
}

static void ui_widget_destroy(ui_widget_t* w)
{
	if (w->parent) {
		ui_widget_add_task(w->parent, UI_TASK_REFLOW);
		ui_widget_unlink(w);
	}
	ui_widget_destroy_background(w);
	ui_widget_destroy_listeners(w);
	ui_widget_destroy_children(w);
	ui_widget_destroy_prototype(w);
	if (w->title) {
		free(w->title);
		w->title = NULL;
	}
	ui_widget_destroy_id(w);
	ui_widget_destroy_style(w);
	ui_widget_destroy_attributes(w);
	ui_widget_destroy_classes(w);
	ui_widget_destroy_status(w);
	ui_widget_set_rules(w, NULL);
	free(w);
}

static void ui_widget_destroy_children(ui_widget_t* w)
{
	/* 先释放显示列表，后销毁部件列表，因为部件在这两个链表中的节点是和它共用
	 * 一块内存空间的，销毁部件列表会把部件释放掉，所以把这个操作放在后面 */
	LinkedList_ClearData(&w->stacking_context, NULL);
	LinkedList_ClearData(&w->children, ui_widget_destroy);
}

size_t ui_trash_clear(void)
{
	size_t count;
	list_node_t *node;

	node = ui_trash.head.next;
	count = ui_trash.length;
	while (node) {
		list_node_t *next = node->next;
		list_unlink(&ui_trash, node);
		ui_widget_destroy(node->data);
		node = next;
	}
	return count;
}

static void ui_trash_add(ui_widget_t *w)
{
	w->state = LCUI_WSTATE_DELETED;
	if (ui_widget_unlink(w) != 0) {
		return;
	}
	list_append_node(&ui_trash, &w->node);
	ui_widget_post_surface_event(w, UI_EVENT_UNLINK, TRUE);
}

ui_widget_t* ui_create_widget(const char* type)
{
	ui_widget_t* widget = malloc(sizeof(ui_widget_t));

	ui_widget_init(widget);
	widget->proto = ui_get_widget_prototype(type);
	if (widget->proto->name) {
		widget->type = widget->proto->name;
	} else if (type) {
		widget->type = strdup2(type);
	}
	widget->proto->init(widget);
	ui_widget_add_task(widget, UI_TASK_REFRESH_STYLE);
	return widget;
}

ui_widget_t* ui_create_widget_with_prototype(const ui_widget_prototype_t* proto)
{
	ui_widget_t* widget = malloc(sizeof(ui_widget_t));

	ui_widget_init(widget);
	widget->proto = proto;
	widget->type = widget->proto->name;
	widget->proto->init(widget);
	ui_widget_add_task(widget, UI_TASK_REFRESH_STYLE);
	return widget;
}

void ui_widget_remove(ui_widget_t* w)
{
	ui_widget_t* root = w;

	assert(w->state != LCUI_WSTATE_DELETED);
	while (root->parent) {
		root = root->parent;
	}
	/* If this widget is not mounted in the root widget tree */
	if (root != ui_root()) {
		w->state = LCUI_WSTATE_DELETED;
		ui_widget_destroy(w);
		return;
	}
	if (w->parent) {
		ui_widget_t* child;
		list_node_t* node;

		/* Update the index of the siblings behind it */
		node = w->node.next;
		while (node) {
			child = node->data;
			child->index -= 1;
			node = node->next;
		}
		if (w->computed_style.position != SV_ABSOLUTE) {
			ui_widget_add_task(w->parent, UI_TASK_REFLOW);
		}
		ui_widget_mark_dirty_rect(w->parent, &w->box.canvas,
				      SV_CONTENT_BOX);
		ui_trash_add(w);
	}
}

void ui_widget_add_state(ui_widget_t* w, ui_widget_state_t state)
{
	/* 如果部件还处于未准备完毕的状态 */
	if (w->state < LCUI_WSTATE_READY) {
		w->state |= state;
		/* 如果部件已经准备完毕则触发 ready 事件 */
		if (w->state == LCUI_WSTATE_READY) {
			ui_event_t e = { 0 };
			e.type = UI_EVENT_READY;
			e.cancel_bubble = TRUE;
			ui_widget_emit_event(w, e, NULL);
			w->state = LCUI_WSTATE_NORMAL;
		}
	}
}

void ui_widget_set_title(ui_widget_t* w, const wchar_t* title)
{
	size_t len;
	wchar_t *new_title, *old_title;

	len = wcslen(title) + 1;
	new_title = (wchar_t*)malloc(sizeof(wchar_t) * len);
	if (!new_title) {
		return;
	}
	wcsncpy(new_title, title, len);
	old_title = w->title;
	w->title = new_title;
	if (old_title) {
		free(old_title);
	}
	ui_widget_add_task(w, UI_TASK_TITLE);
}

void ui_widget_set_text(ui_widget_t* w, const char *text)
{
	if (w->proto && w->proto->settext) {
		w->proto->settext(w, text);
	}
}

void ui_widget_bind_property(ui_widget_t* w, const char *name, LCUI_Object value)
{
	if (w->proto && w->proto->bindprop) {
		w->proto->bindprop(w, name, value);
	}
}

void ui_widget_empty(ui_widget_t* w)
{
	ui_widget_t* root = w;
	ui_widget_t* child;
	list_node_t *node;
	ui_event_t ev;

	while (root->parent) {
		root = root->parent;
	}
	if (root != ui_root()) {
		ui_widget_destroy_children(w);
		return;
	}
	ui_event_init(&ev, "unlink");
	for (list_each(node, &w->children)) {
		child = node->data;
		ui_widget_emit_event(child, ev, NULL);
		if (child->parent == root) {
			ui_widget_post_surface_event(child, UI_EVENT_UNLINK,
						TRUE);
		}
		child->state = LCUI_WSTATE_DELETED;
		child->parent = NULL;
	}
	LinkedList_ClearData(&w->stacking_context, NULL);
	LinkedList_Concat(&ui_trash, &w->children);
	ui_widget_mark_dirty_rect(w, NULL, SV_GRAPH_BOX);
	ui_widget_refresh_style(w);
}

void ui_widget_get_offset(ui_widget_t* w, ui_widget_t* parent, float* offset_x,
			  float* offset_y)
{
	float x = 0, y = 0;
	while (w != parent) {
		x += w->box.border.x;
		y += w->box.border.y;
		w = w->parent;
		if (w) {
			x += w->box.padding.x - w->box.border.x;
			y += w->box.padding.y - w->box.border.y;
		} else {
			break;
		}
	}
	*offset_x = x;
	*offset_y = y;
}

LCUI_BOOL ui_widget_in_viewport(ui_widget_t* w)
{
	list_node_t* node;
	pd_rectf_t rect;
	ui_widget_t *self, *parent, *child;
	ui_widget_style_t* style;

	rect = w->box.padding;
	/* If the size of the widget is not fixed, then set the maximum size to
	 * avoid it being judged invisible all the time. */
	if (rect.width < 1 && ui_widget_has_auto_style(w, key_width)) {
		rect.width = w->parent->box.padding.width;
	}
	if (rect.height < 1 && ui_widget_has_auto_style(w, key_height)) {
		rect.height = w->parent->box.padding.height;
	}
	for (self = w, parent = w->parent; parent;
	     self = parent, parent = parent->parent) {
		if (!ui_widget_is_visible(parent)) {
			return FALSE;
		}
		for (node = self->node_show.prev; node && node->prev;
		     node = node->prev) {
			child = node->data;
			style = &child->computed_style;
			if (child->state < LCUI_WSTATE_LAYOUTED ||
			    child == self || !ui_widget_is_visible(child)) {
				continue;
			}
			DEBUG_MSG("rect: (%g,%g,%g,%g), child rect: "
				  "(%g,%g,%g,%g), child: %s %s\n",
				  rect.x, rect.y, rect.width, rect.height,
				  child->box.border.x, child->box.border.y,
				  child->box.border.width,
				  child->box.border.height, child->type,
				  child->id);
			if (!LCUIRectF_IsIncludeRect(&child->box.border,
						     &rect)) {
				continue;
			}
			if (style->opacity == 1.0f &&
			    style->background.color.alpha == 255) {
				return FALSE;
			}
		}
		rect.x += parent->box.padding.x;
		rect.y += parent->box.padding.y;
		LCUIRectF_ValidateArea(&rect, parent->box.padding.width,
				       parent->box.padding.height);
		if (rect.width < 1 || rect.height < 1) {
			return FALSE;
		}
	}
	return TRUE;
}
