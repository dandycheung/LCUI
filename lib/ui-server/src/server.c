// TODO: Reduce dependence on lcui header files

#include <LCUI.h>
#include <LCUI/graph.h>

#include <app.h>
#include <ui.h>

#include "config.h"

#ifdef ENABLE_OPENMP
#include <omp.h>
#endif

typedef struct ui_flash_rect_t {
	int64_t paint_time;
	pd_rect_t rect;
} ui_flash_rect_t;

typedef struct ui_dirty_layer_t {
	list_t rects;
	pd_rect_t rect;
	int dirty;
} ui_dirty_layer_t;

typedef struct ui_connection_t {
	/** whether new content has been rendered */
	LCUI_BOOL rendered;

	/** flashing rect list */
	list_t flash_rects;

	app_window_t *window;
	ui_widget_t *widget;
} ui_connection_t;

static struct ui_server_t {
	/** list_t<ui_connection_t> */
	list_t connections;

	LCUI_BOOL paint_flashing_enabled;
	int num_rendering_threads;
} ui_server;

INLINE int is_rect_equals(const pd_rect_t *a, const pd_rect_t *b)
{
	return a->x == b->x && a->y == b->y && a->width == b->width &&
	       a->height == b->height;
}

static void ui_connection_destroy(ui_connection_t *conn)
{
	app_window_close(conn->window);
	list_destroy(&conn->flash_rects, free);
	free(conn);
}

int ui_server_disconnect(ui_widget_t *widget, app_window_t *window)
{
	int count;
	ui_connection_t *conn;
	list_node_t *node, *prev;

	for (list_each(node, &ui_server.connections)) {
		conn = node->data;
		if ((widget && conn->widget != widget) ||
		    (window && conn->window != window)) {
			continue;
		}
		prev = node->prev;
		list_delete_node(&ui_server.connections, node);
		node = prev;
		count++;
	}
	return count;
}

static void ui_server_on_window_close(app_event_t *e, void *arg)
{
	ui_server_disconnect(NULL, e->window);
}

static void ui_server_on_window_paint(app_event_t *e, void *arg)
{
	pd_rectf_t rect;
	list_node_t *node;
	ui_connection_t *conn;

	for (list_each(node, &ui_server.connections)) {
		conn = node->data;
		if (conn && conn->window != e->window) {
			continue;
		}
		LCUIRect_ToRectF(&e->paint.rect, &rect, ui_get_scale());
		ui_widget_mark_dirty_rect(conn->widget, &rect, SV_GRAPH_BOX);
	}
}

static void ui_server_on_window_resize(app_event_t *e, void *arg)
{
	ui_widget_t *widget;
	float scale = ui_get_scale();
	float width = e->size.width / scale;
	float height = e->size.height / scale;

	widget = ui_server_get_widget(e->window);
	ui_widget_resize(widget, width, height);
}

static void ui_server_on_window_minmaxinfo(app_event_t *e, void *arg)
{
	pd_rect_t rect;
	LCUI_BOOL resizable = FALSE;
	float scale = ui_get_scale();
	int width = app_window_get_width(e->window);
	int height = app_window_get_height(e->window);
	ui_widget_t *widget = ui_server_get_widget(e->window);
	ui_widget_style_t *style = &widget->computed_style;

	if (style->min_width >= 0) {
		e->minmaxinfo.min_width = y_iround(scale * style->min_width);
		if (width < e->minmaxinfo.min_width) {
			height = e->minmaxinfo.min_width;
			resizable = TRUE;
		}
	}
	if (style->max_width >= 0) {
		e->minmaxinfo.max_width = y_iround(scale * style->max_width);
		if (width > e->minmaxinfo.max_width) {
			height = e->minmaxinfo.max_width;
			resizable = TRUE;
		}
	}
	if (style->min_height >= 0) {
		e->minmaxinfo.min_height = y_iround(scale * style->min_height);
		if (height < e->minmaxinfo.min_height) {
			height = e->minmaxinfo.min_height;
			resizable = TRUE;
		}
	}
	if (style->max_height >= 0) {
		e->minmaxinfo.max_height = y_iround(scale * style->max_height);
		if (height > e->minmaxinfo.max_height) {
			height = e->minmaxinfo.max_height;
			resizable = TRUE;
		}
	}
	if (resizable) {
		app_window_set_size(e->window, width, height);
	}
}

static void ui_server_on_destroy_widget(ui_widget_t *widget, ui_event_t *e,
					void *arg)
{
	ui_server_disconnect(widget, NULL);
}

void ui_server_connect(ui_widget_t *widget, app_window_t *window)
{
	pd_rect_t rect;
	ui_connection_t *conn;

	conn = malloc(sizeof(ui_connection_t));
	conn->window = window;
	conn->widget = widget;
	conn->rendered = FALSE;
	list_create(&conn->flash_rects);
	ui_widget_mark_dirty_rect(widget, NULL, SV_GRAPH_BOX);
	ui_widget_on(widget, "destroy", ui_server_on_destroy_widget,
		     NULL, NULL);
	// TODO: 添加 MutationObserver 以监听 widget 的显示、隐藏、移动、调整大小等属性变化
	list_append(&ui_server.connections, conn);
}

static void get_rendering_layer_size(int *width, int *height)
{
	float scale = ui_get_scale();

	*width = (int)(app_get_screen_width() * scale);
	*height = (int)(app_get_screen_height() * scale);
	*height = y_max(200, *height / ui_server.num_rendering_threads + 1);
}

static void ui_server_dump_rects(ui_connection_t *conn, list_t *out_rects)
{
	int i;
	int max_dirty;
	int layer_width;
	int layer_height;

	pd_rect_t rect;
	pd_rect_t *sub_rect;
	ui_dirty_layer_t *layer;
	ui_dirty_layer_t *layers;
	list_node_t *node;
	list_t rects;

	list_create(&rects);
	ui_widget_get_dirty_rects(conn->widget, &rects);
	get_rendering_layer_size(&layer_width, &layer_height);
	max_dirty = (int)(0.8 * layer_width * layer_height);
	layers =
	    malloc(sizeof(ui_dirty_layer_t) * ui_server.num_rendering_threads);
	for (i = 0; i < ui_server.num_rendering_threads; ++i) {
		layer = &layers[i];
		layer->dirty = 0;
		layer->rect.y = i * layer_height;
		layer->rect.x = 0;
		layer->rect.width = layer_width;
		layer->rect.height = layer_height;
		list_create(&layer->rects);
	}
	sub_rect = malloc(sizeof(pd_rect_t));
	for (list_each(node, &rects)) {
		rect = *(pd_rect_t *)node->data;
		for (i = 0; i < ui_server.num_rendering_threads; ++i) {
			layer = &layers[i];
			if (layer->dirty >= max_dirty) {
				continue;
			}
			if (!pd_rect_get_overlay_rect(&layer->rect, &rect,
						      sub_rect)) {
				continue;
			}
			list_append(&layer->rects, sub_rect);
			rect.y += sub_rect->height;
			rect.height -= sub_rect->height;
			layer->dirty += sub_rect->width * sub_rect->height;
			sub_rect = malloc(sizeof(pd_rect_t));
			if (rect.height < 1) {
				break;
			}
		}
	}
	for (i = 0; i < ui_server.num_rendering_threads; ++i) {
		layer = &layers[i];
		if (layer->dirty >= max_dirty) {
			RectList_AddEx(out_rects, &layer->rect, FALSE);
			RectList_Clear(&layer->rects);
		} else {
			list_concat(out_rects, &layer->rects);
		}
	}
	RectList_Clear(&rects);
	free(sub_rect);
	free(layers);
}

static size_t ui_server_render_flash_rect(ui_connection_t *conn,
					  ui_flash_rect_t *flash_rect)
{
	size_t count;
	int64_t period;
	float duration = 1000;

	pd_pos_t pos;
	pd_color_t color;
	pd_canvas_t mask;
	pd_paint_context_t *paint;

	paint = app_window_begin_paint(conn->window, &flash_rect->rect);
	if (!paint) {
		return 0;
	}
	period = get_time_delta(flash_rect->paint_time);
	count = Widget_Render(conn->widget, paint);
	if (period >= duration) {
		flash_rect->paint_time = 0;
		app_window_end_paint(conn->window, paint);
		return count;
	}
	pd_canvas_init(&mask);
	mask.color_type = PD_COLOR_TYPE_ARGB;
	pd_canvas_create(&mask, flash_rect->rect.width,
			 flash_rect->rect.height);
	pd_canvas_fill_rect(&mask, ARGB(125, 124, 179, 5), NULL, TRUE);
	mask.opacity = 0.6f * (duration - (float)period) / duration;
	pos.x = pos.y = 0;
	color = RGB(124, 179, 5);
	pd_graph_draw_horiz_line(&mask, color, 1, pos, mask.width - 1);
	pd_graph_draw_verti_line(&mask, color, 1, pos, mask.height - 1);
	pos.x = mask.width - 1;
	pd_graph_draw_verti_line(&mask, color, 1, pos, mask.height - 1);
	pos.x = 0;
	pos.y = mask.height - 1;
	pd_graph_draw_horiz_line(&mask, color, 1, pos, mask.width - 1);
	pd_canvas_mix(&paint->canvas, &mask, 0, 0, TRUE);
	pd_canvas_free(&mask);
	app_window_end_paint(conn->window, paint);
	return count;
}

static size_t ui_server_update_flash_rects(ui_connection_t *conn)
{
	size_t count = 0;
	ui_flash_rect_t *flash_rect;
	list_node_t *node, *prev;

	for (list_each(node, &conn->flash_rects)) {
		flash_rect = node->data;
		if (flash_rect->paint_time == 0) {
			prev = node->prev;
			free(node->data);
			list_delete_node(&conn->flash_rects, node);
			node = prev;
			continue;
		}
		ui_server_render_flash_rect(conn, flash_rect);
		conn->rendered = TRUE;
	}
	return count;
}

static void ui_server_add_flash_rect(ui_connection_t *conn, pd_rect_t *rect)
{
	list_node_t *node;
	ui_flash_rect_t *flash_rect;

	for (list_each(node, &conn->flash_rects)) {
		flash_rect = node->data;
		if (is_rect_equals(&flash_rect->rect, rect)) {
			flash_rect->paint_time = get_time_ms();
			return;
		}
	}

	flash_rect = malloc(sizeof(ui_flash_rect_t));
	flash_rect->rect = *rect;
	flash_rect->paint_time = get_time_ms();
	list_append(&conn->flash_rects, flash_rect);
}

static size_t ui_server_render_rect(ui_connection_t *conn, pd_rect_t *rect)
{
	size_t count;
	pd_paint_context_t *paint;

	if (!conn->widget || !conn->window) {
		return 0;
	}
	paint = app_window_begin_paint(conn->window, rect);
	if (!paint) {
		return 0;
	}
	DEBUG_MSG("[thread %d/%d] rect: (%d,%d,%d,%d)\n", omp_get_thread_num(),
		  omp_get_num_threads(), paint->rect.x, paint->rect.y,
		  paint->rect.width, paint->rect.height);
	count = Widget_Render(conn->widget, paint);
	if (ui_server.paint_flashing_enabled) {
		ui_server_add_flash_rect(conn, &paint->rect);
	}
	app_window_end_paint(conn->window, paint);
	return count;
}

static size_t ui_server_render_window(ui_connection_t *conn)
{
	int i = 0;
	int dirty = 0;
	int layer_width;
	int layer_height;
	size_t count = 0;
	pd_rect_t **rect_array;
	list_t rects;
	list_node_t *node;

	list_create(&rects);
	get_rendering_layer_size(&layer_width, &layer_height);
	ui_server_dump_rects(conn, &rects);
	if (rects.length < 1) {
		return 0;
	}
	rect_array = (pd_rect_t **)malloc(sizeof(pd_rect_t *) * rects.length);
	for (list_each(node, &rects)) {
		// TODO:
		LCUI_SysEventRec ev;

		rect_array[i] = node->data;
		ev.type = APP_EVENT_PAINT;
		ev.paint.rect = *rect_array[i];
		LCUI_TriggerEvent(&ev, NULL);
		dirty += rect_array[i]->width * rect_array[i]->height;
		i++;
	}
	// Use OPENMP if the render area is larger than two render layers
	if (dirty >= layer_width * layer_height * 2) {
#ifdef ENABLE_OPENMP
#pragma omp parallel for \
	default(none) \
	shared(display, rects, rect_array) \
	firstprivate(conn) \
	reduction(+:count)
#endif
		for (i = 0; i < (int)rects.length; ++i) {
			count += ui_server_render_rect(conn, rect_array[i]);
		}
	} else {
		for (i = 0; i < (int)rects.length; ++i) {
			count += ui_server_render_rect(conn, rect_array[i]);
		}
	}
	free(rect_array);
	RectList_Clear(&rects);
	conn->rendered = count > 0;
	count += ui_server_update_flash_rects(conn);
	return count;
}

size_t ui_server_render(void)
{
	size_t count = 0;
	list_node_t *node;

	for (list_each(node, &ui_server.connections)) {
		count += ui_server_render_window(node->data);
		count += ui_server_update_flash_rects(node->data);
	}
	return count;
}

ui_widget_t *ui_server_get_widget(app_window_t *window)
{
	ui_connection_t *conn;
	list_node_t *node;

	for (list_each(node, &ui_server.connections)) {
		conn = node->data;
		if (conn && conn->window == window) {
			return conn->widget;
		}
	}
	return NULL;
}

static app_window_t *ui_server_get_window(ui_widget_t *widget)
{
	ui_connection_t *conn;
	list_node_t *node;

	for (list_each(node, &ui_server.connections)) {
		conn = node->data;
		if (conn && conn->widget == widget) {
			return conn->window;
		}
	}
	return NULL;
}

// TODO:
static ui_server_update_window()
{
	LCUIMetrics_ComputeRectActual(&rect, &widget->box.canvas);
	if (Widget_CheckStyleValid(widget, key_top) &&
	    Widget_CheckStyleValid(widget, key_left)) {
		Surface_Move(conn->window, rect.x, rect.y);
	}
	Surface_SetCaptionW(conn->window, widget->title);
	Surface_Resize(conn->window, rect.width, rect.height);
	if (widget->computed_style.visible) {
		Surface_Show(conn->window);
	} else {
		Surface_Hide(conn->window);
	}
}

void ui_server_init(void)
{
	app_on_event(APP_EVENT_MINMAXINFO, ui_server_on_window_minmaxinfo, NULL);
	app_on_event(APP_EVENT_SIZE, ui_server_on_window_resize, NULL);
	app_on_event(APP_EVENT_CLOSE, ui_server_on_window_close, NULL);
	app_on_event(APP_EVENT_PAINT, ui_server_on_window_paint, NULL);
}

void ui_server_set_threads(int threads)
{
	ui_server.num_rendering_threads = threads;
}

void ui_server_set_paint_flashing_enabled(LCUI_BOOL enabled)
{
	ui_server.paint_flashing_enabled = enabled;
}

void ui_server_destroy(void)
{
	app_off_event(APP_EVENT_MINMAXINFO, ui_server_on_window_minmaxinfo, NULL);
	app_off_event(APP_EVENT_SIZE, ui_server_on_window_resize, NULL);
	app_off_event(APP_EVENT_CLOSE, ui_server_on_window_close, NULL);
}
