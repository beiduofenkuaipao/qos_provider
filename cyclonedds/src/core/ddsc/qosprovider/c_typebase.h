#ifndef C_TYPEBASE_H
#define C_TYPEBASE_H

#if defined (__cplusplus)
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <limits.h>
#include <sys/types.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include "dds/ddsrt/avl.h"
#include "dds/ddsrt/atomics.h"
#include "dds/ddsrt/atomics/gcc.h"
#include "dds/ddsrt/heap.h"
#include "dds/ddsrt/string.h"


#ifndef OS_API_EXPORT
#define OS_API_EXPORT
#endif

#ifndef OS_API_IMPORT
#define OS_API_IMPORT
#endif


#ifdef OSPL_BUILD_CORE
#define OS_API OS_API_EXPORT
#else
#define OS_API OS_API_IMPORT
#endif

#define _STATISTICS_
#define _READ_BEGIN_(t)
#define _READ_END_(t)
#define _ACCESS_BEGIN_(t)
#define _ACCESS_END_(t)
#define _CHECK_CONSISTENCY_(t,n)

#define TRUE                1
#define FALSE               0


typedef unsigned long int        c_address;
typedef size_t                   c_size;
typedef void                    *c_object;
typedef void                    *c_voidp;
typedef uint8_t                  c_octet;
typedef int16_t                  c_short;
typedef int32_t                  c_long;
typedef uint16_t                 c_ushort;
typedef uint32_t                c_ulong;
typedef char                    c_char;
typedef int16_t                 c_wchar;
typedef float                   c_float;
typedef double                  c_double;
typedef uint8_t                 c_bool;
typedef int64_t                 c_longlong;
typedef uint64_t                c_ulonglong;
typedef char                    *c_string;
typedef int16_t                 *c_wstring;
typedef c_object                *c_array;
typedef c_object                *c_sequence;



#define C_STRUCT(name)  struct name##_s
#define C_EXTENDS(type) C_STRUCT(type) _parent
#define C_CLASS(name)   typedef C_STRUCT(name) *name
#define C_ADDRESS(ptr)  ((c_address)(ptr))
#define C_SIZEOF(name)  sizeof(C_STRUCT(name))

#define OS_STRUCT(name)  struct name##_s
#define OS_EXTENDS(type) OS_STRUCT(type) _parent
#define OS_CLASS(name)   typedef OS_STRUCT(name) *name
#define OS_SIZEOF(name)  sizeof(OS_STRUCT(name))
#define OS_SUPER(obj)    (&((obj)->_parent))

#define C_DISPLACE(ptr,offset) ((c_object)(C_ADDRESS(ptr)+C_ADDRESS(offset)))
#define purify_memset(v,b,s) memset(v,b,s)

#define C_REFGET(ptr,offset) \
        (c_object(*((c_object *)C_DISPLACE((ptr),(offset)))))















typedef enum c_metaKind {
    M_UNDEFINED,
    M_ANNOTATION, M_ATTRIBUTE, M_CLASS, M_COLLECTION, M_CONSTANT, M_CONSTOPERAND,
    M_ENUMERATION, M_EXCEPTION, M_EXPRESSION, M_INTERFACE,
    M_LITERAL, M_MEMBER, M_MODULE, M_OPERATION, M_PARAMETER,
    M_PRIMITIVE, M_RELATION, M_BASE, M_STRUCTURE, M_TYPEDEF,
    M_UNION, M_UNIONCASE,
    M_COUNT
} c_metaKind;


typedef enum cf_kindEnum {
    CF_NODE,
    CF_ATTRIBUTE,
    CF_ELEMENT,
    CF_DATA,
    CF_COUNT
} cf_kind;

typedef enum ut_collectionType {
    UT_LIST,
    UT_SET,
    UT_BAG,
    UT_TABLE
}ut_collectionType;

typedef enum os_equality {
    OS_PL = -4,    /* Partial less (structure)    */
    OS_EL = -3,    /* Less or Equal (set)         */
    OS_LE = -2,    /* Less or Equal               */
    OS_LT = -1,    /* Less                        */
    OS_EQ = 0,     /* Equal                       */
    OS_GT = 1,     /* Greater                     */
    OS_GE = 2,     /* Greater or Equal            */
    OS_EG = 3,     /* Greater or Equal (set)      */
    OS_PG = 4,     /* Partial greater (structure) */
    OS_PE = 10,    /* Partial Equal               */
    OS_NE = 20,    /* Not equal                   */
    OS_ER = 99     /* Error: equality undefined   */
} os_equality;

typedef enum c_valueKind {
    V_UNDEFINED,
    V_ADDRESS, V_BOOLEAN, V_OCTET,
    V_SHORT,   V_LONG,   V_LONGLONG,
    V_USHORT,  V_ULONG,  V_ULONGLONG,
    V_FLOAT,   V_DOUBLE,
    V_CHAR,    V_STRING,
    V_WCHAR,   V_WSTRING,
    V_FIXED,   V_OBJECT,
    V_VOIDP,
    V_COUNT
} c_valueKind;

typedef enum c_collKind {
    OSPL_C_UNDEFINED,
    OSPL_C_LIST, OSPL_C_ARRAY, OSPL_C_BAG, OSPL_C_SET, OSPL_C_MAP, OSPL_C_DICTIONARY,
    OSPL_C_SEQUENCE, OSPL_C_STRING, OSPL_C_WSTRING, OSPL_C_QUERY, OSPL_C_SCOPE,
    OSPL_C_COUNT
} c_collKind;

typedef enum c_primKind {
    P_UNDEFINED,
    P_ADDRESS, P_BOOLEAN, P_CHAR, P_WCHAR, P_OCTET,
    P_SHORT, P_USHORT, P_LONG, P_ULONG, P_LONGLONG, P_ULONGLONG,
    P_FLOAT, P_DOUBLE, P_VOIDP,
    P_MUTEX, P_LOCK, P_COND,
    P_PA_UINT32, P_PA_UINTPTR, P_PA_VOIDP,
    P_COUNT
} c_primKind;

typedef enum c_equality {
    C_PL = -4,    /* Partial less (structure)    */
    C_EL = -3,    /* Less or Equal (set)         */
    C_LE = -2,    /* Less or Equal               */
    C_LT = -1,    /* Less                        */
    C_EQ = 0,     /* Equal                       */
    C_GT = 1,     /* Greater                     */
    C_GE = 2,     /* Greater or Equal            */
    C_EG = 3,     /* Greater or Equal (set)      */
    C_PG = 4,     /* Partial greater (structure) */
    C_PE = 10,    /* Partial Equal               */
    C_NE = 20,    /* Not equal                   */
    C_ER = 99     /* Error: equality undefined   */
} c_equality;

typedef enum c_qKind {
    CQ_UNDEFINED,
    CQ_FIELD,CQ_CONST,CQ_TYPE,
    CQ_AND, CQ_OR, CQ_NOT,
    CQ_EQ, CQ_NE, CQ_LT, CQ_LE, CQ_GT, CQ_GE,
    CQ_LIKE, CQ_CALLBACK, CQ_COUNT
} c_qKind;

typedef enum c_qBoundKind {
    B_UNDEFINED,
    B_INCLUDE,
    B_EXCLUDE
} c_qBoundKind;

typedef enum c_operator {
    O_AND,  O_OR,
    O_LE,   O_LT,    O_EQ,  O_GT,   O_GE,  O_NE,
    O_ADD,  O_SUB,   O_MUL, O_DIV,  O_MOD, O_POW,
    O_LEFT, O_RIGHT, O_LOR, O_LXOR, O_LAND
} c_operator;

typedef enum {
    QP_RESULT_OK,
    QP_RESULT_NO_DATA,
    QP_RESULT_OUT_OF_MEMORY,
    QP_RESULT_PARSE_ERROR,
    QP_RESULT_ILL_PARAM,
    QP_RESULT_UNKNOWN_ELEMENT,
    QP_RESULT_UNEXPECTED_ELEMENT,
    QP_RESULT_UNKNOWN_ARGUMENT,
    QP_RESULT_ILLEGAL_VALUE,
    QP_RESULT_NOT_IMPLEMENTED
} cmn_qpResult;

typedef enum os_boolean {
    OS_FALSE = 0,
    OS_TRUE = 1
} os_boolean;

typedef enum c_exprKind {
    E_UNDEFINED,
    E_OR, E_XOR, E_AND,
    E_SHIFTRIGHT, E_SHIFTLEFT,
    E_PLUS, E_MINUS, E_MUL, E_DIV,
    E_MOD, E_NOT,
    E_COUNT
} c_exprKind;

typedef enum c_state {
    ST_ERROR,
    ST_START,
    ST_IDENT,
    ST_REFID,
    ST_SCOPE,
    ST_REF,
    ST_END
} c_state;

typedef enum c_token {
    TK_ERROR,
    TK_IDENT,
    TK_DOT,
    TK_REF,
    TK_SLASH,
    TK_SCOPE,
    TK_COMMA,
    TK_END
} c_token;

typedef enum c_metaEquality {
    E_UNEQUAL, E_EQUAL
} c_metaEquality;

typedef enum c_direction {
    D_UNDEFINED,
    D_IN, D_OUT, D_INOUT,
    D_COUNT
} c_direction;

typedef enum c_result {
    S_ACCEPTED, S_REDECLARED, S_ILLEGALOBJECT, S_REDEFINED, S_ILLEGALRECURSION, S_COUNT
} c_result;














typedef struct c_value {
    c_valueKind kind;
    union {
        c_address    Address;
        c_short      Short;
        c_long       Long;
        c_longlong   LongLong;
        c_octet      Octet;
        c_ushort     UShort;
        c_ulong      ULong;
        c_ulonglong  ULongLong;
        c_char       Char;
        c_wchar      WChar;
        c_float      Float;
        c_double     Double;
        c_string     String;
        c_wstring    WString;
        c_string     Fixed;
        c_bool       Boolean;
        c_voidp      Object;
        c_voidp      Voidp;
    } is;
} c_value;














typedef void(*cmn_qpCopyOut)(void *from, void *to);
typedef os_equality (*ut_compareElementsFunc) (void *o1, void *o2, void *args);
typedef void (*ut_freeElementFunc)(void *o, void *arg);







c_equality c_valueCompare(const c_value v1, const c_value v2);
c_value c_boolValue (const c_bool value);
c_value c_octetValue (const c_octet value);
c_value c_shortValue (const c_short value);
c_value c_longValue (const c_long value);
c_value c_longlongValue (const c_longlong value);
c_value c_ushortValue (const c_ushort value);
c_value c_ulongValue (const c_ulong value);
c_value c_ulonglongValue (const c_ulonglong value);
c_value c_charValue (const c_char value);
c_value c_wcharValue (const c_wchar value);
c_value c_valueCalculate(const c_value v1,const c_value v2,const c_operator o);
c_valueKind c_normalizedKind (c_value v1,c_value v2);
c_value c_valueCast (const c_value v,c_valueKind kind);
c_value c_valueSL (c_value v, c_value a);
c_value c_valueSR (c_value v,c_value a);
c_value c_valueLOR (c_value v1,c_value v2);
c_value c_valueLXOR (c_value v1,c_value v2);
c_value c_valueLAND (c_value v1,c_value v2);
c_value c_valueMUL (c_value v1,c_value v2);
c_value c_valueDIV (c_value v1,c_value v2);
c_value c_valueMOD (c_value v1,c_value v2);
c_value c_valuePOW (c_value v1,c_value v2);
c_value c_valueSL (c_value v,c_value a);
c_value c_valueSR (c_value v,c_value a);
c_value c_valueADD (c_value v1,c_value v2);
c_value c_valueSUB (c_value v1,c_value v2);
c_value c_valueFreeRef(c_value v);
c_value c_valueKeepRef(c_value v);
c_value c_objectValue (const c_voidp object);
c_value c_valueStringMatch (const c_value patternValue,const c_value stringValue);
int patmatch (const char *pat, const char *str);
c_value c_stringValue (const c_string value);




#if defined (__cplusplus)
}
#endif

#endif /* C_TYPEBASE_H */