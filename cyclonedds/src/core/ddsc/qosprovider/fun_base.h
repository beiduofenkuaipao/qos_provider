#ifndef C_ITERATOR_H
#define C_ITERATOR_H

#if defined (__cplusplus)
extern "C" {
#endif

#include "c_typebase.h"
#include "c_mmbase.h"
#include "c_base.h"


/* Search Type Qualifiers */
#define CQ_ATTRIBUTE       (1u << (M_ATTRIBUTE-1))
#define CQ_ANNOTATION      (1u << (M_ANNOTATION-1))
#define CQ_CLASS           (1u << (M_CLASS-1))
#define CQ_COLLECTION      (1u << (M_COLLECTION-1))
#define CQ_CONSTANT        (1u << (M_CONSTANT-1))
#define CQ_CONSTOPERAND    (1u << (M_CONSTOPERAND-1))
#define CQ_ENUMERATION     (1u << (M_ENUMERATION-1))
#define CQ_EXCEPTION       (1u << (M_EXCEPTION-1))
#define CQ_EXPRESSION      (1u << (M_EXPRESSION-1))
#define CQ_INTERFACE       (1u << (M_INTERFACE-1))
#define CQ_LITERAL         (1u << (M_LITERAL-1))
#define CQ_MEMBER          (1u << (M_MEMBER-1))
#define CQ_MODULE          (1u << (M_MODULE-1))
#define CQ_OPERATION       (1u << (M_OPERATION-1))
#define CQ_PARAMETER       (1u << (M_PARAMETER-1))
#define CQ_PRIMITIVE       (1u << (M_PRIMITIVE-1))
#define CQ_RELATION        (1u << (M_RELATION-1))
#define CQ_BASE            (1u << (M_BASE-1))
#define CQ_STRUCTURE       (1u << (M_STRUCTURE-1))
#define CQ_TYPEDEF         (1u << (M_TYPEDEF-1))
#define CQ_UNION           (1u << (M_UNION-1))
#define CQ_UNIONCASE       (1u << (M_UNIONCASE-1))

#define CQ_ALL\
        (CQ_ATTRIBUTE | CQ_CLASS | CQ_COLLECTION | CQ_CONSTANT | \
         CQ_CONSTOPERAND | CQ_ENUMERATION | CQ_EXCEPTION | \
         CQ_EXPRESSION | CQ_INTERFACE | CQ_LITERAL | CQ_MEMBER | \
         CQ_MODULE | CQ_OPERATION | CQ_PARAMETER | CQ_PRIMITIVE | \
         CQ_RELATION | CQ_BASE | CQ_STRUCTURE | CQ_TYPEDEF | CQ_UNION | \
         CQ_UNIONCASE | CQ_ANNOTATION)

#define CQ_METAOBJECTS \
        (CQ_TYPEDEF | CQ_COLLECTION | CQ_PRIMITIVE | CQ_ENUMERATION | \
         CQ_UNION | CQ_STRUCTURE | CQ_EXCEPTION | CQ_INTERFACE | \
         CQ_MODULE | CQ_CONSTANT | CQ_OPERATION | CQ_ATTRIBUTE | \
         CQ_RELATION | CQ_CLASS | CQ_ANNOTATION)

#define CQ_SPECIFIERS \
        (CQ_PARAMETER | CQ_MEMBER | CQ_UNIONCASE)

#define CQ_CASEINSENSITIVE (1U << 31) /* Search case insensitive, which is slower */
#define CQ_FIXEDSCOPE      (1U << 30) /* Only search in the provided namespace */

typedef FILE os_cfg_file;
typedef void  *c_scopeWalkActionArg;
typedef void (*c_scopeWalkAction) (c_metaObject o, c_scopeWalkActionArg actionArg);



typedef c_bool (*c_replaceCondition)
               (c_object original, c_object replacement, c_voidp arg);

typedef c_bool (*c_removeCondition)
               (c_object found, c_object requested, c_voidp arg);

typedef void *cf_nodeWalkActionArg;
typedef unsigned int (*cf_nodeWalkAction)(/*c_object, cf_nodeWalkActionArg*/);

typedef void    *c_iterActionArg;
typedef void   (*c_iterWalkAction) (void *o, c_iterActionArg arg);


#define c_qKind_t(b)      (b->baseCache.queryCache.c_qKind_t)
#define c_qBoundKind_t(b) (b->baseCache.queryCache.c_qBoundKind_t)
#define c_qConst_t(b)     (b->baseCache.queryCache.c_qConst_t)
#define c_qType_t(b)      (b->baseCache.queryCache.c_qType_t)
#define c_qVar_t(b)       (b->baseCache.queryCache.c_qVar_t)
#define c_qField_t(b)     (b->baseCache.queryCache.c_qField_t)
#define c_qFunc_t(b)      (b->baseCache.queryCache.c_qFunc_t)
#define c_qPred_t(b)      (b->baseCache.queryCache.c_qPred_t)
#define c_qKey_t(b)       (b->baseCache.queryCache.c_qKey_t)
#define c_qRange_t(b)     (b->baseCache.queryCache.c_qRange_t)
#define c_qExpr_t(b)      (b->baseCache.queryCache.c_qExpr_t)



c_bool c_typeIsRef (c_type type);
c_type c_typeActualType (c_type type);
c_type c_getMetaType(c_base base,c_metaKind kind);
c_base c_getBase(c_object object);
c_bool c_typeHasRef (c_type type);
void c_scopeDeinit(c_scope scope);
int c_bindingCompare(const void *va, const void *vb);
void c_bindingFreeWrapper (void *binding, void *mm);
void c_bindingFree (c_binding binding, c_mm mm);
void c_free (c_object object);
void c_mmTrackObject (struct c_mm_s *mm, const void *ptr, uint32_t code);
void c_mmFree (c_mm mm, void *memory);
void ut_tableFree(ut_table table);
void ut_tableFreeHelper (void *vnode, void *varg);
void *c_iterTakeFirst(c_iter iter);
c_iter c_iterInsert(c_iter iter, void *object);
c_iter c_iterNew(void *object);
void walkReferences(c_metaObject metaObject,c_object o,c_trashcan trashcan);
void collectReferenceGarbage(c_voidp *p,c_type type,c_trashcan trashcan);
void c_scopeClean(c_scope scope);
void c_clear ( c_collection c);
c_object c_take(c_collection c);
c_object c_queryTakeOne (C_STRUCT(c_query) *b,c_qPred q);
c_bool c_collectionIsType( c_collection c,c_collKind kind);
c_bool c_queryTake(C_STRUCT(c_query) *query,c_qPred q,c_action action,c_voidp arg);
c_bool queryReadAction (c_object o,c_voidp arg);
c_bool c_qPredEval ( c_qPred q,c_object o);
c_bool c_qKeyEval(c_qKey key,c_object o);
c_value c_qValue(c_qExpr q,c_object o);
void c_iterFree(c_iter iter);
c_bool readOne(c_object o, c_voidp arg);
c_iter c_iterAppend(c_iter iter, void *object);
void c_scopeWalk(c_scope scope,c_scopeWalkAction action,c_scopeWalkActionArg actionArg);
c_bool c_walk(c_collection c,c_action action,c_voidp actionArg);
c_bool c_tableWalk(c_table _this,c_action action,c_voidp actionArg);
c_bool tableReadTakeWalk (ddsrt_avl_tree_t *tree, c_bool (*action) (), void *actionarg);
int tableCmp (const void *va, const void *vb);
c_bool c_tableWalkAction (c_tableNode n,c_tableWalkActionArg arg);
c_object c_tableTakeOne(c_table _this,c_qPred p);
c_bool c_tableTake (c_table _this,c_qPred p,c_action action,c_voidp arg);
c_bool predMatches (c_qPred qp, c_object o);
c_bool tableTakeAction( c_tableNode n,tableTakeActionArg arg);
c_bool tableReadTakeActionNonMatchingKey (c_tableNode n,c_qPred query,c_ulong keyIndex);
c_bool tableTakeActionWalk (c_tableNode n, tableTakeActionArg arg);
void tableTakeActionDeleteDisposed (c_tableNode n, tableTakeActionArg arg);
c_object c_setTakeOne (c_set _this, c_qPred q);
c_object c_setReadOne (c_set _this, c_qPred q);
c_bool c_setRead(c_set _this,c_qPred q,c_action action,c_voidp arg);
int ptrCompare(const void *a, const void *b);
c_object c_bagTakeOne (c_bag _this,c_qPred q);
c_object c_bagReadOne (c_bag _this,c_qPred q);
c_bool c_bagRead(c_bag _this,c_qPred q,c_action action,c_voidp arg);
c_object c_bagRemove (c_bag _this,c_object o,c_removeCondition condition,c_voidp arg);
c_bool c_setWalk(c_set _this,c_action action,c_voidp actionArg);
c_bool c_bagWalk(c_bag _this,c_action action,c_voidp actionArg);
c_bool c_listWalk(c_list _this, c_action action, c_voidp actionArg);

c_object c_listTakeOne(c_list _this, c_qPred q);
c_bool c_listTake(c_list _this, c_qPred q, c_action action, c_voidp arg);
c_bool takeOne(c_object o, c_voidp arg);
c_bool c_setTake (c_set _this,c_qPred q,c_action action,c_voidp arg);
c_object c_setRemove (c_set _this,c_object o,c_removeCondition condition,c_voidp arg);
c_bool c_bagTake (c_bag _this,c_qPred q,c_action action,c_voidp arg);
c_value c_fieldValue(c_field field,c_object o);
c_value qExecute(c_qExpr e,c_object o);
c_bool c_queryRead(C_STRUCT(c_query) *query,c_qPred q,c_action action,c_voidp arg);
c_bool c_listRead(c_list _this, c_qPred q, c_action action, c_voidp arg);
c_bool c_tableRead (c_table _this,c_qPred q,c_action action,c_voidp arg);
c_bool tableReadAction(c_tableNode n,tableReadActionArg arg);
void tableReadTakeActionGetRange (c_qRange range,c_value *start,c_value **startRef,c_bool *startInclude,c_value *end,c_value **endRef,c_bool *endInclude);
c_bool tableReadTakeRangeWalk (ddsrt_avl_tree_t *tree,const c_value *start,int include_start,const c_value *end,int include_end,c_bool (*action) (),void *actionarg);
void *get_field_address (c_field field, c_object o);
c_bool tableTakeActionWalkRange (c_tableNode n, tableTakeActionArg arg, const c_value *startRef, c_bool startInclude, const c_value *endRef, c_bool endInclude);
void cf_elementFree (cf_element element);
void cf_nodeFree (cf_node node);
void cf_elementDeinit (cf_element element);
void cf_nodeListClear(cf_nodeList list);
void cf_nodeListFree(cf_nodeList list);
void cf_attributeDeinit (cf_attribute attribute);
void cf_nodeDeinit (cf_node node);
void cf_dataDeinit (cf_data data);
cf_element cf_elementNew (const c_char *tagName);
void cf_elementInit (cf_element element,const c_char *tagName);
void cf_nodeInit (cf_node node,cf_kind kind,const c_char *name);
cf_nodeList cf_nodeListNew();
cf_attribute cf_elementAddAttribute (cf_element element,cf_attribute attribute);
c_object cf_nodeListInsert(cf_nodeList list, cf_node o);
cf_attribute cf_attributeNew (const c_char *name,c_value value);
void cf_attributeInit (cf_attribute attribute,const c_char *name,c_value value);
cf_node cf_elementAddChild(cf_element element,cf_node child);
cf_data cf_dataNew (c_value value);
void cf_dataInit (cf_data data,c_value value);
c_iterIter c_iterIterGet(c_iter i);
void *c_iterNext(c_iterIter* iterator);
void os_cfgRelease(os_cfg_handle *cfg);
os_cfg_handle *os_cfgRead(const char *uri);
os_cfg_handle *os_fileReadURI(const char *uri);
os_cfg_file *os_fileOpenURI(const char *uri);
cf_kind cf_nodeKind (cf_node node);
c_value cf_dataValue(cf_data data);
c_string c_fieldName (c_field _this);
ut_table ut_tableNew(const ut_compareElementsFunc cmpFunc,void *arg,const ut_freeElementFunc freeKey,void *freeKeyArg,const ut_freeElementFunc freeValue,void *freeValueArg);
void ut_collectionInit(ut_collection c,ut_collectionType type,const ut_compareElementsFunc cmpFunc,void *cmpArg,const ut_freeElementFunc freeValue,void *freeValueArg);
void u__serviceInitialise(void);
c_metaObject c_metaDeclare(c_metaObject scope,const c_char *name,c_metaKind kind);
c_baseObject c_metaFindByName (c_metaObject scope,const char *name,c_ulong metaFilter);
c_baseObject c_metaFindByComp (c_metaObject scope,const char *name,c_ulong metaFilter);
c_bool c_isOneOf (c_char c,const c_char *symbolList);
c_char * c_skipSpaces (const c_char *str);
c_bool c_isLetter (c_char c);
c_bool c_isDigit (c_char c);
c_bool c_getInstantiationType(const c_char **str);
void c_metaTypeInit(c_object o,size_t size,size_t alignment);
void c_scopeInit(c_scope s);
c_metaObject c_metaBind(c_metaObject scope,const c_char *name,c_metaObject object);
c_metaObject c__metaBindCommon(c_metaObject scope,const c_char *name,c_metaObject object,c_bool check);
c_metaObject metaScopeInsert(c_metaObject scope,c_metaObject object);
c_metaObject metaScopeInsertEnum(c_metaObject scope,c_metaObject object);
c_scope metaClaim(c_metaObject scope);
c_metaObject c_scopeInsert(c_scope scope,c_metaObject object);
c_binding c_bindingNew(c_scope scope,c_metaObject object);
c_bool c_isFinal(c_metaObject o);
void c_metaCopy(c_metaObject s,c_metaObject d);
void metaScopeWalk(c_metaObject scope,c_scopeWalkAction action,c_scopeWalkActionArg actionArg);
void metaRelease(c_metaObject scope);
void copyScopeObjectScopeWalkAction(c_metaObject o,c_scopeWalkActionArg arg /* c_metaObject */);
void copyScopeObject(c_metaObject o,c_scopeWalkActionArg scope);
void c_typeCopy(c_type s,c_type d);
c_object c_metaDefine(c_metaObject scope,c_metaKind kind);
c_object c__metaDefineCommon(c_metaObject scope,c_metaKind kind,c_bool check);
c_scope c_scopeNew_s(c_base base);
c_scope c__scopeNewCommon(c_base base,c_bool check);
c_metaObject c_metaResolve (c_metaObject scope,const char *name);
c_scope c_scopeNew(c_base base);
c_result c_metaFinalize(c_metaObject o);
c_result c__metaFinalize(c_metaObject o,c_bool normalize);
void c_typeInit(c_metaObject o,size_t alignment,size_t size);
size_t alignSize(size_t size,size_t alignment);
size_t propertyAlignment(c_type type);
c_object c_metaDefine_s(c_metaObject scope,c_metaKind kind);
c_metaObject c_metaBind_s(c_metaObject scope,const c_char *name,c_metaObject object);
void c_metaAttributeNew(c_metaObject scope,const c_char *name,c_type type,size_t offset);
c_char * c_metaScopedName(c_metaObject o);
size_t c_metaNameLength(c_metaObject o);
c_string c_metaName(c_metaObject o);
size_t propertySize(c_type type);
void c_collectionInit (c_base base);
c_array c_arrayNew(c_type subType,c_ulong length);
c_array c__arrayNewCommon(c_type subType,c_ulong length,c_bool check);
c_type c_metaArrayTypeNew_s(c_metaObject scope,const c_char *name,c_type subType,c_ulong maxSize);
c_type c_metaArrayTypeNew(c_metaObject scope,const c_char *name,c_type subType,c_ulong maxSize);
c_constant c_metaDeclareEnumElement(c_metaObject scope,const c_char *name);
c_long c_compareProperty(const void *ptr1,const void *ptr2);
c_baseObject _metaResolveName(c_metaObject scope,const char *name,c_ulong metaFilter);
c_metaEquality _metaNameCompare (c_baseObject baseObject,const char *name,c_ulong metaFilter);
c_metaObject metaScopeLookup(c_metaObject scope,const c_char *name,c_ulong metaFilter);
c_baseObject c_scopeResolve(c_scope scope,const char *name,c_ulong metaFilter);
c_metaEquality c_metaNameCompare (c_baseObject baseObject,const char *key,c_ulong metaFilter);
c_metaEquality c_metaCompare(c_metaObject object,c_metaObject obj);
os_equality comparePointers(void *o1,void *o2,void *args);
c_metaEquality _c_metaCompare (c_metaObject object,c_metaObject obj,ut_collection adm);
void * ut_get(ut_collection c,void *o);
c_ulong metaScopeCount(c_metaObject scope);
c_ulong c_scopeCount(c_scope scope);
void getPropertiesScopeWalkAction(c_metaObject o,c_scopeWalkActionArg arg);
c_literal c_enumValue (c_enumeration e,const c_char *label);
c_literal c_operandValue(c_operand operand);
c_literal c_expressionValue(c_expression expr);
int32_t ut_tableInsert(ut_table t,void *key,void *value);
ut_tableNode ut_newTableNode(void *key,void *value);
void initInterface(c_object o);
c_unionCase c_unionCaseNew (c_metaObject scope,const c_char *name,c_type type,c_iter labels);
c_ulong c_iterLength(c_iter iter);
void c_iterArray(c_iter iter, void *ar[]);
c_metaObject c_metaResolveType (c_metaObject scope,const char *name);
void c_cloneIn (c_type type,const void *data,c_voidp *dest);
c_bool c__cloneReferences (c_type module,const void *data,c_voidp dest);
c_bool _cloneReference (c_type type,const void *data,c_voidp dest);
c_type c_field_t (c_base _this);
c_field c_fieldConcat (c_field head,c_field tail);
c_iter cf_elementGetAttributes(cf_element element);
unsigned int
attributesToIterator(
    c_object o,
    c_iter *c);
c_bool
cf_nodeListWalk(
    cf_nodeList list,
    cf_nodeWalkAction action,
    cf_nodeWalkActionArg arg);

c_iter
cf_elementGetChilds(
    cf_element element);

unsigned int
childsToIterator(
    c_object o,
    c_iter *c);

c_field
c_fieldNew (
    c_type type,
    const c_char *fieldName);


void c_iterWalk(c_iter iter, c_iterWalkAction action, c_iterActionArg actionArg);



const c_char *
cf_nodeGetName (
    cf_node node);



c_bool
c_objectIsType(
    c_baseObject o);

c_type
c_fieldType (
    c_field _this);

c_metaObject
c_scopeLookup(
    c_scope scope,
    const c_char *name,
    c_ulong metaFilter);

c_bool
c_scopeExists (
    c_scope scope,
    const c_char *name);

int
os_sprintf(
    char *s,
    const char *format,
    ...);


int
os_strncasecmp(
    const char *s1,
    const char *s2,
    size_t n);

long long
os_strtoll(
    const char *str,
    char **endptr,
    int32_t base);


unsigned long long
os__strtoull__ (
    const char *str,
    char **endptr,
    int32_t base,
    unsigned long long max);

int
os_todigit (
    const int chr);

unsigned long long
os_strtoull (
    const char *str,
    char **endptr,
    int32_t base);

float
os_strtof(const char *nptr, char **endptr);

unsigned long long
os_strtoull (
    const char *str,
    char **endptr,
    int32_t base);


double
os_strtod(const char *nptr, char **endptr);


char *
os_str_trim (
    const char *str,
    const char *trim);

char *
os_strchrs (
    const char *str,
    const char *chrs,
    os_boolean inc);


char *
os_strrchrs (
    const char *str,
    const char *chrs,
    os_boolean inc);


char *
os_strncpy(
    char *s1,
    const char *s2,
    size_t num);


int
os_strcasecmp(
    const char *s1,
    const char *s2);

c_iter
c_splitString(
    const c_char *str,
    const c_char *delimiters);


c_char *
c_skipUntil (
    const c_char *str,
    const c_char *symbolList);

void
c_fieldAssign (
    c_field field,
    c_object o,
    c_value v);

char
os_lcNumericGet(void);

void
c_fieldInit (
    c_base base);


void
c_querybaseInit(
    c_base base);



















#define C_COLLECTION_CHECK(c,f) \
        { c_type type_; \
          type_ = c_typeActualType(c__getType(c)); \
          if (c_baseObject(type_)->kind != M_COLLECTION) {  assert(FALSE);}}
            //   OS_REPORT(OS_ERROR,"Database Collection",0, 
            //               "%s: given collection (%d) is not a collection", 
            //               #f, c_collectionType(type_)->kind); 
             

#define C_META_TYPEINIT_(o,t) do { \
        C_ALIGNMENT_C_STRUCT_TYPE(t); \
        c_metaTypeInit(o,C_SIZEOF(t),C_ALIGNMENT_C_STRUCT(t)); \
    } while(0)


#define metaResolveName(scope,name,filter) \
        _metaResolveName(c_metaObject(scope),name,filter)

#define metaNameCompare(scope,name,filter) \
        _metaNameCompare (c_baseObject(scope),name,filter)

#define CQ_KIND_IN_MASK(object,set) \
        ((1u << (c_baseObject(object)->kind-1)) & (set))

#define c_primitiveKind(_this) \
        c_primitive(_this)->kind

#define c_fieldPath_t(base) (base->baseCache.fieldCache.c_fieldPath_t)
#define c_fieldRefs_t(base) (base->baseCache.fieldCache.c_fieldRefs_t)

#define c_newSequence(t,s) \
        (assert(c_collectionTypeKind(t) == OSPL_C_SEQUENCE), c_newBaseArrayObject(c_collectionType(t),s))

#if defined (__cplusplus)
}
#endif

#endif