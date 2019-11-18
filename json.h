
#ifndef LEPTJSON_H__
#define LEPTJSON_H__
#include <cassert>
#include <memory>
#include <string>
enum Lept_type { LEPT_NULL, LEPT_FALSE, LEPT_TRUE, LEPT_NUMBER, LEPT_STRING, LEPT_ARRAY, LEPT_OBJECT };
struct Lept_context {
	const char *json;
	char *stack;
	size_t top, capacity;
};
struct Lept_member; 
struct Lept_value {
	union {
		struct { Lept_member* m; size_t size; } o;
		struct { Lept_value* e; size_t size; } a;
		struct { char* s; size_t len; } s;
		double n;
	} u;
	Lept_type type;
	int parse(Lept_context* c);
	int parse(const char *json);
	inline void init() { type = LEPT_NULL; }
	void free();
	inline Lept_type get_type() const { return type; }
	inline void set_null() { free(); }
	inline int get_boolean() const { assert(type == LEPT_TRUE || type == LEPT_FALSE); return type == LEPT_TRUE; }
	inline void set_boolean(int b) { free(); type = (b == 0) ? LEPT_FALSE : LEPT_TRUE; }
	inline double get_number() const { return u.n; }
	inline void set_number(double num) { free();  u.n = num; type = LEPT_NUMBER; }
	inline const char *get_string() const { return u.s.s; }
	inline size_t get_string_length() const { return u.s.len; }
	inline void set_string(const char *string, size_t len);
	inline size_t get_array_size() const { return u.a.size; }
	inline Lept_value &get_array_element(size_t index) { return u.a.e[index]; } /*涉及重载问题*/
	inline const Lept_value& get_array_element(size_t index) const { return u.a.e[index]; }
	inline size_t get_object_size() const { return u.o.size; }
	inline const char* get_object_key(size_t index) const;
	inline size_t get_object_key_length(size_t index) const;
	inline Lept_value& get_object_value(size_t index);
	inline const Lept_value& get_object_value(size_t index) const;

private:
	void lept_parse_whitespace(Lept_context* c);
	int lept_parse_number(Lept_context* c);
	int lept_parse_string(Lept_context* c);
	int lept_parse_literal(Lept_context* c, const char *literal, Lept_type set_type);
	int lept_parse_array(Lept_context* c);
	int lept_parse_object(Lept_context* c);
	void* lept_push(Lept_context* c, size_t size);
	void* lept_pop(Lept_context* c, size_t size);
	const char* lept_parse_hex4(const char* p, unsigned* u);
	void lept_encode_utf8(Lept_context* c, unsigned u);
};

struct Lept_member {
	char *k;
	size_t klen;
	Lept_value v;
};
const char* Lept_value::get_object_key(size_t index) const { return u.o.m[index].k; }
size_t Lept_value::get_object_key_length(size_t index) const { return u.o.m[index].klen; }
Lept_value& Lept_value::get_object_value(size_t index) { return u.o.m[index].v; }
const Lept_value& Lept_value::get_object_value(size_t index) const { return u.o.m[index].v; }

enum {
	LEPT_PARSE_OK = 0,
	LEPT_EXPECT_VALUE,
	LEPT_INVALID_VALUE,
	LEPT_NOT_ROOT_SINGULAR,
	LEPT_NUMBER_TOO_BIG,
	LEPT_MISS_QUOTATION_MARK,
	LEPT_INVALID_STRING_ESCAPE,
	LEPT_INVALID_STRING_CHAR,
	LEPT_INVALID_UNICODE_HEX,
	LEPT_INVALID_UNICODE_SURROGATE,
	LEPT_MISS_COMMA_OR_SQUARE_BRACKET,
	LEPT_MISS_COMMA_OR_CURLY_BRACKET,
	LEPT_MISS_KEY,
	LEPT_MISS_COLON
};

#endif /*LEPTJSON_H__*/
