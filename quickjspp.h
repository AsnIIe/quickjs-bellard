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
#define JS_CHECK_TAG(val, tag) JS_VALUE_GET_TAG(val) == tag

/*Note : check when argc >= idx */
#define JS_CHECK_PARAMETER_TAG(ctx, argc, argv, tag, idx, fmt, ...)\
	if (argc >= (idx) && JS_VALUE_GET_TAG(argv[(idx-1)]) != tag) {\
		return JS_ThrowTypeError(ctx, fmt, __VA_ARGS__);\
	}\

/*Note : when argc < idx or check failed return JS_ThrowTypeError*/
#define JS_STRICT_PARAMETER_TAG(ctx, argc, argv, tag, idx, fmt, ...)\
	if (argc < (idx) || JS_VALUE_GET_TAG(argv[(idx-1)]) != tag) {\
		return JS_ThrowTypeError(ctx, fmt, __VA_ARGS__);\
	}\

/*Note : The result string allocates memory from the JSRuntime and needs to be released*/
static inline char* JS_DupCStringRT(JSRuntime* rt, const char* str) {
	char* ret = str ? JS_MalloczRT(rt, char, strlen(str) + 1) : NULL;
	return ret ? strcpy(ret, str) : NULL;
}

/*Note : Convert the str of default system encoding to UTF8 and return JS_NewString*/
static inline JSValue JS_NewStringA(JSContext* ctx, const char* str) {
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

/*Note : Invoke JS_ToCString and convert UTF8 str to default system encoding */
static inline const char* JS_ToCStringA(JSContext* ctx, JSValue val) {
#if defined(_WIN32)
	const char* content = JS_ToCString(ctx, val);
	char* acp_str = NULL;
	wchar_t* wide_buffer = NULL;
	int wide_len = 0;
	int utf8_len = 0;

	wide_len = MultiByteToWideChar(CP_ACP, 0, content, -1, NULL, 0);
	if (wide_len <= 0) {
		return NULL;
	}
	wide_buffer = (wchar_t*)malloc(sizeof(wchar_t) * wide_len);
	if (!wide_buffer) {
		return NULL;
	}
	if (MultiByteToWideChar(CP_ACP, 0, content, -1, wide_buffer, wide_len) == 0) {
		free(wide_buffer);
		return NULL;
	}
	JS_FreeCString(ctx, content);

	utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide_buffer, -1, NULL, 0, NULL, NULL);
	if (utf8_len <= 0) {
		free(wide_buffer);
		return NULL;
	}
	acp_str = JS_Mallocz(ctx, char, utf8_len);
	if (!acp_str) {
		free(wide_buffer);
		return NULL;
	}
	if (WideCharToMultiByte(CP_UTF8, 0, wide_buffer, -1, acp_str, utf8_len, NULL, NULL) == 0) {
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

namespace quickjs {
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
	class JSCString {
	public:
		JSCString() noexcept = default;

		explicit JSCString(JSContext* ctx, JSValue val)
			:mCtx(ctx), mCstr(character.unwrap(ctx, val)) {
		}

		JSCString(const JSCString& JsCstr) = delete;

		JSCString(JSCString&& JsCstr) noexcept
			: mCstr(JsCstr.mCstr), mCtx(JsCstr.mCtx) {
			JsCstr.mCtx = nullptr;
			JsCstr.mCstr = nullptr;
		}

		~JSCString() {
			if (mCtx && mCstr) {
				character.release(mCtx, mCstr);
			}
			mCtx = nullptr;
			mCstr = nullptr;
		}

		JSCString& operator=(const JSCString& JsCstr) = delete;

		JSCString& operator=(JSCString&& JsCstr) noexcept {
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

	class JSArray {
	public:
		class JSValueProxy {
		private:
			JSContext* ctx = nullptr;
			JSValue array = JS_UNDEFINED;
			size_t index = 0;

		public:
			JSValueProxy(JSContext* ctx, JSValue array, size_t index)
				: ctx(ctx), array(JS_DupValue(ctx, array)), index(index) {
			}
			~JSValueProxy() {
				if (ctx) {
					JS_FreeValue(ctx, array);
				}
			}
			operator JSValue() const {
				return JS_GetPropertyUint32(ctx, array, index);
			}
			operator JSValueRef() const {
				return JSValueRef(ctx, JS_GetPropertyUint32(ctx, array, index));
			}
			JSValueProxy& operator=(JSValue value) {
				JS_SetPropertyUint32(ctx, array, index, value);
				return *this;
			}
			JSValueProxy& operator=(const JSValueProxy& other) {
				JSValue value = JS_GetPropertyUint32(other.ctx, other.array, other.index);
				JS_SetPropertyUint32(ctx, array, index, value);
				return *this;
			}
		};

		class Iterator {

		public:
			Iterator() noexcept = delete;

			explicit Iterator(JSContext* ctx)
				:Iterator(ctx, JS_UNDEFINED) {
			}

			explicit Iterator(JSContext* ctx, JSValue arr)
				:ctx(ctx), array(arr) {
			}

			Iterator(Iterator&& iter) noexcept {
				ctx = iter.ctx;
				array = iter.array;
				index = iter.index;

				iter.ctx = nullptr;
				iter.array = JS_UNDEFINED;
				iter.index = 0;
			}

			Iterator(const Iterator& iter) noexcept {
				ctx = iter.ctx;
				array = iter.array;
				index = iter.index;
			}

			Iterator& operator=(const Iterator&) = default;
			Iterator& operator=(Iterator&&) = default;

			JSValue operator*() {
				if (!JS_IsUndefined(array)) {
					return JS_GetPropertyUint32(ctx, array, index);
				}
				return JS_UNDEFINED;
			}

			JSArray::Iterator& operator++() {
				++index;
				return *this;
			}

			JSArray::Iterator& operator++(int) {
				JSArray::Iterator temp = (*this);
				++(*this);
				return temp;
			}

			JSArray::Iterator& operator--() {
				--index;
				return *this;
			}

			JSArray::Iterator& operator--(int) {
				JSArray::Iterator temp = (*this);
				--(*this);
				return temp;
			}

			JSArray::Iterator& operator+=(int r) {
				index += r;
				return *this;
			}

			JSArray::Iterator& operator-=(int r) {
				index -= r;
				return *this;
			}

			bool operator==(const JSArray::Iterator& iter) {
				return index == iter.index;
			}

			bool operator!=(const JSArray::Iterator& iter) {
				return index != iter.index;
			}

			bool operator>(const JSArray::Iterator& iter) {
				return index > iter.index;
			}

			bool operator<(const JSArray::Iterator& iter) {
				return index < iter.index;
			}

			bool operator>=(const JSArray::Iterator& iter) {
				return index >= iter.index;
			}

			bool operator<=(const JSArray::Iterator& iter) {
				return index <= iter.index;
			}

			~Iterator() {

			}

		private:
			JSContext* ctx = nullptr;
			JSValue array = JS_UNDEFINED;
			int32_t index = 0;
		};

	public:
		JSArray() noexcept = delete;

		explicit JSArray(JSContext* ctx)
			:JSArray(ctx, JS_NewArray(ctx)) {
		}

		explicit JSArray(JSContext* ctx, JSValue arr)
			:ctx(ctx), array(arr), iter_begin(ctx), iter_end(ctx) {
			if (!JS_IsArray(ctx, array) || JS_GetPropertyLength(ctx, &capacity, array) == -1) {
				throw std::exception("JSArray : not an JSArray");
			} else {
				iter_begin = JSArray::Iterator(ctx, array);

				iter_end = JSArray::Iterator(ctx, array);
				iter_end += capacity;
			}
		}

		JSArray(const JSArray& arr) noexcept
			:iter_begin(arr.ctx), iter_end(arr.ctx) {
			ctx = arr.ctx;
			array = JS_DupValue(ctx, arr.array);
			capacity = arr.capacity;
			iter_begin = arr.iter_begin;
			iter_end = arr.iter_end;
		}

		JSArray(JSArray&& arr) noexcept
			:iter_begin(arr.ctx), iter_end(arr.ctx) {
			ctx = arr.ctx;
			array = arr.array;
			capacity = arr.capacity;
			iter_begin = arr.iter_begin;
			iter_end = arr.iter_end;

			arr.ctx = nullptr;
			arr.array = JS_UNDEFINED;
			arr.capacity = 0;
			arr.iter_begin = JSArray::Iterator(ctx);
			arr.iter_end = JSArray::Iterator(ctx);
		}

		JSArray& operator=(const JSArray&) = default;
		JSArray& operator=(JSArray&&) = default;

		~JSArray() {
			if (!JS_IsUndefined(array)) {
				JS_FreeValue(ctx, array);
			}
		}

		JSValue release() noexcept {
			JSValue arr = array;
			ctx = nullptr;
			array = JS_UNDEFINED;
			capacity = 0;
			return arr;
		}

		JSArray::Iterator begin() {
			return iter_begin;
		}

		JSArray::Iterator end() {
			return iter_end;
		}

		JSArray::Iterator rbegin() {
			return iter_end;
		}

		JSArray::Iterator rend() {
			return iter_begin;
		}

		size_t size() {
			return capacity;
		}

		bool empty() {
			return iter_begin == iter_end;
		}

		void push_back(JSValue val) {
			if (ctx && JS_SetPropertyUint32(ctx, array, capacity, val) != -1) {
				capacity++;
				iter_end++;
			}
		}

		explicit operator bool() const noexcept {
			return JS_IsArray(ctx, array);
		}

		JSValue operator*() const noexcept {
			return array;
		}

		JSValueProxy operator[](size_t idx) {
			if (idx >= capacity || idx < 0) {
				throw std::exception("JSArray : out of bounds exception");
			}
			return JSValueProxy(ctx, array, idx);
		}

	private:
		JSContext* ctx = nullptr;
		JSValue array = JS_UNDEFINED;
		int64_t capacity = 0;
		JSArray::Iterator iter_begin;
		JSArray::Iterator iter_end;
	};
}
#endif //__cplusplus

#endif // QUICKJSPP_H