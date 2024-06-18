
## TairDoc [English introduction](README.md)
TairDoc 是一个支持 JSON 数据存储和查询的 Valkey 模块。

## 主要特性
- 支持 [RFC8259](https://datatracker.ietf.org/doc/html/rfc8259) JSON 标准。
- 支持 [RFC6901](https://datatracker.ietf.org/doc/html/rfc6901) JSONPointer 语法。
- 部分兼容 [RFC9535](https://datatracker.ietf.org/doc/rfc9535/) JSONPath 标准。（目前只有`JSON.GET`命令支持 JSONPath 语法）

## 依赖项目
TairDoc 依赖 [cJSON](https://github.com/DaveGamble/cJSON)，并再其之上实现了 JSONPath 语法，详见 src/cJSON/cJSON_Utils.[h|c]

## 快速开始
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
## 如何运行
```
make
```
编译成功后，会在当前目录下产生`tairdoc.so`文件。
```
./valkey-server --loadmodule /path/to/tairdoc.so
```

## 测试方法
修改 test 目录下 tairdoc.tcl 文件中的路径为：`set testmodule [file your_path/tairdoc.so]`

将 test 目录下 tairdoc.tcl 拷贝至 valkey 目录 tests 下
```  
cp test/tairdoc.tcl your_valkey_path/tests
```
然后在 valkey 根目录下运行 ./runtest --single tairdoc，得到下述结果表明运行成功。
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

## 客户端
| language | GitHub |
|----------|---|
| Java     |https://github.com/alibaba/alibabacloud-tairjedis-sdk|
| Python   |https://github.com/alibaba/tair-py|
| Go       |https://github.com/alibaba/tair-go|
| .Net     |https://github.com/alibaba/AlibabaCloud.TairSDK|

## API

### JSON.SET

- **语法**: `JSON.SET key path json [NX | XX]`
- **时间复杂度**: O(N)
- **命令描述**: 创建key并将JSON的值存储在对应的path中，若key及目标path已经存在，则更新对应的JSON值。
- **选项**:
    - key：TairDoc的key。
    - path：目标key的path，根元素支持`.`或`$`。
    - json：待新增或更新的JSON数据。
    - NX：当path不存在时写入。
    - XX：当path存在时写入。
- **返回值**:
    - 执行成功：OK。
    - 指定了XX且path不存在：nil。
    - 指定了NX且path已存在：nil。
    - 若返回`ERR could not find object to add, please check path`：表示您输入的path有误。
    - 其它情况返回相应的异常信息。

### JSON.GET

- **语法**: `JSON.GET key path`
- **时间复杂度**: O(N)
- **命令描述**: 获取目标key、path中存储的JSON数据。
- **选项**:
    - key：TairDoc的key。
    - path：目标key的path，支持JSONPath与JSONPointer语法。
- **返回值**:
    - 执行成功：对应的JSON数据。
    - 其它情况返回相应的异常信息。

### JSON.DEL

- **语法**: `JSON.DEL key path`
- **时间复杂度**: O(N)
- **命令描述**: 删除目标key中path对应的JSON数据。若未指定path，则删除整个key。若指定的key不存在或path不存在，则忽略该操作。

- **选项**:
  - **key**: TairDoc的key。
  - **path**: 目标key的path，用于指定需要删除的JSON数据的部分。

- **返回值**:
  - 执行成功：返回 `1`。
  - 执行失败：返回 `0`。
  - 若key不存在或path不存在：返回 `-1` 或者相应的异常信息。

### JSON.TYPE

- **语法**: `JSON.TYPE key path`
- **时间复杂度**: O(N)
- **命令描述**: 获取目标key中path对应值的类型，结果可能包括`boolean`、`string`、`number`、`array`、`object`、`null`等。

- **选项**:
  - **key**: TairDoc的key。
  - **path**: 目标key的path，用于指定需要查询类型的JSON数据的部分。

- **返回值**:
  - 执行成功：返回查询到的类型。
  - 执行失败：返回 `0`。
  - 若key或path不存在：返回 `nil`。
  - 其它情况返回相应的异常信息。

### JSON.NUMINCRBY

- **语法**: `JSON.NUMINCRBY key path value`
- **时间复杂度**: O(N)
- **命令描述**: 对目标key中path对应的值增加value，path对应的值和待增加的value必须是int或double类型。

- **选项**:
  - **key**: TairDoc的key。
  - **path**: 目标key的path。
  - **value**: 待增加的数值。

- **返回值**:
  - 执行成功：返回操作完成后path对应的值。
  - key或path不存在：返回错误。

### JSON.STRAPPEND

- **语法**: `JSON.STRAPPEND key path json-string`
- **时间复杂度**: O(N)
- **命令描述**: 在指定path对应值中追加json-string字符串，path对应值的类型也需要为字符串。

- **选项**:
  - **key**: TairDoc的key。
  - **path**: 目标key的path。
  - **json-string**: 待追加的字符串。

- **返回值**:
  - 执行成功：返回操作完成后path对应值的字符串长度。
  - key不存在：返回 `-1`。

### JSON.STRLEN

- **语法**: `JSON.STRLEN key path`
- **时间复杂度**: O(N)
- **命令描述**: 获取目标key中path对应值的字符串长度，path对应值的类型需要为字符串。

- **选项**:
  - **key**: TairDoc的key。
  - **path**: 目标key的path。

- **返回值**:
  - 执行成功：返回path对应值的字符串长度。
  - key不存在：返回 `-1`。

### JSON.ARRPOP

- **语法**: `JSON.ARRPOP key path [index]`
- **时间复杂度**: O(M*N)，M是key包含的子元素，N是数组元素数量。
- **命令描述**: 移除并返回path对应数组中指定位置的元素。

- **选项**:
  - **key**: TairDoc的key。
  - **path**: 目标key的path。
  - **index**: 数组的索引，若不传该参数默认为最后一个元素。

- **返回值**:
  - 执行成功：移除并返回该元素。
  - 数组为空：返回错误信息。

### JSON.ARRINSERT

- **语法**: `JSON.ARRINSERT key path [index] json [json ...]`
- **时间复杂度**: O(M*N)，M是要插入的元素数量，N是数组元素数量。
- **命令描述**: 将JSON插入到path对应的数组中，原有元素会往后移动。

- **选项**:
  - **key**: TairDoc的key。
  - **path**: 目标key的path。
  - **index**: 数组的索引。
  - **json**: 需要插入的数据。

- **返回值**:
  - 执行成功：返回操作完成后数组中的元素数量。
  - 数组为空：返回错误信息。

### JSON.ARRLEN

- **语法**: `JSON.ARRLEN key path`
- **时间复杂度**: O(N)
- **命令描述**: 获取path对应数组的长度。

- **选项**:
  - **key**: TairDoc的key。
  - **path**: 目标key的path。

- **返回值**:
  - 执行成功：返回数组的长度。
  - key不存在：返回 `-1`。

### JSON.ARRTRIM

- **语法**: `JSON.ARRTRIM key path start stop`
- **时间复杂度**: O(N)
- **命令描述**: 修剪目标key的path对应的数组，保留start至stop范围内的数据。

- **选项**:
  - **key**: TairDoc的key。
  - **path**: 目标key的path。
  - **start**: 修剪的开始位置。
  - **stop**: 修剪的结束位置。

- **返回值**:
  - 执行成功：返回操作完成后数组的长度。
  - key不存在：返回 `-1`。