#ifndef C_BASE_H
#define C_BASE_H

#if defined (__cplusplus)
extern "C" {
#endif

#include "c_typebase.h"
#include "c_mmbase.h"
#include "fun_base.h"

#if __GNUC__
#define c_likely(x)       __builtin_expect(!!(x),1)
#define c_unlikely(x)     __builtin_expect(!!(x),0)
#else
#define c_likely(x) (x)
#define c_unlikely(x) (x)
#endif

#define CONFIDENCE                      (0x504F5448)
#define REFCOUNT_MASK                   0x1ffffffu
#define REFCOUNT_FLAG_TRACE             0x4000000u
#define REFCOUNT_FLAG_ATOMIC            0x2000000u
#define REFCOUNT_FLAG_CLAMP             0x1000000u
#define REFCOUNT_FLAG_GARBAGE           0x10000000u
#define REFCOUNT_FLAG_TRACETYPE         0x8000000u
#define C_MMTRACKOBJECT_CODE_MIN        2
#define MIN_DB_SIZE                     (150000)
#define MAXREFCOUNT                     (50000)
#define MEMSIZE(size)   ((size)+HEADERSIZE)
#define ARRAYMEMSIZE(size)   ((size)+ARRAYHEADERSIZE)
#define c_oid(o)    ((c_object)(C_ADDRESS(o) + HEADERSIZE))

#define OS_UNUSED_ARG(a) (void) (a)

#define ResolveType(s,t) c_type(c_metaResolve(c_metaObject(s),t))

#define C_META_OFFSET_(t,f) \
        ((size_t)(&((t)0)->f))

#define C_META_ATTRIBUTE_(c,o,n,t) \
        c_metaAttributeNew(o,#n,t,C_META_OFFSET_(c,n))


c_mm c_baseMM(c_base base);
c_type c_getType(c_object object);


#define c__getBase(o) c_getBase(o)
#define c__getType(o) c_getType(o)
#define MM(o)     (c_baseMM(c__getType(o)->base))
c_bool _c_freeReferences(c_metaObject metaObject, c_object o, c_freelist freelist);
c_bool _freeReference (c_voidp *p,c_type type,c_freelist freelist);
c_base c_create (const c_char *name,void *address,c_size size,c_size threshold);
c_string c_stringNew(c_base base,const c_char *str);
c_string c_stringMalloc(c_base base,c_size length);
c_string c__stringMalloc(c_base base,c_size length,c_bool check);
c_string c_stringNew_s(c_base base,const c_char *str);
c_string c_stringMalloc_s(c_base base,c_size length);
c_type c_newMetaClass(c_base base,const c_char *name,size_t size,size_t alignment);
c_object c_new_s(c_type type);
c_type c_resolve (c_base base,const c_char *typeName);


#define c_freeReferences(m,o,l) ((m && o)? _c_freeReferences(m,o,l):TRUE)
#define freeReference(p,t,l) (p?_freeReference(p,t,l):TRUE)

#define c_qRangeStartValue(_this) (_this->start)
#define c_qRangeEndValue(_this)   (_this->end)

OS_API c_type c_voidp_t     (c_base _this);
OS_API c_type c_address_t   (c_base _this);
OS_API c_type c_object_t    (c_base _this);
OS_API c_type c_bool_t      (c_base _this);
OS_API c_type c_octet_t     (c_base _this);
OS_API c_type c_char_t      (c_base _this);
OS_API c_type c_short_t     (c_base _this);
OS_API c_type c_long_t      (c_base _this);
OS_API c_type c_longlong_t  (c_base _this);
OS_API c_type c_uchar_t     (c_base _this);
OS_API c_type c_ushort_t    (c_base _this);
OS_API c_type c_ulong_t     (c_base _this);
OS_API c_type c_ulonglong_t (c_base _this);
OS_API c_type c_float_t     (c_base _this);
OS_API c_type c_double_t    (c_base _this);
OS_API c_type c_string_t    (c_base _this);
OS_API c_type c_wchar_t     (c_base _this);
OS_API c_type c_wstring_t   (c_base _this);
OS_API c_type c_type_t      (c_base _this);
OS_API c_type c_valueKind_t (c_base _this);

OS_API void c_baseSerLock   (c_base _this);
OS_API void c_baseSerUnlock (c_base _this);

OS_API c_type c_member_t    (c_base _this);
c_type c_constant_t  (c_base _this);
OS_API c_type c_literal_t   (c_base _this);
OS_API c_type c_property_t  (c_base _this);
OS_API c_type c_unionCase_t (c_base _this);

#define _TYPE_CACHE_(_type) \
        c_type \
        _type##_t (c_base _this) \
        { \
            return _this->baseCache.typeCache._type##_t; \
        }








void c_freelistInit(c_freelist freelist);
c_object c_freelistTake(c_freelist freelist);
c_ulong c_arraySize(c_array _this);
void c_freelistInsert(c_freelist freelist, c_object object);
c_valueKind c_metaValueKind(c_metaObject o);
void c_destroy (c_base _this);
void deleteGarbage(c_base base);
void freeBindings (void *binding, void *arg);
void c_mmDestroy(c_mm mm);
c_object c_keep (c_object object);
void collectScopeGarbage(c_metaObject o,c_voidp trashcan);
c_bool collectGarbage(c_object o,c_voidp trashcan);
c_object ospl_c_bind (c_object object,const c_char *name);
c_object c_newBaseArrayObject (c_collectionType arrayType,c_ulong size);
c_object c__newBaseArrayObjectCommon (c_collectionType arrayType,c_ulong size,c_bool check);
void c_baseInit (c_base base,c_mm mm);
c_object c_new (c_type type);
size_t c_typeSize(c_type type);
c_object c_newBaseArrayObject_s (c_collectionType arrayType,c_ulong size);
c_bool c_imageValue(const char *image,c_value *imgValue,c_type imgType);
c_object initClass(c_object o,c_type baseClass);
c_ulong c_sequenceSize(c_sequence _this);
c_object initAnnotation(c_object o);




#define c_newArray_s(t,s) \
        (assert(c_collectionTypeKind(t) == OSPL_C_ARRAY), c_newBaseArrayObject_s(c_collectionType(t),s))


#define c_newArray(t,s) \
        (assert(c_collectionTypeKind(t) == OSPL_C_ARRAY), c_newBaseArrayObject(c_collectionType(t),s))

#if defined (__cplusplus)
}
#endif

#endif