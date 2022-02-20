#include "../../include/css/computed.h"
#include "../../include/css/properties.h"

int css_cascade_visibility(const css_style_array_value_t input,
			   css_computed_style_t* computed)
{
	uint8_t value = CSS_VISIBILITY_VISIBLE;

	switch (input[0].keyword_value) {
	case CSS_KEYWORD_HIDDEN:
		value = CSS_VISIBILITY_HIDDEN;
		break;
	default:
		break;
	}
	computed->type_bits.visibility = value;
	return 0;
}
