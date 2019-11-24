// leptJson.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <cstdio>
#include <cstdlib>
#include "json.h"
#include <cmath>
#include <cstring>
#include <crtdbg.h>

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
do{\
	test_count++;\
	if (equality) {\
		test_pass++;\
	}\
	else {\
		fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
		main_ret = 1;\
	}\
}while(0)

#define EXPECT_EQ_INT(expect, actual) 	EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_SIZET(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%u")
#define EXPECT_EQ_DOUBLE(expect, actual)  EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) == 0, "false", "true", "%s")
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
#define EXPECT_EQ_STRING(expect, actual, alength)\
  EXPECT_EQ_BASE(((sizeof(expect) - 1 == alength) && (memcmp(expect, actual, alength) == 0)), expect, actual, "%s")

static void test_parse_null() {
	Lept_value v;
	v.init();
	EXPECT_EQ_INT(LEPT_PARSE_OK, v.parse("null"));
	EXPECT_EQ_INT(LEPT_NULL, v.get_type());
	v.free();
}
static void test_parse_false() {
	Lept_value v;
	v.init();
	EXPECT_EQ_INT(LEPT_PARSE_OK, v.parse("\nfalse  "));
	EXPECT_EQ_INT(LEPT_FALSE, v.get_type());
	v.free();
}
static void test_parse_true() {
	Lept_value v;
	v.init();
	EXPECT_EQ_INT(LEPT_PARSE_OK, v.parse("  true"));
	EXPECT_EQ_INT(LEPT_TRUE, v.get_type());
	v.free();
}

#define TEST_NUMBER(num, json)\
do{\
	Lept_value v;\
	v.init();\
	EXPECT_EQ_INT(LEPT_PARSE_OK, v.parse(json));\
	EXPECT_EQ_DOUBLE(num, v.get_number());\
	EXPECT_EQ_INT(LEPT_NUMBER, v.get_type());\
	v.free();\
}while(0)


static void test_parse_number() {
	TEST_NUMBER(0.0, "0");
	TEST_NUMBER(0.0, "-0");
	TEST_NUMBER(0.0, "-0.0");
	TEST_NUMBER(1.0, "1");
	TEST_NUMBER(-1.0, "-1");
	TEST_NUMBER(1.5, "1.5");
	TEST_NUMBER(-1.5, "-1.5");
	TEST_NUMBER(3.1416, "3.1416");
	TEST_NUMBER(1E10, "1E10");
	TEST_NUMBER(1e10, "1e10");
	TEST_NUMBER(1E+10, "1E+10");
	TEST_NUMBER(1E-10, "1E-10");
	TEST_NUMBER(-1E10, "-1E10");
	TEST_NUMBER(-1e10, "-1e10");
	TEST_NUMBER(-1E+10, "-1E+10");
	TEST_NUMBER(-1E-10, "-1E-10");
	TEST_NUMBER(1.234E+10, "1.234E+10");
	TEST_NUMBER(1.234E-10, "1.234E-10");
	TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

	TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
	TEST_NUMBER(4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
	TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
	TEST_NUMBER(2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
	TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
	TEST_NUMBER(2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
	TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
	TEST_NUMBER(1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
	TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

#define TEST_STRING(string, json)\
do{\
	Lept_value v;\
	v.init();\
	EXPECT_EQ_INT(LEPT_PARSE_OK, v.parse(json));\
	EXPECT_EQ_STRING(string, v.get_string(), v.get_string_length());\
	EXPECT_EQ_INT(LEPT_STRING, v.get_type());\
	v.free();\
}while(0)

static void test_parse_string() {
	TEST_STRING("", "\"\"");
	TEST_STRING("Hello", "\"Hello\"");
	TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
	TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
	TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
	TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
	TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
	TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");  /* G clef sign U+1D11E */
	TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");  /* G clef sign U+1D11E */
}

static void test_parse_array() {
	Lept_value v;
	v.init();
	EXPECT_EQ_INT(LEPT_PARSE_OK, v.parse("[]"));
	EXPECT_EQ_INT(LEPT_ARRAY, v.get_type());
	EXPECT_EQ_SIZET(0, v.get_array_size());
	v.free();

	v.init();
	EXPECT_EQ_INT(LEPT_PARSE_OK, v.parse("[null , false , true, \"abc\", 1.2 ]"));
	EXPECT_EQ_INT(LEPT_ARRAY, v.get_type());
	EXPECT_EQ_SIZET(5, v.get_array_size());
	EXPECT_EQ_INT(LEPT_NULL, v.get_array_element(0).get_type());
	EXPECT_EQ_INT(LEPT_FALSE, v.get_array_element(1).get_type());
	EXPECT_EQ_INT(LEPT_TRUE, v.get_array_element(2).get_type());
	EXPECT_EQ_INT(LEPT_STRING, v.get_array_element(3).get_type());
	EXPECT_EQ_INT(LEPT_NUMBER, v.get_array_element(4).get_type());
	EXPECT_EQ_STRING("abc", v.get_array_element(3).get_string(), v.get_array_element(3).get_string_length());
	EXPECT_EQ_DOUBLE(1.2, v.get_array_element(4).get_number());
	v.free();

	v.init();
	EXPECT_EQ_INT(LEPT_PARSE_OK, v.parse("[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
	EXPECT_EQ_INT(LEPT_ARRAY, v.get_type());
	EXPECT_EQ_SIZET(4, v.get_array_size());
	for (int i = 0; i < 4; i++) {
		Lept_value& a = v.get_array_element(i);
		EXPECT_EQ_INT(LEPT_ARRAY, a.get_type());
		EXPECT_EQ_SIZET(i, a.get_array_size());
		for (int j = 0; j < i; j++) {
			Lept_value& e = a.get_array_element(j);
			EXPECT_EQ_INT(LEPT_NUMBER, e.get_type());
			EXPECT_EQ_DOUBLE((double)j, e.get_number());
		}
	}
	v.free();
}

static void test_parse_object() {
	Lept_value v;
	size_t i;

	v.init();
	EXPECT_EQ_INT(LEPT_PARSE_OK, v.parse(" { } "));
	EXPECT_EQ_INT(LEPT_OBJECT, v.get_type());
	EXPECT_EQ_SIZET(0, v.get_object_size());
	v.free();

	v.init();
	EXPECT_EQ_INT(LEPT_PARSE_OK, v.parse(
		" { "
		"\"n\" : null , "
		"\"f\" : false , "
		"\"t\" : true , "
		"\"i\" : 123 , "
		"\"s\" : \"abc\" , "
		"\"a\" : [ 1, 2, 3 ],"
		"\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
		" } "
	));
	EXPECT_EQ_INT(LEPT_OBJECT, v.get_type());
	EXPECT_EQ_SIZET(7, v.get_object_size());
	EXPECT_EQ_STRING("n", v.get_object_key(0), v.get_object_key_length(0));
	EXPECT_EQ_INT(LEPT_NULL, v.get_object_value(0).get_type());
	EXPECT_EQ_STRING("f", v.get_object_key(1), v.get_object_key_length(1));
	EXPECT_EQ_INT(LEPT_FALSE, v.get_object_value(1).get_type());
	EXPECT_EQ_STRING("t", v.get_object_key(2), v.get_object_key_length(2));
	EXPECT_EQ_INT(LEPT_TRUE, v.get_object_value(2).get_type());
	EXPECT_EQ_STRING("i", v.get_object_key(3), v.get_object_key_length(3));
	EXPECT_EQ_INT(LEPT_NUMBER, v.get_object_value(3).get_type());
	EXPECT_EQ_DOUBLE(123.0, v.get_object_value(3).get_number());
	EXPECT_EQ_STRING("s", v.get_object_key(4), v.get_object_key_length(4));
	EXPECT_EQ_INT(LEPT_STRING, v.get_object_value(4).get_type());
	EXPECT_EQ_STRING("abc", v.get_object_value(4).get_string(), v.get_object_value(4).get_string_length());
	EXPECT_EQ_STRING("a", v.get_object_key(5), v.get_object_key_length(5));
	EXPECT_EQ_INT(LEPT_ARRAY, v.get_object_value(5).get_type());
	EXPECT_EQ_SIZET(3, v.get_object_value(5).get_array_size());
	for (i = 0; i < 3; i++) {
		Lept_value& e = v.get_object_value(5).get_array_element(i);
		EXPECT_EQ_INT(LEPT_NUMBER, e.get_type());
		EXPECT_EQ_DOUBLE(i + 1.0, e.get_number());
	}
	EXPECT_EQ_STRING("o", v.get_object_key(6), v.get_object_key_length(6));
	{
		Lept_value& o = v.get_object_value(6);
		EXPECT_EQ_INT(LEPT_OBJECT, o.get_type());
		for (i = 0; i < 3; i++) {
			Lept_value& oi = o.get_object_value(i);
			EXPECT_TRUE('1' + i == o.get_object_key(i)[0]);
			EXPECT_EQ_SIZET(1, o.get_object_key_length(i));
			EXPECT_EQ_INT(LEPT_NUMBER, oi.get_type());
			EXPECT_EQ_DOUBLE(i + 1.0, oi.get_number());
		}
	}
	v.free();

}
static void test_parse() {
	test_parse_null();
	test_parse_false();
	test_parse_true();
	test_parse_number();
	test_parse_string();
	test_parse_array();
	test_parse_object();
}

#define TEST_ROUNDTRIP(json) \
do{\
	Lept_value v;\
	v.init();\
	size_t length = 0;\
	char* json2;\
	EXPECT_EQ_INT(LEPT_PARSE_OK, v.parse(json)); \
	json2 = v.stringify(&length);\
	EXPECT_EQ_STRING(json, json2, length);\
	v.free();\
	free(json2);\
}while(0)
//
//static void test_stringify() {
//	TEST_ROUNDTRIP("null");
//	TEST_ROUNDTRIP("true");
//	TEST_ROUNDTRIP("false");
//
//	TEST_ROUNDTRIP("0");
//	TEST_ROUNDTRIP("-0");
//	TEST_ROUNDTRIP("1");
//	TEST_ROUNDTRIP("-1");
//	TEST_ROUNDTRIP("1.5");
//	TEST_ROUNDTRIP("-1.5");
//	TEST_ROUNDTRIP("3.25");
//	TEST_ROUNDTRIP("1e+20");
//	TEST_ROUNDTRIP("1.234e+20");
//	TEST_ROUNDTRIP("1.234e-20");
//
//	TEST_ROUNDTRIP("1.0000000000000002"); /* the smallest number > 1 */
//	TEST_ROUNDTRIP("4.9406564584124654e-324"); /* minimum denormal */
//	TEST_ROUNDTRIP("-4.9406564584124654e-324");
//	TEST_ROUNDTRIP("2.2250738585072009e-308");  /* Max subnormal double */
//	TEST_ROUNDTRIP("-2.2250738585072009e-308");
//	TEST_ROUNDTRIP("2.2250738585072014e-308");  /* Min normal positive double */
//	TEST_ROUNDTRIP("-2.2250738585072014e-308");
//	TEST_ROUNDTRIP("1.7976931348623157e+308");  /* Max double */
//	TEST_ROUNDTRIP("-1.7976931348623157e+308");
//
//	TEST_ROUNDTRIP("\"\"");
//	TEST_ROUNDTRIP("\"Hello\"");
//	TEST_ROUNDTRIP("\"Hello\\nWorld\"");
//	TEST_ROUNDTRIP("\"\\\" \\\\ / \\b \\f \\n \\r \\t\"");
//	TEST_ROUNDTRIP("\"Hello\\u0000World\"");
//	TEST_ROUNDTRIP("\"\\u20AC\"");
//
//	TEST_ROUNDTRIP("[]");
//	TEST_ROUNDTRIP("[null,false,true,123,\"abc\",[1,2,3]]");
//
//	TEST_ROUNDTRIP("{}");
//	TEST_ROUNDTRIP("{\"n\":null,\"f\":false,\"t\":true,\"i\":123,\"s\":\"abc\",\"a\":[1,2,3],\"o\":{\"1\":1,\"2\":2,\"3\":3}}");
//}

#define TEST_ERROR(error, json)\
do{\
	Lept_value v;\
	v.init();\
	v.type = LEPT_FALSE;\
	EXPECT_EQ_INT(error, v.parse(json));\
	EXPECT_EQ_INT(LEPT_NULL, v.get_type());\
	v.free();\
}while(0)

static void test_expect_value() {
	TEST_ERROR(LEPT_EXPECT_VALUE, " ");
	TEST_ERROR(LEPT_EXPECT_VALUE, "\n");
	TEST_ERROR(LEPT_EXPECT_VALUE, "\t");
	TEST_ERROR(LEPT_EXPECT_VALUE, "");
}

static void test_invalid_value() {
	TEST_ERROR(LEPT_INVALID_VALUE, "nul");
	TEST_ERROR(LEPT_INVALID_VALUE, "-tr");
	TEST_ERROR(LEPT_INVALID_VALUE, "faltr");
	TEST_ERROR(LEPT_INVALID_VALUE, "  truu");
	TEST_ERROR(LEPT_INVALID_VALUE, "+0");
	TEST_ERROR(LEPT_INVALID_VALUE, "+1");
	TEST_ERROR(LEPT_INVALID_VALUE, ".123"); /* at least one digit before '.' */
	TEST_ERROR(LEPT_INVALID_VALUE, "1.");   /* at least one digit after '.' */
	TEST_ERROR(LEPT_INVALID_VALUE, "1e");
	TEST_ERROR(LEPT_INVALID_VALUE, "INF");
	TEST_ERROR(LEPT_INVALID_VALUE, "inf");
	TEST_ERROR(LEPT_INVALID_VALUE, "NAN");
	TEST_ERROR(LEPT_INVALID_VALUE, "nan");
}

static void test_not_root_singular() {
	TEST_ERROR(LEPT_NOT_ROOT_SINGULAR, "false  t");
	TEST_ERROR(LEPT_NOT_ROOT_SINGULAR, "truen");
	TEST_ERROR(LEPT_NOT_ROOT_SINGULAR, "0123"); /* after zero should be '.' or nothing */
	TEST_ERROR(LEPT_NOT_ROOT_SINGULAR, "0x0");
	TEST_ERROR(LEPT_NOT_ROOT_SINGULAR, "0x123");
}

static void test_number_too_big() {
	TEST_ERROR(LEPT_NUMBER_TOO_BIG, "1e309");
	TEST_ERROR(LEPT_NUMBER_TOO_BIG, "-1e309");
}

static void test_missing_quotation_mark() {
	TEST_ERROR(LEPT_MISS_QUOTATION_MARK, "\"");
	TEST_ERROR(LEPT_MISS_QUOTATION_MARK, "\"abc");
}

static void test_invalid_string_escape() {
	TEST_ERROR(LEPT_INVALID_STRING_ESCAPE, "\"\\v\"");
	TEST_ERROR(LEPT_INVALID_STRING_ESCAPE, "\"\\'\"");
	TEST_ERROR(LEPT_INVALID_STRING_ESCAPE, "\"\\0\"");
	TEST_ERROR(LEPT_INVALID_STRING_ESCAPE, "\"\\x12\"");

}

static void test_invalid_string_char() {

	TEST_ERROR(LEPT_INVALID_STRING_CHAR, "\"\x01\"");
	TEST_ERROR(LEPT_INVALID_STRING_CHAR, "\"\x1F\"");

}

static void test_invalid_unicode_hex() {
	TEST_ERROR(LEPT_INVALID_UNICODE_HEX, "\"\\u\"");
	TEST_ERROR(LEPT_INVALID_UNICODE_HEX, "\"\\u0\"");
	TEST_ERROR(LEPT_INVALID_UNICODE_HEX, "\"\\u01\"");
	TEST_ERROR(LEPT_INVALID_UNICODE_HEX, "\"\\u012\"");
	TEST_ERROR(LEPT_INVALID_UNICODE_HEX, "\"\\u/000\"");
	TEST_ERROR(LEPT_INVALID_UNICODE_HEX, "\"\\uG000\"");
	TEST_ERROR(LEPT_INVALID_UNICODE_HEX, "\"\\u0/00\"");
	TEST_ERROR(LEPT_INVALID_UNICODE_HEX, "\"\\u0G00\"");
	TEST_ERROR(LEPT_INVALID_UNICODE_HEX, "\"\\u00/0\"");
	TEST_ERROR(LEPT_INVALID_UNICODE_HEX, "\"\\u00G0\"");
	TEST_ERROR(LEPT_INVALID_UNICODE_HEX, "\"\\u000/\"");
	TEST_ERROR(LEPT_INVALID_UNICODE_HEX, "\"\\u000G\"");
}

static void test_invalid_unicode_surrogate() {
	TEST_ERROR(LEPT_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
	TEST_ERROR(LEPT_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
	TEST_ERROR(LEPT_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
	TEST_ERROR(LEPT_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
	TEST_ERROR(LEPT_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

static void test_miss_comma_or_square_bracket() {
	TEST_ERROR(LEPT_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
	TEST_ERROR(LEPT_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
	TEST_ERROR(LEPT_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
	TEST_ERROR(LEPT_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
}

static void test_miss_key() {
	TEST_ERROR(LEPT_MISS_KEY, "{:1,");
	TEST_ERROR(LEPT_MISS_KEY, "{1:1,");
	TEST_ERROR(LEPT_MISS_KEY, "{true:1,");
	TEST_ERROR(LEPT_MISS_KEY, "{false:1,");
	TEST_ERROR(LEPT_MISS_KEY, "{null:1,");
	TEST_ERROR(LEPT_MISS_KEY, "{[]:1,");
	TEST_ERROR(LEPT_MISS_KEY, "{{}:1,");
	TEST_ERROR(LEPT_MISS_KEY, "{\"a\":1,");
}

static void test_miss_colon() {
	TEST_ERROR(LEPT_MISS_COLON, "{\"a\"}");
	TEST_ERROR(LEPT_MISS_COLON, "{\"a\",\"b\"}");
}

static void test_miss_comma_or_curly_bracket() {
	TEST_ERROR(LEPT_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1");
	TEST_ERROR(LEPT_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1]");
	TEST_ERROR(LEPT_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1 \"b\"");
	TEST_ERROR(LEPT_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{}");
}

static void test_parse_error() {
	test_expect_value();
	test_invalid_value();
	test_not_root_singular();
	test_number_too_big();
	test_missing_quotation_mark();
	test_invalid_string_escape();
	test_invalid_string_char();
	test_invalid_unicode_hex();
	test_invalid_unicode_surrogate();
	test_miss_comma_or_square_bracket();
	test_miss_key();
	test_miss_colon();
	test_miss_comma_or_curly_bracket();
}

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	test_parse();
	test_parse_error();
	test_stringify();
	printf("%d/%d, %3.2f%% passes", test_pass, test_count, static_cast<double>(test_pass) / static_cast<double>(test_count) * 100);
	return main_ret;
}

