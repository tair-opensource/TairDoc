/*
  Copyright (c) 2024 yangbodong22011

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

#include <stdio.h>
#include <stdlib.h>

#include "unity/src/unity.h"
#include "common.h"
#include "../cJSON_Utils.h"

static cJSON *parse_test_file(const char * const filename)
{
    char *file = NULL;
    cJSON *json = NULL;

    file = read_file(filename);
    TEST_ASSERT_NOT_NULL_MESSAGE(file, "Failed to read file.");

    json = cJSON_Parse(file);
    TEST_ASSERT_NOT_NULL_MESSAGE(json, "Failed to parse test json.");

    free(file);

    return json;
}

static cJSON_bool test_apply_jsonpath(const cJSON * const test)
{
    cJSON *name = NULL;
    cJSON *selector = NULL;
    cJSON *document = NULL;
    cJSON *result = NULL;

    cJSON *invalid_selector = NULL;
    cJSON_bool successful = false;
    Selector *selectors = NULL;
    cJSON *real_result = NULL;

    /* extract test name */
    name = cJSON_GetObjectItemCaseSensitive(test, "name");
    if (cJSON_IsString(name))
    {
        printf("Testing \"%s\"\n", name->valuestring);
    }
    else
    {
        printf("Testing unknown\n");
    }

    selector = cJSON_GetObjectItemCaseSensitive(test, "selector");
    TEST_ASSERT_NOT_NULL_MESSAGE(selector, "No \"selector\" in the test.");

    selectors = cJSONUtils_CompileSelector(selector->valuestring);
    invalid_selector = cJSON_GetObjectItemCaseSensitive(test, "invalid_selector");
    if (selectors == NULL || (invalid_selector != NULL && cJSON_IsTrue(invalid_selector)))
    {
        successful = true;
        goto end;
    }

    document = cJSON_GetObjectItemCaseSensitive(test, "document");
    TEST_ASSERT_NOT_NULL_MESSAGE(document, "No \"document\"in the test.");
    result = cJSON_GetObjectItemCaseSensitive(test, "result");
    TEST_ASSERT_NOT_NULL_MESSAGE(result, "No \"result\"in the test.");

    real_result = cJSONUtils_GetSelector(document, selectors);
//    cJSONUtils_SortObjectCaseSensitive(result);
//    cJSONUtils_SortObjectCaseSensitive(real_result);

    successful = cJSON_Compare(result, real_result, true);

end:
    cJSONUtils_Delete_Selector(selectors);
    cJSON_Delete(real_result);

    if (successful)
    {
        printf("OK\n");
    }
    else
    {
        printf("FAILED\n");
    }

    return successful;
}

static void cts_tests(void)
{
    cJSON *cts = parse_test_file("json-path-tests/jsonpath-compliance-test-suite/cts.json");
    cJSON *tests = cJSON_GetObjectItem(cts, "tests");
    cJSON *test = NULL;

    cJSON_bool failed = false;
    cJSON_ArrayForEach(test, tests)
    {
        test_apply_jsonpath(test);
    }

    cJSON_Delete(cts);

    TEST_ASSERT_FALSE_MESSAGE(failed, "Some tests failed.");
}

static void book_store_test_1(void)
{
    cJSON *book_store = parse_test_file("json-path-tests/book_store.json");
    cJSON *items = cJSONUtils_GetPath(book_store, "$.store.book[*].author");
    char *actual = cJSON_Print(items);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual, "Failed to print items, items may be null");

    char *expected = read_file("json-path-tests/book_store_test_1.expected");
    TEST_ASSERT_NOT_NULL_MESSAGE(expected, "Failed to read expected output.");

    TEST_ASSERT_EQUAL_STRING(expected, actual);

    cJSON_free(expected);
    cJSON_free(actual);
    cJSON_Delete(items);
    cJSON_Delete(book_store);
}

static void book_store_test_2(void)
{
    cJSON *book_store = parse_test_file("json-path-tests/book_store.json");
    cJSON *items = cJSONUtils_GetPath(book_store, "$..author");
    char *actual = cJSON_Print(items);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual, "Failed to print items, items may be null");

    char *expected = read_file("json-path-tests/book_store_test_2.expected");
    TEST_ASSERT_NOT_NULL_MESSAGE(expected, "Failed to read expected output.");

    TEST_ASSERT_EQUAL_STRING(expected, actual);

    cJSON_free(expected);
    cJSON_free(actual);
    cJSON_Delete(items);
    cJSON_Delete(book_store);
}

static void book_store_test_3(void)
{
    cJSON *book_store = parse_test_file("json-path-tests/book_store.json");
    cJSON *items = cJSONUtils_GetPath(book_store, "$.store.*");
    char *actual = cJSON_Print(items);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual, "Failed to print items, items may be null");

    char *expected = read_file("json-path-tests/book_store_test_3.expected");
    TEST_ASSERT_NOT_NULL_MESSAGE(expected, "Failed to read expected output.");

    TEST_ASSERT_EQUAL_STRING(expected, actual);

    cJSON_free(expected);
    cJSON_free(actual);
    cJSON_Delete(items);
    cJSON_Delete(book_store);
}

static void book_store_test_4(void)
{
    cJSON *book_store = parse_test_file("json-path-tests/book_store.json");
    cJSON *items = cJSONUtils_GetPath(book_store, "$.store..price");
    char *actual = cJSON_Print(items);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual, "Failed to print items, items may be null");

    char *expected = read_file("json-path-tests/book_store_test_4.expected");
    TEST_ASSERT_NOT_NULL_MESSAGE(expected, "Failed to read expected output.");

    TEST_ASSERT_EQUAL_STRING(expected, actual);

    cJSON_free(expected);
    cJSON_free(actual);
    cJSON_Delete(items);
    cJSON_Delete(book_store);
}

static void book_store_test_5(void)
{
    cJSON *book_store = parse_test_file("json-path-tests/book_store.json");
    cJSON *items = cJSONUtils_GetPath(book_store, "$..book[2]");
    char *actual = cJSON_Print(items);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual, "Failed to print items, items may be null");

    char *expected = read_file("json-path-tests/book_store_test_5.expected");
    TEST_ASSERT_NOT_NULL_MESSAGE(expected, "Failed to read expected output.");

    TEST_ASSERT_EQUAL_STRING(expected, actual);

    cJSON_free(expected);
    cJSON_free(actual);
    cJSON_Delete(items);
    cJSON_Delete(book_store);
}

static void book_store_test_6(void)
{
    cJSON *book_store = parse_test_file("json-path-tests/book_store.json");
    cJSON *items = cJSONUtils_GetPath(book_store, "$..book[-2]");
    char *actual = cJSON_Print(items);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual, "Failed to print items, items may be null");

    char *expected = read_file("json-path-tests/book_store_test_6.expected");
    TEST_ASSERT_NOT_NULL_MESSAGE(expected, "Failed to read expected output.");

    TEST_ASSERT_EQUAL_STRING(expected, actual);

    cJSON_free(expected);
    cJSON_free(actual);
    cJSON_Delete(items);
    cJSON_Delete(book_store);
}

static void book_store_test_7(void)
{
    cJSON *book_store = parse_test_file("json-path-tests/book_store.json");
    cJSON *items = cJSONUtils_GetPath(book_store, "$..book[0,1]");
    char *actual = cJSON_Print(items);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual, "Failed to print items, items may be null");

    char *expected = read_file("json-path-tests/book_store_test_7.expected");
    TEST_ASSERT_NOT_NULL_MESSAGE(expected, "Failed to read expected output.");

    TEST_ASSERT_EQUAL_STRING(expected, actual);

    cJSON_free(expected);
    cJSON_free(actual);
    cJSON_Delete(items);
    cJSON_Delete(book_store);
}

static void book_store_test_8(void)
{
    cJSON *book_store = parse_test_file("json-path-tests/book_store.json");
    cJSON *items = cJSONUtils_GetPath(book_store, "$..book[0:2:1]");
    char *actual = cJSON_Print(items);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual, "Failed to print items, items may be null");

    char *expected = read_file("json-path-tests/book_store_test_8.expected");
    TEST_ASSERT_NOT_NULL_MESSAGE(expected, "Failed to read expected output.");

    TEST_ASSERT_EQUAL_STRING(expected, actual);

    cJSON_free(expected);
    cJSON_free(actual);
    cJSON_Delete(items);
    cJSON_Delete(book_store);
}

static void book_store_test_9(void)
{
    cJSON *book_store = parse_test_file("json-path-tests/book_store.json");
    cJSON *items = cJSONUtils_GetPath(book_store, "$..book[?(@.isbn)]");
    char *actual = cJSON_Print(items);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual, "Failed to print items, items may be null");

    char *expected = read_file("json-path-tests/book_store_test_9.expected");
    TEST_ASSERT_NOT_NULL_MESSAGE(expected, "Failed to read expected output.");

    TEST_ASSERT_EQUAL_STRING(expected, actual);

    cJSON_free(expected);
    cJSON_free(actual);
    cJSON_Delete(items);
    cJSON_Delete(book_store);
}

static void book_store_test_10(void)
{
    cJSON *book_store = parse_test_file("json-path-tests/book_store.json");
    cJSON *items = cJSONUtils_GetPath(book_store, "$.store.book[?(@.price < 10)]");
    char *actual = cJSON_Print(items);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual, "Failed to print items, items may be null");

    char *expected = read_file("json-path-tests/book_store_test_10.expected");
    TEST_ASSERT_NOT_NULL_MESSAGE(expected, "Failed to read expected output.");

    TEST_ASSERT_EQUAL_STRING(expected, actual);

    cJSON_free(expected);
    cJSON_free(actual);
    cJSON_Delete(items);
    cJSON_Delete(book_store);
}

static void book_store_test_11(void)
{
    cJSON *book_store = parse_test_file("json-path-tests/book_store.json");
    cJSON *items = cJSONUtils_GetPath(book_store, "$..book[?((@.price == 12.99 || $.store.bicycle.price < @.price) || @.category == \"reference\")]");
    char *actual = cJSON_Print(items);
    TEST_ASSERT_NOT_NULL_MESSAGE(actual, "Failed to print items, items may be null");

    char *expected = read_file("json-path-tests/book_store_test_11.expected");
    TEST_ASSERT_NOT_NULL_MESSAGE(expected, "Failed to read expected output.");

    TEST_ASSERT_EQUAL_STRING(expected, actual);

    cJSON_free(expected);
    cJSON_free(actual);
    cJSON_Delete(items);
    cJSON_Delete(book_store);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(book_store_test_1);
    RUN_TEST(book_store_test_2);
    RUN_TEST(book_store_test_3);
    RUN_TEST(book_store_test_4);
    RUN_TEST(book_store_test_5);
    RUN_TEST(book_store_test_6);
    RUN_TEST(book_store_test_7);
    RUN_TEST(book_store_test_8);
    RUN_TEST(book_store_test_9);
    RUN_TEST(book_store_test_10);
    RUN_TEST(book_store_test_11);

    RUN_TEST(cts_tests);
    
    return UNITY_END();
}
