#ifndef __CCPY_UTIL_MACRO__
#define __CCPY_UTIL_MACRO__

#define DECL_REFL_ENUM(NAME, LIST) \
  enum class NAME { \
    LIST(DECL_REFL_ENUM__VAR) \
  }; \
  extern const char *(NAME ## Map)[0 LIST(DECL_REFL_ENUM__CNT)] // No semicolon
#define DECL_REFL_ENUM__VAR(VAR, STR) VAR,
#define DECL_REFL_ENUM__CNT(VAR, STR) + 1
#define IMPL_REFL_ENUM(NAME, LIST) \
  const char *(NAME ## Map)[0 LIST(DECL_REFL_ENUM__CNT)] = { \
    LIST(IMPL_REFL_ENUM__STR) \
  } // No semicolon
#define IMPL_REFL_ENUM__STR(VAR, STR) STR,

// Shortcut for declare `operator<<` for `put` and `operator>>` for `get`.
#define DECL_OP_GET \
  template<typename _OP_GET> \
  auto &operator>>(const _OP_GET &x) { x = this->get(); return *this; }
#define DECL_OP_PUT \
  template<typename _OP_PUT> \
  auto &operator<<(_OP_PUT x) { this->put(x); return *this; }

#endif // __CCPY_UTIL_MACRO__
