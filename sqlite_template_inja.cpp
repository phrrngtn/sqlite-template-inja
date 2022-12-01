
#include <sqlite3ext.h>
SQLITE_EXTENSION_INIT1

#include <inja/inja.hpp>
#include <iostream>

#ifdef WIN32
#define SQLITE_EXTENSION_ENTRY_POINT __declspec(dllexport)
#else
#define SQLITE_EXTENSION_ENTRY_POINT
#endif

using json = nlohmann::json;
using namespace std;

// This needs to be callable from C
extern "C" SQLITE_EXTENSION_ENTRY_POINT int sqlite3_template_init(
    sqlite3 *db,
    char **pzErrMsg,
    const sqlite3_api_routines *pApi);

void free_inja_environment(void *p) {
    //cerr << "free environment" << endl;
    delete (inja::Environment *)p;
}

void free_inja_template(void *p) {
  //cerr << "free template" << endl;
    delete (inja::Template *)p;
}

// This is the function that will be called from SQLite and it has to
// take care of a lot of the details of type-conversion, error-checking etc.
static void inja_func(
    sqlite3_context *context,
    int argc,
    sqlite3_value **argv)
{
   assert(argc >= 2);

   if (sqlite3_value_type(argv[0]) == SQLITE_NULL ||
        sqlite3_value_type(argv[1]) == SQLITE_NULL) {
            sqlite3_result_null(context);
            return;
        }   

  std::string expanded, template_string, json_data, template_options;
  int save_env=0;
  int save_template=0;

  inja::Environment *env;
  inja::Template *temp;
  env = (inja::Environment *)sqlite3_get_auxdata(context,2);
  temp = (inja::Template *)sqlite3_get_auxdata(context,0);


  if (env == nullptr){
    save_env=1;
    env = new inja::Environment();

    // not sure what happens if we try and access an argv beyond what is provided
    // This test is supposed to read as "We have been provided with at least three arguments and the third argument is
    // not null and is a string". The "at least three" as opposed to "exactly three" is that we may add more arguments into the
    // signature later. Although SQLite supports registering different function pointers for different various of the
    // same (SQL) function, it seems likely that we will use a single implementation with conditional logic
    if (argc >= 3 && sqlite3_value_type(argv[2]) == SQLITE_TEXT){
      // TODO: put in some schema-validation/error-recovery.
        template_options = (reinterpret_cast<const char *>(sqlite3_value_text(argv[2])));
        auto options = json::parse(template_options);
        if (options.contains("expression")){
          auto v = options["expression"];
          env->set_expression(v[0], v[1]);
        }
        if (options.contains("comment")){
          auto v = options["comment"];
          env->set_comment(v[0], v[1]);          
        }
        if (options.contains("statement")){
          auto v = options["statement"];
          env->set_statement(v[0], v[1]);  
        }     
        if (options.contains("line_statement")){
          auto v = options["line_statement"];
          env->set_line_statement(v[0]);  
        }           
    }  
  }
  if (temp == nullptr){
    save_template = 1;
    template_string = (reinterpret_cast<const char *>(sqlite3_value_text(argv[0])));
    // attempt to stash the parsed template somewhere
    // want to use the env to parse it (because it may have options) but don't
    // know how to get a pointer to a Template other than 
    inja::Template t1 = env->parse(template_string);
    temp = new inja::Template(t1);
  }


  // hope that overloaded assignment operator will do the right thing.
  // see if there is excessive copying going on.
 
  json_data = (reinterpret_cast<const char *>(sqlite3_value_text(argv[1])));
  // TODO: figure out when to return an error vs sqlite3_result_null
  try
  {
    json data = json::parse(json_data);
    expanded  = env->render(*temp, data);
  }
  catch (inja::InjaError e)
  {
    // how to do sprintf-like formatting?
    std::string message = "Inja exception: " + e.message + "(" + e.type + ")";
    // TODO: figure out how the memory management is working so as to avoid memory-leaks
    // "The sqlite3_result_error() and sqlite3_result_error16() routines make a
    //  private copy of the error message text before they return"
    sqlite3_result_error(context, message.data(), (int)message.length());
    return;
  }
  catch (json::exception e)
  {
    std::string message = e.what();
    sqlite3_result_error(context, message.data(), (int)message.length());
    return;
  }

  // TODO: deal with encodings, preferred encodings etc.
  sqlite3_result_text(context, expanded.data(), (int)expanded.length(), SQLITE_TRANSIENT);
  // not sure if we have to do anything with freeing 'expanded'
  // I think it will be taken care of by the runtime simply by going out of scope
  // and that nothing has to be done to it explicitly.

   if (save_env==1){
        // make sure to only set this the first time around as 
        // "After each call to sqlite3_set_auxdata(C,N,P,X) where X is not NULL, SQLite will invoke the destructor 
        //  function X with parameter P exactly once, when the metadata is discarded.""
        sqlite3_set_auxdata(context, 2, env, free_inja_environment);
    } 

    if (save_template==1){
      sqlite3_set_auxdata(context, 0, temp, free_inja_template);
    }
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
  rc = sqlite3_create_function(db, "template_render", 3,
                               SQLITE_UTF8 | SQLITE_DETERMINISTIC,
                               0, inja_func, 0, 0);
  return rc;
}