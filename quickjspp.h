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
#include <array>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <deque>
#include <forward_list>
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
		explicit type_error(const std::string& msg) : msg_(msg) {}

		const char* what() const noexcept override {
			return msg_.c_str();
		}

	private:
		std::string msg_;
	};

	template<typename Type>
	class JSMemRef;
	class JSValueRef;
	template<typename Character>
	class JSCStringRef;
	class JSAtomRef;
	class JSPropertyRef;
	class JSArgumentRef;
	class JSArguments;

	std::string to_string(const JSValueRef& ref);
	template<typename T>
	quickjs::JSValueRef toJSValue(JSContext* ctx, const T& val);

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

	using JSCString = JSCStringRef<DefaultCharacter>;
	/*convert UTF8 to the default system encoding*/
	using JSCStringA = JSCStringRef<SystemCharacter>;

	namespace type_traits {
		template <typename T>
		struct is_std_vector : std::false_type {};

		template <typename T, typename Alloc>
		struct is_std_vector<std::vector<T, Alloc>> : std::true_type {
			using value_type = T;
			using allocator_type = Alloc;
		};

		template <typename T>
		struct is_std_map : std::false_type {};

		template <typename Key, typename T, typename Compare, typename Alloc>
		struct is_std_map<std::map<Key, T, Compare, Alloc>> : std::true_type {
			using key_type = Key;
			using mapped_type = T;
			using value_type = std::pair<const Key, T>;
			using key_compare = Compare;
			using allocator_type = Alloc;
		};

		template<typename K, typename V, typename H, typename E, typename A>
		struct is_std_map<std::unordered_map<K, V, H, E, A>> : std::true_type {
			using key_type = K;
			using mapped_type = V;
			using value_type = std::pair<const K, V>;
			using hasher = H;
			using key_equal = E;
			using allocator_type = A;
		};

		template <typename T>
		struct is_std_set : std::false_type {};

		template <typename Key, typename Compare, typename Alloc>
		struct is_std_set<std::set<Key, Compare, Alloc>> : std::true_type {
			using key_type = Key;
			using value_type = Key;
			using key_compare = Compare;
			using allocator_type = Alloc;
		};

		template <typename Key, typename Hash, typename Eq, typename Alloc>
		struct is_std_set<std::unordered_set<Key, Hash, Eq, Alloc>> : std::true_type {
			using key_type = Key;
			using value_type = Key;
			using hasher = Hash;
			using key_equal = Eq;
			using allocator_type = Alloc;
		};

		template <typename T>
		struct is_std_deque : std::false_type {};

		template <typename T, typename Alloc>
		struct is_std_deque<std::deque<T, Alloc>> : std::true_type {
			using value_type = T;
			using allocator_type = Alloc;
		};

		template <typename T>
		struct is_std_list : std::false_type {};

		template <typename T, typename Alloc>
		struct is_std_list<std::list<T, Alloc>> : std::true_type {
			using value_type = T;
			using allocator_type = Alloc;
		};

		template <typename T>
		struct is_std_forward_list : std::false_type {};

		template <typename T, typename Alloc>
		struct is_std_forward_list<std::forward_list<T, Alloc>> : std::true_type {
			using value_type = T;
			using allocator_type = Alloc;
		};

		template <typename T>
		struct is_std_array : std::false_type {};

		template <typename T, std::size_t N>
		struct is_std_array<std::array<T, N>> : std::true_type {
			using value_type = T;
			static constexpr std::size_t size = N;
		};

		template <typename T>
		struct is_std_tuple : std::false_type {};

		template <typename... Args>
		struct is_std_tuple<std::tuple<Args...>> : std::true_type {
			using types = std::tuple<Args...>;
			static constexpr std::size_t size = sizeof...(Args);
		};

		template <typename T1, typename T2>
		struct is_std_tuple<std::pair<T1, T2>> : std::true_type {
			using types = std::tuple<T1, T2>;
			static constexpr std::size_t size = 2;
		};

		template <typename T>
		struct is_std_function : std::false_type {};

		template <typename Ret, typename... Args>
		struct is_std_function<std::function<Ret(Args...)>> : std::true_type {
			using return_type = Ret;
			using arg_types = std::tuple<Args...>;
			static constexpr std::size_t size = sizeof...(Args);
		};
		/* ===============================[ template_types ]=================================== */
		template <typename T>
		struct template_types {
			static constexpr bool valid = false;
		};

		template <template <typename...> class Tmpl, typename... Args>
		struct template_types<Tmpl<Args...>> {
			static constexpr bool valid = true;
			using type = std::tuple<Args...>;
		};

		template <typename T1, typename T2>
		struct template_types<std::pair<T1, T2>> {
			static constexpr bool valid = true;
			using type = std::tuple<T1, T2>;
		};

		template <typename T, std::size_t N>
		struct template_types<std::array<T, N>> {
			static constexpr bool valid = true;
			using type = std::tuple<T>;
		};

		template <typename Ret, typename... Args>
		struct template_types<std::function<Ret(Args...)>> {
			static constexpr bool valid = true;
			using type = std::tuple<Args...>;
		};

		template <typename T>
		using template_types_t = typename quickjs::type_traits::template_types<T>::type;

		template <typename T, std::size_t N,
			typename = typename std::enable_if<template_types<T>::valid>::type>
		using template_type_t = typename std::tuple_element<N, template_types_t<T>>::type;

		namespace function {
			// Forward declaration
			template <typename F, typename = void>
			struct function_traits {};

			// 1) Function pointer
			template <typename Ret, typename... Args>
			struct function_traits<Ret(*)(Args...), void> {
				using signature = Ret(Args...);
			};

			// 2) Pointer to member function (const)
			template <typename C, typename Ret, typename... Args>
			struct function_traits<Ret(C::*)(Args...) const, void> {
				using signature = Ret(Args...);
			};

			// 3) Pointer to member function (non-const)
			template <typename C, typename Ret, typename... Args>
			struct function_traits<Ret(C::*)(Args...), void> {
				using signature = Ret(Args...);
			};

			// 4) Functor / lambda: matches only when F has an operator()
			template <typename F>
			struct function_traits<F, std::void_t<decltype(&F::operator())>>
				: function_traits<decltype(&F::operator())> {};

			// 5) std::function
			template <typename Ret, typename... Args>
			struct function_traits<std::function<Ret(Args...)>, void> {
				using signature = Ret(Args...);
			};

			template <typename F, typename = void>
			struct has_signature : std::false_type {};

			template <typename F>
			struct has_signature<F, std::void_t<typename function_traits<std::decay_t<F>>::signature>>
				: std::true_type {};

			template <typename F>
			using function_t = std::function<typename function_traits<std::decay_t<F>>::signature>;
		}

		namespace types {
			template <typename Container>
			using element_t = template_type_t<Container, 0>;

			template <typename Map>
			using key_t = template_type_t<Map, 0>;

			template <typename Map>
			using mapped_t = template_type_t<Map, 1>;

			template <typename Container>
			using allocator_t = template_type_t<Container,
				std::tuple_size<template_types_t<Container>>::value - 1>;
		}

		namespace container {
			template <typename T>
			struct name {
				static constexpr const char* value = "container";
			};

			template <typename T, typename A>
			struct name<std::vector<T, A>> {
				static constexpr const char* value = "vector";
			};

			template <typename T, std::size_t N>
			struct name<std::array<T, N>> {
				static constexpr const char* value = "array";
			};

			template <typename T, typename A>
			struct name<std::deque<T, A>> {
				static constexpr const char* value = "deque";
			};

			template <typename T, typename A>
			struct name<std::list<T, A>> {
				static constexpr const char* value = "list";
			};

			template <typename T, typename A>
			struct name<std::forward_list<T, A>> {
				static constexpr const char* value = "forward_list";
			};

			template <typename K, typename C, typename A>
			struct name<std::set<K, C, A>> {
				static constexpr const char* value = "set";
			};

			template <typename K, typename H, typename E, typename A>
			struct name<std::unordered_set<K, H, E, A>> {
				static constexpr const char* value = "unordered_set";
			};

			template <typename K, typename V, typename C, typename A>
			struct name<std::map<K, V, C, A>> {
				static constexpr const char* value = "map";
			};

			template <typename K, typename V, typename H, typename E, typename A>
			struct name<std::unordered_map<K, V, H, E, A>> {
				static constexpr const char* value = "unordered_map";
			};

			template <typename... Args>
			struct name<std::tuple<Args...>> {
				static constexpr const char* value = "tuple";
			};

			template <typename T1, typename T2>
			struct name<std::pair<T1, T2>> {
				static constexpr const char* value = "pair";
			};
		}

		namespace detail {
			namespace func {
				template <typename...>
				struct JSFnContext;

				template <typename Ret, typename... Args>
				struct JSFnContext<std::function<Ret(Args...)>> {
					JSContext* ctx;
					std::function<Ret(Args...)> caller;
				};

				template <typename Fn, std::size_t... Is>
				JSValue js_fn_call_impl(std::false_type, std::index_sequence<Is...>, JSFnContext<Fn>* fnctx, const JSArguments& arguments) {
					using traits = type_traits::is_std_function<Fn>;
					using R = typename traits::return_type;
					try {
						R ret = fnctx->caller(arguments[Is].template to<std::tuple_element_t<Is, typename traits::arg_types>>()...);
						return quickjs::toJSValue<R>(fnctx->ctx, ret).release();
					} catch (quickjs::type_error& err) {
						throw quickjs::type_error(err.what());
					} catch (const std::exception& err) {
						return JS_ThrowReferenceError(fnctx->ctx, err.what());
					}
				}

				template <typename Fn, std::size_t... Is>
				JSValue js_fn_call_impl(std::true_type, std::index_sequence<Is...>, JSFnContext<Fn>* fnctx, const JSArguments& arguments) {
					using traits = type_traits::is_std_function<Fn>;
					using R = typename traits::return_type;
					try {
						fnctx->caller(arguments[Is].template to<std::tuple_element_t<Is, typename traits::arg_types>>()...);
						return JS_UNDEFINED;
					} catch (quickjs::type_error& err) {
						throw quickjs::type_error(err.what());
					} catch (const std::exception& err) {
						return JS_ThrowReferenceError(fnctx->ctx, err.what());
					}
				}

				template <typename Fn>
				std::enable_if_t<type_traits::is_std_function<Fn>::value, JSValue>
					js_fn_call(JSContext* ctx, JSValueConst this_val,
							   int argc, JSValueConst* argv,
							   int magic, void* data) {
					using traits = type_traits::is_std_function<Fn>;
					using FnContext = type_traits::detail::func::JSFnContext<Fn>;
					using is_void = std::is_void<std::decay_t<typename traits::return_type>>;
					FnContext* fnctx = (FnContext*)data;
					JSArguments arguments(ctx, argc, argv);

					constexpr size_t N = traits::size;
					try {
						arguments.requireArgumentSize(N);
						return js_fn_call_impl<Fn>(is_void{}, std::make_index_sequence<N>{}, fnctx, arguments);
					} catch (const quickjs::type_error& err) {
						return JS_ThrowTypeError(fnctx->ctx, err.what());
					}
				}

				template <typename T>
				static void js_fn_finalizer(void* data) {
					delete (JSFnContext<T>*)data;
				}
			}

			template<typename T>
			std::enable_if_t<std::is_same_v<std::decay_t<T>, JSValueRef>
				|| std::is_base_of_v<JSValueRef, std::decay_t<T>>, quickjs::JSValueRef>
				new_value_impl(JSContext* ctx, const T& val) {
				return JSValueRef(ctx, JS_DupValue(ctx, val.get()));
			}

			template<typename T>
			std::enable_if_t<std::is_same_v<std::decay_t<T>, JSValue>, quickjs::JSValueRef>
				new_value_impl(JSContext* ctx, const T& val) {
				return JSValueRef(ctx, JS_DupValue(ctx, val));
			}

			template<typename T>
			std::enable_if_t<std::is_same_v<std::decay_t<T>, bool>, quickjs::JSValueRef>
				new_value_impl(JSContext* ctx, const T& val) {
				return JSValueRef(ctx, JS_NewBool(ctx, val));
			}

			template<typename T>
			std::enable_if_t<std::is_integral_v<std::decay_t<T>>
				&& !std::is_same_v<std::decay_t<T>, JSValue>
				&& !std::is_same_v<std::decay_t<T>, bool>, quickjs::JSValueRef>
				new_value_impl(JSContext* ctx, const T& val) {
				return JSValueRef(ctx, JS_NewInt32(ctx, val));
			}

			template<typename T>
			std::enable_if_t<std::is_floating_point_v<std::decay_t<T>>, quickjs::JSValueRef>
				new_value_impl(JSContext* ctx, const T& val) {
				return JSValueRef(ctx, JS_NewFloat64(ctx, val));
			}

			template<typename T>
			std::enable_if_t<std::is_same_v<std::decay_t<T>, std::string>, quickjs::JSValueRef>
				new_value_impl(JSContext* ctx, const T& val) {
				return JSValueRef(ctx, JS_NewStringA(ctx, val.c_str()));
			}

			template<typename T>
			std::enable_if_t<std::is_same_v<std::decay_t<T>, const char*>
				|| std::is_same_v<std::decay_t<T>, char*>, quickjs::JSValueRef>
				new_value_impl(JSContext* ctx, const T& val) {
				return JSValueRef(ctx, JS_NewStringA(ctx, val));
			}

			template<typename T>
			std::enable_if_t<std::is_same_v<std::decay_t<T>, quickjs::JSCString>
				|| std::is_same_v<std::decay_t<T>, quickjs::JSCStringA>, quickjs::JSValueRef>
				new_value_impl(JSContext* ctx, const T& val) {
				return JSValueRef(ctx, JS_NewStringA(ctx, *val));
			}

			template<typename Map>
			std::enable_if_t<type_traits::is_std_map<Map>::value, quickjs::JSValueRef>
				new_value_impl(JSContext* ctx, const Map& val) {
				using k_type = type_traits::types::key_t<Map>;
				using v_type = type_traits::types::mapped_t<Map>;

				JSValueRef object(ctx, JS_NewObject(ctx));
				for (auto it = val.begin(); it != val.end(); it++) {
					JSAtom key = JS_ValueToAtom(ctx, *quickjs::toJSValue<k_type>(ctx, it->first));
					JSValueRef value = quickjs::toJSValue<v_type>(ctx, it->second);
					JS_SetProperty(ctx, object.get(), key, value.release());
				}
				return object;
			}

			template<typename Arr>
			std::enable_if_t<type_traits::is_std_vector<Arr>::value
				|| type_traits::is_std_array<Arr>::value
				|| type_traits::is_std_set<Arr>::value
				|| type_traits::is_std_deque<Arr>::value
				|| type_traits::is_std_list<Arr>::value
				|| type_traits::is_std_forward_list<Arr>::value, quickjs::JSValueRef>
				new_value_impl(JSContext* ctx, const Arr& val) {
				using ele_type = type_traits::types::element_t<Arr>;
				JSValueRef array(ctx, JS_NewArray(ctx));
				uint32_t i = 0;
				for (auto it = val.begin(); it != val.end(); ++it) {
					JSValueRef value = quickjs::toJSValue<ele_type>(ctx, *it);
					JS_SetPropertyUint32(ctx, array.get(), i++, value.release());
				}
				return array;
			}

			template<typename Tuple, std::size_t... I>
			quickjs::JSValueRef new_tuple_impl(JSContext* ctx, const Tuple& val,
										   std::index_sequence<I...>) {
				JSValueRef array(ctx, JS_NewArray(ctx));
				using expander = int[];
				(void)expander {
					0, (
					 JS_SetPropertyUint32(ctx, array.get(), static_cast<uint32_t>(I),
										  quickjs::toJSValue<std::decay_t<std::tuple_element_t<I, Tuple>>>(
											  ctx, std::get<I>(val)).release()
					 ), 0)...
				};
				return array;
			}

			template<typename Tuple>
			std::enable_if_t<type_traits::is_std_tuple<Tuple>::value, quickjs::JSValueRef>
				new_value_impl(JSContext* ctx, const Tuple& val) {
				return new_tuple_impl(ctx, val,
								  std::make_index_sequence<std::tuple_size<Tuple>::value>{});
			}

			template<typename Fn>
			std::enable_if_t<type_traits::is_std_function<Fn>::value, quickjs::JSValueRef>
				new_value_impl(JSContext* ctx, const Fn& val) {
				using traits = type_traits::is_std_function<Fn>;
				using FnContext = type_traits::detail::func::JSFnContext<Fn>;
				constexpr size_t N = traits::size;
				FnContext* fnctx = new FnContext;
				fnctx->ctx = ctx;
				fnctx->caller = val;
				std::string name = "lambda_" + std::to_string(reinterpret_cast<uintptr_t>(&fnctx)) + "@" + std::to_string(N);
				JSValue closure = JS_NewCClosure(ctx,
												 &type_traits::detail::func::js_fn_call<Fn>,
												 name.c_str(),
												 &type_traits::detail::func::js_fn_finalizer<Fn>,
												 static_cast<int>(N), 0, fnctx);
				return JSValueRef(ctx, closure);
			}
		}
	}

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
		exception,
		error,
		promise,
		proxy
	};

	template<typename Type>
	class JSMemRef {
	public:
		JSMemRef() noexcept = default;

		explicit JSMemRef(JSRuntime* rt, Type* ptr)
			: runtime(rt), ptref(ptr) {}

		explicit JSMemRef(JSContext* ctx, Type* ptr)
			: runtime(JS_GetRuntime(ctx)), ptref(ptr) {}

		JSMemRef(const JSMemRef& other) = delete;

		JSMemRef(JSMemRef&& other) noexcept {
			reset(other.runtime, other.ptref);
			other.release();
		}

		~JSMemRef() {
			JS_FreeRT(runtime, ptref);
			release();
		}

		JSMemRef& operator=(const JSMemRef& other) = delete;

		JSMemRef& operator=(JSMemRef&& other) noexcept {
			reset(other.runtime, other.ptref);
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
			runtime = nullptr;
			return std::exchange(ptref, nullptr);
		}

		void reset(JSRuntime* rt = nullptr, Type* ptr = nullptr) noexcept {
			JS_FreeRT(runtime, ptref);
			runtime = rt;
			ptref = ptr;
		}

		void reset(JSContext* ctx = nullptr, Type* ptr = nullptr) noexcept {
			reset(ctx ? JS_GetRuntime(ctx) : nullptr, ptr);
		}

	private:
		Type* ptref = nullptr;
		JSRuntime* runtime = nullptr;
	};

	template<typename T>
	inline std::enable_if_t<!std::is_void_v<T>, quickjs::JSMemRef<T>>
		make_unique(JSContext* ctx, size_t size = 1) {
		return quickjs::JSMemRef<T>(ctx, static_cast<T*>(js_mallocz(ctx, sizeof(T) * size)));
	}
	template<typename T>
	inline std::enable_if_t<!std::is_void_v<T>, quickjs::JSMemRef<T>>
		make_unique(JSRuntime* rt, size_t size = 1) {
		return quickjs::JSMemRef<T>(rt, static_cast<T*>(js_mallocz_rt(rt, sizeof(T) * size)));
	}

	template<typename Character = quickjs::DefaultCharacter>
	class JSCStringRef {
	public:
		JSCStringRef() noexcept = default;

		explicit JSCStringRef(JSContext* ctx, JSValue val)
			:context(ctx), strptr(character.unwrap(ctx, val)) {}

		explicit JSCStringRef(JSContext* ctx, JSAtom atom) :context(ctx) {
			JSValue v = JS_AtomToValue(ctx, atom);
			strptr = character.unwrap(ctx, v);
			JS_FreeValue(ctx, v);
		}

		JSCStringRef(const JSCStringRef& other) = delete;

		JSCStringRef(JSCStringRef&& other) noexcept {
			reset(other.context, other.strptr);
			other.release();
		}

		~JSCStringRef() {
			if (context && strptr) {
				character.release(context, strptr);
			}
			release();
		}

		JSCStringRef& operator=(const JSCStringRef& other) = delete;

		JSCStringRef& operator=(JSCStringRef&& other) noexcept {
			reset(other.context, other.strptr);
			other.release();
			return *this;
		}

		const char* get() const noexcept {
			return strptr;
		}

		const char* operator*() const noexcept {
			return strptr;
		}

		explicit operator bool() const noexcept {
			return strptr != nullptr;
		}

		operator std::string() const noexcept {
			return std::string(strptr);
		}

		const char* release() noexcept {
			context = nullptr;
			return std::exchange(strptr, nullptr);
		}

		void reset(JSContext* ctx = nullptr, const char* cstr = nullptr) noexcept {
			if (context && strptr) {
				character.release(context, strptr);
			}
			context = ctx;
			strptr = cstr;
		}

		friend std::ostream& operator<<(std::ostream& os, const JSCStringRef& s) {
			return os << s.strptr;
		}

	private:
		const char* strptr = nullptr;
		JSContext* context = nullptr;
		Character character;
	};

	class JSAtomRef {
	public:
		JSAtomRef() noexcept = default;

		explicit JSAtomRef(JSContext* ctx, JSAtom atom)
			:context(ctx), jsatom(atom) {

		}

		explicit JSAtomRef(JSContext* ctx, JSValue val)
			:context(ctx), jsatom(JS_ValueToAtom(ctx, val)) {}

		JSAtomRef(const JSAtomRef& other) = delete;

		JSAtomRef(JSAtomRef&& other) noexcept {
			reset(other.context, other.jsatom);
			other.release();
		}

		JSAtomRef& operator=(const JSAtomRef& other) = delete;
		JSAtomRef& operator=(JSAtomRef&& other) noexcept {
			reset(other.context, other.jsatom);
			other.release();
			return *this;
		}

		~JSAtomRef() {
			if (context && jsatom != JS_ATOM_NULL) {
				JS_FreeAtom(context, jsatom);
			}
			release();
		}

		JSAtom get() const {
			return jsatom;
		}

		JSAtom operator*() const {
			return get();
		}

		operator bool() const noexcept {
			return jsatom != JS_ATOM_NULL;
		}

		JSAtom release() noexcept {
			context = nullptr;
			jsatom = JS_ATOM_NULL;
			return std::exchange(jsatom, 0);
		}

		void reset(JSContext* ctx = nullptr, JSAtom atom = 0) {
			if (context && jsatom) {
				JS_FreeAtom(context, jsatom);
			}
			context = ctx;
			jsatom = atom;
		}

	private:
		JSContext* context;
		JSAtom jsatom;
	};

	class JSValueRef {
	public:
		JSValueRef() noexcept = default;

		explicit JSValueRef(JSContext* ctx, JSValue val)
			: context(ctx), jsvalue(val) {}

		JSValueRef(const JSValueRef& other) = delete;

		JSValueRef(JSValueRef&& other) noexcept {
			reset(other.context, other.jsvalue);
			other.release();
		}

		virtual ~JSValueRef() {
			if (context && jsvalue != JS_UNDEFINED) {
				JS_FreeValue(context, jsvalue);
			}
			release();
		}

		JSValueRef& operator=(const JSValueRef& other) = delete;

		JSValueRef& operator=(JSValueRef&& other) noexcept {
			reset(other.context, other.jsvalue);
			other.release();
			return *this;
		}

		JSValue get() const noexcept {
			return jsvalue;
		}

		JSValue operator*() const noexcept {
			return jsvalue;
		}

		JSValue release() noexcept {
			context = nullptr;
			return std::exchange(jsvalue, JS_UNDEFINED);
		}

		void reset(JSContext* ctx = nullptr, JSValue val = JS_UNDEFINED) noexcept {
			if (context && jsvalue != JS_UNDEFINED) {
				JS_FreeValue(context, jsvalue);
			}
			context = ctx;
			jsvalue = val;
		}

		operator int() const {
			return as<int>();
		}

		operator size_t() const {
			return as<size_t>();
		}

		operator double() const {
			return as<double>();
		}

		operator float() const {
			return as<float>();
		}

		operator bool() const {
			return as<bool>();
		}

		operator std::string() const {
			return as<std::string>();
		}

		operator quickjs::JSCString() const {
			return as<quickjs::JSCString>();
		}

		operator quickjs::JSCStringA() const {
			return as<quickjs::JSCStringA>();
		}

		/*Note: When using the class quickjs::JSPropertyRef, it must be defined after quickjs::JSPropertyRef */
		//Note: If is object
		quickjs::JSPropertyRef operator[](const std::string& key) const;
		//Note: If is object
		quickjs::JSPropertyRef operator[](const char* key) const;
		//Note: If is object
		quickjs::JSPropertyRef operator[](char* key) const;

		//Note: If is array, without verify length
		quickjs::JSPropertyRef operator[](int idx) const;

		//Note: If is string/array/function, get property .length
		size_t length() const {
			if (!is<JSType::string>() && !is<JSType::array>() && !is<JSType::function>()) {
				throw type_error(quickjs::format_str("cannot read property 'length' of %s", type_name(type()).c_str()));
			}
			int64_t len;
			if (JS_GetPropertyLength(context, &len, jsvalue) == -1) {
				throw type_error("failed to read property 'length'");
			}
			return static_cast<size_t>(len);
		}

		/**
		 * @note For std::map, this is stricter than is<JSType::object>():
		 *       it requires a plain object (checked via JS_IsObjectPlain, which
		 *       inspects the object's class id), whereas is<JSType::object>()
		 *       only checks that the tag is JS_TAG_OBJECT.
		 */
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
			} else if (type_traits::is_std_array<T>::value
					   || type_traits::is_std_set<T>::value
					   || type_traits::is_std_vector<T>::value
					   || type_traits::is_std_tuple<T>::value
					   || type_traits::is_std_deque<T>::value
					   || type_traits::is_std_list<T>::value
					   || type_traits::is_std_forward_list<T>::value) {
				return is<JSType::array>();
			} else if (type_traits::is_std_map<T>::value) {
				if (!context)
					return false;
				return JS_IsObjectPlain(context, jsvalue);
			} else if (type_traits::is_std_function<T>::value) {
				return is<JSType::function>();
			}
			return false;
		}

		bool isNAN() const {
			return JS_VALUE_IS_NAN(jsvalue);
		}

		template<JSType T>
		bool is() const {
			return false;
		}

		template<>
		bool is<JSType::integer>() const {
			return JS_VALUE_GET_TAG(jsvalue) == JS_TAG_INT;
		}

		template<>
		bool is<JSType::boolean>() const {
			return JS_VALUE_GET_TAG(jsvalue) == JS_TAG_BOOL;
		}

		template<>
		bool is<JSType::number>() const {
			return JS_IsNumber(jsvalue);
		}

		template<>
		bool is<JSType::string>() const {
			return JS_VALUE_GET_TAG(jsvalue) == JS_TAG_STRING;
		}

		template<>
		bool is<JSType::object>() const {
			return (JS_VALUE_GET_TAG(jsvalue) == JS_TAG_OBJECT);
		}

		template<>
		bool is<JSType::array>() const {
			if (!context)
				return false;
			return JS_IsArray(context, jsvalue);
		}

		template<>
		bool is<JSType::function>() const {
			if (!context)
				return false;
			return JS_IsFunction(context, jsvalue);
		}

		template<>
		bool is<JSType::symbol>() const {
			return JS_VALUE_GET_TAG(jsvalue) == JS_TAG_SYMBOL;
		}

		template<>
		bool is<JSType::undefined>() const {
			return JS_VALUE_GET_TAG(jsvalue) == JS_TAG_UNDEFINED;
		}

		template<>
		bool is<JSType::uninitialized>() const {
			return JS_VALUE_GET_TAG(jsvalue) == JS_TAG_UNINITIALIZED;
		}

		template<>
		bool is<JSType::null>() const {
			return JS_VALUE_GET_TAG(jsvalue) == JS_TAG_NULL;
		}

		template<>
		bool is<JSType::exception>() const {
			return JS_VALUE_GET_TAG(jsvalue) == JS_TAG_EXCEPTION;
		}

		template<>
		bool is<JSType::error>() const {
			if (!context)
				return false;
			return JS_IsError(context, jsvalue);
		}

		template<>
		bool is<JSType::promise>() const {
			return JS_IsPromise(jsvalue);
		}

		template<>
		bool is<JSType::proxy>() const {
			return JS_IsProxy(jsvalue);
		}

		/**
		 * @brief Check whether the JSValue matches the given runtime type.
		 *
		 * @param type  The JSType to validate against.
		 * @return True if the current value matches the given type; false otherwise
		 *         (including unsupported types).
		 */
		bool is(JSType type) const {
			switch (type) {
				case quickjs::JSType::object:       return is<JSType::object>();
				case quickjs::JSType::array:        return is<JSType::array>();
				case quickjs::JSType::integer:      return is<JSType::integer>();
				case quickjs::JSType::boolean:      return is<JSType::boolean>();
				case quickjs::JSType::number:       return is<JSType::number>();
				case quickjs::JSType::string:       return is<JSType::string>();
				case quickjs::JSType::function:     return is<JSType::function>();
				case quickjs::JSType::symbol:       return is<JSType::symbol>();
				case quickjs::JSType::undefined:    return is<JSType::undefined>();
				case quickjs::JSType::uninitialized:return is<JSType::uninitialized>();
				case quickjs::JSType::null:         return is<JSType::null>();
				case quickjs::JSType::exception:    return is<JSType::exception>();
				case quickjs::JSType::error:        return is<JSType::error>();
				case quickjs::JSType::promise:      return is<JSType::promise>();
				case quickjs::JSType::proxy:        return is<JSType::proxy>();
				default: break;
			}
			return false;
		}

		/**
		 * @brief Converts the current JSValue to the requested C++ type T.
		 *
		 * @tparam T The desired C++ type.
		 * @return The converted value as type T.
		 *
		 * @throws type_error If the current value does not match the type required
		 *                    by T, if the underlying JS conversion fails, or if an
		 *                    unsigned integer type is requested but the value is
		 *                    negative.
		 *
		 * @note For integral types the value is converted via JS_ToInt32, so the
		 *       range is limited to 32-bit signed integers. Negative values are
		 *       rejected when T is an unsigned type.
		 */
		template<typename T>
		std::enable_if_t<!std::is_same_v<std::decay_t<T>, const char*>
			&& !std::is_same_v<std::decay_t<T>, char*>, T>
			as() const {
			return as_impl_<T>(true, true);
		}

		/**
		 * @brief Fully unchecked conversion following JavaScript's conversion
		 *        rules, including nested element types.
		 *
		 * @tparam T The desired C++ type.
		 * @return The converted value as T.
		 *
		 * @throws type_error If the underlying JS conversion fails, or if an
		 *                    unsigned integer type is requested but the value is
		 *                    negative.
		 *
		 * @note For integral types the value is converted via JS_ToInt32, so the
		 *       range is limited to 32-bit signed integers.
		 */
		template<typename T>
		std::enable_if_t<!std::is_same_v<std::decay_t<T>, const char*>
			&& !std::is_same_v<std::decay_t<T>, char*>, T>
			cast() const {
			return as_impl_<T>(false, false);
		}

		/**
		 * @brief Converts the current JSValue to T per JavaScript's rules,
		 *        with strict checking of nested element types.
		 *
		 * @tparam T The desired C++ type.
		 * @return The converted value as T.
		 *
		 * @throws type_error If the underlying JS conversion fails, or if an
		 *                    unsigned integer type is requested but the value is
		 *                    negative.
		 *
		 * | Call      | strict | nested | Semantics                        |
		 * |-----------|--------|--------|----------------------------------|
		 * | as<T>()   | true   | true   | Strict value + strict nesting    |
		 * | cast<T>() | false  | false  | Lenient value + lenient nesting  |
		 * | to<T>()   | false  | true   | Lenient value + strict nesting   |
		 */
		template<typename T>
		std::enable_if_t<!std::is_same_v<std::decay_t<T>, const char*>
			&& !std::is_same_v<std::decay_t<T>, char*>, T>
			to() const {
			return as_impl_<T>(false, true);
		}

		/**
		 * @brief Note: If the argument is passed, verify the type. Or return the default value.
		 *
		 * @param default_val  The fallback value to return if validation fails or context is null.
		 * @param throw_err    If true, throws quickjs::type_error on type mismatch; if false, returns default_val.
		 */
		template<typename T>
		std::enable_if_t<!std::is_same_v<std::decay_t<T>, const char*>
			&& !std::is_same_v<std::decay_t<T>, char*>, T>
			as(T default_val, bool throw_err = true) const {
			if (!context) {
				return default_val;
			} else if (!is<T>()) {
				if (!throw_err)
					return default_val;

				throw type_error(quickjs::format_str("%s is required", type_name<T>().c_str()));
			}
			return as<T>();
		}

		template<typename T>
		T stringify() const = delete;

		template<>
		std::string stringify<std::string>() const {
			if (is<JSType::string>()) {
				return as<std::string>();
			}
			JSValueRef jsstr(context, JS_JSONStringify(context, jsvalue, JS_UNDEFINED, JS_UNDEFINED));
			if (!jsstr.is<JSType::string>()) {
				throw type_error("stringify error");
			}
			return jsstr.as<std::string>();
		}

		template<>
		quickjs::JSCString stringify<quickjs::JSCString>() const {
			if (is<JSType::string>()) {
				return as<quickjs::JSCString>();
			}
			JSValueRef jsstr(context, JS_JSONStringify(context, jsvalue, JS_UNDEFINED, JS_UNDEFINED));
			if (!jsstr.is<JSType::string>()) {
				throw type_error("stringify error");
			}
			return jsstr.as<quickjs::JSCString>();
		}

		template<>
		quickjs::JSCStringA stringify<quickjs::JSCStringA>() const {
			if (is<JSType::string>()) {
				return as<quickjs::JSCStringA>();
			}
			JSValueRef jsstr(context, JS_JSONStringify(context, jsvalue, JS_UNDEFINED, JS_UNDEFINED));
			if (!jsstr.is<JSType::string>()) {
				throw type_error("stringify error");
			}
			return jsstr.as<quickjs::JSCStringA>();
		}

	protected:
		virtual quickjs::type_error type_error(const std::string& msg) const {
			return quickjs::type_error(msg);
		}

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
			} else if (type_traits::is_std_array<T>::value
					   || type_traits::is_std_set<T>::value
					   || type_traits::is_std_vector<T>::value
					   || type_traits::is_std_tuple<T>::value
					   || type_traits::is_std_deque<T>::value
					   || type_traits::is_std_list<T>::value
					   || type_traits::is_std_forward_list<T>::value) {
				return "array";
			} else if (type_traits::is_std_map<T>::value) {
				return "object";
			} else if (type_traits::is_std_function<T>::value) {
				return "function";
			}
			return "[type mismatch]";
		}

		template<JSType T>
		std::string type_name() const noexcept {
			switch (T) {
				case quickjs::JSType::object:       return "object";
				case quickjs::JSType::array:        return "array";
				case quickjs::JSType::integer:      return "integer";
				case quickjs::JSType::boolean:      return "boolean";
				case quickjs::JSType::number:       return "number";
				case quickjs::JSType::string:       return "string";
				case quickjs::JSType::function:     return "function";
				case quickjs::JSType::symbol:       return "symbol";
				case quickjs::JSType::undefined:    return "undefined";
				case quickjs::JSType::uninitialized:return "uninitialized";
				case quickjs::JSType::null:         return "null";
				case quickjs::JSType::exception:    return "exception";
				case quickjs::JSType::error:        return "error";
				case quickjs::JSType::promise:      return "promise";
				case quickjs::JSType::proxy:        return "proxy";
				default: break;
			}
			return "[type mismatch]";
		}

		std::string type_name(JSType type) const noexcept {
			switch (type) {
				case quickjs::JSType::object:       return type_name<JSType::object>();
				case quickjs::JSType::array:        return type_name<JSType::array>();
				case quickjs::JSType::integer:      return type_name<JSType::integer>();
				case quickjs::JSType::boolean:      return type_name<JSType::boolean>();
				case quickjs::JSType::number:       return type_name<JSType::number>();
				case quickjs::JSType::string:       return type_name<JSType::string>();
				case quickjs::JSType::function:     return type_name<JSType::function>();
				case quickjs::JSType::symbol:       return type_name<JSType::symbol>();
				case quickjs::JSType::undefined:    return type_name<JSType::undefined>();
				case quickjs::JSType::uninitialized:return type_name<JSType::uninitialized>();
				case quickjs::JSType::null:         return type_name<JSType::null>();
				case quickjs::JSType::exception:    return type_name<JSType::exception>();
				case quickjs::JSType::error:        return type_name<JSType::error>();
				case quickjs::JSType::promise:      return type_name<JSType::promise>();
				case quickjs::JSType::proxy:        return type_name<JSType::proxy>();
				default: break;
			}
			return "[type mismatch]";
		}

		/**
		* @brief Returns the concrete JavaScript type of the wrapped value.
		* @return The first matching JSType, or JSType::uninitialized if none matched.
		* @warning Order matters: test more specific types before general ones
		*          (e.g. array/function before object), since probes may overlap.
		* @see is()
		*/
		JSType type() const {
			if (is<quickjs::JSType::object>())       return quickjs::JSType::object;
			if (is<quickjs::JSType::array>())        return quickjs::JSType::array;
			if (is<quickjs::JSType::integer>())      return quickjs::JSType::integer;
			if (is<quickjs::JSType::boolean>())      return quickjs::JSType::boolean;
			if (is<quickjs::JSType::number>())       return quickjs::JSType::number;
			if (is<quickjs::JSType::string>())       return quickjs::JSType::string;
			if (is<quickjs::JSType::function>())     return quickjs::JSType::function;
			if (is<quickjs::JSType::symbol>())       return quickjs::JSType::symbol;
			if (is<quickjs::JSType::undefined>())    return quickjs::JSType::undefined;
			if (is<quickjs::JSType::uninitialized>())return quickjs::JSType::uninitialized;
			if (is<quickjs::JSType::null>())         return quickjs::JSType::null;
			if (is<quickjs::JSType::exception>())    return quickjs::JSType::exception;
			if (is<quickjs::JSType::error>())        return quickjs::JSType::error;
			if (is<quickjs::JSType::promise>())      return quickjs::JSType::promise;
			if (is<quickjs::JSType::proxy>())        return quickjs::JSType::proxy;
			return quickjs::JSType::uninitialized;
		}

	private:
		/**
		 * @brief Checks whether the current JSValue can be safely converted to type T.
		 *
		 * @tparam T The desired C++ type.
		 * @param[out] msg Filled with a human-readable reason when the check fails.
		 * @param[in]  strict If true, require is<T>() to hold; if false, follow the
		 *                    JavaScript type-conversion rules and accept any value
		 *                    that is convertible to T.
		 * @return true if the value can be converted to T, false otherwise.
		 *
		 * @note The default implementation accepts any value; specialized overloads
		 *       narrow this down according to the JavaScript type-conversion rules
		 *       (e.g. Symbol cannot be converted to a number or string, objects must
		 *       be ToPrimitive-convertible, containers require an array/object source,
		 *       std::function requires a callable).
		 */
		template<typename T>
		std::enable_if_t<std::is_same_v<std::decay_t<T>, bool>, bool>
			cast_check(std::string& err, bool strict) const {
			if (strict && !is<T>()) {
				err = quickjs::format_str("%s is required", type_name<T>().c_str());
				return false;
			}
			return true;
		}

		template<typename T>
		std::enable_if_t<(std::is_integral_v<std::decay_t<T>>
			|| std::is_floating_point_v<std::decay_t<T>>) && !std::is_same_v<std::decay_t<T>, bool>, bool>
			cast_check(std::string& err, bool strict) const {
			if (is<JSType::symbol>()) {
				err = "cannot convert a Symbol value to a number";
				return false;
			}
			if (strict && !is<T>()) {
				err = quickjs::format_str("%s is required", type_name<T>().c_str());
				return false;
			}
			if (is<JSType::object>()) {
				double probe;
				if (JS_ToFloat64(context, &probe, jsvalue) < 0) {
					//clear exception
					JSValueRef jserr(context, JS_GetException(context));
					err = "cannot convert object to primitive value";
					return false;
				}
			}
			return true;
		}

		template<typename T>
		std::enable_if_t<std::is_same_v<std::decay_t<T>, std::string>
			|| std::is_same_v<std::decay_t<T>, quickjs::JSCString>
			|| std::is_same_v<std::decay_t<T>, quickjs::JSCStringA>, bool>
			cast_check(std::string& err, bool strict) const {
			if (is<JSType::symbol>()) {
				err = "cannot convert a Symbol value to a string";
				return false;
			}
			if (strict && !is<T>()) {
				err = quickjs::format_str("%s is required", type_name<T>().c_str());
				return false;
			}
			return true;
		}

		template<typename T>
		std::enable_if_t<type_traits::is_std_function<T>::value, bool>
			cast_check(std::string& err, bool strict) const {
			if (!is<T>()) {
				err = "function is required";
				return false;
			}
			return true;
		}

		template<typename T>
		std::enable_if_t<type_traits::is_std_array<T>::value
			|| type_traits::is_std_set<T>::value
			|| type_traits::is_std_vector<T>::value
			|| type_traits::is_std_tuple<T>::value
			|| type_traits::is_std_deque<T>::value
			|| type_traits::is_std_list<T>::value
			|| type_traits::is_std_forward_list<T>::value, bool>
			cast_check(std::string& err, bool strict) const {
			if (!is<T>()) {
				err = "array is required";
				return false;
			}
			return true;
		}

		template<typename T>
		std::enable_if_t<type_traits::is_std_map<T>::value, bool>
			cast_check(std::string& err, bool strict) const {
			if (!is<T>()) {
				err = "object is required";
				return false;
			}
			return true;
		}

		/**
		 * @brief Converts the current JSValue to the requested C++ type T.
		 *
		 * @tparam T The desired C++ type.
		 * @param[in] strict If true, strictly check that the JS value matches the
		 *                   corresponding C++ type; if false, convert per
		 *                   JavaScript's conversion rules. Note that for
		 *                   std::function<...>, strict only checks the declared
		 *                   return type.
		 * @param[in] nested If true, strictly validate nested element types of
		 *                   wrapper types (e.g. std::map, std::vector,
		 *                   std::tuple, std::function); if false, convert nested
		 *                   elements per JavaScript's rules.
		 *
		 * @return The converted value as type T.
		 *
		 * @throws type_error Thrown when the value does not match the type
		 *                    required by T according to @p strict.
		 *
		 * @note Using as<std::function<...>>() strictly enforces the declared
		 *       return type, throwing on mismatch.
		 *       Using cast<std::function<...>>() converts the return value
		 *       according to JavaScript's conversion rules, throwing on
		 *       mismatch.
		 *
		 * @note Integral types are converted via JS_ToInt32, limiting the range
		 *       to 32-bit signed integers. Negative values are rejected when T
		 *       is an unsigned type.
		 */
		template<typename T>
		T as_impl_(bool strict, bool nested) const {
			return as_impl<T>(strict, nested);
		}

		template<typename Map>
		std::enable_if_t<type_traits::is_std_map<Map>::value, Map>
			as_impl(bool strict, bool nested) const {
			std::string err;
			if (!cast_check<Map>(err, strict))
				throw type_error(err);

			using key_type = type_traits::types::key_t<Map>;
			using val_type = type_traits::types::mapped_t<Map>;

			Map map{};
			uint32_t len = 0;
			JSPropertyEnum* tab;
			if (JS_GetOwnPropertyNames(context, &tab, &len, jsvalue,
									   JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) < 0) {
				throw type_error("fail to read property names");
			}
			struct JSPropertyEnumGuard {
				JSContext* ctx;
				JSPropertyEnum* tab;
				uint32_t len;
				~JSPropertyEnumGuard() {
					if (tab) JS_FreePropertyEnum(ctx, tab, len);
				}
			} peg{ context, tab, len };

			for (int i = 0; i < len; i++) {
				JSValueRef key(context, JS_AtomToValue(context, tab[i].atom));
				JSValueRef val(context, JS_GetProperty(context, jsvalue, tab[i].atom));

				std::string err;
				if (!key.cast_check<key_type>(err, nested))
					throw type_error(quickjs::format_str("[%s.k]: %s"
														 , type_traits::container::name<Map>::value
														 , err.c_str()));
				if (!val.cast_check<val_type>(err, nested))
					throw type_error(quickjs::format_str("[%s.v]: %s"
														 , type_traits::container::name<Map>::value
														 , err.c_str()));
				key_type kv = key.as_impl_<key_type>(nested, nested);
				val_type vv = val.as_impl_<val_type>(nested, nested);
				map.insert(std::make_pair<>(kv, vv));
			}
			return map;
		}

		template<typename Seq>
		std::enable_if_t<type_traits::is_std_vector<Seq>::value
			|| type_traits::is_std_deque<Seq>::value, Seq>
			as_impl(bool strict, bool nested) const {
			std::string err;
			if (!cast_check<Seq>(err, strict))
				throw type_error(err);

			using ele_type = type_traits::types::element_t<Seq>;
			size_t len = length();

			Seq vec{};
			for (size_t i = 0; i < len; i++) {
				JSValueRef elem(context, JS_GetPropertyUint32(context, jsvalue, i));
				std::string err;
				if (!elem.cast_check<ele_type>(err, nested))
					throw type_error(quickjs::format_str("[%s]: %s"
														 , type_traits::container::name<Seq>::value
														 , err.c_str()));
				ele_type v = elem.as_impl_<ele_type>(nested, nested);
				vec.push_back(std::move(v));
			}
			return vec;
		}

		template<typename List>
		std::enable_if_t<type_traits::is_std_list<List>::value, List>
			as_impl(bool strict, bool nested) const {
			std::string err;
			if (!cast_check<List>(err, strict))
				throw type_error(err);

			using ele_type = std::decay_t<type_traits::types::element_t<List>>;
			size_t len = length();
			List lst{};
			for (size_t i = 0; i < len; i++) {
				JSValueRef elem(context,
								JS_GetPropertyUint32(context, jsvalue, static_cast<uint32_t>(i)));
				std::string err;
				if (!elem.cast_check<ele_type>(err, nested))
					throw type_error(quickjs::format_str(
						"[%s]: %s"
						, type_traits::container::name<List>::value
						, err.c_str()));
				lst.push_back(elem.as_impl_<ele_type>(nested, nested));
			}
			return lst;
		}

		template<typename FL>
		std::enable_if_t<type_traits::is_std_forward_list<FL>::value, FL>
			as_impl(bool strict, bool nested) const {
			std::string err;
			if (!cast_check<FL>(err, strict))
				throw type_error(err);

			using ele_type = type_traits::types::element_t<FL>;

			size_t len = length();
			FL fl{};

			auto it = fl.before_begin();
			for (size_t i = 0; i < len; i++) {
				JSValueRef elem(context,
								JS_GetPropertyUint32(context, jsvalue, static_cast<uint32_t>(i)));
				std::string err;
				if (!elem.cast_check<ele_type>(nested))
					throw type_error(quickjs::format_str(
						"[%s]: %s"
						, type_traits::container::name<FL>::value
						, err.c_str()));

				it = fl.insert_after(it, elem.as_impl_<ele_type>(nested, nested));
			}
			return fl;
		}

		template<typename Set>
		std::enable_if_t<type_traits::is_std_set<Set>::value, Set>
			as_impl(bool strict, bool nested) const {
			std::string err;
			if (!cast_check<Set>(err, strict))
				throw type_error(err);

			using ele_type = type_traits::types::element_t<Set>;
			size_t len = length();

			Set set{};
			for (size_t i = 0; i < len; i++) {
				JSValueRef elem(context, JS_GetPropertyUint32(context, jsvalue, i));
				std::string err;
				if (!elem.cast_check<ele_type>(err, nested))
					throw type_error(quickjs::format_str("[%s]: %s"
														 , type_traits::container::name<Set>::value
														 , err.c_str()));
				ele_type v = elem.as_impl_<ele_type>(nested, nested);
				set.insert(std::move(v));
			}
			return set;
		}

		template<typename Arr>
		std::enable_if_t<type_traits::is_std_array<Arr>::value, Arr>
			as_impl(bool strict, bool nested) const {
			std::string err;
			if (!cast_check<Arr>(err, strict))
				throw type_error(err);

			using ele_type = type_traits::types::element_t<Arr>;
			size_t len = length();

			constexpr std::size_t N = type_traits::is_std_array<Arr>::size;
			Arr array{};
			for (size_t i = 0; i < len; i++) {
				if (i == N)
					break;
				JSValueRef elem(context, JS_GetPropertyUint32(context, jsvalue, i));
				std::string err;
				if (!elem.cast_check<ele_type>(err, nested))
					throw type_error(quickjs::format_str("[%s]: %s"
														 , type_traits::container::name<Arr>::value
														 , err.c_str()));
				array[i] = elem.as_impl_<ele_type>(nested, nested);
			}
			return array;
		}

		template <typename Tuple, std::size_t I>
		using tuple_element_t = typename std::tuple_element<I, Tuple>::type;

		template<typename Tuple, std::size_t... Is>
		Tuple as_tuple_impl(std::index_sequence<Is...>, bool nested) const {
			return Tuple{
				[this, nested]() -> tuple_element_t<Tuple, Is> {
				using ele_type = tuple_element_t<Tuple, Is>;
				JSValueRef elem(context,
								JS_GetPropertyUint32(context, jsvalue, static_cast<uint32_t>(Is)));
				std::string err;
				if (!elem.template cast_check<ele_type>(err, nested)) {
					throw type_error(quickjs::format_str(
						"[%s]: %s"
						, type_traits::container::name<Tuple>::value
						, err.c_str()));
				}
				return elem.template as_impl_<ele_type>(nested, nested);
			}()...
			};
		}

		template<typename Tuple>
		std::enable_if_t<type_traits::is_std_tuple<Tuple>::value, Tuple>
			as_impl(bool strict, bool nested) const {
			constexpr std::size_t N = std::tuple_size<Tuple>::value;
			std::string err;
			if (!cast_check<Tuple>(err, strict))
				throw type_error(err);

			if (static_cast<std::size_t>(length()) < N) {
				throw type_error(quickjs::format_str("[%s]: length mismatch"
													 , type_traits::container::name<Tuple>::value));
			}
			return as_tuple_impl<Tuple>(std::make_index_sequence<N>{}, nested);
		}

		template<typename ArgTypes, std::size_t N>
		std::enable_if_t<std::tuple_size<ArgTypes>::value != 0, JSValueRef>
			as_function_impl_thisArg(const std::array<JSValueRef, N>& arg_refs) {
			using Arg0 = typename std::tuple_element<0, ArgTypes>::type;
			if (std::is_same_v<std::decay_t<Arg0>, JSValueRef>
				|| std::is_base_of_v<JSValueRef, std::decay_t<Arg0>>
				|| std::is_same_v<std::decay_t<Arg0>, JSValue>) {
				return JSValueRef(context, JS_DupValue(context, arg_refs[0].get()));
			}
			return JSValueRef(context, JS_UNDEFINED);
		}

		template<typename ArgTypes, std::size_t N>
		std::enable_if_t<std::tuple_size<ArgTypes>::value == 0, JSValueRef>
			as_function_impl_thisArg(const std::array<JSValueRef, N>& arg_refs) {
			return JSValueRef(context, JS_UNDEFINED);
		}

		template <typename Fn, std::size_t... Is,
			typename std::enable_if<!std::is_void<typename type_traits::is_std_function<Fn>::return_type>::value,
			int>::type = 0>
		Fn as_function_impl(std::index_sequence<Is...>, bool strict, bool nested) const {
			typedef type_traits::is_std_function<Fn> traits;
			typedef typename traits::return_type     R;

			auto holder = std::make_shared<JSValueRef>(context, JS_DupValue(context, jsvalue));
			return Fn([this, holder, context = this->context, strict, nested](typename std::tuple_element<Is, typename traits::arg_types>::type... args) -> R {
				std::array<JSValueRef, sizeof...(Is)> arg_refs = { {
						quickjs::toJSValue<typename std::tuple_element<Is, typename traits::arg_types>::type>(context, args)...
					} };
				JSValueRef thisArg = as_function_impl_thisArg<typename traits::arg_types, sizeof...(Is)>(arg_refs);
				std::size_t idx = thisArg.is<JSType::undefined>() ? 0 : 1;
				std::vector<JSValue> argv;
				for (; idx < sizeof...(Is); ++idx)
					argv.emplace_back(arg_refs[idx].get());

				JSValueRef ret(context, JS_Call(context, holder->get(), thisArg.get(), argv.size(), argv.data()));
				if (ret.template is<JSType::exception>()) {
					throw std::runtime_error(quickjs::to_string(ret));
				}
				std::string err;
				if (!ret.cast_check<R>(err, strict))
					throw quickjs::type_error(err.c_str());
				return ret.template as_impl_<R>(strict, nested);
			});
		}

		template <typename Fn, std::size_t... Is,
			typename std::enable_if<
			std::is_void<typename type_traits::is_std_function<Fn>::return_type>::value,
			int>::type = 0>
		Fn as_function_impl(std::index_sequence<Is...>, bool strict, bool nested) const {
			typedef type_traits::is_std_function<Fn> traits;

			auto holder = std::make_shared<JSValueRef>(context, JS_DupValue(context, jsvalue));
			return Fn([this, holder, context = this->context](typename std::tuple_element<Is, typename traits::arg_types>::type... args) -> void {
				std::array<JSValueRef, sizeof...(Is)> arg_refs = { {
						quickjs::toJSValue<typename std::tuple_element<Is, typename traits::arg_types>::type>(context, args)...
					} };
				JSValueRef thisArg = as_function_impl_thisArg<typename traits::arg_types, sizeof...(Is)>(arg_refs);
				std::size_t idx = thisArg.is<JSType::undefined>() ? 0 : 1;
				std::vector<JSValue> argv;
				for (; idx < sizeof...(Is); ++idx)
					argv.emplace_back(arg_refs[idx].get());

				JSValueRef ret(context, JS_Call(context, holder->get(), thisArg.get(), argv.size(), argv.data()));
				if (ret.template is<JSType::exception>()) {
					throw std::runtime_error(quickjs::to_string(ret));
				}
			});
		}

		template<typename Fn>
		std::enable_if_t<type_traits::is_std_function<Fn>::value, Fn>
			as_impl(bool strict, bool nested) const {
			std::string err;
			if (!cast_check<Fn>(err, strict))
				throw type_error(err);

			constexpr std::size_t N = type_traits::is_std_function<Fn>::size;
			return as_function_impl<Fn>(std::make_index_sequence<N>{}, strict, nested);
		}

		template<typename T>
		std::enable_if_t<std::is_same_v<std::decay_t<T>, bool>, T>
			as_impl(bool strict, bool nested) const {
			std::string err;
			if (!cast_check<T>(err, strict))
				throw type_error(err);
			return static_cast<T>(JS_ToBool(context, jsvalue));
		}

		template<typename T>
		std::enable_if_t<std::is_integral_v<std::decay_t<T>>
			&& !std::is_same_v<std::decay_t<T>, bool>, T>
			as_impl(bool strict, bool nested) const {
			std::string err;
			if (!cast_check<T>(err, strict))
				throw type_error(err);
			int val;
			if (JS_ToInt32(context, &val, jsvalue) == -1) {
				throw type_error("type conversion error");
			}
			if (std::is_unsigned_v<T> && val < 0) {
				throw type_error("positive integer is required");
			}
			return static_cast<T>(val);
		}

		template<typename T>
		std::enable_if_t<std::is_floating_point_v<std::decay_t<T>>, T>
			as_impl(bool strict, bool nested) const {
			std::string err;
			if (!cast_check<T>(err, strict))
				throw type_error(err);
			double val;
			if (JS_ToFloat64(context, &val, jsvalue) == -1) {
				throw type_error("type conversion error");
			}
			return static_cast<T>(val);
		}

		template<typename T>
		std::enable_if_t<std::is_same_v<std::decay_t<T>, std::string>, T>
			as_impl(bool strict, bool nested) const {
			std::string err;
			if (!cast_check<T>(err, strict))
				throw type_error(err);
			const char* s = JS_ToCString(context, jsvalue);
			if (!s)
				throw type_error("string is nullptr");
			std::string str(s);
			JS_FreeCString(context, s);
			return s;
		}

		template<typename T>
		std::enable_if_t<std::is_same_v<std::decay_t<T>, quickjs::JSCString>, T>
			as_impl(bool strict, bool nested) const {
			std::string err;
			if (!cast_check<T>(err, strict))
				throw type_error(err);
			return quickjs::JSCString(context, jsvalue);
		}

		template<typename T>
		std::enable_if_t<std::is_same_v<std::decay_t<T>, quickjs::JSCStringA>, T>
			as_impl(bool strict, bool nested) const {
			std::string err;
			if (!cast_check<T>(err, strict))
				throw type_error(err);
			return quickjs::JSCStringA(context, jsvalue);
		}

		JSValue jsvalue = JS_UNDEFINED;
		JSContext* context = nullptr;

		friend std::string to_string(const JSValueRef& ref);
	};

	class JSPropertyRef :public JSValueRef {
	public:
		JSPropertyRef() = delete;

		explicit JSPropertyRef(JSContext* ctx, JSValue val, std::string name) :JSValueRef(ctx, val),
			property_name(name),
			property_exists(ctx != nullptr) {}

		explicit JSPropertyRef(std::string name) :JSValueRef(),
			property_name(name),
			property_exists(false) {}

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
	inline quickjs::JSPropertyRef quickjs::JSValueRef::operator[](const std::string& key) const {
		return operator[](key.c_str());
	}

	inline quickjs::JSPropertyRef quickjs::JSValueRef::operator[](const char* key) const {
		return operator[](const_cast<char*>(key));
	}

	inline quickjs::JSPropertyRef quickjs::JSValueRef::operator[](char* key) const {
		if (!is<JSType::object>()) {
			throw type_error("object is required");
		}
		JSAtomRef prop(context, *JSValueRef(context, JS_NewString(context, key)));
		if (JS_HasProperty(context, jsvalue, *prop)) {
			return quickjs::JSPropertyRef(context, JS_GetProperty(context, jsvalue, *prop), key);
		}
		return quickjs::JSPropertyRef(key);
	}

	inline quickjs::JSPropertyRef quickjs::JSValueRef::operator[](int idx) const {
		if (!is<JSType::array>()) {
			throw type_error("array is required");
		}
		JSAtomRef prop(context, *JSValueRef(context, JS_NewInt32(context, idx)));
		if (JS_HasProperty(context, jsvalue, *prop)) {
			return quickjs::JSPropertyRef(context, JS_GetProperty(context, jsvalue, *prop), quickjs::format_str("[%d]", idx));
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
			no_arguments(ctx == nullptr) {}

		explicit JSArgumentRef(int arg_idx) :JSValueRef(),
			argument_idx(arg_idx),
			no_arguments(true) {}

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

		/**
		* @brief Verifies that the JSValue matches any of the given types; throws on mismatch.
		*
		* @param types  The expected JSType(s) to validate against. The check passes if the
		*               value matches at least one of them.
		* @return The index of the first matching type in @p types.
		* @throws type_error If the current value matches none of the given types.
		*/
		template<typename... Args, typename = std::enable_if_t<std::conjunction<std::is_same<Args, JSType>...>::value>>
		size_t require(Args... types) const {
			constexpr size_t arg_size = sizeof...(Args);
			bool matched = false;
			std::string names;
			size_t i = 0;
			const JSType tags[] = { static_cast<JSType>(types)... };
			for (; i < arg_size; i++) {
				names = names.empty() ? type_name(tags[i]) : names + "|" + type_name(tags[i]);
				if (is(tags[i])) {
					matched = true;
					break;
				}
			}
			if (!matched) {
				throw type_error(quickjs::format_str("%s is required", names.c_str()));
			}
			return i;
		}

		template<>
		size_t require() const = delete;

		/**
		* @brief Verify the JSValue matches the compile-time type T, throw on mismatch.
		*
		* @tparam T  The expected JSType, checked at compile time.
		* @throws type_error If the current value does not match the given type T.
		*/
		template<JSType T>
		void require() const {
			if (!is<T>()) {
				throw type_error(quickjs::format_str("%s is required", type_name<T>().c_str()));
			}
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
	inline quickjs::JSArgumentRef dump_argument(JSContext* ctx, int argc, JSValue* argv) {
		if (Index < argc) {
			return quickjs::JSArgumentRef(ctx, JS_DupValue(ctx, argv[Index]), Index);
		}
		return quickjs::JSArgumentRef(Index);
	}

	class JSArguments {
	public:
		explicit JSArguments(JSContext* ctx, int argc, JSValueConst* argv)
			:ctx_(ctx), argc_(argc), argv_(argv), argumentsGroupId(-1), isGroupRegisted(false) {}

		JSArguments(const JSArguments& arguments) = delete;
		JSArguments(JSArguments&& arguments) = delete;

		JSArguments& operator=(const JSArguments& arguments) = delete;
		JSArguments& operator=(JSArguments&& arguments) = delete;

		/* maybe assertion failed: list_empty(&rt->gc_obj_list), when use 'arguments[0][5].value<bool>()' but throw exception */
		/* Fix : Enable the MSVC compiler's exception handling(/EHsc) for the C++ project */
		quickjs::JSArgumentRef operator[](int idx) const {
			if (std::abs(idx) >= size()) {
				return quickjs::JSArgumentRef(idx);
			}
			if (idx >= 0) {
				return quickjs::JSArgumentRef(ctx_, JS_DupValue(ctx_, argv_[idx]), idx);
			} else {
				return quickjs::JSArgumentRef(ctx_, JS_DupValue(ctx_, argv_[argc_ + idx]), argc_ + idx);
			}
		}

		/**
		 * @brief Explicit conversion operator to bool.
		 *
		 * Allows the object to be used in boolean contexts (e.g., if, while)
		 * without implicit conversions.
		 *
		 * @return true if the object is considered "valid"; false otherwise.
		 *         - If the group is registered, returns true when argumentsGroupId != -1.
		 *         - Otherwise, returns true when argc_ > 0.
		 *
		 * @note This operator is marked noexcept and does not throw exceptions.
		 */
		explicit operator bool() const noexcept {
			if (isGroupRegisted) {
				return argumentsGroupId != -1;
			}
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
		template<size_t GroupId, typename... TagTypes>
		std::enable_if_t<std::conjunction<std::is_same<TagTypes, JSType>...>::value, void>
			registerGroup(TagTypes... require_tags) noexcept {
			isGroupRegisted = true;
			constexpr size_t require_size = sizeof...(TagTypes);
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
			isGroupRegisted = true;
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
		template<typename... TagTypes>
		std::enable_if_t<std::conjunction<std::is_same<TagTypes, JSType>...>::value, bool>
			IsArgsOf(TagTypes... require_types) noexcept {
			constexpr size_t require_size = sizeof...(TagTypes);
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
		template<typename... TagTypes>
		std::enable_if_t<std::conjunction<std::is_same<TagTypes, JSType>...>::value, bool>
			IsArgsOf(size_t s_idx, TagTypes... require_types) noexcept {
			constexpr size_t require_size = sizeof...(TagTypes);
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
		template<typename... TagTypes>
		std::enable_if_t<std::conjunction<std::is_same<TagTypes, JSType>...>::value, bool>
			ExpectArgsOf(TagTypes... require_types) noexcept {
			constexpr size_t require_size = sizeof...(TagTypes);
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
		template<typename... TagTypes>
		std::enable_if_t<std::conjunction<std::is_same<TagTypes, JSType>...>::value, bool>
			ExpectArgsOf(size_t s_idx, TagTypes... require_types) noexcept {
			constexpr size_t require_size = sizeof...(TagTypes);
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
		bool isGroupRegisted;
	};

	/**
	 * @brief Converts a JSValueRef to a std::string.like exception/undefined or others
	 * @param ref The JSValueRef to convert.
	 * @return The string representation, or an empty string on failure.
	 */
	inline std::string to_string(const quickjs::JSValueRef& ref) {
		const char* cstr;
		if (ref.is<JSType::exception>()) {
			JSValue err = JS_GetException(ref.context);
			cstr = JS_ToCString(ref.context, err);
			JS_FreeValue(ref.context, err);
		} else {
			cstr = JS_ToCString(ref.context, ref.jsvalue);
		}
		if (!cstr)
			return std::string();
		std::string result(cstr);
		JS_FreeCString(ref.context, cstr);
		return result;
	}

	/**
	 * @brief Converts a C++ value to a QuickJS value.
	 *
	 * Dispatches to the matching @c new_value_impl overload via SFINAE.
	 *
	 * @tparam T   Input type (decayed before dispatch).
	 * @param ctx  QuickJS context.
	 * @param val  Value to convert (const ref, no copy).
	 * @return Owning @c JSValueRef of the new QuickJS value.
	 * @throws std::exception on unsupported or failed conversion.
	 */
	template<typename T>
	inline quickjs::JSValueRef toJSValue(JSContext* ctx, const T& val) {
		return type_traits::detail::new_value_impl<T>(ctx, val);
	}

	template <typename Signature, typename Fn>
	inline std::enable_if_t <type_traits::function::has_signature<std::decay_t<Fn>>::value &&
		std::is_same_v<Signature,typename type_traits::function::function_traits<std::decay_t<Fn>>::signature>, quickjs::JSValueRef>
		toJSValue(JSContext* ctx, const Fn& fn) {
		return type_traits::detail::new_value_impl<std::function<Signature>>(ctx, fn);
	}

	template <typename Fn>
	inline std::enable_if_t<type_traits::function::has_signature<std::decay_t<Fn>>::value, quickjs::JSValueRef>
		toJSValue(JSContext* ctx, Fn&& fn) {
		using fn_t = std::function<typename type_traits::function::function_traits<std::decay_t<Fn>>::signature>;
		return type_traits::detail::new_value_impl<fn_t>(ctx, fn_t(std::forward<Fn>(fn)));
	}
}
#endif //__cplusplus

#endif // QUICKJSPP_H