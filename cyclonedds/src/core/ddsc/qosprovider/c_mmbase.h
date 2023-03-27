#ifndef C_MMBASE_H
#define C_MMBASE_H



#if defined (__cplusplus)
extern "C" {
#endif

#include "c_typebase.h"
#include "os_mutex.h"
#include "/home/dong/work/09_cyc_qos_test_3/cyclonedds/src/core/ddsi/src/ddsi__list_tmpl.h"

#ifndef _Inout_
#define _Inout_
#endif

#define REFBUF_SIZE (256)
#define LIST_BLOCKSIZE 10
#define CF_DATANAME "#text"

typedef struct c_mm_s {
    int dummy;
}*c_mm;


typedef c_bool (*c_action)
               (_Inout_ c_object o, _Inout_ c_voidp arg);



DDSI_LIST_TYPES_TMPL(c__iterImpl, void *, /**/, 32)
DDSI_LIST_TYPES_TMPL(c__listImpl, c_object, struct c_mm_s *mm;, 32)


C_CLASS(c_binding);
C_CLASS(c_scope);
C_CLASS(c_baseObject);
C_CLASS(c_metaObject);
C_CLASS(c_type);
C_CLASS(c_base);
C_CLASS(cf_element);
C_CLASS(cf_nodeList);
C_CLASS(cf_node);
C_CLASS(qp_entityAttr);
C_CLASS(c_module);
C_CLASS(c_header);
C_CLASS(c_freelist);
C_CLASS(c_refbuf);
C_CLASS(c_typeDef);
C_CLASS(c_collectionType);
C_CLASS(c_arrayHeader);
C_CLASS(c_class);
C_CLASS(c_interface);
C_CLASS(c_property);
C_CLASS(c_member);
C_CLASS(c_specifier);
C_CLASS(c_enumeration);
C_CLASS(c_constant);
C_CLASS(c_operand);
C_CLASS(c_primitive);
C_CLASS(c_structure);
C_CLASS(c_constOperand);
C_CLASS(c_literal);
C_CLASS(c_operation);
C_CLASS(c_union);
C_CLASS(c_unionCase);
C_CLASS(c_trashcan);
C_CLASS(c_iter);
C_CLASS(c_baseBinding);
C_CLASS(c_collection);
C_CLASS(c_qPred);
C_CLASS(c_qExpr);
C_CLASS(queryReadActionArg);
C_CLASS(c_qKey);
C_CLASS(c_field);
C_CLASS(c_qRange);
C_CLASS(c_qFunc);
C_CLASS(c_tableNode);
C_CLASS(tableTakeActionArg);
C_CLASS(c_setNode);
C_CLASS(c_bagNode);
C_CLASS(c_qField);
C_CLASS(c_qConst);
C_CLASS(c_qType);
C_CLASS(tableReadActionArg);
C_CLASS(cf_data);
C_CLASS(cf_attribute);
C_CLASS(cmn_qosProviderInputAttr);
C_CLASS(c_expression);
C_CLASS(c_attribute);
C_CLASS(c_annotation);
C_CLASS(c_parameter);
C_CLASS(c_relation);
C_CLASS(c_exception);
C_CLASS(c_qVar);

OS_CLASS(ut_table);
OS_CLASS(ut_set);
OS_CLASS(ut_collection);
OS_CLASS(ut_tableNode);
OS_CLASS(ut_setNode);








C_STRUCT(c_baseObject) {
    c_metaKind kind;
};


C_STRUCT(c_metaObject) {
    C_EXTENDS(c_baseObject);
    c_metaObject definedIn;
    c_string name;
};

C_STRUCT(c_binding) {
    ddsrt_avl_node_t avlnode;
    c_metaObject object;
    c_binding nextInsOrder;
};

C_STRUCT(c_scope) {
    ddsrt_avl_ctree_t bindings;
    c_binding headInsOrder;
    c_binding tailInsOrder;
};

C_STRUCT(c_type) {
    C_EXTENDS(c_metaObject);
    size_t alignment;
    c_base base;
    ddsrt_atomic_uint32_t objectCount;
    size_t size;
};

C_STRUCT(c_queryCache) {
    c_type c_qConst_t;
    c_type c_qType_t;
    c_type c_qVar_t;
    c_type c_qField_t;
    c_type c_qFunc_t;
    c_type c_qPred_t;
    c_type c_qKey_t;
    c_type c_qRange_t;
    c_type c_qExpr_t;
    c_type c_qKind_t;
    c_type c_qBoundKind_t;
};

C_STRUCT(c_fieldCache) {
    c_type c_field_t;
    c_type c_fieldPath_t;
    c_type c_fieldRefs_t;
};


C_STRUCT(c_typeCache) {
    c_type c_object_t;
    c_type c_voidp_t;
    c_type c_bool_t;
    c_type c_address_t;
    c_type c_octet_t;
    c_type c_char_t;
    c_type c_short_t;
    c_type c_long_t;
    c_type c_longlong_t;
    c_type c_uchar_t;
    c_type c_ushort_t;
    c_type c_ulong_t;
    c_type c_ulonglong_t;
    c_type c_float_t;
    c_type c_double_t;
    c_type c_string_t;
    c_type c_wchar_t;
    c_type c_wstring_t;
    c_type c_type_t;
    c_type c_valueKind_t;
    c_type c_member_t;
    c_type c_literal_t;
    c_type c_constant_t;
    c_type c_unionCase_t;
    c_type c_property_t;
    c_type c_mutex_t;
    c_type c_lock_t;
    c_type c_cond_t;
    c_type pa_uint32_t_t;
    c_type pa_uintptr_t_t;
    c_type pa_voidp_t_t;
};

C_STRUCT(c_baseCache) {
    C_STRUCT(c_queryCache) queryCache;
    C_STRUCT(c_fieldCache) fieldCache;
    C_STRUCT(c_typeCache) typeCache;
};

C_STRUCT(c_module) {
    C_EXTENDS(c_metaObject);
    c_mutex mtx;
    c_scope scope;
};




C_STRUCT(c_base) {
    C_EXTENDS(c_module);
    c_mm                    mm;
    c_bool                  maintainObjectCount;
    c_bool                  y2038Ready;
    c_long                  confidence;
    ddsrt_avl_tree_t       bindings;
    c_mutex                 bindLock;
    c_mutex                 serLock; /* currently only used for defining enums from sd_serializerXMLTypeinfo.c */
    c_type                  metaType[M_COUNT];
    c_type                  string_type;
    c_string                emptyString;
    c_wstring               emptyWstring;
    C_STRUCT(c_baseCache) baseCache;
};

C_STRUCT(cf_node) {
    cf_kind kind;
    c_char *name;
};

C_STRUCT(cf_nodeList) {
    int maxNrNodes;
    int nrNodes;
    cf_node *theList;
};

C_STRUCT(cf_element) {
    C_EXTENDS(cf_node);
    cf_nodeList attributes;
    cf_nodeList children;
};

OS_STRUCT(ut_collection) {
    ut_collectionType type;
    ut_compareElementsFunc cmpFunc; /* pointer to user provided compare function */
    void *cmpArg;
    ut_freeElementFunc freeValue;
    void *freeValueArg;
};

OS_STRUCT(ut_table) {
    OS_EXTENDS(ut_collection);
    ddsrt_avl_ctreedef_t td;
    ddsrt_avl_ctree_t tree; /* Internal collection that stores the table elements */
    ut_freeElementFunc freeKey;
    void *freeKeyArg;
};

OS_STRUCT(ut_tableNode) {
    ddsrt_avl_node_t avlnode; /* tableNode is a specialized avlNode */
    void *key; /* identifier for this node */
    void *value; /* value of this node */
};

OS_STRUCT(ut_set) {
    OS_EXTENDS(ut_collection);
    ddsrt_avl_ctreedef_t td;
    ddsrt_avl_ctree_t tree; /* Internal collection that stores the table elements */
};

OS_STRUCT(ut_setNode) {
    ddsrt_avl_node_t avlnode; /* setNode is a specialized avlNode */
    void *value; /* value of this node */
};

C_STRUCT (qp_entityAttr) {
    c_object                    defaultQosTmplt;
    c_type                      qosSampleType;
    cmn_qpCopyOut               copyOut;
    ut_table                    qosTable; /* (char *, c_object)  */
};

C_STRUCT(c_header) {
#ifndef NDEBUG
    c_ulong confidence;
#ifdef OBJECT_WALK
    c_object nextObject;
    c_object prevObject;
#endif
#endif
    ddsrt_atomic_uint32_t refCount;
    c_type type;
};

C_STRUCT(c_refbuf) {
    c_object element[REFBUF_SIZE];
    c_refbuf next;
};

C_STRUCT(c_freelist) {
    C_EXTENDS(c_refbuf);  /* Initial object buffer that is initialized on stack. */
    c_refbuf tail_buf;    /* Tail buffer of the queue, objects are read from this buffer. */
    c_refbuf head_buf;    /* Head buffer of the queue, objects are inserted in this buffer. */
    uint32_t tail_idx;   /* Tail buffer index of the queue, oldest object that is next to be read. */
    uint32_t head_idx;   /* Head buffer index of the queue, place where the next object will be inserted. */
};


C_STRUCT(c_typeDef) {
    C_EXTENDS(c_type);
    c_type alias;
};

C_STRUCT(c_collectionType) {
    C_EXTENDS(c_type);
    c_collKind kind;
    c_ulong maxSize;
    c_type subType;
};

C_STRUCT(c_arrayHeader) {
    c_ulonglong size;
    C_EXTENDS(c_header);
};

C_STRUCT(c_interface) {
    C_EXTENDS(c_type);
    c_bool abstract;
    c_array inherits;   /* c_interface references!!!!! */
    c_array references; /* optimization */
    c_scope scope;
};

C_STRUCT(c_class) {
    C_EXTENDS(c_interface);
    c_class extends;
    c_array keys;       /* c_string */
};

C_STRUCT(c_property) {
    C_EXTENDS(c_metaObject);
    size_t offset;      /* implementation memory mapping */
    c_type type;
};

C_STRUCT(c_specifier) {
    C_EXTENDS(c_baseObject);
    c_string name;
    c_type type;
};

C_STRUCT(c_member) {
    C_EXTENDS(c_specifier);
    size_t offset;      /* implementation memory mapping */
};

C_STRUCT(c_enumeration) {
    C_EXTENDS(c_type);
    c_array elements;   /* c_constant */
};

C_STRUCT(c_constant) {
    C_EXTENDS(c_metaObject);
    c_operand operand;
    c_type type;
};

C_STRUCT(c_operand) {
    C_EXTENDS(c_baseObject);
};

C_STRUCT(c_primitive) {
    C_EXTENDS(c_type);
    c_primKind kind;
};

C_STRUCT(c_structure) {
    C_EXTENDS(c_type);
    c_array members;    /* c_member */
    c_array references; /* optimization */
    c_scope scope;
};

C_STRUCT(c_constOperand) {
    C_EXTENDS(c_operand);
    c_constant constant;
};

C_STRUCT(c_literal) {
    C_EXTENDS(c_operand);
    c_value value;
};

C_STRUCT(c_operation) {
    C_EXTENDS(c_metaObject);
    c_array parameters;
    c_type result;
};

C_STRUCT(c_union) {
    C_EXTENDS(c_type);
    c_array cases;      /* c_unionCase */
    c_array references; /* optimization */
    c_scope scope;
    c_type switchType;
};

C_STRUCT(c_unionCase) {
    C_EXTENDS(c_specifier);
    c_array labels;     /* c_literal */
};

C_STRUCT(c_iter) {
    struct c__iterImpl x;
};

C_STRUCT(c_trashcan) {
    c_iter trash;
    c_iter arrays;
    c_iter scopes;
    c_iter collections;
    c_iter references;
};

C_STRUCT(c_baseBinding) {
    ddsrt_avl_node_t avlnode;
    c_object object;
    c_char  *name;
};

C_STRUCT(c_qExpr) {
    c_qKind kind;
};

C_STRUCT(c_qPred) {
    c_bool fixed;
    c_qExpr expr;
    c_array keyField;  /* ARRAY<c_qKey> */
    c_array varList;   /* ARRAY<c_qVar> */
    c_qPred next;
};


C_STRUCT(c_query) {
    c_qPred pred;
    c_collection source;
};

C_STRUCT(queryReadActionArg) {
    c_action action;
    c_voidp arg;
    c_qPred predicate;
};

C_STRUCT(c_field) {
    c_valueKind kind;
    c_address   offset;
    c_string    name;
    c_array     path;
    c_array     refs;
    c_type      type;
};

C_STRUCT(c_qKey) {
    c_qExpr expr;
    c_field field;
    c_array range;     /* ARRAY<c_qRange> */
};

C_STRUCT(c_qRange) {
    c_qBoundKind startKind;
    c_qBoundKind endKind;
    c_qExpr startExpr;
    c_qExpr endExpr;
    c_value start;
    c_value end;
};

C_STRUCT(c_qFunc) {
    C_EXTENDS(c_qExpr);
    c_array params;
};

union c_tableContents {
    c_object object;
    ddsrt_avl_tree_t tree;
};

C_STRUCT(c_table) {
    union c_tableContents contents;
    c_array cursor;
    c_array key;
    c_ulong count;
    c_mm mm;
    _STATISTICS_
};

C_STRUCT(c_tableNode) {
    ddsrt_avl_node_t avlnode;
    c_value keyValue;
    union c_tableContents contents;
#ifdef _CONSISTENCY_CHECKING_
    c_voidp table;
#endif
};

C_STRUCT(tableTakeActionArg) {
    c_array key;
    c_ulong keyIndex;
    c_qPred pred;
    c_tableNode disposed;
    c_action action;
    c_voidp arg;
    c_ulong count;
    c_bool proceed;
    c_mm mm;
#ifdef _CONSISTENCY_CHECKING_
    C_STRUCT(c_table) *t;
#endif
};

C_STRUCT(c_set) {
    ddsrt_avl_ctree_t tree;
    c_mm mm;
};

C_STRUCT(c_setNode) {
    ddsrt_avl_node_t avlnode;
    c_object object;
};

C_STRUCT(c_bag) {
    ddsrt_avl_tree_t tree;
    c_ulong count;
    c_mm mm;
};

C_STRUCT(c_bagNode) {
    ddsrt_avl_node_t avlnode;
    c_object object;
    c_long count;
};

C_STRUCT(c_list) {
    struct c__listImpl x;
};

C_STRUCT(c_qField) {
    C_EXTENDS(c_qExpr);
    c_field field;
};

C_STRUCT(c_qConst) {
    C_EXTENDS(c_qExpr);
    c_value value;
};

C_STRUCT(cf_data) {
    C_EXTENDS(cf_node);
    c_value value;
};

C_STRUCT(c_qType) {
    C_EXTENDS(c_qExpr);
    c_type type;
};

C_STRUCT(tableReadActionArg) {
    c_array key;
    c_ulong keyIndex;
    c_qPred query;
    c_action action;
    c_voidp arg;
#ifdef _CONSISTENCY_CHECKING_
    C_STRUCT(c_table) *t;
#endif
};

C_STRUCT(cf_attribute) {
    C_EXTENDS(cf_node);
    c_value value;
};

C_STRUCT(cmn_qosInputAttr) {
    cmn_qpCopyOut               copyOut;
};

C_STRUCT(cmn_qosProviderInputAttr) {
    C_STRUCT(cmn_qosInputAttr)   participantQos;
    C_STRUCT(cmn_qosInputAttr)   topicQos;
    C_STRUCT(cmn_qosInputAttr)   subscriberQos;
    C_STRUCT(cmn_qosInputAttr)   dataReaderQos;
    C_STRUCT(cmn_qosInputAttr)   publisherQos;
    C_STRUCT(cmn_qosInputAttr)   dataWriterQos;
};

C_STRUCT(c_expression) {
    C_EXTENDS(c_operand);
    c_exprKind kind;
    c_array operands;
};

C_STRUCT(c_attribute) {
    C_EXTENDS(c_property);
    c_bool isReadOnly;
};

C_STRUCT(c_annotation) {
    C_EXTENDS(c_interface);
    c_annotation extends;
};

C_STRUCT(c_parameter) {
    C_EXTENDS(c_specifier);
    c_direction mode;
};

C_STRUCT(c_relation) {
    C_EXTENDS(c_property);
    c_string inverse;
};

C_STRUCT(c_exception) {
    C_EXTENDS(c_structure);
};


C_STRUCT(c_qVar) {
    c_long id;
    c_bool hasChanged;
    c_collection keys;
    c_qConst variable;
#if 1
    c_type type;
#endif
};






typedef struct bindArg {
    c_mm mm;
    c_trashcan trashcan;
} *bindArgp;

typedef struct c_iterIter {
    struct c__iterImpl_iter it;
    void *current;
} c_iterIter;



#define C_ALIGNMENT_TYPE(t) struct c_alignment_type_##t { char c; t x; }
C_ALIGNMENT_TYPE (c_value);
C_ALIGNMENT_TYPE (c_metaKind);
C_ALIGNMENT_TYPE (c_string);
C_ALIGNMENT_TYPE (c_address);
// C_ALIGNMENT_TYPE (c_size);
C_ALIGNMENT_TYPE (c_voidp);
C_ALIGNMENT_TYPE (c_octet);
C_ALIGNMENT_TYPE (c_short);
C_ALIGNMENT_TYPE (c_ushort);
C_ALIGNMENT_TYPE (c_long);
C_ALIGNMENT_TYPE (c_ulong);
C_ALIGNMENT_TYPE (c_char);
C_ALIGNMENT_TYPE (c_wchar);
C_ALIGNMENT_TYPE (c_float);
C_ALIGNMENT_TYPE (c_double);
C_ALIGNMENT_TYPE (c_bool);
C_ALIGNMENT_TYPE (c_longlong);
C_ALIGNMENT_TYPE (c_ulonglong);
// C_ALIGNMENT_TYPE (c_string);
// C_ALIGNMENT_TYPE (c_wstring);
// C_ALIGNMENT_TYPE (c_array);
// C_ALIGNMENT_TYPE (c_sequence);
C_ALIGNMENT_TYPE (ddsrt_atomic_uint32_t);
C_ALIGNMENT_TYPE (ddsrt_atomic_uintptr_t);
C_ALIGNMENT_TYPE (ddsrt_atomic_voidp_t);
// C_ALIGNMENT_TYPE (c_value);
C_ALIGNMENT_TYPE (c_object);
C_ALIGNMENT_TYPE (c_mutex);
C_ALIGNMENT_TYPE (c_lock);
C_ALIGNMENT_TYPE (c_cond);




#define C_ALIGNMENT_C_STRUCT_TYPE(t) struct c_alignment_type_struct_##t { char c; C_STRUCT(t) x; }
#define C_ALIGNMENT_C_STRUCT(t) ((c_ulong) offsetof (struct c_alignment_type_struct_##t, x))


C_ALIGNMENT_C_STRUCT_TYPE (c_baseObject);
C_ALIGNMENT_C_STRUCT_TYPE (c_metaObject);
C_ALIGNMENT_C_STRUCT_TYPE (c_type);
C_ALIGNMENT_C_STRUCT_TYPE (c_module);
C_ALIGNMENT_C_STRUCT_TYPE (c_typeDef);
C_ALIGNMENT_C_STRUCT_TYPE (c_collectionType);
C_ALIGNMENT_C_STRUCT_TYPE (c_class);
C_ALIGNMENT_C_STRUCT_TYPE (c_interface);
C_ALIGNMENT_C_STRUCT_TYPE (c_property);
C_ALIGNMENT_C_STRUCT_TYPE (c_specifier);
C_ALIGNMENT_C_STRUCT_TYPE (c_member);
C_ALIGNMENT_C_STRUCT_TYPE (c_enumeration);
C_ALIGNMENT_C_STRUCT_TYPE (c_constant);
C_ALIGNMENT_C_STRUCT_TYPE (c_operand);
C_ALIGNMENT_C_STRUCT_TYPE (c_primitive);
C_ALIGNMENT_C_STRUCT_TYPE (c_structure);
C_ALIGNMENT_C_STRUCT_TYPE (c_constOperand);
C_ALIGNMENT_C_STRUCT_TYPE (c_literal);
C_ALIGNMENT_C_STRUCT_TYPE (c_operation);
C_ALIGNMENT_C_STRUCT_TYPE (c_union);
C_ALIGNMENT_C_STRUCT_TYPE (c_unionCase);
C_ALIGNMENT_C_STRUCT_TYPE (c_expression);
C_ALIGNMENT_C_STRUCT_TYPE (c_attribute);
C_ALIGNMENT_C_STRUCT_TYPE (c_annotation);
C_ALIGNMENT_C_STRUCT_TYPE (c_parameter);
C_ALIGNMENT_C_STRUCT_TYPE (c_relation);
C_ALIGNMENT_C_STRUCT_TYPE (c_exception);







#define c_collectionTypeMaxSize(_this) c_collectionType(_this)->maxSize
#define c_collectionTypeSubType(_this) c_collectionType(_this)->subType
#define C_ALIGNMENT(t) ((c_ulong) offsetof (struct c_alignment_type_##t, x))

#define ACTUALTYPE(_t,_type) \
        _t = _type; \
        while (c_baseObject(_t)->kind == M_TYPEDEF) { \
            _t = c_typeDef(_t)->alias; \
        };

#define OBJECTTYPE(t,o) ACTUALTYPE(t,c_header(o)->type)
#define CHECKOBJECT(o) assert(o == NULL || c_header(o)->confidence == CONFIDENCE)

#define c_baseObjectKind(_this) \
        c_baseObject(_this)->kind

#define c_collectionTypeKind(_this) \
        c_collectionType(_this)->kind


#define C_MAXALIGNMENT C_ALIGNMENT(c_value)
#define C_ALIGNSIZE(size,alignment) \
        (((((size)-1)/(alignment))+1)*(alignment))

#define C_MAXALIGNSIZE(size) \
        C_ALIGNSIZE(size,C_MAXALIGNMENT)

static const c_ulong HEADERSIZE = C_MAXALIGNSIZE(C_SIZEOF(c_header));
static const c_ulong ARRAYHEADERSIZE = C_MAXALIGNSIZE(C_SIZEOF(c_arrayHeader));

#define C_CAST(o,t)         ((t)(o))
#define c_scope(o)          ((c_scope)(o))
#define c_metaObject(o)     ((c_metaObject)(o))
#define c_binding(o)        ((c_binding)(o))
#define cf_element(o)       ((cf_element)(o))
#define cf_nodeList(o)      ((cf_nodeList)(o))
#define cf_node(o)          ((cf_node)(o))
#define ut_table(t)         ((ut_table) (t))
#define ut_collection(c)    ((ut_collection)(c))
#define c_header(o)         ((c_header)(C_ADDRESS(o) - HEADERSIZE))
#define c_arrayHeader(o)    ((c_arrayHeader)(C_ADDRESS(o) - ARRAYHEADERSIZE))
#define c_refbuf(o)         ((c_refbuf)o)
#define c_value(v)          ((c_value)(v))
#define c_baseObject(o)     ((c_baseObject)(o))
#define c_type(o)           ((c_type)(o))
#define c_operand(o)        ((c_operand)(o))
#define c_interface(o)      ((c_interface)(o))
#define c_property(o)       ((c_property)(o))
#define c_object(o)         ((c_object)(o))
#define c_specifier(o)      ((c_specifier)(o))
#define ut_tableNode(node)  ((ut_tableNode) (node))
#define cf_data(o)          ((cf_data)(o))
#define cf_attribute(o)     ((cf_attribute)(o))
#define c_annotation(o)     ((c_annotation)(o))
#define ut_set(s)           ((ut_set) (s))
#define ut_setNode(node)    ((ut_setNode) (node))
#define c_array(c)          ((c_array)(c))
#define c_sequence(c)       ((c_sequence)(c))




#define c_module(o)         (C_CAST(o,c_module))
#define c_typeDef(o)        (C_CAST(o,c_typeDef))
#define c_collectionType(o) (C_CAST(o,c_collectionType))
#define c_enumeration(o)    (C_CAST(o,c_enumeration))
#define c_constant(o)       (C_CAST(o,c_constant))
#define c_class(o)          (C_CAST(o,c_class))
#define c_primitive(o)      (C_CAST(o,c_primitive))
#define c_structure(o)      (C_CAST(o,c_structure))
#define c_member(o)         (C_CAST(o,c_member))
#define c_constOperand(o)   (C_CAST(o,c_constOperand))
#define c_literal(o)        (C_CAST(o,c_literal))
#define c_operation(o)      (C_CAST(o,c_operation))
#define c_union(o)          (C_CAST(o,c_union))
#define c_unionCase(o)      (C_CAST(o,c_unionCase))
#define c_qPred(q)          (C_CAST(q, c_qPred))
#define c_qExpr(q)          (C_CAST(q, c_qExpr))
#define c_qKey(q)           (C_CAST(q, c_qKey))
#define c_qRange(q)         (C_CAST(q, c_qRange))
#define c_qFunc(q)          (C_CAST(q, c_qFunc))
#define c_qField(q)         (C_CAST(q, c_qField))
#define c_qConst(q)         (C_CAST(q, c_qConst))
#define c_qType(q)          (C_CAST(q, c_qType))
#define c_expression(o)     (C_CAST(o,c_expression))
#define c_attribute(o)      (C_CAST(o,c_attribute))
#define c_parameter(o)      (C_CAST(o,c_parameter))
#define c_relation(o)       (C_CAST(o,c_relation))
#define c_exception(o)      (C_CAST(o,c_exception))
#define c_qVar(q)           (C_CAST(q, c_qVar))



typedef c_collection c_query;
typedef c_collection c_table;
typedef c_collection c_set;
typedef c_collection c_bag;
typedef c_collection c_list;
#define c_query(c) ((C_STRUCT(c_query) *)(c))
#define c_table(c) ((C_STRUCT(c_table) *)(c))
#define c_set(c)   ((C_STRUCT(c_set)   *)(c))
#define c_bag(c)   ((C_STRUCT(c_bag)   *)(c))
#define c_list(c)  ((C_STRUCT(c_list)  *)(c))



struct ut_tableFreeHelper_arg {
    ut_freeElementFunc freeKey;
    void *freeKeyArg;
    ut_freeElementFunc freeValue;
    void *freeValueArg;
};

typedef struct c_tableWalkActionArgs {
    c_action action;
    c_voidp actionArg;
    c_ulong nrOfKeys;
} *c_tableWalkActionArg;

typedef struct os_cfg_handle
{
   char *ptr;
   int isStatic;
   size_t size;
} os_cfg_handle;

typedef struct os_URIListNode
{
   const char * const uri;
   char * const config; /* double null terminated to use with */
                            /* yy_scan_buffer */
   const unsigned long size;
} os_URIListNode;


c_mm c_mmCreate(void *address,c_size size,c_size threshold);
void * c_mmMallocThreshold (c_mm mm,size_t size);
void * c_mmMalloc(c_mm mm,size_t size);
void * c_mmBind(c_mm mm,const c_char *name,void *memory);



#if defined (__cplusplus)
}
#endif

#endif /* C_MM_H */