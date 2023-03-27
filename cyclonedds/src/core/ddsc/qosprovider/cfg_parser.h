/*
 *                         Vortex OpenSplice
 *
 *   This software and documentation are Copyright 2006 to TO_YEAR ADLINK
 *   Technology Limited, its affiliated companies and licensors. All rights
 *   reserved.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
#ifndef CFG_PARSER_H
#define CFG_PARSER_H


#if defined (__cplusplus)
extern "C" {
#endif

#include "c_typebase.h"
#include "c_mmbase.h"
#include "dds/ddsrt/xmlparser.h"
#include "dds/ddsrt/environ.h"

/* !!!!!!!!NOTE From here no more includes are allowed!!!!!!! */

typedef enum cfgprs_status_enum {
    CFGPRS_OK,
    CFGPRS_NO_INPUT,
    CFGPRS_ERROR
} cfgprs_status;


struct parg {
    int error;
    cf_element *toplevel;
    c_iter stack;
};

OS_API cfgprs_status
cfg_parse_init ();

OS_API cfgprs_status
cfg_parse_deinit ();

OS_API cfgprs_status
cfg_parse_ospl (
    const char *uri,
    cf_element *spliceElement);

OS_API cfgprs_status
cfg_parse_str (
    const char *str,
    cf_element *spliceElement);


#if defined (__cplusplus)
}
#endif

#endif /* CFG_PARSER_H */
