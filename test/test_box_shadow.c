#include <LCUI_Build.h>
#include <LCUI/LCUI.h>
#include <LCUI/ui.h>
#include <LCUI/gui/builder.h>

int main(void)
{
	int ret = 0;
	ui_widget_t* root, box;

	lcui_init();
	box = LCUIBuilder_LoadFile("test_box_shadow.xml");
	if (!box) {
		lcui_destroy();
		return ret;
	}
	root = ui_root();
	ui_widget_append(root, box);
	ui_widget_unwrap(box);
	return lcui_main();
}
