#ifndef __CCPY_UTIL_MACRO__
#define __CCPY_UTIL_MACRO__

#define DECL_REFL_ENUM(NAME, LIST) \
  enum class NAME { \
    LIST(DECL_REFL_ENUM__VAR) \
  }; \
  extern const char *(NAME ## Map)[0 LIST(DECL_REFL_ENUM__CNT)];
#define DECL_REFL_ENUM__VAR(VAR, STR) VAR,
#define DECL_REFL_ENUM__CNT(VAR, STR) + 1
#define IMPL_REFL_ENUM(NAME, LIST) \
  const char *(NAME ## Map)[0 LIST(DECL_REFL_ENUM__CNT)] = { \
    LIST(IMPL_REFL_ENUM__STR) \
  };
#define IMPL_REFL_ENUM__STR(VAR, STR) STR,

// Shortcut for declare `operator<<` for `put` and `operator>>` for `get`.
#define DECL_OP_GET \
  template<typename _OP_GET> \
  auto &operator>>(const _OP_GET &x) { x = this->get(); return *this; }
#define DECL_OP_PUT \
  template<typename _OP_PUT> \
  auto &operator<<(_OP_PUT &&x) { \
    this->put(std::forward<_OP_PUT>(x)); \
    return *this; \
  }

#define DECL_TAGGED_UNION(NAME, LIST) \
  LIST(DECL_TAGGED_UNION__FORWARD) \
  using NAME = tagged_union<TAIL((int LIST(DECL_TAGGED_UNION__VAR)))>; \
  LIST(DECL_TAGGED_UNION__STRUCT)
#define DECL_TAGGED_UNION__FORWARD(STRUCT, ...) struct STRUCT;
#define DECL_TAGGED_UNION__VAR(STRUCT, ...) , STRUCT
#define DECL_TAGGED_UNION__STRUCT(STRUCT, ...) struct STRUCT __VA_ARGS__;
                                  // Prevent split `,` inside braces ^
#define TAIL(a) TAIL_1 a
#define TAIL_1(_, ...) __VA_ARGS__

#endif // __CCPY_UTIL_MACRO__
