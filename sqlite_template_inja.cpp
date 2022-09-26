
#include <sqlite3ext.h>
SQLITE_EXTENSION_INIT1

#include <inja.hpp>
#include <iostream>

using json = nlohmann::json;
using namespace std;

extern "C"
{
  // This needs to be callable from C
  int sqlite3_template_init(
      sqlite3 *db,
      char **pzErrMsg,
      const sqlite3_api_routines *pApi);
}

// this is the 'logic' of the extension and can be written in idiomatic C++
// exception handling etc can be done outside.
// TODO: maybe pass in an Environment? Keep an LRU of templates?
// It is probably expensive to parse the template for every function invocation
std::string template_render(string template_string, string json_data)
{
  json data = json::parse(json_data);

  return inja::render(template_string, data);
}

// This is the function that will be called from SQLite and it has to
// take care of a lot of the details of type-conversion, error-checking etc.
static void inja_func(
    sqlite3_context *context,
    int argc,
    sqlite3_value **argv)
{
  // TODO: replace with an error
  assert(argc == 2);

  // do some more soundess checking
  if (sqlite3_value_type(argv[0]) == SQLITE_NULL)
    return;

  std::string expanded, template_string, json_data;

  // hope that overloaded assignment operator will do the right thing.
  // see if there is excessive copying going on.
  template_string = (reinterpret_cast<const char *>(sqlite3_value_text(argv[0])));
  json_data = (reinterpret_cast<const char *>(sqlite3_value_text(argv[1])));
  // TODO: figure out when to return an error vs sqlite3_result_null
  try
  {
    expanded = template_render(template_string, json_data);
  }
  catch (inja::InjaError  e)
  {
    // how to do sprintf-like formatting?
    std::string message = "Inja exception: " + e.message + "(" + e.type + ")";
    // TODO: figure out how the memory management is working so as to avoid memory-leaks
    // "The sqlite3_result_error() and sqlite3_result_error16() routines make a
    //  private copy of the error message text before they return"
    sqlite3_result_error(context, message.data(), message.length());
    return;
  }
  catch (json::exception e) {
    std::string message = e.what();
    sqlite3_result_error(context, message.data(),message.length());
    return;
  }

  // TODO: deal with encodings, preferred encodings etc.
  sqlite3_result_text(context, expanded.data(), expanded.length(), SQLITE_TRANSIENT);
  // not sure if we have to do anything with freeing 'expanded'
  // I think it will be taken care of by the runtime simply by going out of scope
  // and that nothing has to be done to it explicitly.
  return;
}

int sqlite3_template_init(
    sqlite3 *db,
    char **pzErrMsg,
    const sqlite3_api_routines *pApi)
{
  int rc = SQLITE_OK;
  SQLITE_EXTENSION_INIT2(pApi);
  (void)pzErrMsg; /* Unused parameter */
  rc = sqlite3_create_function(db, "template_render", 2,
                               SQLITE_UTF8 | SQLITE_DETERMINISTIC,
                               0, inja_func, 0, 0);
  return rc;
}
