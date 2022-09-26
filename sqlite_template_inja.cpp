
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
  assert(argc == 2);
  if (sqlite3_value_type(argv[0]) == SQLITE_NULL)
    return;

  std::string expanded, template_string, json_data;

  template_string = (reinterpret_cast<const char *>(sqlite3_value_text(argv[0])));
  json_data = (reinterpret_cast<const char *>(sqlite3_value_text(argv[1])));
  try
  {
    expanded = template_render(template_string, json_data);
  }
  catch (inja::RenderError e)
  {
    // how to do sprintf-like formatting?
    std::string message = "Inja exception: " + e.message + "(" + e.type + ")";
    // TODO: figure out how the memory management is working so as to avoid memory-leaks
    sqlite3_result_error(context, message.data(), message.length());
    return;
  }

  // How do we catch exceptions or check for errors
  // void sqlite3_result_error(sqlite3_context*, const char*, int);
  sqlite3_result_text(context, expanded.data(), expanded.length(), SQLITE_TRANSIENT);
  // not sure if we have to do anything with freeing 'expanded'
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
