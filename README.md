# sqlite-template-inja
SQLite extension for templating with Inja

```sql
sqlite> select template_render('my first choice fruit is {{fruit.0}} but I also like {{fruit.1}}', JSON_OBJECT("fruit", JSON_ARRAY("banana", "apple", "mango")));
```
```
my first choice fruit is banana but I also like apple
```
Very rough initial revision with crude error-checking and
exception-handling.

Developed on WSL but should run on anything with a modern C++
compiler due to the excellent portability of sqlite, inja and nlohmann::json.

Building
========
I followed the instructions in https://visitlab.pages.fi.muni.cz/tutorials/vs-code/index.html to use CMake and vcpkg.

For each remote machine (in my case wsl2 and Mac), I hand-edited the user-settings
for the remote machine e.g. `/Users/phrrngtn/.vscode-server/data/Machine/settings.json`
with this seemingly redundant setting but I was not able to get it to work using only
one.
```json
{
    "cmake.configureArgs": [
        "-DCMAKE_TOOLCHAIN_FILE=/Users/phrrngtn/work/vcpkg/scripts/buildsystems/vcpkg.cmake"
    ],
    "cmake.configureSettings": {
        "CMAKE_TOOLCHAIN_FILE": "/Users/phrrngtn/work/vcpkg/scripts/buildsystems/vcpkg.cmake"
    },
}
```

Here is how to do it by hand if you don't want to use VS code
```
cmake .. -DCMAKE_TOOLCHAIN_FILE=/home/phrrngtn/work/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-linux
```

* load
```sql
-- On Windows
sqlite> .load ./sqlite_template_inja sqlite3_template_init

-- On Linux
 sqlite> .load ./libsqlite_template_inja sqlite3_template_init

 -- On Mac
 sqlite> .load libsqlite_template_inja sqlite3_template_init

sqlite> select * FROM pragma_function_list where name = 'template_render';
template_render|0|s|utf8|2|2048
```
* extension exports just one function, `template_render` which takes an Inja template string and a JSON object and returns the
template rendered wrt the JSON data.
```sql
sqlite> select template_render('what is a  {{fruit}}, if not perfection', json_object('fruit', 'banana'));
```
```
what is a  banana, if not perfection
```

There is an optional third argument which if present will be interpreted as a JSON object with keys of `comment`, `expression`, `statement` and `line_statement` and used to construct an `inja::Environment` pointer with options set as per https://github.com/pantor/inja#template-rendering

For example, to use C-style comments in the template source, you would pass an object like this:
```sql
JSON_OBJECT('comment', json_array('/*', '*/'))
```

This mechanism from Inja was exposed to support templating of ODBC connection strings where curly braces ('{}') are 
used extensively and clash with the Inja default `expression` delimiters of `{{` and `}}`.

Error handling is still a bit rough
```sql
-- bogus JSON (passed in a string that looks like JSON but isn't)
sqlite> select template_render('what is a  {{}}, if not perfection', '{"fruitdsf", "banana"}');
```
```
Runtime error: [json.exception.parse_error.101] parse error at line 1, column 12: syntax error while parsing object separator - unexpected ','; expected ':'
```
```sql
-- fix the JSON (as a string literal) but with an invalid template
sqlite> select template_render('what is a  {{}}, if not perfection', '{"fruitdsf": "banana"}');
```
```
Runtime error: Inja exception: empty expression(render_error)
```
```sql
-- valid template structure but wrong key in the JSON
sqlite> select template_render('what is a  {{fruit}}, if not perfection', JSON_OBJECT("fruitdsf", "banana"));
```
```
Runtime error: Inja exception: variable 'fruit' not found(render_error)
```