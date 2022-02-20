#include <stdio.h>
#include "test.h"
#include "ctest.h"
#include "../include/css.h"

static void test_css_valdef_string(const css_valdef_t *valdef)
{
	int ret;
	css_style_value_t val = { 0 };

	ret = css_parse_value(valdef, "\"abc\"", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE;
	if (ret) {
		ret = val.array_value[0].type == CSS_STRING_VALUE;
	}
	css_style_value_destroy(&val);
	it_b("match('\"abc\"')", ret, 1);
	it_b("match('auto')", css_parse_value(valdef, "auto", &val) > 0, 1);
	css_style_value_destroy(&val);
}

static void test_css_valdef_color(const css_valdef_t *valdef)
{
	css_color_value_t color;
	css_style_value_t val = { 0 };

	color = 0;
	if (css_parse_value(valdef, "#fff", &val) > 0 &&
	    val.array_value[0].type == CSS_COLOR_VALUE) {
		color = val.array_value[0].color_value;
	}
	css_style_value_destroy(&val);

	it_u("match('#fff')", (int)color, (int)css_color(255, 255, 255, 255));

	color = 0;
	if (css_parse_value(valdef, "#ffffff", &val) > 0 &&
	    val.array_value[0].type == CSS_COLOR_VALUE) {
		color = val.array_value[0].color_value;
	}
	css_style_value_destroy(&val);

	it_u("match('#ffffff')", color, css_color(255, 255, 255, 255));

	color = 0;
	if (css_parse_value(valdef, "transparent", &val) > 0 &&
	    val.array_value[0].type == CSS_COLOR_VALUE) {
		color = val.array_value[0].color_value;
	}
	css_style_value_destroy(&val);

	it_u("match('transparent')", color, CSS_COLOR_TRANSPARENT);

	color = 0;
	if (css_parse_value(valdef, "rgb(100, 200, 255)", &val) > 0 &&
	    val.array_value[0].type == CSS_COLOR_VALUE) {
		color = val.array_value[0].color_value;
	}
	css_style_value_destroy(&val);

	it_u("match('rgb(100, 200, 255)')", color,
	     css_color(255, 100, 200, 255));

	color = 0;
	if (css_parse_value(valdef, "rgba(100, 200, 255, 0.5)", &val) > 0 &&
	    val.array_value[0].type == CSS_COLOR_VALUE) {
		color = val.array_value[0].color_value;
	}
	css_style_value_destroy(&val);

	it_u("match('rgba(100, 200, 255, 0.5)')", color,
	     css_color(127, 100, 200, 255));
}

static void test_css_valdef_none(const css_valdef_t *valdef)
{
	int ret;
	css_style_value_t val = { 0 };

	ret = css_parse_value(valdef, "none", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE;
	if (ret) {
		ret = val.array_value[0].keyword_value ==
		      css_get_keyword_key("none");
	}
	css_style_value_destroy(&val);
	it_b("match('none')", ret, 1);
	it_b("notMatch('auto')", css_parse_value(valdef, "auto", &val) <= 0, 1);
	css_style_value_destroy(&val);
}

static void test_css_valdef_none_or_auto(const css_valdef_t *valdef)
{
	int ret;
	css_style_value_t val = { 0 };

	ret = css_parse_value(valdef, "none", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE;
	if (ret) {
		ret = val.array_value[0].keyword_value ==
		      css_get_keyword_key("none");
	}
	css_style_value_destroy(&val);

	it_b("match('none')", ret, 1);

	ret = css_parse_value(valdef, "auto", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE;
	it_b("match('auto')", ret, 1);
	it_b("notMatch('normal')", css_parse_value(valdef, "normal", &val) <= 0,
	     1);
	css_style_value_destroy(&val);
}

static void test_css_valdef_border(const css_valdef_t *valdef)
{
	int ret;
	css_style_value_t val = { 0 };

	it_b("match('1px solid #eee')",
	     css_parse_value(valdef, "1px solid #eee", &val) > 0, 1);
	css_style_value_destroy(&val);

	it_b("match('#eee 1px solid')",
	     css_parse_value(valdef, "#eee 1px solid", &val) > 0, 1);
	css_style_value_destroy(&val);

	it_b("match('solid #eee 1px')",
	     css_parse_value(valdef, "solid #eee 1px", &val) > 0, 1);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "1px", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE;
	if (ret) {
		ret = val.array_value[0].type == CSS_UNIT_VALUE &&
		      val.array_value[0].unit_value.value == 1.;
	}
	it_b("match('1px')", ret, 1);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "solid", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE;
	if (ret) {
		ret = val.array_value[0].type == CSS_KEYWORD_VALUE &&
		      val.array_value[0].keyword_value ==
			  css_get_keyword_key("solid");
	}
	it_b("match('solid')", ret, 1);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "#eee", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE;
	ret = val.array_value[0].color_value == 0xffeeeeee;
	it_b("match('#eee')", ret, 1);
	css_style_value_destroy(&val);
}

static void test_css_valdef_border_2(const css_valdef_t *valdef)
{
	int ret;
	css_style_value_t val = { 0 };

	it_b("match('1px solid #eee')",
	     css_parse_value(valdef, "1px solid #eee", &val) > 0, 1);
	css_style_value_destroy(&val);

	it_b("match('#eee 1px solid')",
	     css_parse_value(valdef, "#eee 1px solid", &val) > 0, 1);
	css_style_value_destroy(&val);

	it_b("match('solid #eee 1px')",
	     css_parse_value(valdef, "solid #eee 1px", &val) > 0, 1);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "1px", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE;
	it_b("notMatch('1px')", ret, 0);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "solid", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE;
	it_b("notMatch('solid')", ret, 0);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "#eee", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE;
	it_b("notMatch('#eee')", ret, 0);
	css_style_value_destroy(&val);
}

static void test_css_valdef_box_shadow(const css_valdef_t *valdef)
{
	css_style_value_t val = { 0 };

	it_b("match('none')",
	     css_parse_value(valdef, "none", &val) > 0 &&
		 val.array_value[0].keyword_value == CSS_KEYWORD_NONE,
	     1);
	css_style_value_destroy(&val);

	it_b("match('0 0')",
	     css_parse_value(valdef, "0 0", &val) > 0 &&
		 css_style_value_get_array_length(&val) == 2,
	     1);
	css_style_value_destroy(&val);

	it_b("match('0 0 4px')",
	     css_parse_value(valdef, "0 0 4px", &val) > 0 &&
		 css_style_value_get_array_length(&val) == 3,
	     1);
	css_style_value_destroy(&val);

	it_b("match('0 0 4px #000')",
	     css_parse_value(valdef, "0 0 4px #000", &val) > 0 &&
		 css_style_value_get_array_length(&val) == 4,
	     1);
	css_style_value_destroy(&val);

	it_b(
	    "match('0 0 6px rgba(33,150,243,0.4)')",
	    css_parse_value(valdef, "0 0 6px rgba(33,150,243,0.4)", &val) > 0 &&
		css_style_value_get_array_length(&val) == 4 &&
		val.array_value[3].color_value ==
		    css_color((uint8_t)(0.4 * 0xff), 33, 150, 243),
	    1);
	css_style_value_destroy(&val);
}

static void test_css_valdef_length_2(const css_valdef_t *valdef)
{
	int ret;
	css_style_value_t val = { 0 };

	ret = css_parse_value(valdef, "1px 2px", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE &&
	      css_style_value_get_array_length(&val) == 2;
	it_b("match('1px 2px')", ret, 1);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "1px", &val) != 0;
	it_b("notMatch('1px')", ret, 1);
	css_style_value_destroy(&val);
}

static void test_css_valdef_length_1_4(const css_valdef_t *valdef)
{
	int ret;
	css_style_value_t val = { 0 };

	ret = css_parse_value(valdef, "", &val) <= 0;
	it_b("notMatch('')", ret, 1);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "1px", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE &&
	      css_style_value_get_array_length(&val) == 1;
	it_b("match('1px')", ret, 1);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "1px 2px", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE &&
	      css_style_value_get_array_length(&val) == 2;
	it_b("match('1px 2px')", ret, 1);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "1px 2px 3px", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE &&
	      css_style_value_get_array_length(&val) == 3;
	it_b("match('1px 2px 3px')", ret, 1);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "1px 2px 3px 4px", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE &&
	      css_style_value_get_array_length(&val) == 4;
	it_b("match('1px 2px 3px 4px')", ret, 1);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "1px 2px 3px 4px 5px", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE &&
	      css_style_value_get_array_length(&val) == 4;
	it_b("Match('1px 2px 3px 4px 5px')", ret, 1);
	css_style_value_destroy(&val);
}

static void test_css_valdef_length_percentage_1_4(const css_valdef_t *valdef)
{
	int ret;
	css_style_value_t val = { 0 };

	ret = css_parse_value(valdef, "", &val) <= 0;
	it_b("notMatch('')", ret, 1);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "1px", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE &&
	      css_style_value_get_array_length(&val) == 1;
	it_b("match('1px')", ret, 1);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "1px", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE &&
	      css_style_value_get_array_length(&val) == 1;
	it_b("match('50%')", ret, 1);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "1px 2px", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE &&
	      css_style_value_get_array_length(&val) == 2;
	it_b("match('1px 2px')", ret, 1);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "1px 2px", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE &&
	      css_style_value_get_array_length(&val) == 2;
	it_b("match('50% 50%')", ret, 1);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "1px 2px 3px", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE &&
	      css_style_value_get_array_length(&val) == 3;
	it_b("match('1px 2px 3px')", ret, 1);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "1px 2px 3px", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE &&
	      css_style_value_get_array_length(&val) == 3;
	it_b("match('50% 50% 50%')", ret, 1);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "1px 2px 3px 4px", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE &&
	      css_style_value_get_array_length(&val) == 4;
	it_b("match('1px 2px 3px 4px')", ret, 1);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "1px 2px 3px 4px", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE &&
	      css_style_value_get_array_length(&val) == 4;
	it_b("match('50% 50% 50% 50%')", ret, 1);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "1px 2px 3px 4px 5px", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE &&
	      css_style_value_get_array_length(&val) == 4;
	it_b("Match('1px 2px 3px 4px 5px')", ret, 1);
	css_style_value_destroy(&val);
}

static void test_css_valdef_background_position(const css_valdef_t *valdef)
{
	int ret;
	size_t len;
	css_style_value_t val = { 0 };

	ret = css_parse_value(valdef, "center", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE &&
	      css_style_value_get_array_length(&val) == 1;
	it_b("match('center')", ret, 1);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "10px", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE &&
	      css_style_value_get_array_length(&val) == 1;
	it_b("match('10px')", ret, 1);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "50%", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE &&
	      css_style_value_get_array_length(&val) == 1;
	it_b("match('50%')", ret, 1);
	css_style_value_destroy(&val);

	if (css_parse_value(valdef, "top center", &val) > 0) {
		len = css_style_value_get_array_length(&val);
		ret = len == 2;
	} else {
		ret = 0;
	}
	it_b("match('top center')", ret, 1);
	css_style_value_destroy(&val);

	ret = css_parse_value(valdef, "bottom 10px right 20px", &val) > 0 &&
	      val.type == CSS_ARRAY_VALUE &&
	      css_style_value_get_array_length(&val) == 4;
	it_b("match('bottom 10px right 20px')", ret, 1);
	css_style_value_destroy(&val);
}

static void test_css_valdef(const char *definition,
			    void (*func)(const css_valdef_t *))
{
	char str[256] = { 0 };
	css_valdef_t *valdef;

	snprintf(str, 255, "valdef('%s')", definition);
	test_msg("%s\n", str);
	test_begin();
	valdef = css_compile_valdef(definition);
	if (valdef) {
		func(valdef);
	} else {
		it_b("should be able to parse", 0, 1);
	}
	test_end();
	test_msg("\n");
	css_valdef_destroy(valdef);
}

void test_css_value(void)
{
	css_init();

	/**
	 * TODO: 添加 <font-family> 测试
	 * 参考：https://developer.mozilla.org/zh-CN/docs/Web/CSS/Value_definition_syntax#%E4%BA%95%E5%8F%B7
	 * 添加支持用 # 号标识值能出现一次或多次，多次时以逗号分隔
	*/
	test_css_valdef("<string>", test_css_valdef_string);
	test_css_valdef("<color>", test_css_valdef_color);
	test_css_valdef("none", test_css_valdef_none);
	test_css_valdef("none | auto", test_css_valdef_none_or_auto);
	test_css_valdef("<line-width> || <line-style> || <color>",
			test_css_valdef_border);
	test_css_valdef("<line-width> && <line-style> && <color>",
			test_css_valdef_border_2);
	test_css_valdef("none | <shadow>", test_css_valdef_box_shadow);
	test_css_valdef("<length>{2}", test_css_valdef_length_2);
	test_css_valdef("<length>{1, 4}", test_css_valdef_length_1_4);
	test_css_valdef("<length-percentage>{1, 4}",
			test_css_valdef_length_percentage_1_4);
	test_css_valdef("<bg-position>",
	test_css_valdef_background_position);
	css_destroy();
}
