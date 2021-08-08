#include <LCUI_Build.h>
#include <LCUI/LCUI.h>
#include <LCUI/gui/widget.h>
#include <LCUI/gui/builder.h>

int main(void)
{
	int ret = 0;
	LCUI_Widget root, box;

	lcui_init();
	box = LCUIBuilder_LoadFile("test_box_shadow.xml");
	if (!box) {
		lcui_destroy();
		return ret;
	}
	root = LCUIWidget_GetRoot();
	Widget_Append(root, box);
	Widget_Unwrap(box);
	return lcui_main();
}
