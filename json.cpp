#include "stdafx.h"
#include "json.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cerrno>
#include <cstring>
#include <memory>

#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')
#define MAKE_SURE(c, ch) do{ assert(*c->json == (ch)); c->json++; } while(0)
#define PUT_CHAR(c, ch) do{ *(char*)lept_push(c, sizeof(char)) = (ch); } while(0)
#define PUTS(c, s, len) memcpy(lept_push(c, len), s, len)
#define STRING_ERROR(error) do { c->top = head; return error; }while(0)
#define INITIAL_SIZE 256

void* Lept_value::lept_push(Lept_context* c, size_t size) {
	void *ret = nullptr;
	if (c->top + size >= c->capacity) {
		if (c->capacity == 0)
			c->capacity = INITIAL_SIZE;
		while (c->top + size >= c->capacity)
			c->capacity += c->capacity;
		char *tmp = nullptr;
		tmp = (char*)realloc(c->stack, c->capacity);
		if (tmp != nullptr)
			c->stack = tmp;
	}
	ret = c->stack + c->top;
	c->top += size;
	return ret;
}

void* Lept_value::lept_pop(Lept_context* c, size_t size) {
	assert(c->top >= size);
	return c->stack + (c->top -= size);
}

void Lept_value::set_string(const char* string, size_t len) {
	free();
	u.s.s = (char*)malloc(sizeof(char) * (len + 1));
	if (u.s.s != nullptr) {
		memcpy(u.s.s, string, len);
		u.s.s[len] = '\0';
		u.s.len = len;
		type = LEPT_STRING;
	}
	else
		type = LEPT_NULL;
}


void Lept_value::free() {
	if (type == LEPT_STRING)
		std::free(u.s.s);
	else if (type == LEPT_ARRAY) {
		for (int i = u.a.size - 1; i >= 0; --i)
			u.a.e[i].free();
		std::free(u.a.e);
	}
	else if (type == LEPT_OBJECT) {
		for (int i = u.o.size - 1; i >= 0; --i) {
			u.o.m[i].v.free();
			std::free(u.o.m[i].k);
		}
		std::free(u.o.m);
	}
	type = LEPT_NULL;
}

void Lept_value::lept_parse_whitespace(Lept_context* c) {
	assert(c != nullptr);
	while (*c->json == ' ' || *c->json == '\n' || *c->json == '\t' || *c->json == '\r')
		c->json++;
}


int Lept_value::lept_parse_literal(Lept_context* c, const char *literal, Lept_type set_type) {
	MAKE_SURE(c, *literal);
	const char *p = c->json;
	for (literal++; *literal != '\0';) {
		if (*p++ != *literal++)
			return LEPT_INVALID_VALUE;
	}
	c->json = p;
	type = set_type;
	return LEPT_PARSE_OK;
}

int Lept_value::lept_parse_number(Lept_context* c) {
	const char* p = c->json;
	if (*p == '-')
		++p;
	if (*p == '0') {
		++p;
	}
	else {
		if (ISDIGIT1TO9(*p)) {
			for (++p; ISDIGIT(*p); ++p);
		}
		else
			return LEPT_INVALID_VALUE;
	}
	if (*p == '.') {
		++p;
		if (!ISDIGIT(*p))
			return LEPT_INVALID_VALUE;
		for (++p; ISDIGIT(*p); ++p);
	}
	if (*p == 'e' || *p == 'E') {
		++p;
		if (*p == '+' || *p == '-')
			++p;
		if (!ISDIGIT(*p))
			return LEPT_INVALID_VALUE;
		for (++p; ISDIGIT(*p); ++p);
	}
	errno = 0;
	u.n = strtod(c->json, nullptr); /*因为strtod会产生+INFINITY和-INFINITY这样的结果，所以可以把负号消掉*/
	if (u.n == -INFINITY)
		u.n = INFINITY;
	if (errno == ERANGE && u.n == HUGE_VAL) { /*该种情况下才是too big number，其他下溢不属于这种错误*/
		return LEPT_NUMBER_TOO_BIG;
	}
	type = LEPT_NUMBER;
	c->json = p;
	return LEPT_PARSE_OK;
}


const char* Lept_value::lept_parse_hex4(const char* p, unsigned* u) {
	unsigned ret = 0;
	char ch;
	for (int i = 0; i < 4; ++i) {
		ch = *p++;
		ret <<= 4;
		if (ch >= 'a' && ch <= 'f')
			ret += (ch - 'a') + 10;
		else if (ch >= 'A' && ch <= 'F')
			ret += (ch - 'A') + 10;
		else if (ch >= '0' && ch <= '9')
			ret += (ch - '0');
		else {
			p = nullptr;
			break;
		}
	}
	*u = ret;
	return p;
}

void Lept_value::lept_encode_utf8(Lept_context* c, unsigned u) {
	assert(u <= 0x10ffff);
	if (u <= 0x007f) {
		PUT_CHAR(c, u);
	}
	else if (u >= 0x0080 && u <= 0x07ff) {
		PUT_CHAR(c, (0xc0 | ((u >> 6) & 0x1f)));
		PUT_CHAR(c, (0x80 | u & 0x3f));
	}
	else if (u >= 0x0800 && u <= 0xffff) {
		PUT_CHAR(c, (0xe0 | ((u >> 12) & 0x0f)));
		PUT_CHAR(c, (0x80 | ((u >> 6) & 0x3f)));
		PUT_CHAR(c, (0x80 | u & 0x3f));
	}
	else {
		PUT_CHAR(c, (0xf0 | ((u >> 18) & 0x07)));
		PUT_CHAR(c, (0x80 | ((u >> 12) & 0x3f)));
		PUT_CHAR(c, (0x80 | ((u >> 6) & 0x3f)));
		PUT_CHAR(c, (0x80 | (u & 0x3f)));
	}
}

int Lept_value::lept_parse_string(Lept_context* c) {
	MAKE_SURE(c, '\"');
	size_t len = 0, head = c->top;
	unsigned u = 0;
	const char *p = c->json;
	for (;;) {
		char ch = *p++;
		switch (ch) {
		case '\0':
			STRING_ERROR(LEPT_MISS_QUOTATION_MARK);
		case '\"':
			len = c->top - head;
			set_string((char*)lept_pop(c, len), len);
			c->json = p;
			return LEPT_PARSE_OK;
		case '\\':
			switch (*p++) {
			case 'b': PUT_CHAR(c, '\b'); break;
			case 'r': PUT_CHAR(c, '\r'); break;
			case 'n': PUT_CHAR(c, '\n'); break;
			case 'f': PUT_CHAR(c, '\f'); break;
			case 't': PUT_CHAR(c, '\t'); break;
			case '"': PUT_CHAR(c, '\"'); break;
			case '\\': PUT_CHAR(c, '\\'); break;
			case '/': PUT_CHAR(c, '/'); break;
			case 'u':
				if ((p = lept_parse_hex4(p, &u)) == nullptr)
					STRING_ERROR(LEPT_INVALID_UNICODE_HEX);
				if (u >= 0xd800 && u <= 0xdbff) {
					if (*p != '\\' || *(p + 1) != 'u')
						STRING_ERROR(LEPT_INVALID_UNICODE_SURROGATE);
					unsigned u2;
					p += 2;
					if ((p = lept_parse_hex4(p, &u2)) == nullptr)
						STRING_ERROR(LEPT_INVALID_UNICODE_HEX);
					if (u2 < 0xdc00 || u2 > 0xdfff)
						STRING_ERROR(LEPT_INVALID_UNICODE_SURROGATE);
					u = 0x10000 + (u - 0xd800) * 0x400 + (u2 - 0xdc00);
				}
				lept_encode_utf8(c, u);
				break;
			default:
				STRING_ERROR(LEPT_INVALID_STRING_ESCAPE);
			}
			break;
		default:
			if ((unsigned char)ch < 0x20) {
				STRING_ERROR(LEPT_INVALID_STRING_CHAR);
			}
			PUT_CHAR(c, ch);
		}
	}
}

int Lept_value::lept_parse_array(Lept_context* c) {
	MAKE_SURE(c, '[');
	int ret;
	size_t size = 0;
	//还是漏了前面的空格
	lept_parse_whitespace(c);
	if (*c->json == ']') {
		c->json++;
		u.a.e = nullptr;
		u.a.size = 0;
		type = LEPT_ARRAY;
		return LEPT_PARSE_OK;
	}
	for (;;) {
		Lept_value e;
		e.init();
		if ((ret = e.parse(c)) != LEPT_PARSE_OK)
			break;
		memcpy((Lept_value*)lept_push(c, sizeof(Lept_value)), &e, sizeof(Lept_value));
		size++;
		lept_parse_whitespace(c);
		if (*c->json == ',') {
			c->json++;
			lept_parse_whitespace(c);
		}
		else if (*c->json == ']') {
			u.a.e = (Lept_value*)malloc(sizeof(Lept_value) * size);
			memcpy(u.a.e, (Lept_value*)lept_pop(c, size * sizeof(Lept_value)), size * sizeof(Lept_value));
			u.a.size = size;
			type = LEPT_ARRAY;
			c->json++;
			ret = LEPT_PARSE_OK;
			break;
		}
		else {
			ret = LEPT_MISS_COMMA_OR_SQUARE_BRACKET;
			break;
		}
	}
	if (ret != LEPT_PARSE_OK) {
		for (int i = size - 1; i >= 0; --i) {
			auto p = (Lept_value*)lept_pop(c, sizeof(Lept_value));
			p->free();
		}
		type = LEPT_NULL;
	}
	return ret;
}

int Lept_value::lept_parse_object(Lept_context* c) {
	MAKE_SURE(c, '{');
	int ret;
	lept_parse_whitespace(c);
	if (*c->json == '}') {
		c->json++;
		u.o.m = nullptr;
		u.o.size = 0;
		type = LEPT_OBJECT;
		return LEPT_PARSE_OK;
	}
	size_t size = 0;
	Lept_member m;
	for (;;) {
		if (*c->json == '\"') {
			Lept_value key;
			key.init();
			if ((ret = key.parse(c)) != LEPT_PARSE_OK)
				break;
			lept_parse_whitespace(c);
			m.k = key.u.s.s;
			m.klen = key.u.s.len;
			if (*c->json != ':') {
				ret = LEPT_MISS_COLON;
				std::free(m.k);
				break;
			}
			c->json++;
			lept_parse_whitespace(c);
			if ((ret = m.v.parse(c)) != LEPT_PARSE_OK) {
				std::free(m.k);
				break;
			}
			memcpy((Lept_member*)lept_push(c, sizeof(Lept_member)), &m, sizeof(Lept_member));
			size++;
			m.k = nullptr;
			lept_parse_whitespace(c);
			if (*c->json == ',') {
				c->json++;
				lept_parse_whitespace(c);
			}
			else if (*c->json == '}') {
				c->json++;
				u.o.m = (Lept_member*)malloc(sizeof(Lept_member) * size);
				memcpy(u.o.m, (Lept_member*)lept_pop(c, size * sizeof(Lept_member)), size * sizeof(Lept_member));
				u.o.size = size;
				type = LEPT_OBJECT;
				return LEPT_PARSE_OK;
			}
			else {
				ret = LEPT_MISS_COMMA_OR_CURLY_BRACKET;
				break;
			}
		}
		else {
			ret = LEPT_MISS_KEY;
			break;
		}
	}
	m.k = nullptr;
	for (int i = size - 1; i >= 0; --i) {
		auto p = (Lept_member*)lept_pop(c, sizeof(Lept_member));
		p->v.free();
		std::free(p->k);
	}
	type = LEPT_NULL;
	return ret;
}
int Lept_value::parse(Lept_context* c) {
	assert(c != nullptr);
	switch (*c->json) {
	case 'n': return lept_parse_literal(c, "null", LEPT_NULL);
	case 't': return lept_parse_literal(c, "true", LEPT_TRUE);
	case 'f': return lept_parse_literal(c, "false", LEPT_FALSE);
	case '\"': return lept_parse_string(c);
	case '\0': return LEPT_EXPECT_VALUE;
	default: return lept_parse_number(c);
	case '[': return lept_parse_array(c);
	case '{': return lept_parse_object(c);
	}
}
int Lept_value::parse(const char *json) {
	Lept_context c;
	int ret;
	c.json = json;
	c.stack = nullptr;
	c.top = c.capacity = 0;
	init();
	lept_parse_whitespace(&c);
	if ((ret = parse(&c)) == LEPT_PARSE_OK) {
		lept_parse_whitespace(&c);
		if (*c.json != '\0') {
			type = LEPT_NULL;
			ret = LEPT_NOT_ROOT_SINGULAR;
		}
	}
	assert(c.top == 0);
	std::free(c.stack);
	return ret;
}

//
//void Lept_value::stringify_string(Lept_context* c, const char* str, size_t len)
//{
//	const char* p = str;
//	PUT_CHAR(c, '\"');
//	for (;*p != '\0';) {
//		unsigned char ch = (unsigned char)*p++;
//		switch (ch) {
//		case '\b': PUT_CHAR(c, '\\'); PUT_CHAR(c, 'b'); break;
//		case '\f': PUT_CHAR(c, '\\'); PUT_CHAR(c, 'f'); break;
//		case '\n': PUT_CHAR(c, '\\'); PUT_CHAR(c, 'n'); break;
//		case '\t': PUT_CHAR(c, '\\'); PUT_CHAR(c, 't'); break;
//		case '\r': PUT_CHAR(c, '\\'); PUT_CHAR(c, 'r'); break;
//		case '\"': PUT_CHAR(c, '\\'); PUT_CHAR(c, '\"'); break;
//		case '\\': PUT_CHAR(c, '\\'); PUT_CHAR(c, '\\'); break;
//		default:
//			if (ch < 0x20) {
//				char buffer[7];
//				sprintf(buffer, "\\u%04X", ch);
//				PUTS(c, buffer, 6);
//			}else
//				PUT_CHAR(c, ch);
//			break;
//		}
//	}
//	PUT_CHAR(c, '\"');
//}
//void Lept_value::stringify(Lept_context* c) {
//	switch (type) {
//	case LEPT_NULL: PUTS(c, "null", 4); break;
//	case LEPT_TRUE: PUTS(c, "true", 4); break;
//	case LEPT_FALSE: PUTS(c, "false", 5); break;
//	case LEPT_NUMBER: c->top -= 32 - sprintf((char*)lept_push(c, 32), "%.17g", u.n); break;
//	case LEPT_STRING:
//		stringify_string(c, u.s.s, u.s.len); break;
//	case LEPT_ARRAY:
//		PUT_CHAR(c, '[');
//		for (size_t i = 0; i < u.a.size; ++i) {
//			u.a.e[i].stringify(c);
//			if (i != u.a.size - 1)
//				PUT_CHAR(c, ',');
//		}
//		PUT_CHAR(c, ']');
//		break;
//	case LEPT_OBJECT:
//		PUT_CHAR(c, '{');
//		for (size_t i = 0; i < u.o.size; ++i) {
//			stringify_string(c, u.o.m[i].k, u.o.m[i].klen);
//			PUT_CHAR(c, ':');
//			u.o.m[i].v.stringify(c);
//			if (i != u.o.size - 1)
//				PUT_CHAR(c, ',');
//		}
//		PUT_CHAR(c, '}');
//		break;
//	}
//}
//
//char * Lept_value::stringify(size_t *length) {
//	Lept_context c;
//
//	c.stack = (char*)malloc(INITIAL_SIZE);
//	c.top = 0;
//	c.capacity = INITIAL_SIZE;
//	stringify(&c);
//	if(length)
//		*length = c.top;
//	PUT_CHAR(&c, '\0');
//
//	return c.stack;
//}
