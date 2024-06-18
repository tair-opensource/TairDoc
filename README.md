## TairDoc [中文介绍](README-CN.md)
TairDoc is a Valkey module that supports JSON data storage and querying.

## Main Features
- Supports [RFC8259](https://datatracker.ietf.org/doc/html/rfc8259) JSON standard.
- Supports [RFC6901](https://datatracker.ietf.org/doc/html/rfc6901) JSONPointer syntax.
- Partially compatible with [RFC9535](https://datatracker.ietf.org/doc/rfc9535/) JSONPath standard. (Currently, only the `JSON.GET` command supports JSONPath syntax)

## Dependent Projects
TairDoc depends on [cJSON](https://github.com/DaveGamble/cJSON) and has implemented JSONPath syntax on top of it, see src/cJSON/cJSON_Utils.[h|c]

## Quick Start
```
127.0.0.1:6379> JSON.SET key . '{"store":{"book":[{"category":"reference","author":"Nigel Rees","title":"Sayings of the Century","price":8.95},{"category":"fiction","author":"Evelyn Waugh","title":"Sword of Honour","price":12.99},{"category":"fiction","author":"Herman Melville","title":"Moby Dick","isbn":"0-553-21311-3","price":8.99},{"category":"fiction","author":"J. R. R. Tolkien","title":"The Lord of the Rings","isbn":"0-395-19395-8","price":22.99}],"bicycle":{"color":"red","price":19.95}},"expensive":10}'
OK
127.0.0.1:6379> JSON.GET key $.store.book[*].author
"[\"Nigel Rees\",\"Evelyn Waugh\",\"Herman Melville\",\"J. R. R. Tolkien\"]"
127.0.0.1:6379> JSON.GET key $.store..price
"[8.95,12.99,8.99,22.99,19.95]"
127.0.0.1:6379> JSON.GET key $..book[?(@.isbn)]
"[{\"category\":\"fiction\",\"author\":\"Herman Melville\",\"title\":\"Moby Dick\",\"isbn\":\"0-553-21311-3\",\"price\":8.99},{\"category\":\"fiction\",\"author\":\"J. R. R. Tolkien\",\"title\":\"The Lord of the Rings\",\"isbn\":\"0-395-19395-8\",\"price\":22.99}]"
```

## How to run
```
make
```
After a successful compilation, a `tairdoc.so` file will be generated in the current directory.
```
./valkey-server --loadmodule /path/to/tairdoc.so
```

## Run Test
Modify the path in the tairdoc.tcl file under the test directory to: `set testmodule [file your_path/tairdoc.so]`

Copy the tairdoc.tcl from the test directory to the valkey directory tests
```
cp test/tairdoc.tcl your_valkey_path/tests
```
Then run `./runtest --single tairdoc` in the valkey root directory, and the following result indicates a successful run.
```
...
[ok]: json.arrpush/json.arrpop master-slave (2 ms)
[ok]: json.arrinsert/json.arrtrim (2 ms)
[ok]: Check for memory leaks (pid 57468) (499 ms)
[ok]: Check for memory leaks (pid 57456) (457 ms)
[1/1 done]: tairdoc (5 seconds)

                   The End

Execution time of different units:
  5 seconds - tairdoc

\o/ All tests passed without errors!
```

## Clients
| language | GitHub |
|----------|---|
| Java     |[https://github.com/alibaba/alibabacloud-tairjedis-sdk](https://github.com/alibaba/alibabacloud-tairjedis-sdk)|
| Python   |[https://github.com/alibaba/tair-py](https://github.com/alibaba/tair-py)|
| Go       |[https://github.com/alibaba/tair-go](https://github.com/alibaba/tair-go)|
| .Net     |[https://github.com/alibaba/AlibabaCloud.TairSDK](https://github.com/alibaba/AlibabaCloud.TairSDK)|

## API

### JSON.SET

- **Syntax**: `JSON.SET key path json [NX | XX]`
- **Time Complexity**: O(N)
- **Command Description**: Creates a key and stores the JSON value at the corresponding path. If the key and the target path already exist, the corresponding JSON value is updated.
- **Options**:
    - key: The key of TairDoc.
    - path: The path of the target key, the root element supports `.` or `$`.
    - json: The JSON data to be added or updated.
    - NX: Write when the path does not exist.
    - XX: Write when the path exists.
- **Return Values**:
    - On success: OK.
    - Specified XX and the path does not exist: nil.
    - Specified NX and the path already exists: nil.
    - If it returns `ERR could not find object to add, please check path`: It means that the input path is incorrect.
    - Other situations return the corresponding exception information.

### JSON.GET

- **Syntax**: `JSON.GET key path`
- **Time Complexity**: O(N)
- **Command Description**: Gets the JSON data stored in the target key and path.
- **Options**:
    - key: The key of TairDoc.
    - path: The path of the target key, supporting JSONPath and JSONPointer syntax.
- **Return Values**:
    - On success: The corresponding JSON data.
    - Other situations return the corresponding exception information.

### JSON.DEL

- **Syntax**: `JSON.DEL key path`
- **Time Complexity**: O(N)
- **Command Description**: Deletes the JSON data corresponding to the path in the target key. If no path is specified, the entire key is deleted. If the specified key or path does not exist, the operation is ignored.

- **Options**:
    - **key**: The key of TairDoc.
    - **path**: The path of the target key, used to specify the part of the JSON data that needs to be deleted.

- **Return Values**:
    - On success: Returns `1`.
    - On failure: Returns `0`.
    - If the key or path does not exist: Returns `-1` or the corresponding exception information.

### JSON.TYPE

- **Syntax**: `JSON.TYPE key path`
- **Time Complexity**: O(N)
- **Command Description**: Gets the type of the value corresponding to the path in the target key, which may include `boolean`, `string`, `number`, `array`, `object`, `null`, etc.

- **Options**:
    - **key**: The key of TairDoc.
    - **path**: The path of the target key, used to specify the part of the JSON data whose type needs to be queried.

- **Return Values**:
    - On success: Returns the type found.
    - On failure: Returns `0`.
    - If the key or path does not exist: Returns `nil`.
    - Other situations return the corresponding exception information.

### JSON.NUMINCRBY

- **Syntax**: `JSON.NUMINCRBY key path value`
- **Time Complexity**: O(N)
- **Command Description**: Increases the value of the target key at the path by value, the value at the path and the value to be increased must be of type int or double.

- **Options**:
    - **key**: The key of TairDoc.
    - **path**: The path of the target key.
    - **value**: The value to be increased.

- **Return Values**:
    - On success: Returns the value at the path after the operation.
    - If the key or path does not exist: Returns an error.

### JSON.STRAPPEND

- **Syntax**: `JSON.STRAPPEND key path json-string`
- **Time Complexity**: O(N)
- **Command Description**: Appends the `json-string` to the value at the specified `path`, where the value type at the `path` must also be a string.

- **Options**:
    - **key**: The TairDoc key.
    - **path**: The target key path.
    - **json-string**: The string to be appended.

- **Return Values**:
    - On success: Returns the length of the string at the path after the operation.
    - If the key does not exist: Returns `-1`.

### JSON.STRLEN

- **Syntax**: `JSON.STRLEN key path`
- **Time Complexity**: O(N)
- **Command Description**: Retrieves the length of the string value at the specified `path` for the target key, where the value type at the path must be a string.

- **Options**:
    - **key**: The TairDoc key.
    - **path**: The target key path.

- **Return Values**:
    - On success: Returns the length of the string at the specified path.
    - If the key does not exist: Returns `-1`.

### JSON.ARRPOP

- **Syntax**: `JSON.ARRPOP key path [index]`
- **Time Complexity**: O(M*N), where M is the number of child elements contained in the key, and N is the number of elements in the array.
- **Command Description**: Removes and returns the element at the specified index in the array corresponding to the path.

- **Options**:
    - **key**: The TairDoc key.
    - **path**: The target key path.
    - **index** (optional): The index in the array. If not provided, defaults to the last element.

- **Return Values**:
    - On success: Removes and returns the element at the specified index.
    - If the array is empty: Returns an error message.

### JSON.ARRINSERT

- **Syntax**: `JSON.ARRINSERT key path [index] json [json ...]`
- **Time Complexity**: O(M*N), where M is the number of elements to be inserted, and N is the number of elements in the array.
- **Command Description**: Inserts JSON data into the array at the specified path, causing existing elements to shift backward.

- **Options**:
    - **key**: The TairDoc key.
    - **path**: The target key path.
    - **index**: The index in the array where the new elements will be inserted.
    - **json**: The data to be inserted.

- **Return Values**:
    - On success: Returns the number of elements in the array after the operation.
    - If the array is empty: Returns an error message.

### JSON.ARRLEN

- **Syntax**: `JSON.ARRLEN key path`
- **Time Complexity**: O(N)
- **Command Description**: Retrieves the length of the array corresponding to the path for the target key.

- **Options**:
    - **key**: The TairDoc key.
    - **path**: The target key path.

- **Return Values**:
    - On success: Returns the number of elements in the array.
    - If the key does not exist: Returns `-1`.

### JSON.ARRTRIM

- **Syntax**: `JSON.ARRTRIM key path start stop`
- **Time Complexity**: O(N)
- **Command Description**: Trims the array corresponding to the path of the target key, retaining only the elements within the specified range from start to stop.

- **Options**:
    - **key**: The TairDoc key.
    - **path**: The target key path.
    - **start**: The starting index for trimming the array.
    - **stop**: The ending index for trimming the array.

- **Return Values**:
    - On success: Returns the number of elements in the array after trimming.
    - If the key does not exist: Returns `-1`.