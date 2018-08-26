#ifndef __CCPY_RUNTIME_OBJECT__
#define __CCPY_RUNTIME_OBJECT__

#include <memory>
#include <unordered_map>
#include <vector>
#include "../util/adt.h"
#include "../util/macro.h"
#include "../util/types.h"

namespace ccpy::runtime {

struct Object;

using ObjectRef = std::shared_ptr<Object>;
using ObjectPlace = std::shared_ptr<ObjectRef>;

using ObjectTuple = std::vector<ObjectRef>;
using ObjectPool = std::vector<ObjectPlace>;

using Dict = std::unordered_map<Str, ObjectRef>;

#define PRIMITIVE_OBJ_LIST(F) \
  F(ObjBool, { bool value; }) \
  F(ObjInt, { Integer value; }) \
  F(ObjStr, { Str value; }) \
  F(ObjTuple, { ObjectTuple elems; }) \
  F(ObjDict, { Dict value; }) \
  F(ObjClosure, { \
    std::size_t closure_id; \
    std::shared_ptr<ObjectPool> captured; \
    ObjectRef defaults; \
  }) \
  F(ObjObject, { \
    optional<ObjectRef> base, type; \
  }) \
  F(ObjNull, {}) \

DECL_TAGGED_UNION(PrimitiveObj, PRIMITIVE_OBJ_LIST)

struct Object {
  PrimitiveObj primitive;
  Dict attrs;
};


} // namespace ccpy::runtime

#endif // __CCPY_RUNTIME_OBJECT__
