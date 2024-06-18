set testmodule [file normalize /Users/yangbodong/git/kvstore/TairDoc/tairdoc.so]

start_server {tags {"tairdoc"} overrides {bind 0.0.0.0}} {
    r module load $testmodule

    # -------------------------------------------------
    # json.set/json.get
    # -------------------------------------------------
    test {json.set not at root} {
        r del tairdockey
        catch {r json.set tairdockey {/abc} {"foo"}} err
        assert_match {*ERR*new objects must be created at the root*} $err
    }

    test {json.set nx or xx error} {
        r del tairdockey
        catch {r json.set tairdockey {/abc} {"foo"} abc} err
        assert_match {*ERR syntax error*} $err
    }

    test {json.set nx or xx error} {
        r del tairdockey
        assert_equal "" [r json.set tairdockey {/abc} {"foo"} xx]
    }

    test {json.set type error} {
        r del tairdockey
        r set tairdockey value
        catch {r json.set tairdockey {""} {""}} err
        assert_match {*WRONGTYPE Operation against*} $err
    }

    test {json.set json lexer error} {
        r del tairdockey
        catch {r json.set tairdockey {""} {{abc:abc}}} err
        assert_match {*ERR json lexer error at*} $err
    }

    test {json.set key not exists} {
        r del tairdockey
        assert_equal "" [r json.get tairdockey]
    }

    test {json.set pointer illegal} {
        r del tairdockey
        assert_equal "" [r json.get tairdockey]
    }

    test {add replaces any existing field} {
        r del tairdockey
        # NOTICE: In TCL, {} means that special characters will not be treated specially and will be
        # treated the same as ordinary characters, so you need to wrap the json string with {}
        assert_equal "OK" [r json.set tairdockey "" {{"foo": null}}]
        assert_equal "OK" [r json.set tairdockey {/foo} {1}]
        assert_equal {{"foo":1}} [r json.get tairdockey]
    }

    test {add replaces any existing field nx} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"foo": null}} nx]
        assert_equal "" [r json.set tairdockey "" {{"foo": null}} nx]
        assert_equal {{"foo":null}} [r json.get tairdockey]
    }

    test {toplevel array} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[]}]
        assert_equal "OK" [r json.set tairdockey /0 {"foo"}]
        assert_equal {["foo"]} [r json.get tairdockey]
    }

    test {toplevel array nx/xx} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[]} nx]
        assert_equal "OK" [r json.set tairdockey /0 {"foo"} nx]

        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[]}]
        assert_equal "" [r json.set tairdockey /0 {"foo"} xx]

        assert_equal {[]} [r json.get tairdockey]
    }

    test {toplevel object, numeric string} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{}}]
        assert_equal "OK" [r json.set tairdockey /foo {1}]
        assert_equal {{"foo":1}} [r json.get tairdockey]
    }

    test {toplevel object, numeric string nx/xx} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{}}]
        assert_equal "OK" [r json.set tairdockey /foo {1} nx]
        assert_equal {{"foo":1}} [r json.get tairdockey]
    }

    test {toplevel object, integer} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{}}]
        assert_equal "OK" [r json.set tairdockey /foo {1}]
        assert_equal {{"foo":1}} [r json.get tairdockey]
    }

    test {replace object document with array document?} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{}}]
        assert_equal "OK" [r json.set tairdockey "" {[]}]
        assert_equal {[]} [r json.get tairdockey]
    }

    test {replace array document with object document?} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[]}]
        assert_equal "OK" [r json.set tairdockey "" {{}}]
        assert_equal {{}} [r json.get tairdockey]
    }

    test {append to root array document?} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[]}]
        assert_equal "OK" [r json.set tairdockey /- {"hi"}]
        assert_equal {["hi"]} [r json.get tairdockey]
    }

    test {append to root array document? nx} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[]}]
        assert_equal "OK" [r json.set tairdockey /- {"hi"} nx]
        assert_equal "" [r json.set tairdockey /- {"hi"} xx]
    }

    test {Add, / target} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{}}]
        assert_equal "OK" [r json.set tairdockey / {1}]
        assert_equal {{"":1}} [r json.get tairdockey]
    }

    test {Add, /foo/ deep target (trailing slash)} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"foo":{}}}]
        assert_equal "OK" [r json.set tairdockey /foo/ {1}]
        assert_equal {{"foo":{"":1}}} [r json.get tairdockey]
    }

    test {Add composite value at top level} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"foo":1}}]
        assert_equal "OK" [r json.set tairdockey /bar {[1,2]}]
        assert_equal {{"foo":1,"bar":[1,2]}} [r json.get tairdockey]
    }

    test {Add into composite value} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"foo":1,"baz":[{"qux":"hello"}]}}]
        assert_equal "OK" [r json.set tairdockey /baz/0/foo {"world"}]
        assert_equal {{"foo":1,"baz":[{"qux":"hello","foo":"world"}]}} [r json.get tairdockey]
    }

    test {tairdoc} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"bar":[1,2]}}]
        catch {r json.set tairdockey /bar/8 {5}} err
        assert_match {*ERR*index*error*} $err
    }

    test {tairdoc} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"bar":[1,2]}}]
        catch {r json.set tairdockey /bar/8 {5}} err
        assert_match {*ERR*index*error*} $err
    }

    test {tairdoc} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"foo":1}}]
        assert_equal "OK" [r json.set tairdockey /bar {true}]
        assert_equal {{"foo":1,"bar":true}} [r json.get tairdockey]
    }

    test {tairdoc} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"foo":1}}]
        assert_equal "OK" [r json.set tairdockey /bar {false}]
        assert_equal {{"foo":1,"bar":false}} [r json.get tairdockey]
    }

    test {tairdoc} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"foo":1}}]
        assert_equal "OK" [r json.set tairdockey /bar {null}]
        assert_equal {{"foo":1,"bar":null}} [r json.get tairdockey]
    }

    test {0 can be an array index or object element name} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"foo":1}}]
        assert_equal "OK" [r json.set tairdockey /0 {"bar"}]
        assert_equal {{"foo":1,"0":"bar"}} [r json.get tairdockey]
    }

    test {tairdoc} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {["foo"]}]
        assert_equal "OK" [r json.set tairdockey /1 {"bar"}]
        assert_equal {["foo","bar"]} [r json.get tairdockey]
    }

    test {tairdoc arr set} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {["foo","sil"]}]
        assert_equal "OK" [r json.set tairdockey /1 {"bar"}]
        assert_equal {["foo","bar"]} [r json.get tairdockey]
    }

    test {tairdoc} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {["foo","sil"]}]
        assert_equal "OK" [r json.set tairdockey /0 {"bar"}]
        assert_equal {["bar","sil"]} [r json.get tairdockey]
    }

    test {push item to array via last index + 1} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {["foo","sil"]}]
        assert_equal "OK" [r json.set tairdockey /2 {"bar"}]
        assert_equal {["foo","sil","bar"]} [r json.get tairdockey]
    }

    test {tairdoc} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {["foo","sil"]}]
        catch {r json.set tairdockey /bar {42}} err
        assert_match {*ERR*index*error*} $err
    }

    test {value in array add not flattened} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {["foo","sil"]}]
        assert_equal "OK" [r json.set tairdockey /1 {["bar","baz"]}]
        assert_equal {["foo",["bar","baz"]]} [r json.get tairdockey]
    }

    test {replacing the root of the document is possible with add} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"foo":"bar"}}]
        assert_equal "OK" [r json.set tairdockey "" {{"baz":"qux"}}]
        assert_equal {{"baz":"qux"}} [r json.get tairdockey]
    }

    test {Adding to "/-" adds to the end of the array} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1,2]}]
        assert_equal "OK" [r json.set tairdockey /- {{"foo":["bar","baz"]}}]
        assert_equal {[1,2,{"foo":["bar","baz"]}]} [r json.get tairdockey]
    }

    test {Adding to "/-" adds to the end of the array, even n levels down} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1,2,[3,[4,5]]]}]
        assert_equal "OK" [r json.set tairdockey /2/1/- {{"foo":["bar","baz"]}}]
        assert_equal {[1,2,[3,[4,5,{"foo":["bar","baz"]}]]]} [r json.get tairdockey]
    }

    test {test add with bad number should fail} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {["foo","sil"]}]
        catch {r json.set tairdockey /1e0 {"bar"}} err
        assert_match {*ERR*index*error*} $err
    }

    test {Patch with different capitalisation than doc} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"foo":"bar"}}]
        assert_equal "OK" [r json.set tairdockey /FOO {"BAR"}]
        assert_equal {{"foo":"bar","FOO":"BAR"}} [r json.get tairdockey]
    }

    # replace
    test {tairdoc} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"foo":1,"baz":[{"qux":"hello"}]}}]
        assert_equal "OK" [r json.set tairdockey /foo {[1,2,3,4]}]
        assert_equal {{"baz":[{"qux":"hello"}],"foo":[1,2,3,4]}} [r json.get tairdockey]
    }

    test {tairdoc} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"foo":[1,2,3,4],"baz":[{"qux":"hello"}]}}]
        assert_equal "OK" [r json.set tairdockey /baz/0/qux {"world"}]
        assert_equal {{"foo":[1,2,3,4],"baz":[{"qux":"world"}]}} [r json.get tairdockey]
    }

    test {tairdoc} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {["foo"]}]
        assert_equal "OK" [r json.set tairdockey /0 {"bar"}]
        assert_equal {["bar"]} [r json.get tairdockey]
    }

    test {tairdoc} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[""]}]
        assert_equal "OK" [r json.set tairdockey /0 {0}]
        assert_equal {[0]} [r json.get tairdockey]
    }

    test {tairdoc} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[""]}]
        assert_equal "OK" [r json.set tairdockey /0 {true}]
        assert_equal {[true]} [r json.get tairdockey]
    }

    test {tairdoc} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[""]}]
        assert_equal "OK" [r json.set tairdockey /0 {false}]
        assert_equal {[false]} [r json.get tairdockey]
    }

    test {tairdoc} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[""]}]
        assert_equal "OK" [r json.set tairdockey /0 {null}]
        assert_equal {[null]} [r json.get tairdockey]
    }

    test {value in array replace not flattened} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {["foo","sil"]}]
        assert_equal "OK" [r json.set tairdockey /1 {["bar","baz"]}]
        assert_equal {["foo",["bar","baz"]]} [r json.get tairdockey]
    }

    test {replace whole document} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"foo":"bar"}}]
        assert_equal "OK" [r json.set tairdockey "" {{"baz":"qux"}}]
        assert_equal {{"baz":"qux"}} [r json.get tairdockey]
    }

    test {test replace with missing parent key should fail} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"bar":"baz"}}]
        catch {r json.set tairdockey /foo/bar {false}} err
        assert_match {*ERR could not find object*} $err
    }

    test {null value should be valid obj property to be replaced with something truthy} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{}}]
        assert_equal "OK" [r json.set tairdockey /foo {"truthy"}]
        assert_equal {{"foo":"truthy"}} [r json.get tairdockey]
    }

    test {null value should still be valid obj property replace other value} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"foo":"bar"}}]
        assert_equal "OK" [r json.set tairdockey /foo {null}]
        assert_equal {{"foo":null}} [r json.get tairdockey]
    }

    test {test replace with bad number should fail} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[""]}]
        catch {r json.set tairdockey /1e0 {false}} err
        assert_match {*ERR insert item in array error, index error*} $err
    }

    test {missing 'value' parameter to replace} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1]}]
        assert_equal "OK" [r json.set tairdockey /0 {null}]
        assert_equal {[null]} [r json.get tairdockey]
    }

    # -------------------------------------------------
    # json.del
    # -------------------------------------------------
    test {json.del key not exists} {
        r del tairdockey
        assert_equal "0" [r json.del tairdockey]
    }

    test {tairdoc del} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"bar":[1,2,3,4],"foo":1}}]
        assert_equal 1 [r json.del tairdockey /bar]
        assert_equal {{"foo":1}} [r json.get tairdockey]
    }

    test {tairdoc del empty fields should be deleted} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"bar":[1,2,3,4],"foo":1}}]
        assert_equal 1 [r json.del tairdockey /bar]
        assert_equal 1 [r json.del tairdockey /foo]
        assert_equal 0 [r exists tairdockey]
    }

    test {tairdoc del} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"foo":1,"baz":[{"qux":"hello"}]}}]
        assert_equal 1 [r json.del tairdockey /baz/0/qux]
        assert_equal {{"foo":1,"baz":[{}]}} [r json.get tairdockey]
    }

    test {null value should be valid obj property to be removed} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"foo":null}}]
        assert_equal 1 [r json.del tairdockey /foo]
        assert_equal {} [r json.get tairdockey]
    }

    test {test remove with bad number should fail} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"foo":1,"baz":[{"qux":"hello"}]}}]
        catch {r json.del tairdockey /baz/1e0/qux} err
        assert_match {*ERR*old item is null*} $err
    }

    test {test remove on array} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1,2,3,4]}]
        assert_equal 1 [r json.del tairdockey /0]
        assert_equal {[2,3,4]} [r json.get tairdockey]
    }

    test {test repeated removes} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1,2,3,4]}]
        assert_equal 1 [r json.del tairdockey /1]
        assert_equal 1 [r json.del tairdockey /2]
        assert_equal {[1,3]} [r json.get tairdockey]
    }

    test {test remove with bad index should fail} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1,2,3,4]}]
        catch {r json.del tairdockey /1e0} err
        assert_match {*ERR*old item is null*} $err
    }

    test {Removing nonexistent field} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"foo":"bar"}}]
        catch {r json.del tairdockey /baz} err
        assert_match {*ERR*old item is null*} $err
    }

    test {Removing nonexistent index} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {["foo","bar"]}]
        catch {r json.del tairdockey /2} err
        assert_match {*ERR*old item is null*} $err
    }

    # -------------------------------------------------
    # json.type
    # -------------------------------------------------

    test {json.type key not exists} {
        r del tairdockey
        assert_equal "" [r json.type tairdockey]
    }

    test {json.type path not exists} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {false}]
        assert_equal "" [r json.type tairdockey /a]
    }

    test {json.type boolean} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {false}]
        assert_equal "boolean" [r json.type tairdockey]
    }

    test {json.type null} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {null}]
        assert_equal "null" [r json.type tairdockey]
    }

    test {json.type number} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {1}]
        assert_equal "number" [r json.type tairdockey]
    }

    test {json.type string} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {"foo"}]
        assert_equal "string" [r json.type tairdockey]
    }

    test {json.type array} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1,2,3]}]
        assert_equal "array" [r json.type tairdockey]
    }

    test {json.type object} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"foo":"bar"}}]
        assert_equal "object" [r json.type tairdockey]
    }

    # -------------------------------------------------
    # json.incrby/json.incrbyfloat
    # -------------------------------------------------
    test {json.incrby error} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {0}]
        catch {r json.incrby tairdockey 3.2} err
        assert_match {*ERR value is not an integer*} $err
    }

    test {json.incrby zero} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {0}]
        assert_equal "0" [r json.incrby tairdockey 0]
    }

    test {json.incrby negative number} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {-10}]
        assert_equal "-20" [r json.incrby tairdockey -10]
    }

    test {json.incrby positive number} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {10}]
        assert_equal "20" [r json.incrby tairdockey 10]
    }

    test {json.incrby negative to positive number} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {-10}]
        assert_equal "10" [r json.incrby tairdockey 20]
    }

    test {json.incrby max} {
        # range is [-2^53, 2^53] [-9007199254740992, 9007199254740992]
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {9007199254740991}]
        assert_equal "9.00719925474099e+15" [r json.incrby tairdockey 1]
    }

    test {json.incrby min} {
        # range is [-2^53, 2^53] [-9007199254740992, 9007199254740992]
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {-9007199254740991}]
        assert_equal "-9.00719925474099e+15" [r json.incrby tairdockey -1]
    }

    test {json.incrby min} {
        # range is [-2^53, 2^53] [-9007199254740992, 9007199254740992]
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {-9007199254740991}]
        assert_equal "-9.00719925474099e+15" [r json.incrby tairdockey -1]
    }

    test {json.incrbyfloat zero} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {0.0}]
        assert_equal "0" [r json.incrbyfloat tairdockey 0.0]
    }

    test {json.incrbyfloat negative number} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {-10}]
        assert_equal "-20.12345" [r json.incrbyfloat tairdockey -10.12345]
    }

    test {json.incrbyfloat positive number} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {10}]
        assert_equal "20.12345" [r json.incrbyfloat tairdockey 10.12345]
    }

    test {json.incrbyfloat negative to positive number} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {-10}]
        assert_match "10.1234*" [r json.incrbyfloat tairdockey 20.12345]
    }

    test {json.incrbyfloat max} {
        # range is [-2^53, 2^53] [-9007199254740992, 9007199254740992]
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {1.7E307}]
        assert_equal "1.7e+307" [r json.incrbyfloat tairdockey 0]
    }

    test {json.incrbyfloat min} {
        # range is [-2^53, 2^53] [-9007199254740992, 9007199254740992]
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {-1.7E307}]
        assert_equal "-1.7e+307" [r json.incrbyfloat tairdockey 0]
    }

    # -------------------------------------------------
    # json.strappend/json.strlen
    # -------------------------------------------------
    test {json.strappend empty} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {""}]
        assert_equal 0 [r json.strappend tairdockey ""]
    }

    test {json.strappend string} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {"foo"}]
        assert_equal 3 [r json.strappend tairdockey ""]
    }

    test {json.strappend string} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {"foo"}]
        assert_equal 6 [r json.strappend tairdockey "bar"]
    }

    test {json.strappend string} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {""}]
        assert_equal 3 [r json.strappend tairdockey "bar"]
    }

    test {json.strlen empty} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {""}]
        assert_equal 0 [r json.strlen tairdockey]
    }

    test {json.strlen string} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {"foo"}]
        assert_equal 3 [r json.strlen tairdockey]
    }

    # -------------------------------------------------
    # json.arrpush/json.arrpop
    # -------------------------------------------------
    test {json.arrpush key not exist} {
        r del tairdockey
        assert_equal "-1" [r json.arrpush tairdockey "" {"foo"}]
    }

    test {json.arrpush path not exist} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1, 2]}]
        catch {r json.arrpush tairdockey "/a" 1} err
        assert_match {*ERR node not exists or not array*} $err
    }

    test {json.arrpush single json} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1, 2]}]
        assert_equal "3" [r json.arrpush tairdockey "" 3]
        assert_equal {[1,2,3]} [r json.get tairdockey]
    }

    test {json.arrpush multi json} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1, 2]}]
        assert_equal "5" [r json.arrpush tairdockey "" 3 4 5]
        assert_equal {[1,2,3,4,5]} [r json.get tairdockey]
    }

    test {json.arrpop key not exist} {
        r del tairdockey
        catch {r json.arrpop tairdockey ""} err
        assert_match {*ERR no such key*} $err
    }

    test {json.arrpop path not exist} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1, 2]}]
        catch {r json.arrpop tairdockey "/a" 1} err
        assert_match {*ERR node not exists or not array*} $err
    }

    test {json.arrpop no index} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1, 2]}]
        assert_equal "2" [r json.arrpop tairdockey ""]
    }

    test {json.arrpop positive index} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1, 2, 3]}]
        assert_equal "3" [r json.arrpop tairdockey "" 2]
        assert_equal {[1,2]} [r json.get tairdockey]
    }

    test {json.arrpop negative index} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1, 2, 3]}]
        assert_equal "2" [r json.arrpop tairdockey "" -2]
        assert_equal {[1,3]} [r json.get tairdockey]
    }

    test {json.arrpop index error} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1, 2, 3]}]
        catch {r json.arrpop tairdockey "" 3} err
        assert_match {*ERR array index outflow*} $err
    }

    # -------------------------------------------------
    # json.arrinsert/json.arrtrim
    # -------------------------------------------------
    test {json.arrinsert index} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1, 2, 3]}]
        assert_equal "5" [r json.arrinsert tairdockey "" 2 4 5]
        assert_equal {[1,2,4,5,3]} [r json.get tairdockey]
    }

    test {json.arrinsert index} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1, 2, 3]}]
        assert_equal "5" [r json.arrinsert tairdockey "" 0 4 5]
        assert_equal {[4,5,1,2,3]} [r json.get tairdockey]
    }

    test {json.arrinsert index} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1, 2, 3]}]
        assert_equal "5" [r json.arrinsert tairdockey "" 3 4 5]
        assert_equal {[1,2,3,4,5]} [r json.get tairdockey]
    }

    test {json.arrtrim from begin} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1, 2, 3]}]
        assert_equal "1" [r json.arrtrim tairdockey "" 0 0]
        assert_equal {[1]} [r json.get tairdockey]
    }

    test {json.arrtrim end} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1, 2, 3]}]
        assert_equal "1" [r json.arrtrim tairdockey "" 2 2]
        assert_equal {[3]} [r json.get tairdockey]
    }

    test {json.arrtrim begin to middle} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1, 2, 3]}]
        assert_equal "2" [r json.arrtrim tairdockey "" 0 1]
        assert_equal {[1,2]} [r json.get tairdockey]
    }

    test {json.arrtrim middle to end} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1, 2, 3]}]
        assert_equal "2" [r json.arrtrim tairdockey "" 1 2]
        assert_equal {[2,3]} [r json.get tairdockey]
    }

    test {json.arrtrim middle to middle} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1, 2, 3, 4]}]
        assert_equal "2" [r json.arrtrim tairdockey "" 1 2]
        assert_equal {[2,3]} [r json.get tairdockey]
    }

    # -------------------------------------------------
    # tairdoc rdb and aof
    # -------------------------------------------------
    test {tairdoc rdb} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"foo": "bar"}}]

        r bgsave
        waitForBgsave r
        r debug reload

        assert_equal {{"foo":"bar"}} [r json.get tairdockey]
    }

    test {tairdoc aof} {
        r config set aof-use-rdb-preamble no
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"foo": "bar"}}]
        assert_equal "OK" [r json.set tairdockey "" {{"foo": "car"}}]

        r bgrewriteaof
        waitForBgrewriteaof r
        r debug loadaof

        assert_equal {{"foo":"car"}} [r json.get tairdockey]
    }

    # -------------------------------------------------
    # tairdoc dump and restore
    # -------------------------------------------------
    test {dump/restore} {
         r del tairdockey
         assert_equal "OK" [r json.set tairdockey "" {{"foo":"bar"}}]

         set dump [r dump tairdockey]
         r del tairdockey

         assert_equal "OK" [r restore tairdockey 0 $dump]
         assert_equal {{"foo":"bar"}} [r json.get tairdockey]
    }


    # -------------------------------------------------
    # tairdoc json.arrlen & json.mget
    # -------------------------------------------------
    test {json.arrlen} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {[1, 2, 3]}]
        assert_equal 3 [r json.arrlen tairdockey]
    }

    test {json.mget} {
        r del tairdockey
        r del tairdockey2
        assert_equal "OK" [r json.set tairdockey "" {{"foo":"bar"}}]
        assert_equal "OK" [r json.set tairdockey2 "" {[1, 2, 3]}]
        assert_equal {{{"foo":"bar"}} {[1,2,3]}} [r json.mget tairdockey tairdockey2 ""]
    }

    # -------------------------------------------------
    # test from RedisJSON
    # -------------------------------------------------
    test {tairdoc json path set} {
        r del tairdockey
        assert_equal "OK" [r json.set tairdockey "" {{"Title":"View from 15th Floor","Width":800,"Animated":false,"Thumbnail":{"Url":"http://www.example.com/image/481989943","Height":125,"Width":100},"IDs":[116,943,234,38793]}}]
        assert_equal {"View from 15th Floor"} [r json.get tairdockey .Title]
        assert_equal {125} [r json.get tairdockey .Thumbnail.Height]
        assert_equal {234} [r json.get tairdockey {.IDs[2]}]
    }

    test {tairdoc testSetReplaceRootShouldSucceed} {
        r del test
        assert_equal "OK" [r json.set test . {{"basic": {"string": "string value","none": null,"bool": true,"int": 42,"num": 4.2,"arr": [42, null, -1.2, false, ["sub", "array"], {"subdict": true}],"dict": {"a": 1,"b": 2,"c": null}}}}]
        assert_equal "OK" [r json.set test . {{"simple": {"foo": "bar"}}}]
        assert_equal {{"simple":{"foo":"bar"}}} [r json.get test]
    }

    test {tairdoc testSetBehaviorModifyingSubcommands} {
        r del test

        assert_equal {} [r json.set test . {""} XX]
        assert_equal "OK" [r json.set test . {""} NX]
        assert_equal {} [r json.set test . {""} NX]
        assert_equal "OK" [r json.set test . {""} XX]
    }

    test {tairdoc testMgetCommand} {
        for {set j 0} {$j < 5} {incr j} {
            r del doc:$j
            assert_equal "OK" [r json.set doc:$j . $j]
        }

        assert_equal {0 1 2 3 4} [r json.mget doc:0 doc:1 doc:2 doc:3 doc:4 .]

        r del test
        assert_equal "OK" [r json.set test . {{"bool":false}}]
        assert_equal {false {} {}} [r json.mget test doc:0 foo .bool]
    }

    test {tairdoc testDelCommand} {
        r flushall
        assert_equal "OK" [r json.set test . {""}]
        assert_equal 1 [r json.del test .]

        assert_equal "OK" [r json.set test . {{}}]
        assert_equal "OK" [r json.set test .foo {"bar"}]
        assert_equal "OK" [r json.set test .baz {"qux"}]
        assert_equal 1 [r json.del test .baz]
        assert_equal 1 [r json.del test .foo]

        assert_equal "OK" [r json.set test . {{}}]
        assert_equal "OK" [r json.set test .foo {"bar"}]
        assert_equal "OK" [r json.set test .baz {"qux"}]
        assert_equal "OK" [r json.set test .arr {[1.2,1,2]}]
        assert_equal 1 [r json.del test {.arr[1]}]
        assert_equal 2 [r json.arrlen test .arr]
        assert_equal 1 [r json.del test .arr]
        assert_equal 1 [r json.del test .]
        assert_equal {} [r json.get test]
    }

    test {tairdoc testObjectCRUD} {
        r flushall
        assert_equal "OK" [r json.set test . {{}}]
        assert_equal "object" [r json.type test .]
        assert_equal {{}} [r json.get test ]

        assert_equal "OK" [r json.set test .foo {"bar"}]
        assert_equal {{"foo":"bar"}} [r json.get test]

        assert_equal "OK" [r json.set test .foo {"baz"}]
        assert_equal {{"foo":"baz"}} [r json.get test]

        assert_equal "OK" [r json.set test .boo {"far"}]
        assert_equal {{"foo":"baz","boo":"far"}} [r json.get test]

        assert_equal 1 [r json.del test .foo]
        assert_equal {{"boo":"far"}} [r json.get test]

        assert_equal "OK" [r json.set test . {{"foo": "bar"}}]
        assert_equal {{"foo":"bar"}} [r json.get test]

        assert_equal 1 [r json.del test .]
        assert_equal {} [r json.get test]
    }

    test {tairdoc testObjectCRUD} {
        r flushall

        assert_equal "OK" [r json.set test . {[]}]
        assert_equal "array" [r json.type test .]
        assert_equal 0 [r json.arrlen test .]

        assert_equal 1 [r json.arrappend test . 1]
        assert_equal 1 [r json.arrlen test .]
        assert_equal 2 [r json.arrinsert test . 0 -1]
        assert_equal 2 [r json.arrlen test .]
        assert_equal {[-1,1]} [r json.get test .]
        assert_equal 3 [r json.arrinsert test . -1 0]
        assert_equal {[-1,0,1]} [r json.get test .]
        assert_equal 5 [r json.arrinsert test . -3 -3 -2]
        assert_equal {[-3,-2,-1,0,1]} [r json.get test .]
        assert_equal 7 [r json.arrappend test . 2 3]
        assert_equal {[-3,-2,-1,0,1,2,3]} [r json.get test .]

        assert_equal "OK" [r json.set test {[0]} {"-inf"}]
        assert_equal "OK" [r json.set test {[6]} {"+inf"}]
        assert_equal "OK" [r json.set test {[3]} {null}]
        assert_equal {["-inf",-2,-1,null,1,2,"+inf"]} [r json.get test .]

        assert_equal 1 [r json.del test {[1]}]
        assert_equal 1 [r json.del test {[4]}]
        assert_equal {["-inf",-1,null,1,"+inf"]} [r json.get test .]

        assert_equal 4 [r json.arrtrim test . 1 4]
        assert_equal {[-1,null,1,"+inf"]} [r json.get test .]
        assert_equal 3 [r json.arrtrim test . 0 2]
        assert_equal {[-1,null,1]} [r json.get test .]
        assert_equal 1 [r json.arrtrim test . 1 1]
        assert_equal {[null]} [r json.get test .]

        assert_equal "OK" [r json.set test . {[true]}]
        assert_equal "array" [r json.type test .]
        assert_equal 1 [r json.arrlen test .]
        assert_equal true [r json.get test {[0]}]
    }
    
    # -------------------------------------------------
    # test for JSONPath
    # -------------------------------------------------    
    test {tairdoc jsonpointer} {
        r flushall
        
        assert_equal "OK" [r json.set key . {{"store":{"book":[{"category":"reference","author":"Nigel Rees","title":"Sayings of the Century","price":8.95},{"category":"fiction","author":"Evelyn Waugh","title":"Sword of Honour","price":12.99},{"category":"fiction","author":"Herman Melville","title":"Moby Dick","isbn":"0-553-21311-3","price":8.99},{"category":"fiction","author":"J. R. R. Tolkien","title":"The Lord of the Rings","isbn":"0-395-19395-8","price":22.99}],"bicycle":{"color":"red","price":19.95}},"expensive":10}}]
        assert_equal {["Nigel Rees","Evelyn Waugh","Herman Melville","J. R. R. Tolkien"]} [r json.get key $.store.book\[*\].author]
        assert_equal {["Nigel Rees","Evelyn Waugh","Herman Melville","J. R. R. Tolkien"]} [r json.get key $..author]
        assert_equal {[[{"category":"reference","author":"Nigel Rees","title":"Sayings of the Century","price":8.95},{"category":"fiction","author":"Evelyn Waugh","title":"Sword of Honour","price":12.99},{"category":"fiction","author":"Herman Melville","title":"Moby Dick","isbn":"0-553-21311-3","price":8.99},{"category":"fiction","author":"J. R. R. Tolkien","title":"The Lord of the Rings","isbn":"0-395-19395-8","price":22.99}],{"color":"red","price":19.95}]} [r json.get key $.store.*]
        assert_equal {[8.95,12.99,8.99,22.99,19.95]} [r json.get key $.store..price]
        assert_equal {[{"category":"fiction","author":"Herman Melville","title":"Moby Dick","isbn":"0-553-21311-3","price":8.99}]} [r json.get key $..book\[2\]]
        assert_equal {[{"category":"fiction","author":"Herman Melville","title":"Moby Dick","isbn":"0-553-21311-3","price":8.99}]} [r json.get key $..book\[-2\]]
        assert_equal {[{"category":"reference","author":"Nigel Rees","title":"Sayings of the Century","price":8.95},{"category":"fiction","author":"Evelyn Waugh","title":"Sword of Honour","price":12.99}]} [r json.get key $..book\[0,1\]]
        assert_equal {[{"category":"reference","author":"Nigel Rees","title":"Sayings of the Century","price":8.95},{"category":"fiction","author":"Evelyn Waugh","title":"Sword of Honour","price":12.99}]} [r json.get key $..book\[0:2:1\]]
        assert_equal {[{"category":"fiction","author":"Herman Melville","title":"Moby Dick","isbn":"0-553-21311-3","price":8.99},{"category":"fiction","author":"J. R. R. Tolkien","title":"The Lord of the Rings","isbn":"0-395-19395-8","price":22.99}]} [r json.get key $..book\[?(@.isbn)\]]
        assert_equal {[{"category":"reference","author":"Nigel Rees","title":"Sayings of the Century","price":8.95},{"category":"fiction","author":"Herman Melville","title":"Moby Dick","isbn":"0-553-21311-3","price":8.99}]} [r json.get key "$.store.book\[?(@.price < 10)\]"]
        assert_equal {[{"category":"reference","author":"Nigel Rees","title":"Sayings of the Century","price":8.95},{"category":"fiction","author":"J. R. R. Tolkien","title":"The Lord of the Rings","isbn":"0-395-19395-8","price":22.99},{"category":"fiction","author":"Evelyn Waugh","title":"Sword of Honour","price":12.99}]} [r json.get key "$..book\[?((@.price == 12.99 || $.store.bicycle.price < @.price) || @.category == 'reference')\]"]
    }    
}

start_server {tags {"ex_json"} overrides {bind 0.0.0.0}} {
    r module load $testmodule
    set slave [srv 0 client]
    set slave_host [srv 0 host]
    set slave_port [srv 0 port]
    set slave_log [srv 0 stdout]

    start_server {tags {"ex_json"} overrides {bind 0.0.0.0}} {
        r module load $testmodule
        set master [srv 0 client]
        set master_host [srv 0 host]
        set master_port [srv 0 port]

        $slave slaveof $master_host $master_port

        wait_for_condition 50 100 {
            [lindex [$slave role] 0] eq {slave} &&
            [string match {*master_link_status:up*} [$slave info replication]]
        } else {
            fail "Can't turn the instance into a replica"
        }

        test {json.set/json.get master-slave} {
            $master del tairdockey

            assert_equal "OK" [$master json.set tairdockey "" {{"foo": null}}]
            assert_equal "OK" [$master json.set tairdockey {/foo} {1}]

            $master WAIT 1 5000

            assert_equal {{"foo":1}} [$slave json.get tairdockey]
        }

        test {expire master-slave} {
            $master del tairdockey

            assert_equal "OK" [$master json.set tairdockey "" {{"foo": null}}]
            assert_equal {{"foo":null}} [$master json.get tairdockey]

            $master expire tairdockey 1
            after 2000
            assert_equal "" [$slave json.get tairdockey]
        }

        test {json.del master-slave} {
            $master del tairdockey

            assert_equal "OK" [$master json.set tairdockey "" {{"bar":[1,2,3,4],"foo":1}}]
            $master WAIT 1 5000
            assert_equal {1}  [$slave json.get tairdockey /foo]

            assert_equal 1 [$master json.del tairdockey /bar]
            $master WAIT 1 5000
            assert_equal {{"foo":1}} [$slave json.get tairdockey]

            assert_equal 1 [$master json.del tairdockey]
            $master WAIT 1 5000
            assert_equal 0 [$slave exists tairdockey]
        }

        test {json.incrby/json.incrbyfloat master-slave} {
            $master del tairdockey
            $master del tairdockey2

            assert_equal "OK" [$master json.set tairdockey "" {10}]
            assert_equal "OK" [$master json.set tairdockey2 "" {10.3}]
            assert_equal "20" [$master json.incrby tairdockey 10]
            assert_equal "20.3" [$master json.incrbyfloat tairdockey2 10]
            $master WAIT 1 5000
            assert_equal "20" [$slave json.get tairdockey]
            assert_equal "20.3" [$slave json.get tairdockey2]
        }

        test {json.strappend master-slave} {
            $master del tairdockey

            assert_equal "OK" [$master json.set tairdockey "" {"foo"}]
            $master WAIT 1 5000
            assert_equal {"foo"} [$slave json.get tairdockey]

            assert_equal 6 [r json.strappend tairdockey "bar"]
            $master WAIT 1 5000
            assert_equal {"foobar"} [$slave json.get tairdockey]
        }

        test {json.arrpush/json.arrpop master-slave} {
            $master del tairdockey

            assert_equal "OK" [$master json.set tairdockey "" {[1, 2]}]
            $master WAIT 1 5000
            assert_equal {[1,2]} [$slave json.get tairdockey]

            assert_equal "3" [$master json.arrpush tairdockey "" 3]
            $master WAIT 1 5000
            assert_equal {[1,2,3]} [$slave json.get tairdockey]

            assert_equal "3" [$master json.arrpop tairdockey ""]
            $master WAIT 1 5000
            assert_equal {[1,2]} [$slave json.get tairdockey]
        }

        test {json.arrinsert/json.arrtrim} {
            $master del tairdockey
            assert_equal "OK" [$master json.set tairdockey "" {[1, 2, 3]}]
            assert_equal "5" [$master json.arrinsert tairdockey "" 2 4 5]
            $master WAIT 1 5000
            assert_equal {[1,2,4,5,3]} [$slave json.get tairdockey]

            assert_equal "2" [$master json.arrtrim tairdockey "" 0 1]
            $master WAIT 1 5000
            assert_equal {[1,2]} [$slave json.get tairdockey]
        }
    }
}
