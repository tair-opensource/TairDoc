/*
  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

/* disable warnings about old C89 functions in MSVC */
#if !defined(_CRT_SECURE_NO_DEPRECATE) && defined(_MSC_VER)
#define _CRT_SECURE_NO_DEPRECATE
#endif

#ifdef __GNUCC__
#pragma GCC visibility push(default)
#endif
#if defined(_MSC_VER)
#pragma warning (push)
/* disable warning about single line comments in system headers */
#pragma warning (disable : 4001)
#endif

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <float.h>
#include <math.h>

#if defined(_MSC_VER)
#pragma warning (pop)
#endif
#ifdef __GNUCC__
#pragma GCC visibility pop
#endif

#include "cJSON_Utils.h"

/* define our own boolean type */
#ifdef true
#undef true
#endif
#define true ((cJSON_bool)1)

#ifdef false
#undef false
#endif
#define false ((cJSON_bool)0)

static unsigned char* cJSONUtils_strdup(const unsigned char* const string)
{
    size_t length = 0;
    unsigned char *copy = NULL;

    length = strlen((const char*)string) + sizeof("");
    copy = (unsigned char*) cJSON_malloc(length);
    if (copy == NULL)
    {
        return NULL;
    }
    memcpy(copy, string, length);

    return copy;
}

/* string comparison which doesn't consider NULL pointers equal */
static int compare_strings(const unsigned char *string1, const unsigned char *string2, const cJSON_bool case_sensitive)
{
    if ((string1 == NULL) || (string2 == NULL))
    {
        return 1;
    }

    if (string1 == string2)
    {
        return 0;
    }

    if (case_sensitive)
    {
        return strcmp((const char*)string1, (const char*)string2);
    }

    for(; tolower(*string1) == tolower(*string2); (void)string1++, string2++)
    {
        if (*string1 == '\0')
        {
            return 0;
        }
    }

    return tolower(*string1) - tolower(*string2);
}

/* securely comparison of floating-point variables */
static cJSON_bool compare_double(double a, double b)
{
    double maxVal = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
    return (fabs(a - b) <= maxVal * DBL_EPSILON);
}


/* Compare the next path element of two JSON pointers, two NULL pointers are considered unequal: */
static cJSON_bool compare_pointers(const unsigned char *name, const unsigned char *pointer, const cJSON_bool case_sensitive)
{
    if ((name == NULL) || (pointer == NULL))
    {
        return false;
    }

    for (; (*name != '\0') && (*pointer != '\0') && (*pointer != '/'); (void)name++, pointer++) /* compare until next '/' */
    {
        if (*pointer == '~')
        {
            /* check for escaped '~' (~0) and '/' (~1) */
            if (((pointer[1] != '0') || (*name != '~')) && ((pointer[1] != '1') || (*name != '/')))
            {
                /* invalid escape sequence or wrong character in *name */
                return false;
            }
            else
            {
                pointer++;
            }
        }
        else if ((!case_sensitive && (tolower(*name) != tolower(*pointer))) || (case_sensitive && (*name != *pointer)))
        {
            return false;
        }
    }
    if (((*pointer != 0) && (*pointer != '/')) != (*name != 0))
    {
        /* one string has ended, the other not */
        return false;;
    }

    return true;
}

/* calculate the length of a string if encoded as JSON pointer with ~0 and ~1 escape sequences */
static size_t pointer_encoded_length(const unsigned char *string)
{
    size_t length;
    for (length = 0; *string != '\0'; (void)string++, length++)
    {
        /* character needs to be escaped? */
        if ((*string == '~') || (*string == '/'))
        {
            length++;
        }
    }

    return length;
}

/* copy a string while escaping '~' and '/' with ~0 and ~1 JSON pointer escape codes */
static void encode_string_as_pointer(unsigned char *destination, const unsigned char *source)
{
    for (; source[0] != '\0'; (void)source++, destination++)
    {
        if (source[0] == '/')
        {
            destination[0] = '~';
            destination[1] = '1';
            destination++;
        }
        else if (source[0] == '~')
        {
            destination[0] = '~';
            destination[1] = '0';
            destination++;
        }
        else
        {
            destination[0] = source[0];
        }
    }

    destination[0] = '\0';
}

CJSON_PUBLIC(char *) cJSONUtils_FindPointerFromObjectTo(const cJSON * const object, const cJSON * const target)
{
    size_t child_index = 0;
    cJSON *current_child = 0;

    if ((object == NULL) || (target == NULL))
    {
        return NULL;
    }

    if (object == target)
    {
        /* found */
        return (char*)cJSONUtils_strdup((const unsigned char*)"");
    }

    /* recursively search all children of the object or array */
    for (current_child = object->child; current_child != NULL; (void)(current_child = current_child->next), child_index++)
    {
        unsigned char *target_pointer = (unsigned char*)cJSONUtils_FindPointerFromObjectTo(current_child, target);
        /* found the target? */
        if (target_pointer != NULL)
        {
            if (cJSON_IsArray(object))
            {
                /* reserve enough memory for a 64 bit integer + '/' and '\0' */
                unsigned char *full_pointer = (unsigned char*)cJSON_malloc(strlen((char*)target_pointer) + 20 + sizeof("/"));
                /* check if conversion to unsigned long is valid
                 * This should be eliminated at compile time by dead code elimination
                 * if size_t is an alias of unsigned long, or if it is bigger */
                if (child_index > ULONG_MAX)
                {
                    cJSON_free(target_pointer);
                    cJSON_free(full_pointer);
                    return NULL;
                }
                sprintf((char*)full_pointer, "/%lu%s", (unsigned long)child_index, target_pointer); /* /<array_index><path> */
                cJSON_free(target_pointer);

                return (char*)full_pointer;
            }

            if (cJSON_IsObject(object))
            {
                unsigned char *full_pointer = (unsigned char*)cJSON_malloc(strlen((char*)target_pointer) + pointer_encoded_length((unsigned char*)current_child->string) + 2);
                full_pointer[0] = '/';
                encode_string_as_pointer(full_pointer + 1, (unsigned char*)current_child->string);
                strcat((char*)full_pointer, (char*)target_pointer);
                cJSON_free(target_pointer);

                return (char*)full_pointer;
            }

            /* reached leaf of the tree, found nothing */
            cJSON_free(target_pointer);
            return NULL;
        }
    }

    /* not found */
    return NULL;
}

/* non broken version of cJSON_GetArrayItem */
static cJSON *get_array_item(const cJSON *array, size_t item)
{
    cJSON *child = array ? array->child : NULL;
    while ((child != NULL) && (item > 0))
    {
        item--;
        child = child->next;
    }

    return child;
}

static cJSON_bool decode_array_index_from_pointer(const unsigned char * const pointer, size_t * const index)
{
    size_t parsed_index = 0;
    size_t position = 0;

    if ((pointer[0] == '0') && ((pointer[1] != '\0') && (pointer[1] != '/')))
    {
        /* leading zeroes are not permitted */
        return 0;
    }

    for (position = 0; (pointer[position] >= '0') && (pointer[0] <= '9'); position++)
    {
        parsed_index = (10 * parsed_index) + (size_t)(pointer[position] - '0');

    }

    if ((pointer[position] != '\0') && (pointer[position] != '/'))
    {
        return 0;
    }

    *index = parsed_index;

    return 1;
}

static cJSON *get_item_from_pointer(cJSON * const object, const char * pointer, const cJSON_bool case_sensitive)
{
    cJSON *current_element = object;

    if (pointer == NULL)
    {
        return NULL;
    }

    /* follow path of the pointer */
    while ((pointer[0] == '/') && (current_element != NULL))
    {
        pointer++;
        if (cJSON_IsArray(current_element))
        {
            size_t index = 0;
            if (!decode_array_index_from_pointer((const unsigned char*)pointer, &index))
            {
                return NULL;
            }

            current_element = get_array_item(current_element, index);
        }
        else if (cJSON_IsObject(current_element))
        {
            current_element = current_element->child;
            /* GetObjectItem. */
            while ((current_element != NULL) && !compare_pointers((unsigned char*)current_element->string, (const unsigned char*)pointer, case_sensitive))
            {
                current_element = current_element->next;
            }
        }
        else
        {
            return NULL;
        }

        /* skip to the next path token or end of string */
        while ((pointer[0] != '\0') && (pointer[0] != '/'))
        {
            pointer++;
        }
    }

    return current_element;
}

CJSON_PUBLIC(cJSON *) cJSONUtils_GetPointer(cJSON * const object, const char *pointer)
{
    return get_item_from_pointer(object, pointer, false);
}

CJSON_PUBLIC(cJSON *) cJSONUtils_GetPointerCaseSensitive(cJSON * const object, const char *pointer)
{
    return get_item_from_pointer(object, pointer, true);
}

/* JSON Patch implementation. */
static void decode_pointer_inplace(unsigned char *string)
{
    unsigned char *decoded_string = string;

    if (string == NULL) {
        return;
    }

    for (; *string; (void)decoded_string++, string++)
    {
        if (string[0] == '~')
        {
            if (string[1] == '0')
            {
                decoded_string[0] = '~';
            }
            else if (string[1] == '1')
            {
                decoded_string[1] = '/';
            }
            else
            {
                /* invalid escape sequence */
                return;
            }

            string++;
        }
    }

    decoded_string[0] = '\0';
}

/* non-broken cJSON_DetachItemFromArray */
static cJSON *detach_item_from_array(cJSON *array, size_t which)
{
    cJSON *c = array->child;
    while (c && (which > 0))
    {
        c = c->next;
        which--;
    }
    if (!c)
    {
        /* item doesn't exist */
        return NULL;
    }
    if (c != array->child)
    {
        /* not the first element */
        c->prev->next = c->next;
    }
    if (c->next)
    {
        c->next->prev = c->prev;
    }
    if (c == array->child)
    {
        array->child = c->next;
    }
    else if (c->next == NULL)
    {
        array->child->prev = c->prev;
    }
    /* make sure the detached item doesn't point anywhere anymore */
    c->prev = c->next = NULL;

    return c;
}

/* detach an item at the given path */
static cJSON *detach_path(cJSON *object, const unsigned char *path, const cJSON_bool case_sensitive)
{
    unsigned char *parent_pointer = NULL;
    unsigned char *child_pointer = NULL;
    cJSON *parent = NULL;
    cJSON *detached_item = NULL;

    /* copy path and split it in parent and child */
    parent_pointer = cJSONUtils_strdup(path);
    if (parent_pointer == NULL) {
        goto cleanup;
    }

    child_pointer = (unsigned char*)strrchr((char*)parent_pointer, '/'); /* last '/' */
    if (child_pointer == NULL)
    {
        goto cleanup;
    }
    /* split strings */
    child_pointer[0] = '\0';
    child_pointer++;

    parent = get_item_from_pointer(object, (char*)parent_pointer, case_sensitive);
    decode_pointer_inplace(child_pointer);

    if (cJSON_IsArray(parent))
    {
        size_t index = 0;
        if (!decode_array_index_from_pointer(child_pointer, &index))
        {
            goto cleanup;
        }
        detached_item = detach_item_from_array(parent, index);
    }
    else if (cJSON_IsObject(parent))
    {
        detached_item = cJSON_DetachItemFromObject(parent, (char*)child_pointer);
    }
    else
    {
        /* Couldn't find object to remove child from. */
        goto cleanup;
    }

cleanup:
    if (parent_pointer != NULL)
    {
        cJSON_free(parent_pointer);
    }

    return detached_item;
}

/* sort lists using mergesort */
static cJSON *sort_list(cJSON *list, const cJSON_bool case_sensitive)
{
    cJSON *first = list;
    cJSON *second = list;
    cJSON *current_item = list;
    cJSON *result = list;
    cJSON *result_tail = NULL;

    if ((list == NULL) || (list->next == NULL))
    {
        /* One entry is sorted already. */
        return result;
    }

    while ((current_item != NULL) && (current_item->next != NULL) && (compare_strings((unsigned char*)current_item->string, (unsigned char*)current_item->next->string, case_sensitive) < 0))
    {
        /* Test for list sorted. */
        current_item = current_item->next;
    }
    if ((current_item == NULL) || (current_item->next == NULL))
    {
        /* Leave sorted lists unmodified. */
        return result;
    }

    /* reset pointer to the beginning */
    current_item = list;
    while (current_item != NULL)
    {
        /* Walk two pointers to find the middle. */
        second = second->next;
        current_item = current_item->next;
        /* advances current_item two steps at a time */
        if (current_item != NULL)
        {
            current_item = current_item->next;
        }
    }
    if ((second != NULL) && (second->prev != NULL))
    {
        /* Split the lists */
        second->prev->next = NULL;
        second->prev = NULL;
    }

    /* Recursively sort the sub-lists. */
    first = sort_list(first, case_sensitive);
    second = sort_list(second, case_sensitive);
    result = NULL;

    /* Merge the sub-lists */
    while ((first != NULL) && (second != NULL))
    {
        cJSON *smaller = NULL;
        if (compare_strings((unsigned char*)first->string, (unsigned char*)second->string, case_sensitive) < 0)
        {
            smaller = first;
        }
        else
        {
            smaller = second;
        }

        if (result == NULL)
        {
            /* start merged list with the smaller element */
            result_tail = smaller;
            result = smaller;
        }
        else
        {
            /* add smaller element to the list */
            result_tail->next = smaller;
            smaller->prev = result_tail;
            result_tail = smaller;
        }

        if (first == smaller)
        {
            first = first->next;
        }
        else
        {
            second = second->next;
        }
    }

    if (first != NULL)
    {
        /* Append rest of first list. */
        if (result == NULL)
        {
            return first;
        }
        result_tail->next = first;
        first->prev = result_tail;
    }
    if (second != NULL)
    {
        /* Append rest of second list */
        if (result == NULL)
        {
            return second;
        }
        result_tail->next = second;
        second->prev = result_tail;
    }

    return result;
}

static void sort_object(cJSON * const object, const cJSON_bool case_sensitive)
{
    if (object == NULL)
    {
        return;
    }
    object->child = sort_list(object->child, case_sensitive);
}

static cJSON_bool compare_json(cJSON *a, cJSON *b, const cJSON_bool case_sensitive)
{
    if ((a == NULL) || (b == NULL) || ((a->type & 0xFF) != (b->type & 0xFF)))
    {
        /* mismatched type. */
        return false;
    }
    switch (a->type & 0xFF)
    {
        case cJSON_Number:
            /* numeric mismatch. */
            if ((a->valueint != b->valueint) || (!compare_double(a->valuedouble, b->valuedouble)))
            {
                return false;
            }
            else
            {
                return true;
            }

        case cJSON_String:
            /* string mismatch. */
            if (strcmp(a->valuestring, b->valuestring) != 0)
            {
                return false;
            }
            else
            {
                return true;
            }

        case cJSON_Array:
            for ((void)(a = a->child), b = b->child; (a != NULL) && (b != NULL); (void)(a = a->next), b = b->next)
            {
                cJSON_bool identical = compare_json(a, b, case_sensitive);
                if (!identical)
                {
                    return false;
                }
            }

            /* array size mismatch? (one of both children is not NULL) */
            if ((a != NULL) || (b != NULL))
            {
                return false;
            }
            else
            {
                return true;
            }

        case cJSON_Object:
            sort_object(a, case_sensitive);
            sort_object(b, case_sensitive);
            for ((void)(a = a->child), b = b->child; (a != NULL) && (b != NULL); (void)(a = a->next), b = b->next)
            {
                cJSON_bool identical = false;
                /* compare object keys */
                if (compare_strings((unsigned char*)a->string, (unsigned char*)b->string, case_sensitive))
                {
                    /* missing member */
                    return false;
                }
                identical = compare_json(a, b, case_sensitive);
                if (!identical)
                {
                    return false;
                }
            }

            /* object length mismatch (one of both children is not null) */
            if ((a != NULL) || (b != NULL))
            {
                return false;
            }
            else
            {
                return true;
            }

        default:
            break;
    }

    /* null, true or false */
    return true;
}

/* non broken version of cJSON_InsertItemInArray */
static cJSON_bool insert_item_in_array(cJSON *array, size_t which, cJSON *newitem)
{
    cJSON *child = array->child;
    while (child && (which > 0))
    {
        child = child->next;
        which--;
    }
    if (which > 0)
    {
        /* item is after the end of the array */
        return 0;
    }
    if (child == NULL)
    {
        cJSON_AddItemToArray(array, newitem);
        return 1;
    }

    /* insert into the linked list */
    newitem->next = child;
    newitem->prev = child->prev;
    child->prev = newitem;

    /* was it at the beginning */
    if (child == array->child)
    {
        array->child = newitem;
    }
    else
    {
        newitem->prev->next = newitem;
    }

    return 1;
}

static cJSON *get_object_item(const cJSON * const object, const char* name, const cJSON_bool case_sensitive)
{
    if (case_sensitive)
    {
        return cJSON_GetObjectItemCaseSensitive(object, name);
    }

    return cJSON_GetObjectItem(object, name);
}

enum patch_operation { INVALID, ADD, REMOVE, REPLACE, MOVE, COPY, TEST };

static enum patch_operation decode_patch_operation(const cJSON * const patch, const cJSON_bool case_sensitive)
{
    cJSON *operation = get_object_item(patch, "op", case_sensitive);
    if (!cJSON_IsString(operation))
    {
        return INVALID;
    }

    if (strcmp(operation->valuestring, "add") == 0)
    {
        return ADD;
    }

    if (strcmp(operation->valuestring, "remove") == 0)
    {
        return REMOVE;
    }

    if (strcmp(operation->valuestring, "replace") == 0)
    {
        return REPLACE;
    }

    if (strcmp(operation->valuestring, "move") == 0)
    {
        return MOVE;
    }

    if (strcmp(operation->valuestring, "copy") == 0)
    {
        return COPY;
    }

    if (strcmp(operation->valuestring, "test") == 0)
    {
        return TEST;
    }

    return INVALID;
}

/* overwrite and existing item with another one and free resources on the way */
static void overwrite_item(cJSON * const root, const cJSON replacement)
{
    if (root == NULL)
    {
        return;
    }

    if (root->string != NULL)
    {
        cJSON_free(root->string);
    }
    if (root->valuestring != NULL)
    {
        cJSON_free(root->valuestring);
    }
    if (root->child != NULL)
    {
        cJSON_Delete(root->child);
    }

    memcpy(root, &replacement, sizeof(cJSON));
}

static int apply_patch(cJSON *object, const cJSON *patch, const cJSON_bool case_sensitive)
{
    cJSON *path = NULL;
    cJSON *value = NULL;
    cJSON *parent = NULL;
    enum patch_operation opcode = INVALID;
    unsigned char *parent_pointer = NULL;
    unsigned char *child_pointer = NULL;
    int status = 0;

    path = get_object_item(patch, "path", case_sensitive);
    if (!cJSON_IsString(path))
    {
        /* malformed patch. */
        status = 2;
        goto cleanup;
    }

    opcode = decode_patch_operation(patch, case_sensitive);
    if (opcode == INVALID)
    {
        status = 3;
        goto cleanup;
    }
    else if (opcode == TEST)
    {
        /* compare value: {...} with the given path */
        status = !compare_json(get_item_from_pointer(object, path->valuestring, case_sensitive), get_object_item(patch, "value", case_sensitive), case_sensitive);
        goto cleanup;
    }

    /* special case for replacing the root */
    if (path->valuestring[0] == '\0')
    {
        if (opcode == REMOVE)
        {
            static const cJSON invalid = { NULL, NULL, NULL, cJSON_Invalid, NULL, 0, 0, NULL};

            overwrite_item(object, invalid);

            status = 0;
            goto cleanup;
        }

        if ((opcode == REPLACE) || (opcode == ADD))
        {
            value = get_object_item(patch, "value", case_sensitive);
            if (value == NULL)
            {
                /* missing "value" for add/replace. */
                status = 7;
                goto cleanup;
            }

            value = cJSON_Duplicate(value, 1);
            if (value == NULL)
            {
                /* out of memory for add/replace. */
                status = 8;
                goto cleanup;
            }

            overwrite_item(object, *value);

            /* delete the duplicated value */
            cJSON_free(value);
            value = NULL;

            /* the string "value" isn't needed */
            if (object->string != NULL)
            {
                cJSON_free(object->string);
                object->string = NULL;
            }

            status = 0;
            goto cleanup;
        }
    }

    if ((opcode == REMOVE) || (opcode == REPLACE))
    {
        /* Get rid of old. */
        cJSON *old_item = detach_path(object, (unsigned char*)path->valuestring, case_sensitive);
        if (old_item == NULL)
        {
            status = 13;
            goto cleanup;
        }
        cJSON_Delete(old_item);
        if (opcode == REMOVE)
        {
            /* For Remove, this job is done. */
            status = 0;
            goto cleanup;
        }
    }

    /* Copy/Move uses "from". */
    if ((opcode == MOVE) || (opcode == COPY))
    {
        cJSON *from = get_object_item(patch, "from", case_sensitive);
        if (from == NULL)
        {
            /* missing "from" for copy/move. */
            status = 4;
            goto cleanup;
        }

        if (opcode == MOVE)
        {
            value = detach_path(object, (unsigned char*)from->valuestring, case_sensitive);
        }
        if (opcode == COPY)
        {
            value = get_item_from_pointer(object, from->valuestring, case_sensitive);
        }
        if (value == NULL)
        {
            /* missing "from" for copy/move. */
            status = 5;
            goto cleanup;
        }
        if (opcode == COPY)
        {
            value = cJSON_Duplicate(value, 1);
        }
        if (value == NULL)
        {
            /* out of memory for copy/move. */
            status = 6;
            goto cleanup;
        }
    }
    else /* Add/Replace uses "value". */
    {
        value = get_object_item(patch, "value", case_sensitive);
        if (value == NULL)
        {
            /* missing "value" for add/replace. */
            status = 7;
            goto cleanup;
        }
        value = cJSON_Duplicate(value, 1);
        if (value == NULL)
        {
            /* out of memory for add/replace. */
            status = 8;
            goto cleanup;
        }
    }

    /* Now, just add "value" to "path". */

    /* split pointer in parent and child */
    parent_pointer = cJSONUtils_strdup((unsigned char*)path->valuestring);
    if (parent_pointer) {
        child_pointer = (unsigned char*)strrchr((char*)parent_pointer, '/');
    }
    if (child_pointer != NULL)
    {
        child_pointer[0] = '\0';
        child_pointer++;
    }
    parent = get_item_from_pointer(object, (char*)parent_pointer, case_sensitive);
    decode_pointer_inplace(child_pointer);

    /* add, remove, replace, move, copy, test. */
    if ((parent == NULL) || (child_pointer == NULL))
    {
        /* Couldn't find object to add to. */
        status = 9;
        goto cleanup;
    }
    else if (cJSON_IsArray(parent))
    {
        if (strcmp((char*)child_pointer, "-") == 0)
        {
            cJSON_AddItemToArray(parent, value);
            value = NULL;
        }
        else
        {
            size_t index = 0;
            if (!decode_array_index_from_pointer(child_pointer, &index))
            {
                status = 11;
                goto cleanup;
            }

            if (!insert_item_in_array(parent, index, value))
            {
                status = 10;
                goto cleanup;
            }
            value = NULL;
        }
    }
    else if (cJSON_IsObject(parent))
    {
        if (case_sensitive)
        {
            cJSON_DeleteItemFromObjectCaseSensitive(parent, (char*)child_pointer);
        }
        else
        {
            cJSON_DeleteItemFromObject(parent, (char*)child_pointer);
        }
        cJSON_AddItemToObject(parent, (char*)child_pointer, value);
        value = NULL;
    }
    else /* parent is not an object */
    {
        /* Couldn't find object to add to. */
        status = 9;
        goto cleanup;
    }

cleanup:
    if (value != NULL)
    {
        cJSON_Delete(value);
    }
    if (parent_pointer != NULL)
    {
        cJSON_free(parent_pointer);
    }

    return status;
}

CJSON_PUBLIC(int) cJSONUtils_ApplyPatches(cJSON * const object, const cJSON * const patches)
{
    const cJSON *current_patch = NULL;
    int status = 0;

    if (!cJSON_IsArray(patches))
    {
        /* malformed patches. */
        return 1;
    }

    if (patches != NULL)
    {
        current_patch = patches->child;
    }

    while (current_patch != NULL)
    {
        status = apply_patch(object, current_patch, false);
        if (status != 0)
        {
            return status;
        }
        current_patch = current_patch->next;
    }

    return 0;
}

CJSON_PUBLIC(int) cJSONUtils_ApplyPatchesCaseSensitive(cJSON * const object, const cJSON * const patches)
{
    const cJSON *current_patch = NULL;
    int status = 0;

    if (!cJSON_IsArray(patches))
    {
        /* malformed patches. */
        return 1;
    }

    if (patches != NULL)
    {
        current_patch = patches->child;
    }

    while (current_patch != NULL)
    {
        status = apply_patch(object, current_patch, true);
        if (status != 0)
        {
            return status;
        }
        current_patch = current_patch->next;
    }

    return 0;
}

static void compose_patch(cJSON * const patches, const unsigned char * const operation, const unsigned char * const path, const unsigned char *suffix, const cJSON * const value)
{
    cJSON *patch = NULL;

    if ((patches == NULL) || (operation == NULL) || (path == NULL))
    {
        return;
    }

    patch = cJSON_CreateObject();
    if (patch == NULL)
    {
        return;
    }
    cJSON_AddItemToObject(patch, "op", cJSON_CreateString((const char*)operation));

    if (suffix == NULL)
    {
        cJSON_AddItemToObject(patch, "path", cJSON_CreateString((const char*)path));
    }
    else
    {
        size_t suffix_length = pointer_encoded_length(suffix);
        size_t path_length = strlen((const char*)path);
        unsigned char *full_path = (unsigned char*)cJSON_malloc(path_length + suffix_length + sizeof("/"));

        sprintf((char*)full_path, "%s/", (const char*)path);
        encode_string_as_pointer(full_path + path_length + 1, suffix);

        cJSON_AddItemToObject(patch, "path", cJSON_CreateString((const char*)full_path));
        cJSON_free(full_path);
    }

    if (value != NULL)
    {
        cJSON_AddItemToObject(patch, "value", cJSON_Duplicate(value, 1));
    }
    cJSON_AddItemToArray(patches, patch);
}

CJSON_PUBLIC(void) cJSONUtils_AddPatchToArray(cJSON * const array, const char * const operation, const char * const path, const cJSON * const value)
{
    compose_patch(array, (const unsigned char*)operation, (const unsigned char*)path, NULL, value);
}

static void create_patches(cJSON * const patches, const unsigned char * const path, cJSON * const from, cJSON * const to, const cJSON_bool case_sensitive)
{
    if ((from == NULL) || (to == NULL))
    {
        return;
    }

    if ((from->type & 0xFF) != (to->type & 0xFF))
    {
        compose_patch(patches, (const unsigned char*)"replace", path, 0, to);
        return;
    }

    switch (from->type & 0xFF)
    {
        case cJSON_Number:
            if ((from->valueint != to->valueint) || !compare_double(from->valuedouble, to->valuedouble))
            {
                compose_patch(patches, (const unsigned char*)"replace", path, NULL, to);
            }
            return;

        case cJSON_String:
            if (strcmp(from->valuestring, to->valuestring) != 0)
            {
                compose_patch(patches, (const unsigned char*)"replace", path, NULL, to);
            }
            return;

        case cJSON_Array:
        {
            size_t index = 0;
            cJSON *from_child = from->child;
            cJSON *to_child = to->child;
            unsigned char *new_path = (unsigned char*)cJSON_malloc(strlen((const char*)path) + 20 + sizeof("/")); /* Allow space for 64bit int. log10(2^64) = 20 */

            /* generate patches for all array elements that exist in both "from" and "to" */
            for (index = 0; (from_child != NULL) && (to_child != NULL); (void)(from_child = from_child->next), (void)(to_child = to_child->next), index++)
            {
                /* check if conversion to unsigned long is valid
                 * This should be eliminated at compile time by dead code elimination
                 * if size_t is an alias of unsigned long, or if it is bigger */
                if (index > ULONG_MAX)
                {
                    cJSON_free(new_path);
                    return;
                }
                sprintf((char*)new_path, "%s/%lu", path, (unsigned long)index); /* path of the current array element */
                create_patches(patches, new_path, from_child, to_child, case_sensitive);
            }

            /* remove leftover elements from 'from' that are not in 'to' */
            for (; (from_child != NULL); (void)(from_child = from_child->next))
            {
                /* check if conversion to unsigned long is valid
                 * This should be eliminated at compile time by dead code elimination
                 * if size_t is an alias of unsigned long, or if it is bigger */
                if (index > ULONG_MAX)
                {
                    cJSON_free(new_path);
                    return;
                }
                sprintf((char*)new_path, "%lu", (unsigned long)index);
                compose_patch(patches, (const unsigned char*)"remove", path, new_path, NULL);
            }
            /* add new elements in 'to' that were not in 'from' */
            for (; (to_child != NULL); (void)(to_child = to_child->next), index++)
            {
                compose_patch(patches, (const unsigned char*)"add", path, (const unsigned char*)"-", to_child);
            }
            cJSON_free(new_path);
            return;
        }

        case cJSON_Object:
        {
            cJSON *from_child = NULL;
            cJSON *to_child = NULL;
            sort_object(from, case_sensitive);
            sort_object(to, case_sensitive);

            from_child = from->child;
            to_child = to->child;
            /* for all object values in the object with more of them */
            while ((from_child != NULL) || (to_child != NULL))
            {
                int diff;
                if (from_child == NULL)
                {
                    diff = 1;
                }
                else if (to_child == NULL)
                {
                    diff = -1;
                }
                else
                {
                    diff = compare_strings((unsigned char*)from_child->string, (unsigned char*)to_child->string, case_sensitive);
                }

                if (diff == 0)
                {
                    /* both object keys are the same */
                    size_t path_length = strlen((const char*)path);
                    size_t from_child_name_length = pointer_encoded_length((unsigned char*)from_child->string);
                    unsigned char *new_path = (unsigned char*)cJSON_malloc(path_length + from_child_name_length + sizeof("/"));

                    sprintf((char*)new_path, "%s/", path);
                    encode_string_as_pointer(new_path + path_length + 1, (unsigned char*)from_child->string);

                    /* create a patch for the element */
                    create_patches(patches, new_path, from_child, to_child, case_sensitive);
                    cJSON_free(new_path);

                    from_child = from_child->next;
                    to_child = to_child->next;
                }
                else if (diff < 0)
                {
                    /* object element doesn't exist in 'to' --> remove it */
                    compose_patch(patches, (const unsigned char*)"remove", path, (unsigned char*)from_child->string, NULL);

                    from_child = from_child->next;
                }
                else
                {
                    /* object element doesn't exist in 'from' --> add it */
                    compose_patch(patches, (const unsigned char*)"add", path, (unsigned char*)to_child->string, to_child);

                    to_child = to_child->next;
                }
            }
            return;
        }

        default:
            break;
    }
}

CJSON_PUBLIC(cJSON *) cJSONUtils_GeneratePatches(cJSON * const from, cJSON * const to)
{
    cJSON *patches = NULL;

    if ((from == NULL) || (to == NULL))
    {
        return NULL;
    }

    patches = cJSON_CreateArray();
    create_patches(patches, (const unsigned char*)"", from, to, false);

    return patches;
}

CJSON_PUBLIC(cJSON *) cJSONUtils_GeneratePatchesCaseSensitive(cJSON * const from, cJSON * const to)
{
    cJSON *patches = NULL;

    if ((from == NULL) || (to == NULL))
    {
        return NULL;
    }

    patches = cJSON_CreateArray();
    create_patches(patches, (const unsigned char*)"", from, to, true);

    return patches;
}

CJSON_PUBLIC(void) cJSONUtils_SortObject(cJSON * const object)
{
    sort_object(object, false);
}

CJSON_PUBLIC(void) cJSONUtils_SortObjectCaseSensitive(cJSON * const object)
{
    sort_object(object, true);
}

static cJSON *merge_patch(cJSON *target, const cJSON * const patch, const cJSON_bool case_sensitive)
{
    cJSON *patch_child = NULL;

    if (!cJSON_IsObject(patch))
    {
        /* scalar value, array or NULL, just duplicate */
        cJSON_Delete(target);
        return cJSON_Duplicate(patch, 1);
    }

    if (!cJSON_IsObject(target))
    {
        cJSON_Delete(target);
        target = cJSON_CreateObject();
    }

    patch_child = patch->child;
    while (patch_child != NULL)
    {
        if (cJSON_IsNull(patch_child))
        {
            /* NULL is the indicator to remove a value, see RFC7396 */
            if (case_sensitive)
            {
                cJSON_DeleteItemFromObjectCaseSensitive(target, patch_child->string);
            }
            else
            {
                cJSON_DeleteItemFromObject(target, patch_child->string);
            }
        }
        else
        {
            cJSON *replace_me = NULL;
            cJSON *replacement = NULL;

            if (case_sensitive)
            {
                replace_me = cJSON_DetachItemFromObjectCaseSensitive(target, patch_child->string);
            }
            else
            {
                replace_me = cJSON_DetachItemFromObject(target, patch_child->string);
            }

            replacement = merge_patch(replace_me, patch_child, case_sensitive);
            if (replacement == NULL)
            {
                cJSON_Delete(target);
                return NULL;
            }

            cJSON_AddItemToObject(target, patch_child->string, replacement);
        }
        patch_child = patch_child->next;
    }
    return target;
}

CJSON_PUBLIC(cJSON *) cJSONUtils_MergePatch(cJSON *target, const cJSON * const patch)
{
    return merge_patch(target, patch, false);
}

CJSON_PUBLIC(cJSON *) cJSONUtils_MergePatchCaseSensitive(cJSON *target, const cJSON * const patch)
{
    return merge_patch(target, patch, true);
}

static cJSON *generate_merge_patch(cJSON * const from, cJSON * const to, const cJSON_bool case_sensitive)
{
    cJSON *from_child = NULL;
    cJSON *to_child = NULL;
    cJSON *patch = NULL;
    if (to == NULL)
    {
        /* patch to delete everything */
        return cJSON_CreateNull();
    }
    if (!cJSON_IsObject(to) || !cJSON_IsObject(from))
    {
        return cJSON_Duplicate(to, 1);
    }

    sort_object(from, case_sensitive);
    sort_object(to, case_sensitive);

    from_child = from->child;
    to_child = to->child;
    patch = cJSON_CreateObject();
    if (patch == NULL)
    {
        return NULL;
    }
    while (from_child || to_child)
    {
        int diff;
        if (from_child != NULL)
        {
            if (to_child != NULL)
            {
                diff = strcmp(from_child->string, to_child->string);
            }
            else
            {
                diff = -1;
            }
        }
        else
        {
            diff = 1;
        }

        if (diff < 0)
        {
            /* from has a value that to doesn't have -> remove */
            cJSON_AddItemToObject(patch, from_child->string, cJSON_CreateNull());

            from_child = from_child->next;
        }
        else if (diff > 0)
        {
            /* to has a value that from doesn't have -> add to patch */
            cJSON_AddItemToObject(patch, to_child->string, cJSON_Duplicate(to_child, 1));

            to_child = to_child->next;
        }
        else
        {
            /* object key exists in both objects */
            if (!compare_json(from_child, to_child, case_sensitive))
            {
                /* not identical --> generate a patch */
                cJSON_AddItemToObject(patch, to_child->string, cJSONUtils_GenerateMergePatch(from_child, to_child));
            }

            /* next key in the object */
            from_child = from_child->next;
            to_child = to_child->next;
        }
    }
    if (patch->child == NULL)
    {
        /* no patch generated */
        cJSON_Delete(patch);
        return NULL;
    }

    return patch;
}

CJSON_PUBLIC(cJSON *) cJSONUtils_GenerateMergePatch(cJSON * const from, cJSON * const to)
{
    return generate_merge_patch(from, to, false);
}

CJSON_PUBLIC(cJSON *) cJSONUtils_GenerateMergePatchCaseSensitive(cJSON * const from, cJSON * const to)
{
    return generate_merge_patch(from, to, true);
}

typedef struct Stack
{
    char *mem;
    int size;
    int top;
} Stack;

static Stack *stack_create(int size)
{
    Stack *output = (Stack *) cJSON_malloc(sizeof(Stack));
    output->mem = (char *) cJSON_malloc((size_t) size);
    output->size = size;
    output->top = -1;
    return output;
}

static int stack_size(Stack *stack)
{
    return stack->top + 1;
}

static int stack_full(Stack *stack)
{
    return stack->top == stack->size - 1;
}

static int stack_empty(Stack *stack)
{
    return stack->top == -1;
}

static void stack_push(Stack *stack, char c)
{
    if (stack_full(stack))
    {
        stack->size *= 2;
        stack->mem = (char *) cJSON_realloc(stack->mem, (size_t) stack->size);
    }
    stack->mem[++stack->top] = c;
}

char stack_pop(Stack *stack)
{
    if (stack_empty(stack))
    {
        return -1;
    }
    return stack->mem[stack->top--];
}

void stack_destroy(Stack *stack)
{
    cJSON_free(stack->mem);
    cJSON_free(stack);
}

char stack_peek(Stack *stack)
{
    if (stack_empty(stack))
    {
        return -1;
    }
    return stack->mem[stack->top];
}

static Filter *cJSONUtils_New_Filter() {
    Filter *filter = (Filter *) cJSON_malloc(sizeof(Filter));
    if (filter)
    {
        memset(filter, '\0', sizeof(Filter));
    }

    return filter;
}

static void cJSONUtils_Delete_Filter(Filter *filter) {
    Filter *next = NULL;
    while (filter != NULL)
    {
        next = filter->next;
        if (filter->type == EXISTS)
        {
            cJSON_Delete(filter->value.exists);
        }
        else if (filter->type == COMP)
        {
            cJSON_Delete(filter->value.comp.right);
            cJSON_Delete(filter->value.comp.left);
        }
        else if (filter->type == LIST_ENTRY)
        {
            cJSON_Delete(filter->value.list_entry);
        }
        else if (filter->type == RESULT)
        {
            cJSON_Delete(filter->value.results);
        }
        cJSON_free(filter);
        filter = next;
    }
}

static void cJSONUtils_Free_Filter(Filter *filter)
{
    if (filter != NULL)
    {
        if (filter->type == EXISTS)
        {
            cJSON_Delete(filter->value.exists);
        }
        else if (filter->type == COMP)
        {
            cJSON_Delete(filter->value.comp.right);
            cJSON_Delete(filter->value.comp.left);
        }
        else if (filter->type == LIST_ENTRY)
        {
            cJSON_Delete(filter->value.list_entry);
        }
        else if (filter->type == RESULT)
        {
            cJSON_Delete(filter->value.results);
        }
        cJSON_free(filter);
    }
}

static Filter *cJSONUtils_Duplicate_Filter(Filter *head)
{
    if (head == NULL)
    {
        return NULL;
    }

    Filter *new_filter = cJSONUtils_New_Filter();
    new_filter->type = head->type;
    
    if (new_filter->type == COMP)
    {
        new_filter->value.comp.type = head->value.comp.type;
        new_filter->value.comp.right = cJSON_Duplicate(head->value.comp.right, false);
        new_filter->value.comp.left = cJSON_Duplicate(head->value.comp.left, false);
    }
    else if (new_filter->type == EXISTS)
    {
        new_filter->value.exists = cJSON_Duplicate(head->value.exists, false);
    }
    else if (new_filter->type == RESULT)
    {
        new_filter->value.results = cJSON_Duplicate(head->value.results, false);
    }
    else if (new_filter->type == LIST_ENTRY)
    {
        new_filter->value.list_entry = cJSON_Duplicate(head->value.list_entry, false);
    }

    new_filter->next = cJSONUtils_Duplicate_Filter(head->next);
    if (new_filter->next)
    {
        new_filter->next->prev = new_filter;
    }
    return new_filter;
}

static Selector *cJSONUtils_New_Selector()
{
    Selector *selectors = (Selector *) cJSON_malloc(sizeof(Selector));
    if (selectors)
    {
        memset(selectors, '\0', sizeof(Selector));
    }

    return selectors;
}

CJSON_PUBLIC(void) cJSONUtils_Delete_Selector(Selector *selector)
{
    Selector *next = NULL;
    while (selector != NULL) {
        next = selector->next;
        if (selector->type == DOT)
        {
            cJSON_free(selector->value.path);
        }
        else if (selector->type == LIST || selector->type == FILTER)
        {
            cJSONUtils_Delete_Filter(selector->value.filter);
        }
        cJSON_free(selector);
        selector = next;
    }
}

static cJSON *parse_comp_from_str(const char *str)
{
    cJSON *node = NULL;

    if (str[0] == '@' || str[0] == '$')
    {
        if (str[1] != '.')
        {
            goto error;
        }
        node = cJSON_CreateObject();
        node->valuestring = (char *) cJSONUtils_strdup((const unsigned char *) str);
    }
    else if (str[0] == '-' || isdigit(str[0]))
    {
        char *endptr = NULL;
        double d = strtod(str, &endptr);
        if (endptr != NULL && *endptr != '\0')
        {
            goto error;
        }
        node = cJSON_CreateNumber(d);
    }
    else if (str[0] == '\'' || str[0] == '\"')
    {
        size_t len = strlen(str);
        char path[len];
        memset(path, '\0', len);
        strncpy(path, str + 1, len - 2);
        node = cJSON_CreateString((const char *) path);
    }
    else
    {
        goto error;
    }

    return node;

error:
    return NULL;
}

static Filter *parse_filter_from_str(const char *tmp_path, int last_op, int current)
{
    int size = current - last_op + 1;
    char str[size];
    char left[size];
    char right[size];
    memset(str, '\0', (size_t) size);
    memset(left, '\0', (size_t) size);
    memset(right, '\0', (size_t) size);
    strncpy(str, tmp_path + last_op, size - 1);

    Filter *filter = cJSONUtils_New_Filter();
    int i = 0;
    for (; i < size; ++i)
    {
        if (str[i] == '>' && (size - i) > 1)
        {
            filter->type = COMP;
            filter->value.comp.type = GT;
            strncpy(left, str, i);
            strncpy(right, str + i + 1, size - i - 1);
            break;
        }
        else if (str[i] == '<' && (size - i) > 1)
        {
            filter->type = COMP;
            filter->value.comp.type = LT;
            strncpy(left, str, i);
            strncpy(right, str + i + 1, size - i - 1);
            break;
        }
        else if (str[i] == '>' && str[i + 1] == '=' && (size - i) > 2)
        {
            filter->type = COMP;
            filter->value.comp.type = GTE;
            strncpy(left, str, i);
            strncpy(right, str + i + 2, size - i - 2);
            break;
        }
        else if (str[i] == '<' && str[i + 1] == '=' && (size - i) > 2)
        {
            filter->type = COMP;
            filter->value.comp.type = LTE;
            strncpy(left, str, i);
            strncpy(right, str + i + 2, size - i - 2);
            break;
        }
        else if (str[i] == '!' && str[i + 1] == '=' && (size - i) > 2)
        {
            filter->type = COMP;
            filter->value.comp.type = NE;
            strncpy(left, str, i);
            strncpy(right, str + i + 2, size - i - 2);
            break;
        }
        else if (str[i] == '=' && str[i + 1] == '=' && (size - i) > 2)
        {
            filter->type = COMP;
            filter->value.comp.type = EQ;
            strncpy(left, str, i);
            strncpy(right, str + i + 2, size - i - 2);
            break;
        }
    }

    if (filter->type == COMP)
    {
        cJSON *l = parse_comp_from_str(left);
        cJSON *r = parse_comp_from_str(right);
        if (!l || !r)
        {
            if (l)
            {
                cJSON_Delete(l);
            }
            if (r)
            {
                cJSON_Delete(r);
            }
            goto error;
        }
        filter->value.comp.left = l;
        filter->value.comp.right = r;
    }
    else if (i == size) // just one operator
    {
        filter->type = EXISTS;
        cJSON *exists = parse_comp_from_str(str);
        if (exists == NULL)
        {
            goto error;
        }
        filter->value.exists = exists;
    }
    else
    {
        goto error;
    }

    return filter;

error:
    cJSONUtils_Delete_Filter(filter);
    return NULL;
}

static void append_filter(Filter **next_filter, enum FilterType type)
{
    Filter *filter = cJSONUtils_New_Filter();
    filter->type = type;
    (*next_filter)->next = filter;
    filter->prev = (*next_filter);
    (*next_filter) = filter;
}

static int compile_selector_filter(const char *p, Selector **selectors)
{
    int len = 0;
    Stack *stack = stack_create(1024);

    Filter *next_filter = NULL;
    Selector *filter = cJSONUtils_New_Selector();
    filter->type = FILTER;
    filter->value.filter = cJSONUtils_New_Filter();
    filter->value.filter->type = FIRST;
    next_filter = filter->value.filter;
    (*selectors)->next = filter;
    (*selectors) = filter;

    if (*(p + 1) != '?' || *(p + 2) != '(')
    {
        goto error;
    }
    else
    {
        len += 2;
        p += 2; // skip [ and ?
    }

    while (*p != ']')
    {
        if (*p == '\0')
        {
            goto error;
        }
        else if (*p == ')')
        {
            int size = stack_size(stack);
            int index = size;
            char tmp_path[size];
            memset(tmp_path, '\0', (size_t) size);

            while (!stack_empty(stack) && stack_peek(stack) != '(')
            {
                tmp_path[--index] = stack_pop(stack);
            }

            if (stack_empty(stack))
            {
                goto error;
            }
            else
            {
                tmp_path[--index] = stack_pop(stack); // get '('
            }

            // before pop, stack_peek(stack) == '(' and size == 1
            if (index == size - 1)
            {
                append_filter(&next_filter, RIGHT_BRACKETS);
                p++;
                len++;
                continue;
            }

            if (!stack_empty(stack) && stack_peek(stack) != '(')
            {
                char op[3];
                op[0] = stack_pop(stack);
                op[1] = stack_pop(stack);
                op[2] = '\0';
                if (!strcmp(op, "&&"))
                {
                    Filter *and = cJSONUtils_New_Filter();
                    and->type = AND;
                    next_filter->prev->next = and;
                    and->prev = next_filter->prev;
                    and->next = next_filter;
                    next_filter->prev = and;
                }
                else if (!strcmp(op, "||"))
                {
                    Filter *or = cJSONUtils_New_Filter();
                    or->type = OR;
                    next_filter->prev->next = or;
                    or->prev = next_filter->prev;
                    or->next = next_filter;
                    next_filter->prev = or;
                }
                else
                {
                    goto error;
                }
            }

            int last_op = index + 1;
            for (int i = last_op; i < size; ++i) // i = index + 1, means i point to '(' next char
            {
                if (tmp_path[i] == '&' && tmp_path[i + 1] == '&')
                {
                    // handle ( || C )
                    if (i == last_op) 
                    {
                        append_filter(&next_filter, AND);
                        i += 1;
                        last_op = i + 1;
                        continue;
                    }
                    
                    Filter *f = parse_filter_from_str(tmp_path, last_op, i);
                    if (f == NULL)
                    {
                        goto error;
                    }
                    next_filter->next = f;
                    f->prev = next_filter;
                    next_filter = f;

                    // APPEND &&
                    append_filter(&next_filter, AND);
                    i += 1;
                    last_op = i + 1;
                }
                else if (tmp_path[i] == '|' && tmp_path[i + 1] == '|')
                {
                    if (i == last_op)
                    {
                        append_filter(&next_filter, OR);
                        i += 1;
                        last_op = i + 1;
                        continue;
                    }
                    
                    Filter *f = parse_filter_from_str(tmp_path, last_op, i);
                    if (f == NULL)
                    {
                        goto error;
                    }
                    next_filter->next = f;
                    f->prev = next_filter;
                    next_filter = f;

                    // APPEND &&
                    append_filter(&next_filter, OR);
                    i += 1;
                    last_op = i + 1;
                }
            }

            Filter *f = parse_filter_from_str(tmp_path, last_op, size);
            if (f == NULL)
            {
                goto error;
            }
            next_filter->next = f;
            f->prev = next_filter;
            next_filter = f;

            // APPEND )
            append_filter(&next_filter, RIGHT_BRACKETS);
            
            p++;
            len++;
            continue;
        }
        else if (*p == '(')
        {
            append_filter(&next_filter, LEFT_BRACKETS);
        }
        else if (*p == ' ')
        {
            p++;
            len++;
            continue;
        }

        stack_push(stack, *p);
        p++;
        len++;
    }

    if (!stack_empty(stack))
    {
        goto error;
    }

    stack_destroy(stack);
    return len + 1;

error:
    stack_destroy(stack);
    return -1;
}

static int compile_selector_dot(const char *p, Selector **selectors)
{
    const char *s = p + 1;
    size_t len = 0;

    while (*s != '[' && *s != '.' && *s != '\0')
    {
        len++;
        s++;
    }
    if (len == 0)
    {
        return -1;
    }

    Selector *dot = cJSONUtils_New_Selector();
    dot->type = DOT;
    dot->value.path = (char *) cJSON_malloc(len + 1);
    memset(dot->value.path, '\0', len + 1);
    strncpy(dot->value.path, p + 1, len);
    (*selectors)->next = dot;
    (*selectors) = dot;
    return (int)len + 1;
}

static int compile_selector_root(Selector **selectors)
{
    Selector *root = cJSONUtils_New_Selector();
    root->type = ROOT;
    (*selectors)->next = root;
    (*selectors) = root;
    return 1;
}

static int compile_selector_decendant(Selector **selectors)
{
    Selector *decendant = cJSONUtils_New_Selector();
    decendant->type = DECENDANT;
    (*selectors)->next = decendant;
    (*selectors) = decendant;
    return 0;
}

static int compile_selector_dot_wild(Selector **selectors)
{
    Selector *dot_wild = cJSONUtils_New_Selector();
    dot_wild->type = DOT_WILD;
    (*selectors)->next = dot_wild;
    (*selectors) = dot_wild;
    return 2;
}

static int compile_selector_square_bracket(const char *p, Selector **selectors)
{
    const char *s = p + 2;
    size_t len = 0;
    while (*s != '\'' && *s != '\"')
    {
        if (*s == '\0')
        {
            return -1;
        }
        len++;
        s++;
    }
    if (len == 0)
    {
        return -1;
    }

    Selector *dot = cJSONUtils_New_Selector();
    dot->type = DOT;
    dot->value.path = (char *) cJSON_malloc(len + 1);
    memset(dot->value.path, '\0', len + 1);
    strncpy(dot->value.path, p + 2, len);
    (*selectors)->next = dot;
    (*selectors) = dot;
    return (int)len + 4;
}

#ifdef __linux__
// strsep cannot be compiled with c99, but we want to use c99, so customize strsep
// https://www.linuxquestions.org/questions/programming-9/how-to-address-warnings-895351/
// https://stackoverflow.com/a/58244503/5761134
static char *strsep(char **stringp, const char *delim) {
    char *rv = *stringp;
    if (rv) 
    {
        *stringp += strcspn(*stringp, delim);
        if (**stringp)
            *(*stringp)++ = '\0';
        else
            *stringp = 0; 
    }
    return rv;
}
#endif

static int compile_selector_index_slice_list(const char *p, Selector **selectors)
{
    const char *s = p + 1;
    size_t len = 0;
    size_t digit_num = 0;
    size_t colon_num = 0;
    size_t comma_num = 0;
    size_t asterisk_num = 0;

    while (*s != ']')
    {
        if (*s == '\0')
        {
            return -1;
        }
        else if (isdigit(*s))
        {
            digit_num++;
        }
        else if (*s == ':')
        {
            colon_num++;
        }
        else if (*s == ',')
        {
            comma_num++;
        }
        else if (*s == '*')
        {
            asterisk_num++;
        }
        len++;
        s++;
    }
    if (len == 0)
    {
        return -1;
    }
    char str[len + 1];
    memset(str, '\0', len + 1);
    strncpy(str, p + 1, len);

    // INDEX
    if ((digit_num == len) || ((digit_num == len - 1) && (str[0] == '-' || str[0] == '+')))
    {
        int i = atoi(str);
        Selector *index = cJSONUtils_New_Selector();
        index->type = INDEX;
        index->value.index = i;
        (*selectors)->next = index;
        (*selectors) = index;
        return (int)len + 2;
    }
    // INDEX_WILD [*]
    if (asterisk_num == len)
    {
        Selector *index_wild = cJSONUtils_New_Selector();
        index_wild->type = INDEX_WILD;
        (*selectors)->next = index_wild;
        (*selectors) = index_wild;
        return 3;
    }
    
    // SLICE
    if (colon_num == 2) // we only support 1:1:1
    {
        char *buf = str;
        char *token;

        Selector *slice = cJSONUtils_New_Selector();
        slice->type = ARRAY_SLICE;
        for (int i = 0; i < 3; ++i)
        {
            token = strsep(&buf, ":");
            if (*token == '\0')
            {
                cJSONUtils_Delete_Selector(slice);
                return -1;
            }
            slice->value.slice[i] = atoi(token);
        }
        (*selectors)->next = slice;
        (*selectors) = slice;
        return (int)len + 2;
    }
    // LIST
    if (comma_num != 0)
    {
        Filter *next_filter = NULL;
        Selector *list = cJSONUtils_New_Selector();
        list->type = LIST;
        list->value.filter = cJSONUtils_New_Filter();
        list->value.filter->type = FIRST;
        next_filter = list->value.filter;
        (*selectors)->next = list;
        (*selectors) = list;
        
        char *buf = str;
        char *token;
        while ((token = strsep(&buf, ",")) != NULL)
        {
            cJSON *node = parse_comp_from_str(token);
            if (node == NULL)
            {
                return -1;
            }
            Filter *f = cJSONUtils_New_Filter();
            f->type = LIST_ENTRY;
            f->value.list_entry = node;
            next_filter->next = f;
            f->prev = next_filter;
            next_filter = f;
        }
        return (int)len + 2;
    }
    return -1;
}

static Selector *compile_selector(const char * const path)
{
    if (path == NULL)
    {
        return NULL;
    }

    Selector *selectors = cJSONUtils_New_Selector();
    Selector *head = selectors;
    const char *p = path;
    int skip = 0;
    while (*p != '\0')
    {
        switch (*p) {
            // ROOT
            case '$':
            {
                skip = compile_selector_root(&selectors);
                p += skip;
                break;
            }
            case '.':
            {
                char next_char = *(p + 1);
                // DESCENDANT ..
                if (next_char == '.')
                {
                    compile_selector_decendant(&selectors);
                    if (*(p + 2) == '*')
                    {
                        p += 1;
                        skip = compile_selector_dot_wild(&selectors);
                        p += skip;
                    }
                    else if(*(p + 2) == '[')
                    {
                        p += 2;
                        skip = compile_selector_index_slice_list(p, &selectors);
                        if (skip < 0)
                        {
                            goto error;
                        }
                        p += skip;
                    }
                    else
                    {
                        p += 1;
                        skip = compile_selector_dot(p, &selectors);
                        if (skip < 0)
                        {
                            goto error;
                        }
                        p += skip;
                    }
                }
                // DOT WILD .*
                else if (next_char == '*')
                {
                    skip = compile_selector_dot_wild(&selectors);
                    p += skip;
                }
                // DOT .abc
                else
                {
                    skip = compile_selector_dot(p, &selectors);
                    if (skip < 0)
                    {
                        goto error;
                    }
                    p += skip;
                }
                break;
            }
            case '[':
            {
                char next_char = *(p + 1);
                
                // DOT [' '] or [" "]
                if (next_char == '\'' || next_char == '\"')
                {
                    skip = compile_selector_square_bracket(p, &selectors);
                    if (skip < 0)
                    {
                        goto error;
                    }
                    p += skip;
                }
                    // FILTER [?]
                else if (next_char == '?')
                {
                    skip = compile_selector_filter(p, &selectors);
                    if (skip < 0)
                    {
                        goto error;
                    }
                    p += skip;
                }
                else
                {
                    skip = compile_selector_index_slice_list(p, &selectors);
                    if (skip < 0)
                    {
                        goto error;
                    }
                    p += skip;
                }
                break;
            }
            default:
            {
                goto error;
            }
        }
    }
    return head;

error:
    cJSONUtils_Delete_Selector(head);
    return NULL;
}

CJSON_PUBLIC(Selector *) cJSONUtils_CompileSelector(const char * const path)
{
    return compile_selector(path);
}

static cJSON *root_selector(const cJSON * const root)
{
    cJSON *new_items = cJSON_CreateArray();
    cJSON_AddItemToArray(new_items, cJSON_CreateObjectReference(root));
    return new_items;
}

static cJSON *dot_selector(cJSON *items, const char *path, const cJSON_bool case_sensitive)
{
    cJSON *item_a = NULL;
    cJSON *item_b = NULL;

    cJSON *new_items = cJSON_CreateArray();
    cJSON_ArrayForEach(item_a, items)
    {
        item_b = get_object_item(item_a->child, path, case_sensitive);
        if (item_b)
        {
            cJSON *item = cJSON_CreateObjectReference(item_b);
            item->valuestring = (char *) ((void *) (item_a->child));
            cJSON_AddItemToArray(new_items, item);
        }
    }
    return new_items;
}

static cJSON *dot_index_wild_selector(cJSON *items)
{
    cJSON *item_a = NULL;
    cJSON *item_b = NULL;

    cJSON *new_items = cJSON_CreateArray();
    cJSON_ArrayForEach(item_a, items)
    {
        item_b = item_a->child->child;
        while (item_b != NULL)
        {
            cJSON_AddItemToArray(new_items, cJSON_CreateObjectReference(item_b));
            item_b = item_b->next;
        }
    }
    return new_items;
}

static cJSON *index_selector(cJSON *array, Selector *selector)
{
    cJSON *new_items = cJSON_CreateArray();
    int index = selector->value.index;
    if (index < 0)
    {
        index += cJSON_GetArraySize(array);
    }
    cJSON *item = get_array_item(array, (size_t) index);
    if (item)
    {
        cJSON_AddItemToArray(new_items, cJSON_CreateObjectReference(item));
    }
    return new_items;
}

static int array_slice_normalize(int i, int size)
{
    if (i >= 0)
    {
        return i;
    }
    else
    {
        return size + i;
    }
}

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
static void array_slice_bounds(int start, int end, int step, int len, int *lower, int *upper)
{
    int n_start = array_slice_normalize(start, len);
    int n_end = array_slice_normalize(end, len);

    if (step >= 0) 
    {
        *lower = MIN(MAX(n_start, 0), len);
        *upper = MIN(MAX(n_end, 0), len);
    } 
    else 
    {
        *upper = MIN(MAX(n_start, -1), len - 1);
        *lower = MIN(MAX(n_end, -1), len - 1);
    }
}

static cJSON *array_slice_selector(cJSON *array, Selector *selector)
{
    cJSON *new_items = cJSON_CreateArray();

    int start = selector->value.slice[0];
    int end = selector->value.slice[1];
    int step = selector->value.slice[2];
    int size = cJSON_GetArraySize(array);

    if (step == 0)
    {
        return new_items;
    }

    int lower, upper;
    int reverse = step < 0;
    array_slice_bounds(start, end, step, size, &lower, &upper);

    if (reverse)
    {
        cJSON *prev = array->child;
        for (int i = size; i > upper; --i)
        {
            if (prev)
            {
                prev = prev->prev;
            }
        }
        for (; upper > lower; upper += step)
        {
            if (prev)
            {
                cJSON_AddItemToArray(new_items, cJSON_CreateObjectReference(prev));
            }
            for (int i = 0; i > step && fabs((double)i) < upper; --i)
            {
                if (prev)
                {
                    prev = prev->prev;
                }
            }
        }
    }
    else
    {
        cJSON *next = array->child;
        for (int i = 0; i < lower; ++i)
        {
            if (next)
            {
                next = next->next;
            }
        }
        for (; lower < upper; lower += step)
        {
            if (next)
            {
                cJSON_AddItemToArray(new_items, cJSON_CreateObjectReference(next));
            }
            for (int i = 0; i < step && i < upper; ++i)
            {
                if (next)
                {
                    next = next->next;
                }
            }
        }
    }
    return new_items;
}

static void add_all_decendant(cJSON *items, cJSON *node)
{
    cJSON *next = NULL;
    while (node != NULL)
    {
        next = node->next;
        if (node->child != NULL)
        {
            add_all_decendant(items, node->child);
        }
        cJSON_AddItemToArray(items, cJSON_CreateObjectReference(node));
        node = next;
    }
}

static cJSON *decendant_all_selector(cJSON *items) {
    cJSON *item_a = NULL;

    cJSON *new_items = cJSON_CreateArray();
    cJSON_ArrayForEach(item_a, items)
    {
        cJSON *node = item_a->child;
        add_all_decendant(new_items, node);
    }
    return new_items;
}

static cJSON *list_selector(cJSON *array, Selector *selector)
{
    Filter *filter = cJSONUtils_Duplicate_Filter(selector->value.filter);
    Filter *head = filter;
    Filter *next = NULL;
    filter = filter->next; // SKIP FIRST
    cJSON *new_items = cJSON_CreateArray();
    while (filter != NULL)
    {
        next = filter->next;
        if (filter->type == LIST_ENTRY)
        {
            cJSON *list_entry = filter->value.list_entry;
            if (list_entry->type == cJSON_Number)
            {
                int index = list_entry->valueint;
                if (index < 0)
                {
                    index += cJSON_GetArraySize(array);
                }
                cJSON *item = get_array_item(array, (size_t) index);
                if (item)
                {
                    cJSON_AddItemToArray(new_items, cJSON_CreateObjectReference(item));
                }
            }
            else
            {
                goto error;
            }
        }
        else
        {
            goto error;
        }
        
        filter = next;
    }
    
    cJSONUtils_Delete_Filter(head);
    return new_items;

error:
    cJSON_Delete(new_items);
    cJSONUtils_Delete_Filter(head);
    return cJSON_CreateArray();
}

static cJSON *get_item_from_selector(const cJSON * const object, const Selector *selector, 
                                     const cJSON_bool case_sensitive, const cJSON_bool reference);
static cJSON *execute_filter_value(const cJSON * const root, cJSON *items, cJSON *node)
{
    if (node->valuestring && node->valuestring[0] == '@')
    {
        size_t len = strlen(node->valuestring);
        char path[len];
        memset(path, '\0', len);
        strncpy(path, node->valuestring + 2, len - 2);
        return dot_selector(items, path, true);
    }
    else if (node->valuestring && node->valuestring[0] == '$')
    {
        Selector *selector = compile_selector(node->valuestring);
        if (selector == NULL)
        {
            return cJSON_CreateArray();
        }
        cJSON *results = get_item_from_selector(root, selector, true, true);
        cJSONUtils_Delete_Selector(selector);
        
        if (results->child == NULL) {
            return results;
        }
        
        cJSON *child = (cJSON *)results->child->child;
        if (child->type != cJSON_Array && child->type != cJSON_Object)
        {
            cJSON_Delete(results);
            return child;
        }
        else 
        {
            return cJSON_CreateArray();
        }
    }
    return node;
}

static int filter_compare(cJSON *left, cJSON *right, enum CompType type)
{
    if (left->type != right->type)
    {
        return 0;
    }

    switch (type) {
        case GT:
        {
            if (left->type == cJSON_String || left->type == cJSON_Raw)
            {
                return strcmp(left->valuestring, right->valuestring) > 0;
            }
            else if (left->type == cJSON_Number)
            {
                return left->valuedouble > right->valuedouble;
            }
            return 0;
        }
        case LT:
        {
            if (left->type == cJSON_String || left->type == cJSON_Raw)
            {
                return strcmp(left->valuestring, right->valuestring) < 0;
            }
            else if (left->type == cJSON_Number)
            {
                return left->valuedouble < right->valuedouble;
            }
            return 0;
        }
        case GTE:
        {
            if (left->type == cJSON_String || left->type == cJSON_Raw)
            {
                return strcmp(left->valuestring, right->valuestring) >= 0;
            }
            else if (left->type == cJSON_Number)
            {
                return left->valuedouble >= right->valuedouble;
            }
            return 1;
        }
        case LTE:
        {
            if (left->type == cJSON_String || left->type == cJSON_Raw)
            {
                return strcmp(left->valuestring, right->valuestring) <= 0;
            }
            else if (left->type == cJSON_Number)
            {
                return left->valuedouble <= right->valuedouble;
            }
            return 1;
        }
        case EQ:
        {
            if (left->type == cJSON_String || left->type == cJSON_Raw)
            {
                return strcmp(left->valuestring, right->valuestring) == 0;
            }
            else if (left->type == cJSON_Number)
            {
                return left->valuedouble == right->valuedouble;
            }
            return 1;
        }
        case NE:
        {
            if (left->type == cJSON_String || left->type == cJSON_Raw)
            {
                return strcmp(left->valuestring, right->valuestring) != 0;
            }
            else if (left->type == cJSON_Number)
            {
                return left->valuedouble != right->valuedouble;
            }
            return 0;
        }
    }
    return 0;
}

static Filter *execute_filter(const cJSON * const root, cJSON *items, Filter *f)
{
    Filter *result = cJSONUtils_New_Filter();
    result->type = RESULT;

    if (f->type == EXISTS)
    {
        cJSON *item_a = NULL;
        cJSON *item_b = NULL;
        size_t len = strlen(f->value.exists->valuestring);
        char path[len];
        memset(path, '\0', len);
        strncpy(path, f->value.exists->valuestring + 2, len - 2);
        result->value.results = cJSON_CreateArray();
        cJSON_ArrayForEach(item_a, items)
        {
            item_b = get_object_item(item_a->child, path, true);
            if (item_b)
            {
                // we add item_a, because @ means parent node
                cJSON_AddItemToArray(result->value.results, cJSON_CreateObjectReference(item_a->child));
            }
        }
    }
    else if (f->type == COMP)
    {
        cJSON *left_values = execute_filter_value(root, items, f->value.comp.left);
        cJSON *right_values = execute_filter_value(root, items, f->value.comp.right);
        cJSON *new_items = cJSON_CreateArray();
        if (cJSON_IsArray(left_values) && cJSON_IsArray(right_values))
        {
            cJSON *lnode = left_values->child;
            cJSON *rnode = right_values->child;
            while (lnode && rnode)
            {
                if (filter_compare(lnode->child, rnode->child, f->value.comp.type))
                {
                    // we need add lnode->valuestring, which means father node
                    cJSON_AddItemToArray(new_items, cJSON_CreateObjectReference((cJSON *)((void *)lnode->valuestring)));
                }
                lnode = lnode->next;
                rnode = rnode->next;
            }
            cJSON_Delete(left_values);
            cJSON_Delete(right_values);
            result->value.results = new_items;
        }
        else if (cJSON_IsArray(left_values) && !cJSON_IsArray(right_values))
        {
            cJSON *lnode = left_values->child;
            cJSON *rnode = right_values;
            while (lnode)
            {
                if (filter_compare(lnode->child, rnode, f->value.comp.type))
                {
                    // we need add lnode->valuestring, which means father node
                    cJSON_AddItemToArray(new_items, cJSON_CreateObjectReference((cJSON *)((void *)lnode->valuestring)));
                }
                lnode = lnode->next;
            }
            cJSON_Delete(left_values);
            result->value.results = new_items;
        }
        else if (!cJSON_IsArray(left_values) && cJSON_IsArray(right_values))
        {
            cJSON *lnode = left_values;
            cJSON *rnode = right_values->child;
            while (rnode)
            {
                if (filter_compare(lnode, rnode->child, f->value.comp.type))
                {
                    // we need add rnode->valuestring, which means father node
                    cJSON_AddItemToArray(new_items, cJSON_CreateObjectReference((cJSON *)((void *)rnode->valuestring)));
                }
                rnode = rnode->next;
            }
            cJSON_Delete(right_values);
            result->value.results = new_items;
        }
        else
        {
            goto error;
        }
    }
    else
    {
        goto error;
    }

    return result;

error:
    cJSONUtils_Delete_Filter(result);
    return NULL;
}

static Filter *execute_filter_and(const cJSON * const root, cJSON *items, Filter *left, Filter *right)
{
    Filter *lef = NULL;
    Filter *ref = NULL;
    int lneedfree = 0;
    int rneedfree = 0;

    if (left->type == RESULT)
    {
        lef = left;
    }
    else
    {
        lef = execute_filter(root, items, left);
        lneedfree = 1;
    }

    if (right->type == RESULT)
    {
        ref = right;
    }
    else
    {
        ref = execute_filter(root, items, right);
        rneedfree = 1;
    }

    if (lef == NULL || ref == NULL)
    {
        return NULL;
    }

    cJSON *new_items = cJSON_CreateArray();
    for (cJSON *l = lef->value.results->child; l != NULL; l = l->next)
    {
        cJSON *r = NULL;
        for (r = ref->value.results->child; r != NULL; r = r->next)
        {
            if (l->child == r->child)
            {
                break;
            }
        }
        if (r != NULL)
        {
            cJSON_AddItemToArray(new_items, cJSON_CreateObjectReference(r->child));
        }
    }

    if (lneedfree)
    {
        cJSONUtils_Delete_Filter(lef);
    }
    if (rneedfree)
    {
        cJSONUtils_Delete_Filter(ref);
    }

    Filter *result = cJSONUtils_New_Filter();
    result->type = RESULT;
    result->value.results = new_items;
    return result;
}

static Filter *execute_filter_or(const cJSON * const root, cJSON *items, Filter *left, Filter *right)
{
    Filter *lef = NULL;
    Filter *ref = NULL;
    int lneedfree = 0;
    int rneedfree = 0;

    if (left->type == RESULT)
    {
        lef = left;
    }
    else
    {
        lef = execute_filter(root, items, left);
        lneedfree = 1;
    }

    if (right->type == RESULT)
    {
        ref = right;
    }
    else
    {
        ref = execute_filter(root, items, right);
        rneedfree = 1;
    }

    if (lef == NULL || ref == NULL)
    {
        return NULL;
    }

    cJSON *new_items = cJSON_CreateArray();
    for (cJSON *r = ref->value.results->child; r != NULL; r = r->next)
    {
        cJSON_AddItemToArray(new_items, cJSON_CreateObjectReference(r->child));
    }
    for (cJSON *l = lef->value.results->child; l != NULL; l = l->next)
    {
        cJSON *r = NULL;
        for (r = ref->value.results->child; r != NULL; r = r->next)
        {
            if (l->child == r->child)
            {
                break;
            }
        }
        if (r == NULL)
        {
            cJSON_AddItemToArray(new_items, cJSON_CreateObjectReference(l->child));
        }
    }

    if (lneedfree)
    {
        cJSONUtils_Delete_Filter(lef);
    }
    if (rneedfree)
    {
        cJSONUtils_Delete_Filter(ref);
    }

    Filter *result = cJSONUtils_New_Filter();
    result->type = RESULT;
    result->value.results = new_items;
    return result;
}

static Filter *execute_filter_unit(const cJSON * const root, cJSON *items, Filter *start, Filter *end)
{
    // First we handle &&
    Filter *f = NULL;
    for (f = start; f != end; f = f->next)
    {
        if (f->type == AND)
        {
            if (f->prev && f->next)
            {
                Filter *result = execute_filter_and(root, items, f->prev, f->next);
                if (result == NULL)
                {
                    goto error;
                }
                else
                {
                    f->prev->prev->next = result;
                    result->prev = f->prev->prev;
                    result->next = f->next->next;
                    f->next->next->prev = result;

                    cJSONUtils_Free_Filter(f->prev);
                    cJSONUtils_Free_Filter(f);
                    cJSONUtils_Free_Filter(f->next);
                    f = result;
                }
            }
            else
            {
                goto error;
            }
        }
    }

    // Then we handle ||
    for (f = start; f != end; f = f->next)
    {
        if (f->type == OR)
        {
            if (f->prev && f->next)
            {
                Filter *result = execute_filter_or(root, items, f->prev, f->next);
                if (result == NULL)
                {
                    goto error;
                }
                else
                {
                    f->prev->prev->next = result;
                    result->prev = f->prev->prev;
                    result->next = f->next->next;
                    f->next->next->prev = result;

                    cJSONUtils_Free_Filter(f->prev);
                    cJSONUtils_Free_Filter(f);
                    cJSONUtils_Free_Filter(f->next);
                    f = result;
                }
            }
            else
            {
                goto error;
            }
        }
    }

    if (start->type != LEFT_BRACKETS || end->type != RIGHT_BRACKETS || start->next->next != end)
    {
        goto error;
    }

    if (start->next->type != RESULT && (start->next->type == EXISTS || start->next->type == COMP))
    {
        Filter *result = execute_filter(root, items, start->next);
        cJSONUtils_Free_Filter(start->next);
        start->next = result;
        result->prev = start;
        result->next = end;
        end->prev = result;
    }

    return start->next;

error:
    return NULL;
}

static cJSON *filter_selector(const cJSON * const root, cJSON *items, Selector *selector)
{
    Filter *filter = cJSONUtils_Duplicate_Filter(selector->value.filter);
    Filter *head = filter;
    Filter *next = NULL;
    Filter *prev = NULL;
    while (filter != NULL)
    {
        next = filter->next;
        prev = filter;
        if (filter->type == RIGHT_BRACKETS)
        {
            while (prev->type != LEFT_BRACKETS)
            {
                prev = prev->prev;
            }
            Filter *result = execute_filter_unit(root, items, prev, filter);
            if (result == NULL)
            {
                goto error;
            }
            else {
                prev->prev->next = result;
                result->prev = prev->prev;
                result->next = next;
                if (next)
                {
                    next->prev = result;
                }
                cJSONUtils_Free_Filter(prev);
                cJSONUtils_Free_Filter(filter);
            }
        }

        filter = next;
    }

    if (head->next->next != NULL || head->next->type != RESULT)
    {
        goto error;
    }

    cJSON *new_items = cJSON_CreateArray();
    for (cJSON *r = head->next->value.results->child; r != NULL; r = r->next)
    {
        cJSON_AddItemToArray(new_items, cJSON_CreateObjectReference(r->child));
    }

    cJSONUtils_Delete_Filter(head);
    return new_items;

error:
    cJSONUtils_Delete_Filter(head);
    return cJSON_CreateArray();
}

static cJSON *get_item_from_selector(const cJSON * const object, const Selector *selector,
                                     const cJSON_bool case_sensitive, const cJSON_bool reference)
{
    if (selector == NULL)
    {
        return NULL;
    }
    cJSON *items = root_selector(object); // Automatically create ROOT selector.
    Selector *next_selector = (Selector *)selector;
    while (next_selector != NULL)
    {
        switch (next_selector->type) {
            case HEAD:
            case ROOT:
            {
                break; // do nothing
            }
            case DOT:
            {
                cJSON *new_items = dot_selector(items, next_selector->value.path, case_sensitive);
                cJSON_Delete(items);
                items = new_items;
                break;
            }
            case DOT_WILD:
            case INDEX_WILD:
            {
                cJSON *new_items = dot_index_wild_selector(items);
                cJSON_Delete(items);
                items = new_items;
                break;
            }
            case INDEX:
            {
                // We assume that for INDEX, only one operand is allowed, and it must be an array type.
                cJSON *array = items->child->child;
                if (cJSON_GetArraySize(items) != 1 || !cJSON_IsArray(array))
                {
                    goto error;
                }
                cJSON *new_items = index_selector(array, next_selector);
                cJSON_Delete(items);
                items = new_items;
                break;
            }
            case ARRAY_SLICE:
            {
                // We assume that for ARRAY_SLICE, only one operand is allowed, and it must be an array type.
                cJSON *array = items->child->child;
                if (cJSON_GetArraySize(items) != 1 || !cJSON_IsArray(array))
                {
                    goto error;
                }
                cJSON *new_items = array_slice_selector(array, next_selector);
                cJSON_Delete(items);
                items = new_items;
                break;
            }
            case DECENDANT:
            {
                cJSON *new_items = decendant_all_selector(items);
                cJSON_Delete(items);
                items = new_items;
                break;
            }
            case LIST:
            {
                // We assume that for LIST, only one operand is allowed, and it must be an array type.
                cJSON *array = items->child->child;
                if (cJSON_GetArraySize(items) != 1 || !cJSON_IsArray(array))
                {
                    goto error;
                }
                cJSON *new_items = list_selector(array, next_selector);
                cJSON_Delete(items);
                items = new_items;
                break;
            }
            case FILTER:
            {
                // execute split first
                if (cJSON_GetArraySize(items) != 1)
                {
                    goto error;
                }
                cJSON *new_items = dot_index_wild_selector(items);
                cJSON_Delete(items);
                items = new_items;

                new_items = filter_selector(object, items, next_selector);
                cJSON_Delete(items);
                items = new_items;
                break;
            }
        }

        if (cJSON_GetArraySize(items) == 0) {
            return items;
        }
        next_selector = next_selector->next;
    }

    if (reference)
    {
        return items;
    }
    
    cJSON *item = NULL;
    cJSON *dup_array = cJSON_CreateArray();
    cJSON_ArrayForEach(item, items)
    {
        cJSON_AddItemToArray(dup_array, cJSON_Duplicate(item->child, true));
    }
    cJSON_Delete(items);
    return dup_array;

error:
    cJSON_Delete(items);
    return cJSON_CreateArray();
}

CJSON_PUBLIC(cJSON *) cJSONUtils_GetSelector(const cJSON * const object, const Selector *selector)
{
    return get_item_from_selector(object, selector, true, false);
}

CJSON_PUBLIC(cJSON *) cJSONUtils_GetPath(const cJSON * const object, const char * const path)
{
    Selector *selector = compile_selector(path);
    if (selector == NULL)
    {
        return NULL;
    }
    cJSON *items = get_item_from_selector(object, selector, true, false);
    cJSONUtils_Delete_Selector(selector);
    return items;
}

CJSON_PUBLIC(cJSON *) cJSONUtils_GetPathReference(const cJSON * const object, const char * const path)
{
    Selector *selector = compile_selector(path);
    if (selector == NULL)
    {
        return NULL;
    }
    cJSON *items = get_item_from_selector(object, selector, true, true);
    cJSONUtils_Delete_Selector(selector);
    return items;
}
