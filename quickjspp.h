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

/*Note : check when argc >= idx */
#define JS_ExpectArgTypeThrow(ctx, argc, argv, tag, idx, fmt, ...)\
	if (argc >= (idx) && JS_VALUE_GET_TAG(argv[(idx-1)]) != tag) {\
		return JS_ThrowTypeError(ctx, fmt, __VA_ARGS__);\
	}\

/*Note : when argc < idx or check failed return JS_ThrowTypeError*/
#define JS_RequireArgTypeThrow(ctx, argc, argv, tag, idx, fmt, ...)\
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
	class JSValueRef {
	public:
		JSValueRef() noexcept = default;

		explicit JSValueRef(JSRuntime* rt, JSValue val)
			: mRt(rt), mRef(val) {
		}

		explicit JSValueRef(JSContext* ctx, JSValue val)
			: mRt(JS_GetRuntime(ctx)), mRef(val) {
		}

		JSValueRef(const JSValueRef& valueRef) = delete;

		JSValueRef(JSValueRef&& valueRef) noexcept
			: mRef(valueRef.mRef), mRt(valueRef.mRt) {
			valueRef.mRt = nullptr;
			valueRef.mRef = JS_UNDEFINED;
		}

		~JSValueRef() {
			if (mRt && mRef != JS_UNDEFINED) {
				JS_FreeValueRT(mRt, mRef);
			}
			mRt = nullptr;
			mRef = JS_UNDEFINED;
		}

		JSValueRef& operator=(const JSValueRef& valueRef) = delete;

		JSValueRef& operator=(JSValueRef&& valueRef) noexcept {
			reset(valueRef.mRt, valueRef.mRef);
			valueRef.mRt = nullptr;
			valueRef.mRef = JS_UNDEFINED;
			return *this;
		}

		JSValue get() const noexcept {
			return mRef;
		}

		JSValue operator*() const noexcept {
			return mRef;
		}

		explicit operator bool() const noexcept {
			return mRef != JS_UNDEFINED;;
		}

		JSValue release() noexcept {
			JSValue tmp = mRef;
			mRt = nullptr;
			mRef = JS_UNDEFINED;
			return tmp;
		}

		void reset(JSRuntime* rt = nullptr, JSValue val = JS_UNDEFINED) noexcept {
			if (mRt && mRef != JS_UNDEFINED) {
				JS_FreeValueRT(mRt, mRef);
			}
			mRt = rt;
			mRef = val;
		}

	private:
		JSValue mRef = JS_UNDEFINED;
		JSRuntime* mRt = nullptr;
	};

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
			:mCtx(ctx), mCstr(character.unwrap(ctx, val)) {
		}

		explicit JSCStringRef(JSContext* ctx, JSAtom atom)
			:mCtx(ctx), mCstr(character.wrap(ctx, *quickjs::JSValueRef(ctx, JS_AtomToValue(ctx, atom)))) {
		}

		JSCStringRef(const JSCStringRef& JsCstr) = delete;

		JSCStringRef(JSCStringRef&& JsCstr) noexcept
			: mCstr(JsCstr.mCstr), mCtx(JsCstr.mCtx) {
			JsCstr.mCtx = nullptr;
			JsCstr.mCstr = nullptr;
		}

		~JSCStringRef() {
			if (mCtx && mCstr) {
				character.release(mCtx, mCstr);
			}
			mCtx = nullptr;
			mCstr = nullptr;
		}

		JSCStringRef& operator=(const JSCStringRef& JsCstr) = delete;

		JSCStringRef& operator=(JSCStringRef&& JsCstr) noexcept {
			reset(JsCstr.mCtx, JsCstr.mCstr);
			JsCstr.mCtx = nullptr;
			JsCstr.mCstr = nullptr;
			return *this;
		}

		const char* get() const noexcept {
			return mCstr;
		}

		const char* operator*() const noexcept {
			return mCstr;
		}

		explicit operator bool() const noexcept {
			return mCstr != nullptr;
		}

		const char* release() noexcept {
			const char* cstr = mCstr;
			mCtx = nullptr;
			mCstr = nullptr;
			return cstr;
		}

		void reset(JSContext* ctx = nullptr, const char* cstr = nullptr) noexcept {
			if (mCtx && mCstr) {
				character.release(mCtx, mCstr);
			}
			mCtx = ctx;
			mCstr = cstr;
		}

	private:
		const char* mCstr = nullptr;
		JSContext* mCtx = nullptr;
		Character character;
	};

	using JSCString = JSCStringRef<DefaultCharacter>;
	/*convert UTF8 to the default system encoding*/
	using JSCStringA = JSCStringRef<SystemCharacter>;

	/*Note : Match the TAG of parameters */
	template<typename... Args>
	static inline bool JS_IsArgsOf(int argc, JSValue* argv, Args... args) {
		constexpr size_t argsc = sizeof...(args);
		if (argc != argsc) {
			return false;
		}
		std::common_type_t<Args...> argsv[] = { args... };
		for (size_t i = 0; i < argsc; i++) {
			if (JS_VALUE_GET_TAG(argv[i]) != argsv[i]) {
				return false;
			}
		}
		return true;
	}
	
	typedef std::function<void(JSValue, JSValue, const size_t)> Iterator;
	/*Note : Traverse JSArray or JSObject, Callback(key, value, index), return property size or < 0 */
	static inline size_t for_each(JSContext* ctx, JSValue v, quickjs::Iterator iter) {
		if (!JS_IsArray(ctx, v) && !JS_IsObject(v)) {
			return -1;
		}
		if (JS_IsArray(ctx, v)) {
			int64_t len = 0;
			JS_GetPropertyLength(ctx, &len, v);
			for (size_t i = 0; i < len; i++) {
				JSValueRef key(ctx, JS_NewInt32(ctx, i));
				JSValueRef value(ctx, JS_GetPropertyUint32(ctx, v, i));
				iter(*key, *value, i);
			}
			return len;
		} else {
			uint32_t len = 0;
			JSPropertyEnum* tab;
			if (JS_GetOwnPropertyNames(ctx, &tab, &len, v,
									   JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) < 0) {
				return -2;
			}
			for (int i = 0; i < len; i++) {
				JSValueRef key(ctx, JS_AtomToString(ctx, tab[i].atom));
				JSValueRef value(ctx, JS_GetProperty(ctx, v, tab[i].atom));
				iter(*key, *value, i);
			}
			JS_FreePropertyEnum(ctx, tab, len);
			return len;
		}
	}
}
#endif //__cplusplus

#endif // QUICKJSPP_H