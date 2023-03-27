#include "fun_base.h"
#include "c_base.h"
#include "/home/dong/work/09_cyc_qos_test_3/cyclonedds/src/core/ddsi/src/ddsi__list_tmpl.h"
#include <ctype.h>
#include <float.h>


static const char fileURI[] = "file://";
static const size_t fileURILen = sizeof(fileURI)-1;
////
static const char osplcfgURI[] = "osplcfg://";
static const size_t osplcfgURILen = sizeof(osplcfgURI)-1;
////
const os_URIListNode os_cfg_cfgs [] = {{ NULL, NULL, 0UL }};

static os_mutex atExitMutex;

#define	EINVAL		22	/* Invalid argument */

#define	ERANGE		34	/* Math result not representable */


#define OS_MAX_INTEGER(T) ((T)(((T)~0) ^ ((T)!((T)~0 > 0) << (CHAR_BIT * sizeof(T) - 1))))

extern int *__errno_location (void) __THROW __attribute_const__;
# define errno (*__errno_location ())

#define DOUBLE_STRING_MAX_LENGTH (3 + DBL_MANT_DIG - DBL_MIN_EXP)

#define OS_STR_SPACE " \t\r\n"



#define VALID_DOUBLE_CHAR(c) ( (isspace(c)            ) || /* (leading) whitespaces   */ \
                               (isxdigit(c)           ) || /* (hexa)decimal digits    */ \
                               (c == '.'              ) || /* ospl LC_NUMERIC         */ \
                               (c == os_lcNumericGet()) || /* locale LC_NUMERIC       */ \
                               (c == '+') || (c == '-') || /* signs                   */ \
                               (c == 'x') || (c == 'X') || /* hexadecimal indication  */ \
                               (c == 'e') || (c == 'E') || /* exponent chars          */ \
                               (c == 'p') || (c == 'P') || /* binary exponent chars   */ \
                               (c == 'a') || (c == 'A') || /* char for NaN            */ \
                               (c == 'n') || (c == 'N') || /* char for NaN & INFINITY */ \
                               (c == 'i') || (c == 'I') || /* char for INFINITY       */ \
                               (c == 'f') || (c == 'F') || /* char for INFINITY       */ \
                               (c == 't') || (c == 'T') || /* char for INFINITY       */ \
                               (c == 'y') || (c == 'Y') )  /* char for INFINITY       */


static const ddsrt_avl_ctreedef_t c_scope_bindings_td =
    DDSRT_AVL_TREEDEF_INITIALIZER_INDKEY (offsetof (C_STRUCT(c_binding), avlnode),
                                        offsetof (C_STRUCT(c_binding), object),
                                        c_bindingCompare, 0);

static const ddsrt_avl_treedef_t c_table_td =
    DDSRT_AVL_TREEDEF_INITIALIZER (
        offsetof (C_STRUCT(c_tableNode), avlnode), offsetof (C_STRUCT(c_tableNode), keyValue),
        tableCmp, 0);

static const ddsrt_avl_ctreedef_t c_set_td =
    DDSRT_AVL_CTREEDEF_INITIALIZER_INDKEY (
        offsetof (C_STRUCT(c_setNode), avlnode), offsetof (C_STRUCT(c_setNode), object),
        ptrCompare, 0);

static const ddsrt_avl_treedef_t c_bag_td =
    DDSRT_AVL_TREEDEF_INITIALIZER_INDKEY (
        offsetof (C_STRUCT(c_bagNode), avlnode), offsetof (C_STRUCT(c_bagNode), object),
        ptrCompare, 0);


#define C__LISTIMPL_MALLOC(a) c_mmMalloc(list->mm, (a))
#define C__LISTIMPL_FREE(a) c_mmFree(list->mm, (a))
DDSI_LIST_CODE_TMPL(static, c__iterImpl, void *, NULL, ddsrt_malloc, ddsrt_free)
DDSI_LIST_CODE_TMPL(static, c__listImpl, c_object, NULL, C__LISTIMPL_MALLOC, C__LISTIMPL_FREE)
#define CHECKOBJECT(o) assert(o == NULL || c_header(o)->confidence == CONFIDENCE)

#define C_UNLIMITED (0)




int
c_bindingCompare(const void *va, const void *vb)
{
    const C_STRUCT(c_metaObject) *a = va;
    const C_STRUCT(c_metaObject) *b = vb;
    if (a->name == NULL) return 1;
    if (b->name == NULL) return -1;
    return strcmp (a->name, b->name);
}

c_base
c_getBase(
    c_object object)
{
    if (object == NULL) {
        return NULL;
    }
    return c_header(object)->type->base;
}

c_type
c_getMetaType(
    c_base base,
    c_metaKind kind)
{
    return base->metaType[kind];
}

c_type
c_typeActualType(
    c_type type)
{
    c_type t = type;

    while (c_baseObjectKind(t) == M_TYPEDEF) {
        t = c_typeDef(t)->alias;
    }

    return t;
}


c_bool
c_typeIsRef (
    c_type type)
{
    c_bool result;

    switch(c_baseObjectKind(type)) {
    case M_CLASS:
    case M_INTERFACE:
    case M_ANNOTATION:
        result = TRUE;
    break;
    case M_COLLECTION:
        switch(c_collectionType(type)->kind) {
        case OSPL_C_ARRAY:
            if (c_collectionType(type)->maxSize == 0) {
                result = TRUE;
            } else {
                result = FALSE;
            }
        break;
        default:
            result = TRUE;
        }
    break;
    case M_TYPEDEF:
        result = c_typeIsRef(c_typeActualType(type));
    break;
    default:
        result = FALSE;
    break;
    }

    return result;
}

c_bool
c_typeHasRef (
    c_type type)
{
    switch(c_baseObjectKind(type)) {
    case M_CLASS:
    case M_INTERFACE:
    case M_ANNOTATION:
        return TRUE;
    case M_COLLECTION:
        switch(c_collectionType(type)->kind) {
        case OSPL_C_ARRAY:
            if (c_collectionType(type)->maxSize == 0) {
                return TRUE;
            } else {
                return c_typeHasRef(c_collectionType(type)->subType);
            }
        default:
            return TRUE;
        }
    case M_EXCEPTION:
    case M_STRUCTURE:
        if (c_structure(type)->references != NULL) {
            return TRUE;
        }
        return FALSE;
    case M_UNION:
        if (c_union(type)->references != NULL) {
            return TRUE;
        }
        return FALSE;
    case M_PRIMITIVE:
        switch (c_primitive(type)->kind) {
        case P_MUTEX:
        case P_LOCK:
        case P_COND:
            return TRUE;
        default:
            return FALSE;
        }
    case M_ENUMERATION:
    case M_BASE:
        return FALSE;
    case M_TYPEDEF:
        return c_typeHasRef(c_typeDef(type)->alias);
    default:
        // OS_REPORT(OS_WARNING,
        //           "c_typeHasRef failed",0,
        //           "specified type is not a type");
        assert(FALSE); /* not a type */
    break;
    }
    return FALSE;
}

void
c_bindingFree (
    c_binding binding,
    c_mm mm)
{
    c_free(binding->object);
    c_mmFree(mm,binding);
}

void c_bindingFreeWrapper (void *binding, void *mm)
{
    c_bindingFree (binding, mm);
}

void
c_scopeDeinit(
    c_scope scope)
{
    c_scopeClean (scope);
}

void
c_mmFree(
    c_mm mm,
    void *memory)
{
    OS_UNUSED_ARG(mm);
    if(memory != NULL)
    {
        ddsrt_free(memory);
    }
}

void c_mmTrackObject (struct c_mm_s *mm, const void *ptr, uint32_t code)
{
    OS_UNUSED_ARG(mm);
    OS_UNUSED_ARG(ptr);
    OS_UNUSED_ARG(code);
}





void
c_free (
    c_object object)
{
    c_header header;
    c_type type, headerType;
    C_STRUCT(c_freelist) freelist;
    uint32_t safeCount;
#if CHECK_REF
    c_bool matchesRefRequest = FALSE;
#endif

    if (object == NULL) {
        return;
    }

    c_freelistInit(&freelist);

    while (object) {
        header = c_header (object);

#ifndef NDEBUG
        assert(header->confidence == CONFIDENCE);
        if ((ddsrt_atomic_ld32(&header->refCount) & REFCOUNT_MASK) == 0) {
#if CHECK_REF
            UT_TRACE("\n\n===========Free(%p) already freed =======\n", object);
#endif
            // OS_REPORT(OS_ERROR,
            //             "Database",0,
            //             "Object (%p) of type '%s', kind '%s' already deleted",
            //             object,
            //             c_metaName(c_metaObject(header->type)),
            //             metaKindImage(c_baseObject(header->type)->kind));
            assert(0);
        }
#endif /* NDEBUG */

#if CHECK_REF
        /* Take a local pointer, since header->type pointer will be deleted */
        headerType = header->type;
        ACTUALTYPE(type,headerType);
        if (type && c_metaObject(type)->name) {
          if (strlen(c_metaObject(type)->name) >= CHECK_REF_TYPE_LEN) {
            if (strncmp(c_metaObject(type)->name, CHECK_REF_TYPE, strlen(CHECK_REF_TYPE)) == 0) {
                matchesRefRequest = TRUE;
            }
          }
        }
#endif
        safeCount = ddsrt_atomic_dec32_nv(&header->refCount);

        if ((safeCount & REFCOUNT_MASK) != 0) {
            if (safeCount & REFCOUNT_FLAG_TRACE) {
                c_type headerType = header->type, type;
                void *block;
                ACTUALTYPE (type, headerType);
                if ((c_baseObjectKind (type) == M_COLLECTION) &&
                    ((c_collectionTypeKind (type) == OSPL_C_ARRAY) ||
                     (c_collectionTypeKind (type) == OSPL_C_SEQUENCE)))
                {
                    block = c_arrayHeader (object);
                } else {
                    block = header;
                }
                c_mmTrackObject (type->base->mm, block, C_MMTRACKOBJECT_CODE_MIN + 1);
            }
        } else {
            c_base base;

            /* Take a local pointer, since header->type pointer will be deleted */
#if ! CHECK_REF
            headerType = header->type;
            ACTUALTYPE(type,headerType);
#endif

            base = type->base;
            if (!(safeCount & REFCOUNT_FLAG_ATOMIC)) {
                c_freeReferences(c_metaObject(type),object, &freelist);
            }
#ifndef NDEBUG
#ifdef OBJECT_WALK
            {
                c_object *prevNext;
                c_object *nextPrev;

                if (header->prevObject != NULL) {
                    prevNext = &c_header(header->prevObject)->nextObject;
                } else {
                    prevNext = &type->base->firstObject;
                }
                if (header->nextObject != NULL) {
                    nextPrev = &c_header(header->nextObject)->prevObject;
                } else {
                    nextPrev = &type->base->lastObject;
                }
                *prevNext = header->nextObject;
                *nextPrev = header->prevObject;
            }
#endif
#endif

            if ((c_baseObjectKind(type) == M_COLLECTION) &&
                ((c_collectionTypeKind(type) == OSPL_C_ARRAY) ||
                 (c_collectionTypeKind(type) == OSPL_C_SEQUENCE))) {
                c_arrayHeader hdr;

                hdr = c_arrayHeader(object);
#ifdef OSPL_STRICT_MEM
                {
                    c_long size;
                    size = c_arraySize(object);
                    memset(hdr,0xff,ARRAYMEMSIZE(size));
                }
#endif
                if (safeCount & REFCOUNT_FLAG_TRACE) {
                    c_mmTrackObject (base->mm, hdr, C_MMTRACKOBJECT_CODE_MIN + 3);
                }
                c_mmFree(base->mm, hdr);
            } else {
#ifdef OSPL_STRICT_MEM
                {
                    /* Only when OSPL_STRICT_MEM has been set so that we can abort on the detection of illegal usage
                     * of a deleted mutex or conditon, without blocking the calling thread indefinitely
                     */
                    c_long size;
                    size = c_typeSize(type);
                    memset(header,0xff,MEMSIZE(size));
                }
#endif
                if (safeCount & REFCOUNT_FLAG_TRACE) {
                    c_mmTrackObject (base->mm, header, C_MMTRACKOBJECT_CODE_MIN + 3);
                }
                c_mmFree(base->mm, header);
            }
            /* Do not use type, as it refers to an actual type, while
             * we incremented the header->type.
             */
            if (base->maintainObjectCount) {
                /* Since no special actions need to be performed on going to 0, the
                 * return-value of the decrement isn't needed.
                 */
                (void) ddsrt_atomic_dec32_nv(&headerType->objectCount);
            }
#if TYPE_REFC_COUNTS_OBJECTS
            c_free(headerType); /* free the header->type */
#endif
        }

#if CHECK_REF
        if (matchesRefRequest) {
            UT_TRACE("\n\n============ Free(%p): %d -> %d =============\n",
                     object, (safeCount & REFCOUNT_MASK)+1, safeCount & REFCOUNT_MASK);
        }
#endif
        object = c_freelistTake(&freelist);
    }
} 


void ut_tableFreeHelper (void *vnode, void *varg)
{
    ut_tableNode node = vnode;
    struct ut_tableFreeHelper_arg *arg = varg;
    if (arg->freeKey) {
        arg->freeKey(node->key, arg->freeKeyArg);
    }
    if (arg->freeValue) {
        arg->freeValue(node->value, arg->freeValueArg);
    }
    ddsrt_free (node);
}



void
ut_tableFree(
    ut_table table)
{
    struct ut_tableFreeHelper_arg arg;
    arg.freeKey = table->freeKey;
    arg.freeKeyArg = table->freeKeyArg;
    arg.freeValue = ut_collection(table)->freeValue;
    arg.freeValueArg = ut_collection(table)->freeValueArg;
    ddsrt_avl_cfree_arg (&table->td, &table->tree, ut_tableFreeHelper, &arg);
    ddsrt_free(table);
}

c_iter c_iterNew(void *object)
{
    c_iter iter = ddsrt_malloc(sizeof(*iter));
    c__iterImpl_init(&iter->x);
    if (object != NULL) {
        c__iterImpl_append(&iter->x, object);
    }
    return iter;
}

c_iter c_iterInsert(c_iter iter, void *object)
{
    if (iter == NULL) {
        return c_iterNew(object);
    } else {
        c__iterImpl_insert(&iter->x, object);
    }
    return iter;
}



void *c_iterTakeFirst(c_iter iter)
{
    if (iter == NULL) {
        return NULL;
    } else {
        return c__iterImpl_take_first(&iter->x);
    }
}




void
walkReferences(
    c_metaObject metaObject,
    c_object o,
    c_trashcan trashcan)
{
    c_type type;
    c_class cls;
    c_array references, labels, ar;
    c_property property;
    c_member member;
    c_ulong i,j,length;
    c_size size;
    c_ulong nrOfRefs,nrOfLabs;
    c_value v;
    c_bool fixType = FALSE;

    if (metaObject == NULL || o == NULL) return;

    switch (c_baseObjectKind(metaObject)) {
    case M_CLASS:
        cls = c_class(metaObject);
        while (cls) {
            length = c_arraySize(c_interface(cls)->references);
            for (i=0;i<length;i++) {
                property = c_property(c_interface(cls)->references[i]);
                type = property->type;
                collectReferenceGarbage(C_DISPLACE(o,property->offset),type, trashcan);
            }
            cls = cls->extends;
        }
    break;
    case M_INTERFACE:
    case M_ANNOTATION:
        length = c_arraySize(c_interface(metaObject)->references);
        for (i=0;i<length;i++) {
            property = c_property(c_interface(metaObject)->references[i]);
            type = property->type;
            collectReferenceGarbage(C_DISPLACE(o,property->offset),type, trashcan);
        }
    break;
    case M_EXCEPTION:
    case M_STRUCTURE:
        length = c_arraySize(c_structure(metaObject)->references);
        for (i=0;i<length;i++) {
            member = c_member(c_structure(metaObject)->references[i]);
            type = c_specifier(member)->type;
            collectReferenceGarbage(C_DISPLACE(o,member->offset),type, trashcan);
        }
    break;
    case M_UNION:
#define _CASE_(k,t) case k: v = t##Value(*((t *)o)); break
        switch (c_metaValueKind(c_metaObject(c_union(metaObject)->switchType))) {
        _CASE_(V_BOOLEAN,   c_bool);
        _CASE_(V_OCTET,     c_octet);
        _CASE_(V_SHORT,     c_short);
        _CASE_(V_LONG,      c_long);
        _CASE_(V_LONGLONG,  c_longlong);
        _CASE_(V_USHORT,    c_ushort);
        _CASE_(V_ULONG,     c_ulong);
        _CASE_(V_ULONGLONG, c_ulonglong);
        _CASE_(V_CHAR,      c_char);
        _CASE_(V_WCHAR,     c_wchar);
        default:
            return;
        }
#undef _CASE_
        references = c_union(metaObject)->references;
        if (references != NULL) {
            i=0; type=NULL;
            nrOfRefs = c_arraySize(references);
            while ((i<nrOfRefs) && (type == NULL)) {
                labels = c_unionCase(references[i])->labels;
                j=0;
                nrOfLabs = c_arraySize(labels);
                while ((j<nrOfLabs) && (type == NULL)) {
                    if (c_valueCompare(v,c_literal(labels[j])->value) == C_EQ) {
                        walkReferences(c_metaObject(references[i]),
                                       C_DISPLACE(o,c_type(metaObject)->alignment), trashcan);
                        type = c_specifier(references[i])->type;
                    }
                    j++;
                }
                i++;
            }
        }
    break;
    case M_COLLECTION:
        switch (c_collectionTypeKind(metaObject)) {
        case OSPL_C_ARRAY:
        case OSPL_C_SEQUENCE:
            ACTUALTYPE(type,((c_collectionType)metaObject)->subType);
            /* Some internal database arrays have an incorrect subtype.
             * This should be fixed but for now detect and correct during
             * garbage collection.
             */
            if (type == c_getMetaType(c_getBase(type),M_BASE)) {
                fixType = TRUE;
            }
            ar = (c_array)o;

            if (c_collectionTypeKind(metaObject) == OSPL_C_ARRAY &&
                c_collectionTypeMaxSize(metaObject) > 0)
            {
                length = ((c_collectionType)metaObject)->maxSize;
            } else {
                length = c_arraySize(ar);
            }

            if (c_typeIsRef(type)) {
                for (i=0;i<length;i++) {
                    if (fixType) {
                        OBJECTTYPE(type, ar[i]);
                    }
                    /* Need to check type again as it might have been fixed */
                    if (c_typeIsRef(type)) {
                        CHECKOBJECT(ar[i]);
                        trashcan->references = c_iterAppend(trashcan->references, ar[i]);
                    } else {
                        walkReferences(c_metaObject(type),ar[i],trashcan);
                    }
                }
            } else {
                if (c_typeHasRef(type)) {
                    size = type->size;
                    for (i=0;i<length;i++) {
                        walkReferences(c_metaObject(type),C_DISPLACE(ar,(i*size)),trashcan);
                    }
                }
            }
        break;
        case OSPL_C_SCOPE:
            c_scopeWalk(c_scope(o), collectScopeGarbage, trashcan);
        break;
        case OSPL_C_STRING:
        break;
        default:
            (void)c_walk(o, collectGarbage, trashcan);
        break;
        }
    break;
    case M_BASE:
    break;
    case M_TYPEDEF:
        walkReferences(c_metaObject(c_typeDef(metaObject)->alias),o, trashcan);
    break;
    case M_ATTRIBUTE:
    case M_RELATION:
        ACTUALTYPE(type,c_property(metaObject)->type);
        collectReferenceGarbage(C_DISPLACE(o,c_property(metaObject)->offset),type, trashcan);
    break;
    case M_MEMBER:
        ACTUALTYPE(type,c_specifier(metaObject)->type);
        collectReferenceGarbage(C_DISPLACE(o,c_member(metaObject)->offset),type, trashcan);
    break;
    case M_UNIONCASE:
        ACTUALTYPE(type,c_specifier(metaObject)->type);
        collectReferenceGarbage(o,type, trashcan);
    break;
    case M_MODULE:
        CHECKOBJECT(c_module(o)->scope);
        trashcan->references = c_iterAppend(trashcan->references, c_module(o)->scope);
    break;
    case M_PRIMITIVE:
        /* Do nothing */
    break;
    default:
    break;
    }
}

void
collectReferenceGarbage(
    c_voidp *p,
    c_type type,
    c_trashcan trashcan)
{
    c_type t = type;

    if (p == NULL || t == NULL) return;

    while (c_baseObject(t)->kind == M_TYPEDEF) {
        t = c_typeDef(t)->alias;
    }
    switch (c_baseObject(t)->kind) {
    case M_CLASS:
    case M_INTERFACE:
    case M_ANNOTATION:
        CHECKOBJECT(*p);
        trashcan->references = c_iterAppend(trashcan->references, c_object(*p));
    break;
    case M_BASE:
    case M_COLLECTION:
        if ((c_collectionTypeKind(t) == OSPL_C_ARRAY) &&
            (c_collectionTypeMaxSize(t) != 0))
        {
            walkReferences((c_metaObject)t, p, trashcan);
        } else {
            CHECKOBJECT(*p);
            trashcan->references = c_iterAppend(trashcan->references, c_object(*p));
        }
    break;
    case M_EXCEPTION:
    case M_STRUCTURE:
    case M_UNION:
        walkReferences(c_metaObject(type),p, trashcan);
    break;
    case M_PRIMITIVE:
    break;
    default:
    break;
    }
}

void
c_scopeClean(
    c_scope scope)
{
    ddsrt_avl_cfree_arg (&c_scope_bindings_td, &scope->bindings, c_bindingFreeWrapper, MM (scope));
}


void
c_clear (
    c_collection c)
{
    c_type type;
    c_object o;
    C_STRUCT(c_query) *q;

    if (c == NULL) return;
    type = c__getType(c);
    type = c_typeActualType(type);
    assert(type != NULL);
    assert(c_baseObject(type)->kind == M_COLLECTION);

    switch (c_collectionType(type)->kind) {
    case OSPL_C_DICTIONARY:
        while ((o = c_take(c)) != NULL) {
            c_free(o);
        }
        c_free(c_table(c)->key);
        c_free(c_table(c)->cursor);
    break;
    case OSPL_C_SET:
        while ((o = c_take(c)) != NULL) {
            c_free(o);
        }
    break;
    case OSPL_C_BAG:
        while ((o = c_take(c)) != NULL) {
            c_free(o);
        }
    break;
    case OSPL_C_LIST:
        while ((o = c_take(c)) != NULL) {
            c_free(o);
        }
    break;
    case OSPL_C_QUERY:
        q = c_query(c);
        c_free(q->pred);
    break;
    case OSPL_C_STRING:
    break;
    case OSPL_C_SCOPE:
        c_scopeClean((c_scope)c);
    break;
    default:
        // OS_REPORT(OS_ERROR,
        //             "Database Collection",0,
        //             "c_walk: illegal collection kind (%d) specified",
        //             c_collectionType(type)->kind);
        assert(FALSE);
    break;
    }
}


c_object
c_take(
    c_collection c)
{
    c_type type;

    C_COLLECTION_CHECK(c, c_take);

    type = c_typeActualType(c__getType(c));
    switch(c_collectionType(type)->kind) {
        case OSPL_C_QUERY:      return c_queryTakeOne(c_query(c),NULL);
        case OSPL_C_DICTIONARY: return c_tableTakeOne(c,NULL);
        case OSPL_C_SET:        return c_setTakeOne(c,NULL);
        case OSPL_C_BAG:        return c_bagTakeOne(c,NULL);
        case OSPL_C_LIST:       return c_listTakeOne(c,NULL);
        default:
            // OS_REPORT(OS_ERROR,"Database Collection",0,
            //             "c_take: illegal collection kind (%d) specified",
            //             c_collectionType(type)->kind);
            assert(FALSE);
        break;
    }
    return NULL;
}

c_bool readOne(c_object o, c_voidp arg)
{
    *(c_object *)arg = c_keep(o);
    return 0;
}

c_object
c_queryTakeOne (
    C_STRUCT(c_query) *b,
    c_qPred q)
{
    c_object o = NULL;

    assert(c_collectionIsType((c_query)b, OSPL_C_QUERY));

    c_queryTake(b,q,readOne,&o);
    return o;
}

c_bool
c_collectionIsType(
    c_collection c,
    c_collKind kind)
{
    c_type type = c__getType(c);

    type = c_typeActualType(type);
    if (c_baseObject(type)->kind != M_COLLECTION) {
        return FALSE;
    }
    return c_collectionType(type)->kind == kind;
}

c_bool
c_queryTake(
    C_STRUCT(c_query) *query,
    c_qPred q,
    c_action action,
    c_voidp arg)
{
    C_STRUCT(queryReadActionArg) a;
    c_qPred pred;
    c_collection source;
    c_type type;

    assert(c_collectionIsType((c_query)query, OSPL_C_QUERY));

    pred = query->pred;
    source = query->source;
    type = c__getType(source);
    type = c_typeActualType(type);

    a.action = action;
    a.arg = arg;
    a.predicate = q;

    switch(c_collectionType(type)->kind) {
    case OSPL_C_QUERY:      return c_queryTake(c_query(source),pred,queryReadAction,&a);
    case OSPL_C_DICTIONARY: return c_tableTake(source,pred,queryReadAction,&a);
    case OSPL_C_SET:        return c_setTake(source,pred,queryReadAction,&a);
    case OSPL_C_BAG:        return c_bagTake(source,pred,queryReadAction,&a);
    case OSPL_C_LIST:       return c_listTake(source,pred,queryReadAction,&a);
    default:
        // OS_REPORT(OS_ERROR,"Database Collection",0,
        //             "c_queryTake: illegal collection kind (%d) specified",
        //             c_collectionType(type)->kind);
        assert(FALSE);
    break;
    }
    return NULL;
}

c_bool
queryReadAction (
    c_object o,
    c_voidp arg)
{
    queryReadActionArg a = (queryReadActionArg)arg;
    c_bool proceed = TRUE;
    if (c_qPredEval(a->predicate,o)) {
        proceed = a->action(o,a->arg);
    }
    return proceed;
}

c_bool
c_qPredEval (
    c_qPred q,
    c_object o)
{
    c_ulong i,nrOfKeys;
    c_bool dontStop;
    c_value v;

    if (q == NULL) {
        return TRUE;
    }
    while (q != NULL) {
        i = 0; dontStop = TRUE;
        nrOfKeys = c_arraySize(q->keyField);
        while ((i<nrOfKeys) && dontStop) {
            dontStop = c_qKeyEval(q->keyField[i],o);
            i++;
        }
        if (dontStop) {
            if (q->expr == NULL) {
              return TRUE;
            }
            v = c_qValue(q->expr,o);
            assert(v.kind == V_BOOLEAN);
            return v.is.Boolean;
        }
        q = q->next;
    }
    return FALSE;
}

c_bool
c_qKeyEval(
    c_qKey key,
    c_object o)
{
    c_value v;
    c_ulong i,length;
    c_qRange r;
    c_bool rangeResult;


    if (key == NULL) {
        return TRUE; /* open range */
    }
    if (key->expr != NULL) {
        v = c_qValue(key->expr,o);
        assert(v.kind == V_BOOLEAN);
        if (!v.is.Boolean) {
            return FALSE;
        }
    }
    if (key->range != NULL) {
        v = c_fieldValue(key->field,o);
        length = c_arraySize(key->range);
        for (i=0;i<length;i++) {
            r = key->range[i];
            rangeResult = TRUE;
            switch (r->startKind) {
            case B_UNDEFINED:
            break;
            case B_INCLUDE:
                if (c_valueCompare(v,r->start) == C_LT) {
                    rangeResult = FALSE;
                }
            break;
            case B_EXCLUDE:
                if (c_valueCompare(v,r->start) != C_GT) {
                    rangeResult = FALSE;
                }
            break;
            default:
                assert(FALSE);
            break;
            }
            if (rangeResult) {
                switch (r->endKind) {
                case B_UNDEFINED:
                break;
                case B_INCLUDE:
                    if (c_valueCompare(v,r->end) == C_GT) {
                        rangeResult = FALSE;
                    }
                break;
                case B_EXCLUDE:
                    if (c_valueCompare(v,r->end) != C_LT) {
                        rangeResult = FALSE;
                    }
                break;
                default:
                    assert(FALSE);
                break;
                }
            }
            if (rangeResult) {
                c_valueFreeRef(v);
                return TRUE;
            }
        }
        c_valueFreeRef(v);
        return FALSE;
    }
    return TRUE;
}

c_value
c_qValue(
    c_qExpr q,
    c_object o)
{
    c_value v;
    c_value rightValue;
    c_value leftValue;
    c_ulong i,size;

#define _RVAL_(q,o) \
        (c_qValue(c_qFunc(q)->params[0],o))

#define _LVAL_(q,o) \
        (c_qValue(c_qFunc(q)->params[1],o))

#define _CASE_(l,q,o) \
        case CQ_##l: \
            rightValue = _RVAL_(q,o);\
            leftValue = _LVAL_(q,o);\
            v = c_valueCalculate(rightValue,leftValue,O_##l);\
            c_valueFreeRef(rightValue);\
            c_valueFreeRef(leftValue);\
            return v

    switch(q->kind) {
    case CQ_FIELD:
        v = c_fieldValue(c_qField(q)->field,o);
    break;
    case CQ_CONST:
        v = c_valueKeepRef(c_qConst(q)->value);
    break;
    case CQ_TYPE:
       v = c_objectValue(c_qType(q)->type);
    break;
    _CASE_(EQ,q,o);
    _CASE_(NE,q,o);
    _CASE_(LT,q,o);
    _CASE_(LE,q,o);
    _CASE_(GT,q,o);
    _CASE_(GE,q,o);
    case CQ_AND:
        v = _LVAL_(q,o);

        if (v.is.Boolean) {
            v = _RVAL_(q,o);
        }
    break;
    case CQ_OR:
        v = _LVAL_(q,o);

        if (!v.is.Boolean) {
            v = _RVAL_(q,o);
        }
    break;
    case CQ_NOT:
        v = _RVAL_(q,o);

        if (v.is.Boolean) {
            v.is.Boolean = FALSE;
        } else {
            v.is.Boolean = TRUE;
        }
    break;
    case CQ_LIKE:
        leftValue = _LVAL_(q,o);
        rightValue = _RVAL_(q,o);
        v = c_valueStringMatch(leftValue,rightValue);
        c_valueFreeRef(leftValue);
        c_valueFreeRef(rightValue);
        return v;
    case CQ_CALLBACK: return qExecute(q,o);
    default:
        v = c_qValue(c_qFunc(q)->params[0],o);
        size = c_arraySize(c_qFunc(q)->params);
        for (i=1;i<size;i++) {
            leftValue = v;
            rightValue = c_qValue(c_qFunc(q)->params[i],o);
            v = c_valueCalculate(leftValue,rightValue,(c_operator)q->kind);
            c_valueFreeRef(rightValue);
        }
        assert(i>1);
        c_valueFreeRef(leftValue);
    }
#undef _CASE_
#undef _LVAL_
#undef _RVAL_
    return v;
}

void c_iterFree(c_iter iter)
{
    if (iter != NULL) {
        c__iterImpl_free(&iter->x);
        ddsrt_free(iter);
    }
}

c_iter c_iterAppend(c_iter iter, void *object)
{
    if (iter == NULL) {
        return c_iterNew(object);
    } else {
        if (object != NULL) {
            c__iterImpl_append(&iter->x, object);
        }
        return iter;
    }
}

void
c_scopeWalk(
    c_scope scope,
    c_scopeWalkAction action,
    c_scopeWalkActionArg actionArg)
{
    c_binding binding;

    binding = scope->headInsOrder;
    while(binding) {
        action(binding->object, actionArg);
        binding = binding->nextInsOrder;
    }
}

c_bool
c_walk(
    c_collection c,
    c_action action,
    c_voidp actionArg)
{
    c_type type;

    if(c == NULL){
        return TRUE;
    }
    C_COLLECTION_CHECK(c, c_walk);

    type = c_typeActualType(c__getType(c));
    switch(c_collectionType(type)->kind) {
        case OSPL_C_DICTIONARY: return c_tableWalk((c_table)c,action,actionArg);
        case OSPL_C_SET:        return c_setWalk((c_set)c,action,actionArg);
        case OSPL_C_BAG:        return c_bagWalk((c_bag)c,action,actionArg);
        case OSPL_C_LIST:       return c_listWalk((c_list)c,action,actionArg);
        case OSPL_C_QUERY:      return c_queryRead(c_query(c),NULL,action,actionArg);
        default:
            // OS_REPORT(OS_ERROR,"Database Collection",0,
            //             "c_walk: illegal collection kind (%d) specified",
            //             c_collectionType(type)->kind);
            assert(FALSE);
        break;
    }
    return FALSE;
}

c_bool
c_tableWalk(
    c_table _this,
    c_action action,
    c_voidp actionArg)
{
    C_STRUCT(c_table) *table = (C_STRUCT(c_table) *) _this;
    struct c_tableWalkActionArgs walkActionArg;
    c_bool result = TRUE;

    assert(c_collectionIsType(_this, OSPL_C_DICTIONARY));

    _READ_BEGIN_(table);
    if (table->count > 0) {
        if ((table->key == NULL) || (c_arraySize(table->key) == 0)) {
            result = action(table->contents.object,actionArg);
        } else {
            walkActionArg.action = action;
            walkActionArg.actionArg = actionArg;
            walkActionArg.nrOfKeys = c_arraySize(table->key) - 1;
            result = tableReadTakeWalk(&table->contents.tree, c_tableWalkAction, &walkActionArg);
        }
    }
    _READ_END_(table);
    return result;
}

c_bool tableReadTakeWalk (ddsrt_avl_tree_t *tree, c_bool (*action) (), void *actionarg)
{
    ddsrt_avl_iter_t it;
    c_tableNode *n;
    c_bool proceed = TRUE;
    for (n = ddsrt_avl_iter_first (&c_table_td, tree, &it);
         n && proceed;
         n = ddsrt_avl_iter_next (&it)) {
        proceed = action (n, actionarg);
    }
    return proceed;
}

int tableCmp (const void *va, const void *vb)
{
    const c_value *a = va;
    const c_value *b = vb;
    return (int) c_valueCompare (*a, *b);
}

c_bool
c_tableWalkAction (
    c_tableNode n,
    c_tableWalkActionArg arg)
{
    c_bool result;

    if (arg->nrOfKeys == 0) {
        result = arg->action(n->contents.object,arg->actionArg);
    } else {
        arg->nrOfKeys--;
        result = tableReadTakeWalk(&n->contents.tree,c_tableWalkAction,arg);
        arg->nrOfKeys++;
    }
    return result;
}


c_object
c_tableTakeOne(
    c_table _this,
    c_qPred p)
{
    c_object o = NULL;

    assert(c_collectionIsType(_this, OSPL_C_DICTIONARY));

    c_tableTake(_this,p,readOne,&o);

    return o;
}

c_bool
c_tableTake (
    c_table _this,
    c_qPred p,
    c_action action,
    c_voidp arg)
{
    C_STRUCT(c_table) *table = (C_STRUCT(c_table) *) _this;
    C_STRUCT(tableTakeActionArg) a;
    C_STRUCT(c_tableNode) root;
    c_bool proceed = TRUE;

    assert(c_collectionIsType(_this, OSPL_C_DICTIONARY));

    _ACCESS_BEGIN_(table);

    if (table->key == NULL || c_arraySize(table->key) == 0) {
        if (table->contents.object != NULL) {
            c_object o = NULL;
            /* FIXME: this can't be right: action is called regardless
             * of whether the predicate is satisfied
             */
            if (predMatches (p, table->contents.object)) {
                o = table->contents.object;
                table->contents.object = NULL;
                table->count--;
            }
            proceed = action (o, arg);
            c_free (o);
        }
    } else {
        a.mm = MM(table);
        a.key = table->key;
        a.action = action;
        a.arg = arg;
        a.count = 0;
        a.proceed = TRUE;
#ifdef _CONSISTENCY_CHECKING_
        a.t = table;
        root.table = (c_voidp)table;
#endif
        root.contents = table->contents;
        if (p == NULL) {
            a.keyIndex = 0;
            a.pred = p;
            a.disposed = NULL;
            proceed = tableTakeAction(&root,&a);
        }
        while ((p != NULL) && proceed) {
            a.keyIndex = 0;
            a.pred = p;
            a.disposed = NULL;
            proceed = tableTakeAction(&root,&a);
            p = p->next;
        }
        table->contents = root.contents;
        table->count -= a.count;
    }
    _ACCESS_END_(table);
    return proceed;
}


c_bool predMatches (c_qPred qp, c_object o)
{
    if (qp == NULL) {
        return TRUE;
    } else {
        do {
            if (c_qPredEval (qp, o)) {
                return TRUE;
            }
            qp = qp->next;
        } while (qp);
        return FALSE;
    }
}

c_bool tableTakeActionWalkRange (c_tableNode n, tableTakeActionArg arg, const c_value *startRef, c_bool startInclude, const c_value *endRef, c_bool endInclude)
{
    c_bool proceed;
    do {
        proceed = tableReadTakeRangeWalk(&n->contents.tree,
                                         startRef,startInclude,
                                         endRef,endInclude,
                                         tableTakeAction,arg);
        if ((!proceed) && (arg->disposed != NULL)) {
            tableTakeActionDeleteDisposed (n, arg);
        }
    } while ((proceed == FALSE) && (arg->proceed == TRUE));
    return arg->proceed;
}


c_bool
tableTakeAction(
    c_tableNode n,
    tableTakeActionArg arg)
{
    c_value start, end;
    c_value *startRef = NULL, *endRef = NULL;
    c_bool startInclude = TRUE, endInclude = TRUE;
    c_bool proceed = TRUE;
    c_qKey key;
    c_ulong i, nrOfRanges;

    _CHECK_CONSISTENCY_(arg->t,n);

    if (tableReadTakeActionNonMatchingKey (n, arg->pred, arg->keyIndex)) {
        return TRUE;
    } else if (arg->keyIndex == c_arraySize (arg->key)) {
        if (!predMatches (arg->pred, n->contents.object)) {
            return TRUE;
        } else {
            arg->disposed = n;
            arg->count++;
            arg->proceed = arg->action (n->contents.object, arg->arg);
            return FALSE;
        }
    } else {
        key = arg->pred ? arg->pred->keyField[arg->keyIndex] : NULL;
        arg->keyIndex++;
        if (key == NULL || key->range == NULL || (nrOfRanges = c_arraySize (key->range)) == 0) {
            proceed = tableTakeActionWalk (n, arg);
        } else {
            for (i = 0; i < nrOfRanges && proceed; i++) {
                tableReadTakeActionGetRange (key->range[i], &start, &startRef, &startInclude, &end, &endRef, &endInclude);
                proceed = tableTakeActionWalkRange (n, arg, startRef, startInclude, endRef, endInclude);
            }
        }
        if (ddsrt_avl_is_empty (&n->contents.tree)) {
            arg->disposed = n;
        } else {
            arg->disposed = NULL;
        }
        arg->keyIndex--;
        return proceed;
    }
}

c_bool
tableReadTakeActionNonMatchingKey (
    c_tableNode n,
    c_qPred query,
    c_ulong keyIndex)
{
    /* FIXME: this can't be right: for any key but the one nested most
     * deeply, N doesn't actually point to an object.  Hence any table
     * with multi-level keys can be tricked into returning the wrong
     * result or crashes (in practice, only if the key is a string)
     */
    if ((keyIndex > 0) && (query != NULL)) {
        c_qKey key = query->keyField[keyIndex-1];
        if (key->expr != NULL) {
            c_value v = c_qValue(key->expr,n->contents.object);
            assert(v.kind == V_BOOLEAN);
            if (!v.is.Boolean) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

c_bool tableTakeActionWalk (c_tableNode n, tableTakeActionArg arg)
{
    c_bool proceed;
    do {
        proceed = tableReadTakeWalk(&n->contents.tree, tableTakeAction, arg);
        if ((!proceed) && (arg->disposed != NULL)) {
            tableTakeActionDeleteDisposed (n, arg);
        }
    } while ((proceed == FALSE) && (arg->proceed == TRUE));
    return arg->proceed;
}

void tableTakeActionDeleteDisposed (c_tableNode n, tableTakeActionArg arg)
{
    c_tableNode d = arg->disposed;
    ddsrt_avl_delete (&c_table_td, &n->contents.tree, d);
    if (arg->keyIndex == c_arraySize(arg->key)) {
        c_free(d->contents.object);
    }
    c_valueFreeRef(d->keyValue);
    c_mmFree(arg->mm, d);
}

c_object
c_setTakeOne (
    c_set _this,
    c_qPred q)
{
    c_object o;
    c_object found;

    assert(c_collectionIsType(_this, OSPL_C_SET));

    o = c_setReadOne(_this,q);
    if (o == NULL) {
        return NULL;
    }
    found = c_setRemove(_this,o,NULL,NULL);
    assert(found == o);
    c_free(found);

    return o;
}

c_object
c_setReadOne (
    c_set _this,
    c_qPred q)
{
    c_object o = NULL;

    assert(c_collectionIsType(_this, OSPL_C_SET));

    c_setRead(_this,q,readOne,&o);
    return o;
}

c_bool
c_setRead(
    c_set _this,
    c_qPred q,
    c_action action,
    c_voidp arg)
{
    C_STRUCT(c_set) *set = (C_STRUCT(c_set) *) _this;
    ddsrt_avl_citer_t it;
    c_setNode n;
    c_bool proceed;
    assert(c_collectionIsType(_this, OSPL_C_SET));
    proceed = TRUE;
    for (n = ddsrt_avl_citer_first (&c_set_td, &set->tree, &it);
         n != NULL && proceed;
         n = ddsrt_avl_citer_next (&it)) {
        if (predMatches (q, n->object)) {
            proceed = action (n->object, arg);
        }
    }
    return proceed;
}

int ptrCompare(const void *a, const void *b)
{
    return (a == b) ? 0 : (a < b) ? -1 : 1;
}


c_object
c_bagTakeOne (
    c_bag _this,
    c_qPred q)
{
    c_object o;
    c_object found;

    assert(c_collectionIsType(_this, OSPL_C_BAG));

    o = c_bagReadOne(_this,q);
    if (o == NULL) {
        return NULL;
    }
    found = c_bagRemove(_this,o,NULL,NULL);
    assert(found == o);
    c_free(found);

    return o;
}

c_object
c_bagReadOne (
    c_bag _this,
    c_qPred q)
{
    c_object o = NULL;

    assert(c_collectionIsType(_this, OSPL_C_BAG));

    c_bagRead(_this,q,readOne,&o);
    return o;
}

c_bool
c_bagRead(
    c_bag _this,
    c_qPred q,
    c_action action,
    c_voidp arg)
{
    C_STRUCT(c_bag) *bag = (C_STRUCT(c_bag) *) _this;
    ddsrt_avl_iter_t it;
    c_bagNode n;
    c_bool proceed;
    assert(c_collectionIsType(_this, OSPL_C_BAG));
    proceed = TRUE;
    for (n = ddsrt_avl_iter_first (&c_bag_td, &bag->tree, &it);
         n != NULL && proceed;
         n = ddsrt_avl_iter_next (&it)) {
        if (predMatches (q, n->object)) {
            c_long i;
            for (i = 0; i < n->count && proceed; i++) {
                proceed = action (n->object, arg);
            }
        }
    }
    return proceed;
}

c_object
c_bagRemove (
    c_bag _this,
    c_object o,
    c_removeCondition condition,
    c_voidp arg)
{
    C_STRUCT(c_bag) *bag = (C_STRUCT(c_bag) *) _this;
    ddsrt_avl_dpath_t p;
    c_bagNode f;
    assert(c_collectionIsType(_this, OSPL_C_BAG));
    if ((f = ddsrt_avl_lookup_dpath (&c_bag_td, &bag->tree, o, &p)) == NULL) {
        return NULL;
    } else if (condition && !condition (f->object, o, arg)) {
        return NULL;
    } else {
        c_object obj = f->object;
        if (--f->count == 0) {
            ddsrt_avl_delete_dpath (&c_bag_td, &bag->tree, f, &p);
            c_mmFree (bag->mm, f);
        }
        bag->count--;
        return obj;
    }
}

c_bool
c_setWalk(
    c_set _this,
    c_action action,
    c_voidp actionArg)
{
    C_STRUCT(c_set) *set = (C_STRUCT(c_set) *) _this;
    ddsrt_avl_citer_t it;
    c_setNode n;
    c_bool proceed;
    assert(c_collectionIsType(_this, OSPL_C_SET));
    proceed = TRUE;
    for (n = ddsrt_avl_citer_first (&c_set_td, &set->tree, &it);
         n != NULL && proceed;
         n = ddsrt_avl_citer_next (&it)) {
        proceed = action (n->object, actionArg);
    }
    return proceed;
}

c_bool
c_bagWalk(
    c_bag _this,
    c_action action,
    c_voidp actionArg)
{
    C_STRUCT(c_bag) *bag = (C_STRUCT(c_bag) *) _this;
    ddsrt_avl_iter_t it;
    c_bagNode n;
    c_bool proceed;
    assert(c_collectionIsType(_this, OSPL_C_BAG));
    proceed = TRUE;
    for (n = ddsrt_avl_iter_first (&c_bag_td, &bag->tree, &it);
         n != NULL && proceed;
         n = ddsrt_avl_iter_next (&it)) {
        c_long i;
        for (i = 0; i < n->count && proceed; i++) {
            proceed = action (n->object, actionArg);
        }
    }
    return proceed;
}

c_bool c_listWalk(c_list _this, c_action action, c_voidp actionArg)
{
    struct c_list_s *l = (struct c_list_s *) _this;
    struct c__listImpl_iter it;
    c_object o1;
    for (o1 = c__listImpl_iter_first(&l->x, &it); o1 != NULL; o1 = c__listImpl_iter_next(&it)) {
        if (!action(o1, actionArg)) {
            return 0;
        }
    }
    return 1;
}


c_object c_listTakeOne(c_list _this, c_qPred q)
{
    c_object o = NULL;
    c_listTake(_this, q, takeOne, &o);
    return o;
}

c_bool c_listTake(c_list _this, c_qPred q, c_action action, c_voidp arg)
{
    struct c_list_s *l = (struct c_list_s *) _this;
    struct c__listImpl_iter_d it;
    c_object o1;
    for (o1 = c__listImpl_iter_d_first(&l->x, &it); o1 != NULL; o1 = c__listImpl_iter_d_next(&it)) {
        if (predMatches(q, o1)) {
            c__listImpl_iter_d_remove(&it);
            if (!action(o1, arg)) {
                return 0;
            }
        }
    }
    return 1;
}

c_bool takeOne(c_object o, c_voidp arg)
{
    *(c_object *)arg = o;
    return 0;
}


c_bool
c_setTake (
    c_set _this,
    c_qPred q,
    c_action action,
    c_voidp arg)
{
    c_bool proceed = TRUE;
    c_object o;

    assert(c_collectionIsType(_this, OSPL_C_SET));

    while (proceed) {
        o = c_setTakeOne(_this,q);
        if (o != NULL) {
            proceed = action(o,arg);
        } else {
            proceed = FALSE;
        }
    }
    return proceed;
}

c_object
c_setRemove (
    c_set _this,
    c_object o,
    c_removeCondition condition,
    c_voidp arg)
{
    C_STRUCT(c_set) *set = (C_STRUCT(c_set) *) _this;
    ddsrt_avl_dpath_t p;
    c_setNode f;
    assert(c_collectionIsType(_this, OSPL_C_SET));
    if ((f = ddsrt_avl_clookup_dpath (&c_set_td, &set->tree, o, &p)) == NULL) {
        return NULL;
    } else if (condition && !condition (f->object, o, arg)) {
        return NULL;
    } else {
        c_object old = f->object;
        ddsrt_avl_cdelete_dpath (&c_set_td, &set->tree, f, &p);
        c_mmFree (set->mm, f);
        return old;
    }
}

void *get_field_address (c_field field, c_object o)
{
    void *p = o;
    c_ulong i,n;
    c_array refs;
    if (field->refs) {
        refs = field->refs;
        n = c_arraySize(refs);
        for(i = 0; i < n; i++) {
            if ((p = C_DISPLACE(p,refs[i])) == NULL) {
                return NULL;
            }
            p = *(c_voidp *)p;
        }
        if(p == NULL) {
            return NULL;
        }
    }
    p = C_DISPLACE(p,field->offset);
    return p;
}


c_bool
c_bagTake (
    c_bag _this,
    c_qPred q,
    c_action action,
    c_voidp arg)
{
    c_bool proceed = TRUE;
    c_object o;

    assert(c_collectionIsType(_this, OSPL_C_BAG));

    while (proceed) {
        o = c_bagTakeOne(_this,q);
        if (o != NULL) {
            proceed = action(o,arg);
        } else {
            proceed = FALSE;
        }
    }
    return proceed;
}


c_value
c_fieldValue(
    c_field field,
    c_object o)
{
    c_voidp p = get_field_address (field, o);
    c_value v;

    /* initialize v! variable v has to be initialized as parts of the
     * union might not be initialized.
     * Example:
     * kind = V_LONG;
     * is.Long = 1;
     * then the last 4 bytes have not been initialized,
     * since the double field is 8 bytes, making the
     * c_value structure effectively 12 bytes.
     * This causes a lot of MLK's in purify.
     */
#ifndef NDEBUG
    memset(&v, 0, sizeof(v));
#endif

    if (p == NULL) {
        v.kind = V_UNDEFINED;
        return v;
    }

#define _VAL_(f,t) v.is.f = *(t *)p

    v.kind = field->kind;

    switch(field->kind) {
    case V_ADDRESS:   _VAL_(Address,c_address); break;
    case V_BOOLEAN:   _VAL_(Boolean,c_bool); break;
    case V_SHORT:     _VAL_(Short,c_short); break;
    case V_LONG:      _VAL_(Long,c_long); break;
    case V_LONGLONG:  _VAL_(LongLong,c_longlong); break;
    case V_OCTET:     _VAL_(Octet,c_octet); break;
    case V_USHORT:    _VAL_(UShort,c_ushort); break;
    case V_ULONG:     _VAL_(ULong,c_ulong); break;
    case V_ULONGLONG: _VAL_(ULongLong,c_ulonglong); break;
    case V_CHAR:      _VAL_(Char,c_char); break;
    case V_WCHAR:     _VAL_(WChar,c_wchar); break;
    case V_STRING:    _VAL_(String,c_string); c_keep(v.is.String); break;
    case V_WSTRING:   _VAL_(WString,c_wstring); c_keep(v.is.WString); break;
    case V_FLOAT:     _VAL_(Float,c_float); break;
    case V_DOUBLE:    _VAL_(Double,c_double); break;
    case V_OBJECT:    _VAL_(Object,c_object); c_keep(v.is.Object); break;
    case V_VOIDP:     _VAL_(Voidp,c_voidp); break;
    case V_FIXED:
    case V_UNDEFINED:
    case V_COUNT:
        // OS_REPORT(OS_ERROR,"c_fieldAssign failed",0,
        //             "illegal field value kind (%d)", v.kind);
        assert(FALSE);
    break;
    }

    return v;

#undef _VAL_
}

typedef void
(*c_qCallback)(
    c_object o,
    c_value argument,
    c_value *result);

c_value
qExecute(
    c_qExpr e,
    c_object o)
{
    c_qCallback callback;
    c_value argument, result, v;

    assert(e->kind == CQ_CALLBACK);

    v = c_qValue(c_qFunc(e)->params[1],NULL);
    assert(v.kind == V_ADDRESS);
    callback = (c_qCallback)v.is.Address;
    argument = c_qValue(c_qFunc(e)->params[2],o);
    callback(o,argument,&result);
    return result;
}


c_bool
c_queryRead(
    C_STRUCT(c_query) *query,
    c_qPred q,
    c_action action,
    c_voidp arg)
{
    C_STRUCT(queryReadActionArg) a;
    c_qPred pred;
    c_collection source;
    c_type type;

    assert(c_collectionIsType((c_query)query, OSPL_C_QUERY));

    pred = query->pred;
    source = query->source;
    type = c__getType(source);
    type = c_typeActualType(type);

    a.action = action;
    a.arg = arg;
    a.predicate = q;

    switch(c_collectionType(type)->kind) {
    case OSPL_C_QUERY:      return c_queryRead(c_query(source),pred,queryReadAction,&a);
    case OSPL_C_DICTIONARY: return c_tableRead(source,pred,queryReadAction,&a);
    case OSPL_C_SET:        return c_setRead(source,pred,queryReadAction,&a);
    case OSPL_C_BAG:        return c_bagRead(source,pred,queryReadAction,&a);
    case OSPL_C_LIST:       return c_listRead(source,pred,queryReadAction,&a);
    default:
        // OS_REPORT(OS_ERROR,"Database Collection",0,
        //             "c_queryRead: illegal collection kind (%d) specified",
        //             c_collectionType(type)->kind);
        assert(FALSE);
    break;
    }
    return NULL;
}

c_bool c_listRead(c_list _this, c_qPred q, c_action action, c_voidp arg)
{
    struct c_list_s *l = (struct c_list_s *) _this;
    struct c__listImpl_iter it;
    c_object o1;
    assert(c_collectionIsType(_this, OSPL_C_LIST));
    for (o1 = c__listImpl_iter_first(&l->x, &it); o1 != NULL; o1 = c__listImpl_iter_next(&it)) {
        if (predMatches(q, o1)) {
            if (!action(o1, arg)) {
                return 0;
            }
        }
    }
    return 1;
}

c_bool
c_tableRead (
    c_table _this,
    c_qPred q,
    c_action action,
    c_voidp arg)
{
    C_STRUCT(c_table) *table = (C_STRUCT(c_table) *) _this;
    C_STRUCT(tableReadActionArg) a;
    C_STRUCT(c_tableNode) root;
    c_bool proceed = TRUE;

    assert(c_collectionIsType(_this, OSPL_C_DICTIONARY));

    _READ_BEGIN_(t);

    if ((table->key == NULL) || (c_arraySize(table->key) == 0)) {
        if (table->contents.object == NULL) {
            /* skip */
        } else if (predMatches (q, table->contents.object)) {
            proceed = action(table->contents.object,arg);
        }
        _READ_END_(table);
        return proceed;
    }

    root.contents = table->contents;
    a.key = table->key;
    a.action = action;
    a.arg = arg;
#ifdef _CONSISTENCY_CHECKING_
    a.t = table;
    root.table = (c_voidp)table;
#endif
    if (q == NULL) {
        a.keyIndex = 0;
        a.query = q;
        proceed = tableReadAction(&root,&a);
    } else {
        while ((q != NULL) && proceed) {
            a.keyIndex = 0;
            a.query = q;
            proceed = tableReadAction(&root,&a);
            q = q->next;
        }
    }
    _READ_END_(table);
    return proceed;
}


c_bool
tableReadAction(
    c_tableNode n,
    tableReadActionArg arg)
{
    c_value start, end;
    c_value *startRef = NULL, *endRef = NULL;
    c_bool startInclude = TRUE, endInclude = TRUE;
    c_bool proceed = TRUE;
    c_qKey key;
    c_ulong i,nrOfRanges;

    _CHECK_CONSISTENCY_(arg->t,n);

    if (tableReadTakeActionNonMatchingKey (n, arg->query, arg->keyIndex)) {
        return TRUE;
    } else if (arg->keyIndex == c_arraySize(arg->key)) {
        if (!predMatches (arg->query, n->contents.object)) {
            return TRUE;
        } else {
            return arg->action (n->contents.object, arg->arg);
        }
    } else {
        key = arg->query ? arg->query->keyField[arg->keyIndex] : NULL;
        arg->keyIndex++;
        if (key == NULL || key->range == NULL || (nrOfRanges = c_arraySize (key->range)) == 0) {
            proceed = tableReadTakeWalk (&n->contents.tree, tableReadAction, arg);
        } else {
            for (i = 0; i < nrOfRanges && proceed; i++) {
                tableReadTakeActionGetRange (key->range[i], &start, &startRef, &startInclude, &end, &endRef, &endInclude);
                proceed = tableReadTakeRangeWalk(&n->contents.tree,
                                                 startRef,startInclude,
                                                 endRef,endInclude,
                                                 tableReadAction,arg);
            }
        }
        arg->keyIndex--;
        return proceed;
    }
}

void
tableReadTakeActionGetRange (
    c_qRange range,
    c_value *start,
    c_value **startRef,
    c_bool *startInclude,
    c_value *end,
    c_value **endRef,
    c_bool *endInclude)
{
    if (range == NULL) {
        *startRef = NULL; *startInclude = TRUE;
        *endRef   = NULL; *endInclude   = TRUE;
    } else {
        *start = c_qRangeStartValue(range);
        *end = c_qRangeEndValue(range);
        switch (range->startKind) {
        case B_UNDEFINED: *startRef = NULL;  *startInclude = TRUE;  break;
        case B_INCLUDE:   *startRef = start; *startInclude = TRUE;  break;
        case B_EXCLUDE:   *startRef = start; *startInclude = FALSE; break;
        default:
            // OS_REPORT(OS_ERROR,
            //             "Database Collection",0,
            //             "Internal error: undefined range kind %d",
            //             range->startKind);
            assert(FALSE);
        }
        switch (range->endKind) {
        case B_UNDEFINED: *endRef = NULL; *endInclude = TRUE;  break;
        case B_INCLUDE:   *endRef = end;  *endInclude = TRUE;  break;
        case B_EXCLUDE:   *endRef = end;  *endInclude = FALSE; break;
        default:
            // OS_REPORT(OS_ERROR,
            //             "Database Collection",0,
            //             "Internal error: undefined range kind %d",
            //             range->endKind);
            assert(FALSE);
        }
    }
}


c_bool
tableReadTakeRangeWalk (
    ddsrt_avl_tree_t *tree,
    const c_value *start,
    int include_start,
    const c_value *end,
    int include_end,
    c_bool (*action) (),
    void *actionarg)
{
    ddsrt_avl_iter_t it;
    c_tableNode n, endn;
    c_bool proceed = TRUE;
    /* Starting point for iteration */
    if (start == NULL) {
        n = ddsrt_avl_iter_first (&c_table_td, tree, &it);
    } else if (include_start) {
        n = ddsrt_avl_iter_succ_eq (&c_table_td, tree, &it, start);
    } else {
        n = ddsrt_avl_iter_succ (&c_table_td, tree, &it, start);
    }
    /* Don't bother looking at the end of the range if there's no
     * matching data
     */
    if (n == NULL) {
        return proceed;
    }
    /* Endpoint of the iteration: if N is outside the range, abort,
     * else look up the first node beyond the range
     */
    if (end == NULL) {
        endn = NULL;
    } else if (include_end) {
        if (tableCmp (&n->keyValue, end) > 0) {
            return proceed;
        }
        endn = ddsrt_avl_lookup_succ (&c_table_td, tree, end);
    } else {
        if (tableCmp (&n->keyValue, end) >= 0) {
            return proceed;
        }
        endn = ddsrt_avl_lookup_succ_eq (&c_table_td, tree, end);
    }
    /* Then, just iterate starting at n until endn is reached */
    while (n != endn && proceed) {
        proceed = action (n, actionarg);
        n = ddsrt_avl_iter_next (&it);
    }
    return proceed;
}

void
cf_elementFree (
    cf_element element)
{
    assert(element != NULL);

    cf_nodeFree(cf_node(element));
}

void
cf_nodeFree (
    cf_node node)
{
    assert(node != NULL);

    if (node != NULL) {
        switch (node->kind) {
        case CF_ELEMENT: cf_elementDeinit(cf_element(node)); break;
        case CF_DATA: cf_dataDeinit(cf_data(node)); break;
        case CF_ATTRIBUTE: cf_attributeDeinit(cf_attribute(node)); break;
        case CF_NODE: /* abstract */
        default:
            assert (FALSE); /* catch undefined behaviour */
            break;
        }
        ddsrt_free(node);
    }
}

void
cf_elementDeinit (
    cf_element element)
{
    assert(element != NULL);
    
    if (element->attributes != NULL) {
        cf_nodeListClear(element->attributes);
        cf_nodeListFree(element->attributes);
        element->attributes = NULL;
    }
    if (element->children != NULL) {
        cf_nodeListClear(element->children);
        cf_nodeListFree(element->children);
        element->children = NULL;
    }

    cf_nodeDeinit(cf_node(element));
}

void
cf_nodeListClear(
    cf_nodeList list)
{
    c_long i;

    assert(list != NULL);
    for (i=0; i<list->nrNodes; i++) {
      cf_nodeFree(list->theList[i]);
    }
    list->nrNodes = 0;
}

void
cf_nodeListFree(
    cf_nodeList list)
{
    assert(list != NULL);

    if (list->theList != NULL) {
        ddsrt_free(list->theList);
        list->theList = NULL;
    }
    list->nrNodes = 0;
    list->maxNrNodes = 0;
    ddsrt_free(list);
};


void
cf_attributeDeinit (
    cf_attribute attribute)
{
    assert(attribute != NULL);

    switch (attribute->value.kind) {
    case V_BOOLEAN:
    case V_OCTET:
    case V_SHORT:
    case V_LONG:
    case V_LONGLONG:
    case V_USHORT:
    case V_ULONG:
    case V_ULONGLONG:
    case V_FLOAT:
    case V_DOUBLE:
    case V_CHAR:
    break;
    case V_STRING:
        ddsrt_free(attribute->value.is.String);
    break;
    case V_WCHAR:
    case V_WSTRING:
    case V_FIXED:
    case V_OBJECT:
    case V_UNDEFINED:
    case V_COUNT:
    default:
        assert(0); /* catch undefined attribute */
    break;
    }

    cf_nodeDeinit(cf_node(attribute));
}

void
cf_dataDeinit (
    cf_data data)
{
    assert(data != NULL);

    switch (data->value.kind) {
    case V_BOOLEAN:
    case V_OCTET:
    case V_SHORT:
    case V_LONG:
    case V_LONGLONG:
    case V_USHORT:
    case V_ULONG:
    case V_ULONGLONG:
    case V_FLOAT:
    case V_DOUBLE:
    case V_CHAR:
      /* nothing to free */;
    break;
    case V_STRING:
        ddsrt_free(data->value.is.String);
    break;
    case V_WCHAR:
    case V_WSTRING:
    case V_FIXED:
    case V_OBJECT:
    case V_UNDEFINED:
    case V_COUNT:
    default:
        assert(0); /* catch undefined behaviour */
    break;
    }
    data->value.kind = V_UNDEFINED;

    cf_nodeDeinit(cf_node(data));
}

void
cf_nodeDeinit (
    cf_node node)
{
    assert(node != NULL);

    ddsrt_free(node->name);
}


cf_element
cf_elementNew (
    const c_char *tagName)
{
    cf_element el;

    assert(tagName != NULL);

    el = cf_element(ddsrt_malloc((uint32_t)C_SIZEOF(cf_element)));
    cf_elementInit(el, tagName);

    return el;
}

void
cf_elementInit (
    cf_element element,
    const c_char *tagName)
{
    assert(element != NULL);
    assert(tagName != NULL);

    cf_nodeInit(cf_node(element), CF_ELEMENT, tagName);

    element->attributes = cf_nodeListNew();
    element->children = cf_nodeListNew();
}

void
cf_nodeInit (
    cf_node node,
    cf_kind kind,
    const c_char *name)
{
    assert(node != NULL);
    assert(name != NULL);

    node->kind = kind;
    node->name = ddsrt_strdup(name);
}

cf_nodeList
cf_nodeListNew()
{
    cf_nodeList list;

    list = (cf_nodeList)ddsrt_malloc((uint32_t)C_SIZEOF(cf_nodeList));

    list->maxNrNodes = 0;
    list->nrNodes = 0;
    list->theList = NULL;

    return list;
}

cf_attribute
cf_elementAddAttribute (
    cf_element element,
    cf_attribute attribute)
{
    cf_attribute attr;

    assert(element != NULL);
    assert(attribute != NULL);

    attr = cf_attribute(cf_nodeListInsert(element->attributes, 
                                          cf_node(attribute)));

    return attr;
}

c_object
cf_nodeListInsert(
    cf_nodeList list,
    cf_node o)
{
    cf_node *newList;
   
    assert(list != NULL);

    if (list->nrNodes == list->maxNrNodes) {
      list->maxNrNodes += LIST_BLOCKSIZE;
      newList = (cf_node *)ddsrt_malloc((uint32_t)(list->maxNrNodes * (int)sizeof(cf_node)));
      memcpy(newList, list->theList, 
             (size_t)((size_t)(list->maxNrNodes - LIST_BLOCKSIZE) * sizeof(cf_node)));
      if (list->theList != NULL) {
          ddsrt_free(list->theList);
      }
      list->theList = newList;
    }

    list->nrNodes++;
    list->theList[list->nrNodes - 1] = o;

    return NULL;
}


cf_attribute
cf_attributeNew (
    const c_char *name,
    c_value value)
{
    cf_attribute attr;

    assert(name != NULL);

    attr = cf_attribute(ddsrt_malloc((uint32_t)C_SIZEOF(cf_attribute)));
    cf_attributeInit(attr, name, value);

    return attr;
}


void
cf_attributeInit (
    cf_attribute attribute,
    const c_char *name,
    c_value value)
{
    assert(attribute != NULL);
    assert(name != NULL);

    cf_nodeInit(cf_node(attribute), CF_ATTRIBUTE, name);
    attribute->value.kind = value.kind;
    switch (value.kind) {
    case V_BOOLEAN:
    case V_OCTET:
    case V_SHORT:
    case V_LONG:
    case V_LONGLONG:
    case V_USHORT:
    case V_ULONG:
    case V_ULONGLONG:
    case V_FLOAT:
    case V_DOUBLE:
    case V_CHAR:
        attribute->value.is = value.is;
    break;
    case V_STRING:
        attribute->value.is.String = ddsrt_strdup(value.is.String);
    break;
    case V_WCHAR:
    case V_WSTRING:
    case V_FIXED:
    case V_OBJECT:
    case V_UNDEFINED:
    case V_COUNT:
    default:
        attribute->value.kind = V_UNDEFINED;
        assert(0); /* catch undefined attribute */
    break;
    }
}

cf_node
cf_elementAddChild(
    cf_element element,
    cf_node child)
{
    cf_node ch;

    assert(element != NULL);
    assert(child != NULL);

    ch = cf_node(cf_nodeListInsert(element->children, 
                                   cf_node(child)));

    return ch;
}

cf_data
cf_dataNew (
    c_value value)
{
    cf_data d;

    assert(value.kind != V_UNDEFINED);

    d = cf_data(ddsrt_malloc((uint32_t)C_SIZEOF(cf_data)));
    cf_dataInit(d, value);

    return d;
}


void
cf_dataInit (
    cf_data data,
    c_value value)
{
    assert(data != NULL);
    assert(value.kind != V_UNDEFINED);

    cf_nodeInit(cf_node(data), CF_DATA, CF_DATANAME);

    data->value.kind = value.kind;
    switch (value.kind) {
    case V_BOOLEAN:
    case V_OCTET:
    case V_SHORT:
    case V_LONG:
    case V_LONGLONG:
    case V_USHORT:
    case V_ULONG:
    case V_ULONGLONG:
    case V_FLOAT:
    case V_DOUBLE:
    case V_CHAR:
        data->value.is = value.is;
    break;
    case V_STRING: {
        char *tmp = ddsrt_strdup(value.is.String);
        char *trimmed;
        /* os_str_trim returns original str if no chars were trimmed */
        ////os_str_trim
        if ((trimmed = os_str_trim(tmp, NULL)) != tmp) {
           ddsrt_free(tmp);
        }
        data->value.is.String = trimmed;
        break;
    }
    case V_WCHAR:
    case V_WSTRING:
    case V_FIXED:
    case V_OBJECT:
    case V_UNDEFINED:
    case V_COUNT:
    default:
        data->value.kind = V_UNDEFINED;
        assert(0); /* catch undefined value */
    break;
    }
}

c_iterIter c_iterIterGet(c_iter i)
{
    c_iterIter result;
    memset(&result, 0, sizeof(result));
    if (i != NULL) {
        result.current = c__iterImpl_iter_first(&i->x, &result.it);
    }
    return result;
}

void *c_iterNext(c_iterIter* iterator)
{
    void *result = 0;
    if (iterator->current) {
        result = iterator->current;
        iterator->current = c__iterImpl_iter_next(&iterator->it);
    }
    return result;
}

os_cfg_handle *os_cfgRead(const char *uri)
{
   const os_URIListNode *current;
   int i;

   if ( uri == NULL || uri[0] == '\0' )
   {
       return NULL;
   }

   if ( strncmp(uri, fileURI, fileURILen ) == 0 )
   {
      return os_fileReadURI(uri);
   }
   else
   {
     for (i=0; (current=&os_cfg_cfgs[i])->uri != NULL; i++ )
     {
        if ( strncmp(current->uri, osplcfgURI, osplcfgURILen ) == 0 )
        {
           os_cfg_handle *handle = (os_cfg_handle *)ddsrt_malloc(sizeof *handle);
           handle->ptr = current->config;
           handle->isStatic=1;
           handle->size = current->size;
           return handle;
        }
     }
     return NULL;
   }
}

void os_cfgRelease(os_cfg_handle *cfg)
{
   if( ! cfg->isStatic )
   {
      ddsrt_free(cfg->ptr);
   }
   ddsrt_free(cfg);
}


os_cfg_handle *os_fileReadURI(const char *uri)
{
   os_cfg_file *cfg_file = os_fileOpenURI(uri);
   if ( cfg_file )
   {
      size_t count, r;
      os_cfg_handle *handle = (os_cfg_handle *)ddsrt_malloc(sizeof *handle);

      /* Determine worst-case size of the file-contents. On Windows ftell will
       * provide the number of characters available (including \r\n), but fread
       * will replace \r\n by \n, so fread may actually return less. */
      fseek(cfg_file, 0, SEEK_END);
      count = (size_t)ftell(cfg_file);
      fseek(cfg_file, 0, SEEK_SET);

      handle->ptr = ddsrt_malloc(count + 2);

      /* On Windows fread may return less due to \r\n replacements, so NULL-terminate
       * the resulting string on position r instead of count. */
      r = fread(handle->ptr, 1, count, cfg_file);

      /* Double null-terminate the buffer should anyone which to use yy_scan_buffer
       * on the result. */
      handle->ptr[r] = '\0';
      handle->ptr[r+1] = '\0';

      handle->size = r+2;
      fclose(cfg_file);

      handle->isStatic=0;
      return handle;
   }
   else
   {
     return NULL;
   }
}

os_cfg_file *os_fileOpenURI(const char *uri)
{
   if ( strncmp(uri, fileURI, fileURILen ) == 0 )
   {
      return(fopen(&uri[fileURILen], "r"));
   }
   else
   {
      return(NULL);
   }
}


cf_kind
cf_nodeKind (
    cf_node node)
{
    assert (node);

    return node->kind;
}

c_value
cf_dataValue(
    cf_data data)
{
    assert(data != NULL);

    return data->value;
}

c_string
c_fieldName (
    c_field _this)
{
    return (_this == NULL ? NULL : _this->name);
}


ut_table
ut_tableNew(
    const ut_compareElementsFunc cmpFunc,
    void *arg,
    const ut_freeElementFunc freeKey,
    void *freeKeyArg,
    const ut_freeElementFunc freeValue,
    void *freeValueArg)
{
    ut_table table;

    table = ddsrt_malloc(sizeof(*table));
    ut_collectionInit(
                ut_collection(table),
                UT_TABLE,
                cmpFunc,
                arg,
                freeValue,
                freeValueArg);
#if 1
    ddsrt_avl_ctreedef_init_r (&table->td,
                              offsetof (struct ut_tableNode_s, avlnode),
                              offsetof (struct ut_tableNode_s, key),
                              (int (*) (const void *, const void *, void *)) cmpFunc, arg, 0,
                              DDSRT_AVL_TREEDEF_FLAG_INDKEY);
#else
    ddsrt_avl_ctreedef_init_r (&table->td,
                              offsetof (struct ut_tableNode_s, avlnode),
                              offsetof (struct ut_tableNode_s, key),
                              tableCmp, ut_collection(table), 0,
                              DDSRT_AVL_TREEDEF_FLAG_INDKEY);
#endif
    ddsrt_avl_cinit (&table->td, &table->tree);
    table->freeKey = freeKey;
    table->freeKeyArg = freeKeyArg;
    return table;
}


void
ut_collectionInit(
        ut_collection c,
        ut_collectionType type,
        const ut_compareElementsFunc cmpFunc,
        void *cmpArg,
        const ut_freeElementFunc freeValue,
        void *freeValueArg)
{
    c->type = type;
    c->cmpFunc = (ut_compareElementsFunc)cmpFunc;
    c->cmpArg = cmpArg;
    c->freeValue = (ut_freeElementFunc) freeValue;
    c->freeValueArg = freeValueArg;
}


void
u__serviceInitialise(void)
{
    ////os_mutexInit(&atExitMutex, NULL);
    return;
}

static c_metaObject
c__metaDeclareCommon(
    c_metaObject scope,
    const c_char *name,
    c_metaKind kind,
    c_bool check)
{
    c_metaObject o,found = NULL;

    assert(scope != NULL);
    assert(name != NULL);
    o = c_metaObject(c_metaFindByName (scope, name, CQ_METAOBJECTS | CQ_FIXEDSCOPE));
    if (o == NULL) {
        switch(kind) {
        case M_CLASS:
        case M_COLLECTION:
        case M_CONSTANT:
        case M_ATTRIBUTE:
        case M_ENUMERATION:
        case M_EXCEPTION:
        case M_INTERFACE:
        case M_MODULE:
        case M_OPERATION:
        case M_PARAMETER:
        case M_PRIMITIVE:
        case M_RELATION:
        case M_STRUCTURE:
        case M_TYPEDEF:
        case M_UNION:
        case M_ANNOTATION:
            if (check) {
                o = c_metaDefine_s(scope,kind);
                if (o) {
                    found = c_metaBind_s(scope,name,o);
                    c_free(o);
                }
            } else {
                o = c_metaDefine(scope,kind);
                assert(o);
                found = c_metaBind(scope,name,o);
                c_free(o);
            }
            o = found;
            break;
        default:
            // OS_REPORT(OS_WARNING,"c_metaDeclare failed",0,
            //             "illegal meta kind (%d) specified",kind);
            assert(FALSE);
            o = NULL;
            break;
        }
    } else if (c_baseObjectKind(o) != kind) {
        c_free(o);
        o = NULL;
    }

    return o;
}


c_metaObject
c_metaDeclare(
    c_metaObject scope,
    const c_char *name,
    c_metaKind kind)
{
    return c__metaDeclareCommon(scope, name, kind, FALSE);
}


c_baseObject
c_metaFindByName (
    c_metaObject scope,
    const char *name,
    c_ulong metaFilter)
{
    return c_metaFindByComp (scope, name, metaFilter);
}


c_bool
c_isOneOf (
    c_char c,
    const c_char *symbolList)
{
    const c_char *symbol;

    symbol = symbolList;
    while (symbol != NULL && *symbol != '\0') {
        if (c == *symbol) return TRUE;
        symbol++;
    }
    return FALSE;
}

c_char *
c_skipSpaces (
    const c_char *str)
{
    if (str == NULL) return NULL;
    while (c_isOneOf(*str, " \t")) str++;
    return (c_char *)str;
}


static c_token
c_getToken(
    const c_char **str)
{
    c_bool stop,result;
    if ((str == NULL) || (*str == NULL)) return TK_ERROR;
    *str = c_skipSpaces(*str);
    if (**str == '.') {
        (*str)++;
        return TK_DOT;
    }
    if (**str == '/') {
        (*str)++;
        return TK_SLASH;
    }
    if (**str == ',') {
        (*str)++;
        return TK_COMMA;
    }
    if (strncmp(*str,"->",2) == 0) {
        (*str) += 2;
        return TK_REF;
    }
    if (strncmp(*str,"::",2) == 0)  {
        (*str) += 2;
        return TK_SCOPE;
    }
    if (c_isLetter(**str) || (**str == '_')) {
        (*str)++;
        stop = FALSE;
        while (!stop) {
            if (c_isLetter(**str) || c_isDigit(**str) || (**str == '_') || (**str == ',')) {
                (*str)++;
            } else if (**str == '<') {
                result = c_getInstantiationType(str);
                if (result == FALSE) {
                    return TK_ERROR;
                }
            } else {
                stop = TRUE;
            }
        }
        return TK_IDENT;
    }
    if (**str == 0) {
        return TK_END;
    }
    return TK_ERROR;
}


c_baseObject
c_metaFindByComp (
    c_metaObject scope,
    const char *name,
    c_ulong metaFilter)
{
    c_baseObject o = NULL;
    const c_char *head,*tail;
    c_char *str;
    c_size length;
    c_state state;
    c_token tok;

    str = NULL;
    tail = name;
    state = ST_START;

    while ((state != ST_END) && (state != ST_ERROR)) {
        ////c_skipSpaces
        head = c_skipSpaces(tail);
        tail = head;
        tok = c_getToken(&tail);
        
        switch (tok) {
        case TK_DOT:
        case TK_REF:
            switch (state) {
            case ST_IDENT:
            case ST_REFID:
                switch (o->kind) {
                case M_STRUCTURE:
                case M_EXCEPTION:
                case M_INTERFACE:
                case M_CLASS:
                case M_ANNOTATION:
                    scope = c_metaObject(o);
                    state = ST_REF;
                break;
                case M_ATTRIBUTE:
                case M_RELATION:
                    scope = c_metaObject(c_property(o)->type);
                    state = ST_REF;
                break;
                case M_MEMBER:
                    scope = c_metaObject(c_specifier(o)->type);
                    state = ST_REF;
                break;
                default:
                    state = ST_ERROR;
                }
                c_free(o);
            break;
            default:
                state = ST_ERROR;
            break;
            }
        break;
        case TK_SCOPE:
        case TK_SLASH:
            switch (state) {
            case ST_START:
                scope = c_metaObject(c__getBase(scope));
                state = ST_SCOPE;
            break;
            case ST_IDENT:
                if (o->kind == M_MODULE ||
                    o->kind == M_STRUCTURE ||
                    o->kind == M_UNION) {
                    scope = c_metaObject(o);
                    state = ST_SCOPE;
                } else {
                    state = ST_ERROR;
                }
            break;
            default:
                state = ST_ERROR;
            break;
            }
        break;
        case TK_IDENT:
            length = (c_size) (tail - head + 1);
            str = (char *)ddsrt_malloc(length);
            memcpy(str,head,length-1);
            str[length-1]=0;

            switch (state) {
            case ST_START:
                o = NULL;
                while ((scope != NULL) && (o == NULL)) {
                    if ((metaFilter & CQ_FIXEDSCOPE) == 0) {
                        o = metaResolveName(scope,str,metaFilter);
                        scope = scope->definedIn;
                    } else {
                        o = metaResolveName(scope,str,metaFilter);
                        scope = NULL;
                    }
                }
                if (o == NULL) {
                    state = ST_ERROR;
                } else {
                    state = ST_IDENT;
                }
            break;
            case ST_SCOPE:
                o = metaResolveName(scope,str,metaFilter);
                if (o == NULL) {
                    state = ST_ERROR;
                } else {
                    state = ST_IDENT;
                }
            break;
            case ST_REF:
                o = metaResolveName(scope,str,metaFilter);
                if (o == NULL) {
                    state = ST_ERROR;
                } else {
                    state = ST_REFID;
                }
            break;
            default:
                state = ST_ERROR;
            break;
            }
            ddsrt_free(str);
        break;
        case TK_END:
            switch (state) {
            case ST_IDENT:
            case ST_REFID:
                state = ST_END;
            break;
            default:
                state = ST_ERROR;
            }
        break;
        default:
            state = ST_ERROR;
        break;
        }
    }

    if (state == ST_ERROR) return NULL;

    return o;
}


c_bool
c_isLetter (
    c_char c)
{
    return (((c>='a')&&(c<='z'))||((c>='A')&&(c<='Z')));
}

c_bool
c_isDigit (
    c_char c)
{
    return ((c>='0')&&(c<='9'));
}

c_bool
c_getInstantiationType(
    const c_char **str)
{
    c_bool result;

    if (**str != '<') {
        return TRUE;
    }
    (*str)++;
    while (**str != '>') {
        if (**str == '<') {
            /* Nested brackets, recursively scan */
            result = c_getInstantiationType(str);
            if (result == FALSE) {
                return FALSE;
            }
        } else {
            /* Unexpected end of string */
            if (**str == '\0') {
                return FALSE;
            } else {
                /* No bracket, no terminator, so skip to next */
                (*str)++;
            }
        }
    }
    /* We have reached the closing tag '>' */
    (*str)++;
    return TRUE;
}

void
c_metaTypeInit(
    c_object o,
    size_t size,
    size_t alignment)
{
    c_type(o)->base = c__getBase(o);
    c_type(o)->alignment = alignment;
    c_type(o)->size = size;
}

void
c_scopeInit(
    c_scope s)
{
    assert(s != NULL);

    ddsrt_avl_cinit (&c_scope_bindings_td, &s->bindings);
    s->headInsOrder = NULL;
    s->tailInsOrder = NULL;
}


c_metaObject
c__metaBindCommon(
    c_metaObject scope,
    const c_char *name,
    c_metaObject object,
    c_bool check)
{
    c_metaObject found;
    c_metaEquality equality;
    c_char *scopedName;

    assert(name != NULL);
    assert(object != NULL);

    if (object->name != NULL) {
        // OS_REPORT(OS_ERROR,"c_metaObject::c_metaBind",0,
        //             "Object already bound to \"%s\"",object->name);
        return NULL;
    }
    if (check) {
        object->name = c_stringNew_s(c__getBase(scope),name);
        if (!object->name) {
            goto err_alloc_name;
        }
    } else {
        object->name = c_stringNew(c__getBase(scope),name);
    }

    found = metaScopeInsert(scope,object);

    if (found == object) {
        object->definedIn = scope;
        return object;
    } else {
        equality = c_metaCompare(found,object);
        if (equality != E_EQUAL) {
            scopedName = c_metaScopedName(found);
            // OS_REPORT(OS_ERROR, "c_metaObject::c_metaBind", 0,
            //     "Redeclaration of '%s' doesn't match existing declaration",
            //     scopedName);
            ddsrt_free(scopedName);
            c_free(found);
            return NULL;
        }
        c_free(object->name);
        object->name = NULL;
        return found;
    }

err_alloc_name:
    return NULL;
}

c_metaObject
c_metaBind(
    c_metaObject scope,
    const c_char *name,
    c_metaObject object)
{
    return c__metaBindCommon(scope, name, object, FALSE);
}


c_metaObject
metaScopeInsert(
    c_metaObject scope,
    c_metaObject object)
{
    c_scope s;
    c_metaObject o;

    if (c_baseObjectKind(object) == M_ENUMERATION) {
        /* Inserting a type in a scope only succeeds if the name of the type
         * does not already exist in the scope.
         * For Enumerations this also applies to the names of each enum value.
         * The following operation verifies if none of the enumeration names
         * already exists and if not it will insert all names to the scope.
         */
        o = metaScopeInsertEnum(scope,object);
        if (o == NULL) {
            c_char *name = c_metaName(object);
            o = c_metaResolveType(scope,name);
            c_free(name);
        }
    } else {
        s = metaClaim(scope);
        o = c_scopeInsert(s,object);
        metaRelease(scope);
    }
    return o;
}

c_metaObject
metaScopeInsertEnum(
    c_metaObject scope,
    c_metaObject object)
{
    c_metaObject o = NULL;
    c_char *name = NULL;
    c_ulong i, length;
    c_bool redeclared = FALSE;
    c_array elements;
    c_scope s;

    s = metaClaim(scope);
    elements = c_enumeration(object)->elements;
    length = c_arraySize(elements);
    for (i=0; i<length; i++) {
        assert(elements[i] != NULL);
        name = c_metaName(elements[i]);
        if (c_scopeExists(s, name)) {
            redeclared = TRUE;
        }
        c_free(name);
    }
    if (!redeclared) {
        o = c_scopeInsert(s,object);
        if (o == object) {
            c_metaObject element;
            for (i=0; i<length; i++ ) {
                element = c_scopeInsert(s, elements[i]);
                assert(element == elements[i]);
                element->definedIn = scope;
            }
        }
    }
    metaRelease(scope);
    return o;
}

c_scope
metaClaim(
    c_metaObject scope)
{
    if (scope == NULL) {
        return NULL;
    }
    switch(c_baseObjectKind(scope)) {
    case M_MODULE:
    ////c_mutexLock
        ////c_mutexLock(&c_module(scope)->mtx);
        return c_module(scope)->scope;
    case M_INTERFACE:
    case M_CLASS:
    case M_ANNOTATION:
        return c_interface(scope)->scope;
    case M_STRUCTURE:
    case M_EXCEPTION:
        return c_structure(scope)->scope;
    case M_ENUMERATION:
        return metaClaim(scope->definedIn);
    case M_UNION:
        return c_union(scope)->scope;
    case M_TYPEDEF:
        return metaClaim(scope->definedIn);
    default:
        return NULL;
    }
}


c_metaObject
c_scopeInsert(
    c_scope scope,
    c_metaObject object)
{
    ddsrt_avl_ipath_t p;
    c_binding binding = NULL;

    if ((binding = ddsrt_avl_clookup_ipath (&c_scope_bindings_td, &scope->bindings, object, &p)) == NULL) {
        binding = c_bindingNew(scope, object);
        ddsrt_avl_cinsert_ipath (&c_scope_bindings_td, &scope->bindings, binding, &p);
        if (!scope->headInsOrder) {
            scope->headInsOrder = binding;
        }
        if (scope->tailInsOrder) {
            scope->tailInsOrder->nextInsOrder = binding;
        }
        scope->tailInsOrder = binding;
    } else {
        if (c_isFinal(binding->object) == FALSE) {
            c_metaCopy(object,binding->object);
        }
    }
    c_keep(binding->object);

    /** Note that if inserted the object reference count is increased
        by 2.  one for being inserted and one for being returned.
     */
    return binding->object;
}

c_binding
c_bindingNew(
    c_scope scope,
    c_metaObject object)
{
    c_binding result;

    result = c_binding(c_mmMalloc(MM(scope),C_SIZEOF(c_binding)));
    if (result) {
        result->object = c_keep(object);
        result->nextInsOrder = NULL;
    }

    return result;
}

c_bool
c_isFinal(
    c_metaObject o)
{
    switch(c_baseObjectKind(o)) {
    case M_CLASS:
    case M_COLLECTION:
    case M_ENUMERATION:
    case M_EXCEPTION:
    case M_INTERFACE:
    case M_ANNOTATION:
    case M_PRIMITIVE:
    case M_STRUCTURE:
    case M_TYPEDEF:
    case M_UNION:
        if (c_type(o)->alignment != 0) return TRUE; /* what about c_object ??? */
    break;
    default:
    break;
    }
    return FALSE;
}


void
c_metaCopy(
    c_metaObject s,
    c_metaObject d)
{
    // if (c_baseObjectKind(s) != c_baseObjectKind(d)) {
    //     return;
    // }
    // switch(c_baseObjectKind(s)) {
    // case M_CLASS:
    //     c_class(d)->extends = c_keep(c_class(s)->extends);
    //     c_class(d)->keys = c_keep(c_class(s)->keys);
    // case M_ANNOTATION:
    // case M_INTERFACE:
    //     c_interface(d)->abstract = c_interface(s)->abstract;
    //     c_interface(d)->inherits = c_keep(c_interface(s)->inherits);
    //     c_interface(d)->references = c_keep(c_interface(s)->references);
    //     metaScopeWalk(s, copyScopeObjectScopeWalkAction, d);
    //     c_typeCopy(c_type(s),c_type(d));
    // break;
    // case M_COLLECTION:
    //     c_collectionType(d)->kind = c_collectionType(s)->kind;
    //     c_collectionType(d)->maxSize = c_collectionType(s)->maxSize;
    //     c_collectionType(d)->subType = c_keep(c_collectionType(s)->subType);
    //     c_typeCopy(c_type(s),c_type(d));
    // break;
    // case M_ATTRIBUTE:
    //     c_attribute(d)->isReadOnly = c_attribute(s)->isReadOnly;
    //     c_specifierCopy(c_specifier(s),c_specifier(d));
    // break;
    // case M_ENUMERATION:
    //     c_enumeration(d)->elements = c_keep(c_enumeration(s)->elements);
    //     c_typeCopy(c_type(s),c_type(d));
    // break;
    // case M_EXCEPTION:
    // case M_STRUCTURE:
    //     c_structure(d)->members = c_keep(c_structure(s)->members);
    //     c_structure(d)->references = c_keep(c_structure(s)->references);
    //     metaScopeWalk(s, copyScopeObjectScopeWalkAction, d);
    // break;
    // case M_MODULE:
    //     metaScopeWalk(s, copyScopeObjectScopeWalkAction, d);
    // break;
    // case M_PRIMITIVE:
    //     c_primitive(d)->kind = c_primitive(s)->kind;
    //     c_typeCopy(c_type(s),c_type(d));
    // break;
    // case M_TYPEDEF:
    //     c_typeDef(d)->alias = c_keep(c_typeDef(s)->alias);
    //     c_typeCopy(c_type(s),c_type(d));
    // break;
    // case M_UNION:
    //     c_union(d)->cases = c_keep(c_union(s)->cases);
    //     c_union(d)->references = c_keep(c_union(s)->references);
    //     c_union(d)->switchType = c_keep(c_union(s)->switchType);
    //     c_typeCopy(c_type(s),c_type(d));
    // break;
    // case M_OPERATION:
    //     c_operation(d)->parameters = c_keep(c_operation(s)->parameters);
    //     c_operation(d)->result = c_keep(c_operation(s)->result);
    // break;
    // case M_PARAMETER:
    //     c_parameter(d)->mode = c_parameter(s)->mode;
    // break;
    // case M_CONSTANT:
    //     c_constant(d)->operand = c_keep(c_constant(s)->operand);
    //     c_constant(d)->type = c_keep(c_constant(s)->type);
    // break;
    // default:
    // break;
    // }
    return;
}

void
metaScopeWalk(
    c_metaObject scope,
    c_scopeWalkAction action,
    c_scopeWalkActionArg actionArg)
{
    c_scope s;

    s = metaClaim(scope);
    if(s != NULL){
        c_scopeWalk(s, action, actionArg);
    }
    metaRelease(scope);
}


void
metaRelease(
    c_metaObject scope)
{
    switch(c_baseObjectKind(scope)) {
    case M_MODULE:
    ////
        ////c_mutexUnlock(&c_module(scope)->mtx);
    break;
    case M_ENUMERATION:
        metaRelease(scope->definedIn);
    break;
    case M_TYPEDEF:
        metaRelease(scope->definedIn);
    break;
    default:
        /* nothing to do for others. */
    break;
    }
}

void
copyScopeObjectScopeWalkAction(
    c_metaObject o,
    c_scopeWalkActionArg arg /* c_metaObject */)
{
    copyScopeObject(o, arg);
}


void
copyScopeObject(
    c_metaObject o,
    c_scopeWalkActionArg scope)
{
    c_metaObject found;

    found = metaScopeInsert((c_metaObject)scope,o);
    assert(found == o);
    c_free(found);
}

void
c_typeCopy(
    c_type s,
    c_type d)
{
    d->alignment = s->alignment;
    d->base = c_keep(s->base);
    d->size = s->size;
}

c_object
c_metaDefine(
    c_metaObject scope,
    c_metaKind kind)
{
    return c__metaDefineCommon(scope, kind, FALSE);
}


c_object
c__metaDefineCommon(
    c_metaObject scope,
    c_metaKind kind,
    c_bool check)
{
    c_baseObject o;
    c_base base;

    assert(scope != NULL);

    base = c__getBase(scope);

    switch (kind) {
    case M_ATTRIBUTE:
    case M_CONSTANT:
    case M_CONSTOPERAND:
    case M_EXPRESSION:
    case M_LITERAL:
    case M_MEMBER:
    case M_OPERATION:
    case M_PARAMETER:
    case M_RELATION:
    case M_UNIONCASE:
        if (check) {
            o = (c_baseObject)c_new_s(c_getMetaType(base,kind));
            if (!o) {
                goto err_alloc_object;
            }
        } else {
            o = (c_baseObject)c_new(c_getMetaType(base,kind));
        }
        assert(o);
        o->kind = kind;
    break;
    case M_COLLECTION:
    case M_ENUMERATION:
    case M_PRIMITIVE:
    case M_TYPEDEF:
    case M_BASE:
        if (check) {
            o = (c_baseObject)c_new_s(c_getMetaType(base,kind));
            if (!o) {
                goto err_alloc_object;
            }
        } else {
            o = (c_baseObject)c_new(c_getMetaType(base,kind));
        }
        assert(o);
        o->kind = kind;
        c_type(o)->base = base; /* REMARK c_keep(base); */
    break;
    case M_UNION:
        if (check) {
            o = (c_baseObject)c_new_s(c_getMetaType(base,kind));
            if (!o) {
                goto err_alloc_object;
            }
            c_union(o)->scope = c_scopeNew_s(base);
            if (!c_union(o)->scope) {
                goto err_alloc_scope;
            }
        } else {
            o = (c_baseObject)c_new(c_getMetaType(base,kind));
            c_union(o)->scope = c_scopeNew(base);
        }
        assert(o);
        o->kind = kind;
        c_type(o)->base = base; /* REMARK c_keep(base); */
    break;
    case M_STRUCTURE:
    case M_EXCEPTION:
        if (check) {
            o = (c_baseObject)c_new_s(c_getMetaType(base,kind));
            if (!o) {
                goto err_alloc_object;
            }
            c_structure(o)->scope = c_scopeNew_s(base);
            if (!c_structure(o)->scope) {
                goto err_alloc_scope;
            }
        } else {
            o = (c_baseObject)c_new(c_getMetaType(base,kind));
            c_structure(o)->scope = c_scopeNew(base);
        }
        assert(o);
        o->kind = kind;
        c_type(o)->base = base; /* REMARK c_keep(base); */
    break;
    case M_MODULE:
        if (check) {
            o = (c_baseObject)c_new_s(c_getMetaType(base,kind));
            if (!o) {
                goto err_alloc_object;
            }
            c_module(o)->scope = c_scopeNew_s(base);
            if (!c_module(o)->scope) {
                goto err_alloc_scope;
            }
        } else {
            o = (c_baseObject)c_new(c_getMetaType(base,kind));
            c_module(o)->scope = c_scopeNew(base);
        }
        assert(o);
        o->kind = kind;
        ////(void)c_mutexInit(base, &c_module(o)->mtx);
    break;
    case M_ANNOTATION:
    case M_CLASS:
    case M_INTERFACE:
        if (check) {
            o = (c_baseObject)c_new_s(c_getMetaType(base,kind));
            if (!o) {
                goto err_alloc_object;
            }
            c_interface(o)->scope = c_scopeNew_s(base);
        } else {
            o = (c_baseObject)c_new(c_getMetaType(base,kind));
            if (!o) {
                goto err_alloc_object;
            }
            c_interface(o)->scope = c_scopeNew(base);
            if (!c_interface(o)->scope) {
                goto err_alloc_scope;
            }
        }
        o->kind = kind;
        if (kind == M_CLASS) {
            c_class(o)->extends = NULL;
        }else if (kind == M_ANNOTATION) {
            c_annotation(o)->extends = NULL;
        }
        c_type(o)->base = base; /* REMARK c_keep(base); */
    break;
    default:
        o = NULL;
    }
    return c_object(o);

err_alloc_scope:
    c_free(o);
err_alloc_object:
    return NULL;
}


c_scope
c_scopeNew_s(
    c_base base)
{
    return c__scopeNewCommon(base, TRUE);
}

c_scope
c__scopeNewCommon(
    c_base base,
    c_bool check)
{
    c_scope o;
    c_type scopeType;

    scopeType = c_resolve(base,"c_scope");
    if (check) {
        o = c_scope(c_new_s(scopeType));
        if (!o) {
            goto err_alloc_scope;
        }
    } else {
        o = c_scope(c_new(scopeType));
    }
    assert(o);
    c_scopeInit(o);

err_alloc_scope:
    c_free(scopeType);
    return o;
}


c_metaObject
c_metaResolve (
    c_metaObject scope,
    const char *name)
{
    return c_metaObject(c_metaFindByComp (scope,
                                          name,
                                          CQ_ALL));
}


c_scope
c_scopeNew(
    c_base base)
{
    return c__scopeNewCommon(base, FALSE);
}

c_result
c_metaFinalize(
    c_metaObject o) {
    return c__metaFinalize(o,TRUE);
}






c_result
c__metaFinalize(
    c_metaObject o,
    c_bool normalize)
{
    c_ulong i;
    size_t alignment, size;
    size_t objectAlignment = C_ALIGNMENT(c_object);
    size_t ps;
    size_t pa;

#define _TYPEINIT_(o,t) c_typeInit(o,C_ALIGNMENT(t),sizeof(t))

    switch(c_baseObjectKind(o)) {
    case M_PRIMITIVE:
        switch(c_primitive(o)->kind) {
        case P_ADDRESS:   _TYPEINIT_(o,c_address);   break;
        case P_BOOLEAN:   _TYPEINIT_(o,c_bool);      break;
        case P_CHAR:      _TYPEINIT_(o,c_char);      break;
        case P_WCHAR:     _TYPEINIT_(o,c_wchar);     break;
        case P_OCTET:     _TYPEINIT_(o,c_octet);     break;
        case P_SHORT:     _TYPEINIT_(o,c_short);     break;
        case P_USHORT:    _TYPEINIT_(o,c_ushort);    break;
        case P_LONG:      _TYPEINIT_(o,c_long);      break;
        case P_ULONG:     _TYPEINIT_(o,c_ulong);     break;
        case P_LONGLONG:  _TYPEINIT_(o,c_longlong);  break;
        case P_ULONGLONG: _TYPEINIT_(o,c_ulonglong); break;
        case P_DOUBLE:    _TYPEINIT_(o,c_double);    break;
        case P_FLOAT:     _TYPEINIT_(o,c_float);     break;
        case P_VOIDP:     _TYPEINIT_(o,c_voidp);     break;
        case P_MUTEX:     _TYPEINIT_(o,c_mutex);     break;
        case P_LOCK:      _TYPEINIT_(o,c_lock);      break;
        case P_COND:      _TYPEINIT_(o,c_cond);      break;
        case P_PA_UINT32: _TYPEINIT_(o,ddsrt_atomic_uint32_t); break;
        case P_PA_UINTPTR:_TYPEINIT_(o,ddsrt_atomic_uintptr_t); break;
        case P_PA_VOIDP:  _TYPEINIT_(o,ddsrt_atomic_voidp_t); break;
        default:          c_typeInit(o,0,0);         break;
        }
      
    break;
    case M_EXCEPTION:
    case M_STRUCTURE:
    {
        c_member member;
        c_type type;
        c_iter refList;
        c_ulong length;

        alignment = 0; size = 0; refList = NULL;
        if (c_structure(o)->members != NULL) {
            length = c_arraySize(c_structure(o)->members);
            for (i=0; i<length; i++) {
                member = c_member(c_structure(o)->members[i]);
                type = c_specifier(member)->type;
                if (c_typeHasRef(type)) {
                    refList = c_iterInsert(refList,member);
                }
                if (!c_isFinal(o)) {
                    switch(c_baseObjectKind(type)) {
                    case M_INTERFACE:
                    case M_ANNOTATION:
                    case M_CLASS:
                    case M_BASE:
                        if (alignment < objectAlignment) {
                            alignment = objectAlignment;
                        }
                        member->offset = alignSize(size,objectAlignment);
                        size = member->offset + sizeof(void *);
                    break;
                    default:
                        pa = propertyAlignment(type);
                        if (pa > alignment) {
                            alignment = pa;
                        }
                        member->offset = alignSize(size,pa);
                        ps = propertySize(type);
                        if (ps == 0) {
                            c_iterFree(refList);
                            return S_ILLEGALRECURSION;
                        }
                        size = member->offset + ps;
                    break;
                    }
                }
            }
            if (refList != NULL) {
                i=0;
                length = c_iterLength(refList);
                c_structure(o)->references = c_arrayNew(c_member_t(c__getBase(o)), length);
                while ((member = c_iterTakeFirst(refList)) != NULL) {
                    c_structure(o)->references[i++] = c_keep(member);
                }
                c_iterFree(refList);
            }
        }
        if (!c_isFinal(o)) {
            size = alignSize(size,alignment);
            c_typeInit(o,alignment,size);
        }
    }
    break;
    case M_UNION:
    {
        c_unionCase unCase;
        c_type type;
        c_iter refList;
        c_ulong length;

        /* A union has the following C syntax:
         * struct union {
         *   <discriminant> _d;
         *   <the union>    _u;
         * };
         *
         * So a union has a discriminant '_d' and the c-union '_u'.
         * The size of this union is determined by the maximum size of
         * the members of '_u' + the size of the descriminant '_d'. We
         * also need to take into account the alignment of '_u', since
         * we might need to add padding bytes between '_d' and '_u'. Finally
         * we need to align the entire struct.
         *
         * This results in the following algorithm:
         * 1. determine size of '_u', which is equal to the largest member
         * 2. determine alignment which is equal to the maximum of the
         *    alignment of '_d' and '_u'. The alignment of '_u' is equal
         *    to the alignment of the member with the largest alignment
         * 3. The total size of the union is equal to:
         *    sizeof('_d') + sizeof ('_d') + alignSize('_u') + alignSize('struct union')
         *    where the alignSize-function calculates the amount of padding
         *    for the identified member/type.
         */
        alignment = c_union(o)->switchType->alignment;
        size = 0;
        refList = NULL;
        if (c_union(o)->cases != NULL) {
            length = c_arraySize(c_union(o)->cases);
            for (i=0; i<length; i++) {
                unCase = c_unionCase(c_union(o)->cases[i]);
                type = c_specifier(unCase)->type;
                if (c_typeHasRef(type)) {
                    refList = c_iterInsert(refList,unCase);
                }
                if (!c_isFinal(o)) {
                    switch(c_baseObjectKind(type)) {
                    case M_INTERFACE:
                    case M_CLASS:
                    case M_ANNOTATION:
                    case M_BASE:
                        if (type->alignment > alignment) {
                            alignment = type->alignment;
                        }
                        if (objectAlignment > size) {
                            size = objectAlignment;
                        }
                    break;
                    default:
                        pa = propertyAlignment(type);
                        if (pa > alignment) {
                            alignment = pa;
                        }
                        ps = propertySize(type);
                        if (ps == 0) {
                            c_iterFree(refList);
                            return S_ILLEGALRECURSION;
                        }
                        if (ps > size) {
                            size = ps;
                        }
                    break;
                    }
                }
            }
            size += alignSize(c_union(o)->switchType->size, alignment);
            if (refList != NULL) {
                i=0;
                c_union(o)->references = c_arrayNew(c_unionCase_t(c__getBase(o)),c_iterLength(refList));
                while ((unCase = c_iterTakeFirst(refList)) != NULL) {
                    c_union(o)->references[i++] = c_keep(unCase);
                }
                c_iterFree(refList);
            }
        }
        if (!c_isFinal(o)) {
            size = alignSize(size,alignment);
            c_typeInit(o,alignment,size);
        }
    }
    break;
    case M_COLLECTION:
        switch (c_collectionType(o)->kind) {
        case OSPL_C_ARRAY:
            if (c_collectionType(o)->maxSize != 0) {
                ps = propertySize(c_collectionType(o)->subType);
                if (ps == 0) {
                    return S_ILLEGALRECURSION;
                }
                size = c_collectionType(o)->maxSize * ps;
                alignment = propertyAlignment(c_collectionType(o)->subType);
                c_typeInit(o,alignment,size);
            } else {
                _TYPEINIT_(o,c_voidp);
            }
        break;
        default:
            _TYPEINIT_(o,c_voidp);
        break;
        }
    break;
    case M_ENUMERATION:
    {
        c_constant c;
        c_ulong length;
        c_long value;

        length = c_arraySize(c_enumeration(o)->elements);

        for (i=0, value=(c_long)i; i<length; i++, value++) {
            c = c_constant(c_enumeration(o)->elements[i]);
            c->type = c_type(o); /* The constant has an un-managed ref. to the enum type, _c_freeReferences will deal with this */
            if (c->operand == NULL) { /* typical in the odlpp */
                c->operand = (c_operand)c_metaDefine(c_metaObject(o), M_LITERAL);
                c_literal(c->operand)->value.kind = V_LONG;
                c_literal(c->operand)->value.is.Long = (c_long)value;
            } else {
                c_literal literal = NULL;

                switch (c_baseObjectKind (c->operand)) {
                case M_EXPRESSION:
                    literal = c_expressionValue(c_expression(c->operand));
                    c_free(c->operand);
                    c->operand = c_operand(literal);
                break;
                case M_LITERAL:
                    literal = c_literal(c->operand);
                break;
                default:
                break;
                }

                if (literal == NULL) {
                    return S_ILLEGALRECURSION;
                }

                switch (literal->value.kind) {
                case V_LONGLONG:
                    value = (c_long)literal->value.is.LongLong;
                    literal->value.kind = V_LONG;
                    literal->value.is.Long = value;
                break;
                case V_LONG:
                    value = (c_long)literal->value.is.Long;
                break;
                default:
                    return S_ILLEGALRECURSION;
                }
            }
        }
        _TYPEINIT_(o,c_metaKind);
    }
    break;
    case M_INTERFACE:
    case M_ANNOTATION:
    case M_CLASS:
    {
        c_property property;
        c_iter iter = NULL;
        c_type type;
        c_property *properties;
        size_t offset;
        c_ulong length;
        c_iter refList;

        alignment = 0; size = 0;
        if (c_baseObjectKind(o) == M_CLASS) {
            if (c_class(o)->extends != NULL) {
                assert(c_isFinal(c_metaObject(c_class(o)->extends)));
                size = c_type(c_class(o)->extends)->size;
                assert(size > 0);
                alignment = c_type(c_class(o)->extends)->alignment;
                assert(alignment > 0);
            }
        }
        metaScopeWalk(c_metaObject(o), getPropertiesScopeWalkAction, &iter);
        if (c_interface(o)->inherits != NULL) {
            length = c_arraySize(c_interface(o)->inherits);
            for (i=0; i<length; i++) {
                iter = c_iterInsert(iter,c_interface(o)->inherits[i]);
            }
        }

        refList = NULL;
        properties = NULL;
        length = c_iterLength(iter);
        if (length > 0) {
            properties = (c_property *)ddsrt_malloc(length*sizeof(c_property));
            if (normalize) {
                for (i=0;i<length;i++) {
                    properties[i] = c_iterTakeFirst(iter);
                }
            } else {
                for (i=length;i>0;i--) {
                    properties[i-1] = c_iterTakeFirst(iter);
                }
            }
        }
        c_iterFree(iter);

        if (properties != NULL) {
            if (!c_isFinal(o)) {
                if (normalize) {
                    qsort(properties,length,sizeof(void *),c_compareProperty);
                }
                for (i=0; i<length; i++) {
                    property = c_property(properties[i]);
                    offset = property->offset;
                    type = property->type;
                    if (c_typeHasRef(type)) {
                        refList = c_iterInsert(refList,property);
                    }
                    while(c_baseObjectKind(type) == M_TYPEDEF) {
                        type = c_typeDef(type)->alias;
                    }
                    switch(c_baseObjectKind(type)) {
                    case M_INTERFACE:
                    case M_ANNOTATION:
                    case M_CLASS:
                    case M_BASE:
                        if (alignment < objectAlignment) {
                            alignment = objectAlignment;
                        }
                        if (normalize) {
                            property->offset = alignSize(size,objectAlignment);
                        }
                        size = property->offset + objectAlignment;
                    break;
                    default:
                        if (property->type->alignment > alignment) {
                            alignment = property->type->alignment;
                        }
                        if (normalize) {
                            property->offset = alignSize(size,propertyAlignment(property->type));
                        }
                        ps = propertySize(property->type);
                        if (ps == 0) {
                            ddsrt_free(properties);
                            if (refList != NULL) {
                                c_iterFree(refList);
                            }
                            return S_ILLEGALRECURSION;
                        }
                        size = property->offset + ps;
                    break;
                    }
#ifndef NDEBUG
if ((offset != 0) && (offset != property->offset)) {
    c_string propertyMetaName = c_metaName(c_metaObject(property));
    c_string objectMetaName = c_metaName(o);
    /* apperantly the new order differs from the existing order */
    printf("property %s of type %s meta offset = "PA_ADDRFMT" but was "PA_ADDRFMT" \n",
            propertyMetaName,objectMetaName,(PA_ADDRCAST)property->offset,(PA_ADDRCAST)offset);
    c_free(propertyMetaName);
    c_free(objectMetaName);
}
#else
    OS_UNUSED_ARG(offset);
#endif

                }
            }
            ddsrt_free(properties);
        }

        /* c_any and c_object have no properties and therefore alignment has
         * not been calculated.
         */
        if (alignment == 0) {
            alignment = objectAlignment;
        }

        if (!c_isFinal(o)) {
            size = alignSize(size,alignment);
            c_typeInit(o,alignment,size);
        }

        if (refList != NULL) {
            i=0;
            c_interface(o)->references = c_arrayNew(c_property_t(c__getBase(o)),c_iterLength(refList));
            while ((property = c_iterTakeFirst(refList)) != NULL) {
                c_interface(o)->references[i++] = c_keep(property);
            }
            c_iterFree(refList);
        }
    }
    break;
    case M_TYPEDEF:
        c_typeInit(o,c_type(c_typeDef(o)->alias)->alignment,
                     c_type(c_typeDef(o)->alias)->size);
    break;
    default:
    break;
    }
    return S_ACCEPTED;

#undef _TYPEINIT_
}

void
c_typeInit(
    c_metaObject o,
    size_t alignment,
    size_t size)
{
    c_type(o)->base = c__getBase(o);
    c_type(o)->alignment = alignment;
    c_type(o)->size = size;
}

size_t
alignSize(
    size_t size,
    size_t alignment)
{
    size_t gap;

    if (size == 0) return 0;
    assert(alignment > 0);

    gap = size % alignment;
    if (gap != 0) gap = alignment - gap;
    return (size + gap);
}


size_t
propertyAlignment(
    c_type type)
{
    switch(c_baseObjectKind(type)) {
    case M_COLLECTION:
        if (c_collectionType(type)->kind == OSPL_C_ARRAY) {
            return type->alignment;
        } else {
            return C_ALIGNMENT(c_voidp);
        }
    case M_CLASS:
        return C_ALIGNMENT(c_voidp);
    case M_TYPEDEF:
        return propertyAlignment(c_typeDef(type)->alias);
    default:
        return type->alignment;
    }
}

c_object
c_metaDefine_s(
    c_metaObject scope,
    c_metaKind kind)
{
    return c__metaDefineCommon(scope, kind, TRUE);
}


c_metaObject
c_metaBind_s(
    c_metaObject scope,
    const c_char *name,
    c_metaObject object)
{
    return c__metaBindCommon(scope, name, object, TRUE);
}

void
c_metaAttributeNew(
    c_metaObject scope,
    const c_char *name,
    c_type type,
    size_t offset)
{
    c_metaObject o,found;

    o = c_metaObject(c_metaDefine(scope,M_ATTRIBUTE));
    c_property(o)->type = c_type(c_keep(type));
    c_property(o)->offset = offset;
    c_attribute(o)->isReadOnly = TRUE;
    found = c_metaBind(scope,name,o);
    assert(found == o);
    c_free(found);
    c_free(o);
}


c_char *
c_metaScopedName(
    c_metaObject o)
{
    c_char *scopedName, *ptr;
    size_t length;
    c_iter path = NULL;
    c_metaObject scope,previous;
    c_char *name;

    if (o == NULL) {
        return NULL;
    }

    length = 1; /* \0 */
    scope = o;
    while (scope != NULL) {
        path = c_iterInsert(path,scope);
        length += c_metaNameLength(scope);
        previous = scope->definedIn;
        if (previous != NULL) {
            switch (c_baseObjectKind(scope)) {
            case M_ATTRIBUTE:
            case M_MEMBER:
            case M_RELATION:
            case M_UNIONCASE:
                length += 1; /* .  */
            break;
            default:
                length += 2; /* :: */
            break;
            }
        }
        scope = scope->definedIn;
    }
    scopedName = ddsrt_malloc(length);
    ptr = scopedName;
    previous = NULL;
    while ((scope = c_iterTakeFirst(path)) != NULL) {
        length = c_metaNameLength(scope);
        if (length != 0) {
            if (previous != NULL) {
                switch (c_baseObjectKind(scope)) {
                case M_ATTRIBUTE:
                case M_MEMBER:
                case M_RELATION:
                case M_UNIONCASE:
                ////
                    ////os_sprintf(ptr,".");
                    os_sprintf(ptr,".");
                    ptr = C_DISPLACE(ptr,1);
                break;
                default:
                    ////os_sprintf(ptr,"::");
                    os_sprintf(ptr,"::");
                    ptr = C_DISPLACE(ptr,2);
                break;
                }
            }
            name = c_metaName(scope);
            ////
            os_strncpy(ptr,name,length);
            c_free(name);
            ptr = C_DISPLACE(ptr,length);
            previous = scope;
        }
    }
    c_iterFree(path);
    *ptr = 0;
    return scopedName;
}


size_t
c_metaNameLength(
    c_metaObject o)
{
    c_char *name;
    size_t length;

    name = c_metaName(o);
    if (name == NULL) {
        length = 0;
    } else {
        length = strlen(name);
        c_free(name);
    }
    return length;
}

c_string
c_metaName(
    c_metaObject o)
{
    if (o == NULL) {
        return NULL;
    }
    switch(c_baseObjectKind(o)) {
    case M_PARAMETER:
    case M_MEMBER:
    case M_UNIONCASE:
        return c_keep(c_specifier(o)->name);
    case M_LITERAL:
    case M_CONSTOPERAND:
    case M_EXPRESSION:
        return NULL;
    default:
        return c_keep(o->name);
    }
}


size_t
propertySize(
    c_type type)
{
    switch(c_baseObjectKind(type)) {
    case M_COLLECTION:
        if ((c_collectionType(type)->kind == OSPL_C_ARRAY) &&
            (c_collectionType(type)->maxSize != 0)) {
            assert(type->size > 0);
            return type->size;
        } else {
            return sizeof(void *);
        }
    case M_CLASS:
        return sizeof(void *);
    case M_TYPEDEF:
        return propertySize(c_typeDef(type)->alias);
    default:
        return type->size;
    }
}


void
c_collectionInit (
    c_base base)
{
    c_collectionType o;
    c_metaObject scope = c_metaObject(base);

#define INITCOLL(s,n,k,t) \
    o = (c_collectionType)c_metaDeclare(s,#n,M_COLLECTION); \
    o->kind = k; \
    o->subType = (c_type)c_metaResolve(s,#t); \
    o->maxSize = C_UNLIMITED; \
    c_metaFinalize(c_metaObject(o)); \
    c_free(o)

    INITCOLL(scope,c_string, OSPL_C_STRING,    c_char);
    INITCOLL(scope,c_wstring,OSPL_C_WSTRING,   c_wchar);
    INITCOLL(scope,c_list,   OSPL_C_LIST,      c_object);
    INITCOLL(scope,c_set,    OSPL_C_SET,       c_object);
    INITCOLL(scope,c_bag,    OSPL_C_BAG,       c_object);
    INITCOLL(scope,c_table,  OSPL_C_DICTIONARY,c_object);
    INITCOLL(scope,c_array,  OSPL_C_ARRAY,     c_object);
    INITCOLL(scope,c_query,  OSPL_C_QUERY,     c_object);

#undef INITCOLL
}


c_array
c_arrayNew(
    c_type subType,
    c_ulong length)
{
    return c__arrayNewCommon(subType, length, FALSE);
}

c_array
c__arrayNewCommon(
    c_type subType,
    c_ulong length,
    c_bool check)
{
    c_base base;
    c_type arrayType;
    c_char *name;
    size_t str_size;
    c_array _this = NULL;

    if (length == 0) {
        /* NULL is specified to represent an array of length 0. */
        return NULL;
    } else if (c_metaObject(subType)->name != NULL) {

        base = c__getBase(subType);
        str_size =  strlen(c_metaObject(subType)->name)
                    + 7 /* ARRAY<> */
                    + 1; /* \0 character */
        name = (char *)ddsrt_malloc(str_size);
        ////
        os_sprintf(name,"ARRAY<%s>",c_metaObject(subType)->name);
        if (check) {
            arrayType = c_metaArrayTypeNew_s(c_metaObject(base), name,subType,0);
            ddsrt_free(name);
            if (arrayType) {
                _this = (c_array)c_newArray_s(c_collectionType(arrayType), length);
                c_free(arrayType);
            }
        } else {
            arrayType = c_metaArrayTypeNew(c_metaObject(base), name,subType,0);
            assert(arrayType);
            ddsrt_free(name);
            _this = (c_array)c_newArray(c_collectionType(arrayType), length);
            c_free(arrayType);
        }
    }

    return _this;
}



c_type
c_metaArrayTypeNew_s(
    c_metaObject scope,
    const c_char *name,
    c_type subType,
    c_ulong maxSize)
{
    c_metaObject _this = NULL;
    c_metaObject found;

    if (name) {
        _this = c_metaResolve(scope, name);
    }
    if (_this == NULL) {
        _this = c_metaDefine_s(c_metaObject(c__getBase(scope)),M_COLLECTION);
        c_collectionType(_this)->kind = OSPL_C_ARRAY;
        c_collectionType(_this)->subType = c_keep(subType);
        c_collectionType(_this)->maxSize = maxSize;
        c_metaFinalize(_this);
        if (name) {
            found = c_metaBind_s(scope, name, _this);
            if (found) {
                c_free(_this);
                if (found != _this) {
                    _this = found;
                }
            } else {
                c_free(_this);
                _this = NULL;
            }
        }
    }
    return c_type(_this);
}


c_type
c_metaArrayTypeNew(
    c_metaObject scope,
    const c_char *name,
    c_type subType,
    c_ulong maxSize)
{
    c_metaObject _this = NULL;
    c_metaObject found;

    if (name) {
        _this = c_metaResolve(scope, name);
    }
    if (_this == NULL) {
        _this = c_metaDefine(c_metaObject(c__getBase(scope)),M_COLLECTION);
        c_collectionType(_this)->kind = OSPL_C_ARRAY;
        c_collectionType(_this)->subType = c_keep(subType);
        c_collectionType(_this)->maxSize = maxSize;
        c_metaFinalize(_this);
        if (name) {
            found = c_metaBind(scope, name, _this);
            assert(found != NULL);
            c_free(_this);
            if (found != _this) {
                _this = found;
            }
        }
    }
    return c_type(_this);
}

c_constant
c_metaDeclareEnumElement(
    c_metaObject scope,
    const c_char *name)
{
    c_metaObject object;
    object = c_metaDefine(scope,M_CONSTANT);
    if (object) {
        object->name = c_stringNew(c__getBase(scope),name);
    }
    return (c_constant)object;
}


c_long
c_compareProperty(
    const void *ptr1,
    const void *ptr2)
{
    c_type t1,t2;
    const c_property *p1, *p2;

    p1 = (const c_property *)ptr1;
    p2 = (const c_property *)ptr2;

#define _ALIGN_(p) ((p)->type->alignment == 8 ? 4 : (p)->type->alignment)
#define _KIND_(p)  (c_baseObjectKind((p)->type))
#define _NAME_(p)  (c_metaObject(p)->name)
    if (*p1 == *p2) return 0;
    if (*p1 == NULL) return 1;
    if (*p2 == NULL) return -1;
    t1 = (*p1)->type;
    while (c_baseObjectKind(t1) == M_TYPEDEF) {
        t1 = c_typeDef(t1)->alias;
    }
    t2 = (*p2)->type;
    while (c_baseObjectKind(t2) == M_TYPEDEF) {
        t2 = c_typeDef(t2)->alias;
    }
    if (c_typeIsRef(t1) && !c_typeIsRef(t2)) return 1;
    if (!c_typeIsRef(t1) && c_typeIsRef(t2)) return -1;
    if (_ALIGN_(*p1) > _ALIGN_(*p2)) return -1;
    if (_ALIGN_(*p1) < _ALIGN_(*p2)) return 1;
    return strcmp(_NAME_(*p1),_NAME_(*p2));
#undef _NAME_
#undef _KIND_
#undef _ALIGN_
}





c_baseObject
_metaResolveName(
    c_metaObject scope,
    const char *name,
    c_ulong metaFilter)
{
    c_baseObject o;
    c_specifier sp;
    c_unionCase uc;
    c_ulong i,length;

    if (scope == NULL) {
        return NULL;
    }
    switch (c_baseObjectKind(scope)) {
    case M_EXCEPTION:
    case M_STRUCTURE:
        if (metaFilter & CQ_SPECIFIERS) {
            length = c_arraySize(c_structure(scope)->members);
            for (i=0; i<length; i++) {
                sp = c_specifier(c_structure(scope)->members[i]);
                if (metaNameCompare(sp,name,metaFilter) == E_EQUAL) {
                    return c_keep(c_structure(scope)->members[i]);
                }
            }
        }
        return c_baseObject(metaScopeLookup(scope,name,metaFilter));
    case M_UNION:
        if (metaFilter & CQ_SPECIFIERS) {
            length = c_arraySize(c_union(scope)->cases);
            for (i=0; i<length; i++) {
                uc = c_unionCase(c_union(scope)->cases[i]);
                if (metaNameCompare(uc, name, metaFilter) == E_EQUAL) {
                    return c_keep(c_union(scope)->cases[i]);
                }
            }
        }
        return c_baseObject(metaScopeLookup(scope,name,metaFilter));
    case M_MODULE:
    case M_INTERFACE:
    case M_ANNOTATION:
    case M_CLASS:
        o = c_baseObject(metaScopeLookup(scope,name,metaFilter));
        if (o != NULL) {
            return o;
        } else if (c_baseObjectKind(scope) == M_CLASS) {
            return metaResolveName(c_class(scope)->extends,name,metaFilter);
        }
    break;
    case M_TYPEDEF:
        return metaResolveName(c_typeDef(scope)->alias,name,metaFilter);
    default:
    break;
    }
    return NULL;
}


c_metaEquality
_metaNameCompare (
    c_baseObject baseObject,
    const char *name,
    c_ulong metaFilter)
{
    c_metaEquality equality = E_UNEQUAL;
    char *objName;

    if (baseObject != NULL) {
        if (CQ_KIND_IN_MASK (baseObject, metaFilter)) {
            if (CQ_KIND_IN_MASK (baseObject, CQ_SPECIFIERS)) {
                objName = c_specifier(baseObject)->name;
            } else if (CQ_KIND_IN_MASK (baseObject, CQ_METAOBJECTS)) {
                objName = c_metaObject(baseObject)->name;
            } else {
                return equality;
            }
            if (metaFilter & CQ_CASEINSENSITIVE) {
                ////
                if (os_strcasecmp (objName, name) == 0) {
                    equality = E_EQUAL;
                }
            } else {
                if (strcmp (objName, name) == 0) {
                    equality = E_EQUAL;
                }
            }
        }
    }
    return equality;
}


c_metaObject
metaScopeLookup(
    c_metaObject scope,
    const c_char *name,
    c_ulong metaFilter)
{
    c_scope s;
    c_metaObject o;

    s = metaClaim(scope);
    if (s != NULL) {
        o = c_metaObject(c_scopeResolve(s,name,metaFilter));
    } else {
        o = NULL;
    }
    metaRelease(scope);
    return o;
}


c_baseObject
c_scopeResolve(
    c_scope scope,
    const char *name,
    c_ulong metaFilter)
{
    c_metaObject o = NULL;
    if (!(metaFilter & CQ_CASEINSENSITIVE)) {
        o = c_scopeLookup(scope, name, metaFilter);
    } else {
        ddsrt_avl_citer_t it;
        c_binding b;
        for (b = ddsrt_avl_citer_first (&c_scope_bindings_td, &scope->bindings, &it);
             b != NULL && o == NULL;
             b = ddsrt_avl_citer_next (&it)) {
            if (c_metaNameCompare (c_baseObject(b->object), name, metaFilter) == E_EQUAL) {
                o = c_keep (c_baseObject (b->object));
            }
        }
    }
    return c_baseObject(o);
}


c_metaEquality
c_metaNameCompare (
    c_baseObject baseObject,
    const char *key,
    c_ulong metaFilter)
{
    c_metaEquality equality = E_UNEQUAL;
    char *name;

    if (CQ_KIND_IN_MASK (baseObject, metaFilter)) {
        if (CQ_KIND_IN_MASK (baseObject, CQ_SPECIFIERS)) {
            name = c_specifier(baseObject)->name;
        } else if (CQ_KIND_IN_MASK (baseObject, CQ_METAOBJECTS)) {
            name = c_metaObject(baseObject)->name;
        } else {
            return equality;
        }
        if (metaFilter & CQ_CASEINSENSITIVE) {
            ////
            if (os_strcasecmp (name, key) == 0) {
                equality = E_EQUAL;
            }
        } else {
            if (strcmp (name, key) == 0) {
                equality = E_EQUAL;
            }
        }
    }
    return equality;
}


c_metaEquality
c_metaCompare(
        c_metaObject object,
        c_metaObject obj)
{
    ut_table adm;
    c_metaEquality result;
    adm = ut_tableNew(comparePointers, NULL, NULL, NULL, NULL, NULL);

    result = _c_metaCompare(
            object,
            obj,
            ut_collection(adm));

    ut_tableFree(adm);

    return result;
}

os_equality
comparePointers(
        void *o1,
        void *o2,
        void *args)
{
    os_equality result = OS_EQ;

    OS_UNUSED_ARG(args);

    if(o1 > o2) {
        result = OS_GT;
    }
    else if(o1 < o2)
    {
        result = OS_LT;
    }
    return result;
}


c_metaEquality
_c_metaCompare (
    c_metaObject object,
    c_metaObject obj,
    ut_collection adm)
{
    c_ulong i, length;
    c_metaEquality result;
    c_metaObject o = obj;

    if(object && o && (o == ut_get(adm, object)))
    {
        /* if the object-obj pair is in the adm-tree, then it is already being compared,
         * so ignore here by returning equal.
         */
        return E_EQUAL;
    }
    else if(object && o)
    {
        (void) ut_tableInsert(ut_table(adm), object, o);
    }

    if (object == o) {
        return E_EQUAL;
    }
    if ((object == NULL) || (o == NULL)) {
        return E_UNEQUAL;
    }
    if (c_baseObjectKind(object) != c_baseObjectKind(o)) {
        return E_UNEQUAL;
    }

    switch(c_baseObjectKind(object)) {
    case M_PRIMITIVE:
        if ( c_primitive(object)->kind == c_primitive(o)->kind) {
            return E_EQUAL;
        } else {
            return E_UNEQUAL;
        }
    case M_EXCEPTION:
    case M_STRUCTURE:
    {
        c_member member,m;

        length = c_arraySize(c_structure(object)->members);

        if (length != c_arraySize(c_structure(o)->members)) {
            return E_UNEQUAL;
        }
        for (i=0; i<length; i++) {
            member = c_member(c_structure(object)->members[i]);
            m = c_member(c_structure(o)->members[i]);
            if (member == m) {
                return E_EQUAL;
            }
            result = _c_metaCompare(c_metaObject(c_specifier(member)->type),
                                   c_metaObject(c_specifier(m)->type),
                                   adm);
            if (result != E_EQUAL) {
                return result;
            }
            if (strcmp(c_specifier(member)->name,c_specifier(m)->name) != 0) {
                return E_UNEQUAL;
            }
        }
    }
    break;
    case M_UNION:
    {
        c_unionCase c1,c2;

        result = _c_metaCompare(c_metaObject(c_union(object)->switchType),
                               c_metaObject(c_union(o)->switchType),
                               adm);
        if (result != E_EQUAL) {
            return result;
        }
        if (c_union(object)->cases == c_union(o)->cases) {
            return E_EQUAL; /* used to identify both being NULL */
        }
        length = c_arraySize(c_union(object)->cases);
        if (length != c_arraySize(c_union(o)->cases)) {
            return E_UNEQUAL;
        }
        for (i=0; i<length; i++) {
            c1 = c_unionCase(c_union(object)->cases[i]);
            c2 = c_unionCase(c_union(o)->cases[i]);
            result = _c_metaCompare(c_metaObject(c_specifier(c1)->type),
                                   c_metaObject(c_specifier(c2)->type),
                                   adm);
            if (result != E_EQUAL) {
                return result;
            }
            if (strcmp(c_specifier(c1)->name,c_specifier(c2)->name) != 0) {
                return E_UNEQUAL;
            }
        }
    }
    break;
    case M_COLLECTION:
        if (c_collectionType(object)->kind != c_collectionType(o)->kind) {
            return E_UNEQUAL;
        }
        result = _c_metaCompare(c_metaObject(c_collectionType(object)->subType),
                               c_metaObject(c_collectionType(o)->subType),
                               adm);
        if (result != E_EQUAL) {
            return result;
        }
        if (c_collectionType(object)->maxSize != c_collectionType(o)->maxSize) {
            return E_UNEQUAL;
        }
    break;
    case M_ENUMERATION:
    {
        c_constant c;

        length = c_arraySize(c_enumeration(object)->elements);
        if (length != c_arraySize(c_enumeration(o)->elements)) {
            return E_UNEQUAL;
        }

        for (i=0; i<length; i++) {
            c = c_constant(c_enumeration(object)->elements[i]);
            if (strcmp (c_metaObject(c)->name, c_metaObject(c_enumeration(o)->elements[i])->name) != 0) {
                return E_UNEQUAL;
            }
        }
    }
    break;
    case M_INTERFACE:
    case M_CLASS:
    case M_ANNOTATION:
    {
        c_property property,p;
        c_iter iter = NULL;

        if (c_baseObjectKind(o) == M_CLASS) {
            result = _c_metaCompare(c_metaObject(c_class(object)->extends),
                                   c_metaObject(c_class(o)->extends),
                                   adm);
            if (result != E_EQUAL) {
                return result;
            }
        }

        if (metaScopeCount(c_metaObject(o)) != metaScopeCount(c_metaObject(object))) {
            return E_UNEQUAL;
        }
        metaScopeWalk(c_metaObject(o), getPropertiesScopeWalkAction, &iter);

        length = c_iterLength(iter);
        if (length > 0) {
            for (i=0;i<length;i++) {
                property = c_iterTakeFirst(iter);
                p = c_property(metaScopeLookup(c_metaObject(o),
                         c_metaObject(property)->name,CQ_METAOBJECTS));
                if (p == NULL) {
                    c_free(property); c_free(p);
                    return E_UNEQUAL;
                }
                result = _c_metaCompare(c_metaObject(property->type),
                                       c_metaObject(p->type),
                                       adm);
                c_free(p);
                if (result != E_EQUAL) {
                    return result;
                }
            }
        }
        c_iterFree(iter);
    }
    break;
    case M_TYPEDEF:
    {
        return _c_metaCompare(c_metaObject(c_typeDef(object)->alias),
                             c_metaObject(c_typeDef(o)->alias),
                             adm);
    }
    default:
    break;
    }
    return E_EQUAL;
}


void *
ut_get(
    ut_collection c,
    void *o)
{
    void *result;

    assert(c);
    assert(o);

    result = NULL;

    switch (c->type) {
    case UT_TABLE :
        {
            ut_table table = ut_table(c);
            ut_tableNode foundNode = ddsrt_avl_clookup (&table->td, &table->tree, o);
            if (foundNode != NULL) {
                result = foundNode->value;
            }
        }
        break;
    case UT_SET :
        {
            ut_set set = ut_set(c);
            ut_setNode foundNode = ddsrt_avl_clookup (&set->td, &set->tree, o);
            if (foundNode != NULL) {
                result = foundNode->value;
            }
        }
        break;
    default :
        fprintf(stderr, "ut_get: This collection type is not yet supported\n");
        assert(0);
        break;
    }

    return result;
}


c_ulong
metaScopeCount(
    c_metaObject scope)
{
    c_scope s;
    c_ulong n = 0;

    s = metaClaim(scope);
    if(s != NULL){
        n = c_scopeCount(s);
    }
    metaRelease(scope);
    return n;
}

c_ulong
c_scopeCount(
    c_scope scope)
{
    return (c_ulong) ddsrt_avl_ccount (&scope->bindings);
}


void
getPropertiesScopeWalkAction(
    c_metaObject o,
    c_scopeWalkActionArg arg)
{
    c_iter *iter = (c_iter *)arg;
    switch(c_baseObjectKind(o)) {
    case M_ATTRIBUTE:
    case M_RELATION:
    case M_MEMBER:
    case M_UNIONCASE:
        *iter = c_iterInsert(*iter, o);
    break;
    default:
    break;
    }
}


c_literal
c_enumValue (
    c_enumeration e,
    const c_char *label)
{
    c_ulong i,length;
    c_string metaName;

    if (e == NULL) {
        return NULL;
    }
    if (label == NULL) {
        return NULL;
    }
    length = c_arraySize(e->elements);
    for (i=0;i<length;i++) {
        metaName = c_metaName(c_metaObject(e->elements[i]));
        if (strcmp(label,metaName) == 0) {
            c_free(metaName);
            return c_operandValue(c_operand(e->elements[i]));
        }
        c_free(metaName);
    }
    return NULL;
}

c_literal
c_operandValue(
    c_operand operand)
{
    switch (c_baseObjectKind(operand)) {
    case M_CONSTOPERAND:
        return c_operandValue(c_constOperand(operand)->constant->operand);
    case M_EXPRESSION:
        return c_expressionValue(c_expression(operand));
    case M_LITERAL:
        return c_keep(c_literal(operand));
    case M_CONSTANT:
        return c_operandValue(c_constant(operand)->operand);
    default: assert(FALSE);
    }
    return NULL;
}

c_literal
c_expressionValue(
    c_expression expr)
{
    c_literal left, right, result;
    c_metaObject scope;

    left = NULL; right = NULL;
    if (c_arraySize(expr->operands) > 0) {
        left = c_operandValue(c_operand(expr->operands[0]));
    }
    if (c_arraySize(expr->operands) > 1) {
        right = c_operandValue(c_operand(expr->operands[1]));
    }

    scope = c_metaObject(c__getBase(expr));
    result = (c_literal)c_metaDefine(scope,M_LITERAL);

#define _CASE_(l,o) \
    case l: assert(left && right); result->value = c_valueCalculate(left->value,right->value,o); \
    break

    switch(expr->kind) {
    _CASE_(E_OR,O_LOR);
    _CASE_(E_XOR,O_LXOR);
    _CASE_(E_AND,O_LAND);
    _CASE_(E_SHIFTRIGHT,O_RIGHT);
    _CASE_(E_SHIFTLEFT,O_LEFT);
    _CASE_(E_MUL,O_MUL);
    _CASE_(E_DIV,O_DIV);
    _CASE_(E_MOD,O_MOD);
    case E_PLUS:
        if (c_arraySize(expr->operands) > 1) {
            assert(left && right);
            result->value = c_valueCalculate(left->value,right->value,O_ADD);
        } else {
            assert(left);
            result->value = left->value;
        }
    break;
    case E_MINUS:
        if (c_arraySize(expr->operands) > 1) {
            assert(left && right);
            result->value = c_valueCalculate(left->value,right->value,O_SUB);
        } else {
#define _MINUS_(d,s,t,ct) d.is.t = (ct) -s.is.t
            assert(left);
            result->value.kind = left->value.kind;
            switch (left->value.kind) {
            case V_OCTET: _MINUS_(result->value,left->value,Octet,c_octet); break;
            case V_SHORT: _MINUS_(result->value,left->value,Short,c_short); break;
            case V_LONG: _MINUS_(result->value,left->value,Long,c_long); break;
            case V_LONGLONG: _MINUS_(result->value,left->value,LongLong,c_longlong); break;
            case V_FLOAT: _MINUS_(result->value,left->value,Float,c_float); break;
            case V_DOUBLE: _MINUS_(result->value,left->value,Double,c_double); break;
            default : assert(FALSE);
            }
#undef _MINUS_
        }
    break;
    case E_NOT:
#define _NOT_(d,s,t,ct) d.is.t = (ct) ~s.is.t
        assert(left);
        result->value.kind = left->value.kind;
        switch (left->value.kind) {
        case V_OCTET: _NOT_(result->value,left->value,Octet,c_octet); break;
        case V_SHORT: _NOT_(result->value,left->value,Short,c_short); break;
        case V_USHORT: _NOT_(result->value,left->value,UShort,c_ushort); break;
        case V_LONG: _NOT_(result->value,left->value,Long,c_long); break;
        case V_ULONG: _NOT_(result->value,left->value,ULong,c_ulong); break;
        case V_LONGLONG: _NOT_(result->value,left->value,LongLong,c_longlong); break;
        case V_ULONGLONG: _NOT_(result->value,left->value,ULongLong,c_ulonglong); break;
        default : assert(FALSE);
        }
#undef _NOT_
    break;
    default:
      assert(FALSE);
    }
#undef _CASE_
    c_free(left);
    c_free(right);
    return result;
}


int32_t
ut_tableInsert(
    ut_table t,
    void *key,
    void *value)
{
    int32_t returnValue;
    ut_tableNode node;
    ddsrt_avl_ipath_t p;

    returnValue = 0;

    assert(t);
    assert(ut_collection(t)->type == UT_TABLE);

    if (ddsrt_avl_clookup_ipath (&t->td, &t->tree, key, &p) == NULL) {
        node = ut_newTableNode(key, value);
        ddsrt_avl_cinsert_ipath (&t->td, &t->tree, node, &p);
        returnValue = 1;
    }
    return returnValue;
}

ut_tableNode
ut_newTableNode(
    void *key,
    void *value)
{
    ut_tableNode node;
    node = NULL;

    node = ddsrt_malloc(OS_SIZEOF(ut_tableNode));
    node->key = key;
    node->value = value;

    return node;
}

c_unionCase
c_unionCaseNew (
    c_metaObject scope,
    const c_char *name,
    c_type type,
    c_iter labels)
{
    c_unionCase o;
    c_ulong nrOfLabels;
    c_type subType;

    nrOfLabels = c_iterLength(labels);
    o = c_unionCase(c_metaDefine(scope,M_UNIONCASE));
    subType = c_type(c_metaResolve(scope,"c_literal"));
    o->labels = c_arrayNew(subType,nrOfLabels);
    c_free(subType);
    c_iterArray(labels,o->labels);
    c_specifier(o)->name = c_stringNew(c__getBase(scope),name);
    /* Do not keep type as usage expects transferral of refcount.
     * If changed then odlpp and idlpp must be adapted accordingly.
     */
    c_specifier(o)->type = type;
    return o;
}

c_ulong c_iterLength(c_iter iter)
{
    if (iter == NULL) {
        return 0;
    } else {
        return c__iterImpl_count(&iter->x);
    }
}

void c_iterArray(c_iter iter, void *ar[])
{
    if (iter != NULL) {
        struct c__iterImpl_iter it;
        void *o;
        uint32_t i = 0;
        for (o = c__iterImpl_iter_first(&iter->x, &it); o != NULL; o = c__iterImpl_iter_next(&it)) {
            ar[i++] = o;
        }
    }
}

c_metaObject
c_metaResolveType (
    c_metaObject scope,
    const char *name)
{
    return c_metaObject(c_metaFindByComp (scope,
                                          name,
                                          CQ_METAOBJECTS));
}


void
c_cloneIn (
    c_type type,
    const void *data,
    c_voidp *dest)
{
    c_ulong size;
    size_t subSize;
    c_type t;

    if (data == NULL) {
        *dest = NULL;
        return;
    }

    t = c_typeActualType(type);
    if (c_baseObject(t)->kind == M_COLLECTION) {
        switch(c_collectionType(t)->kind) {
        case OSPL_C_STRING:
            *dest = c_stringNew(c_getBase(t), data);
            break;
        case OSPL_C_LIST:
        case OSPL_C_BAG:
        case OSPL_C_SET:
        case OSPL_C_MAP:
        case OSPL_C_DICTIONARY:
            // OS_REPORT(OS_WARNING,"Database misc",0,
            //           "c_cloneIn: ODL collections unsupported");
        break;
        case OSPL_C_ARRAY:
            subSize = c_collectionType(t)->subType->size;
            size = c_collectionType(t)->maxSize;
            if (size == 0) {
                size = c_arraySize(c_array(data));
                *dest = c_newArray(c_collectionType(t), size);
            }
            if (size > 0) {
                memcpy(*dest, data, size * subSize);
                /* Find indirections */
                c__cloneReferences(t, data, *dest);
            }
            break;
        case OSPL_C_SEQUENCE:
            subSize = c_collectionType(t)->subType->size;
            size = c_sequenceSize(c_sequence(data));
            *dest = c_newSequence(c_collectionType(t), size);
            if (size > 0) {
                memcpy(*dest, data, size * subSize);
                /* Find indirections */
                c__cloneReferences(t, data, *dest);
            }
            break;
        default:
            // OS_REPORT(OS_ERROR,"Database misc",0,
            //             "c_cloneIn: unknown collection kind (%d)",
            //             c_collectionType(t)->kind);
            assert(FALSE);
        break;
        }
    } else if (c_typeIsRef(t)) {
        *dest = c_new(t);
        memcpy(*dest, data, t->size);
        /* Find indirections */
        c__cloneReferences(t, data, *dest);
    } else {
        memcpy(*dest, data, t->size);
        /* Find indirections */
        c__cloneReferences(t, data, *dest);
    }
}


c_bool
c__cloneReferences (
    c_type module,
    const void *data,
    c_voidp dest)
{
    c_type refType;
    c_class cls;
    c_array references, labels, ar, destar;
    c_sequence seq, destseq;
    c_property property;
    c_member member;
    c_ulong i,j,length;
    size_t size;
    c_ulong nrOfRefs,nrOfLabs;
    c_value v;

    switch (c_baseObject(module)->kind) {
    case M_CLASS:
        cls = c_class(module);
        while (cls) {
            length = c_arraySize(c_interface(cls)->references);
            for (i=0;i<length;i++) {
                property = c_property(c_interface(cls)->references[i]);
                refType = property->type;
                _cloneReference(refType,
                        C_DISPLACE(data, property->offset),
                        C_DISPLACE(dest, property->offset));
            }
            cls = cls->extends;
        }
    break;
    case M_ANNOTATION:
    case M_INTERFACE:
        length = c_arraySize(c_interface(module)->references);
        for (i=0;i<length;i++) {
            property = c_property(c_interface(module)->references[i]);
            refType = property->type;
            _cloneReference(refType,
                    C_DISPLACE(data, property->offset),
                    C_DISPLACE(dest, property->offset));
        }
    break;
    case M_EXCEPTION:
    case M_STRUCTURE:
        length = c_arraySize(c_structure(module)->references);
        for (i=0;i<length;i++) {
            member = c_member(c_structure(module)->references[i]);
            refType = c_specifier(member)->type;
            _cloneReference(refType,
                    C_DISPLACE(data, member->offset),
                    C_DISPLACE(dest, member->offset));
        }
    break;
    case M_UNION:
#define _CASE_(k,t) case k: v = t##Value(*((t *)data)); break
        switch (c_metaValueKind(c_metaObject(c_union(module)->switchType))) {
        _CASE_(V_BOOLEAN,   c_bool);
        _CASE_(V_OCTET,     c_octet);
        _CASE_(V_SHORT,     c_short);
        _CASE_(V_LONG,      c_long);
        _CASE_(V_LONGLONG,  c_longlong);
        _CASE_(V_USHORT,    c_ushort);
        _CASE_(V_ULONG,     c_ulong);
        _CASE_(V_ULONGLONG, c_ulonglong);
        _CASE_(V_CHAR,      c_char);
        _CASE_(V_WCHAR,     c_wchar);
        default:
            // OS_REPORT(OS_ERROR,
            //           "c__cloneReferences",0,
            //           "illegal union switch type detected");
            assert(FALSE);
            return FALSE;
        }
#undef _CASE_
        references = c_union(module)->references;
        if (references != NULL) {
            i=0; refType=NULL;
            nrOfRefs = c_arraySize(references);
            while ((i<nrOfRefs) && (refType == NULL)) {
                labels = c_unionCase(references[i])->labels;
                j=0;
                nrOfLabs = c_arraySize(labels);
                while ((j<nrOfLabs) && (refType == NULL)) {
                    if (c_valueCompare(v,c_literal(labels[j])->value) == C_EQ) {
                        c__cloneReferences(c_type(references[i]),
                                           C_DISPLACE(data, c_type(module)->alignment),
                                           C_DISPLACE(dest, c_type(module)->alignment));
                        refType = c_specifier(references[i])->type;
                    }
                    j++;
                }
                i++;
            }
        }
    break;
    case M_COLLECTION:
        refType = c_typeActualType(c_collectionType(module)->subType);
        switch (c_collectionType(module)->kind) {
        case OSPL_C_ARRAY:
            ar = c_array(data);
            destar = c_array(dest);
            length = c_collectionType(module)->maxSize;
            if (length == 0) {
                length = c_arraySize(ar);
            }
            if (c_typeIsRef(refType)) {
                for (i=0;i<length;i++) {
                    c_cloneIn(refType, ar[i], &destar[i]);
                }
            } else {
                if (c_typeHasRef(refType)) {
                    size = refType->size;
                    for (i=0;i<length;i++) {
                        _cloneReference(refType, C_DISPLACE(data, (i*size)), C_DISPLACE(dest, (i*size)));
                    }
                }
            }
        break;
        case OSPL_C_SEQUENCE:
            seq = c_sequence(data);
            destseq = c_sequence(dest);
            length = c_sequenceSize(seq);
            if (c_typeIsRef(refType)) {
                for (i=0;i<length;i++) {
                    c_cloneIn(refType, seq[i], &destseq[i]);
                }
            } else {
                if (c_typeHasRef(refType)) {
                    size = refType->size;
                    for (i=0;i<length;i++) {
                        _cloneReference(refType, C_DISPLACE(seq, (i*size)), C_DISPLACE(dest, (i*size)));
                    }
                }
            }
        break;
        default:
            // OS_REPORT(OS_ERROR,
            //       "c__cloneReferences",0,
            //       "illegal collectionType found");
        break;
        }
    break;
    case M_BASE:
    break;
    case M_TYPEDEF:
        c__cloneReferences(c_type(c_typeDef(module)->alias), data, dest);
    break;
    case M_ATTRIBUTE:
    case M_RELATION:
        refType = c_typeActualType(c_property(module)->type);
        _cloneReference(refType,
                C_DISPLACE(data, c_property(module)->offset),
                C_DISPLACE(dest, c_property(module)->offset));
    break;
    case M_MEMBER:
        refType = c_typeActualType(c_specifier(module)->type);
        _cloneReference(refType,
                C_DISPLACE(data, c_member(module)->offset),
                C_DISPLACE(dest, c_member(module)->offset));
    break;
    case M_UNIONCASE:
        refType = c_typeActualType(c_specifier(module)->type);
        _cloneReference(refType, data, dest);
    break;
    case M_MODULE:
        /* Do nothing */
    break;
    case M_PRIMITIVE:
        /* Do nothing */
    break;
    default:
        // OS_REPORT(OS_ERROR,
        //           "c__cloneReferences",0,
        //           "illegal meta object specified");
        assert(FALSE);
        return FALSE;
    }
    return TRUE;
}

c_bool
_cloneReference (
    c_type type,
    const void *data,
    c_voidp dest)
{
    c_type t = type;

    assert(data);
    assert(type);

    while (c_baseObject(t)->kind == M_TYPEDEF) {
        t = c_typeDef(t)->alias;
    }
    switch (c_baseObject(t)->kind) {
    case M_CLASS:
    case M_INTERFACE:
    case M_ANNOTATION:
        c_cloneIn(t, C_REFGET(data, 0), (c_voidp *) dest);
    break;
    case M_BASE:
    case M_COLLECTION:
        if ((c_collectionType(t)->kind == OSPL_C_ARRAY) &&
            (c_collectionType(t)->maxSize != 0)) {
            c__cloneReferences(t, data, dest);
        } else {
            c_cloneIn(t, C_REFGET(data, 0), (c_voidp *) dest);
        }
    break;
    case M_EXCEPTION:
    case M_STRUCTURE:
    case M_UNION:
        c__cloneReferences(t, data, dest);
    break;
    default:
        // OS_REPORT(OS_ERROR,
        //           "cloneReference",0,
        //           "illegal object detected");
        assert(FALSE);
        return FALSE;
    }
    return TRUE;
}


c_field
c_fieldConcat (
    c_field head,
    c_field tail)
{
    c_base base;
    c_ulong i, len1, len2, totlen;
    c_field field;
    c_bool headIsRef = FALSE;

    base = c__getBase(head);

    if (c_typeIsRef(head->type)) {
        headIsRef = TRUE;
    };

    len1 = (c_ulong) c_arraySize(head->path);
    len2 = (c_ulong) c_arraySize(tail->path);

    field = c_new(c_field_t(base));

    if (field) {
        field->type = c_keep(tail->type);
        field->kind = tail->kind;
        field->path = c_newArray(c_collectionType(c_fieldPath_t(base)), len1 + len2);
        for (i=0;i<len1;i++) {
            field->path[i] = c_keep(head->path[i]);
        }
        for (i=0;i<len2;i++) {
            field->path[i+len1] = c_keep(tail->path[i]);
        }

        len1 = c_arraySize(head->refs);
        len2 = c_arraySize(tail->refs);

        totlen = len1 + len2 + (headIsRef ? 1 : 0);
        if (totlen == 0) {
            field->refs = NULL;
        } else {
            field->refs = c_newArray(c_fieldRefs_t(base),totlen);
            for (i = 0; i < len1; i++) {
                field->refs[i] = head->refs[i];
            }
            if (headIsRef) {
                field->refs[len1++] = (c_voidp)head->offset;
            }
            for (i = len1; i < totlen; i++) {
                field->refs[i] = tail->refs[i - len1];
            }
        }

        /* If the tail does not add any additional refs (in which case
         * len2 equals 0 AND the head field is not a ref itself so that
         * totlen = len1), then the tail offset may be added to the head
         * offset.
         * In all other cases the tail already refers to the correct offset
         * of the inner-most indirection.
         */
        if (totlen == len1) {
            field->offset = head->offset + tail->offset;
        } else {
            field->offset = tail->offset;
        }

        len1 = (c_ulong) strlen(head->name);
        len2 = (c_ulong) strlen(tail->name);

        field->name = c_stringMalloc(base,len1+len2+2);
        ////
        os_sprintf(field->name,"%s.%s",head->name,tail->name);
    } else {
        // OS_REPORT(OS_ERROR,
        //           "database::c_fieldConcat",0,
        //           "Failed to allocate c_field object.");
    }

    return field;
}


c_type
c_field_t (
    c_base _this)
{
    return _this->baseCache.fieldCache.c_field_t;
}


c_iter
cf_elementGetAttributes(
    cf_element element)
{
    c_iter i = NULL;

    assert(element != NULL);

    cf_nodeListWalk(element->attributes, attributesToIterator, &i);

    return i;
}

unsigned int
attributesToIterator(
    c_object o,
    c_iter *c)
{
    *c = c_iterInsert(*c, o);
    return 1;
}

c_bool
cf_nodeListWalk(
    cf_nodeList list,
    cf_nodeWalkAction action,
    cf_nodeWalkActionArg arg)
{
    c_bool result;
    c_long i;
    unsigned int actionResult;

    result = TRUE;
    actionResult = 1;
    for (i = 0; (i < list->nrNodes) && ((int)actionResult > 0); i++) {
        actionResult = action(list->theList[i], arg);
        if ((int)actionResult == 0) {
            result = FALSE;
        }
    }
    return result;
}


c_iter
cf_elementGetChilds(
    cf_element element)
{
    c_iter i = NULL;

    assert(element != NULL);

    cf_nodeListWalk(element->children, childsToIterator, &i);

    return i;
}

unsigned int
childsToIterator(
    c_object o,
    c_iter *c)
{
    *c = c_iterInsert(*c, o);
    return 1;
}


c_field
c_fieldNew (
    c_type type,
    const c_char *fieldName)
{
    c_array path;
    c_field field;
    c_metaObject o;
    c_ulong n,length;
    c_address offset;
    c_iter nameList, refsList;
    c_string name;
    c_base base;

    if ((fieldName == NULL) || (type == NULL)) {
        // OS_REPORT(OS_ERROR,
        //           "c_fieldNew failed",0,
        //           "illegal parameter");
        return NULL;
    }

    base = c__getBase(type);
    if (base == NULL) {
        // OS_REPORT(OS_ERROR,
        //           "c_fieldNew failed",0,
        //           "failed to retreive base");
        return NULL;
    }

    ////
    nameList = c_splitString(fieldName,".");
    length = c_iterLength(nameList);
    field = NULL;

    if (length > 0) {
        o = NULL;
        offset = 0;
        refsList = NULL;
        path = c_newArray(c_collectionType(c_fieldPath_t(base)),length);
        if (path) {
            for (n=0;n<length;n++) {
                name = c_iterTakeFirst(nameList);
                o = c_metaResolve(c_metaObject(type),name);
                ddsrt_free(name);
                if (o == NULL) {
                    c_iterWalk(nameList,(c_iterWalkAction)ddsrt_free,NULL);
                    c_iterFree(nameList);
                    c_iterFree(refsList);
                    c_free(path);
                    return NULL;
                }
                path[n] = o;
                switch (c_baseObject(o)->kind) {
                case M_ATTRIBUTE:
                case M_RELATION:
                    type = c_property(o)->type;
                    offset += c_property(o)->offset;
                break;
                case M_MEMBER:
                    type = c_specifier(o)->type;
                    offset += c_member(o)->offset;
                break;
                default:
                    c_iterWalk(nameList,(c_iterWalkAction)ddsrt_free,NULL);
                    c_iterFree(nameList);
                    c_iterFree(refsList);
                    c_free(path);
                    return NULL;
                }
                if (n < length -1) {
                    switch (c_baseObject(type)->kind) {
                    case M_INTERFACE:
                    case M_CLASS:
                    case M_COLLECTION:
                        /*Longs are inserted in an iterator? Explanation please...*/
                        refsList = c_iterInsert(refsList,(c_voidp)offset);
                        offset = 0;
                    break;
                    default:
                    break;
                    }
                }
            }

            field = c_new(c_field_t(base));
            field->offset = offset;
            field->name = c_stringNew(base,fieldName);
            field->path = path;
            field->type = c_keep(type);
            field->kind = c_metaValueKind(o);
            field->refs = NULL;

            if (refsList) {
                length = c_iterLength(refsList);
                if (length > 0) {
                    field->refs = c_newArray(c_collectionType(c_fieldRefs_t(base)),length);
                    if (field->refs) {
                        n = length;
                        while (n-- > 0) {
                            field->refs[n] = c_iterTakeFirst(refsList);
                        }
                    } else {
                        // OS_REPORT(OS_ERROR,
                        //           "c_fieldNew failed",0,
                        //           "failed to allocate field->refs array");
                        c_free(field);
                        field = NULL;
                    }
                }
                c_iterFree(refsList);
            }
        } else {
            // OS_REPORT(OS_ERROR,
            //           "c_fieldNew failed",0,
            //           "failed to allocate field->path array");
            c_iterWalk(nameList,(c_iterWalkAction)ddsrt_free,NULL);
        }
    } else {
        // OS_REPORT(OS_ERROR,
        //             "c_fieldNew failed",0,
        //             "failed to process field name <%s>",
        //             fieldName);
    }
    c_iterFree(nameList);
    return field;
}

void c_iterWalk(c_iter iter, c_iterWalkAction action, c_iterActionArg actionArg)
{
    if (iter != NULL) {
        struct c__iterImpl_iter it;
        void *o;
        for (o = c__iterImpl_iter_first(&iter->x, &it); o != NULL; o = c__iterImpl_iter_next(&it)) {
            action(o, actionArg);
        }
    }
}

const c_char *
cf_nodeGetName (
    cf_node node)
{
    assert(node != NULL);

    return (const c_char *)node->name;
}


c_bool
c_objectIsType(
    c_baseObject o)
{
    if (o == NULL) {
        return FALSE;
    }
    switch(o->kind) {
    case M_TYPEDEF:
    case M_CLASS:
    case M_COLLECTION:
    case M_ENUMERATION:
    case M_EXCEPTION:
    case M_INTERFACE:
    case M_ANNOTATION:
    case M_PRIMITIVE:
    case M_STRUCTURE:
    case M_UNION:
    case M_BASE:     /* meta types are also types */
        return TRUE;
    default:
        return FALSE;
    }
}


c_type
c_fieldType (
    c_field _this)
{
    return (_this == NULL ? NULL : c_keep(_this->type));
}



c_metaObject
c_scopeLookup(
    c_scope scope,
    const c_char *name,
    c_ulong metaFilter)
{
    C_STRUCT(c_metaObject) tmp;
    c_binding binding;
    c_metaObject o;

    tmp.name = (char *) name;
    if ((binding = ddsrt_avl_clookup (&c_scope_bindings_td, &scope->bindings, &tmp)) == NULL) {
        o = NULL;
    } else if (CQ_KIND_IN_MASK (binding->object, metaFilter)) {
        o = c_keep(binding->object);
    } else {
        o = NULL;
    }
    return o;
}


c_bool
c_scopeExists (
    c_scope scope,
    const c_char *name)
{
    C_STRUCT(c_metaObject) tmp;
    tmp.name = (char *) name;
    return (ddsrt_avl_clookup (&c_scope_bindings_td, &scope->bindings, &tmp) != NULL);
}


int
os_sprintf(
    char *s,
    const char *format,
    ...)
{
   int result;
   va_list args;

   va_start(args, format);

   result = vsprintf(s, format, args);

   va_end(args);

   return result;
}



int
os_strncasecmp(
    const char *s1,
    const char *s2,
    size_t n)
{
    int cr = 0;

    while (*s1 && *s2 && n) {
        cr = tolower(*s1) - tolower(*s2);
        if (cr) {
            return cr;
        }
        s1++;
        s2++;
        n--;
    }
    if (n) {
        cr = tolower(*s1) - tolower(*s2);
    }
    return cr;
}


long long
os_strtoll(
    const char *str,
    char **endptr,
    int32_t base)
{
    size_t cnt = 0;
    long long tot = 1;
    unsigned long long max = OS_MAX_INTEGER(long long);

    assert (str != NULL);

    for (; isspace(str[cnt]); cnt++) {
        /* ignore leading whitespace */
    }

    if (str[cnt] == '-') {
        tot = -1;
        max++;
        cnt++;
    } else if (str[cnt] == '+') {
        cnt++;
    }

    tot *= (long long) os__strtoull__ (str + cnt, endptr, base, max);

    if (endptr && *endptr == (str + cnt)) {
       *endptr = (char *)str;
    }

    return tot;
}


unsigned long long
os__strtoull__ (
    const char *str,
    char **endptr,
    int32_t base,
    unsigned long long max)
{
    int err = 0;
    int num;
    size_t cnt = 0;
    unsigned long long tot = 0;

    assert (str != NULL);

    if (base == 0) {
        if (str[0] == '0') {
            if ((str[1] == 'x' || str[1] == 'X') && os_todigit (str[2]) < 16) {
                base = 16;
                cnt = 2;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
            cnt = 2;
        }
    } else if (base < 2 || base > 36) {
        err = EINVAL;
    }

    while (!err && (num = os_todigit (str[cnt])) >= 0 && num < base) {
        if (tot <= (max / (unsigned) base)) {
            tot *= (unsigned) base;
            tot += (unsigned) num;
            cnt++;
        } else {
            err = ERANGE;
            tot = max;
        }
    }

    if (endptr != NULL) {
        *endptr = (char *)str + cnt;
    }

    if (err) {
        errno = err;
    }

    return tot;
}

int
os_todigit (
    const int chr)
{
    int num = -1;

    if (chr >= '0' && chr <= '9') {
        num = chr - '0';
    } else if (chr >= 'a' && chr <= 'z') {
        num = 10 + (chr - 'a');
    } else if (chr >= 'A' && chr <= 'Z') {
        num = 10 + (chr - 'A');
    }

    return num;
}


unsigned long long
os_strtoull (
    const char *str,
    char **endptr,
    int32_t base)
{
    size_t cnt = 0;
    unsigned long long tot = 1;
    unsigned long long max = OS_MAX_INTEGER(unsigned long long);

    assert (str != NULL);

    for (; isspace(str[cnt]); cnt++) {
        /* ignore leading whitespace */
    }

    if (str[cnt] == '-') {
        tot = (unsigned long long) -1;
        cnt++;
    } else if (str[cnt] == '+') {
        cnt++;
    }

    tot *= os__strtoull__ (str + cnt, endptr, base, max);

    if (endptr && *endptr == (str + cnt)) {
       *endptr = (char *)str;
    }

    return tot;
}

float
os_strtof(const char *nptr, char **endptr) {
    /* Just use os_strtod(). */
    return (float)os_strtod(nptr, endptr);
}


double
os_strtod(const char *nptr, char **endptr) {
    double ret;

    if (os_lcNumericGet() == '.') {
        /* The current locale uses '.', so we can use the
         * standard functions as is. */
        ret = strtod(nptr, endptr);
    } else {
        /* The current locale uses ',', so we can not use the
         * standard functions as is, but have to do extra work
         * because ospl uses "x.x" doubles (notice the dot).
         * Just copy the string and replace the LC_NUMERIC. */
        char  nptrCopy[DOUBLE_STRING_MAX_LENGTH];
        char *nptrCopyIdx;
        char *nptrCopyEnd;
        char *nptrIdx;

        /* It is possible that the given nptr just starts with a double
         * representation but continues with other data.
         * To be able to copy not too much and not too little, we have
         * to scan across nptr until we detect the doubles' end. */
        nptrIdx = (char*)nptr;
        nptrCopyIdx = nptrCopy;
        nptrCopyEnd = nptrCopyIdx + DOUBLE_STRING_MAX_LENGTH - 1;
        while (VALID_DOUBLE_CHAR(*nptrIdx) && (nptrCopyIdx < nptrCopyEnd)) {
            if (*nptrIdx == '.') {
                /* Replace '.' with locale LC_NUMERIC to get strtod to work. */
                *nptrCopyIdx = os_lcNumericGet();
            } else {
                *nptrCopyIdx = *nptrIdx;
            }
            nptrIdx++;
            nptrCopyIdx++;
        }
        *nptrCopyIdx = '\0';

        /* Now that we have a copy with the proper locale LC_NUMERIC,
         * we can use strtod() for the conversion. */
        ret = strtod(nptrCopy, &nptrCopyEnd);

        /* Calculate the proper end char when needed. */
        if (endptr != NULL) {
            *endptr = (char*)nptr + (nptrCopyEnd - nptrCopy);
        }
    }
    return ret;
}


char *
os_str_trim (
    const char *str,
    const char *trim)
{
    char *lim, *off, *ptr;

    assert (str != NULL);

    if (trim == NULL) {
        trim = OS_STR_SPACE;
    }

    off = os_strchrs (str, trim, OS_FALSE);
    if (off != NULL) {
        lim = os_strrchrs (off, trim, OS_FALSE);
        assert (lim != NULL);
        if (lim[1] != '\0') {
            ptr = ddsrt_strndup (off, (size_t)((lim - off) + 1));
        } else if (off != str) {
            ptr = ddsrt_strdup (off);
        } else {
            ptr = (char *)str;
        }
    } else {
        ptr = ddsrt_strdup ("");
    }

    return ptr;
}


char *
os_strchrs (
    const char *str,
    const char *chrs,
    os_boolean inc)
{
    os_boolean eq;
    char *ptr = NULL;
    size_t i, j;

    assert (str != NULL);
    assert (chrs != NULL);

    for (i = 0; str[i] != '\0' && ptr == NULL; i++) {
        for (j = 0, eq = OS_FALSE; chrs[j] != '\0' && eq == OS_FALSE; j++) {
            if (str[i] == chrs[j]) {
                eq = OS_TRUE;
            }
        }
        if (eq == inc) {
            ptr = (char *)str + i;
        }
    }

    return ptr;
}

char *
os_strrchrs (
    const char *str,
    const char *chrs,
    os_boolean inc)
{
    os_boolean eq;
    char *ptr = NULL;
    size_t i, j;

    assert (str != NULL);
    assert (chrs != NULL);

    for (i = 0; str[i] != '\0'; i++) {
        for (j = 0, eq = OS_FALSE; chrs[j] != '\0' && eq == OS_FALSE; j++) {
            if (str[i] == chrs[j]) {
                eq = OS_TRUE;
            }
        }
        if (eq == inc) {
            ptr = (char *)str + i;
        }
    }

    return ptr;
}


char *
os_strncpy(
    char *s1,
    const char *s2,
    size_t num)
{
   return strncpy(s1, s2, num);
}


int
os_strcasecmp(
    const char *s1,
    const char *s2)
{
    int cr;

    while (*s1 && *s2) {
        cr = tolower(*s1) - tolower(*s2);
        if (cr) {
            return cr;
        }
        s1++;
        s2++;
    }
    cr = tolower(*s1) - tolower(*s2);
    return cr;
}



c_iter
c_splitString(
    const c_char *str,
    const c_char *delimiters)
{
    const c_char *head, *tail;
    c_char *nibble;
    c_iter iter = NULL;
    size_t length;

    if (str == NULL) return NULL;

    tail = str;
    while (*tail != '\0') {
        head = c_skipUntil(tail,delimiters);
        length = (size_t) (head - tail);
        if (length != 0) {
            length++;
            nibble = (c_string)ddsrt_malloc(length);
            os_strncpy(nibble,tail,length);
            nibble[length-1]=0;
            iter = c_iterAppend(iter,nibble);
        }
        tail = head;
        if (c_isOneOf(*tail,delimiters)) tail++;
    }
    return iter;
}

c_char *
c_skipUntil (
    const c_char *str,
    const c_char *symbolList)
{
    c_char *ptr = (c_char *)str;

    assert(symbolList != NULL);
    if (ptr == NULL) return NULL;

    while (*ptr != '\0' && !c_isOneOf(*ptr,symbolList)) ptr++;
    return ptr;
}


void
c_fieldAssign (
    c_field field,
    c_object o,
    c_value v)
{
    c_voidp p = get_field_address (field, o);
    v.kind = field->kind;

#define _VAL_(f,t) *((t *)p) = v.is.f

    switch(v.kind) {
    case V_ADDRESS:   _VAL_(Address,c_address); break;
    case V_BOOLEAN:   _VAL_(Boolean,c_bool); break;
    case V_SHORT:     _VAL_(Short,c_short); break;
    case V_LONG:      _VAL_(Long,c_long); break;
    case V_LONGLONG:  _VAL_(LongLong,c_longlong); break;
    case V_OCTET:     _VAL_(Octet,c_octet); break;
    case V_USHORT:    _VAL_(UShort,c_ushort); break;
    case V_ULONG:     _VAL_(ULong,c_ulong); break;
    case V_ULONGLONG: _VAL_(ULongLong,c_ulonglong); break;
    case V_CHAR:      _VAL_(Char,c_char); break;
    case V_WCHAR:     _VAL_(WChar,c_wchar); break;
    case V_STRING:
        c_free((c_object)(*(c_string *)p));
        _VAL_(String,c_string);
        c_keep((c_object)(*(c_string *)p));
    break;
    case V_WSTRING:
        c_free((c_object)(*(c_wstring *)p));
        _VAL_(WString,c_wstring);
        c_keep((c_object)(*(c_wstring *)p));
    break;
    case V_FLOAT:     _VAL_(Float,c_float); break;
    case V_DOUBLE:    _VAL_(Double,c_double); break;
    case V_OBJECT:
        c_free(*(c_object *)p);
        _VAL_(Object,c_object);
        c_keep(*(c_object *)p);
    break;
    case V_VOIDP:
        _VAL_(Voidp,c_voidp);
    break;
    case V_FIXED:
    case V_UNDEFINED:
    case V_COUNT:
        // OS_REPORT(OS_ERROR,"c_fieldAssign failed",0,
        //             "illegal field value kind (%d)", v.kind);
        assert(FALSE);
    break;
    }
#undef _VAL_
}


char
os_lcNumericGet(void)
{
    static char lcNumeric = ' ';

    /* Detect lcNumeric only once. */
    if (lcNumeric == ' ') {
        /* There could be multiple threads here, but it is still save and works.
         * Only side effect is that possibly multiple os_reports are traced. */
        char num[] = { '\0', '\0', '\0', '\0' };
        snprintf(num, 4, "%3f", 2.2);
        lcNumeric = num [1];
        if (lcNumeric != '.') {
            // OS_REPORT(OS_WARNING, "os_stdlib", 0,
            //           "Locale with LC_NUMERIC \'%c\' detected, which is not '.'. This can decrease performance.",
            //           lcNumeric);
        }
    }

    return lcNumeric;
}


void
c_fieldInit (
    c_base base)
{
    c_object scope;

    base->baseCache.fieldCache.c_fieldPath_t =
         c_metaArrayTypeNew(c_metaObject(base),
                            "C_ARRAY<c_base>",
                            c_getMetaType(base,M_BASE),
                            0);
    base->baseCache.fieldCache.c_fieldRefs_t =
         c_metaArrayTypeNew(c_metaObject(base),
                            "C_ARRAY<c_address>",
                            c_address_t(base),
                            0);

    scope = c_metaDeclare((c_object)base,"c_field",M_CLASS);
        C_META_ATTRIBUTE_(c_field,scope,kind,c_valueKind_t(base));
        C_META_ATTRIBUTE_(c_field,scope,offset,c_address_t(base));
        C_META_ATTRIBUTE_(c_field,scope,name,c_string_t(base));
        C_META_ATTRIBUTE_(c_field,scope,path,c_fieldPath_t(base));
        C_META_ATTRIBUTE_(c_field,scope,refs,c_fieldRefs_t(base));
        C_META_ATTRIBUTE_(c_field,scope,type,c_type_t(base));
    c__metaFinalize(scope,FALSE);
    base->baseCache.fieldCache.c_field_t = scope;
    c_free(scope);
}



void
c_querybaseInit(
    c_base base)
{
    c_object scope,module;
    c_object o;
    c_object type;

    module = c_metaDeclare(c_object(base),"c_querybase",M_MODULE);

    o = c_metaDefine(module,M_ENUMERATION);
        c_qBoundKind_t(base) = o;
        c_enumeration(o)->elements = c_arrayNew(c_constant_t(base),3);
        c_enumeration(o)->elements[0] = c_metaDeclareEnumElement(module,"B_UNDEFINED");
        c_enumeration(o)->elements[1] = c_metaDeclareEnumElement(module,"B_INCLUDE");
        c_enumeration(o)->elements[2] = c_metaDeclareEnumElement(module,"B_EXCLUDE");
    c_metaFinalize(o);
    c_metaBind(module,"c_qBoundKind",o);
    c_free(o);

    o = c_metaDefine(module,M_ENUMERATION);
        c_qKind_t(base) = o;
        c_enumeration(o)->elements = c_arrayNew(c_constant_t(base),12);
        c_enumeration(o)->elements[0] = c_metaDeclareEnumElement(module,"CQ_FIELD");
        c_enumeration(o)->elements[1] = c_metaDeclareEnumElement(module,"CQ_CONST");
        c_enumeration(o)->elements[2] = c_metaDeclareEnumElement(module,"CQ_AND");
        c_enumeration(o)->elements[3] = c_metaDeclareEnumElement(module,"CQ_OR");
        c_enumeration(o)->elements[4] = c_metaDeclareEnumElement(module,"CQ_EQ");
        c_enumeration(o)->elements[5] = c_metaDeclareEnumElement(module,"CQ_NE");
        c_enumeration(o)->elements[6] = c_metaDeclareEnumElement(module,"CQ_LT");
        c_enumeration(o)->elements[7] = c_metaDeclareEnumElement(module,"CQ_LE");
        c_enumeration(o)->elements[8] = c_metaDeclareEnumElement(module,"CQ_GT");
        c_enumeration(o)->elements[9] = c_metaDeclareEnumElement(module,"CQ_GE");
        c_enumeration(o)->elements[10] = c_metaDeclareEnumElement(module,"CQ_LIKE");
        c_enumeration(o)->elements[11] = c_metaDeclareEnumElement(module,"CQ_NOT");
    c_metaFinalize(o);
    c_metaBind(module,"c_qKind",o);
    c_free(o);

    scope = c_metaDeclare(module,"c_qExpr",M_CLASS);
        c_qExpr_t(base) = scope;
        c_class(scope)->extends = NULL;
        C_META_ATTRIBUTE_(c_qExpr,scope,kind,c_qKind_t(base));
    c__metaFinalize(scope,FALSE);
    c_free(scope);

    scope = c_metaDeclare(module,"c_qConst",M_CLASS);
        c_qConst_t(base) = scope;
        c_class(scope)->extends = c_class(c_keep(c_qExpr_t(base)));
        type = ResolveType(scope,"c_value");
        C_META_ATTRIBUTE_(c_qConst,scope,value,type);
        c_free(type);
    c__metaFinalize(scope,FALSE);
    c_free(scope);

    scope = c_metaDeclare(module,"c_qType",M_CLASS);
        c_qType_t(base) = scope;
        c_class(scope)->extends = c_class(c_keep(c_qExpr_t(base)));
        C_META_ATTRIBUTE_(c_qType,scope,type,c_type_t(base));
    c__metaFinalize(scope,FALSE);
    c_free(scope);

    scope = c_metaDeclare(module,"c_qField",M_CLASS);
        c_qField_t(base) = scope;
        c_class(scope)->extends = c_class(c_keep(c_qExpr_t(base)));
        C_META_ATTRIBUTE_(c_qField,scope,field,c_field_t(base));
    c__metaFinalize(scope,FALSE);
    c_free(scope);

    scope = c_metaDeclare(module,"c_qFunc",M_CLASS);
        c_qFunc_t(base) = scope;
        type = c_metaDefine(scope,M_COLLECTION);
            c_metaObject(type)->name = c_stringNew(base,"ARRAY<c_qExpr>");
            c_collectionType(type)->kind = OSPL_C_ARRAY;
            c_collectionType(type)->subType = c_keep(c_qExpr_t(base));
            c_collectionType(type)->maxSize = 0;
        c_metaFinalize(type);

        c_class(scope)->extends = c_keep(c_qExpr_t(base));
        C_META_ATTRIBUTE_(c_qFunc,scope,params,type);
        c_free(type);
    c__metaFinalize(scope,FALSE);
    c_free(scope);

    scope = c_metaDeclare(module,"c_qRange",M_CLASS);
        c_qRange_t(base) = scope;
        c_class(scope)->extends = NULL;
        C_META_ATTRIBUTE_(c_qRange,scope,startKind,c_qBoundKind_t(base));
        C_META_ATTRIBUTE_(c_qRange,scope,endKind,c_qBoundKind_t(base));
        C_META_ATTRIBUTE_(c_qRange,scope,startExpr,c_qExpr_t(base));
        C_META_ATTRIBUTE_(c_qRange,scope,endExpr,c_qExpr_t(base));
        type = ResolveType(scope,"c_value");
        C_META_ATTRIBUTE_(c_qRange,scope,start,type);
        C_META_ATTRIBUTE_(c_qRange,scope,end,type);
        c_free(type);
    c__metaFinalize(scope,FALSE);
    c_free(scope);

    scope = c_metaDeclare(module,"c_qKey",M_CLASS);
        c_qKey_t(base) = scope;
        c_class(scope)->extends = NULL;
        C_META_ATTRIBUTE_(c_qKey,scope,expr,c_qExpr_t(base));
        C_META_ATTRIBUTE_(c_qKey,scope,field,c_field_t(base));
        type = c_metaDefine(scope,M_COLLECTION);
            c_metaObject(type)->name = c_stringNew(base,"ARRAY<c_qRange>");
            c_collectionType(type)->kind = OSPL_C_ARRAY;
            c_collectionType(type)->subType = c_keep(c_qRange_t(base));
            c_collectionType(type)->maxSize = 0;
        c_metaFinalize(type);
        C_META_ATTRIBUTE_(c_qKey,scope,range,type);
        c_free(type);
    c__metaFinalize(scope,FALSE);
    c_free(scope);

    scope = c_metaDeclare(module,"c_qVar",M_CLASS);
        c_qVar_t(base) = scope;
        c_class(scope)->extends = NULL;
        C_META_ATTRIBUTE_(c_qVar,scope,hasChanged,c_bool_t(base));
        C_META_ATTRIBUTE_(c_qVar,scope,id,c_long_t(base));
        type = c_metaDefine(scope,M_COLLECTION);
            c_metaObject(type)->name = c_stringNew(base,"SET<c_qKey>");
            c_collectionType(type)->kind = OSPL_C_SET;
            c_collectionType(type)->subType = c_keep(c_qKey_t(base));
            c_collectionType(type)->maxSize = 0;
        c_metaFinalize(type);
        C_META_ATTRIBUTE_(c_qVar,scope,keys,type);
        c_free(type);
        C_META_ATTRIBUTE_(c_qVar,scope,variable,c_qConst_t(base));
        C_META_ATTRIBUTE_(c_qVar,scope,type,c_type_t(base));

    c__metaFinalize(scope,FALSE);
    c_free(scope);

    scope = c_metaDeclare(module,"c_qPred",M_CLASS);
        c_qPred_t(base) = scope;
        c_class(scope)->extends = NULL;
        C_META_ATTRIBUTE_(c_qPred,scope,fixed,c_bool_t(base));
        C_META_ATTRIBUTE_(c_qPred,scope,expr,c_qExpr_t(base));
        type = c_metaDefine(scope,M_COLLECTION);
            c_metaObject(type)->name = c_stringNew(base,"ARRAY<c_qKey>");
            c_collectionType(type)->kind = OSPL_C_ARRAY;
            c_collectionType(type)->subType = c_keep(c_qKey_t(base));
            c_collectionType(type)->maxSize = 0;
        c_metaFinalize(type);
        C_META_ATTRIBUTE_(c_qPred,scope,keyField,type);
        c_free(type);
        type = c_metaDefine(scope,M_COLLECTION);
            c_metaObject(type)->name = c_stringNew(base,"ARRAY<c_qVar>");
            c_collectionType(type)->kind = OSPL_C_ARRAY;
            c_collectionType(type)->subType = c_keep(c_qVar_t(base));
            c_collectionType(type)->maxSize = 0;
        c_metaFinalize(type);
        C_META_ATTRIBUTE_(c_qPred,scope,varList,type);
        c_free(type);
        C_META_ATTRIBUTE_(c_qPred,scope,next,c_qPred_t(base));
    c__metaFinalize(scope,FALSE);
    c_free(scope);
    c_free(module);

#undef ResolveType
}