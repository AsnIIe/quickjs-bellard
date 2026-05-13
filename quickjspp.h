#include "quickjs.h"
#if defined(_WIN32)
#include <windows.h>
#endif

#if !defined(QUICKJSPP_H)
#define QUICKJSPP_H

/*Note : Throw out of memory in case of error*/
#define JS_Mallocz(ctx, type, n) (type*)js_mallocz(ctx, sizeof(type)* n)
#define JS_MalloczRT(rt, type, n) (type*)js_mallocz_rt(rt, sizeof(type)* n)
#define JS_Free(ctx, ptr) if (ctx && ptr) { js_free(ctx, ptr); }
#define JS_FreeRT(rt, ptr) if (rt && ptr) { js_free_rt(rt, ptr); }

#define JS_DupCString(ctx, str) js_strdup(ctx, str)

/*Note : use JS_VALUE_GET_TAG(val) == tag */
#define JS_IsArgOf(val, tag) (JS_VALUE_GET_TAG(val) == tag)

/*Note : idx starts from 1, check when argc >= idx or ignore or check failed return JS_ThrowTypeError*/
#define JS_ExpectArgTypeThrow(ctx, argc, argv, idx, tag, fmt, ...)\
    if (argc >= (idx) && JS_VALUE_GET_TAG(argv[(idx-1)]) != tag) {\
        return JS_ThrowTypeError(ctx, fmt, __VA_ARGS__);\
    }\

/*Note : idx starts from 1, when argc < idx or check failed return JS_ThrowTypeError*/
#define JS_RequireArgTypeThrow(ctx, argc, argv, idx, tag, fmt, ...)\
    if (argc < (idx) || JS_VALUE_GET_TAG(argv[(idx-1)]) != tag) {\
        return JS_ThrowTypeError(ctx, fmt, __VA_ARGS__);\
    }\

/*Note : The result string allocates memory from the JSRuntime and needs to be released*/
static inline char* JS_DupCStringRT(JSRuntime* rt, const char* str) {
	char* ret = str ? JS_MalloczRT(rt, char, strlen(str) + 1) : NULL;
	return ret ? strcpy(ret, str) : NULL;
}

static inline BOOL JS_IsValidUtf8(const char* str) {
	const unsigned char* unsigned_str = (const unsigned char*)str;
	int offset = 0;
	while (str[offset] != '\0') {
		const unsigned char& c1 = unsigned_str[offset + 0];
		unsigned char c2 = unsigned_str[offset + 1];
		unsigned char c3 = unsigned_str[offset + 2];
		unsigned char c4 = unsigned_str[offset + 3];

		//prevent going outside of the string
		if (c1 == '\0')
			c2 = c3 = c4 = '\0';
		else if (c2 == '\0')
			c3 = c4 = '\0';
		else if (c3 == '\0')
			c4 = '\0';

		//size in bytes of the code point
		int n = 1;

		//See http://www.unicode.org/versions/Unicode6.0.0/ch03.pdf, Table 3-7. Well-Formed UTF-8 Byte Sequences
		// ## | Code Points         | First Byte | Second Byte | Third Byte | Fourth Byte
		// #1 | U+0000   - U+007F   | 00 - 7F    |             |            | 
		// #2 | U+0080   - U+07FF   | C2 - DF    | 80 - BF     |            | 
		// #3 | U+0800   - U+0FFF   | E0         | A0 - BF     | 80 - BF    | 
		// #4 | U+1000   - U+CFFF   | E1 - EC    | 80 - BF     | 80 - BF    | 
		// #5 | U+D000   - U+D7FF   | ED         | 80 - 9F     | 80 - BF    | 
		// #6 | U+E000   - U+FFFF   | EE - EF    | 80 - BF     | 80 - BF    | 
		// #7 | U+10000  - U+3FFFF  | F0         | 90 - BF     | 80 - BF    | 80 - BF
		// #8 | U+40000  - U+FFFFF  | F1 - F3    | 80 - BF     | 80 - BF    | 80 - BF
		// #9 | U+100000 - U+10FFFF | F4         | 80 - 8F     | 80 - BF    | 80 - BF

		if (c1 <= 0x7F) // #1 | U+0000   - U+007F, (ASCII)
			n = 1;
		else if (0xC2 <= c1 && c1 <= 0xDF &&
				 0x80 <= c2 && c2 <= 0xBF)  // #2 | U+0080   - U+07FF
			n = 2;
		else if (0xE0 == c1 &&
				 0xA0 <= c2 && c2 <= 0xBF &&
				 0x80 <= c3 && c3 <= 0xBF)  // #3 | U+0800   - U+0FFF
			n = 3;
		else if (0xE1 <= c1 && c1 <= 0xEC &&
				 0x80 <= c2 && c2 <= 0xBF &&
				 0x80 <= c3 && c3 <= 0xBF)  // #4 | U+1000   - U+CFFF
			n = 3;
		else if (0xED == c1 &&
				 0x80 <= c2 && c2 <= 0x9F &&
				 0x80 <= c3 && c3 <= 0xBF)  // #5 | U+D000   - U+D7FF
			n = 3;
		else if (0xEE <= c1 && c1 <= 0xEF &&
				 0x80 <= c2 && c2 <= 0xBF &&
				 0x80 <= c3 && c3 <= 0xBF)  // #6 | U+E000   - U+FFFF
			n = 3;
		else if (0xF0 == c1 &&
				 0x90 <= c2 && c2 <= 0xBF &&
				 0x80 <= c3 && c3 <= 0xBF &&
				 0x80 <= c4 && c4 <= 0xBF)  // #7 | U+10000  - U+3FFFF
			n = 4;
		else if (0xF1 <= c1 && c1 <= 0xF3 &&
				 0x80 <= c2 && c2 <= 0xBF &&
				 0x80 <= c3 && c3 <= 0xBF &&
				 0x80 <= c4 && c4 <= 0xBF)  // #8 | U+40000  - U+FFFFF
			n = 4;
		else if (0xF4 == c1 &&
				 0x80 <= c2 && c2 <= 0xBF &&
				 0x80 <= c3 && c3 <= 0xBF &&
				 0x80 <= c4 && c4 <= 0xBF)  // #7 | U+10000  - U+3FFFF
			n = 4;
		else
			return FALSE; // invalid UTF-8 sequence

		//next code point
		offset += n;
	}
	return TRUE;
}

/*Note : Auto convert the str of default system encoding to UTF8 and return JS_NewString*/
static inline JSValue JS_NewStringA(JSContext* ctx, const char* str) {
	if (!str) {
		return JS_EXCEPTION;
	}
	if (JS_IsValidUtf8(str)) {
		return JS_NewString(ctx, str);
	}
#if defined(_WIN32)
	int utf8_len = 0;
	wchar_t* wide_buffer = NULL;
	char* utf8_buffer = NULL;

	int wide_len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	if (wide_len <= 0) {
		return JS_EXCEPTION;
	}
	wide_buffer = (wchar_t*)malloc(sizeof(wchar_t) * wide_len);
	if (!wide_buffer) {
		return JS_EXCEPTION;
	}
	if (MultiByteToWideChar(CP_ACP, 0, str, -1, wide_buffer, wide_len) == 0) {
		free(wide_buffer);
		return JS_EXCEPTION;
	}
	utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide_buffer, -1, NULL, 0, NULL, NULL);
	if (utf8_len <= 0) {
		free(wide_buffer);
		return JS_EXCEPTION;
	}
	utf8_buffer = JS_Mallocz(ctx, char, utf8_len);
	if (!utf8_buffer) {
		free(wide_buffer);
		return JS_EXCEPTION;
	}
	if (WideCharToMultiByte(CP_UTF8, 0, wide_buffer, -1, utf8_buffer, utf8_len, NULL, NULL) == 0) {
		free(wide_buffer);
		JS_Free(ctx, utf8_buffer);
		return JS_EXCEPTION;
	}
	free(wide_buffer);

	JSValue ret = JS_NewString(ctx, utf8_buffer);
	JS_Free(ctx, utf8_buffer);
	return ret;
#else
#error "other types of compilers are not supported yet";
#endif
}

/*Note : Invoke JS_ToCString and auto convert UTF8 str to default system encoding */
static inline const char* JS_ToCStringA(JSContext* ctx, JSValue val) {
#if defined(_WIN32)
	const char* content = JS_ToCString(ctx, val);
	char* acp_str = NULL;
	wchar_t* wide_buffer = NULL;
	int wide_len = 0;
	int utf8_len = 0;

	wide_len = MultiByteToWideChar(CP_UTF8, 0, content, -1, NULL, 0);
	if (wide_len <= 0) {
		return NULL;
	}
	wide_buffer = (wchar_t*)malloc(sizeof(wchar_t) * wide_len);
	if (!wide_buffer) {
		return NULL;
	}
	if (MultiByteToWideChar(CP_UTF8, 0, content, -1, wide_buffer, wide_len) == 0) {
		free(wide_buffer);
		return NULL;
	}
	JS_FreeCString(ctx, content);

	utf8_len = WideCharToMultiByte(CP_ACP, 0, wide_buffer, -1, NULL, 0, NULL, NULL);
	if (utf8_len <= 0) {
		free(wide_buffer);
		return NULL;
	}
	acp_str = JS_Mallocz(ctx, char, utf8_len);
	if (!acp_str) {
		free(wide_buffer);
		return NULL;
	}
	if (WideCharToMultiByte(CP_ACP, 0, wide_buffer, -1, acp_str, utf8_len, NULL, NULL) == 0) {
		free(wide_buffer);
		JS_Free(ctx, acp_str);
		return NULL;
	}
	free(wide_buffer);
	return acp_str;
#else
#error "other types of compilers are not supported yet";
#endif
}

/*Note : Invoke JS_Free */
static inline void JS_FreeCStringA(JSContext* ctx, const char* str) {
	if (ctx && str)
		JS_Free(ctx, (char*)str);
}

#if defined(__cplusplus)
#include <iostream>
#include <functional>

namespace quickjs {
	inline std::string format_str(const char* fmt, ...) {
		static char buf[2048];
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif
		va_list args;
		va_start(args, fmt);
		vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);
#ifdef _MSC_VER
#pragma warning(default : 4996)
#endif
		return std::string(buf);
	}

	class type_error : public std::exception {
	public:
		explicit type_error(const std::string& msg) : msg_(msg) {
		}

		const char* what() const noexcept override {
			return msg_.c_str();
		}

	private:
		std::string msg_;
	};

	enum class JSType {
		object,
		array,
		integer,
		boolean,
		number,
		string,
		function,
		symbol,
		undefined,
		null,
		uninitialized,
		exception
	};

	template<typename Type>
	class JSMemRef {
	public:
		JSMemRef() noexcept = default;

		explicit JSMemRef(JSRuntime* rt, Type* ptr)
			: rt(rt), ptref(ptr) {
		}

		explicit JSMemRef(JSContext* ctx, Type* ptr)
			: rt(JS_GetRuntime(ctx)), ptref(ptr) {
		}

		JSMemRef(const JSMemRef& other) = delete;

		JSMemRef(JSMemRef&& other) noexcept {
			reset(other.rt, other.ptref);
			other.release();
		}

		~JSMemRef() {
			JS_FreeRT(rt, ptref);
			release();
		}

		JSMemRef& operator=(const JSMemRef& other) = delete;

		JSMemRef& operator=(JSMemRef&& other) noexcept {
			reset(other.rt, other.ptref);
			other.release();
			return *this;
		}

		Type* get() const noexcept {
			return ptref;
		}

		Type* operator*() const noexcept {
			return ptref;
		}

		explicit operator bool() const noexcept {
			return ptref != nullptr;
		}

		Type* release() noexcept {
			rt = nullptr;
			return std::exchange(ptref, nullptr);
		}

		void reset(JSRuntime* rt = nullptr, Type* ptr = nullptr) noexcept {
			JS_FreeRT(rt, ptref);
			rt = rt;
			ptref = ptr;
		}

		void reset(JSContext* ctx = nullptr, Type* ptr = nullptr) noexcept {
			reset(JS_GetRuntime(ctx), ptr);
		}

	private:
		Type* ptref = nullptr;
		JSRuntime* rt = nullptr;
	};

	template<typename T, typename = std::enable_if_t<!std::is_void_v<T>>>
	static inline quickjs::JSMemRef<T> make_unique(JSContext* ctx, size_t size = 1) {
		return quickjs::JSMemRef<T>(ctx, static_cast<T*>(js_mallocz(ctx, sizeof(T) * size)));
	}
	template<typename T, typename = std::enable_if_t<!std::is_void_v<T>>>
	static inline quickjs::JSMemRef<T> make_unique(JSRuntime* rt, size_t size = 1) {
		return quickjs::JSMemRef<T>(rt, static_cast<T*>(js_mallocz_rt(rt, sizeof(T) * size)));
	}


	namespace JSCStringCharacter {
		struct DefaultJSCStringCharacter {
			const char* unwrap(JSContext* ctx, JSValue val) {
				return JS_ToCString(ctx, val);
			}
			void release(JSContext* ctx, const char* str) {
				JS_FreeCString(ctx, str);
			}
		};

		struct DefaultJSCStringSystemCharacter {
			const char* unwrap(JSContext* ctx, JSValue val) {
				return JS_ToCStringA(ctx, val);
			}
			void release(JSContext* ctx, const char* str) {
				JS_FreeCStringA(ctx, str);
			}
		};
	}
	using DefaultCharacter = quickjs::JSCStringCharacter::DefaultJSCStringCharacter;
	/*convert UTF8 to the default system encoding*/
	using SystemCharacter = quickjs::JSCStringCharacter::DefaultJSCStringSystemCharacter;

	template<typename Character = quickjs::DefaultCharacter>
	class JSCStringRef {
	public:
		JSCStringRef() noexcept = default;

		explicit JSCStringRef(JSContext* ctx, JSValue val)
			:ctx(ctx), string(character.unwrap(ctx, val)) {
		}

		explicit JSCStringRef(JSContext* ctx, JSAtom atom) :ctx(ctx) {
			JSValue v = JS_AtomToValue(ctx, atom);
			string = character.unwrap(ctx, v);
			JS_FreeValue(ctx, v);
		}

		JSCStringRef(const JSCStringRef& other) = delete;

		JSCStringRef(JSCStringRef&& other) noexcept {
			reset(other.ctx, other.string);
			other.release();
		}

		~JSCStringRef() {
			if (ctx && string) {
				character.release(ctx, string);
			}
			release();
		}

		JSCStringRef& operator=(const JSCStringRef& other) = delete;

		JSCStringRef& operator=(JSCStringRef&& other) noexcept {
			reset(other.ctx, other.string);
			other.release();
			return *this;
		}

		const char* get() const noexcept {
			return string;
		}

		const char* operator*() const noexcept {
			return string;
		}

		explicit operator bool() const noexcept {
			return string != nullptr;
		}

		operator std::string() const noexcept {
			return std::string(string);
		}

		const char* release() noexcept {
			ctx = nullptr;
			return std::exchange(string, nullptr);
		}

		void reset(JSContext* ctx_ = nullptr, const char* cstr = nullptr) noexcept {
			if (ctx && string) {
				character.release(ctx, string);
			}
			ctx = ctx_;
			string = cstr;
		}

	private:
		const char* string = nullptr;
		JSContext* ctx = nullptr;
		Character character;
	};

	using JSCString = JSCStringRef<DefaultCharacter>;
	/*convert UTF8 to the default system encoding*/
	using JSCStringA = JSCStringRef<SystemCharacter>;


	class JSAtomRef {
	public:
		JSAtomRef() noexcept = default;

		explicit JSAtomRef(JSContext* ctx, JSAtom atom)
			:ctx_(ctx), atom_(atom) {

		}

		explicit JSAtomRef(JSContext* ctx, JSValue val)
			:ctx_(ctx), atom_(JS_ValueToAtom(ctx, val)) {
		}

		JSAtomRef(const JSAtomRef& other) = delete;

		JSAtomRef(JSAtomRef&& other) noexcept {
			reset(other.ctx_, other.atom_);
			other.release();
		}

		JSAtomRef& operator=(const JSAtomRef& other) = delete;
		JSAtomRef& operator=(JSAtomRef&& other) noexcept {
			reset(other.ctx_, other.atom_);
			other.release();
			return *this;
		}

		~JSAtomRef() {
			if (ctx_ && atom_) {
				JS_FreeAtom(ctx_, atom_);
			}
			release();
		}

		JSAtom get() const {
			return atom_;
		}

		JSAtom operator*() const {
			return get();
		}

		operator bool() const noexcept {
			return atom_ != 0;
		}

		JSAtom release() noexcept {
			ctx_ = nullptr;
			atom_ = 0;
			return std::exchange(atom_, 0);
		}

		void reset(JSContext* ctx = nullptr, JSAtom atom = 0) {
			if (ctx_ && atom_) {
				JS_FreeAtom(ctx_, atom_);
			}
			ctx_ = ctx;
			atom_ = atom;
		}

	private:
		JSContext* ctx_;
		JSAtom atom_;
	};

	class JSPropertyRef;

	class JSValueRef {
	public:
		JSValueRef() noexcept = default;

		explicit JSValueRef(JSContext* ctx, JSValue val)
			: ctx(ctx), ref(val) {
		}

		JSValueRef(const JSValueRef& other) = delete;

		JSValueRef(JSValueRef&& other) noexcept {
			reset(other.ctx, other.ref);
			other.release();
		}

		virtual ~JSValueRef() {
			if (ctx && ref != JS_UNDEFINED) {
				JS_FreeValue(ctx, ref);
			}
			release();
		}

		JSValueRef& operator=(const JSValueRef& other) = delete;

		JSValueRef& operator=(JSValueRef&& other) noexcept {
			reset(other.ctx, other.ref);
			other.release();
			return *this;
		}

		JSValue get() const noexcept {
			return ref;
		}

		JSValue operator*() const noexcept {
			return ref;
		}

		JSValue release() noexcept {
			ctx = nullptr;
			return std::exchange(ref, JS_UNDEFINED);
		}

		void reset(JSContext* ctx_ = nullptr, JSValue val = JS_UNDEFINED) noexcept {
			if (ctx && ref != JS_UNDEFINED) {
				JS_FreeValue(ctx, ref);
			}
			ctx = ctx_;
			ref = val;
		}

		operator int() const {
			return value<int>();
		}

		operator size_t() const {
			return value<size_t>();
		}

		operator double() const {
			return value<double>();
		}

		operator float() const {
			return value<float>();
		}

		operator bool() const {
			return value<bool>();
		}

		operator std::string() const {
			return value<std::string>();
		}

		operator quickjs::JSCString() const {
			return value<quickjs::JSCString>();
		}

		operator quickjs::JSCStringA() const {
			return value<quickjs::JSCStringA>();
		}

		/*Note: When using the class quickjs::JSPropertyRef, it must be defined after quickjs::JSPropertyRef */
		//Note: If is object
		quickjs::JSPropertyRef operator[](std::string key) const;
		//Note: If is object
		quickjs::JSPropertyRef operator[](const char* key) const;
		//Note: If is object
		quickjs::JSPropertyRef operator[](char* key) const;

		//Note: If is array, without verify length
		quickjs::JSPropertyRef operator[](int idx) const;

		//Note: If is array, get property .length
		size_t length() const {
			if (!is<JSType::array>()) {
				throw type_error("array is required");
			}
			int64_t len;
			if (JS_GetPropertyLength(ctx, &len, ref) == -1) {
				throw type_error("fail to get .length");
			}
			return static_cast<size_t>(len);
		}

		template<typename T>
		bool is() const {
			if (std::is_same_v<std::decay_t<T>, bool>) {
				return is<JSType::boolean>();
			} else if (std::is_integral_v<std::decay_t<T>>) {
				return is<JSType::integer>();
			} else if (std::is_floating_point_v<std::decay_t<T>>) {
				return is<JSType::number>();
			} else if (std::is_same_v<std::decay_t<T>, std::string>
					   || std::is_same_v<std::decay_t<T>, const char*>
					   || std::is_same_v<std::decay_t<T>, char*>
					   || std::is_same_v<std::decay_t<T>, quickjs::JSCString>
					   || std::is_same_v<std::decay_t<T>, quickjs::JSCStringA>) {
				return is<JSType::string>();
			}
			return false;
		}

		template<JSType T>
		bool is() const {
			return false;
		}

		template<>
		bool is<JSType::integer>() const {
			return JS_VALUE_GET_TAG(ref) == JS_TAG_INT;
		}

		template<>
		bool is<JSType::boolean>() const {
			return JS_VALUE_GET_TAG(ref) == JS_TAG_BOOL;
		}

		template<>
		bool is<JSType::number>() const {
			return JS_IsNumber(ref);
		}

		template<>
		bool is<JSType::string>() const {
			return JS_VALUE_GET_TAG(ref) == JS_TAG_STRING;
		}

		template<>
		bool is<JSType::object>() const {
			return (JS_VALUE_GET_TAG(ref) == JS_TAG_OBJECT) && !JS_IsArray(ctx, ref);
		}

		template<>
		bool is<JSType::array>() const {
			return JS_IsArray(ctx, ref);
		}

		template<>
		bool is<JSType::function>() const {
			return JS_IsFunction(ctx, ref);
		}

		template<>
		bool is<JSType::symbol>() const {
			return JS_VALUE_GET_TAG(ref) == JS_TAG_SYMBOL;
		}

		template<>
		bool is<JSType::undefined>() const {
			return JS_VALUE_GET_TAG(ref) == JS_TAG_UNDEFINED;
		}

		template<>
		bool is<JSType::uninitialized>() const {
			return JS_VALUE_GET_TAG(ref) == JS_TAG_UNINITIALIZED;
		}

		template<>
		bool is<JSType::null>() const {
			return JS_VALUE_GET_TAG(ref) == JS_TAG_NULL;
		}

		template<>
		bool is<JSType::exception>() const {
			return JS_VALUE_GET_TAG(ref) == JS_TAG_EXCEPTION;
		}

		bool is(JSType type) const {
			switch (type) {
				case quickjs::JSType::object:
					return is<JSType::object>();
				case quickjs::JSType::array:
					return is<JSType::array>();
				case quickjs::JSType::integer:
					return is<JSType::integer>();
				case quickjs::JSType::boolean:
					return is<JSType::boolean>();
				case quickjs::JSType::number:
					return is<JSType::number>();
				case quickjs::JSType::string:
					return is<JSType::string>();
				case quickjs::JSType::function:
					return is<JSType::function>();
				case quickjs::JSType::symbol:
					return is<JSType::symbol>();
				case quickjs::JSType::undefined:
					return is<JSType::undefined>();
				case quickjs::JSType::uninitialized:
					return is<JSType::uninitialized>();
				case quickjs::JSType::null:
					return is<JSType::null>();
				case quickjs::JSType::exception:
					return is<JSType::exception>();
				default:
					break;
			}
			return false;
		}

		template<typename T>
		T value() const {
			if (!is<T>()) {
				throw type_error(quickjs::format_str("%s is required", type_name<T>().c_str()));
			}
			if (std::is_same_v<std::decay_t<T>, bool>) {
				return static_cast<T>(JS_ToBool(ctx, ref));
			} else if (std::is_integral_v<std::decay_t<T>>) {
				int val;
				if (JS_ToInt32(ctx, &val, ref) == -1) {
					throw type_error("type conversion error");
				}
				if (std::is_unsigned_v<T> && val < 0) {
					throw type_error("positive integer is required");
				}
				return static_cast<T>(val);
			} else if (std::is_floating_point_v<std::decay_t<T>>) {
				double val;
				if (JS_ToFloat64(ctx, &val, ref) == -1) {
					throw type_error("type conversion error");
				}
				if (std::is_unsigned_v<T> && val < 0) {
					throw type_error("positive number is required");
				}
				return static_cast<T>(val);
			}
			throw type_error("type mismatch");
		}

		template<>
		const char* value<const char*>() const = delete;
		template<>
		char* value<char*>() const = delete;

		template<>
		std::string value<std::string>() const {
			if (!is<std::string>()) {
				throw type_error("string is required");
			}
			const char* s = JS_ToCString(ctx, ref);
			std::string val(s);
			JS_FreeCString(ctx, s);
			return val;
		}

		template<>
		quickjs::JSCString value<quickjs::JSCString>() const {
			if (!is<JSCString>()) {
				throw type_error("string is required");
			}
			return quickjs::JSCString(ctx, ref);
		}

		template<>
		quickjs::JSCStringA value<quickjs::JSCStringA>() const {
			if (!is<JSCStringA>()) {
				throw type_error("string is required");
			}
			return quickjs::JSCStringA(ctx, ref);
		}

		/**
		 * @brief Note: If the argument is passed, verify the type. Or return the default value.
		 *
		 * @param default_val  The fallback value to return if validation fails or context is null.
		 * @param throw_err    If true, throws quickjs::type_error on type mismatch; if false, returns default_val.
		 */
		template<typename T>
		T value(T default_val, bool throw_err = true) const {
			if (!ctx) {
				return default_val;
			} else if (!is<T>()) {
				if (!throw_err)
					return default_val;

				throw type_error(quickjs::format_str("%s is required", type_name<T>().c_str()));
			}
			return value<T>();
		}

		template<>
		const char* value<const char*>(const char* default_val, bool throw_err) const = delete;
		template<>
		char* value<char*>(char* default_val, bool throw_err) const = delete;

	protected:
		virtual quickjs::type_error type_error(const std::string& msg) const {
			return quickjs::type_error(msg);
		}

	private:
		template<typename T>
		std::string type_name() const noexcept {
			if (std::is_same_v<std::decay_t<T>, bool>) {
				return "boolean";
			} else if (std::is_integral_v<std::decay_t<T>>) {
				return "integer";
			} else if (std::is_floating_point_v<std::decay_t<T>>) {
				return "number";
			} else if (std::is_same_v<std::decay_t<T>, std::string>
					   || std::is_same_v<std::decay_t<T>, const char*>
					   || std::is_same_v<std::decay_t<T>, char*>
					   || std::is_same_v<std::decay_t<T>, quickjs::JSCString>
					   || std::is_same_v<std::decay_t<T>, quickjs::JSCStringA>) {
				return "string";
			}
			return typeid(T).name();
		}

		JSValue ref = JS_UNDEFINED;
		JSContext* ctx = nullptr;
	};

	class JSPropertyRef :public JSValueRef {
	public:
		//using JSValueRef::JSValueRef;

		JSPropertyRef() = delete;

		explicit JSPropertyRef(JSContext* ctx, JSValue val, std::string name) :JSValueRef(ctx, val),
			property_name(name),
			property_exists(ctx != nullptr) {
		}

		explicit JSPropertyRef(std::string name) :JSValueRef(),
			property_name(name),
			property_exists(false) {
		}

		JSPropertyRef(const JSPropertyRef& other) = delete;

		JSPropertyRef(JSPropertyRef&& other) noexcept :JSValueRef(std::move(other)),
			property_name(other.property_name),
			property_exists(other.property_exists) {
			other.property_name.clear();
			other.property_exists = false;
		}

		JSPropertyRef& operator=(const JSPropertyRef& other) = delete;

		JSPropertyRef& operator=(JSPropertyRef&& other) noexcept {
			if (this != &other) {
				property_name = other.property_name;
				property_exists = other.property_exists;
				other.property_name.clear();
				other.property_exists = false;
				JSValueRef::operator=(std::move(other));
			}
			return *this;
		}

		void reset(JSContext* ctx = nullptr, JSValue val = JS_UNDEFINED, std::string name = std::string()) noexcept {
			property_name = name;
			property_exists = ctx != nullptr;
			JSValueRef::reset(ctx, val);
		}

		JSValue release() noexcept {
			property_name.clear();
			property_exists = false;
			return JSValueRef::release();
		}

		virtual ~JSPropertyRef() {
			property_name.clear();
			property_exists = false;
		}

	private:
		bool property_exists;
		std::string property_name;

	protected:
		virtual quickjs::type_error type_error(const std::string& msg) const override {
			std::string msg_ = msg;
			if (!property_exists) {
				msg_ = quickjs::format_str("no property named '%s'", property_name.c_str());
			} else if (!property_name.empty()) {
				msg_ = quickjs::format_str("%s at property named '%s'", msg.c_str(), property_name.c_str());
			}
			return quickjs::type_error(msg_);
		}
	};

	/*
	=============================================[ JSValueRef ]===============================================
	*/
	/* Marking as inline is mandatory, otherwise redefining when linking */
	inline quickjs::JSPropertyRef quickjs::JSValueRef::operator[](std::string key) const {
		return operator[](key.c_str());
	}

	inline quickjs::JSPropertyRef quickjs::JSValueRef::operator[](const char* key) const {
		return operator[](const_cast<char*>(key));
	}

	inline quickjs::JSPropertyRef quickjs::JSValueRef::operator[](char* key) const {
		if (!is<JSType::object>()) {
			throw type_error("object is required");
		}
		JSAtomRef prop(ctx, *JSValueRef(ctx, JS_NewString(ctx, key)));
		if (JS_HasProperty(ctx, ref, *prop)) {
			return quickjs::JSPropertyRef(ctx, JS_GetProperty(ctx, ref, *prop), key);
		}
		return quickjs::JSPropertyRef(key);
	}

	inline quickjs::JSPropertyRef quickjs::JSValueRef::operator[](int idx) const {
		if (!is<JSType::array>()) {
			throw type_error("array is required");
		}
		JSAtomRef prop(ctx, *JSValueRef(ctx, JS_NewInt32(ctx, idx)));
		if (JS_HasProperty(ctx, ref, *prop)) {
			return quickjs::JSPropertyRef(ctx, JS_GetProperty(ctx, ref, *prop), quickjs::format_str("[%d]", idx));
		}
		return quickjs::JSPropertyRef(quickjs::format_str("[%d]", idx));
	}
	/*
	=============================================[ JSValueRef ]===============================================
	*/

	class JSArgumentRef :public JSValueRef {
	public:
		JSArgumentRef() = delete;

		explicit JSArgumentRef(JSContext* ctx, JSValue val, int arg_idx) :JSValueRef(ctx, val),
			argument_idx(arg_idx),
			no_arguments(ctx == nullptr) {
		}

		explicit JSArgumentRef(int arg_idx) :JSValueRef(),
			argument_idx(arg_idx),
			no_arguments(true) {
		}

		JSArgumentRef(const JSArgumentRef& other) = delete;

		JSArgumentRef(JSArgumentRef&& other) noexcept :JSValueRef(std::move(other)),
			argument_idx(other.argument_idx),
			no_arguments(other.no_arguments) {
			other.argument_idx = -1;
			other.no_arguments = true;
		}

		JSArgumentRef& operator=(const JSArgumentRef& other) = delete;

		JSArgumentRef& operator=(JSArgumentRef&& other) noexcept {
			if (this != &other) {
				argument_idx = other.argument_idx;
				no_arguments = other.no_arguments;
				other.argument_idx = -1;
				other.no_arguments = true;
				JSValueRef::operator=(std::move(other));
			}
			return *this;
		}

		void reset(JSContext* ctx = nullptr, JSValue val = JS_UNDEFINED, int arg_idx = -1) noexcept {
			argument_idx = arg_idx;
			no_arguments = ctx == nullptr;
			JSValueRef::reset(ctx, val);
		}

		JSValue release() noexcept {
			argument_idx = -1;
			no_arguments = true;
			return JSValueRef::release();
		}

		virtual ~JSArgumentRef() {
			argument_idx = -1;
			no_arguments = true;
		}

	protected:
		virtual quickjs::type_error type_error(const std::string& msg) const override {
			std::string msg_ = msg;
			if (no_arguments) {
				msg_ = quickjs::format_str("%s at arguments[%d], but no argument present", msg.c_str(), argument_idx);
			} else if (argument_idx >= 0) {
				msg_ = quickjs::format_str("%s at arguments[%d]", msg.c_str(), argument_idx);
			}
			return quickjs::type_error(msg_);
		}

	private:
		int argument_idx = -1;
		bool no_arguments = true;
	};

	/*Note : Return quickjs::JSArgumentRef, Invoke JS_DupValue and save an additional argument index */
	template<size_t Index>
	static inline quickjs::JSArgumentRef dump_argument(JSContext* ctx, int argc, JSValue* argv) {
		if (Index < argc) {
			return quickjs::JSArgumentRef(ctx, JS_DupValue(ctx, argv[Index]), Index);
		}
		return quickjs::JSArgumentRef(Index);
	}

	class JSArguments {
	public:
		explicit JSArguments(JSContext* ctx, int argc, JSValueConst* argv)
			:ctx_(ctx), argc_(argc), argv_(argv), argumentsGroupId(-1) {

		}

		JSArguments(const JSArguments& arguments) = delete;
		JSArguments(JSArguments&& arguments) = delete;

		JSArguments& operator=(const JSArguments& arguments) = delete;
		JSArguments& operator=(JSArguments&& arguments) = delete;

		/* maybe assertion failed: list_empty(&rt->gc_obj_list), when use 'arguments[0][5].value<bool>()' but throw exception */
		quickjs::JSArgumentRef operator[](int idx) {
			if (std::abs(idx) >= size()) {
				return quickjs::JSArgumentRef(idx);
			}
			if (idx >= 0) {
				return quickjs::JSArgumentRef(ctx_, JS_DupValue(ctx_, argv_[idx]), idx);
			} else {
				return quickjs::JSArgumentRef(ctx_, JS_DupValue(ctx_, argv_[argc_ + idx]), argc_ + idx);
			}
		}

		explicit operator bool() const noexcept {
			return argc_ > 0;
		}

		int size() const noexcept {
			return argc_;
		}

		/**
		 * @brief Registers an argument group with a specific GroupId and expected JSType tags.
		 *
		 * This function stores a mapping of GroupId to a vector of JSType tags, which represents
		 * the expected type signature for a specific overload or group of arguments.
		 *
		 * @tparam GroupId       A compile-time constant used as the key to identify this argument group.
		 * @tparam TagTypes      Variadic template parameter pack representing the types of the provided tags.
		 *                       (Constrained by std::enable_if_t to strictly accept only JSType).
		 * @tparam (unnamed)     SFINAE constraint ensuring all TagTypes are exactly JSType.
		 *                       Compilation will fail if any other type is passed.
		 * @param require_tags   A parameter pack of JSType values representing the expected argument types.
		 */
		template<size_t GroupId, typename... TagTypes, typename = std::enable_if_t<std::conjunction<std::is_same<TagTypes, JSType>...>::value>>
		void registerGroup(TagTypes... require_tags) noexcept {
			constexpr size_t require_size = sizeof...(require_tags);
			if (argc_ != require_size) {
				return;
			}
			bool matched = true;
			const JSType tags[] = { static_cast<JSType>(require_tags)... };
			for (size_t i = 0; i < require_size; i++) {
				if (!(*this)[i].is(tags[i])) {
					matched = false;
					break;
				}
			}
			if (matched) {
				argumentsGroupId = GroupId;
			}
		}

		template<size_t GroupId>
		void registerGroup() noexcept {
			if (argc_ == 0) {
				argumentsGroupId = GroupId;
			}
		}

		/**
		 * @brief Determines which predefined argument group matches the current arguments.
		 *
		 * @return int The ID (GroupId) of the matching argument group.
		 * @return -1 If no registered group matches the current argument types.
		 */
		int groupId() const noexcept {
			return argumentsGroupId;
		}

		/**
		 * @brief Validates that the function received a minimum number of arguments.
		 *
		 * @param required_size The required minimum number of arguments.
		 * @throws quickjs::type_error If the actual number of arguments is less than required_size.
		 */
		void requireArgumentSize(size_t required_size) const {
			if (argc_ < required_size) {
				throw quickjs::type_error(quickjs::format_str("%d arguments required, but only %d present", required_size, argc_));
			}
		}

		/**
		 * @brief Checks if the arguments strictly match the expected types from index 0.
		 *
		 * This function verifies that the total number of arguments exactly matches the number
		 * of required types, and that each argument's type strictly corresponds to the provided JSType.
		 *
		 * @tparam TagTypes Variadic template parameters (must all be of type JSType).
		 * @param require_types A pack of JSType values representing the expected types.
		 * @return true if the argument count matches exactly AND all types are correct; false otherwise.
		 */
		template<typename... TagTypes, typename = std::enable_if_t<std::conjunction<std::is_same<TagTypes, JSType>...>::value>>
		bool IsArgsOf(TagTypes... require_types) noexcept {
			constexpr size_t require_size = sizeof...(require_types);
			if (size() != static_cast<int>(require_size)) {
				return false;
			}
			const JSType types[] = { static_cast<JSType>(require_types)... };
			for (size_t i = 0; i < require_size; i++) {
				if (!(*this)[i].is(types[i])) {
					return false;
				}
			}
			return true;
		}

		/**
		 * @brief Checks if the arguments strictly match the expected types starting from a specific index.
		 *
		 * Similar to IsArgsOf, but begins the strict type and count validation at the given offset (s_idx).
		 *
		 * @tparam TagTypes Variadic template parameters (must all be of type JSType).
		 * @param s_idx     The starting index in the arguments list to begin checking.
		 * @param require_types A pack of JSType values representing the expected types.
		 * @return true if the remaining arguments from s_idx exactly match the required types and count.
		 */
		template<typename... TagTypes, typename = std::enable_if_t<std::conjunction<std::is_same<TagTypes, JSType>...>::value>>
		bool IsArgsOf(size_t s_idx, TagTypes... require_types) noexcept {
			constexpr size_t require_size = sizeof...(require_types);
			if (size() - s_idx != static_cast<int>(require_size)) {
				return false;
			}
			const JSType types[] = { static_cast<JSType>(require_types)... };
			for (size_t i = s_idx; i < require_size; i++) {
				if (!(*this)[i].is(types[i])) {
					return false;
				}
			}
			return true;
		}

		/**
		 * @brief Checks if there are *at least* enough arguments matching the expected types from index 0.
		 *
		 * Unlike IsArgsOf (which requires an exact match), this function allows for extra arguments
		 * at the end. It only verifies that the first N arguments match the first N required types.
		 *
		 * @tparam TagTypes Variadic template parameters (must all be of type JSType).
		 * @param require_types A pack of JSType values representing the minimum expected types.
		 * @return true if the argument count is greater than or equal to required, and the first N types match.
		 */
		template<typename... TagTypes, typename = std::enable_if_t<std::conjunction<std::is_same<TagTypes, JSType>...>::value>>
		bool ExpectArgsOf(TagTypes... require_types) noexcept {
			constexpr size_t require_size = sizeof...(require_types);
			if (size() < static_cast<int>(require_size)) {
				return false;
			}
			const JSType types[] = { static_cast<JSType>(require_types)... };
			for (size_t i = 0; i < require_size; i++) {
				if (!(*this)[i].is(types[i])) {
					return false;
				}
			}
			return true;
		}

		/**
		 * @brief Checks if there are *at least* enough arguments matching the expected types starting from a specific index.
		 *
		 * Combines the offset logic of the second IsArgsOf and the relaxed count logic of the first ExpectArgsOf.
		 * Useful for checking optional parameters or skipping a 'this' pointer.
		 *
		 * @tparam TagTypes Variadic template parameters (must all be of type JSType).
		 * @param s_idx     The starting index in the arguments list to begin checking.
		 * @param require_types A pack of JSType values representing the minimum expected types from s_idx onwards.
		 * @return true if the remaining arguments from s_idx are enough and their types match.
		 */
		template<typename... TagTypes, typename = std::enable_if_t<std::conjunction<std::is_same<TagTypes, JSType>...>::value>>
		bool ExpectArgsOf(size_t s_idx, TagTypes... require_types) noexcept {
			constexpr size_t require_size = sizeof...(require_types);
			if (size() - s_idx < static_cast<int>(require_size)) {
				return false;
			}
			const JSType types[] = { static_cast<JSType>(require_types)... };
			for (size_t i = s_idx; i < require_size; i++) {
				if (!(*this)[i].is(types[i])) {
					return false;
				}
			}
			return true;
		}

		~JSArguments() = default;

	private:
		int argc_;
		JSValueConst* argv_;
		JSContext* ctx_;
		int argumentsGroupId;
	};

	/**
	* @brief Checks if the arguments strictly match the expected count and type tags.
	*
	* @note This function performs a strict low-level tag comparison. It requires the
	*       number of arguments to be exactly the same, and does not handle JavaScript
	*       implicit type coercions (e.g., Int to Double).
	*
	* @param argc          The actual number of arguments passed from the JS context.
	* @param argv          A pointer to the array of JSValue arguments.
	* @param require_tags A variadic list of expected type tags to match against.
	* @return true         If argc exactly equals the number of expected tags, and all tags match.
	* @return false        If the argument count differs, or any tag mismatch occurs.
	*/
	template<typename... TagTypes>
	static inline bool JS_IsArgsOf(int argc, JSValue* argv, TagTypes... require_tags) {
		constexpr size_t require_size = sizeof...(require_tags);
		if (argc != static_cast<int>(require_size)) {
			return false;
		}
		const int tags[] = { static_cast<int>(require_tags)... };
		for (size_t i = 0; i < require_size; i++) {
			if (JS_VALUE_GET_TAG(argv[i]) != tags[i]) {
				return false;
			}
		}
		return true;
	}

	/**
	* @brief Checks if the type tags of the first N JSValue arguments exactly match the expected tags.
	*
	* @note This function performs a strict low-level tag comparison. It does not handle
	*       JavaScript implicit type coercions (e.g., Int to Double).
	*
	* @param argc          The actual number of arguments passed from the JS context.
	* @param argv          A pointer to the array of JSValue arguments.
	* @param expected_tags A variadic list of expected type tags to match against.
	* @return true         If argc is sufficient and the first N argument tags match exactly.
	* @return false        If there are not enough arguments, or any tag mismatch occurs.
	*/
	template<typename... TagTypes>
	static inline bool JS_ExceptedArgsOf(int argc, JSValue* argv, TagTypes... expected_tags) {
		constexpr size_t expected_size = sizeof...(expected_tags);
		if (argc < static_cast<int>(expected_size)) {
			return false;
		}
		const int tags[] = { static_cast<int>(expected_tags)... };
		for (size_t i = 0; i < expected_size; i++) {
			if (JS_VALUE_GET_TAG(argv[i]) != tags[i]) {
				return false;
			}
		}
		return true;
	}

	using array_iterator = std::function<bool(JSValueRef, const size_t)>;
	using object_iterator = std::function<bool(JSValueRef, JSValueRef)>;

	/**
	 * @brief Iterates over a QuickJS array value and applies a callback to each element.
	 *
	 * This static utility function checks if the provided JSValue is an array, retrieves its length,
	 * and invokes the provided iterator function for each element. The iteration can be terminated
	 * early if the iterator callback returns false.
	 *
	 * @param ctx   The QuickJS context.
	 * @param val   The JSValue to iterate over (must be an array).
	 * @param iter  The callback function (array_iterator) to execute for each element.
	 *              It takes a JSValueRef (the element) and a size_t (the index).
	 *              Return false to break the loop early, true to continue.
	 *
	 * @return The total length of the array as an int.
	 *
	 * @throws quickjs::type_error If the provided 'val' is not a valid array.
	 */
	static inline int for_each(JSContext* ctx, JSValue val, quickjs::array_iterator iter) {
		if (!JS_IsArray(ctx, val)) {
			throw quickjs::type_error("not an array");
		}
		int64_t len = 0;
		JS_GetPropertyLength(ctx, &len, val);
		for (size_t i = 0; i < len; i++) {
			if (!iter(JSValueRef(ctx, JS_GetPropertyUint32(ctx, val, i)), i)) {
				break;
			}
		}
		return static_cast<int>(len);
	}

	/**
	 * @brief Iterates over the enumerable string properties of a QuickJS object.
	 *
	 * This static utility function retrieves all own enumerable properties of the given object,
	 * converts their keys and values into JSValueRef wrappers, and passes them to the provided
	 * callback. The iteration can be terminated early if the callback returns false.
	 *
	 * @param ctx   The QuickJS context.
	 * @param val   The JSValue to iterate over (must be an object).
	 * @param iter  The callback function (object_iterator) to execute for each property.
	 *              It takes two JSValueRef arguments: the property key (as a string) and the property value.
	 *              Return false to break the loop early, true to continue.
	 *
	 * @return The number of properties iterated over as an int.
	 *
	 * @throws quickjs::type_error If the provided 'val' is not an object, or if
	 *         retrieving the property names fails.
	 */
	static inline int enum_object(JSContext* ctx, JSValue val, quickjs::object_iterator iter) {
		if (!JS_IsObject(val)) {
			throw quickjs::type_error("not an object");
		}
		uint32_t len = 0;
		JSPropertyEnum* tab;
		if (JS_GetOwnPropertyNames(ctx, &tab, &len, val,
								   JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) < 0) {
			throw quickjs::type_error("fail to get own property names");
		}
		for (int i = 0; i < len; i++) {
			if (!iter(JSValueRef(ctx, JS_AtomToString(ctx, tab[i].atom)), JSValueRef(ctx, JS_GetProperty(ctx, val, tab[i].atom)))) {
				break;
			}
		}
		JS_FreePropertyEnum(ctx, tab, len);
		return static_cast<int>(len);
	}
}
#endif //__cplusplus

#endif // QUICKJSPP_H