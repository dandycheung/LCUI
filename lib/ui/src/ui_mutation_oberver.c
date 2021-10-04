#include <ui.h>

typedef struct ui_mutation_connection_t {
	ui_widget_t *widget;
	ui_mutation_observer_init_t options;
	list_node_t node;
} ui_mutation_connection_t;

struct ui_mutation_observer_t {
	/** list_t<ui_mutation_connection_t> */
	list_t connections;

	/** list_t<ui_mutation_record_t> */
	list_t records;

	ui_mutation_observer_callback_t callback;
	void *callback_arg;
};

/**
 * @see https://developer.mozilla.org/en-US/docs/Web/API/MutationObserver
 */
ui_mutation_observer_t *ui_mutation_observer_create(
    ui_mutation_observer_callback_t callback, void *callback_arg)
{
	ui_mutation_observer_t *observer;

	observer = malloc(sizeof(ui_mutation_observer_t));
	if (!observer) {
		return NULL;
	}
	observer->callback = callback;
	observer->callback_arg = callback_arg;
	list_create(&observer->connections);
	return observer;
}

static void ui_mutation_observer_on_widget_destroy(ui_widget_t *w,
						   ui_event_t *e, void *arg)
{
	list_node_t *node, *prev;
	ui_mutation_connection_t *conn;
	ui_mutation_observer_t *observer = e->data;

	for (list_each(node, &observer->connections)) {
		prev = node->prev;
		conn = node->data;
		if (conn->widget == w) {
			conn->widget->extra->observer = NULL;
			conn->widget = NULL;
			list_unlink(&observer->connections, &node);
			free(conn);
			node = prev;
		}
	}
}

int ui_mutation_observer_observe(ui_mutation_observer_t *observer,
				 ui_widget_t *w,
				 ui_mutation_observer_init_t options)
{
	ui_mutation_connection_t *conn;

	conn = malloc(sizeof(ui_mutation_connection_t));
	if (!conn) {
		return -ENOMEM;
	}
	conn->widget = w;
	conn->options = options;
	list_append_node(&observer->connections, &conn->node);
	ui_widget_use_extra_data(w)->observer = observer;
	ui_widget_on(w, "destroy", ui_mutation_observer_on_widget_destroy,
		     observer, NULL);
	return 0;
}

void ui_mutation_observer_disconnect(ui_mutation_observer_t *observer)
{
	list_node_t *node;
	ui_mutation_connection_t *conn;

	for (list_each(node, &observer->connections)) {
		conn = node->data;
		if (conn->widget->extra) {
			conn->widget->extra->observer = NULL;
		}
	}
	list_destroy_without_node(&observer->connections, free);
}
