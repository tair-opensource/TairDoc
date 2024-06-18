#include "valkeymodule.h"
#include "tairdoc.h"
#include "cJSON/cJSON.h"
#include "cJSON/cJSON_Utils.h"

#include <strings.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <assert.h>

static ValkeyModuleType *TairDocType;
#define TAIRDOC_ENC_VER 0

void debugPrint(ValkeyModuleCtx *ctx, char *name, cJSON *root) {
    VALKEYMODULE_NOT_USED(ctx);
    VALKEYMODULE_NOT_USED(name);
    VALKEYMODULE_NOT_USED(root);
#ifdef JSON_DEBUG
    char *print = cJSON_Print(root);
    ValkeyModule_Log(ctx, "notice", "%s : %s", name, print);
    ValkeyModule_Free(print);
#endif
}

/* ========================== TairDoc function methods ======================= */

#define PATH_TO_POINTER(ctx, path, rpointer)                              \
    if (pathToPointer((ctx), (path), &(rpointer)) != 0) {                 \
        ValkeyModule_ReplyWithError(ctx, TAIRDOC_PATH_TO_POINTER_ERROR);    \
        return VALKEYMODULE_ERR;                                           \
    }

/*
 * create node from json
 */
int createNodeFromJson(cJSON **node, const char *json, ValkeyModuleString **jerr) {
    *node = cJSON_ParseWithOpts(json, NULL, 1);
    if (*node == NULL) {
        *jerr = ValkeyModule_CreateStringPrintf(NULL, "ERR json lexer error at position '%s'", cJSON_GetErrorPtr());
        return VALKEYMODULE_ERR;
    }

    return VALKEYMODULE_OK;
}

int composePatch(cJSON *const patches, const unsigned char *const operation, const char *const path,
                 const cJSON *const value) {
    cJSON *patch = NULL;
    if ((patches == NULL) || (operation == NULL) || (path == NULL)) {
        return VALKEYMODULE_ERR;
    }

    patch = cJSON_CreateObject();
    if (patch == NULL) {
        return VALKEYMODULE_ERR;
    }
    cJSON_AddItemToObject(patch, "op", cJSON_CreateString((const char *) operation));
    cJSON_AddItemToObject(patch, "path", cJSON_CreateString(path));
    if (value != NULL) {
        cJSON_AddItemToObject(patch, "value", cJSON_Duplicate(value, 1));
    }
    cJSON_AddItemToArray(patches, patch);
    return VALKEYMODULE_OK;
}

int applyPatch(cJSON *const object, const cJSON *const patches, ValkeyModuleString **jerr) {
    int ret = cJSONUtils_ApplyPatchesCaseSensitive(object, patches);
    if (ret == 0) {
        goto ok;
    }

    if (ret == 1) {
        *jerr = ValkeyModule_CreateStringPrintf(NULL, "ERR patches is not array");
        goto error;
    } else if (ret == 2) {
        *jerr = ValkeyModule_CreateStringPrintf(NULL, "ERR malformed patch, path is not string");
        goto error;
    } else if (ret == 3) {
        *jerr = ValkeyModule_CreateStringPrintf(NULL, "ERR patch opcode is illegal");
        goto error;
    } else if (ret == 4 || ret == 5) {
        *jerr = ValkeyModule_CreateStringPrintf(NULL, "ERR missing 'from' for copy or move");
        goto error;
    } else if (ret == 7) {
        *jerr = ValkeyModule_CreateStringPrintf(NULL, "ERR missing 'value' for add or replace");
        goto error;
    } else if (ret == 8 || ret == 6) {
        *jerr = ValkeyModule_CreateStringPrintf(NULL, "ERR may be oom");
        goto error;
    } else if (ret == 9) {
        *jerr = ValkeyModule_CreateStringPrintf(NULL, "ERR could not find object to add, please check path");
        goto error;
    } else if (ret == 10) {
        *jerr = ValkeyModule_CreateStringPrintf(NULL, "ERR insert item in array error, index error");
        goto error;
    } else if (ret == 11) {
        *jerr = ValkeyModule_CreateStringPrintf(NULL, "ERR array index error");
        goto error;
    } else if (ret == 13) {
        *jerr = ValkeyModule_CreateStringPrintf(NULL, "ERR old item is null for remove or replace");
        goto error;
    } else {
        *jerr = ValkeyModule_CreateStringPrintf(NULL, "ERR apply patch unknow error");
        goto error;
    }

    error:
    return VALKEYMODULE_ERR;

    ok:
    return VALKEYMODULE_OK;
}

/* Returns the string representation json node's type. */
static inline char *jsonNodeType(const int nt) {
    static char *types[] = {"boolean", "null", "number", "string", "array", "object", "raw", "reference", "const"};
    switch (nt) {
        case cJSON_False:
        case cJSON_True:
            return types[0];
        case cJSON_NULL:
            return types[1];
        case cJSON_Number:
            return types[2];
        case cJSON_String:
            return types[3];
        case cJSON_Array:
            return types[4];
        case cJSON_Object:
            return types[5];
        case cJSON_Raw:
            return types[6];
        case cJSON_IsReference:
            return types[7];
        case cJSON_StringIsConst:
            return types[8];
        default:
            return NULL;
    }
}

int pathToPointer(ValkeyModuleCtx *ctx, const char *jpa, ValkeyModuleString **rpointer) {
    char *jpo = NULL;

    if (jpa[0] != '.' && jpa[0] != '[' && jpa[0] != '$') {
        *rpointer = ValkeyModule_CreateString(ctx, jpa, strlen(jpa));
        return 0;
    }

    if (!strcmp(jpa, ".") || !strcmp(jpa, "") || !strcmp(jpa, "$")) {
        *rpointer = ValkeyModule_CreateString(ctx, "", 0);
        return 0;
    }

    size_t i, j, size;
    size_t len = strlen(jpa), step = 0;
    for (i = 0; i < len; ++i) {
        char c = jpa[i];
        switch (c) {
            case '.':
                jpo = (char *) ValkeyModule_Realloc(jpo, step + 1);
                memcpy(jpo + step, "/", 1);
                step++;
                break;

            case '[':
                jpo = (char *) ValkeyModule_Realloc(jpo, step + 1);
                memcpy(jpo + step, "/", 1);
                step++;

                j = i + 1;
                if (jpa[j] == '\"' || jpa[j] == '\'') {
                    ++j;
                    while (j < len && jpa[j] != '\"' && jpa[j] != '\'') ++j;
                    if (j >= len) {
                        goto error;
                    }
                    size = j - i - 2;
                    jpo = (char *) ValkeyModule_Realloc(jpo, step + size);
                    memcpy(jpo + step, &jpa[i + 2], (size_t) size);
                    step += size;
                    i = j + 1;
                    if (jpa[i] != ']') {
                        goto error;
                    }
                } else if (isdigit(jpa[j]) || jpa[j] == '-') {
                    while (j < len && jpa[j] != ']') ++j;
                    if (j >= len) {
                        goto error;
                    }
                    size = j - i - 1;
                    jpo = (char *) ValkeyModule_Realloc(jpo, step + size);
                    memcpy(jpo + step, &jpa[i + 1], (size_t) size);
                    step += size;
                    i = j;
                } else {
                    goto error;
                }
                break;

            default:
                j = i + 1;
                while (j < len && jpa[j] != '.' && jpa[j] != '[') ++j;
                size = j - i;
                jpo = (char *) ValkeyModule_Realloc(jpo, step + size);
                memcpy(jpo + step, &jpa[i], (size_t) size);
                i += size - 1;
                step += size;
        }
    }

    jpo = (char *) ValkeyModule_Realloc(jpo, step + 1);
    jpo[step] = '\0';
    *rpointer = ValkeyModule_CreateString(ctx, jpo, strlen(jpo));
    ValkeyModule_Free(jpo);
    return 0;

    error:
    if (jpo) ValkeyModule_Free(jpo);
    return -1;
}

/* ========================== TairDoc commands methods ======================= */

/**
 * JSON.SET <key> <path> <json> [NX|XX]
 * Sets the JSON value at `path` in `key`
 *
 * For new Valkey keys the `path` must be the root. For existing keys, when the entire `path` exists,
 * the value that it contains is replaced with the `json` value.
 *
 * `NX` - only set the key if it does not already exists
 * `XX` - only set the key if it already exists
 *
 * Reply: Simple String `OK` if executed correctly, or Null Bulk if the specified `NX` or `XX`
 * conditions were not met.
 */
int TairDocSet_ValkeyCommand(ValkeyModuleCtx *ctx, ValkeyModuleString **argv, int argc) {
    if ((argc < 4) || (argc > 5)) {
        ValkeyModule_WrongArity(ctx);
        return VALKEYMODULE_ERR;
    }
    ValkeyModule_AutoMemory(ctx);

    ValkeyModuleString *jerr = NULL;
    int flags = EX_OBJ_SET_NO_FLAGS;
    int isRootPointer = 0, isKeyExists = 0;
    cJSON *root = NULL, *node = NULL, *patches = NULL, *pnode = NULL;

    const char *pointer = ValkeyModule_StringPtrLen(argv[2], NULL);
    ValkeyModuleString *rpointer = NULL;
    PATH_TO_POINTER(ctx, pointer, rpointer)

    if (argc == 5) {
        const char *a = ValkeyModule_StringPtrLen(argv[4], NULL);
        if (!strncasecmp(a, "nx\0", 3) && !(flags & EX_OBJ_SET_XX)) {
            flags |= EX_OBJ_SET_NX;
        } else if (!strncasecmp(a, "xx\0", 3) && !(flags & EX_OBJ_SET_NX)) {
            flags |= EX_OBJ_SET_XX;
        } else {
            ValkeyModule_ReplyWithError(ctx, TAIRDOC_SYNTAX_ERROR);
            return VALKEYMODULE_ERR;
        }
    }

    ValkeyModuleKey *key = ValkeyModule_OpenKey(ctx, argv[1], VALKEYMODULE_READ | VALKEYMODULE_WRITE);
    int type = ValkeyModule_KeyType(key);
    if (VALKEYMODULE_KEYTYPE_EMPTY == type) {
        isKeyExists = 0;
        if (flags & EX_OBJ_SET_XX) {
            goto null;
        }
    } else {
        isKeyExists = 1;
        if (ValkeyModule_ModuleTypeGetType(key) != TairDocType) {
            ValkeyModule_ReplyWithError(ctx, VALKEYMODULE_ERRORMSG_WRONGTYPE);
            return VALKEYMODULE_ERR;
        }
        root = ValkeyModule_ModuleTypeGetValue(key);
    }

    if (VALKEYMODULE_OK != createNodeFromJson(&node, ValkeyModule_StringPtrLen(argv[3], NULL), &jerr)) {
        ValkeyModule_ReplyWithError(ctx, ValkeyModule_StringPtrLen(jerr, NULL));
        ValkeyModule_FreeString(NULL, jerr);
        goto error;
    }
    debugPrint(ctx, "node", node);

    root = isKeyExists ? root : cJSON_Duplicate(node, 1);
    isRootPointer = strcasecmp("", ValkeyModule_StringPtrLen(rpointer, NULL)) ? 0 : 1;
    if (!isKeyExists) {
        if (!isRootPointer) {
            if (root) cJSON_Delete(root);
            ValkeyModule_ReplyWithError(ctx, TAIRDOC_ERROR_NEW_NOT_ROOT);
            goto error;
        }
        // if key not exists, add it.
        ValkeyModule_ModuleTypeSetValue(key, TairDocType, root);
        goto ok;
    }

    // make a patch and apply
    patches = cJSON_CreateArray();
    pnode = cJSONUtils_GetPointerCaseSensitive(root, ValkeyModule_StringPtrLen(rpointer, NULL));
    if (pnode == NULL) {
        if (flags & EX_OBJ_SET_XX) goto null;
        if (VALKEYMODULE_OK !=
            composePatch(patches, (const unsigned char *) "add", ValkeyModule_StringPtrLen(rpointer, NULL), node)) {
            ValkeyModule_ReplyWithError(ctx, TAIRDOC_ERROR_JSONOBJECT_ERROR);
            goto error;
        }
    } else {
        if (flags & EX_OBJ_SET_NX) goto null;
        if (VALKEYMODULE_OK !=
            composePatch(patches, (const unsigned char *) "replace", ValkeyModule_StringPtrLen(rpointer, NULL), node)) {
            ValkeyModule_ReplyWithError(ctx, TAIRDOC_ERROR_JSONOBJECT_ERROR);
            goto error;
        }
    }
    debugPrint(ctx, "patch", patches);

    if (VALKEYMODULE_OK != applyPatch(root, patches, &jerr)) {
        ValkeyModule_ReplyWithError(ctx, ValkeyModule_StringPtrLen(jerr, NULL));
        ValkeyModule_FreeString(NULL, jerr);
        goto error;
    }

ok:
    debugPrint(ctx, "root", root);
    ValkeyModule_ReplyWithSimpleString(ctx, "OK");
    if (node) cJSON_Delete(node);
    if (patches) cJSON_Delete(patches);
    ValkeyModule_ReplicateVerbatim(ctx);
    return VALKEYMODULE_OK;

null:
    ValkeyModule_ReplyWithNull(ctx);
    if (node) cJSON_Delete(node);
    if (patches) cJSON_Delete(patches);
    return VALKEYMODULE_OK;

error:
    if (node) cJSON_Delete(node);
    if (patches) cJSON_Delete(patches);
    return VALKEYMODULE_ERR;
}

/**
 * JSON.GET <key> [PATH]
 * Return the value at `path` in JSON serialized form.
 *
 * `key` the key
 * `path` the path of json
 *
 * Reply: Bulk String
 */

int TairDocGet_ValkeyCommand(ValkeyModuleCtx *ctx, ValkeyModuleString **argv, int argc) {
    if (argc < 2) {
        ValkeyModule_WrongArity(ctx);
        return VALKEYMODULE_ERR;
    }
    ValkeyModule_AutoMemory(ctx);

    int type = 0, needFree = 0;
    const char *print = NULL, *input = NULL;
    cJSON *root = NULL, *pnode = NULL;

    ValkeyModuleKey *key = ValkeyModule_OpenKey(ctx, argv[1], VALKEYMODULE_READ);
    type = ValkeyModule_KeyType(key);
    if (VALKEYMODULE_KEYTYPE_EMPTY == type) {
        ValkeyModule_ReplyWithNull(ctx);
        return VALKEYMODULE_OK;
    } else {
        if (ValkeyModule_ModuleTypeGetType(key) != TairDocType) {
            ValkeyModule_ReplyWithError(ctx, VALKEYMODULE_ERRORMSG_WRONGTYPE);
            return VALKEYMODULE_ERR;
        }
        root = ValkeyModule_ModuleTypeGetValue(key);
    }

    if (argc >= 3) {
        input = ValkeyModule_StringPtrLen(argv[2], NULL);
        if (input[0] == TAIRDOC_JSONPATH_START_DOLLAR) {
            pnode = cJSONUtils_GetPath(root, input);
            needFree = 1;
        } else if (input[0] == TAIRDOC_JSONPOINTER_START) {
            pnode = cJSONUtils_GetPointerCaseSensitive(root, input);
        } else {
            if (!strcmp(input, TAIRDOC_JSONPOINTER_ROOT)) {
                pnode = cJSONUtils_GetPointerCaseSensitive(root, "");
            } else if (input[0] == TAIRDOC_JSONPATH_START_DOT || input[0] == TAIRDOC_JSONPATH_START_SQUARE_BRACKETS) {
                ValkeyModuleString *rpointer = NULL;
                PATH_TO_POINTER(ctx, input, rpointer)
                pnode = cJSONUtils_GetPointerCaseSensitive(root, ValkeyModule_StringPtrLen(rpointer, NULL));
            } else {
                ValkeyModule_ReplyWithError(ctx, TAIRDOC_ERROR_PATH_OR_POINTER_ILLEGAL);
                return VALKEYMODULE_ERR;
            }
        }
    } else {
        pnode = cJSONUtils_GetPointerCaseSensitive(root, TAIRDOC_JSONPOINTER_ROOT);
    }
    if (pnode == NULL) {
        ValkeyModule_ReplyWithError(ctx, TAIRDOC_ERROR_PARSE_POINTER);
        goto error;
    }

    print = cJSON_PrintUnformatted(pnode);
    assert(print != NULL);
    ValkeyModule_ReplyWithStringBuffer(ctx, print, strlen(print));
    if (print) ValkeyModule_Free((void *) print);
    if (needFree) cJSON_Delete(pnode);
    return VALKEYMODULE_OK;

error:
    if (needFree) cJSON_Delete(pnode);
    return VALKEYMODULE_ERR;
}

/**
 * JSON.DEL <key> [path]
 * Delete a value.
 *
 * `path` defaults to root if not provided. Non-existing keys as well as non-existing paths are
 * ignored. Deleting an object's root is equivalent to deleting the key from Valkey.
 *
 * Reply: Integer, specifically the number of paths deleted (0 or 1).
 */
int TairDocDel_ValkeyCommand(ValkeyModuleCtx *ctx, ValkeyModuleString **argv, int argc) {
    if (argc < 2) {
        ValkeyModule_WrongArity(ctx);
        return VALKEYMODULE_ERR;
    }
    ValkeyModule_AutoMemory(ctx);

    ValkeyModuleString *jerr = NULL;
    int isRootPointer = 0, type;
    char *pointer = NULL;
    cJSON *root = NULL, *patches = NULL;

    ValkeyModuleKey *key = ValkeyModule_OpenKey(ctx, argv[1], VALKEYMODULE_READ | VALKEYMODULE_WRITE);
    type = ValkeyModule_KeyType(key);
    if (VALKEYMODULE_KEYTYPE_EMPTY == type) {
        ValkeyModule_ReplyWithLongLong(ctx, 0);
        return VALKEYMODULE_OK;
    } else {
        if (ValkeyModule_ModuleTypeGetType(key) != TairDocType) {
            ValkeyModule_ReplyWithError(ctx, VALKEYMODULE_ERRORMSG_WRONGTYPE);
            return VALKEYMODULE_ERR;
        }
        root = ValkeyModule_ModuleTypeGetValue(key);
    }

    pointer = argc == 3 ? (char *) ValkeyModule_StringPtrLen(argv[2], NULL) : "";
    ValkeyModuleString *rpointer = NULL;
    PATH_TO_POINTER(ctx, pointer, rpointer)

    isRootPointer = strcasecmp("", ValkeyModule_StringPtrLen(rpointer, NULL)) ? 0 : 1;

    if (isRootPointer) {
        ValkeyModule_DeleteKey(key);
        ValkeyModule_ReplyWithLongLong(ctx, 1);
        ValkeyModule_ReplicateVerbatim(ctx);
        return VALKEYMODULE_OK;
    }

    // make a patch and apply
    patches = cJSON_CreateArray();
    if (VALKEYMODULE_OK !=
        composePatch(patches, (const unsigned char *) "remove", ValkeyModule_StringPtrLen(rpointer, NULL), NULL)) {
        ValkeyModule_ReplyWithError(ctx, TAIRDOC_ERROR_JSONOBJECT_ERROR);
        goto error;
    }
    debugPrint(ctx, "patch", patches);

    if (VALKEYMODULE_OK != applyPatch(root, patches, &jerr)) {
        ValkeyModule_ReplyWithError(ctx, ValkeyModule_StringPtrLen(jerr, NULL));
        ValkeyModule_FreeString(NULL, jerr);
        goto error;
    }

    ValkeyModule_ReplyWithLongLong(ctx, 1);
    if (patches) cJSON_Delete(patches);
    if (!root->next && !root->prev && !root->child) {
        ValkeyModule_DeleteKey(key);
    }
    ValkeyModule_ReplicateVerbatim(ctx);
    return VALKEYMODULE_OK;

error:
    if (patches) cJSON_Delete(patches);
    return VALKEYMODULE_ERR;
}

/**
 * JSON.TYPE <key> [path]
 * Reports the type of JSON value at `path`.
 * `path` defaults to root if not provided. If the `key` or `path` do not exist, null is returned.
 * Reply: Simple string, specifically the type.
 */
int TairDocType_ValkeyCommand(ValkeyModuleCtx *ctx, ValkeyModuleString **argv, int argc) {
    if (argc < 2) {
        ValkeyModule_WrongArity(ctx);
        return VALKEYMODULE_ERR;
    }
    ValkeyModule_AutoMemory(ctx);

    int type = 0;
    char *print = NULL, *pointer = NULL;
    cJSON *root = NULL, *pnode = NULL;

    ValkeyModuleKey *key = ValkeyModule_OpenKey(ctx, argv[1], VALKEYMODULE_READ);
    type = ValkeyModule_KeyType(key);
    if (VALKEYMODULE_KEYTYPE_EMPTY == type) {
        ValkeyModule_ReplyWithNull(ctx);
        return VALKEYMODULE_OK;
    } else {
        if (ValkeyModule_ModuleTypeGetType(key) != TairDocType) {
            ValkeyModule_ReplyWithError(ctx, VALKEYMODULE_ERRORMSG_WRONGTYPE);
            return VALKEYMODULE_ERR;
        }
        root = ValkeyModule_ModuleTypeGetValue(key);
    }

    pointer = argc == 3 ? (char *) ValkeyModule_StringPtrLen(argv[2], NULL) : "";
    ValkeyModuleString *rpointer = NULL;
    PATH_TO_POINTER(ctx, pointer, rpointer)

    pnode = cJSONUtils_GetPointerCaseSensitive(root, ValkeyModule_StringPtrLen(rpointer, NULL));
    if (pnode == NULL) {
        ValkeyModule_ReplyWithNull(ctx);
        return VALKEYMODULE_ERR;
    }

    print = jsonNodeType(pnode->type);
    ValkeyModule_ReplyWithStringBuffer(ctx, print, strlen(print));
    return VALKEYMODULE_OK;
}

int incrGenericCommand(ValkeyModuleCtx *ctx, ValkeyModuleString **argv, int argc, double incr) {
    char *pointer = NULL;
    cJSON *root = NULL, *pnode = NULL;
    double newvalue = 0;
    int type = 0;

    ValkeyModuleKey *key = ValkeyModule_OpenKey(ctx, argv[1], VALKEYMODULE_READ | VALKEYMODULE_WRITE);
    type = ValkeyModule_KeyType(key);
    if (VALKEYMODULE_KEYTYPE_EMPTY == type) {
        ValkeyModule_ReplyWithError(ctx, TAIRDOC_NO_SUCKKEY_ERROR);
        return VALKEYMODULE_ERR;
    } else {
        if (ValkeyModule_ModuleTypeGetType(key) != TairDocType) {
            ValkeyModule_ReplyWithError(ctx, VALKEYMODULE_ERRORMSG_WRONGTYPE);
            return VALKEYMODULE_ERR;
        }
        root = ValkeyModule_ModuleTypeGetValue(key);
    }

    pointer = argc == 4 ? (char *) ValkeyModule_StringPtrLen(argv[2], NULL) : "";
    ValkeyModuleString *rpointer = NULL;
    PATH_TO_POINTER(ctx, pointer, rpointer)

    pnode = cJSONUtils_GetPointerCaseSensitive(root, ValkeyModule_StringPtrLen(rpointer, NULL));
    if (pnode == NULL || !cJSON_IsNumber(pnode)) {
        ValkeyModule_ReplyWithError(ctx, TAIRDOC_ERROR_NOT_NUMBER);
        return VALKEYMODULE_ERR;
    }

    newvalue = pnode->valuedouble + incr;
    if (isnan(newvalue) || isinf(newvalue)) {
        ValkeyModule_ReplyWithError(ctx, TAIRDOC_ERROR_INCR_OVERFLOW);
        return VALKEYMODULE_ERR;
    }

    cJSON_SetNumberHelper(pnode, newvalue);
    char *print = cJSON_PrintUnformatted(pnode);
    ValkeyModule_ReplyWithStringBuffer(ctx, print, strlen(print));
    ValkeyModule_Free(print);

    ValkeyModule_ReplicateVerbatim(ctx);
    return VALKEYMODULE_OK;
}

/**
 * JSON.INCRBY <key> [path] <value>
 * value range: [-2^53, 2^53] [-9007199254740992, 9007199254740992]
 * Increments the value stored under `path` by `value`.
 * `path` must exist path and must be a number value.
 * Reply: int number, specifically the resulting.
 */
int TairDocIncrBy_ValkeyCommand(ValkeyModuleCtx *ctx, ValkeyModuleString **argv, int argc) {
    if (argc < 3) {
        ValkeyModule_WrongArity(ctx);
        return VALKEYMODULE_ERR;
    }
    ValkeyModule_AutoMemory(ctx);

    long long incr = 0;
    if (VALKEYMODULE_OK != ValkeyModule_StringToLongLong(argc == 4 ? argv[3] : argv[2], &incr)) {
        ValkeyModule_ReplyWithError(ctx, TAIRDOC_VALUE_OUTOF_RANGE);
        return VALKEYMODULE_ERR;
    }

    return incrGenericCommand(ctx, argv, argc, (double) incr);
}

/**
 * JSON.INCRBYFLOAT <key> [path] <value>
 * value range: double
 * Increments the value stored under `path` by `value`.
 * `path` must exist path and must be a number value.
 * Reply: String, specifically the resulting JSON number value
 */
int TairDocIncrByFloat_ValkeyCommand(ValkeyModuleCtx *ctx, ValkeyModuleString **argv, int argc) {
    if (argc < 3) {
        ValkeyModule_WrongArity(ctx);
        return VALKEYMODULE_ERR;
    }
    ValkeyModule_AutoMemory(ctx);

    double incr = 0.0;
    if (VALKEYMODULE_OK != ValkeyModule_StringToDouble(argc == 4 ? argv[3] : argv[2], &incr)) {
        ValkeyModule_ReplyWithError(ctx, TAIRDOC_VALUE_OUTOF_RANGE);
        return VALKEYMODULE_ERR;
    }

    return incrGenericCommand(ctx, argv, argc, incr);
}

/**
 * JSON.STRAPPEND <key> [path] <json-string>
 * Append the `json-string` value(s) the string at `path`.
 * `path` defaults to root if not provided.
 * Reply: Integer, -1 : key not exists, other: specifically the string's new length.
 */
int TairDocStrAppend_ValkeyCommand(ValkeyModuleCtx *ctx, ValkeyModuleString **argv, int argc) {
    if (argc < 3) {
        ValkeyModule_WrongArity(ctx);
        return VALKEYMODULE_ERR;
    }
    ValkeyModule_AutoMemory(ctx);

    int type;
    ValkeyModuleString *appendArg = NULL;
    char *pointer = NULL, *appendStr = NULL;
    cJSON *root = NULL, *pnode = NULL;
    size_t oldlen, appendlen, newlen;

    ValkeyModuleKey *key = ValkeyModule_OpenKey(ctx, argv[1], VALKEYMODULE_READ | VALKEYMODULE_WRITE);
    type = ValkeyModule_KeyType(key);
    if (VALKEYMODULE_KEYTYPE_EMPTY == type) {
        ValkeyModule_ReplyWithLongLong(ctx, -1);
        return VALKEYMODULE_ERR;
    } else {
        if (ValkeyModule_ModuleTypeGetType(key) != TairDocType) {
            ValkeyModule_ReplyWithError(ctx, VALKEYMODULE_ERRORMSG_WRONGTYPE);
            return VALKEYMODULE_ERR;
        }
        root = ValkeyModule_ModuleTypeGetValue(key);
    }

    pointer = argc == 4 ? (char *) ValkeyModule_StringPtrLen(argv[2], NULL) : "";
    ValkeyModuleString *rpointer = NULL;
    PATH_TO_POINTER(ctx, pointer, rpointer)

    pnode = cJSONUtils_GetPointerCaseSensitive(root, ValkeyModule_StringPtrLen(rpointer, NULL));
    if (pnode == NULL || !cJSON_IsString(pnode)) {
        ValkeyModule_ReplyWithError(ctx, TAIRDOC_ERROR_NOT_STRING);
        return VALKEYMODULE_ERR;
    }

    appendArg = argc == 4 ? argv[3] : argv[2];
    oldlen = strlen(pnode->valuestring);
    appendStr = (char *) ValkeyModule_StringPtrLen(appendArg, &appendlen);
    if (appendlen == 0) {
        ValkeyModule_ReplyWithLongLong(ctx, (long) oldlen);
    } else {
        newlen = oldlen + appendlen;
        pnode->valuestring = ValkeyModule_Realloc(pnode->valuestring, newlen + 1);
        memcpy(pnode->valuestring + oldlen, appendStr, appendlen);
        pnode->valuestring[newlen] = '\0';
        ValkeyModule_ReplyWithLongLong(ctx, (long) newlen);
    }

    ValkeyModule_ReplicateVerbatim(ctx);
    return VALKEYMODULE_OK;
}

/**
 * JSON.STRLEN <key> [path]
 * Report the length of the JSON value at `path` in `key`.
 *
 * `path` defaults to root if not provided. If the `key` or `path` do not exist, null is returned.
 *
 * Reply: Integer, specifically the length of the value.
 */
int TairDocStrLen_ValkeyCommand(ValkeyModuleCtx *ctx, ValkeyModuleString **argv, int argc) {
    if (argc < 2) {
        ValkeyModule_WrongArity(ctx);
        return VALKEYMODULE_ERR;
    }
    ValkeyModule_AutoMemory(ctx);

    int type = 0;
    char *pointer = NULL;
    cJSON *root = NULL, *pnode = NULL;

    ValkeyModuleKey *key = ValkeyModule_OpenKey(ctx, argv[1], VALKEYMODULE_READ);
    type = ValkeyModule_KeyType(key);
    if (VALKEYMODULE_KEYTYPE_EMPTY == type) {
        ValkeyModule_ReplyWithLongLong(ctx, -1);
        return VALKEYMODULE_ERR;
    } else {
        if (ValkeyModule_ModuleTypeGetType(key) != TairDocType) {
            ValkeyModule_ReplyWithError(ctx, VALKEYMODULE_ERRORMSG_WRONGTYPE);
            return VALKEYMODULE_ERR;
        }
        root = ValkeyModule_ModuleTypeGetValue(key);
    }

    pointer = argc == 3 ? (char *) ValkeyModule_StringPtrLen(argv[2], NULL) : "";
    ValkeyModuleString *rpointer = NULL;
    PATH_TO_POINTER(ctx, pointer, rpointer)

    pnode = cJSONUtils_GetPointerCaseSensitive(root, ValkeyModule_StringPtrLen(rpointer, NULL));
    if (pnode == NULL || !cJSON_IsString(pnode)) {
        ValkeyModule_ReplyWithError(ctx, TAIRDOC_ERROR_NOT_STRING);
        return VALKEYMODULE_ERR;
    }

    ValkeyModule_ReplyWithLongLong(ctx, (long) strlen(pnode->valuestring));
    return VALKEYMODULE_OK;
}

/**
 * JSON.ARRPUSH <key> <path> <json> [<json> ...]
 * Append the `json` value(s) into the array at `path` after the last element in it.
 * Reply: Integer, specifically the array's new size
 */
int TairDocArrPush_ValkeyCommand(ValkeyModuleCtx *ctx, ValkeyModuleString **argv, int argc) {
    if (argc < 4) {
        ValkeyModule_WrongArity(ctx);
        return VALKEYMODULE_ERR;
    }
    ValkeyModule_AutoMemory(ctx);

    int i, type;
    ValkeyModuleString *jerr = NULL;
    char *pointer = NULL;
    cJSON *root = NULL, *pnode = NULL, *node = NULL;

    ValkeyModuleKey *key = ValkeyModule_OpenKey(ctx, argv[1], VALKEYMODULE_READ | VALKEYMODULE_WRITE);
    type = ValkeyModule_KeyType(key);
    if (VALKEYMODULE_KEYTYPE_EMPTY == type) {
        ValkeyModule_ReplyWithLongLong(ctx, -1);
        return VALKEYMODULE_ERR;
    } else {
        if (ValkeyModule_ModuleTypeGetType(key) != TairDocType) {
            ValkeyModule_ReplyWithError(ctx, VALKEYMODULE_ERRORMSG_WRONGTYPE);
            return VALKEYMODULE_ERR;
        }
        root = ValkeyModule_ModuleTypeGetValue(key);
    }

    pointer = (char *) ValkeyModule_StringPtrLen(argv[2], NULL);
    ValkeyModuleString *rpointer = NULL;
    PATH_TO_POINTER(ctx, pointer, rpointer)

    pnode = cJSONUtils_GetPointerCaseSensitive(root, ValkeyModule_StringPtrLen(rpointer, NULL));
    if (pnode == NULL || !cJSON_IsArray(pnode)) {
        ValkeyModule_ReplyWithError(ctx, TAIRDOC_ERROR_NOT_ARRAY);
        return VALKEYMODULE_ERR;
    }

    for (i = 4; i <= argc; ++i) {
        if (VALKEYMODULE_OK != createNodeFromJson(&node, ValkeyModule_StringPtrLen(argv[i - 1], NULL), &jerr)) {
            // jerr will be free in addReplyErrorSds
            ValkeyModule_ReplyWithError(ctx, ValkeyModule_StringPtrLen(jerr, NULL));
            ValkeyModule_FreeString(NULL, jerr);
            goto error;
        }

        cJSON_AddItemToArray(pnode, node);
    }

    ValkeyModule_ReplyWithLongLong(ctx, cJSON_GetArraySize(pnode));
    ValkeyModule_ReplicateVerbatim(ctx);
    return VALKEYMODULE_OK;

    error:
    if (node) cJSON_Delete(node);
    return VALKEYMODULE_ERR;
}

/**
 * JSON.ARRPOP <key> <path> [index]
 * Remove and return element from the index in the array.
 *
 * `path` the array pointer. `index` is the position in the array to start
 * popping from (defaults to -1, meaning the last element). Out of range indices are rounded to
 * their respective array ends. Popping an empty array yields null.
 *
 * Reply: Bulk String, specifically the popped JSON value.
 */
int TairDocArrPop_ValkeyCommand(ValkeyModuleCtx *ctx, ValkeyModuleString **argv, int argc) {
    if (argc < 3) {
        ValkeyModule_WrongArity(ctx);
        return VALKEYMODULE_ERR;
    }
    ValkeyModule_AutoMemory(ctx);

    int type = 0;
    char *pointer = NULL;
    long long index, arrlen;
    cJSON *root = NULL, *pnode = NULL, *node = NULL;

    ValkeyModuleKey *key = ValkeyModule_OpenKey(ctx, argv[1], VALKEYMODULE_READ | VALKEYMODULE_WRITE);
    type = ValkeyModule_KeyType(key);
    if (VALKEYMODULE_KEYTYPE_EMPTY == type) {
        ValkeyModule_ReplyWithError(ctx, TAIRDOC_NO_SUCKKEY_ERROR);
        return VALKEYMODULE_ERR;
    } else {
        if (ValkeyModule_ModuleTypeGetType(key) != TairDocType) {
            ValkeyModule_ReplyWithError(ctx, VALKEYMODULE_ERRORMSG_WRONGTYPE);
            return VALKEYMODULE_ERR;
        }
        root = ValkeyModule_ModuleTypeGetValue(key);
    }

    pointer = (char *) ValkeyModule_StringPtrLen(argv[2], NULL);
    ValkeyModuleString *rpointer = NULL;
    PATH_TO_POINTER(ctx, pointer, rpointer)

    pnode = cJSONUtils_GetPointerCaseSensitive(root, ValkeyModule_StringPtrLen(rpointer, NULL));
    if (pnode == NULL || !cJSON_IsArray(pnode)) {
        ValkeyModule_ReplyWithError(ctx, TAIRDOC_ERROR_NOT_ARRAY);
        return VALKEYMODULE_ERR;
    }

    index = -1;
    arrlen = cJSON_GetArraySize(pnode);
    if (argc > 3 && ValkeyModule_StringToLongLong(argv[3], &index) != VALKEYMODULE_OK) {
        ValkeyModule_ReplyWithError(ctx, TAIRDOC_VALUE_OUTOF_RANGE);
        return VALKEYMODULE_ERR;
    }

    if (index < 0) index = index + arrlen;
    if (index < 0 || index >= arrlen) {
        ValkeyModule_ReplyWithError(ctx, TAIRDOC_ERROR_ARRAY_OUTFLOW);
        return VALKEYMODULE_ERR;
    }

    node = cJSON_DetachItemFromArray(pnode, (int) index);
    if (node != NULL && jsonNodeType(node->type) != NULL) {
        char *print = cJSON_PrintUnformatted(node);
        ValkeyModule_ReplyWithStringBuffer(ctx, print, strlen(print));
        ValkeyModule_Free(print);
        cJSON_Delete(node);

        if (!root->next && !root->prev && !root->child) {
            ValkeyModule_DeleteKey(key);
        }
        ValkeyModule_ReplicateVerbatim(ctx);
    } else {
        ValkeyModule_ReplyWithError(ctx, TAIRDOC_ERROR_JSON_TYPE_ERROR);
        ValkeyModule_Log(ctx, "warning", "%s", TAIRDOC_ERROR_JSON_TYPE_ERROR);
        return VALKEYMODULE_ERR;
    }

    return VALKEYMODULE_OK;
}

/**
 * JSON.ARRINSERT <key> <path> <index> <json> [<json> ...]
 * Insert the `json` value(s) into the array at `path` before the `index` (shifts to the right).
 *
 * The index must be in the array's range. Inserting at `index` 0 prepends to the array.
 * Negative index values are interpreted as starting from the end.
 *
 * Reply: Integer, specifically the array's new size
 */
int TairDocArrInsert_ValkeyCommand(ValkeyModuleCtx *ctx, ValkeyModuleString **argv, int argc) {
    if (argc < 5) {
        ValkeyModule_WrongArity(ctx);
        return VALKEYMODULE_ERR;
    }
    ValkeyModule_AutoMemory(ctx);

    int i, type;
    ValkeyModuleString *jerr = NULL;
    char *pointer = NULL;
    long long index, arrlen;
    cJSON *root = NULL, *pnode = NULL, *node = NULL;

    ValkeyModuleKey *key = ValkeyModule_OpenKey(ctx, argv[1], VALKEYMODULE_READ | VALKEYMODULE_WRITE);
    type = ValkeyModule_KeyType(key);
    if (VALKEYMODULE_KEYTYPE_EMPTY == type) {
        ValkeyModule_ReplyWithLongLong(ctx, -1);
        return VALKEYMODULE_ERR;
    } else {
        if (ValkeyModule_ModuleTypeGetType(key) != TairDocType) {
            ValkeyModule_ReplyWithError(ctx, VALKEYMODULE_ERRORMSG_WRONGTYPE);
            return VALKEYMODULE_ERR;
        }
        root = ValkeyModule_ModuleTypeGetValue(key);
    }

    pointer = (char *) ValkeyModule_StringPtrLen(argv[2], NULL);
    ValkeyModuleString *rpointer = NULL;
    PATH_TO_POINTER(ctx, pointer, rpointer)

    pnode = cJSONUtils_GetPointerCaseSensitive(root, ValkeyModule_StringPtrLen(rpointer, NULL));
    if (pnode == NULL || !cJSON_IsArray(pnode)) {
        ValkeyModule_ReplyWithError(ctx, TAIRDOC_ERROR_NOT_ARRAY);
        return VALKEYMODULE_ERR;
    }

    index = -1;
    arrlen = cJSON_GetArraySize(pnode);
    if (ValkeyModule_StringToLongLong(argv[3], &index) != VALKEYMODULE_OK) {
        ValkeyModule_ReplyWithError(ctx, TAIRDOC_VALUE_OUTOF_RANGE);
        return VALKEYMODULE_ERR;
    }

    if (index < 0) index = index + arrlen;
    if (index < 0 || index > arrlen) {
        ValkeyModule_ReplyWithError(ctx, TAIRDOC_ERROR_ARRAY_OUTFLOW);
        return VALKEYMODULE_ERR;
    }

    for (i = 5; i <= argc; ++i) {
        if (VALKEYMODULE_OK != createNodeFromJson(&node, ValkeyModule_StringPtrLen(argv[i - 1], NULL), &jerr)) {
            ValkeyModule_ReplyWithError(ctx, ValkeyModule_StringPtrLen(jerr, NULL));
            ValkeyModule_FreeString(NULL, jerr);
            goto error;
        }

        cJSON_InsertItemInArray(pnode, (int) index, node);
        index++;
    }

    ValkeyModule_ReplyWithLongLong(ctx, cJSON_GetArraySize(pnode));
    ValkeyModule_ReplicateVerbatim(ctx);
    return VALKEYMODULE_OK;

    error:
    if (node) cJSON_Delete(node);
    return VALKEYMODULE_ERR;
}

/**
 * JSON.ARRLEN <key> [path]
 * Report the length of the array at `path` in `key`.
 *
 * `path` defaults to root if not provided. If the `key` or `path` do not exist, null is returned.
 *
 * Reply: Integer, specifically the length of the array.
 */
int TairDocArrLen_ValkeyCommand(ValkeyModuleCtx *ctx, ValkeyModuleString **argv, int argc) {
    if (argc < 2) {
        ValkeyModule_WrongArity(ctx);
        return VALKEYMODULE_ERR;
    }
    ValkeyModule_AutoMemory(ctx);

    char *pointer = NULL;
    cJSON *root = NULL, *pnode = NULL;

    ValkeyModuleKey *key = ValkeyModule_OpenKey(ctx, argv[1], VALKEYMODULE_READ);
    int type = ValkeyModule_KeyType(key);
    if (VALKEYMODULE_KEYTYPE_EMPTY == type) {
        ValkeyModule_ReplyWithLongLong(ctx, -1);
        return VALKEYMODULE_ERR;
    } else {
        if (ValkeyModule_ModuleTypeGetType(key) != TairDocType) {
            ValkeyModule_ReplyWithError(ctx, VALKEYMODULE_ERRORMSG_WRONGTYPE);
            return VALKEYMODULE_ERR;
        }
        root = ValkeyModule_ModuleTypeGetValue(key);
    }

    pointer = argc == 3 ? (char *) ValkeyModule_StringPtrLen(argv[2], NULL) : "";
    ValkeyModuleString *rpointer = NULL;
    PATH_TO_POINTER(ctx, pointer, rpointer)

    pnode = cJSONUtils_GetPointerCaseSensitive(root, ValkeyModule_StringPtrLen(rpointer, NULL));
    if (pnode == NULL || !cJSON_IsArray(pnode)) {
        ValkeyModule_ReplyWithError(ctx, TAIRDOC_ERROR_NOT_ARRAY);
        return VALKEYMODULE_ERR;
    }

    ValkeyModule_ReplyWithLongLong(ctx, cJSON_GetArraySize(pnode));
    return VALKEYMODULE_OK;
}

/**
 * JSON.ARRTRIM <key> <path> <start> <stop>
 * Trim an array so that it contains only the specified inclusive range of elements.
 *
 * Reply: Integer, specifically the array's new size.
 */
int TairDocArrTrim_ValkeyCommand(ValkeyModuleCtx *ctx, ValkeyModuleString **argv, int argc) {
    if (argc != 5) {
        ValkeyModule_WrongArity(ctx);
        return VALKEYMODULE_ERR;
    }
    ValkeyModule_AutoMemory(ctx);

    char *pointer = NULL;
    long long i, start, stop, arrlen, index;
    cJSON *root = NULL, *pnode = NULL;

    ValkeyModuleKey *key = ValkeyModule_OpenKey(ctx, argv[1], VALKEYMODULE_READ | VALKEYMODULE_WRITE);
    int type = ValkeyModule_KeyType(key);
    if (VALKEYMODULE_KEYTYPE_EMPTY == type) {
        ValkeyModule_ReplyWithLongLong(ctx, -1);
        return VALKEYMODULE_ERR;
    } else {
        if (ValkeyModule_ModuleTypeGetType(key) != TairDocType) {
            ValkeyModule_ReplyWithError(ctx, VALKEYMODULE_ERRORMSG_WRONGTYPE);
            return VALKEYMODULE_ERR;
        }
        root = ValkeyModule_ModuleTypeGetValue(key);
    }

    pointer = (char *) ValkeyModule_StringPtrLen(argv[2], NULL);
    ValkeyModuleString *rpointer = NULL;
    PATH_TO_POINTER(ctx, pointer, rpointer)

    pnode = cJSONUtils_GetPointerCaseSensitive(root, ValkeyModule_StringPtrLen(rpointer, NULL));
    if (pnode == NULL || !cJSON_IsArray(pnode)) {
        ValkeyModule_ReplyWithError(ctx, TAIRDOC_ERROR_NOT_ARRAY);
        return VALKEYMODULE_ERR;
    }

    if (ValkeyModule_StringToLongLong(argv[3], &start) != VALKEYMODULE_OK ||
        ValkeyModule_StringToLongLong(argv[4], &stop) != VALKEYMODULE_OK) {
        ValkeyModule_ReplyWithError(ctx, TAIRDOC_VALUE_OUTOF_RANGE);
        return VALKEYMODULE_ERR;
    }

    arrlen = cJSON_GetArraySize(pnode);
    if (start < 0 || stop < 0 || start > stop || start >= arrlen || stop >= arrlen) {
        ValkeyModule_ReplyWithError(ctx, TAIRDOC_ERROR_ARRAY_OUTFLOW);
        return VALKEYMODULE_ERR;
    }

    index = 0;
    for (i = index; i < start; ++i) {
        cJSON_DeleteItemFromArray(pnode, (int) index);
    }

    arrlen -= start;
    index = stop - start + 1;
    for (i = index; i < arrlen; ++i) {
        cJSON_DeleteItemFromArray(pnode, (int) index);
    }

    if (!root->next && !root->prev && !root->child) {
        ValkeyModule_DeleteKey(key);
    }

    ValkeyModule_ReplyWithLongLong(ctx, cJSON_GetArraySize(pnode));
    ValkeyModule_ReplicateVerbatim(ctx);
    return VALKEYMODULE_OK;
}

/**
 * JSON.MGET <key> [<key> ...] <path>
 * Returns the values at `path` from multiple `key`s. Non-existing keys and non-existing paths
 * are reported as null.
 * Reply: Array of Bulk Strings, specifically the JSON serialization of
 * the value at each key's path.
 */
int TairDocMget_ValkeyCommand(ValkeyModuleCtx *ctx, ValkeyModuleString **argv, int argc) {
    if (argc < 3) {
        ValkeyModule_WrongArity(ctx);
        return VALKEYMODULE_ERR;
    }
    ValkeyModule_AutoMemory(ctx);

    int j;
    char *print = NULL;
    char *pointer = NULL;
    cJSON *root = NULL, *pnode = NULL;

    pointer = (char *) ValkeyModule_StringPtrLen(argv[argc - 1], NULL);
    ValkeyModuleString *rpointer = NULL;
    PATH_TO_POINTER(ctx, pointer, rpointer)

    ValkeyModule_ReplyWithArray(ctx, argc - 2);
    for (j = 1; j < argc - 1; ++j) {
        ValkeyModuleKey *key = ValkeyModule_OpenKey(ctx, argv[j], VALKEYMODULE_READ);
        int type = ValkeyModule_KeyType(key);
        if (VALKEYMODULE_KEYTYPE_EMPTY == type) {
            ValkeyModule_ReplyWithNull(ctx);
        } else {
            if (ValkeyModule_ModuleTypeGetType(key) != TairDocType) {
                ValkeyModule_ReplyWithNull(ctx);
            } else {
                root = ValkeyModule_ModuleTypeGetValue(key);
                pnode = cJSONUtils_GetPointerCaseSensitive(root, ValkeyModule_StringPtrLen(rpointer, NULL));
                if (pnode == NULL || jsonNodeType(pnode->type) == NULL) {
                    ValkeyModule_ReplyWithNull(ctx);
                    continue;
                }
                print = cJSON_PrintUnformatted(pnode);
                ValkeyModule_ReplyWithStringBuffer(ctx, print, strlen(print));
                ValkeyModule_Free(print);
            }
        }
    }
    return VALKEYMODULE_OK;
}

/* ========================== TairDoc type methods ======================= */

void *TairDocTypeRdbLoad(ValkeyModuleIO *rdb, int encver) {
    if (encver != TAIRDOC_ENC_VER) {
        return NULL;
    }

    cJSON *root = NULL;
    char *json = NULL;
    size_t len = 0;

    json = ValkeyModule_LoadStringBuffer(rdb, &len);
    if (json != NULL && len != 0) {
        root = cJSON_Parse((const char *) json);
        ValkeyModule_Free(json);
        return root;
    } else {
        ValkeyModule_LogIOError(
                rdb, "warning",
                "TairDocTypeRdbLoad load json return NULL, TAIRDOC_ENC_VER: %d", TAIRDOC_ENC_VER);
    }
    return NULL;
}

void TairDocTypeRdbSave(ValkeyModuleIO *rdb, void *value) {
    cJSON *root = value;
    char *serialize = cJSON_PrintUnformatted(root);
    if (serialize != NULL) {
        ValkeyModule_SaveStringBuffer(rdb, serialize, strlen(serialize) + 1);
        ValkeyModule_Free(serialize);
    } else {
        ValkeyModule_LogIOError(
                rdb, "warning",
                "TairDocTypeRdbSave serialize json return NULL, TAIRDOC_ENC_VER: %d", TAIRDOC_ENC_VER);
    }
}

void TairDocTypeAofRewrite(ValkeyModuleIO *aof, ValkeyModuleString *key, void *value) {
    cJSON *root = value;
    if (root != NULL) {
        char *serialize = cJSON_PrintUnformatted(root);
        ValkeyModule_EmitAOF(aof, "JSON.SET", "scc", key, "", serialize);
        ValkeyModule_Free(serialize);
    }
}

size_t TairDocTypeMemUsage(const void *value) {
    VALKEYMODULE_NOT_USED(value);
    return 0;
}

void TairDocTypeFree(void *value) {
    cJSON *root = value;
    cJSON_Delete(root);
}

void TairDocTypeDigest(ValkeyModuleDigest *md, void *value) {
    VALKEYMODULE_NOT_USED(md);
    VALKEYMODULE_NOT_USED(value);
}

static size_t TairDocTypeFreeEffort(ValkeyModuleString * key, const void *value) {
    VALKEYMODULE_NOT_USED(key);
    VALKEYMODULE_NOT_USED(value);
    return 0;
}

int Module_CreateCommands(ValkeyModuleCtx *ctx) {

#define CREATE_CMD(name, tgt, attr)                                                                \
    do {                                                                                           \
        if (ValkeyModule_CreateCommand(ctx, name, tgt, attr, 1, 1, 1) != VALKEYMODULE_OK) {          \
            return VALKEYMODULE_ERR;                                                                \
        }                                                                                          \
    } while (0);
#define CREATE_WRCMD(name, tgt) CREATE_CMD(name, tgt, "write deny-oom")
#define CREATE_ROCMD(name, tgt) CREATE_CMD(name, tgt, "readonly fast")

    CREATE_WRCMD("json.set", TairDocSet_ValkeyCommand)
    CREATE_ROCMD("json.get", TairDocGet_ValkeyCommand)
    CREATE_ROCMD("json.type", TairDocType_ValkeyCommand)
    CREATE_WRCMD("json.del", TairDocDel_ValkeyCommand)
    CREATE_WRCMD("json.forget", TairDocDel_ValkeyCommand)
    CREATE_WRCMD("json.incrby", TairDocIncrBy_ValkeyCommand)
    CREATE_WRCMD("json.incrbyfloat", TairDocIncrByFloat_ValkeyCommand)
    CREATE_WRCMD("json.numincrby", TairDocIncrByFloat_ValkeyCommand)
    CREATE_ROCMD("json.strlen", TairDocStrLen_ValkeyCommand)
    CREATE_WRCMD("json.strappend", TairDocStrAppend_ValkeyCommand)
    CREATE_ROCMD("json.arrlen", TairDocArrLen_ValkeyCommand)
    CREATE_WRCMD("json.arrinsert", TairDocArrInsert_ValkeyCommand)
    CREATE_WRCMD("json.arrpush", TairDocArrPush_ValkeyCommand)
    CREATE_WRCMD("json.arrappend", TairDocArrPush_ValkeyCommand)
    CREATE_WRCMD("json.arrpop", TairDocArrPop_ValkeyCommand)
    CREATE_WRCMD("json.arrtrim", TairDocArrTrim_ValkeyCommand)

    // JSON.MGET is a multi-key command
    if (ValkeyModule_CreateCommand(ctx, "json.mget", TairDocMget_ValkeyCommand, "readonly",
                                   1, -2, 1) != VALKEYMODULE_OK) {
        return VALKEYMODULE_ERR;
    }
    return VALKEYMODULE_OK;
}


/* This function must be present on each Valkey module. It is used in order to
 * register the commands into the Valkey server. */
int ValkeyModule_OnLoad(ValkeyModuleCtx *ctx, ValkeyModuleString **argv, int argc) {
    VALKEYMODULE_NOT_USED(argv);
    VALKEYMODULE_NOT_USED(argc);

    if (ValkeyModule_Init(ctx, "tair-json", 1, VALKEYMODULE_APIVER_1)
        == VALKEYMODULE_ERR)
        return VALKEYMODULE_ERR;

    ValkeyModuleTypeMethods tm = {
            .version = VALKEYMODULE_TYPE_METHOD_VERSION,
            .rdb_load = TairDocTypeRdbLoad,
            .rdb_save = TairDocTypeRdbSave,
            .aof_rewrite = TairDocTypeAofRewrite,
            .mem_usage = TairDocTypeMemUsage,
            .free = TairDocTypeFree,
            .digest = TairDocTypeDigest,
            .free_effort = TairDocTypeFreeEffort,
    };
    TairDocType = ValkeyModule_CreateDataType(ctx, "tair-json", 0, &tm);
    if (TairDocType == NULL) return VALKEYMODULE_ERR;

    // Init cJSON_Hooks
    cJSON_Hooks TairDoc_hooks = {
            ValkeyModule_Alloc,
            ValkeyModule_Free,
            ValkeyModule_Realloc,
    };
    cJSON_InitHooks(&TairDoc_hooks);

    // Create Commands
    if (VALKEYMODULE_ERR == Module_CreateCommands(ctx)) return VALKEYMODULE_ERR;

    return VALKEYMODULE_OK;
}
