#ifndef TAIRDOC_H
#define TAIRDOC_H

#define EX_OBJ_SET_NO_FLAGS 0
#define EX_OBJ_SET_NX (1<<0)            /* Set if key not exists. */
#define EX_OBJ_SET_XX (1<<1)            /* Set if key exists. */

#define TAIRDOC_ERROR_EMPTY_STRING "ERR the empty string is not a valid JSON value"
#define TAIRDOC_ERROR_JSONOBJECT_ERROR "ERR compose patch error (probably OOM)"
#define TAIRDOC_ERROR_NEW_NOT_ROOT "ERR new objects must be created at the root"
#define TAIRDOC_ERROR_PARSE_POINTER "ERR pointer illegal or array index error or object type is not array or map"
#define TAIRDOC_ERROR_PATH_OR_POINTER_ILLEGAL "ERR JSONPointer or JSONPath illegal"
#define TAIRDOC_ERROR_JSON_TYPE_ERROR "ERR (critical) the json type illegal, please check"
#define TAIRDOC_ERROR_NOT_NUMBER "ERR node not exists or not number type"
#define TAIRDOC_ERROR_NOT_STRING "ERR node not exists or not string type"
#define TAIRDOC_ERROR_NOT_ARRAY "ERR node not exists or not array type"
#define TAIRDOC_ERROR_INCR_OVERFLOW "ERR increment would produce NaN or Infinity"
#define TAIRDOC_ERROR_CREATR_NODE "ERR create node error (probably OOM)"
#define TAIRDOC_ERROR_ARRAY_OUTFLOW "ERR array index outflow"
#define TAIRDOC_ERROR_GET_FROMAT_ERROR "ERR format error, must be yaml or xml"
#define TAIRDOC_SYNTAX_ERROR "ERR syntax error"
#define TAIRDOC_NO_SUCKKEY_ERROR "ERR no such key"
#define TAIRDOC_VALUE_OUTOF_RANGE "ERR value is not an integer or out of range"
#define TAIRDOC_PATH_TO_POINTER_ERROR "ERR json path to json pointer fail"

#define TAIRDOC_JSONPATH_START_DOLLAR '$'
#define TAIRDOC_JSONPATH_START_DOT '.'
#define TAIRDOC_JSONPATH_START_SQUARE_BRACKETS '['
#define TAIRDOC_JSONPOINTER_START '/'
#define TAIRDOC_JSONPATH_ROOT "$"
#define TAIRDOC_JSONPOINTER_ROOT ""

#endif // TAIRDOC_H
