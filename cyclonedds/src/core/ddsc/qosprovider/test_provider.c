
#include "test_provider.h"
#include "c_mmbase.h"
#include "fun_base.h"
#include "c_base.h"
#include "cfg_parser.h"
#include "qp_defaultQos.h"

typedef struct {
    const char *                name;
    const char *                value;
} ConstantValuePair;

static const ConstantValuePair ApiConstants[] = {
        { "LENGTH_UNLIMITED",       "-1"         },
        { "DURATION_INFINITE_SEC",  "2147483647" },
        { "DURATION_INFINITE_NSEC", "2147483647" },
        { "DURATION_ZERO_SEC",      "0" },
        { "DURATION_ZERO_NSEC",     "0" }
};

static void
unloadEntityQosAttributes(
        qp_entityAttr attr)
{
    assert(attr);

    c_free(attr->defaultQosTmplt);
    c_free(attr->qosSampleType);
    ut_tableFree(attr->qosTable);
}

static void
unloadQosProviderQosAttributes(
        cmn_qosProvider _this)
{
    unloadEntityQosAttributes(&_this->drQosAttr);
    unloadEntityQosAttributes(&_this->dwQosAttr);
    unloadEntityQosAttributes(&_this->subQosAttr);
    unloadEntityQosAttributes(&_this->pubQosAttr);
    unloadEntityQosAttributes(&_this->tQosAttr);
    unloadEntityQosAttributes(&_this->dpQosAttr);
}


void
cmn_qosProviderFree(
    cmn_qosProvider _this)
{
    if (_this) {
        unloadQosProviderQosAttributes(_this);
        if (_this->baseAddr) {
            c_destroy(_this->baseAddr);
        }
        if (_this->rootElement) {
            cf_elementFree(_this->rootElement);
        }
        ddsrt_free(_this->defaultProfile);
        ddsrt_free (_this);
    }
}

cmn_qpResult
checkQosProviderAttrIsSane (
    const C_STRUCT(cmn_qosProviderInputAttr) *attr)
{
    if(!attr) goto err_attr;
    if(!attr->participantQos.copyOut) goto err_attr;
    if(!attr->topicQos.copyOut) goto err_attr;
    if(!attr->publisherQos.copyOut) goto err_attr;
    if(!attr->dataWriterQos.copyOut) goto err_attr;
    if(!attr->subscriberQos.copyOut) goto err_attr;
    if(!attr->dataReaderQos.copyOut) goto err_attr;

    return QP_RESULT_OK;
err_attr:
    return QP_RESULT_ILL_PARAM;
}









cmn_qosProvider
cmn_qosProviderNew(
    const c_char *uri,
    const c_char *profile,
    const C_STRUCT(cmn_qosProviderInputAttr) *attr)
{
    cfgprs_status s;
    cmn_qosProvider _this;
    const c_char *normalizedProfile = "";

    if(uri == NULL){
        //OS_REPORT(OS_ERROR, "cmn_qosProviderNew", 3, "Illegal parameter; parameter 'uri' may not be NULL.");
        goto err_illegalParameter;
    }
    if(checkQosProviderAttrIsSane (attr) != QP_RESULT_OK){
        //OS_REPORT(OS_ERROR, "cmn_qosProviderNew", 3, "Illegal parameter; parameter 'attr' may not be NULL and all fields must be initialized.");
        goto err_illegalParameter;
    }

    /* First make sure the user layer is initialized to allow Thread-specific
     * memory for error reporting. */
    _this = ddsrt_malloc (C_SIZEOF(cmn_qosProvider));
    memset(_this, 0, C_SIZEOF(cmn_qosProvider));

    if(profile){
        normalizedProfile = strncmp("::", profile, strlen("::")) == 0 ? profile + strlen("::") : profile;
    }
    _this->defaultProfile = ddsrt_strdup(normalizedProfile);
    if((s = cfg_parse_ospl(uri, &_this->rootElement)) != CFGPRS_OK){
        assert((s == CFGPRS_NO_INPUT) || (s == CFGPRS_ERROR));
        goto err_cfg_parse;
    }

    /* Create a heap database */    
    if((_this->baseAddr = c_create("QOSProvider", NULL, QOS_DATABASE_SIZE, 0)) == NULL){

        //(OS_ERROR, "cmn_qosProviderNew", 1, "Out of memory. Failed to allocate heap database.");
        goto err_c_create;
    }

    if (loadQosProviderQosAttributes(_this, attr) != QP_RESULT_OK){
        //OS_REPORT(OS_ERROR, "cmn_qosProviderNew", 1, "Out of memory. Failed to load QosProvider QosAttributes in heap database.");
        goto err_loadAttributes;
    }

    /* Parse the tree */
    {
        C_STRUCT(qp_parseContext) ctx;

        ctx.qosProvider = _this;
        ctx.currentField = NULL;
        ctx.currentFieldType = NULL;
        ctx.qosSample = NULL;
        ctx.scope = QP_SCOPE_NONE;
        ctx.name = NULL;
        ctx.qosTable = NULL;
        if (processElement(_this->rootElement, ctx) != QP_RESULT_OK) {
            /* ErrorReport already generated during recursive parse tree walk. */
            goto err_process;
        }
    }

    return _this;

/* Error handling */
err_process:
    unloadQosProviderQosAttributes(_this);
err_loadAttributes:
    c_destroy(_this->baseAddr);
err_c_create:
    if (_this->rootElement) {
        cf_elementFree(_this->rootElement);
    }
err_cfg_parse:
    ddsrt_free(_this->defaultProfile);
    ddsrt_free(_this);
    /* No undo for u_userInitialise */
err_userInitialize:
err_illegalParameter:
    return NULL;
}



cmn_qpResult
processElement(
        cf_element element,
        C_STRUCT(qp_parseContext) ctx)
{
    cmn_qpResult result = QP_RESULT_OK;
    const c_char *name = cf_nodeGetName((cf_node)element);
    assert(name);

    assert(ctx.qosProvider->defaultProfile);

    /* Enforce the opening <dds> tag. */
    switch(ctx.scope)
    {
    case QP_SCOPE_NONE:
        if (strcmp(name, DDS_TAG) == 0) {
            ctx.scope = QP_SCOPE_DDS;
            QP_TRACE(printf("%*sBEGIN " DDS_TAG "\n", level++ * 2, ""));
            result = processContainedElements(cf_elementGetChilds(element), ctx);
            QP_TRACE(printf("%*sEND " DDS_TAG "\n", --level * 2, ""));
        } else {
            QP_TRACE(printf("ERROR: Unrecognized top-level element ('%s'), expected top-level element '%s'\n", name, DDS_TAG));
            // OS_REPORT(
            //         OS_ERROR,
            //         "cmn_qosProvider::processElement",
            //         3,
            //         "Unrecognized top-level element (\"%s\"), expected top-level element \"%s\"...",
            //         name,
            //         DDS_TAG);
            return QP_RESULT_UNEXPECTED_ELEMENT;
        }
        break;
    case QP_SCOPE_DDS:
        if (strcmp(name, PROFILE_TAG) == 0) {
            ctx.scope = QP_SCOPE_PROFILE;
            /* Fetch the (required according to Annex C of the
             * 'DDS for Lightweight CCM, v1.1' specification) name of the
             * current profile and store it in the current ctx. */
            if((result = processContainedAttributes(cf_elementGetAttributes(element), &ctx)) != QP_RESULT_OK){
                return result;
            }
            if(!ctx.name){
                QP_TRACE(printf("ERROR: Element <" PROFILE_TAG "> has no 'name' attribute, which is mandatory.\n"));
                // OS_REPORT(
                //         OS_ERROR,
                //         "cmn_qosProvider::processElement",
                //         3,
                //         "Element <" PROFILE_TAG "> has no 'name' attribute, which is mandatory...");
                return QP_RESULT_PARSE_ERROR;
            }
            QP_TRACE(printf("%*sBEGIN " PROFILE_TAG " '%s'\n", level++ * 2, "", ctx.name));
            result = processContainedElements(cf_elementGetChilds(element), ctx);
            QP_TRACE(printf("%*sEND " PROFILE_TAG " '%s'\n", --level * 2, "", ctx.name));
            break; /* Only break when new scope has been determined. */
        }
        /* Fall-through intentional !! */
    case QP_SCOPE_PROFILE:
        if (strcmp(name, DPQOS_TAG) == 0) {
            result = prepareQosSample(DPQOS_TAG, &ctx.qosProvider->dpQosAttr, &ctx);
        } else if (strcmp(name, TQOS_TAG) == 0) {
            result = prepareQosSample(TQOS_TAG, &ctx.qosProvider->tQosAttr, &ctx);
        } else if (strcmp(name, PUBQOS_TAG) == 0) {
            result = prepareQosSample(PUBQOS_TAG, &ctx.qosProvider->pubQosAttr, &ctx);
        } else if (strcmp(name, SUBQOS_TAG) == 0) {
            result = prepareQosSample(SUBQOS_TAG, &ctx.qosProvider->subQosAttr, &ctx);
        } else if (strcmp(name, DWQOS_TAG) == 0) {
            result = prepareQosSample(DWQOS_TAG, &ctx.qosProvider->dwQosAttr, &ctx);
        } else if (strcmp(name, DRQOS_TAG) == 0) {
            result = prepareQosSample(DRQOS_TAG, &ctx.qosProvider->drQosAttr, &ctx);
        } else {
            return QP_RESULT_UNEXPECTED_ELEMENT;
        }

        if(result == QP_RESULT_OK){
            const char * profileName = ctx.name;
            ctx.name = NULL;

            /* Fetch the (required according to Annex C of the
             * 'DDS for Lightweight CCM, v1.1' specification, but optional
             * according to the rest of the document) name of the
             * current qos and store it in the current ctx.
             * In this case we ignore the schema and follow the rest of the
             * specification (including Annex D). */
            if((result = processContainedAttributes(cf_elementGetAttributes(element), &ctx)) != QP_RESULT_OK){
                return result;
            }
            QP_TRACE(printf("%*sBEGIN %s (profile: '%s', name: '%s')\n",
                    level++ * 2, "",
                    name,
                    profileName ? profileName : "(null)",
                    ctx.name ? ctx.name : "(null)"));
            if((result = processContainedElements(cf_elementGetChilds(element), ctx)) != QP_RESULT_OK){
                return result;
            }

            {
                char * qosName;
                size_t len = 0;

                if(profileName){
                    len += strlen(profileName);
                } else {
                    len += strlen(ctx.qosProvider->defaultProfile);
                }
                if(ctx.name) {
                    len += strlen(ctx.name);
                    len += 2;
                }

                qosName = ddsrt_malloc(len + 1);
                if(profileName){
                    sprintf(qosName, "%s%s%s", profileName, ctx.name ? "::" : "", ctx.name ? ctx.name : "");
                } else {
                    if(*ctx.qosProvider->defaultProfile == '\0'){
                        sprintf(qosName, "%s", ctx.name ? ctx.name : "");
                    } else {
                        sprintf(qosName, "%s%s%s", ctx.qosProvider->defaultProfile, ctx.name ? "::" : "", ctx.name ? ctx.name : "");
                    }
                }

                if((ut_tableInsert(ctx.qosTable, qosName, ctx.qosSample /* transfer ref */)) == 0){
                    // OS_REPORT(OS_INFO,
                    //         "cmn_qosProvider::processElement",
                    //         3,
                    //         "Identification for QoS '%s' is not unique...", /* TODO: report filename as well? */
                    //         qosName);
                    QP_TRACE(printf("%*sEND %s (%p), not stored under '%s'\n", --level * 2, "", name, (void *)ctx.qosSample, qosName));
                    c_free(ctx.qosSample);
                    ddsrt_free(qosName);
                } else {
                    QP_TRACE(printf("%*sEND %s (%p), stored under '%s'\n", --level * 2, "", name, (void *)ctx.qosSample, qosName));
                }
            }
            c_free(ctx.currentField);
            c_free(ctx.currentFieldType);
        }
        break;
    case QP_SCOPE_QOSPOLICY:
    {
        c_type at;
        c_field unscopedField = c_fieldNew(ctx.currentFieldType, name);

        if (unscopedField) {
            QP_TRACE(printf("%*sBEGIN field '%s'\n", level++ * 2, "", name));
            ctx.currentFieldType = c_fieldType(unscopedField);
            if (ctx.currentField) {
                ctx.currentField = c_fieldConcat(ctx.currentField, unscopedField);
                c_free(unscopedField);
            } else {
                ctx.currentField = unscopedField;
            }

            at =  c_typeActualType(ctx.currentFieldType);
            if(c_baseObjectKind(at) == M_COLLECTION && c_collectionTypeKind(at) == OSPL_C_SEQUENCE) {
                if(c_baseObjectKind(c_collectionTypeSubType(at)) == M_COLLECTION && c_collectionTypeKind(c_collectionTypeSubType(at)) == OSPL_C_STRING){
                QP_TRACE(printf("%*sBEGIN SEQUENCE<%s> field '%s'\n", level++ * 2, "", c_metaName(c_collectionTypeSubType(at)), name));
                    {
                        c_iter elements = cf_elementGetChilds(element);
                        cmn_qpResult result = QP_RESULT_OK;
                        cf_node node;
                        c_string *strings = NULL;
                        c_ulong used, len;

                        len = used = 0;

                        while ((node = (cf_node) c_iterTakeFirst(elements)) != NULL && result == QP_RESULT_OK)
                        {
                            cf_kind kind = cf_nodeKind(node);
                            if(kind == CF_ELEMENT && strcmp("element", cf_nodeGetName(node)) == 0){
                                c_iter stringElement = cf_elementGetChilds(cf_element(node));
                                cf_node stringNode;

                                while ((stringNode = (cf_node) c_iterTakeFirst(stringElement)) != NULL && result == QP_RESULT_OK) {
                                    if(cf_nodeKind(stringNode) == CF_DATA){
                                        if(used == len) {
                                            len += 16;
                                            strings = ddsrt_realloc(strings, sizeof(*strings) * len);
                                        }

                                        if(result == QP_RESULT_OK){
                                            strings[used++] = cf_dataValue((cf_data(stringNode))).is.String;
                                        }
                                    }
                                }
                                c_iterFree(stringElement);
                            } else {
                                /* TODO: report malformed XML */
                                result = QP_RESULT_PARSE_ERROR;
                            }
                        }
                        c_iterFree(elements);

                        if(used > 0) {
                            c_base base = c_getBase(at);
                            c_ulong i;
                            c_string *seq = (c_string*)c_newBaseArrayObject(c_collectionType(at), used);

                            for(i = 0; i < used; i++){
                                seq[i] = c_stringNew(base, strings[used - i - 1]);
                                QP_TRACE(printf("%*s'%s'\n", (level+1) * 2, "", seq[i]));
                            }
                            ddsrt_free(strings);

                            ////
                            c_fieldAssign(ctx.currentField, ctx.qosSample, c_objectValue(seq));
                            c_free(seq);
                        }
                    }
                    QP_TRACE(printf("%*sEND SEQUENCE<%s> field '%s'\n", --level * 2, "", c_metaName(c_collectionTypeSubType(at)), name));
                } else {
                    QP_TRACE(printf("%*sSKIPPING SEQUENCE<%s> field '%s'\n", level * 2, "", c_metaName(c_collectionTypeSubType(at)), name));
                }
            } else {
                if((result = processContainedElements(cf_elementGetChilds(element), ctx)) != QP_RESULT_OK){
                    return result;
                }
            }
            QP_TRACE(printf("%*sEND field '%s'\n", --level * 2, "", name));
            c_free(ctx.currentField);
        } else {
            if ((strcmp(c_fieldName(ctx.currentField), DWQOS_TAG) == 0) &&
                (strcmp(name, "durability_service") == 0)) {
                QP_TRACE(printf("SKIPPING: Unsupported element '%s' inside qosPolicy field '%s'\n", name, c_fieldName(ctx.currentField)));
                result = QP_RESULT_OK;
                // OS_REPORT(
                //     OS_INFO,
                //     "cmn_qosProvider::processElement",
                //     3,
                //     "Policy '%s' cannot be a member of " DWQOS_TAG " (it is applicable to " TQOS_TAG " only). OpenSpliceDDS will ignore this policy.",
                //     name);
            } else {
                QP_TRACE(printf("ERROR: Unrecognized element '%s' inside qosPolicy field '%s'\n", name, c_fieldName(ctx.currentField)));
                result = QP_RESULT_UNKNOWN_ELEMENT;
                // OS_REPORT(
                //     OS_ERROR,
                //     "cmn_qosProvider::processElement",
                //     3,
                //     "Unrecognized element (\"%s\") inside qosPolicy field \"%s\"...",
                //     name,
                //     c_fieldName(ctx.currentField));
            }
        }
        break;
    }
    default:
        assert(0);
    }

    return result;
}


cmn_qpResult
processContainedElements(
        c_iter elements,
        C_STRUCT(qp_parseContext) ctx)
{
    cmn_qpResult result = QP_RESULT_OK;
    cf_node node;

    while ((node = (cf_node) c_iterTakeFirst(elements)) != NULL && result == QP_RESULT_OK)
    {
        cf_kind kind = cf_nodeKind(node);
        switch(kind) {
        case CF_ELEMENT:
            result = processElement(cf_element(node), ctx);
            break;
        case CF_DATA:
            result = processElementData(cf_data(node), ctx);
            break;
        default:
            assert(0);
            break;
        }
    }
    /* Elements of iter elements don't have to be freed, so no worries that iter
     * is potentially not empty here. */
    c_iterFree(elements);
    return result;
}


cmn_qpResult
processElementData(
        cf_data data,
        C_STRUCT(qp_parseContext) ctx)
{
    cmn_qpResult result;
    char first;

    assert(data);

    first = cf_dataValue(data).is.String[0];

    if(strcmp(cf_node(data)->name, "#text") == 0 &&
       (first == '\0'  || first == '\n' || first == '<' || first == ' '))
    {
        /* Skip comments */
        result = QP_RESULT_OK;
    } else {
        c_value xmlValue = cf_dataValue(data);
        c_value qosValue = c_fieldValue(ctx.currentField, ctx.qosSample);
        const char *actualValue = substituteConstants(xmlValue.is.String);

        if (c_imageValue(actualValue, &qosValue, ctx.currentFieldType)) {
            ////
            c_fieldAssign(ctx.currentField, ctx.qosSample, qosValue);
            result = QP_RESULT_OK;
        } else {
            result = QP_RESULT_ILLEGAL_VALUE;
            // OS_REPORT(
            //         OS_ERROR,
            //         "cmn_qosProvider::processElementData",
            //         3,
            //         "Illegal value (\"%s\") for qosPolicy field \"%s\"...",
            //         xmlValue.is.String,
            //         c_fieldName(ctx.currentField));
        }
    }

    return result;
}

const char *
substituteConstants(
        const char *xmlValue)
{
    size_t i;

    assert(xmlValue);

    for (i = 0; i < sizeof(ApiConstants)/sizeof(ConstantValuePair); i++) {
        if (strcmp(xmlValue, ApiConstants[i].name) == 0) return ApiConstants[i].value;
    }
    return xmlValue;
}





/* loadQosSampleType: load qosSample type into the database. */
#undef c_metaObject /* Undef casting macro to avoid ambiguity with return type of function pointer. */
typedef c_metaObject (*qp_fnLoadQosSampleType)(c_base base);

typedef void(*qp_shallowClone)(void *from, void *to);

static cmn_qpResult
loadEntityQosAttributes(
        cmn_qosProvider _this,
        qp_fnLoadQosSampleType loadQosSample,
        const C_STRUCT(cmn_qosInputAttr) *inputAttr,
        qp_shallowClone shallowClone,
        c_voidp defaultQos,
        qp_entityAttr outputAttr)
{
    assert(_this);
    assert(loadQosSample);
    assert(inputAttr);
    assert(shallowClone);
    assert(defaultQos);
    assert(outputAttr);

    if((outputAttr->qosTable = (ut_table)ut_tableNew(cmn_qosCompareByName, NULL, cmn_qosTablefreeKey, NULL, cmn_qosTablefreeData, NULL)) == NULL){
        // OS_REPORT(OS_ERROR, "loadEntityQosAttributes", 1, "Out of memory. Failed to allocate storage for QoS.");
        goto err_table;
    }
    if((outputAttr->qosSampleType = c_type(loadQosSample(_this->baseAddr))) == NULL){
        // OS_REPORT(OS_ERROR, "loadEntityQosAttributes", 1, "Out of memory. Failed to allocate QoS sample-type.");
        goto err_sampleType;
    }
    if((outputAttr->defaultQosTmplt = c_new(outputAttr->qosSampleType)) == NULL){
        // OS_REPORT(OS_ERROR, "loadEntityQosAttributes", 1, "Out of memory. Failed to allocate QoS template.");
        goto err_qosTmplt;
    }

    shallowClone(defaultQos, outputAttr->defaultQosTmplt);

    outputAttr->copyOut = inputAttr->copyOut;
    return QP_RESULT_OK;

/* Error handling */
err_qosTmplt:
    c_free(outputAttr->qosSampleType);
err_sampleType:
    /* Guaranteed empty, so no free-functions needed */
    ut_tableFree(outputAttr->qosTable);
err_table:
    return QP_RESULT_OUT_OF_MEMORY;
}


cmn_qpResult
loadQosProviderQosAttributes(
        cmn_qosProvider _this,
        const C_STRUCT(cmn_qosProviderInputAttr) *attr)
{
    cmn_qpResult result;
    c_voidp qos;

    assert(_this);
    assert(checkQosProviderAttrIsSane (attr) == QP_RESULT_OK);

    qos = (c_voidp)&qp_NamedDomainParticipantQos_default; /* Discard const */
    
    result = loadEntityQosAttributes(_this, __DDS_NamedDomainParticipantQos__load, &attr->participantQos, qp_NamedDomainParticipantQos__shallowClone, qos, &_this->dpQosAttr);
    if (result == QP_RESULT_OK) {
        qos = (c_voidp)&qp_NamedTopicQos_default; /* Discard const */
        result = loadEntityQosAttributes(_this, __DDS_NamedTopicQos__load, &attr->topicQos, qp_NamedTopicQos__shallowClone, qos, &_this->tQosAttr);
    }
    if (result == QP_RESULT_OK) {
        qos = (c_voidp)&qp_NamedPublisherQos_default; /* Discard const */
        result = loadEntityQosAttributes(_this, __DDS_NamedPublisherQos__load, &attr->publisherQos, qp_NamedPublisherQos__shallowClone, qos, &_this->pubQosAttr);
    }
    if (result == QP_RESULT_OK) {
        qos = (c_voidp)&qp_NamedSubscriberQos_default; /* Discard const */
        result = loadEntityQosAttributes(_this, __DDS_NamedSubscriberQos__load, &attr->subscriberQos, qp_NamedSubscriberQos__shallowClone, qos, &_this->subQosAttr);
    }
    if (result == QP_RESULT_OK) {
        qos = (c_voidp)&qp_NamedDataWriterQos_default; /* Discard const */
        result = loadEntityQosAttributes(_this, __DDS_NamedDataWriterQos__load, &attr->dataWriterQos, qp_NamedDataWriterQos__shallowClone, qos, &_this->dwQosAttr);
    }
    if (result == QP_RESULT_OK) {
        qos = (c_voidp)&qp_NamedDataReaderQos_default; /* Discard const */
        result = loadEntityQosAttributes(_this, __DDS_NamedDataReaderQos__load, &attr->dataReaderQos, qp_NamedDataReaderQos__shallowClone, qos, &_this->drQosAttr);
    }
    if (result != QP_RESULT_OK) {
        unloadQosProviderQosAttributes(_this);
    }

    return result;
}


os_equality
cmn_qosCompareByName (
        void *o1,
        void *o2,
        void *unused)
{
    int result;

    OS_UNUSED_ARG(unused);

    assert(o1);
    assert(o2);

    result = strcmp((const char *)o1, (const char *)o2);
    if(result < 0){
        return OS_LT;
    } else if (result > 0) {
        return OS_GT;
    } else {
        return OS_EQ;
    }
}

void
cmn_qosTablefreeKey (
        void *o,
        void *unused)
{
    assert(o);

    OS_UNUSED_ARG(unused);

    ddsrt_free(o);
}

void
cmn_qosTablefreeData (
        void *o,
        void *unused)
{
    assert(o);

    OS_UNUSED_ARG(unused);

    c_free(o);
}


cmn_qpResult
processContainedAttributes(
        c_iter attributes,
        qp_parseContext ctx)
{
    cmn_qpResult result = QP_RESULT_OK;
    cf_attribute attribute;

    assert(ctx);

    while ((attribute = (cf_attribute) c_iterTakeFirst(attributes)) != NULL && result == QP_RESULT_OK)
    {
        result = processAttribute(attribute, ctx);
    }
    c_iterFree(attributes);
    return result;
}


cmn_qpResult
processAttribute(
        cf_attribute attribute,
        qp_parseContext ctx)
{
    cmn_qpResult result = QP_RESULT_OK;
    const c_char *name = cf_nodeGetName((cf_node)attribute);
    c_value value = cf_attributeValue(attribute);

    assert(value.kind == V_STRING);
    switch (ctx->scope)
    {
    case QP_SCOPE_PROFILE:
        if (strcmp(name, "name") == 0) {
            ctx->name = value.is.String;
        } else if (strcmp(name, "base_name") == 0){
            /* TODO: process base_name */
            // OS_REPORT(
            //         OS_INFO,
            //         "cmn_qosProvider::processAttribute",
            //         3,
            //         "Attribute (\"%s\") not yet supported for a <qos_profile> element...",
            //         name);
        } else if (strcmp(name, "topic_filter") != 0){ /* topic_filter has no meaning here */
            result = QP_RESULT_UNKNOWN_ARGUMENT;
            // OS_REPORT(
            //         OS_ERROR,
            //         "cmn_qosProvider::processAttribute",
            //         3,
            //         "Unknown attribute (\"%s\") for a <qos_profile> element...",
            //         name);
        }
        break;
    case QP_SCOPE_QOSPOLICY:
        if (strcmp(name, "name") == 0) {
            ctx->name = value.is.String;
        } else if (strcmp(name, "base_name") == 0){
            /* TODO: process base_name */
            // OS_REPORT(
            //         OS_INFO,
            //         "cmn_qosProvider::processAttribute",
            //         3,
            //         "Attribute (\"%s\") not yet supported for a qos element...",
            //         name);
        } else {
            result = QP_RESULT_UNKNOWN_ARGUMENT;
            // OS_REPORT(
            //         OS_ERROR,
            //         "cmn_qosProvider::processAttribute",
            //         3,
            //         "Unknown attribute (\"%s\") for a <(" DPQOS_TAG "|" TQOS_TAG "|" PUBQOS_TAG "|" SUBQOS_TAG "|" DWQOS_TAG "|" DRQOS_TAG ")_qos> element...",
            //         name);
        }
        break;
    default:
        result = QP_RESULT_UNKNOWN_ARGUMENT;
        // OS_REPORT(
        //         OS_ERROR,
        //         "cmn_qosProvider::processAttribute",
        //         3,
        //         "Element specifies an unknown attribute (\"%s\")...",
        //         name);
    }

    return result;
}

c_value
cf_attributeValue(
    cf_attribute attribute)
{
    assert(attribute != NULL);

    return attribute->value;
}


cmn_qpResult
prepareQosSample(
        const char *qosName,
        qp_entityAttr attr,
        qp_parseContext ctx)
{
    cmn_qpResult result;
    const c_type qosSampleType = attr->qosSampleType;
    assert(qosSampleType);

    assert(qosName);
    assert(ctx);

    ctx->scope = QP_SCOPE_QOSPOLICY;
    ctx->qosSample = c_new(qosSampleType);
    c_cloneIn(qosSampleType, attr->defaultQosTmplt, &ctx->qosSample);
    ctx->currentField = c_fieldNew(qosSampleType, qosName);
    if (ctx->currentField == NULL){
        result = QP_RESULT_OUT_OF_MEMORY;
        goto err_fieldNew;
    }
    ctx->currentFieldType = c_fieldType(ctx->currentField);
    assert(ctx->currentFieldType);

    ctx->qosTable = attr->qosTable;
    return QP_RESULT_OK;

/* Error handling */
err_fieldNew:
    c_free(ctx->qosSample);
    ctx->qosSample = NULL;
    return result;
}

cmn_qpResult
cmn_qosProviderGetTopicQos(
     cmn_qosProvider _this,
     const c_char *id,
     c_voidp qos)
{
    c_object q;

    assert(_this);
    assert(_this->defaultProfile);
    assert(qos);

    if((q = cmn_qosTableLookup(_this->tQosAttr.qosTable, _this->defaultProfile, id)) != NULL){
        _this->tQosAttr.copyOut(q, qos);
        return QP_RESULT_OK;
    } else {
        return QP_RESULT_NO_DATA;
    }
}

cmn_qpResult
cmn_qosProviderGetParticipantQos(
     cmn_qosProvider _this,
     const c_char *id,
     c_voidp qos)
{
    c_object q;

    assert(_this);
    assert(_this->defaultProfile);
    assert(qos);

    if((q = cmn_qosTableLookup(_this->dpQosAttr.qosTable, _this->defaultProfile, id)) != NULL){
        _this->dpQosAttr.copyOut(q, qos);
        return QP_RESULT_OK;
    } else {
        return QP_RESULT_NO_DATA;
    }
}

cmn_qpResult
cmn_qosProviderGetDataReaderQos(
     cmn_qosProvider _this,
     const c_char *id,
     c_voidp qos)
{
    c_object q;

    assert(_this);
    assert(_this->defaultProfile);
    assert(qos);

    if((q = cmn_qosTableLookup(_this->drQosAttr.qosTable, _this->defaultProfile, id)) != NULL){
        _this->drQosAttr.copyOut(q, qos);
        return QP_RESULT_OK;
    } else {
        return QP_RESULT_NO_DATA;
    }
}

cmn_qpResult
cmn_qosProviderGetDataWriterQos(
     cmn_qosProvider _this,
     const c_char *id,
     c_voidp qos)
{
    c_object q;

    assert(_this);
    assert(_this->defaultProfile);
    assert(qos);

    if((q = cmn_qosTableLookup(_this->dwQosAttr.qosTable, _this->defaultProfile, id)) != NULL){
        _this->dwQosAttr.copyOut(q, qos);
        return QP_RESULT_OK;
    } else {
        return QP_RESULT_NO_DATA;
    }
}

cmn_qpResult
cmn_qosProviderGetPublisherQos(
     cmn_qosProvider _this,
     const c_char *id,
     c_voidp qos)
{
    c_object q;

    assert(_this);
    assert(_this->defaultProfile);
    assert(qos);

    if((q = cmn_qosTableLookup(_this->pubQosAttr.qosTable, _this->defaultProfile, id)) != NULL){
        _this->pubQosAttr.copyOut(q, qos);
        return QP_RESULT_OK;
    } else {
        return QP_RESULT_NO_DATA;
    }
}

cmn_qpResult
cmn_qosProviderGetSubscriberQos(
     cmn_qosProvider _this,
     const c_char *id,
     c_voidp qos)
{
    c_object q;

    assert(_this);
    assert(_this->defaultProfile);
    assert(qos);

    if((q = cmn_qosTableLookup(_this->subQosAttr.qosTable, _this->defaultProfile, id)) != NULL){
        _this->subQosAttr.copyOut(q, qos);
        return QP_RESULT_OK;
    } else {
        return QP_RESULT_NO_DATA;
    }
}

c_object
cmn_qosTableLookup(
        ut_table t,
        const c_char *defaultProfile,
        const c_char *id)
{
    c_object qos = NULL;
    const c_char * normalized = NULL;
    const c_char * normalizedDefaultProfile;

    assert(t);
    assert(defaultProfile);

    normalizedDefaultProfile = strncmp("::", defaultProfile, strlen("::")) == 0 ? defaultProfile + strlen("::") : defaultProfile;

    if(id) {
        normalized = strncmp("::", id, strlen("::")) == 0 ? id + strlen("::") : id;
        if(id != normalized){
            /* Fully-qualified name */
            qos = ut_get(ut_collection(t), (void *)normalized);
        } else {
            c_char fullname[256];
            c_char *fn = fullname;
            int len;

            if((len = snprintf(fn, sizeof(fullname), "%s::%s", normalizedDefaultProfile, id)) >= (int) sizeof(fullname)) {
                fn = ddsrt_malloc((size_t) len + 1);
                (void) snprintf(fn, (size_t) len + 1, "%s::%s", normalizedDefaultProfile, id);
            }
            qos = ut_get(ut_collection(t), (void *)fn);
            if(fn != fullname) {
                ddsrt_free(fn);
            }
        }
    } else {
        /* Empty qosName in default profile */
        qos = ut_get(ut_collection(t), (void *)normalizedDefaultProfile);
    }

    if (qos == NULL) {
        // OS_REPORT(OS_ERROR, "QosProvider", QP_RESULT_NO_DATA,
        //                     "Could not find Qos value for profile: \"%s\" and id: \"%s\".",
        //                     defaultProfile,
        //                     (id == NULL ? "not specified" : id));
    }
    return qos;
}
#define c_metaObject(o)     ((c_metaObject)(o))

c_metaObject
__dds_namedQosTypes_DDS__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject o;

    scope = c_metaObject(base);
    o = c_metaResolve(scope,"DDS");
    if (o == NULL) {
        o = c_metaDeclare(scope,"DDS",M_MODULE);
    }

    return o;
}

c_metaObject
__DDS_NamedDomainParticipantQos__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_namedQosTypes_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));

    members = c_arrayNew(ResolveType(base,"c_object"),2);

    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"name");
    tscope = scope;
    c_specifier(members[0])->type = ResolveType(tscope,"c_string");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"domainparticipant_qos");
    tscope = __DDS_DomainParticipantQos__load (base)->definedIn;
    c_specifier(members[1])->type = ResolveType(tscope,"DomainParticipantQos");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"NamedDomainParticipantQos",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_PresentationQosPolicyAccessScopeKind__load (
    c_base base)
{
    c_metaObject scope;
    c_array elements;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_ENUMERATION));
    c_metaObject(o)->definedIn = scope;
    elements = c_arrayNew(ResolveType(base,"c_object"),3);
    elements[0] = (c_voidp)c_metaDeclareEnumElement(scope,"INSTANCE_PRESENTATION_QOS");
    elements[1] = (c_voidp)c_metaDeclareEnumElement(scope,"TOPIC_PRESENTATION_QOS");
    elements[2] = (c_voidp)c_metaDeclareEnumElement(scope,"GROUP_PRESENTATION_QOS");
    c_enumeration(o)->elements = elements;
    if (c_metaFinalize(o) == S_ACCEPTED) {
        found = c_metaBind(scope,"PresentationQosPolicyAccessScopeKind",o);
    } else {
        found = NULL;
    }
    c_free(o);
    return found;
}


c_metaObject
__DDS_PresentationQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),3);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"access_scope");
    tscope = __DDS_PresentationQosPolicyAccessScopeKind__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"PresentationQosPolicyAccessScopeKind");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"coherent_access");
    tscope = scope;
    c_specifier(members[1])->type = ResolveType(tscope,"c_bool");
    members[2] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[2])->name = c_stringNew(base,"ordered_access");
    tscope = scope;
    c_specifier(members[2])->type = ResolveType(tscope,"c_bool");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"PresentationQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_PartitionQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),1);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"name");
    tscope = __DDS_StringSeq__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"StringSeq");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"PartitionQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_GroupDataQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),1);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"value");
    tscope = __DDS_octSeq__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"octSeq");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"GroupDataQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_PublisherQos__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_dcps_builtintopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),4);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"presentation");
    tscope = __DDS_PresentationQosPolicy__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"PresentationQosPolicy");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"partition");
    tscope = __DDS_PartitionQosPolicy__load (base)->definedIn;
    c_specifier(members[1])->type = ResolveType(tscope,"PartitionQosPolicy");
    members[2] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[2])->name = c_stringNew(base,"group_data");
    tscope = __DDS_GroupDataQosPolicy__load (base)->definedIn;
    c_specifier(members[2])->type = ResolveType(tscope,"GroupDataQosPolicy");
    members[3] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[3])->name = c_stringNew(base,"entity_factory");
    tscope = __DDS_EntityFactoryQosPolicy__load (base)->definedIn;
    c_specifier(members[3])->type = ResolveType(tscope,"EntityFactoryQosPolicy");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"PublisherQos",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_NamedPublisherQos__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_namedQosTypes_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),2);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"name");
    tscope = scope;
    c_specifier(members[0])->type = ResolveType(tscope,"c_string");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"publisher_qos");
    tscope = __DDS_PublisherQos__load (base)->definedIn;
    c_specifier(members[1])->type = ResolveType(tscope,"PublisherQos");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"NamedPublisherQos",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_SubscriberQos__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_dcps_builtintopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),5);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"presentation");
    tscope = __DDS_PresentationQosPolicy__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"PresentationQosPolicy");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"partition");
    tscope = __DDS_PartitionQosPolicy__load (base)->definedIn;
    c_specifier(members[1])->type = ResolveType(tscope,"PartitionQosPolicy");
    members[2] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[2])->name = c_stringNew(base,"group_data");
    tscope = __DDS_GroupDataQosPolicy__load (base)->definedIn;
    c_specifier(members[2])->type = ResolveType(tscope,"GroupDataQosPolicy");
    members[3] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[3])->name = c_stringNew(base,"entity_factory");
    tscope = __DDS_EntityFactoryQosPolicy__load (base)->definedIn;
    c_specifier(members[3])->type = ResolveType(tscope,"EntityFactoryQosPolicy");
    members[4] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[4])->name = c_stringNew(base,"share");
    tscope = __DDS_ShareQosPolicy__load (base)->definedIn;
    c_specifier(members[4])->type = ResolveType(tscope,"ShareQosPolicy");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"SubscriberQos",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_NamedSubscriberQos__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_namedQosTypes_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),2);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"name");
    tscope = scope;
    c_specifier(members[0])->type = ResolveType(tscope,"c_string");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"subscriber_qos");
    tscope = __DDS_SubscriberQos__load (base)->definedIn;
    c_specifier(members[1])->type = ResolveType(tscope,"SubscriberQos");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"NamedSubscriberQos",o);
    c_free(o);

    return found;
}


c_metaObject
__DDS_TopicDataQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),1);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"value");
    tscope = __DDS_octSeq__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"octSeq");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"TopicDataQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_DurabilityServiceQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),6);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"service_cleanup_delay");
    tscope = __DDS_Duration_t__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"Duration_t");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"history_kind");
    tscope = __DDS_HistoryQosPolicyKind__load (base)->definedIn;
    c_specifier(members[1])->type = ResolveType(tscope,"HistoryQosPolicyKind");
    members[2] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[2])->name = c_stringNew(base,"history_depth");
    tscope = scope;
    c_specifier(members[2])->type = ResolveType(tscope,"c_long");
    members[3] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[3])->name = c_stringNew(base,"max_samples");
    tscope = scope;
    c_specifier(members[3])->type = ResolveType(tscope,"c_long");
    members[4] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[4])->name = c_stringNew(base,"max_instances");
    tscope = scope;
    c_specifier(members[4])->type = ResolveType(tscope,"c_long");
    members[5] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[5])->name = c_stringNew(base,"max_samples_per_instance");
    tscope = scope;
    c_specifier(members[5])->type = ResolveType(tscope,"c_long");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"DurabilityServiceQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_TransportPriorityQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),1);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"value");
    tscope = scope;
    c_specifier(members[0])->type = ResolveType(tscope,"c_long");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"TransportPriorityQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_LifespanQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),1);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"duration");
    tscope = __DDS_Duration_t__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"Duration_t");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"LifespanQosPolicy",o);
    c_free(o);

    return found;
}


c_metaObject
__DDS_TopicQos__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_dcps_builtintopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),13);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"topic_data");
    tscope = __DDS_TopicDataQosPolicy__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"TopicDataQosPolicy");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"durability");
    tscope = __DDS_DurabilityQosPolicy__load (base)->definedIn;
    c_specifier(members[1])->type = ResolveType(tscope,"DurabilityQosPolicy");
    members[2] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[2])->name = c_stringNew(base,"durability_service");
    tscope = __DDS_DurabilityServiceQosPolicy__load (base)->definedIn;
    c_specifier(members[2])->type = ResolveType(tscope,"DurabilityServiceQosPolicy");
    members[3] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[3])->name = c_stringNew(base,"deadline");
    tscope = __DDS_DeadlineQosPolicy__load (base)->definedIn;
    c_specifier(members[3])->type = ResolveType(tscope,"DeadlineQosPolicy");
    members[4] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[4])->name = c_stringNew(base,"latency_budget");
    tscope = __DDS_LatencyBudgetQosPolicy__load (base)->definedIn;
    c_specifier(members[4])->type = ResolveType(tscope,"LatencyBudgetQosPolicy");
    members[5] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[5])->name = c_stringNew(base,"liveliness");
    tscope = __DDS_LivelinessQosPolicy__load (base)->definedIn;
    c_specifier(members[5])->type = ResolveType(tscope,"LivelinessQosPolicy");
    members[6] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[6])->name = c_stringNew(base,"reliability");
    tscope = __DDS_ReliabilityQosPolicy__load (base)->definedIn;
    c_specifier(members[6])->type = ResolveType(tscope,"ReliabilityQosPolicy");
    members[7] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[7])->name = c_stringNew(base,"destination_order");
    tscope = __DDS_DestinationOrderQosPolicy__load (base)->definedIn;
    c_specifier(members[7])->type = ResolveType(tscope,"DestinationOrderQosPolicy");
    members[8] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[8])->name = c_stringNew(base,"history");
    tscope = __DDS_HistoryQosPolicy__load (base)->definedIn;
    c_specifier(members[8])->type = ResolveType(tscope,"HistoryQosPolicy");
    members[9] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[9])->name = c_stringNew(base,"resource_limits");
    tscope = __DDS_ResourceLimitsQosPolicy__load (base)->definedIn;
    c_specifier(members[9])->type = ResolveType(tscope,"ResourceLimitsQosPolicy");
    members[10] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[10])->name = c_stringNew(base,"transport_priority");
    tscope = __DDS_TransportPriorityQosPolicy__load (base)->definedIn;
    c_specifier(members[10])->type = ResolveType(tscope,"TransportPriorityQosPolicy");
    members[11] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[11])->name = c_stringNew(base,"lifespan");
    tscope = __DDS_LifespanQosPolicy__load (base)->definedIn;
    c_specifier(members[11])->type = ResolveType(tscope,"LifespanQosPolicy");
    members[12] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[12])->name = c_stringNew(base,"ownership");
    tscope = __DDS_OwnershipQosPolicy__load (base)->definedIn;
    c_specifier(members[12])->type = ResolveType(tscope,"OwnershipQosPolicy");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"TopicQos",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_NamedTopicQos__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_namedQosTypes_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),2);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"name");
    tscope = scope;
    c_specifier(members[0])->type = ResolveType(tscope,"c_string");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"topic_qos");
    tscope = __DDS_TopicQos__load (base)->definedIn;
    c_specifier(members[1])->type = ResolveType(tscope,"TopicQos");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"NamedTopicQos",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_OwnershipStrengthQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),1);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"value");
    tscope = scope;
    c_specifier(members[0])->type = ResolveType(tscope,"c_long");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"OwnershipStrengthQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_WriterDataLifecycleQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),3);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"autodispose_unregistered_instances");
    tscope = scope;
    c_specifier(members[0])->type = ResolveType(tscope,"c_bool");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"autopurge_suspended_samples_delay");
    tscope = __DDS_Duration_t__load (base)->definedIn;
    c_specifier(members[1])->type = ResolveType(tscope,"Duration_t");
    members[2] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[2])->name = c_stringNew(base,"autounregister_instance_delay");
    tscope = __DDS_Duration_t__load (base)->definedIn;
    c_specifier(members[2])->type = ResolveType(tscope,"Duration_t");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"WriterDataLifecycleQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_DataWriterQos__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_dcps_builtintopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),14);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"durability");
    tscope = __DDS_DurabilityQosPolicy__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"DurabilityQosPolicy");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"deadline");
    tscope = __DDS_DeadlineQosPolicy__load (base)->definedIn;
    c_specifier(members[1])->type = ResolveType(tscope,"DeadlineQosPolicy");
    members[2] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[2])->name = c_stringNew(base,"latency_budget");
    tscope = __DDS_LatencyBudgetQosPolicy__load (base)->definedIn;
    c_specifier(members[2])->type = ResolveType(tscope,"LatencyBudgetQosPolicy");
    members[3] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[3])->name = c_stringNew(base,"liveliness");
    tscope = __DDS_LivelinessQosPolicy__load (base)->definedIn;
    c_specifier(members[3])->type = ResolveType(tscope,"LivelinessQosPolicy");
    members[4] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[4])->name = c_stringNew(base,"reliability");
    tscope = __DDS_ReliabilityQosPolicy__load (base)->definedIn;
    c_specifier(members[4])->type = ResolveType(tscope,"ReliabilityQosPolicy");
    members[5] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[5])->name = c_stringNew(base,"destination_order");
    tscope = __DDS_DestinationOrderQosPolicy__load (base)->definedIn;
    c_specifier(members[5])->type = ResolveType(tscope,"DestinationOrderQosPolicy");
    members[6] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[6])->name = c_stringNew(base,"history");
    tscope = __DDS_HistoryQosPolicy__load (base)->definedIn;
    c_specifier(members[6])->type = ResolveType(tscope,"HistoryQosPolicy");
    members[7] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[7])->name = c_stringNew(base,"resource_limits");
    tscope = __DDS_ResourceLimitsQosPolicy__load (base)->definedIn;
    c_specifier(members[7])->type = ResolveType(tscope,"ResourceLimitsQosPolicy");
    members[8] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[8])->name = c_stringNew(base,"transport_priority");
    tscope = __DDS_TransportPriorityQosPolicy__load (base)->definedIn;
    c_specifier(members[8])->type = ResolveType(tscope,"TransportPriorityQosPolicy");
    members[9] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[9])->name = c_stringNew(base,"lifespan");
    tscope = __DDS_LifespanQosPolicy__load (base)->definedIn;
    c_specifier(members[9])->type = ResolveType(tscope,"LifespanQosPolicy");
    members[10] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[10])->name = c_stringNew(base,"user_data");
    tscope = __DDS_UserDataQosPolicy__load (base)->definedIn;
    c_specifier(members[10])->type = ResolveType(tscope,"UserDataQosPolicy");
    members[11] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[11])->name = c_stringNew(base,"ownership");
    tscope = __DDS_OwnershipQosPolicy__load (base)->definedIn;
    c_specifier(members[11])->type = ResolveType(tscope,"OwnershipQosPolicy");
    members[12] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[12])->name = c_stringNew(base,"ownership_strength");
    tscope = __DDS_OwnershipStrengthQosPolicy__load (base)->definedIn;
    c_specifier(members[12])->type = ResolveType(tscope,"OwnershipStrengthQosPolicy");
    members[13] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[13])->name = c_stringNew(base,"writer_data_lifecycle");
    tscope = __DDS_WriterDataLifecycleQosPolicy__load (base)->definedIn;
    c_specifier(members[13])->type = ResolveType(tscope,"WriterDataLifecycleQosPolicy");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"DataWriterQos",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_NamedDataWriterQos__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_namedQosTypes_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),2);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"name");
    tscope = scope;
    c_specifier(members[0])->type = ResolveType(tscope,"c_string");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"datawriter_qos");
    tscope = __DDS_DataWriterQos__load (base)->definedIn;
    c_specifier(members[1])->type = ResolveType(tscope,"DataWriterQos");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"NamedDataWriterQos",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_NamedDataReaderQos__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_namedQosTypes_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),2);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"name");
    tscope = scope;
    c_specifier(members[0])->type = ResolveType(tscope,"c_string");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"datareader_qos");
    tscope = __DDS_DataReaderQos__load (base)->definedIn;
    c_specifier(members[1])->type = ResolveType(tscope,"DataReaderQos");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"NamedDataReaderQos",o);
    c_free(o);

    return found;
}


void
qp_NamedDomainParticipantQos__shallowClone(
        void *_from,
        void *_to)
{
    struct _DDS_NamedDomainParticipantQos *from = (struct _DDS_NamedDomainParticipantQos *)_from;
    struct _DDS_NamedDomainParticipantQos *to = (struct _DDS_NamedDomainParticipantQos *)_to;

    *to = *from;
}

void
qp_NamedTopicQos__shallowClone(
        void *_from,
        void *_to)
{
    struct _DDS_NamedTopicQos *from = (struct _DDS_NamedTopicQos *)_from;
    struct _DDS_NamedTopicQos *to = (struct _DDS_NamedTopicQos *)_to;

    *to = *from;
}

void
qp_NamedPublisherQos__shallowClone(
        void *_from,
        void *_to)
{
    struct _DDS_NamedPublisherQos *from = (struct _DDS_NamedPublisherQos *)_from;
    struct _DDS_NamedPublisherQos *to = (struct _DDS_NamedPublisherQos *)_to;

    *to = *from;
}

void
qp_NamedSubscriberQos__shallowClone(
        void *_from,
        void *_to)
{
    struct _DDS_NamedSubscriberQos *from = (struct _DDS_NamedSubscriberQos *)_from;
    struct _DDS_NamedSubscriberQos *to = (struct _DDS_NamedSubscriberQos *)_to;

    *to = *from;
}

void
qp_NamedDataWriterQos__shallowClone(
        void *_from,
        void *_to)
{
    struct _DDS_NamedDataWriterQos *from = (struct _DDS_NamedDataWriterQos *)_from;
    struct _DDS_NamedDataWriterQos *to = (struct _DDS_NamedDataWriterQos *)_to;

    *to = *from;
}

void
qp_NamedDataReaderQos__shallowClone(
        void *_from,
        void *_to)
{
    struct _DDS_NamedDataReaderQos *from = (struct _DDS_NamedDataReaderQos *)_from;
    struct _DDS_NamedDataReaderQos *to = (struct _DDS_NamedDataReaderQos *)_to;

    *to = *from;
}


c_metaObject
__DDS_DomainParticipantQos__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_dcps_builtintopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),4);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"user_data");
    tscope = __DDS_UserDataQosPolicy__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"UserDataQosPolicy");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"entity_factory");
    tscope = __DDS_EntityFactoryQosPolicy__load (base)->definedIn;
    c_specifier(members[1])->type = ResolveType(tscope,"EntityFactoryQosPolicy");
    members[2] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[2])->name = c_stringNew(base,"watchdog_scheduling");
    tscope = __DDS_SchedulingQosPolicy__load (base)->definedIn;
    c_specifier(members[2])->type = ResolveType(tscope,"SchedulingQosPolicy");
    members[3] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[3])->name = c_stringNew(base,"listener_scheduling");
    tscope = __DDS_SchedulingQosPolicy__load (base)->definedIn;
    c_specifier(members[3])->type = ResolveType(tscope,"SchedulingQosPolicy");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"DomainParticipantQos",o);
    c_free(o);

    return found;
}

c_metaObject
__dds_dcps_builtintopics_DDS__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject o;

    scope = c_metaObject(base);
    o = c_metaResolve(scope,"DDS");
    if (o == NULL) {
        o = c_metaDeclare(scope,"DDS",M_MODULE);
    }

    return o;
}


c_metaObject
__DDS_UserDataQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),1);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"value");
    tscope = __DDS_octSeq__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"octSeq");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"UserDataQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__dds_builtinTopics_DDS__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject o;

    scope = c_metaObject(base);
    o = c_metaResolve(scope,"DDS");
    if (o == NULL) {
        o = c_metaDeclare(scope,"DDS",M_MODULE);
    }

    return o;
}

c_metaObject
__DDS_octSeq__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_COLLECTION));
    c_metaObject(o)->definedIn = scope;
    c_collectionType(o)->kind = OSPL_C_SEQUENCE;
    tscope = scope;
    c_collectionType(o)->subType = ResolveType(tscope,"c_octet");
    c_collectionType(o)->maxSize = 0;
    c_metaFinalize(o);

    c_free(c_metaBind(scope,"C_SEQUENCE<c_octet>",o));
    c_free(o);

    tscope = scope;
    o = c_metaObject(c_metaDefine(scope,M_TYPEDEF));
    c_metaObject(o)->definedIn = scope;
    tscope = scope;
    c_typeDef(o)->alias = ResolveType(tscope,"C_SEQUENCE<c_octet>");
    c_metaFinalize(o);
    found = c_metaBind(scope,"octSeq",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_EntityFactoryQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),1);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"autoenable_created_entities");
    tscope = scope;
    c_specifier(members[0])->type = ResolveType(tscope,"c_bool");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"EntityFactoryQosPolicy",o);
    c_free(o);

    return found;
}


c_metaObject
__DDS_SchedulingQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_dcps_builtintopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),3);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"scheduling_class");
    tscope = __DDS_SchedulingClassQosPolicy__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"SchedulingClassQosPolicy");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"scheduling_priority_kind");
    tscope = __DDS_SchedulingPriorityQosPolicy__load (base)->definedIn;
    c_specifier(members[1])->type = ResolveType(tscope,"SchedulingPriorityQosPolicy");
    members[2] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[2])->name = c_stringNew(base,"scheduling_priority");
    tscope = scope;
    c_specifier(members[2])->type = ResolveType(tscope,"c_long");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"SchedulingQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_SchedulingClassQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_dcps_builtintopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),1);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"kind");
    tscope = __DDS_SchedulingClassQosPolicyKind__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"SchedulingClassQosPolicyKind");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"SchedulingClassQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_SchedulingClassQosPolicyKind__load (
    c_base base)
{
    c_metaObject scope;
    c_array elements;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_dcps_builtintopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_ENUMERATION));
    c_metaObject(o)->definedIn = scope;
    elements = c_arrayNew(ResolveType(base,"c_object"),3);
    elements[0] = (c_voidp)c_metaDeclareEnumElement(scope,"SCHEDULE_DEFAULT");
    elements[1] = (c_voidp)c_metaDeclareEnumElement(scope,"SCHEDULE_TIMESHARING");
    elements[2] = (c_voidp)c_metaDeclareEnumElement(scope,"SCHEDULE_REALTIME");
    c_enumeration(o)->elements = elements;
    if (c_metaFinalize(o) == S_ACCEPTED) {
        found = c_metaBind(scope,"SchedulingClassQosPolicyKind",o);
    } else {
        found = NULL;
    }
    c_free(o);
    return found;
}

c_metaObject
__DDS_SchedulingPriorityQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_dcps_builtintopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),1);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"kind");
    tscope = __DDS_SchedulingPriorityQosPolicyKind__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"SchedulingPriorityQosPolicyKind");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"SchedulingPriorityQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_SchedulingPriorityQosPolicyKind__load (
    c_base base)
{
    c_metaObject scope;
    c_array elements;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_dcps_builtintopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_ENUMERATION));
    c_metaObject(o)->definedIn = scope;
    elements = c_arrayNew(ResolveType(base,"c_object"),2);
    elements[0] = (c_voidp)c_metaDeclareEnumElement(scope,"PRIORITY_RELATIVE");
    elements[1] = (c_voidp)c_metaDeclareEnumElement(scope,"PRIORITY_ABSOLUTE");
    c_enumeration(o)->elements = elements;
    if (c_metaFinalize(o) == S_ACCEPTED) {
        found = c_metaBind(scope,"SchedulingPriorityQosPolicyKind",o);
    } else {
        found = NULL;
    }
    c_free(o);
    return found;
}

c_metaObject
__DDS_DurabilityQosPolicyKind__load (
    c_base base)
{
    c_metaObject scope;
    c_array elements;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_ENUMERATION));
    c_metaObject(o)->definedIn = scope;
    elements = c_arrayNew(ResolveType(base,"c_object"),4);
    elements[0] = (c_voidp)c_metaDeclareEnumElement(scope,"VOLATILE_DURABILITY_QOS");
    elements[1] = (c_voidp)c_metaDeclareEnumElement(scope,"TRANSIENT_LOCAL_DURABILITY_QOS");
    elements[2] = (c_voidp)c_metaDeclareEnumElement(scope,"TRANSIENT_DURABILITY_QOS");
    elements[3] = (c_voidp)c_metaDeclareEnumElement(scope,"PERSISTENT_DURABILITY_QOS");
    c_enumeration(o)->elements = elements;
    if (c_metaFinalize(o) == S_ACCEPTED) {
        found = c_metaBind(scope,"DurabilityQosPolicyKind",o);
    } else {
        found = NULL;
    }
    c_free(o);
    return found;
}

c_metaObject
__DDS_DurabilityQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),1);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"kind");
    tscope = __DDS_DurabilityQosPolicyKind__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"DurabilityQosPolicyKind");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"DurabilityQosPolicy",o);
    c_free(o);

    return found;
}


c_metaObject
__DDS_Duration_t__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),2);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"sec");
    tscope = scope;
    c_specifier(members[0])->type = ResolveType(tscope,"c_long");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"nanosec");
    tscope = scope;
    c_specifier(members[1])->type = ResolveType(tscope,"c_ulong");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"Duration_t",o);
    c_free(o);

    return found;
}


c_metaObject
__DDS_DeadlineQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),1);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"period");
    tscope = __DDS_Duration_t__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"Duration_t");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"DeadlineQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_LatencyBudgetQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),1);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"duration");
    tscope = __DDS_Duration_t__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"Duration_t");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"LatencyBudgetQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_LivelinessQosPolicyKind__load (
    c_base base)
{
    c_metaObject scope;
    c_array elements;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_ENUMERATION));
    c_metaObject(o)->definedIn = scope;
    elements = c_arrayNew(ResolveType(base,"c_object"),3);
    elements[0] = (c_voidp)c_metaDeclareEnumElement(scope,"AUTOMATIC_LIVELINESS_QOS");
    elements[1] = (c_voidp)c_metaDeclareEnumElement(scope,"MANUAL_BY_PARTICIPANT_LIVELINESS_QOS");
    elements[2] = (c_voidp)c_metaDeclareEnumElement(scope,"MANUAL_BY_TOPIC_LIVELINESS_QOS");
    c_enumeration(o)->elements = elements;
    if (c_metaFinalize(o) == S_ACCEPTED) {
        found = c_metaBind(scope,"LivelinessQosPolicyKind",o);
    } else {
        found = NULL;
    }
    c_free(o);
    return found;
}

c_metaObject
__DDS_LivelinessQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),2);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"kind");
    tscope = __DDS_LivelinessQosPolicyKind__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"LivelinessQosPolicyKind");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"lease_duration");
    tscope = __DDS_Duration_t__load (base)->definedIn;
    c_specifier(members[1])->type = ResolveType(tscope,"Duration_t");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"LivelinessQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_ReliabilityQosPolicyKind__load (
    c_base base)
{
    c_metaObject scope;
    c_array elements;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_ENUMERATION));
    c_metaObject(o)->definedIn = scope;
    elements = c_arrayNew(ResolveType(base,"c_object"),2);
    elements[0] = (c_voidp)c_metaDeclareEnumElement(scope,"BEST_EFFORT_RELIABILITY_QOS");
    elements[1] = (c_voidp)c_metaDeclareEnumElement(scope,"RELIABLE_RELIABILITY_QOS");
    c_enumeration(o)->elements = elements;
    if (c_metaFinalize(o) == S_ACCEPTED) {
        found = c_metaBind(scope,"ReliabilityQosPolicyKind",o);
    } else {
        found = NULL;
    }
    c_free(o);
    return found;
}

c_metaObject
__DDS_ReliabilityQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),3);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"kind");
    tscope = __DDS_ReliabilityQosPolicyKind__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"ReliabilityQosPolicyKind");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"max_blocking_time");
    tscope = __DDS_Duration_t__load (base)->definedIn;
    c_specifier(members[1])->type = ResolveType(tscope,"Duration_t");
    members[2] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[2])->name = c_stringNew(base,"synchronous");
    tscope = scope;
    c_specifier(members[2])->type = ResolveType(tscope,"c_bool");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"ReliabilityQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_DestinationOrderQosPolicyKind__load (
    c_base base)
{
    c_metaObject scope;
    c_array elements;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_ENUMERATION));
    c_metaObject(o)->definedIn = scope;
    elements = c_arrayNew(ResolveType(base,"c_object"),2);
    elements[0] = (c_voidp)c_metaDeclareEnumElement(scope,"BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS");
    elements[1] = (c_voidp)c_metaDeclareEnumElement(scope,"BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS");
    c_enumeration(o)->elements = elements;
    if (c_metaFinalize(o) == S_ACCEPTED) {
        found = c_metaBind(scope,"DestinationOrderQosPolicyKind",o);
    } else {
        found = NULL;
    }
    c_free(o);
    return found;
}

c_metaObject
__DDS_DestinationOrderQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),1);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"kind");
    tscope = __DDS_DestinationOrderQosPolicyKind__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"DestinationOrderQosPolicyKind");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"DestinationOrderQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_HistoryQosPolicyKind__load (
    c_base base)
{
    c_metaObject scope;
    c_array elements;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_ENUMERATION));
    c_metaObject(o)->definedIn = scope;
    elements = c_arrayNew(ResolveType(base,"c_object"),2);
    elements[0] = (c_voidp)c_metaDeclareEnumElement(scope,"KEEP_LAST_HISTORY_QOS");
    elements[1] = (c_voidp)c_metaDeclareEnumElement(scope,"KEEP_ALL_HISTORY_QOS");
    c_enumeration(o)->elements = elements;
    if (c_metaFinalize(o) == S_ACCEPTED) {
        found = c_metaBind(scope,"HistoryQosPolicyKind",o);
    } else {
        found = NULL;
    }
    c_free(o);
    return found;
}

c_metaObject
__DDS_HistoryQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),2);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"kind");
    tscope = __DDS_HistoryQosPolicyKind__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"HistoryQosPolicyKind");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"depth");
    tscope = scope;
    c_specifier(members[1])->type = ResolveType(tscope,"c_long");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"HistoryQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_ResourceLimitsQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),3);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"max_samples");
    tscope = scope;
    c_specifier(members[0])->type = ResolveType(tscope,"c_long");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"max_instances");
    tscope = scope;
    c_specifier(members[1])->type = ResolveType(tscope,"c_long");
    members[2] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[2])->name = c_stringNew(base,"max_samples_per_instance");
    tscope = scope;
    c_specifier(members[2])->type = ResolveType(tscope,"c_long");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"ResourceLimitsQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_OwnershipQosPolicyKind__load (
    c_base base)
{
    c_metaObject scope;
    c_array elements;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_ENUMERATION));
    c_metaObject(o)->definedIn = scope;
    elements = c_arrayNew(ResolveType(base,"c_object"),2);
    elements[0] = (c_voidp)c_metaDeclareEnumElement(scope,"SHARED_OWNERSHIP_QOS");
    elements[1] = (c_voidp)c_metaDeclareEnumElement(scope,"EXCLUSIVE_OWNERSHIP_QOS");
    c_enumeration(o)->elements = elements;
    if (c_metaFinalize(o) == S_ACCEPTED) {
        found = c_metaBind(scope,"OwnershipQosPolicyKind",o);
    } else {
        found = NULL;
    }
    c_free(o);
    return found;
}

c_metaObject
__DDS_OwnershipQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),1);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"kind");
    tscope = __DDS_OwnershipQosPolicyKind__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"OwnershipQosPolicyKind");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"OwnershipQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_TimeBasedFilterQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),1);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"minimum_separation");
    tscope = __DDS_Duration_t__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"Duration_t");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"TimeBasedFilterQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_InvalidSampleVisibilityQosPolicyKind__load (
    c_base base)
{
    c_metaObject scope;
    c_array elements;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_ENUMERATION));
    c_metaObject(o)->definedIn = scope;
    elements = c_arrayNew(ResolveType(base,"c_object"),3);
    elements[0] = (c_voidp)c_metaDeclareEnumElement(scope,"NO_INVALID_SAMPLES");
    elements[1] = (c_voidp)c_metaDeclareEnumElement(scope,"MINIMUM_INVALID_SAMPLES");
    elements[2] = (c_voidp)c_metaDeclareEnumElement(scope,"ALL_INVALID_SAMPLES");
    c_enumeration(o)->elements = elements;
    if (c_metaFinalize(o) == S_ACCEPTED) {
        found = c_metaBind(scope,"InvalidSampleVisibilityQosPolicyKind",o);
    } else {
        found = NULL;
    }
    c_free(o);
    return found;
}

c_metaObject
__DDS_InvalidSampleVisibilityQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),1);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"kind");
    tscope = __DDS_InvalidSampleVisibilityQosPolicyKind__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"InvalidSampleVisibilityQosPolicyKind");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"InvalidSampleVisibilityQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_ReaderDataLifecycleQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),5);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"autopurge_nowriter_samples_delay");
    tscope = __DDS_Duration_t__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"Duration_t");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"autopurge_disposed_samples_delay");
    tscope = __DDS_Duration_t__load (base)->definedIn;
    c_specifier(members[1])->type = ResolveType(tscope,"Duration_t");
    members[2] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[2])->name = c_stringNew(base,"autopurge_dispose_all");
    tscope = scope;
    c_specifier(members[2])->type = ResolveType(tscope,"c_bool");
    members[3] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[3])->name = c_stringNew(base,"enable_invalid_samples");
    tscope = scope;
    c_specifier(members[3])->type = ResolveType(tscope,"c_bool");
    members[4] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[4])->name = c_stringNew(base,"invalid_sample_visibility");
    tscope = __DDS_InvalidSampleVisibilityQosPolicy__load (base)->definedIn;
    c_specifier(members[4])->type = ResolveType(tscope,"InvalidSampleVisibilityQosPolicy");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"ReaderDataLifecycleQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_StringSeq__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_COLLECTION));
    c_metaObject(o)->definedIn = scope;
    c_collectionType(o)->kind = OSPL_C_SEQUENCE;
    tscope = scope;
    c_collectionType(o)->subType = ResolveType(tscope,"c_string");
    c_collectionType(o)->maxSize = 0;
    c_metaFinalize(o);

    c_free(c_metaBind(scope,"C_SEQUENCE<c_string>",o));
    c_free(o);

    tscope = scope;
    o = c_metaObject(c_metaDefine(scope,M_TYPEDEF));
    c_metaObject(o)->definedIn = scope;
    tscope = scope;
    c_typeDef(o)->alias = ResolveType(tscope,"C_SEQUENCE<c_string>");
    c_metaFinalize(o);
    found = c_metaBind(scope,"StringSeq",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_SubscriptionKeyQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),2);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"use_key_list");
    tscope = scope;
    c_specifier(members[0])->type = ResolveType(tscope,"c_bool");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"key_list");
    tscope = __DDS_StringSeq__load (base)->definedIn;
    c_specifier(members[1])->type = ResolveType(tscope,"StringSeq");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"SubscriptionKeyQosPolicy",o);
    c_free(o);

    return found;
}


c_metaObject
__DDS_ReaderLifespanQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),2);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"use_lifespan");
    tscope = scope;
    c_specifier(members[0])->type = ResolveType(tscope,"c_bool");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"duration");
    tscope = __DDS_Duration_t__load (base)->definedIn;
    c_specifier(members[1])->type = ResolveType(tscope,"Duration_t");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"ReaderLifespanQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_ShareQosPolicy__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_builtinTopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),2);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"name");
    tscope = scope;
    c_specifier(members[0])->type = ResolveType(tscope,"c_string");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"enable");
    tscope = scope;
    c_specifier(members[1])->type = ResolveType(tscope,"c_bool");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"ShareQosPolicy",o);
    c_free(o);

    return found;
}

c_metaObject
__DDS_DataReaderQos__load (
    c_base base)
{
    c_metaObject scope;
    c_metaObject tscope;
    c_array members;
    c_metaObject o;
    c_metaObject found;

    scope = __dds_dcps_builtintopics_DDS__load (base);
    o = c_metaObject(c_metaDefine(scope,M_STRUCTURE));
    members = c_arrayNew(ResolveType(base,"c_object"),15);
    c_metaObject(o)->definedIn = scope;
    members[0] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[0])->name = c_stringNew(base,"durability");
    tscope = __DDS_DurabilityQosPolicy__load (base)->definedIn;
    c_specifier(members[0])->type = ResolveType(tscope,"DurabilityQosPolicy");
    members[1] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[1])->name = c_stringNew(base,"deadline");
    tscope = __DDS_DeadlineQosPolicy__load (base)->definedIn;
    c_specifier(members[1])->type = ResolveType(tscope,"DeadlineQosPolicy");
    members[2] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[2])->name = c_stringNew(base,"latency_budget");
    tscope = __DDS_LatencyBudgetQosPolicy__load (base)->definedIn;
    c_specifier(members[2])->type = ResolveType(tscope,"LatencyBudgetQosPolicy");
    members[3] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[3])->name = c_stringNew(base,"liveliness");
    tscope = __DDS_LivelinessQosPolicy__load (base)->definedIn;
    c_specifier(members[3])->type = ResolveType(tscope,"LivelinessQosPolicy");
    members[4] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[4])->name = c_stringNew(base,"reliability");
    tscope = __DDS_ReliabilityQosPolicy__load (base)->definedIn;
    c_specifier(members[4])->type = ResolveType(tscope,"ReliabilityQosPolicy");
    members[5] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[5])->name = c_stringNew(base,"destination_order");
    tscope = __DDS_DestinationOrderQosPolicy__load (base)->definedIn;
    c_specifier(members[5])->type = ResolveType(tscope,"DestinationOrderQosPolicy");
    members[6] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[6])->name = c_stringNew(base,"history");
    tscope = __DDS_HistoryQosPolicy__load (base)->definedIn;
    c_specifier(members[6])->type = ResolveType(tscope,"HistoryQosPolicy");
    members[7] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[7])->name = c_stringNew(base,"resource_limits");
    tscope = __DDS_ResourceLimitsQosPolicy__load (base)->definedIn;
    c_specifier(members[7])->type = ResolveType(tscope,"ResourceLimitsQosPolicy");
    members[8] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[8])->name = c_stringNew(base,"user_data");
    tscope = __DDS_UserDataQosPolicy__load (base)->definedIn;
    c_specifier(members[8])->type = ResolveType(tscope,"UserDataQosPolicy");
    members[9] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[9])->name = c_stringNew(base,"ownership");
    tscope = __DDS_OwnershipQosPolicy__load (base)->definedIn;
    c_specifier(members[9])->type = ResolveType(tscope,"OwnershipQosPolicy");
    members[10] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[10])->name = c_stringNew(base,"time_based_filter");
    tscope = __DDS_TimeBasedFilterQosPolicy__load (base)->definedIn;
    c_specifier(members[10])->type = ResolveType(tscope,"TimeBasedFilterQosPolicy");
    members[11] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[11])->name = c_stringNew(base,"reader_data_lifecycle");
    tscope = __DDS_ReaderDataLifecycleQosPolicy__load (base)->definedIn;
    c_specifier(members[11])->type = ResolveType(tscope,"ReaderDataLifecycleQosPolicy");
    members[12] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[12])->name = c_stringNew(base,"subscription_keys");
    tscope = __DDS_SubscriptionKeyQosPolicy__load (base)->definedIn;
    c_specifier(members[12])->type = ResolveType(tscope,"SubscriptionKeyQosPolicy");
    members[13] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[13])->name = c_stringNew(base,"reader_lifespan");
    tscope = __DDS_ReaderLifespanQosPolicy__load (base)->definedIn;
    c_specifier(members[13])->type = ResolveType(tscope,"ReaderLifespanQosPolicy");
    members[14] = (c_voidp)c_metaDefine(scope,M_MEMBER);
    c_specifier(members[14])->name = c_stringNew(base,"share");
    tscope = __DDS_ShareQosPolicy__load (base)->definedIn;
    c_specifier(members[14])->type = ResolveType(tscope,"ShareQosPolicy");
    c_structure(o)->members = members;
    c_metaFinalize(o);
    found = c_metaBind(scope,"DataReaderQos",o);
    c_free(o);

    return found;
}