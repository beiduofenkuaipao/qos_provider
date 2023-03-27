#include "c_base.h"
#include "fun_base.h"


const size_t c_listSize  = C_SIZEOF(c_list);
const size_t c_setSize   = C_SIZEOF(c_set);
const size_t c_bagSize   = C_SIZEOF(c_bag);
const size_t c_tableSize = C_SIZEOF(c_table);
const size_t c_querySize = C_SIZEOF(c_query);



static const ddsrt_avl_treedef_t c_base_bindings_td =
  DDSRT_AVL_TREEDEF_INITIALIZER_INDKEY (offsetof (C_STRUCT(c_baseBinding), avlnode),
                                     offsetof (C_STRUCT(c_baseBinding), name),
                                     strcmp, 0);


_TYPE_CACHE_(c_voidp)
_TYPE_CACHE_(c_wstring)
_TYPE_CACHE_(c_constant)
_TYPE_CACHE_(c_member)
_TYPE_CACHE_(c_property)
_TYPE_CACHE_(c_double)
_TYPE_CACHE_(c_string)
_TYPE_CACHE_(c_octet)
_TYPE_CACHE_(c_float)
_TYPE_CACHE_(c_wchar)
_TYPE_CACHE_(c_address)
_TYPE_CACHE_(c_ushort)




_TYPE_CACHE_(c_object)
_TYPE_CACHE_(c_bool)
_TYPE_CACHE_(c_char)
_TYPE_CACHE_(c_short)
_TYPE_CACHE_(c_long)
_TYPE_CACHE_(c_longlong)
_TYPE_CACHE_(c_uchar)
_TYPE_CACHE_(c_ulong)
_TYPE_CACHE_(c_ulonglong)
_TYPE_CACHE_(c_type)
_TYPE_CACHE_(c_valueKind)
_TYPE_CACHE_(pa_uint32_t)
_TYPE_CACHE_(pa_uintptr_t)
_TYPE_CACHE_(pa_voidp_t)
_TYPE_CACHE_(c_literal)
_TYPE_CACHE_(c_unionCase)


void freeBindings (void *binding, void *arg)
{
    c_baseBinding b = (c_baseBinding)binding;
    bindArgp a = (bindArgp)arg;
    c_header header;
    c_type type;
    c_object o;

    a->trashcan->references = c_iterInsert(a->trashcan->references, b->object);
    while ((o = c_iterTakeFirst(a->trashcan->references)) != NULL) {
        header = c_header(o);
        /* skip already freed or corrupted */
        assert(header->confidence == CONFIDENCE);
        if ((ddsrt_atomic_ld32(&header->refCount) & REFCOUNT_FLAG_GARBAGE) == 0) {
            ddsrt_atomic_or32(&header->refCount, REFCOUNT_FLAG_GARBAGE);

            OBJECTTYPE(type,o);
            walkReferences(c_metaObject(type),o,a->trashcan);
            if (c_baseObjectKind(type) == M_COLLECTION) {
                switch (c_collectionTypeKind(type)) {
                case OSPL_C_ARRAY:
                case OSPL_C_SEQUENCE:
                    a->trashcan->arrays = c_iterInsert(a->trashcan->arrays, o);
                break;
                case OSPL_C_SCOPE:
                    a->trashcan->scopes = c_iterInsert(a->trashcan->scopes, o);
                break;
                case OSPL_C_STRING:
                    a->trashcan->trash = c_iterInsert(a->trashcan->trash, o);
                break;
                default:
                    a->trashcan->collections = c_iterInsert(a->trashcan->collections, o);
                }
            } else {
                a->trashcan->trash = c_iterInsert(a->trashcan->trash, o);
            }
        }
    }
    c_free(b->name);
    c_mmFree(a->mm, b);
}


c_valueKind
c_metaValueKind(
    c_metaObject o)
{
    switch (c_baseObjectKind(o)) {
    case M_PRIMITIVE:
        switch (c_primitive(o)->kind) {
        case P_ADDRESS:   return V_ADDRESS;
        case P_BOOLEAN:   return V_BOOLEAN;
        case P_CHAR:      return V_CHAR;
        case P_WCHAR:     return V_WCHAR;
        case P_OCTET:     return V_OCTET;
        case P_SHORT:     return V_SHORT;
        case P_USHORT:    return V_USHORT;
        case P_LONG:      return V_LONG;
        case P_ULONG:     return V_ULONG;
        case P_LONGLONG:  return V_LONGLONG;
        case P_ULONGLONG: return V_ULONGLONG;
        case P_FLOAT:     return V_FLOAT;
        case P_DOUBLE:    return V_DOUBLE;
        case P_VOIDP:     return V_VOIDP;
        default:          assert(FALSE);
        break;
        }
    break;
    case M_ENUMERATION:
        return V_LONG;
    case M_ATTRIBUTE:
    case M_RELATION:
        return c_metaValueKind(c_metaObject(c_property(o)->type));
    case M_MEMBER:
        return c_metaValueKind(c_metaObject(c_specifier(o)->type));
    case M_TYPEDEF:
        return c_metaValueKind(c_metaObject(c_typeDef(o)->alias));
    case M_CONSTANT:
        return c_metaValueKind(c_metaObject(c_constant(o)->type));
    case M_CONSTOPERAND:
        return c_metaValueKind(c_metaObject(c_constOperand(o)->constant));
    case M_LITERAL:
        return c_literal(o)->value.kind;
    case M_OPERATION:
        return c_metaValueKind(c_metaObject(c_operation(o)->result));
    case M_PARAMETER:
    case M_UNIONCASE:
        return c_metaValueKind(c_metaObject(c_specifier(o)->type));
    case M_COLLECTION:
        if (c_collectionType(o)->kind == OSPL_C_STRING) return V_STRING;
    case M_CLASS:
    case M_INTERFACE:
    case M_ANNOTATION:
    case M_MODULE:
    case M_BASE:
    case M_STRUCTURE:
    case M_EXCEPTION:
    case M_UNION:
        return V_OBJECT;

    case M_EXPRESSION:
    case M_UNDEFINED:
    default:
        assert(FALSE);
    break;
    }
    return V_UNDEFINED;
}

void
c_freelistInsert(
    c_freelist freelist,
    c_object object)
{
    if (object == NULL) return;
    assert(c_header(object)->confidence == CONFIDENCE);
    if (freelist->head_idx == REFBUF_SIZE) {
        freelist->head_buf->next = ddsrt_malloc(sizeof(C_STRUCT(c_refbuf)));
        freelist->head_buf = freelist->head_buf->next;
        freelist->head_buf->next = NULL;
        freelist->head_idx = 0;
    }
    freelist->head_buf->element[freelist->head_idx++] = object;
}

c_ulong
c_arraySize(
    c_array _this)
{
    if (_this) {
        assert(c_header(_this)->confidence == CONFIDENCE);
        assert(
            /* For backward compatibility purposes this function can still be used
             * to obtain the length of sequences as well, but new code should use
             * the c_sequenceSize operation instead.
             */
            (c_baseObjectKind(c_header(_this)->type) == M_COLLECTION)
            && (
                    (c_collectionTypeKind(c_header(_this)->type) == OSPL_C_ARRAY)
                    || (c_collectionTypeKind(c_header(_this)->type) == OSPL_C_SEQUENCE)
               )
            );
        return (c_ulong)(c_arrayHeader(_this)->size);
    } else {
        return 0;
    }
}

c_bool
_freeReference (
    c_voidp *p,
    c_type type,
    c_freelist freelist)
{
    c_type t = type;

    assert(p);
    assert(t);

    while (c_baseObject(t)->kind == M_TYPEDEF) {
        t = c_typeDef(t)->alias;
    }
    switch (c_baseObject(t)->kind) {
    case M_CLASS:
    case M_INTERFACE:
    case M_ANNOTATION:
        c_freelistInsert(freelist, c_object(*p));
    break;
    case M_BASE:
    case M_COLLECTION:
        if ((c_collectionType(t)->kind == OSPL_C_ARRAY) &&
            (c_collectionType(t)->maxSize != 0)) {
            c_freeReferences(c_metaObject(t), p, freelist);
        } else {
            c_freelistInsert(freelist, c_object(*p));
        }
    break;
    case M_EXCEPTION:
    case M_STRUCTURE:
    case M_UNION:
        c_freeReferences(c_metaObject(type),p, freelist);
    break;
    case M_PRIMITIVE:
        ////switch (c_primitive(t)->kind) {
        //case P_MUTEX:
        //    c_mutexDestroy((c_mutex *)p);
        //break;
        // case P_LOCK:
        //     c_lockDestroy((c_lock *)p);
        // break;
        // case P_COND:
        //     c_condDestroy((c_cond *)p);
        // break;
        //default:
        //break;
        //}
    break;
    default:
        // OS_REPORT(OS_ERROR,
        //           "freeReference",0,
        //           "illegal object detected");
        assert(FALSE);
        return FALSE;
    }
    return TRUE;
}


c_bool
_c_freeReferences (
    c_metaObject metaObject,
    c_object o,
    c_freelist freelist)
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

    assert(metaObject);
    assert(o);

    switch (c_baseObject(metaObject)->kind) {
    case M_CLASS:
       
        if (metaObject == c_metaObject(c_type(metaObject)->base->metaType[M_ENUMERATION])) {
            c_array elements = ((c_enumeration)o)->elements;
            length = c_arraySize(elements);
            for (i=0; i<length; i++) {
                c_constant(elements[i])->type = NULL;
            }
        }
        cls = c_class(metaObject);
        while (cls) {
            length = c_arraySize(c_interface(cls)->references);
            for (i=0;i<length;i++) {
                property = c_property(c_interface(cls)->references[i]);
                type = property->type;
                freeReference(C_DISPLACE(o,property->offset),type, freelist);
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
            freeReference(C_DISPLACE(o,property->offset),type, freelist);
        }
    break;
    case M_EXCEPTION:
    case M_STRUCTURE:
        length = c_arraySize(c_structure(metaObject)->references);
        for (i=0;i<length;i++) {
            member = c_member(c_structure(metaObject)->references[i]);
            type = c_specifier(member)->type;
            freeReference(C_DISPLACE(o,member->offset),type, freelist);
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
            // OS_REPORT(OS_ERROR,
            //           "c_freeReferences",0,
            //           "illegal union switch type detected");
            assert(FALSE);
            return FALSE;
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
                        c_freeReferences(c_metaObject(references[i]),
                                         C_DISPLACE(o,c_type(metaObject)->alignment), freelist);
                        type = c_specifier(references[i])->type;
                    }
                    j++;
                }
                i++;
            }
        }
    break;
    case M_COLLECTION:
        switch (c_collectionType(metaObject)->kind) {
        case OSPL_C_ARRAY:
        case OSPL_C_SEQUENCE:
            ACTUALTYPE(type,c_collectionType(metaObject)->subType);
            ar = (c_array)o;

            if(c_collectionType(metaObject)->kind == OSPL_C_ARRAY
                && c_collectionType(metaObject)->maxSize > 0){
                length = c_collectionType(metaObject)->maxSize;
            } else {
                length = c_arraySize(ar);
            }

            if (c_typeIsRef(type)) {
                for (i=0;i<length;i++) {
                    c_freelistInsert(freelist, ar[i]);
                }
            } else {
                if (c_typeHasRef(type)) {
                    size = type->size;
                    for (i=0;i<length;i++) {
                        freeReference(C_DISPLACE(ar,(i*size)),type, freelist);
                    }
                }
            }
        break;
        case OSPL_C_SCOPE:
            c_scopeDeinit(c_scope(o));
        break;
        default:
            c_clear(o);
        break;
        }
    break;
    case M_BASE:
    break;
    case M_TYPEDEF:
        c_freeReferences(c_metaObject(c_typeDef(metaObject)->alias),o, freelist);
    break;
    case M_ATTRIBUTE:
    case M_RELATION:
        ACTUALTYPE(type,c_property(metaObject)->type);
        freeReference(C_DISPLACE(o,c_property(metaObject)->offset),type, freelist);
    break;
    case M_MEMBER:
        ACTUALTYPE(type,c_specifier(metaObject)->type);
        freeReference(C_DISPLACE(o,c_member(metaObject)->offset),type, freelist);
    break;
    case M_UNIONCASE:
        ACTUALTYPE(type,c_specifier(metaObject)->type);
        freeReference(o,type, freelist);
    break;
    case M_MODULE:
        c_freelistInsert(freelist, c_module(o)->scope);
    break;
    case M_PRIMITIVE:
        /* Do nothing */
    break;
    default:
        // OS_REPORT(OS_ERROR,
        //           "c_freeReferences",0,
        //           "illegal meta object specified");
        assert(FALSE);
        return FALSE;
    }
    return TRUE;
}

void
c_freelistInit(
    c_freelist freelist)
{
    c_refbuf(freelist)->next = NULL;
    freelist->tail_buf = c_refbuf(freelist);
    freelist->head_buf = c_refbuf(freelist);
    freelist->tail_idx = 0;
    freelist->head_idx = 0;
}

c_object
c_freelistTake(
    c_freelist freelist)
{
    c_object object = NULL;
    c_refbuf freebuf;

    if (freelist->tail_idx == REFBUF_SIZE) {
        /* Reached the end of a buffer so it is empty and if allocated can be freed. */
        freelist->tail_idx = 0;
        freebuf = freelist->tail_buf;
        freelist->tail_buf = freelist->tail_buf->next;

        if (freebuf != c_refbuf(freelist)) {
            /* The empty buffer was allocated so free it now. */
            assert(freebuf == c_refbuf(freelist)->next);
            ddsrt_free(freebuf);
        }
        c_refbuf(freelist)->next = freelist->tail_buf;
        if (freelist->tail_buf == NULL) {
            /* If the tail_buf is NULL then the queue is empty meaning that it can conveniently
             * be reinitialized as a new freelist.
             */
            c_freelistInit(freelist);
        }
    }
    if (freelist->head_idx > freelist->tail_idx || freelist->head_buf != freelist->tail_buf )
    {
        /* the freelist is not empty so take an object from the freelist. */
        object = freelist->tail_buf->element[freelist->tail_idx++];
    } else if (freelist->tail_buf != c_refbuf(freelist)) {
        /* The freelist is empty but also has an allocated buffer so free it now. */
        ddsrt_free(freelist->tail_buf);
    }
    return object;
}

c_mm
c_baseMM(
    c_base base)
{
    if (base == NULL) {
        return NULL;
    }
    return base->mm;
}

c_type
c_getType(
    c_object object)
{
    c_type type;
    if (object == NULL) {
        return NULL;
    }
    type = c_header(object)->type;
    return type;
}

void
deleteGarbage(
    c_base base)
{
    C_STRUCT(c_trashcan) trashcan;
    c_mm mm;
    c_object trash;
    struct bindArg barg;

    if (base == NULL) return;

    assert(base->confidence == CONFIDENCE);

    mm = base->mm;

    trashcan.trash = NULL;
    trashcan.arrays = NULL;
    trashcan.scopes = NULL;
    trashcan.collections = NULL;
    trashcan.references = NULL;

    barg.trashcan = &trashcan;
    barg.mm = mm;

    ddsrt_avl_free_arg (&c_base_bindings_td, &base->bindings, freeBindings, &barg);
    ////OS_REPORT_NOW(OS_INFO,"Database close",0,-1,"Removed %d objects",c_iterLength(trashcan.trash));

    while ((trash = c_iterTakeFirst(trashcan.scopes)) != NULL)
    {
        c_scopeClean(c_scope(trash));
        c_mmFree(mm, c_header(trash));
    }
    while ((trash = c_iterTakeFirst(trashcan.collections)) != NULL)
    {
        c_clear(trash);
        c_mmFree(mm, c_header(trash));
    }
    while ((trash = c_iterTakeFirst(trashcan.arrays)) != NULL)
    {
        c_mmFree(mm, c_arrayHeader(trash));
    }
    while ((trash = c_iterTakeFirst(trashcan.trash)) != NULL)
    {
        c_mmFree(mm, c_header(trash));
    }
    c_iterFree(trashcan.trash);
    c_iterFree(trashcan.arrays);
    c_iterFree(trashcan.scopes);
    c_iterFree(trashcan.collections);
    c_iterFree(trashcan.references);
}


void
c_destroy (
    c_base _this)
{
    c_mm mm = _this->mm;
    deleteGarbage(_this);
    c_mmDestroy(mm);
}

void
c_mmDestroy(
    c_mm mm)
{
    ddsrt_free(mm);
}

c_object
c_keep (
    c_object object)
{
    c_header header;
    uint32_t oldCount;

    if (object == NULL) {
        return NULL;
    }

    header = c_header(object);
    assert(header->confidence == CONFIDENCE);
#if CHECK_REF
    if (header->type) {
        c_type type;
        ACTUALTYPE(type,header->type);
        if (type && c_metaObject(type)->name) {
            if (strlen(c_metaObject(type)->name) >= CHECK_REF_TYPE_LEN) {
                if (strncmp(c_metaObject(type)->name, CHECK_REF_TYPE, CHECK_REF_TYPE_LEN) == 0) {
                    uint32_t refc = ddsrt_atomic_ld32(&header->refCount) & REFCOUNT_MASK;
                    UT_TRACE("\n\n============ Keep(%p): %d -> %d =============\n", object, refc, refc+1);
                }
            }
        }
    }
#endif
    oldCount = ddsrt_atomic_ld32(&header->refCount);
    assert((oldCount & REFCOUNT_MASK) > 0);
    ddsrt_atomic_inc32(&header->refCount); /* FIXME: should add ddsrt_atomic_inc32_ov and use it here */
    if (oldCount & REFCOUNT_FLAG_CLAMP) {
        /* Protect against wrap-around of reference count by forcing
         * the refcount up once half the range (24 bits) has been used
         * up. The memory will then never be freed, but a memory leak
         * is still better than a crash. This resets the flags, which
         * is fine for the current set of flags
         */
        ddsrt_atomic_st32 (&header->refCount, (REFCOUNT_FLAG_CLAMP | (REFCOUNT_FLAG_CLAMP >> 1)));
    }
    if (oldCount & REFCOUNT_FLAG_TRACE) {
        c_type headerType = header->type, type;
        void *block;
        ACTUALTYPE (type, headerType);
        if ((c_baseObjectKind (type) == M_COLLECTION) && ((c_collectionTypeKind (type) == OSPL_C_ARRAY) || (c_collectionTypeKind (type) == OSPL_C_SEQUENCE))) {
            block = c_arrayHeader (object);
        } else {
            block = header;
        }
        c_mmTrackObject (type->base->mm, block, C_MMTRACKOBJECT_CODE_MIN + 0);
    }

    return object;
}

void
collectScopeGarbage(
c_metaObject o,
c_voidp trashcan)
{
    CHECKOBJECT(o);
    ((c_trashcan)trashcan)->references = c_iterAppend(((c_trashcan)trashcan)->references, c_keep(o));
}

c_bool
collectGarbage(
c_object o,
c_voidp trashcan)
{
    CHECKOBJECT(o);
    ((c_trashcan)trashcan)->references = c_iterAppend(((c_trashcan)trashcan)->references, c_keep(o));
    return TRUE;
}

c_base
c_create (
    const c_char *name,
    void *address,
    c_size size,
    c_size threshold)
{
    c_mm  mm;
    c_base base = NULL;
    c_base tempbase;
    c_header header;

    if ((size != 0) && (size < MIN_DB_SIZE)) {
        // OS_REPORT(OS_ERROR,"c_base::c_create",0,
        //             "Specified memory size (%"PA_PRIuSIZE") is too small to occupy a c_base instance,"
        //             "required minimum size is %d bytes.",
        //             size,MIN_DB_SIZE);
        return NULL;
    }
    mm = c_mmCreate(address,size, threshold);
    if (mm == NULL) {
        /* error is reported in c_mmCreate */
        return NULL;
    }
    header = (c_header)c_mmMalloc(mm, MEMSIZE(C_SIZEOF(c_base)));
    if (header) {
#ifndef NDEBUG
        header->confidence = CONFIDENCE;
#ifdef OBJECT_WALK
        header->nextObject = NULL;
        header->prevObject = NULL;
#endif
#endif
        ddsrt_atomic_st32(&header->refCount, 1);
        header->type = NULL; /* Will be set in c_baseInit after bootstrapping */
        tempbase = (c_base)c_oid(header);
        base = (c_base)c_mmBind(mm, name, tempbase);
        if (base != tempbase) {
            // OS_REPORT(OS_ERROR,
            //             "c_base::c_create",0,
            //             "Internal error, memory management seems corrupted.\n"
            //             "             mm = 0x%"PA_PRIxADDR", name = %s,\n"
            //             "             tempbase = 0x%"PA_PRIxADDR", base = 0x%"PA_PRIxADDR"",
            //             (os_address)mm, name ? name : "(null)", (os_address)tempbase, (os_address)base);
            c_mmFree(mm, header);
            return NULL;
        }

        c_baseInit(base, mm);

        ospl_c_bind(base,"c_baseModule");
    }
    return base;
}


c_object
ospl_c_bind (
    c_object object,
    const c_char *name)
{
    ddsrt_avl_ipath_t p;
    c_baseBinding binding;
    c_base base;

    base = c_header(object)->type->base;
    assert(base->confidence == CONFIDENCE);

    if ((binding = ddsrt_avl_lookup_ipath (&c_base_bindings_td, &base->bindings, name, &p)) != NULL) {
        return binding->object;
    } else if ((binding = (c_baseBinding) c_mmMalloc (base->mm, sizeof (*binding))) == NULL) {
        return NULL;
    } else {
        binding->name = c_stringNew(base,name);
        binding->object = c_keep(object);
        ddsrt_avl_insert_ipath (&c_base_bindings_td, &base->bindings, binding, &p);
        return binding->object;
    }
}

c_string
c_stringNew(
    c_base base,
    const c_char *str)
{
    c_string s;
    size_t len;

    assert(base);

    if (str == NULL) {
        return NULL;
    }

    len = strlen(str) + 1;
    if((s = c_stringMalloc(base, len)) != NULL && len > 1){
        /* str is nul-terminated (since we could determine len), so memcpy str
         * including the nul-terminator.
         */
        memcpy(s, str, len);
    }

    /* c_wstringNew/Malloc always return a nul-terminated string */
    assert(!s || s[len - 1] == '\0');
    return s;
}

c_string
c_stringMalloc(
    c_base base,
    c_size length)
{
    assert(base);
    assert(length > 0);

    if (length == 1){
        /* Empty strings are interned. The only valid string with length 1 is the
         * 'empty' string, so in this case return a reference to the intern. This
         * saves precious resources in case of big amounts of empty strings, since
         * each empty string does not only contain the '\0' terminator but also
         * 36-40 bytes of header information.
         */
        assert(base->emptyString[0] == '\0');
        return c_keep(base->emptyString);
    }

    return c__stringMalloc(base, length, FALSE);
}


c_string
c__stringMalloc(
    c_base base,
    c_size length,
    c_bool check)
{
    c_header header;
    c_string s = NULL;

    assert(base);
    assert(length > 0);

    if (check) {
        header = (c_header)c_mmMallocThreshold(c_baseMM(base), MEMSIZE(length));
    } else {
        header = (c_header)c_mmMalloc(c_baseMM(base), MEMSIZE(length));
        if (!header) {
            abort();
        }
    }
    if (header) {
#if TYPE_REFC_COUNTS_OBJECTS
        header->type = c_keep(base->string_type);
#else
        header->type = base->string_type;
#endif
        if (base->maintainObjectCount) {
            ddsrt_atomic_inc32(&base->string_type->objectCount);
        }
        ddsrt_atomic_st32(&header->refCount, 1 | REFCOUNT_FLAG_ATOMIC);
        s = (c_string)c_oid(header);
        s[0] = '\0'; /* c_stringNew/Malloc always return a null-terminated string */
#ifndef NDEBUG
        header->confidence = CONFIDENCE;
#ifdef OBJECT_WALK
        header->nextObject = NULL;
        c_header(base->lastObject)->nextObject = s;
        header->prevObject = base->lastObject;
        assert(base->firstObject != NULL);
        base->lastObject = s;
#endif
#endif
    }
    return s;
}


c_object
c_newBaseArrayObject (
    c_collectionType arrayType,
    c_ulong size)
{
    return c__newBaseArrayObjectCommon(arrayType, size, FALSE);
}


c_object
c__newBaseArrayObjectCommon (
    c_collectionType arrayType,
    c_ulong size,
    c_bool check)
{
    c_size allocSize;
    c_object o = NULL;

    assert(arrayType);
    assert(
        (c_collectionTypeKind(arrayType) == OSPL_C_ARRAY && size > 0)
        ||
        (c_collectionTypeKind(arrayType) == OSPL_C_SEQUENCE)
        );

    if ((c_collectionTypeKind(arrayType) == OSPL_C_ARRAY)
         || (c_collectionTypeKind(arrayType) == OSPL_C_SEQUENCE)) {
        if (    (c_collectionTypeKind(arrayType) == OSPL_C_ARRAY && size > 0)
                ||
                (c_collectionTypeKind(arrayType) == OSPL_C_SEQUENCE) ) {
            c_arrayHeader hdr;
            c_header header;
            c_base base;
            c_type subType;

            subType = c_collectionTypeSubType(arrayType);

            switch(c_baseObjectKind(subType)) {
            case M_INTERFACE:
            case M_CLASS:
            case M_ANNOTATION:
                allocSize = size * sizeof(void *);
            break;
            default:
                if (subType->size == 0) {
                    subType->size = sizeof(void *);
                }
                allocSize = size * subType->size;
            break;
            }

            base = c_type(arrayType)->base;
            if (check) {
                hdr = (c_arrayHeader)c_mmMallocThreshold(base->mm, ARRAYMEMSIZE(allocSize));
            } else {
                hdr = (c_arrayHeader)c_mmMalloc(base->mm, ARRAYMEMSIZE(allocSize));
                if (!hdr) {
                    abort();
                }
            }

            if (hdr) {
                uint32_t traceType;

                hdr->size = size;
                header = (c_header)&hdr->_parent;

                traceType = (ddsrt_atomic_ld32(&c_header(arrayType)->refCount) & REFCOUNT_FLAG_TRACETYPE);
                ddsrt_atomic_st32(&header->refCount, 1);
                if (traceType) {
                    ddsrt_atomic_or32(&header->refCount, REFCOUNT_FLAG_TRACE);
                    c_mmTrackObject (base->mm, header, C_MMTRACKOBJECT_CODE_MIN + 2);
                }
                /* Keep reference to our type */
#if TYPE_REFC_COUNTS_OBJECTS
                header->type = c_keep(c_type(arrayType));
#else
                header->type = c_type(arrayType);
#endif
                if (base->maintainObjectCount) {
                    ddsrt_atomic_inc32(&header->type->objectCount);
                }

                o = c_oid(header);

                /* When an array is freed via c_free, it also checks whether it contains
                 * references and if they need to be freed. If the user did not fill the whole array
                 * a c_free on garbage data could be performed, which causes undefined behaviour.
                 * Therefore the whole array is set on 0.
                 */
                memset(o, 0, allocSize);

#ifndef NDEBUG
                header->confidence = CONFIDENCE;
#ifdef OBJECT_WALK
                header->nextObject = NULL;
                c_header(arrayType->base->lastObject)->nextObject = o;
                header->prevObject = arrayType->base->lastObject;
                assert(arrayType->base->firstObject != NULL);
                arrayType->base->lastObject = o;
#endif
#endif
            }
        } else {
            // OS_REPORT(OS_ERROR,
            //             "Database c_newBaseArrayObject",0,
            //             "Illegal size %d specified", size);
        }
    } else {
        // OS_REPORT(OS_ERROR,
        //           "Database c_newBaseArrayObject",0,
        //           "Specified type is not an array nor a sequence");
    }
    return o;
}

static void
c_queryCacheInit (C_STRUCT(c_queryCache) *queryCache)
{
    queryCache->c_qConst_t = NULL;
    queryCache->c_qType_t = NULL;
    queryCache->c_qVar_t = NULL;
    queryCache->c_qField_t = NULL;
    queryCache->c_qFunc_t = NULL;
    queryCache->c_qPred_t = NULL;
    queryCache->c_qKey_t = NULL;
    queryCache->c_qRange_t = NULL;
    queryCache->c_qExpr_t = NULL;
}

static void
c_fieldCacheInit (C_STRUCT(c_fieldCache) *fieldCache)
{
    fieldCache->c_field_t = NULL;
    fieldCache->c_fieldPath_t = NULL;
    fieldCache->c_fieldRefs_t = NULL;
}

static void
c_typeCacheInit (C_STRUCT(c_typeCache) *typeCache)
{
    typeCache->c_object_t = NULL;
    typeCache->c_voidp_t = NULL;
    typeCache->c_bool_t = NULL;
    typeCache->c_address_t = NULL;
    typeCache->c_octet_t = NULL;
    typeCache->c_char_t = NULL;
    typeCache->c_short_t = NULL;
    typeCache->c_long_t = NULL;
    typeCache->c_longlong_t = NULL;
    typeCache->c_uchar_t = NULL;
    typeCache->c_ushort_t = NULL;
    typeCache->c_ulong_t = NULL;
    typeCache->c_ulonglong_t = NULL;
    typeCache->c_float_t = NULL;
    typeCache->c_double_t = NULL;
    typeCache->c_string_t = NULL;
    typeCache->c_wchar_t = NULL;
    typeCache->c_wstring_t = NULL;
    typeCache->c_type_t = NULL;
    typeCache->c_valueKind_t = NULL;
    typeCache->c_member_t = NULL;
    typeCache->c_literal_t = NULL;
    typeCache->c_constant_t = NULL;
    typeCache->c_unionCase_t = NULL;
    typeCache->c_property_t = NULL;
    typeCache->pa_uint32_t_t = NULL;
    typeCache->pa_uintptr_t_t = NULL;
    typeCache->pa_voidp_t_t = NULL;
}



void
c_baseInit (
    c_base base,
    c_mm mm)
{
    c_type type,scopeType;
    c_header header;
    c_metaObject o,found,_intern,temp;
    c_array members;
    c_iter labels;
    size_t size;
    c_ulong caseNumber;

    /* A c_base inherits as follows:
     *
     * c_baseObject <- c_metaObject <- c_module <- c_base
     *
     * All fields are initialised where possible before the bootstrap. */

    /* First, attach mm to base. */
    base->mm = mm;
    base->maintainObjectCount = FALSE;
    base->y2038Ready = FALSE;

    /* c_baseObject init */
    c_baseObject(base)->kind = M_MODULE;

    /* c_metaObject init */
    c_metaObject(base)->definedIn = NULL;
    c_metaObject(base)->name = NULL;

    /* c_module init */
    ////c_mutexInit
    ////(void)c_mutexInit(base, &((c_module)base)->mtx);
    /* scope is initialized when scope-type is bootstrapped */

    /* c_base init */
    base->confidence = CONFIDENCE;
    ddsrt_avl_init (&c_base_bindings_td, &base->bindings);
    ////(void)c_mutexInit(base, &base->bindLock);
    ////(void)c_mutexInit(base, &base->serLock);

    /* metaType[M_COUNT], string_type and emptyString are initialized when types
     * are available.
     */

    /* c_base init */
    c_queryCacheInit (&base->baseCache.queryCache);
    c_fieldCacheInit (&base->baseCache.fieldCache);
    c_typeCacheInit (&base->baseCache.typeCache);

#ifndef NDEBUG
#ifdef OBJECT_WALK
    base->firstObject = base;
    base->lastObject = base;
#endif
#endif

    /* Declare class type.
     * this is required because all meta meta objects are defined as class.
     */
    size = MEMSIZE(C_SIZEOF(c_class));
    header = (c_header)c_mmMalloc(base->mm,size);
    if (!header) {
    return;
    }
    memset(header,0,size);
    ddsrt_atomic_st32(&header->refCount, 1);
    header->type = NULL;
#ifndef NDEBUG
    header->confidence = CONFIDENCE;
#endif
    o = c_metaObject(c_oid(header));
#ifndef NDEBUG
#ifdef OBJECT_WALK
    header->nextObject = NULL;
    c_header(base->lastObject)->nextObject = o;
    header->prevObject = base->lastObject;
    assert(base->firstObject != NULL);
    base->lastObject = o;
#endif
#endif
    c_type(o)->base = base;
    c_baseObject(o)->kind = M_CLASS;
    c_type (o)->alignment = C_ALIGNMENT_C_STRUCT (c_class);
    c_type(o)->size = C_SIZEOF(c_class);
    base->metaType[M_CLASS] = c_type(o);
    header->type = (c_type) o;
    c_keep(o);

    o = c_metaObject(c_new(base->metaType[M_CLASS]));
    c_baseObject(o)->kind = M_CLASS;
    C_META_TYPEINIT_(o,c_collectionType);
    base->metaType[M_COLLECTION] = c_type(o);

    /* Declare scope class type.
     * this is required because all meta meta objects are managed in the base scope.
     */

    scopeType = c_type(c_new(base->metaType[M_COLLECTION]));
    c_baseObject(scopeType)->kind = M_COLLECTION;
    c_collectionType(scopeType)->kind = OSPL_C_SCOPE;
    c_collectionType(scopeType)->maxSize = 0;
    C_META_TYPEINIT_(scopeType,c_scope);

    /* Declare base class type and initialize the base scope.
     * this is required because all meta meta objects are bound to the base scope.
     */

    o = c_metaObject(c_new(base->metaType[M_CLASS]));
    c_baseObject(o)->kind = M_CLASS;
    C_META_TYPEINIT_(o,c_base);
    base->metaType[M_BASE] = c_type(o);

    /* Overwrite as header->type points to type of metatype class
     * i.o. NULL
     */
    c_header(base)->type = c_keep(o);

    /* c_module->scope init */
    c_module(base)->scope = c_scope(c_new(scopeType));
    c_scopeInit(c_module(base)->scope);

    /* Declare c_string type, this is required to be able to bind objects to names. */
    o = c_metaObject(c_new(base->metaType[M_COLLECTION]));
    c_baseObject(o)->kind = M_COLLECTION;
    c_collectionType(o)->kind = OSPL_C_STRING;
    c_metaTypeInit(o,sizeof(c_string),C_ALIGNMENT(c_string));

    base->string_type = c_keep(o);
    found = c_metaBind(c_metaObject(base),"c_string",o);
    assert(found == o);
    c_free(found);
    c_free(o);

    o = c_metaObject(base->metaType[M_BASE]);
    found = c_metaBind(c_metaObject(base),"c_base",o);
    assert(found == o);
    c_free(found);

    o = c_metaObject(base->metaType[M_CLASS]);
    found = c_metaBind(c_metaObject(base),"c_class",o);
    assert(found == o);
    c_free(found);

    o = c_metaObject(base->metaType[M_COLLECTION]);
    found = c_metaBind(c_metaObject(base),"c_collectionType",o);
    assert(found == o);
    c_free(found);

    found = c_metaBind(c_metaObject(base),"c_scope",c_metaObject(scopeType));
    assert(found == c_metaObject(scopeType));
    c_free(found);

    /* Now allocate, bind and pre-initialize all meta meta objects.
     * pre-initialize will only set size and kind making the meta meta objects
     * ready to be used for meta creation.
     * At this point reflection will be unavailable until the meta meta objects
     * are fully initialized.
     */

    /* Initialize the interned empty string. Strings can now be allocated, so
     * even though in the baseInit no empty strings are used, initialisation
     * is needed here to guarantee the assertions of c_stringNew and
     * c_stringMalloc.
     */
    base->emptyString = c__stringMalloc(base, 1, FALSE);
    base->emptyString[0] = '\0';

#define _META_(b,t) c_object(c_newMetaClass(b,((const c_char *)#t),C_SIZEOF(t),C_ALIGNMENT_C_STRUCT(t)))

    /** Declare abstract meta types **/
    c_free(_META_(base,c_baseObject));
    c_free(_META_(base,c_operand));
    c_free(_META_(base,c_specifier));
    c_free(_META_(base,c_metaObject));

    base->baseCache.typeCache.c_property_t = _META_(base,c_property);
    base->baseCache.typeCache.c_type_t = _META_(base,c_type);

    /* At last set the subType of c_scope type. */
    c_collectionType(scopeType)->subType = ResolveType(base,"c_metaObject");
    c_free(scopeType); /* we can now free our local ref as we don't use it anymore */
    scopeType = NULL;

    /* Declare meta types */
    base->metaType[M_LITERAL] =      _META_(base,c_literal);
    base->metaType[M_CONSTOPERAND] = _META_(base,c_constOperand);
    base->metaType[M_EXPRESSION] =   _META_(base,c_expression);
    base->metaType[M_PARAMETER] =    _META_(base,c_parameter);
    base->metaType[M_MEMBER] =       _META_(base,c_member);
    base->metaType[M_UNIONCASE] =    _META_(base,c_unionCase);
    base->metaType[M_ATTRIBUTE] =    _META_(base,c_attribute);
    base->metaType[M_RELATION] =     _META_(base,c_relation);
    base->metaType[M_MODULE] =       _META_(base,c_module);
    base->metaType[M_CONSTANT] =     _META_(base,c_constant);
    base->metaType[M_OPERATION] =    _META_(base,c_operation);
    base->metaType[M_TYPEDEF] =      _META_(base,c_typeDef);
    base->metaType[M_PRIMITIVE] =    _META_(base,c_primitive);
    base->metaType[M_ENUMERATION] =  _META_(base,c_enumeration);
    base->metaType[M_UNION] =        _META_(base,c_union);
    base->metaType[M_STRUCTURE] =    _META_(base,c_structure);
    base->metaType[M_EXCEPTION] =    _META_(base,c_exception);
    base->metaType[M_INTERFACE] =    _META_(base,c_interface);
    base->metaType[M_ANNOTATION] =   _META_(base,c_annotation);

    base->baseCache.typeCache.c_literal_t = base->metaType[M_LITERAL];
    base->baseCache.typeCache.c_member_t  = base->metaType[M_MEMBER];
    base->baseCache.typeCache.c_unionCase_t = base->metaType[M_UNIONCASE];
    base->baseCache.typeCache.c_constant_t = base->metaType[M_CONSTANT];

    c_free(base->metaType[M_LITERAL]);
    c_free(base->metaType[M_CONSTOPERAND]);
    c_free(base->metaType[M_EXPRESSION]);
    c_free(base->metaType[M_PARAMETER]);
    c_free(base->metaType[M_MEMBER]);
    c_free(base->metaType[M_UNIONCASE]);
    c_free(base->metaType[M_ATTRIBUTE]);
    c_free(base->metaType[M_RELATION]);
    c_free(base->metaType[M_MODULE]);
    c_free(base->metaType[M_CONSTANT]);
    c_free(base->metaType[M_OPERATION]);
    c_free(base->metaType[M_TYPEDEF]);
    c_free(base->metaType[M_PRIMITIVE]);
    c_free(base->metaType[M_ENUMERATION]);
    c_free(base->metaType[M_UNION]);
    c_free(base->metaType[M_STRUCTURE]);
    c_free(base->metaType[M_EXCEPTION]);
    c_free(base->metaType[M_INTERFACE]);
    c_free(base->metaType[M_ANNOTATION]);

#undef _META_

    /* Now allocation of meta objects is operational.
     * For initialization of the meta meta object we need to allocate the meta objects
     * for all internal types.
     */

    /* Definition of the meta objects specifying all internal primitive types. */

#define INITPRIM(s,n,k) \
    o = c_metaDeclare(c_metaObject(s),#n,M_PRIMITIVE); \
    s->baseCache.typeCache.n##_t = c_type(o); \
    c_primitive(o)->kind = k; \
    c_metaFinalize(o); \
    c_free(o)

    INITPRIM(base,c_address,   P_ADDRESS);
    INITPRIM(base,c_bool,      P_BOOLEAN);
    INITPRIM(base,c_char,      P_CHAR);
    INITPRIM(base,c_wchar,     P_WCHAR);
    INITPRIM(base,c_octet,     P_OCTET);
    INITPRIM(base,c_short,     P_SHORT);
    INITPRIM(base,c_ushort,    P_USHORT);
    INITPRIM(base,c_long,      P_LONG);
    INITPRIM(base,c_ulong,     P_ULONG);
    INITPRIM(base,c_longlong,  P_LONGLONG);
    INITPRIM(base,c_ulonglong, P_ULONGLONG);
    INITPRIM(base,c_float,     P_FLOAT);
    INITPRIM(base,c_double,    P_DOUBLE);
    INITPRIM(base,c_voidp,     P_VOIDP);
    INITPRIM(base,c_mutex,     P_MUTEX);
    INITPRIM(base,c_lock,      P_LOCK);
    INITPRIM(base,c_cond,      P_COND);
    INITPRIM(base,pa_uint32_t, P_PA_UINT32);
    INITPRIM(base,pa_uintptr_t,P_PA_UINTPTR);
    INITPRIM(base,pa_voidp_t,  P_PA_VOIDP);

#undef INITPRIM

    o = c_metaDeclare(c_metaObject(base),"c_object",M_CLASS);
    base->baseCache.typeCache.c_object_t = c_type(o);
    c_metaFinalize(o);
    c_free(o);
    o = c_metaDeclare(c_metaObject(base),"c_any",M_CLASS);
    c_metaFinalize(o);
    c_free(o);

    c_collectionInit(base);

    base->baseCache.typeCache.c_string_t = ResolveType(base,"c_string");;
    base->baseCache.typeCache.c_wstring_t = ResolveType(base,"c_wstring");

#define _ENUMVAL_(e,v) \
    c_enumeration(e)->elements[v] = \
    c_metaDeclareEnumElement(c_metaObject(base),#v)

#define _ENUMVAL_PREFIX_(e,v) \
    c_enumeration(e)->elements[v] = \
    c_metaDeclareEnumElement(c_metaObject(base),#v + 5 /* OSPL_ prefix */)

    o = c_metaDefine(c_metaObject(base),M_ENUMERATION);
    /* The following array of objects should be defined as array of c_constant.
     * This is not a problem and is intentional because of the side effect.
     * The side effect is that a type ARRAY<c_object> is created in the database.
     * This type will otherwise not be available but it is required for c_clone to work.
     */
    c_enumeration(o)->elements = c_arrayNew(c_object_t(base),M_COUNT);
    _ENUMVAL_(o,M_UNDEFINED);
    _ENUMVAL_(o,M_ANNOTATION);
    _ENUMVAL_(o,M_ATTRIBUTE);
    _ENUMVAL_(o,M_CLASS);
    _ENUMVAL_(o,M_COLLECTION);
    _ENUMVAL_(o,M_CONSTANT);
    _ENUMVAL_(o,M_CONSTOPERAND);
    _ENUMVAL_(o,M_ENUMERATION);
    _ENUMVAL_(o,M_EXCEPTION);
    _ENUMVAL_(o,M_EXPRESSION);
    _ENUMVAL_(o,M_INTERFACE);
    _ENUMVAL_(o,M_LITERAL);
    _ENUMVAL_(o,M_MEMBER);
    _ENUMVAL_(o,M_MODULE);
    _ENUMVAL_(o,M_OPERATION);
    _ENUMVAL_(o,M_PARAMETER);
    _ENUMVAL_(o,M_PRIMITIVE);
    _ENUMVAL_(o,M_RELATION);
    _ENUMVAL_(o,M_BASE);
    _ENUMVAL_(o,M_STRUCTURE);
    _ENUMVAL_(o,M_TYPEDEF);
    _ENUMVAL_(o,M_UNION);
    _ENUMVAL_(o,M_UNIONCASE);
    c_metaFinalize(o);
    c_metaBind(c_metaObject(base),"c_metaKind",o);
    c_free(o);

    o = c_metaDefine(c_metaObject(base),M_ENUMERATION);
    c_enumeration(o)->elements = c_arrayNew(c_constant_t(base),OSPL_C_COUNT);
    _ENUMVAL_PREFIX_(o,OSPL_C_UNDEFINED);
    _ENUMVAL_PREFIX_(o,OSPL_C_LIST);
    _ENUMVAL_PREFIX_(o,OSPL_C_ARRAY);
    _ENUMVAL_PREFIX_(o,OSPL_C_BAG);
    _ENUMVAL_PREFIX_(o,OSPL_C_SET);
    _ENUMVAL_PREFIX_(o,OSPL_C_MAP);
    _ENUMVAL_PREFIX_(o,OSPL_C_DICTIONARY);
    _ENUMVAL_PREFIX_(o,OSPL_C_SEQUENCE);
    _ENUMVAL_PREFIX_(o,OSPL_C_STRING);
    _ENUMVAL_PREFIX_(o,OSPL_C_WSTRING);
    _ENUMVAL_PREFIX_(o,OSPL_C_QUERY);
    _ENUMVAL_PREFIX_(o,OSPL_C_SCOPE);
    c_metaFinalize(o);
    c_metaBind(c_metaObject(base),"c_collKind",o);
    c_free(o);

    o = c_metaDefine(c_metaObject(base),M_ENUMERATION);
    c_enumeration(o)->elements = c_arrayNew(c_constant_t(base),P_COUNT);
    _ENUMVAL_(o,P_UNDEFINED);
    _ENUMVAL_(o,P_ADDRESS);
    _ENUMVAL_(o,P_BOOLEAN);
    _ENUMVAL_(o,P_CHAR);
    _ENUMVAL_(o,P_WCHAR);
    _ENUMVAL_(o,P_OCTET);
    _ENUMVAL_(o,P_SHORT);
    _ENUMVAL_(o,P_USHORT);
    _ENUMVAL_(o,P_LONG);
    _ENUMVAL_(o,P_ULONG);
    _ENUMVAL_(o,P_LONGLONG);
    _ENUMVAL_(o,P_ULONGLONG);
    _ENUMVAL_(o,P_FLOAT);
    _ENUMVAL_(o,P_DOUBLE);
    _ENUMVAL_(o,P_VOIDP);
    _ENUMVAL_(o,P_MUTEX);
    _ENUMVAL_(o,P_LOCK);
    _ENUMVAL_(o,P_COND);
    _ENUMVAL_(o,P_PA_UINT32);
    _ENUMVAL_(o,P_PA_UINTPTR);
    _ENUMVAL_(o,P_PA_VOIDP);
    c_metaFinalize(o);
    c_metaBind(c_metaObject(base),"c_primKind",o);
    c_free(o);

    o = c_metaDefine(c_metaObject(base),M_ENUMERATION);
    c_enumeration(o)->elements = c_arrayNew(c_constant_t(base),E_COUNT);
    _ENUMVAL_(o,E_UNDEFINED);
    _ENUMVAL_(o,E_OR);
    _ENUMVAL_(o,E_XOR);
    _ENUMVAL_(o,E_AND);
    _ENUMVAL_(o,E_SHIFTRIGHT);
    _ENUMVAL_(o,E_SHIFTLEFT);
    _ENUMVAL_(o,E_PLUS);
    _ENUMVAL_(o,E_MINUS);
    _ENUMVAL_(o,E_MUL);
    _ENUMVAL_(o,E_DIV);
    _ENUMVAL_(o,E_MOD);
    _ENUMVAL_(o,E_NOT);
    c_metaFinalize(o);
    c_metaBind(c_metaObject(base),"c_exprKind",o);
    c_free(o);

    o = c_metaDefine(c_metaObject(base),M_ENUMERATION);
    c_enumeration(o)->elements = c_arrayNew(c_constant_t(base),D_COUNT);
    _ENUMVAL_(o,D_UNDEFINED);
    _ENUMVAL_(o,D_IN);
    _ENUMVAL_(o,D_OUT);
    _ENUMVAL_(o,D_INOUT);
    c_metaFinalize(o);
    c_metaBind(c_metaObject(base),"c_direction",o);
    c_free(o);

    o = c_metaDefine(c_metaObject(base),M_ENUMERATION);
    base->baseCache.typeCache.c_valueKind_t = c_type(o);
    c_enumeration(o)->elements = c_arrayNew(c_constant_t(base),V_COUNT);
    _ENUMVAL_(o,V_UNDEFINED);
    _ENUMVAL_(o,V_ADDRESS);
    _ENUMVAL_(o,V_BOOLEAN);
    _ENUMVAL_(o,V_OCTET);
    _ENUMVAL_(o,V_SHORT);
    _ENUMVAL_(o,V_LONG);
    _ENUMVAL_(o,V_LONGLONG);
    _ENUMVAL_(o,V_USHORT);
    _ENUMVAL_(o,V_ULONG);
    _ENUMVAL_(o,V_ULONGLONG);
    _ENUMVAL_(o,V_FLOAT);
    _ENUMVAL_(o,V_DOUBLE);
    _ENUMVAL_(o,V_CHAR);
    _ENUMVAL_(o,V_STRING);
    _ENUMVAL_(o,V_WCHAR);
    _ENUMVAL_(o,V_WSTRING);
    _ENUMVAL_(o,V_FIXED);
    _ENUMVAL_(o,V_OBJECT);
    _ENUMVAL_(o,V_VOIDP);
    c_metaFinalize(o);
    c_metaBind(c_metaObject(base),"c_valueKind",o);
    c_free(o);

#undef _ENUMVAL_
#undef _ENUMVAL_PREFIX_

    {
        C_ALIGNMENT_TYPE (c_threadId);
        o = c_metaDeclare (c_metaObject (base), "c_threadId", M_STRUCTURE);
        c_metaTypeInit (o, sizeof (c_threadId), C_ALIGNMENT (c_threadId));
        c_free (o);
    }

    o = c_metaDefine(c_metaObject(base),M_STRUCTURE);
    members = c_arrayNew(c_member_t(base),2);
    members[0] = (c_voidp)c_metaDefine(c_metaObject(base),M_MEMBER);
        c_specifier(members[0])->name = c_stringNew(base,"seconds");
        c_specifier(members[0])->type = c_keep(c_long_t(base));
    members[1] = (c_voidp)c_metaDefine(c_metaObject(base),M_MEMBER);
        c_specifier(members[1])->name = c_stringNew(base,"nanoseconds");
        c_specifier(members[1])->type = c_keep(c_ulong_t(base));
    c_structure(o)->members = members;
    c_metaObject(o)->definedIn = c_metaObject(base);
    c_metaFinalize(o);
    found = c_metaBind(c_metaObject(base),"c_time",o);
    c_free(found);
    c_free(o);

    o = c_metaDefine(c_metaObject(base),M_TYPEDEF);
    c_metaObject(o)->definedIn = c_metaObject(base);
    c_typeDef(o)->alias = c_keep(c_ulonglong_t(base));
    c_metaFinalize(o);
    found = c_metaBind(c_metaObject(base),"os_timeW",o);
    c_free(found);
    c_free(o);

    o = c_metaDefine(c_metaObject(base),M_TYPEDEF);
    c_metaObject(o)->definedIn = c_metaObject(base);
    c_typeDef(o)->alias = c_keep(c_ulonglong_t(base));
    c_metaFinalize(o);
    found = c_metaBind(c_metaObject(base),"os_timeM",o);
    c_free(found);
    c_free(o);

    o = c_metaDefine(c_metaObject(base),M_TYPEDEF);
    c_metaObject(o)->definedIn = c_metaObject(base);
    c_typeDef(o)->alias = c_keep(c_ulonglong_t(base));
    c_metaFinalize(o);
    found = c_metaBind(c_metaObject(base),"os_timeE",o);
    c_free(found);
    c_free(o);

    o = c_metaDefine(c_metaObject(base),M_TYPEDEF);
    c_metaObject(o)->definedIn = c_metaObject(base);
    c_typeDef(o)->alias = c_keep(c_longlong_t(base));
    c_metaFinalize(o);
    found = c_metaBind(c_metaObject(base),"os_duration",o);
    c_free(found);
    c_free(o);

    o = c_metaDeclare(c_metaObject(base),"c_value",M_UNION);
    c_metaTypeInit(o,sizeof(struct c_value),C_ALIGNMENT(c_value));
    c_free(o);

    o = c_metaDeclare(c_metaObject(base),"c_annotation", M_CLASS);
    c_metaTypeInit(o,sizeof(struct c_annotation_s),C_ALIGNMENT_C_STRUCT(c_annotation));
    c_free(o);


    /* Now all meta meta references required for initialization are available.
     * The following statements will initialize all meta meta objects.
     * After initialization reflection will be operational.
     */

    /* Initialize abstract meta types */

#define _INITCLASS_(s,c,p) \
    c_metaObject(initClass(ResolveType(s,#c),ResolveType(s,#p)))

#define C_META_FINALIZE_(o) \
        c_type(o)->alignment = 0; c__metaFinalize(o,FALSE)

    o = _INITCLASS_(base,c_baseObject,NULL);
        type = ResolveType(base,"c_metaKind");
        C_META_ATTRIBUTE_(c_baseObject,o,kind,type);
        c_free(type);
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_operand,c_baseObject);
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_specifier,c_baseObject);
        C_META_ATTRIBUTE_(c_specifier,o,name,c_string_t(base));
        type = ResolveType(base,"c_type");
        C_META_ATTRIBUTE_(c_specifier,o,type,type);
        c_free(type);
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_metaObject,c_baseObject);
        C_META_ATTRIBUTE_(c_metaObject,o,name,c_string_t(base));
        C_META_ATTRIBUTE_(c_metaObject,o,definedIn,c_voidp_t(base));
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_property,c_metaObject);
        type = ResolveType(base,"c_type");
        C_META_ATTRIBUTE_(c_property,o,type,type);
        c_free(type);
        type = ResolveType(base,"c_address");
        C_META_ATTRIBUTE_(c_property,o,offset,type);
        c_free(type);
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_type,c_metaObject);
        C_META_ATTRIBUTE_(c_type,o,base,c_voidp_t(base));
        type = ResolveType(base,"c_address");
        C_META_ATTRIBUTE_(c_type,o,alignment,type);
        c_free(type);
        C_META_ATTRIBUTE_(c_type,o,objectCount,c_long_t(base));
        type = ResolveType(base,"c_address");
        C_META_ATTRIBUTE_(c_type,o,size,type);
        c_free(type);
        C_META_FINALIZE_(o);
        c_free(o);

    /* Initialize meta types */
    o = _INITCLASS_(base,c_literal,c_operand);
        type = ResolveType(base,"c_value");
        C_META_ATTRIBUTE_(c_literal,o,value,type);
        c_free(type);
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_constOperand,c_operand);
        type = ResolveType(base,"c_constant");
        C_META_ATTRIBUTE_(c_constOperand,o,constant,type);
        c_free(type);
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_expression,c_operand);
        type = ResolveType(base,"c_exprKind");
        C_META_ATTRIBUTE_(c_expression,o,kind,type);
        c_free(type);
        type = c_type(c_metaDefine(c_metaObject(base),M_COLLECTION));
            c_metaObject(type)->name = c_stringNew(base,"ARRAY<c_operand>");
            c_collectionTypeKind(type) = OSPL_C_ARRAY;
            c_collectionTypeMaxSize(type) = 0;
            c_collectionTypeSubType(type) = ResolveType(base,"c_operand");
            c_metaFinalize(c_metaObject(type));
        C_META_ATTRIBUTE_(c_expression,o,operands,type);
        c_free(type);
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_parameter,c_specifier);
        type = ResolveType(base,"c_direction");
        C_META_ATTRIBUTE_(c_parameter,o,mode,type);
        c_free(type);
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_member,c_specifier);
        type = ResolveType(base,"c_address");
        C_META_ATTRIBUTE_(c_member,o,offset,type);
        c_free(type);
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_unionCase,c_specifier);
        type = c_type(c_metaDefine(c_metaObject(base),M_COLLECTION));
            c_metaObject(type)->name = c_stringNew(base,"ARRAY<c_literal>");
            c_collectionTypeKind(type) = OSPL_C_ARRAY;
            c_collectionTypeMaxSize(type) = 0;
            c_collectionTypeSubType(type) = ResolveType(base,"c_literal");
            c_metaFinalize(c_metaObject(type));
        C_META_ATTRIBUTE_(c_unionCase,o,labels,type);
        c_free(type);
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_attribute,c_property);
        C_META_ATTRIBUTE_(c_attribute,o,isReadOnly,c_bool_t(base));
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_relation,c_property);
        C_META_ATTRIBUTE_(c_relation,o,inverse,c_string_t(base));
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_module,c_metaObject);
        type = ResolveType(base,"c_mutex");
        C_META_ATTRIBUTE_(c_module,o,mtx,type);
        c_free(type);
        type = ResolveType(base,"c_scope");
        C_META_ATTRIBUTE_(c_module,o,scope,type);
        c_free(type);
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_constant,c_metaObject);
        type = ResolveType(base,"c_operand");
        C_META_ATTRIBUTE_(c_constant,o,operand,type);
        c_free(type);
        type = ResolveType(base,"c_type");
        C_META_ATTRIBUTE_(c_constant,o,type,type);
        c_free(type);
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_operation,c_metaObject);
        type = ResolveType(base,"c_type");
        C_META_ATTRIBUTE_(c_operation,o,result,type);
        c_free(type);
        type = c_type(c_metaDefine(c_metaObject(base),M_COLLECTION));
            c_metaObject(type)->name = c_stringNew(base,"ARRAY<c_parameter>");
            c_collectionTypeKind(type) = OSPL_C_ARRAY;
            c_collectionTypeMaxSize(type) = 0;
            c_collectionTypeSubType(type) = ResolveType(base,"c_parameter");
            c_metaFinalize(c_metaObject(type));
        C_META_ATTRIBUTE_(c_operation,o,parameters,type);
        c_free(type);
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_typeDef,c_type);
        type = ResolveType(base,"c_type");
        C_META_ATTRIBUTE_(c_typeDef,o,alias,type);
        c_free(type);
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_collectionType,c_type);
        type = ResolveType(base,"c_collKind");
        C_META_ATTRIBUTE_(c_collectionType,o,kind,type);
        c_free(type);
        C_META_ATTRIBUTE_(c_collectionType,o,maxSize,c_ulong_t(base));
        type = ResolveType(base,"c_type");
        C_META_ATTRIBUTE_(c_collectionType,o,subType,type);
        c_free(type);
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_primitive,c_type);
        type = ResolveType(base,"c_primKind");
        C_META_ATTRIBUTE_(c_primitive,o,kind,type);
        c_free(type);
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_enumeration,c_type);
        type = c_type(c_metaDefine(c_metaObject(base),M_COLLECTION));
            c_metaObject(type)->name = c_stringNew(base,"ARRAY<c_constant>");
            c_collectionTypeKind(type) = OSPL_C_ARRAY;
            c_collectionTypeMaxSize(type) = 0;
            c_collectionTypeSubType(type) = ResolveType(base,"c_constant");
            c_metaFinalize(c_metaObject(type));
        C_META_ATTRIBUTE_(c_enumeration,o,elements,type);
        c_free(type);
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_union,c_type);
        type = c_type(c_metaDefine(c_metaObject(base),M_COLLECTION));
            c_metaObject(type)->name = c_stringNew(base,"ARRAY<c_unionCase>");
            c_collectionTypeKind(type) = OSPL_C_ARRAY;
            c_collectionTypeMaxSize(type) = 0;
            c_collectionTypeSubType(type) = ResolveType(base,"c_unionCase");
            c_metaFinalize(c_metaObject(type));
        C_META_ATTRIBUTE_(c_union,o,cases,type);
        c_free(type);
        type = c_type(c_metaDefine(c_metaObject(base),M_COLLECTION));
            c_metaObject(type)->name = c_stringNew(base,"ARRAY<c_unionCase>");
            c_collectionTypeKind(type) = OSPL_C_ARRAY;
            c_collectionTypeMaxSize(type) = 0;
            c_collectionTypeSubType(type) = ResolveType(base,"c_unionCase");
            c_metaFinalize(c_metaObject(type));
        C_META_ATTRIBUTE_(c_union,o,references,type);
        c_free(type);
        type = ResolveType(base,"c_scope");
        C_META_ATTRIBUTE_(c_union,o,scope,type);
        c_free(type);
        type = ResolveType(base,"c_type");
        C_META_ATTRIBUTE_(c_union,o,switchType,type);
        c_free(type);
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_structure,c_type);
        type = c_type(c_metaDefine(c_metaObject(base),M_COLLECTION));
            c_metaObject(type)->name = c_stringNew(base,"ARRAY<c_member>");
            c_collectionTypeKind(type) = OSPL_C_ARRAY;
            c_collectionTypeMaxSize(type) = 0;
            c_collectionTypeSubType(type) = ResolveType(base,"c_member");
            c_metaFinalize(c_metaObject(type));
        C_META_ATTRIBUTE_(c_structure,o,members,type);
        c_free(type);
        type = c_type(c_metaDefine(c_metaObject(base),M_COLLECTION));
            c_metaObject(type)->name = c_stringNew(base,"ARRAY<c_member>");
            c_collectionTypeKind(type) = OSPL_C_ARRAY;
            c_collectionTypeMaxSize(type) = 0;
            c_collectionTypeSubType(type) = ResolveType(base,"c_member");
            c_metaFinalize(c_metaObject(type));
        C_META_ATTRIBUTE_(c_structure,o,references,type);
        c_free(type);
        type = ResolveType(base,"c_scope");
        C_META_ATTRIBUTE_(c_structure,o,scope,type);
        c_free(type);
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_exception,c_structure);
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_interface,c_type);
        C_META_ATTRIBUTE_(c_interface,o,abstract,c_bool_t(base));
        type = c_type(c_metaDefine(c_metaObject(base),M_COLLECTION));
            c_metaObject(type)->name = c_stringNew(base,"ARRAY<c_interface>");
            c_collectionTypeKind(type) = OSPL_C_ARRAY;
            c_collectionTypeMaxSize(type) = 0;
            c_collectionTypeSubType(type) = ResolveType(base,"c_interface");
            c_metaFinalize(c_metaObject(type));
        C_META_ATTRIBUTE_(c_interface,o,inherits,type);
        c_free(type);
        type = c_type(c_metaDefine(c_metaObject(base),M_COLLECTION));
            c_metaObject(type)->name = c_stringNew(base,"ARRAY<c_property>");
            c_collectionTypeKind(type) = OSPL_C_ARRAY;
            c_collectionTypeMaxSize(type) = 0;
            c_collectionTypeSubType(type) = ResolveType(base,"c_property");
            c_metaFinalize(c_metaObject(type));
        C_META_ATTRIBUTE_(c_interface,o,references,type);
        c_free(type);
        type = ResolveType(base,"c_scope");
        C_META_ATTRIBUTE_(c_interface,o,scope,type);
        c_free(type);
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_class,c_interface);
        type = ResolveType(base,"c_interface");
        C_META_ATTRIBUTE_(c_class,o,extends,type);
        c_free(type);
        type = c_type(c_metaDefine(c_metaObject(base),M_COLLECTION));
            c_metaObject(type)->name = c_stringNew(base,"ARRAY<c_string>");
            c_collectionTypeKind(type) = OSPL_C_ARRAY;
            c_collectionTypeMaxSize(type) = 0;
            c_collectionTypeSubType(type) = ResolveType(base,"c_string");
            c_metaFinalize(c_metaObject(type));
        C_META_ATTRIBUTE_(c_class,o,keys,type);
        c_free(type);
        C_META_FINALIZE_(o);
        c_free(o);

    o = _INITCLASS_(base,c_annotation,c_interface);
        type = ResolveType(base,"c_annotation");
        C_META_ATTRIBUTE_(c_annotation,o,extends,type);
        c_free(type);
        C_META_FINALIZE_(o);
        c_free(o);


/*---------------------------------------------------------------*/

    o = _INITCLASS_(base,c_base,c_module);

        C_META_FINALIZE_(o);
        c_free(o);

/*---------------------------------------------------------------*/

#undef _INITCLASS_

#define _SWITCH_TYPE_ c_enumeration(c_union(o)->switchType)

    o = c_metaDeclare(c_metaObject(base),"c_value",M_UNION);
        c_union(o)->switchType = ResolveType(base,"c_valueKind");
        type = ResolveType(base,"c_literal");
        c_union(o)->cases = c_arrayNew(type,18);
        c_free(type);
        c_free(c_union(o)->scope);
        c_union(o)->scope = NULL;

        caseNumber = 0;
        /* c_unionCaseNew transfers refCount of type and
         * c_enumValue(_SWITCH_TYPE_,"V_BOOLEAN")
         */
        labels = c_iterNew(c_enumValue(_SWITCH_TYPE_,"V_ADDRESS"));
        c_union(o)->cases[caseNumber++] = c_unionCaseNew(c_metaObject(base),
                                              "Address",c_address_t(base),labels);
        c_iterFree(labels);

        labels = c_iterNew(c_enumValue(_SWITCH_TYPE_,"V_BOOLEAN"));
        c_union(o)->cases[caseNumber++] = c_unionCaseNew(c_metaObject(base),
                                              "Boolean",c_bool_t(base),labels);
        c_iterFree(labels);

        labels = c_iterNew(c_enumValue(_SWITCH_TYPE_,"V_OCTET"));
        c_union(o)->cases[caseNumber++] = c_unionCaseNew(c_metaObject(base),
                                              "Octet",c_octet_t(base),labels);
        c_iterFree(labels);

        labels = c_iterNew(c_enumValue(_SWITCH_TYPE_,"V_SHORT"));
        c_union(o)->cases[caseNumber++] = c_unionCaseNew(c_metaObject(base),
                                              "Short",c_short_t(base),labels);
        c_iterFree(labels);

        labels = c_iterNew(c_enumValue(_SWITCH_TYPE_,"V_LONG"));
        c_union(o)->cases[caseNumber++] = c_unionCaseNew(c_metaObject(base),
                                              "Long",c_long_t(base),labels);
        c_iterFree(labels);

        labels = c_iterNew(c_enumValue(_SWITCH_TYPE_,"V_LONGLONG"));
        c_union(o)->cases[caseNumber++] = c_unionCaseNew(c_metaObject(base),
                                              "LongLong",c_longlong_t(base),labels);
        c_iterFree(labels);

        labels = c_iterNew(c_enumValue(_SWITCH_TYPE_,"V_USHORT"));
        c_union(o)->cases[caseNumber++] = c_unionCaseNew(c_metaObject(base),
                                              "UShort",c_ushort_t(base),labels);
        c_iterFree(labels);

        labels = c_iterNew(c_enumValue(_SWITCH_TYPE_,"V_ULONG"));
        c_union(o)->cases[caseNumber++] = c_unionCaseNew(c_metaObject(base),
                                              "ULong",c_ulong_t(base),labels);
        c_iterFree(labels);

        labels = c_iterNew(c_enumValue(_SWITCH_TYPE_,"V_ULONGLONG"));
        c_union(o)->cases[caseNumber++] = c_unionCaseNew(c_metaObject(base),
                                              "ULongLong",c_ulonglong_t(base),labels);
        c_iterFree(labels);

        labels = c_iterNew(c_enumValue(_SWITCH_TYPE_,"V_FLOAT"));
        c_union(o)->cases[caseNumber++] = c_unionCaseNew(c_metaObject(base),
                                              "Float",c_float_t(base),labels);
        c_iterFree(labels);

        labels = c_iterNew(c_enumValue(_SWITCH_TYPE_,"V_DOUBLE"));
        c_union(o)->cases[caseNumber++] = c_unionCaseNew(c_metaObject(base),
                                              "Double",c_double_t(base),labels);
        c_iterFree(labels);

        labels = c_iterNew(c_enumValue(_SWITCH_TYPE_,"V_CHAR"));
        c_union(o)->cases[caseNumber++] = c_unionCaseNew(c_metaObject(base),
                                               "Char",c_char_t(base),labels);
        c_iterFree(labels);

        labels = c_iterNew(c_enumValue(_SWITCH_TYPE_,"V_STRING"));
        c_union(o)->cases[caseNumber++] = c_unionCaseNew(c_metaObject(base),
                                               "String",c_string_t(base),labels);
        c_iterFree(labels);

        labels = c_iterNew(c_enumValue(_SWITCH_TYPE_,"V_WCHAR"));
        c_union(o)->cases[caseNumber++] = c_unionCaseNew(c_metaObject(base),
                                               "WChar",c_wchar_t(base),labels);
        c_iterFree(labels);

        labels = c_iterNew(c_enumValue(_SWITCH_TYPE_,"V_WSTRING"));
        c_union(o)->cases[caseNumber++] = c_unionCaseNew(c_metaObject(base),
                                               "WString",c_wstring_t(base),labels);
        c_iterFree(labels);

        labels = c_iterNew(c_enumValue(_SWITCH_TYPE_,"V_FIXED"));
        c_union(o)->cases[caseNumber++] = c_unionCaseNew(c_metaObject(base),
                                               "Fixed",c_string_t(base),labels);
        c_iterFree(labels);

        labels = c_iterNew(c_enumValue(_SWITCH_TYPE_,"V_OBJECT"));
        c_union(o)->cases[caseNumber++] = c_unionCaseNew(c_metaObject(base),
                                               "Object",c_object_t(base),labels);
        c_iterFree(labels);

        labels = c_iterNew(c_enumValue(_SWITCH_TYPE_,"V_VOIDP"));
        c_union(o)->cases[caseNumber++] = c_unionCaseNew(c_metaObject(base),
                                               "Voidp",c_voidp_t(base),labels);
        c_iterFree(labels);
        c_metaFinalize(o);
        c_free(o);

#undef _SWITCH_TYPE_

    /* ::<_ospl_internal> */
    _intern = c_metaDeclare(c_metaObject(base), "_ospl_internal", M_MODULE);

    /* Declare builtin-annotations
     *
     * Note: these builtin annotation objects are not yet finalized. There is currently
     * no need because creation of annotations won't be supported until version >=7.
     */

    /* ::ID */
    o = c_metaDefine(c_metaObject(_intern), M_ANNOTATION);
    initAnnotation(o);
    found = c_metaDeclare(o, "value", M_ATTRIBUTE);
    c_property(found)->type = c_ulong_t(base);
    c_free(found);
    c_metaBind(c_metaObject(_intern), "ID", o);
    c_free(o);

    /* ::Optional */
    o = c_metaDefine(c_metaObject(_intern), M_ANNOTATION);
    initAnnotation(o);
    found = c_metaDeclare(o, "value", M_ATTRIBUTE);
    c_property(found)->type = c_bool_t(base);
    c_free(found);
    c_metaBind(c_metaObject(_intern), "optional", o);
    c_free(o);

    /* ::Key */
    o = c_metaDefine(c_metaObject(_intern), M_ANNOTATION);
    initAnnotation(o);
    found = c_metaDeclare(o, "value", M_ATTRIBUTE);
    c_property(found)->type = c_bool_t(base);
    c_free(found);
    c_metaBind(c_metaObject(_intern), "Key", o);
    c_free(o);

    /* ::Shared */
    o = c_metaDefine(c_metaObject(_intern), M_ANNOTATION);
    initAnnotation(o);
    found = c_metaDeclare(o, "value", M_ATTRIBUTE);
    c_property(found)->type = c_bool_t(base);
    c_free(found);
    c_metaBind(c_metaObject(_intern), "Shared", o);
    c_free(o);

    /* ::BitBound */
    o = c_metaDefine(c_metaObject(_intern), M_ANNOTATION);
    initAnnotation(o);
    found = c_metaDeclare(o, "value", M_ATTRIBUTE);
    c_property(found)->type = c_ushort_t(base);
    c_free(found);
    c_metaBind(c_metaObject(_intern), "BitBound", o);
    c_free(o);

    /* ::Value */
    o = c_metaDefine(c_metaObject(_intern), M_ANNOTATION);
    initAnnotation(o);
    found = c_metaDeclare(o, "value", M_ATTRIBUTE);
    c_property(found)->type = c_ulong_t(base);
    c_free(found);
    c_metaBind(c_metaObject(_intern), "Value", o);
    c_free(o);

    /* ::BitSet */
    o = c_metaDefine(c_metaObject(_intern), M_ANNOTATION);
    initAnnotation(o);
    c_metaBind(c_metaObject(_intern), "BitSet", o);
    c_free(o);

    /* ::Nested */
    o = c_metaDefine(c_metaObject(_intern), M_ANNOTATION);
    initAnnotation(o);
    found = c_metaDeclare(o, "value", M_ATTRIBUTE);
    c_property(found)->type = c_bool_t(base);
    c_free(found);
    c_metaBind(c_metaObject(_intern), "Nested", o);
    c_free(o);

    temp = c_metaDefine(c_metaObject(base),M_ENUMERATION);
    c_enumeration(temp)->elements = c_arrayNew(c_constant_t(base),3);
    c_enumeration(temp)->elements[0] = c_metaDeclareEnumElement(c_metaObject(base),"FINAL_EXTENSIBILITY");
    c_enumeration(temp)->elements[1] = c_metaDeclareEnumElement(c_metaObject(base),"EXTENSIBLE_EXTENSIBILITY");
    c_enumeration(temp)->elements[2] = c_metaDeclareEnumElement(c_metaObject(base),"MUTABLE_EXTENSIBILITY");
    c_metaFinalize(temp);
    c_metaBind(c_metaObject(base), "ExtensibilityKind", temp);

    /* ::Extensibility */
    o = c_metaDefine(c_metaObject(_intern), M_ANNOTATION);
    initAnnotation(o);
    found = c_metaDeclare(o, "value", M_ATTRIBUTE);
    /* Transfer refCount of temp here, so no c_free on it later. */
    c_property(found)->type = c_type(temp);
    c_free(found);
    c_metaBind(c_metaObject(_intern), "Extensibility", o);
    c_free(o);

    /* ::MustUnderstand */
    o = c_metaDefine(c_metaObject(_intern), M_ANNOTATION);
    initAnnotation(o);
    found = c_metaDeclare(o, "value", M_ATTRIBUTE);
    c_property(found)->type = c_bool_t(base);
    c_free(found);
    c_metaBind(c_metaObject(_intern), "MustUnderstand", o);
    c_free(o);

    /* ::Verbatim */
    o = c_metaDefine(c_metaObject(_intern), M_ANNOTATION);
    initAnnotation(o);
    c_metaBind(c_metaObject(_intern), "Verbatim", o);

    found = c_metaObject(c_metaDefine(c_metaObject(base), M_COLLECTION));
    c_collectionType(found)->kind = OSPL_C_STRING;
    c_collectionType(found)->subType = c_char_t(base);
    c_collectionType(found)->maxSize = 32;
    c_metaObject(found)->definedIn = c_metaObject(base);
    c_metaFinalize(found);
    temp = c_metaBind(c_metaObject(base), "C_STRING<32>", found);
    c_free(found);

    found = c_metaDeclare(o, "language", M_ATTRIBUTE);
    /* Transfer refCount of temp here, so no c_free on it later. */
    c_property(found)->type = c_type(temp);
    c_free(found);

    found = c_metaObject(c_metaDefine(c_metaObject(base), M_COLLECTION));
    c_collectionType(found)->kind = OSPL_C_STRING;
    c_collectionType(found)->subType = c_char_t(base);
    c_collectionType(found)->maxSize = 128;
    c_metaObject(found)->definedIn = c_metaObject(base);
    c_metaFinalize(found);
    temp = c_metaBind(c_metaObject(base), "C_STRING<128>", found);
    c_free(found);

    found = c_metaDeclare(o, "placement", M_ATTRIBUTE);
    /* Transfer refCount of temp here, so no c_free on it later. */
    c_property(found)->type = c_type(temp);
    c_free(found);

    found = c_metaDeclare(o, "text", M_ATTRIBUTE);
    c_property(found)->type = c_string_t(base);
    c_free(found);
    c_free(o);
    ////c_field.c
    ////
    c_fieldInit(base);
    ////
    c_querybaseInit(base);
}


c_object
c__newCommon (
    c_type type,
    c_bool check)
{
    c_header header;
    c_object o;
    size_t size;
    uint32_t traceType;

    assert(type);

    if (c_baseObjectKind (type) != M_COLLECTION) {
        size = type->size;
    } else {
#ifndef NDEBUG
        if (c_collectionTypeKind (type) == OSPL_C_ARRAY ||
            c_collectionTypeKind (type) == OSPL_C_SEQUENCE)
        {
            // OS_REPORT(OS_ERROR,
            //           "Database c_new",0,
            //           "c_new cannot create OSPL_C_ARRAY nor OSPL_C_SEQUENCE, "
            //           "use c_newArray or c_newSequence respectively");
            assert(0);
        }
#endif
        size = c_typeSize (type);
    }
    assert (size > 0);

    if (check) {
        header = (c_header) c_mmMallocThreshold (type->base->mm, MEMSIZE(size));
    } else {
        header = (c_header) c_mmMalloc (type->base->mm, MEMSIZE(size));
        if (!header) {
            abort();
        }
    }
    if (c_unlikely (header == NULL)) {
        return NULL;
    }

    traceType = (ddsrt_atomic_ld32(&c_header(type)->refCount) & REFCOUNT_FLAG_TRACETYPE);
    ddsrt_atomic_st32(&header->refCount, 1);
    if (traceType) {
        ddsrt_atomic_or32(&header->refCount, REFCOUNT_FLAG_TRACE);
        c_mmTrackObject (type->base->mm, header, C_MMTRACKOBJECT_CODE_MIN + 2);
    }
#if TYPE_REFC_COUNTS_OBJECTS
    header->type = c_keep(type);
#else
    header->type = type;
#endif
    if (type->base->maintainObjectCount) {
        ddsrt_atomic_inc32(&type->objectCount);
    }
#ifndef NDEBUG
    header->confidence = CONFIDENCE;
#ifdef OBJECT_WALK
    header->nextObject = NULL;
    c_header(type->base->lastObject)->nextObject = o;
    header->prevObject = type->base->lastObject;
    assert(type->base->firstObject != NULL);
    type->base->lastObject = o;
#endif
#endif

    o = c_oid (header);
    memset (o, 0, size);
#if CHECK_REF
    ACTUALTYPE(type,header->type);
    if (type && c_metaObject(type)->name) {
        if (strlen(c_metaObject(type)->name) == CHECK_REF_TYPE_LEN) {
            if (strncmp(c_metaObject(type)->name, CHECK_REF_TYPE, strlen(CHECK_REF_TYPE)) == 0) {
                UT_TRACE("\n\n============ New(%p) =============\n", o);
            }
        }
    }
#endif
    return o;
}

c_object
c_new (
    c_type type)
{
    return c__newCommon(type, FALSE);
}


size_t
c_typeSize(
    c_type type)
{
    c_type subType;
    size_t size;

    if (c_baseObjectKind(type) == M_COLLECTION) {
        switch(c_collectionTypeKind(type)) {
        case OSPL_C_ARRAY:
            subType = c_collectionTypeSubType(type);
            switch(c_baseObjectKind(subType)) {
            case M_INTERFACE:
            case M_CLASS:
            case M_ANNOTATION:
                size = c_collectionTypeMaxSize(type)*sizeof(void *);
            break;
            default:
                if (subType->size == 0) {
                    subType->size = sizeof(void *);
                }
                size = c_collectionTypeMaxSize(type)*subType->size;
            break;
            }
        break;
        case OSPL_C_LIST:
            size = c_listSize;
        break;
        case OSPL_C_BAG:
            size = c_bagSize;
        break;
        case OSPL_C_SET:
            size = c_setSize;
        break;
        case OSPL_C_DICTIONARY:
            size = c_tableSize;
        break;
        case OSPL_C_QUERY:
            size = c_querySize;
        break;
        case OSPL_C_SCOPE:
            size = C_SIZEOF(c_scope);
        break;
        case OSPL_C_STRING:
            size = sizeof(c_char *);
        break;
        case OSPL_C_SEQUENCE:
            size = sizeof(c_address);
        break;
        default:
            // OS_REPORT(OS_ERROR,
            //           "c_typeSize failed",0,
            //           "illegal type specified");
            assert(FALSE);
            size = ~(size_t)0;
        break;
        }
    } else {
        size = type->size;
    }

    return size;
}

c_string
c_stringNew_s(
    c_base base,
    const c_char *str)
{
    c_string s;
     size_t len;

     assert(base);

     if (str == NULL) {
         return NULL;
     }

     len = strlen(str) + 1;
     if((s = c_stringMalloc_s(base, len)) != NULL && len > 1){
         /* str is nul-terminated (since we could determine len), so memcpy str
          * including the nul-terminator.
          */
         memcpy(s, str, len);
     }

     /* c_wstringNew/Malloc always return a nul-terminated string */
     assert(!s || s[len - 1] == '\0');
     return s;
}

c_string
c_stringMalloc_s(
    c_base base,
    c_size length)
{
    assert(base);
    assert(length > 0);

    if (length == 1){
        /* Empty strings are interned. The only valid string with length 1 is the
         * 'empty' string, so in this case return a reference to the intern. This
         * saves precious resources in case of big amounts of empty strings, since
         * each empty string does not only contain the '\0' terminator but also
         * 36-40 bytes of header information.
         */
        assert(base->emptyString[0] == '\0');
        return c_keep(base->emptyString);
    }

    return c__stringMalloc(base, length, TRUE);
}

c_type
c_newMetaClass(
    c_base base,
    const c_char *name,
    size_t size,
    size_t alignment)
{
    c_metaObject o,found;

    assert(base != NULL);
    assert(name != NULL);
    assert(base->metaType[M_CLASS] != NULL);

    o = c_metaDefine(c_metaObject(base),M_CLASS);
    c_metaTypeInit(o,size,alignment);
    found = c_metaBind(c_metaObject(base),name,o);
    if (found != o) {
        // OS_REPORT(OS_ERROR,"c_newMetaClass failed",0,"%s already defined",name);
        assert(FALSE);
    }
    c_free(o);
    return c_type(found);
}

c_object
c_new_s(
    c_type type)
{
    return c__newCommon(type, TRUE);
}


c_type
c_resolve (
    c_base base,
    const c_char *typeName)
{
    c_metaObject o;

    assert(base->confidence == CONFIDENCE);

    o = c_metaResolve(c_metaObject(base),typeName);
    if (c_objectIsType(c_baseObject(o))) {
        return c_type(o);
    }
    c_free(o);
    return NULL;
}


c_object
c_newBaseArrayObject_s (
    c_collectionType arrayType,
    c_ulong size)
{
    return c__newBaseArrayObjectCommon(arrayType, size, TRUE);
}


c_bool
c_imageValue(
    const char *image,
    c_value *imgValue,
    c_type imgType)
{
    c_type t;
    char *endptr;

    assert(imgType);

    t = c_typeActualType(imgType);
    switch(c_baseObjectKind(t)) {
    case M_ENUMERATION:
    {
        c_literal l = c_enumValue(c_enumeration(imgType), image);
        if (l) {
            *imgValue = l->value;
            c_free(l);
        } else {
            imgValue->kind = V_UNDEFINED;
            // OS_REPORT(OS_ERROR,
            //              "c_typebase::c_imageValue",0,
            //              "expected legal enum label instead of \"%s\".",
            //              image);
        }
        break;
    }
    case M_COLLECTION:
        if (c_collectionTypeKind(t) == OSPL_C_STRING) {
            if(imgValue->is.String){
                c_free(imgValue->is.String);
            }
            imgValue->is.String = c_stringNew(c_getBase(imgType), image);
            imgValue->kind = V_STRING;
        }
        break;
    case M_PRIMITIVE:
        switch (c_primitiveKind(t)) {
        case P_BOOLEAN:
            ////
            if (os_strncasecmp(image,"TRUE",5) == 0) 
            {
                imgValue->kind = V_BOOLEAN;
                imgValue->is.Boolean = TRUE;
            } 
            ////
            else if (os_strncasecmp(image,"FALSE",6) == 0) 
            {
                imgValue->kind = V_BOOLEAN;
                imgValue->is.Boolean = FALSE;
            } 
            ////
            else 
            {
                imgValue->kind = V_UNDEFINED;
            }
            break;
        case P_LONGLONG:
        ////ddsrt_strtoll
            imgValue->is.LongLong = os_strtoll (image, &endptr, 0);
            if (*endptr == '\0') {
                imgValue->kind = V_LONGLONG;
            } else {
                imgValue->kind = V_UNDEFINED;
            }
            break;
        case P_ULONGLONG:
        ////os_strtoull
            imgValue->is.ULongLong = os_strtoull (image, &endptr, 0);
            if (*endptr == '\0') {
                imgValue->kind = V_ULONGLONG;
            } else {
                imgValue->kind = V_UNDEFINED;
            }
            break;
        case P_SHORT:
            if (sscanf(image,"%hd",&imgValue->is.Short)) {
                imgValue->kind = V_SHORT;
            } else {
                imgValue->kind = V_UNDEFINED;
            }
            break;
        case P_LONG:
            imgValue->is.Long = (c_long) strtol (image, &endptr, 0);
            if (*endptr == '\0') {
                imgValue->kind = V_LONG;
            } else {
                imgValue->kind = V_UNDEFINED;
            }
            break;
        case P_OCTET:
        {
            c_short s;

            if (sscanf(image,"%hd",&s)) {
                imgValue->kind = V_OCTET;
                imgValue->is.Octet = (c_octet)s;
            } else {
                imgValue->kind = V_UNDEFINED;
            }
            break;
        }
        case P_USHORT:
            if (sscanf(image,"%hu",&imgValue->is.UShort)) {
                imgValue->kind = V_USHORT;
            } else {
                imgValue->kind = V_UNDEFINED;
            }
            break;
        case P_ULONG:
            imgValue->is.ULong = (c_ulong) strtoul (image, &endptr, 0);
            if (*endptr == '\0') {
                imgValue->kind = V_ULONG;
            } else {
                imgValue->kind = V_UNDEFINED;
            }
            break;
        case P_CHAR:
            if (image != NULL) {
                imgValue->kind = V_CHAR;
                imgValue->is.Char = *image;
            } else {
                imgValue->kind = V_UNDEFINED;
            }
            break;
        case  P_FLOAT:
        ////ddsrt_strtof
            imgValue->is.Float = os_strtof (image, &endptr);
            if (*endptr == '\0') {
                imgValue->kind = V_FLOAT;
            } else {
                imgValue->kind = V_UNDEFINED;
            }
            break;
        case  P_DOUBLE:
        ////ddsrt_strtod
            imgValue->is.Double = os_strtod (image, &endptr);
            if (*endptr == '\0') {
                imgValue->kind = V_DOUBLE;
            } else {
                imgValue->kind = V_UNDEFINED;
            }
            break;
        default:
            assert(0);
        }
        break;
     default:
         assert(FALSE);
         break;
    }
    return (imgValue->kind != V_UNDEFINED);
}


c_object
initClass(
    c_object o,
    c_type baseClass)
{
    initInterface(o);
    c_class(o)->extends = c_class(baseClass);
    c_class(o)->keys = NULL;
    return o;
}

void
initInterface(
    c_object o)
{
    /* The vast majority of classes that get defined during initialisation have the scope already
     * allocated by c_metaDefine, but not all of them.
     */
    if(c_interface(o)->scope == NULL) {
        c_interface(o)->scope = (c_scope)c_scopeNew(c__getBase(o));
    }
    c_interface(o)->abstract = FALSE;
    c_interface(o)->inherits = NULL;
    c_interface(o)->references = NULL;
}


c_ulong
c_sequenceSize(
    c_sequence _this)
{
    if (_this) {
        assert(c_header(_this)->confidence == CONFIDENCE);
        assert(c_baseObjectKind(c_header(_this)->type) == M_COLLECTION &&
                c_collectionTypeKind(c_header(_this)->type) == OSPL_C_SEQUENCE);
        return (c_ulong)(c_arrayHeader(_this)->size);
    } else {
        return 0;
    }
}


c_object
initAnnotation(
    c_object o)
{
    initInterface(o);
    return o;
}