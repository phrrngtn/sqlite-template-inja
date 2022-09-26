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
```
sqlite> .load ./template
```
* extension exports just one function, `template_render` which takes an Inja template string and a JSON object and returns the 
template rendered wrt the JSON data.
```
sqlite> select template_render('what is a  {{fruit}}, if not perfection', json_object('fruit', 'banana'));
what is a  banana, if not perfection
```
  
Any errors at all (!) will cause the process to abort
```
sqlite> select template_render('what is a  {{fsdfuit}}, if not perfection', json_object('fruit', 'banana'));
terminate called after throwing an instance of 'inja::RenderError'
  what():  [inja.exception.render_error] (at 1:14) variable 'fsdfuit' not found
Aborted
```