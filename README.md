# sqlite-template-inja
SQLite extension for templating with Inja

Very rough initial revision without error-checking nor
exception-handling.

Developed on WSL but should run on anything with a modern C++
compiler due to the excellent portability of sqlite, inja and nlohmann::json.

Building
========
The project needs sqlite3ext.h and inja.hpp

* build/install https://github.com/pantor/inja

* compile extension

  ```
  g++ -I ../sqlite-amalgamation-3390200 \
     -I/usr/local/include/inja \
     -fpic -shared \
     sqlite_template_inja.cpp -o template.so
  ```

* load
```sql
sqlite> .load ./template
```
* extension exports just one function, `template_render` which takes an Inja template string and a JSON object and returns the
template rendered wrt the JSON data.
```sql
sqlite> select template_render('what is a  {{fruit}}, if not perfection', json_object('fruit', 'banana'));
what is a  banana, if not perfection
```

Error handling is still a bit rough
```sql
-- bogus JSON (passed in a string that looks like JSON but isn't)
sqlite> select template_render('what is a  {{}}, if not perfection', '{"fruitdsf", "banana"}');
Runtime error: [json.exception.parse_error.101] parse error at line 1, column 12: syntax error while parsing object separator - unexpected ','; expected ':'

-- fix the JSON (as a string literal) but with an invalid template
sqlite> select template_render('what is a  {{}}, if not perfection', '{"fruitdsf": "banana"}');
Runtime error: Inja exception: empty expression(render_error)

-- valid template structure but wrong key in the JSON
sqlite> select template_render('what is a  {{fruit}}, if not perfection', JSON_OBJECT("fruitdsf", "banana"));
Runtime error: Inja exception: variable 'fruit' not found(render_error)
```