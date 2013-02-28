#ifndef __NODE_FREELING_H__
#define __NODE_FREELING_H__

#include <iostream>
#include <node.h>
#include <v8.h>
#include <freeling.h>
#include <iconv.h>

using namespace v8;
using namespace node;
using namespace std;
using namespace freeling;

#define ARG_STRING_TO_CHAR(I, STR)                                                                \
    std::string strfile = std::string(*v8::String::Utf8Value(args[I]->ToString()));    \
    STR = new char[strfile.size() + 1];                                      \
    strcpy(STR, strfile.c_str());                                                  \

#define REQ_FUN_ARG(I, VAR)                                             \
  if (args.Length() <= (I) || !args[I]->IsFunction())                   \
    return v8::ThrowException(v8::Exception::TypeError(                         \
                  v8::String::New("Argument " #I " must be a function")));  \
  Local<Function> VAR = Local<Function>::Cast(args[I]);

class Freeling : public node::ObjectWrap {
public:
    static void init(Handle<Object> target);

    // cv methods
    static Handle<Value> process(const v8::Arguments&);
};

#endif