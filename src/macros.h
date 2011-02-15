// Copyright (c) 2010, Orlando Vazquez <ovazquez@gmail.com>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#ifndef NODE_SQLITE3_SRC_MACROS_H
#define NODE_SQLITE3_SRC_MACROS_H

const char* sqlite_code_string(int code);


#define CHECK(rc) { if ((rc) != SQLITE_OK)                              \
      return ThrowException(Exception::Error(String::New(               \
                                             sqlite3_errmsg(*db)))); }

#define SCHECK(rc) { if ((rc) != SQLITE_OK)                             \
      return ThrowException(Exception::Error(String::New(               \
                        sqlite3_errmsg(sqlite3_db_handle(sto->stmt_))))); }


#define REQUIRE_ARGUMENTS(n)                                                   \
    if (args.Length() < (n)) {                                                 \
        return ThrowException(                                                 \
            Exception::TypeError(String::New("Expected " #n "arguments"))      \
        );                                                                     \
    }


#define REQUIRE_ARGUMENT_EXTERNAL(i, var)                                      \
    if (args.Length() <= (i) || !args[i]->IsExternal()) {                      \
        return ThrowException(                                                 \
            Exception::TypeError(String::New("Argument " #i " invalid"))       \
        );                                                                     \
    }                                                                          \
    Local<External> var = Local<External>::Cast(args[i]);


#define REQUIRE_ARGUMENT_FUNCTION(i, var)                                      \
    if (args.Length() <= (i) || !args[i]->IsFunction()) {                      \
        return ThrowException(Exception::TypeError(                            \
            String::New("Argument " #i " must be a function"))                 \
        );                                                                     \
    }                                                                          \
    Local<Function> var = Local<Function>::Cast(args[i]);


#define REQUIRE_ARGUMENT_STRING(i, var)                                        \
    if (args.Length() <= (i) || !args[i]->IsString()) {                        \
        return ThrowException(Exception::TypeError(                            \
            String::New("Argument " #i " must be a string"))                   \
        );                                                                     \
    }                                                                          \
    String::Utf8Value var(args[i]->ToString());


#define OPTIONAL_ARGUMENT_FUNCTION(i, var)                                     \
    Local<Function> var;                                                       \
    bool var ## _exists = false;                                               \
    if (args.Length() >= i) {                                                  \
        if (!args[i]->IsFunction()) {                                          \
            return ThrowException(Exception::TypeError(                        \
                String::New("Argument " #i " must be a function"))             \
            );                                                                 \
        }                                                                      \
        var = Local<Function>::Cast(args[i]);                                  \
        var ## _exists = true;                                                 \
    }


#define OPTIONAL_ARGUMENT_INTEGER(i, var, default)                             \
    int var;                                                                   \
    if (args.Length() <= (i)) {                                                \
        var = (default);                                                       \
    }                                                                          \
    else if (args[i]->IsInt32()) {                                             \
        var = args[i]->Int32Value();                                           \
    }                                                                          \
    else {                                                                     \
        return ThrowException(Exception::TypeError(                            \
            String::New("Argument " #i " must be an integer"))                 \
        );                                                                     \
    }


#define DEFINE_CONSTANT_INTEGER(target, constant, name)                        \
    (target)->Set(                                                             \
        String::NewSymbol(#name),                                              \
        Integer::New(constant),                                                \
        static_cast<PropertyAttribute>(ReadOnly | DontDelete)                  \
    );

#define DEFINE_CONSTANT_STRING(target, constant, name)                         \
    (target)->Set(                                                             \
        String::NewSymbol(#name),                                              \
        String::NewSymbol(constant),                                           \
        static_cast<PropertyAttribute>(ReadOnly | DontDelete)                  \
    );


#define NODE_SET_GETTER(target, name, function)                                \
    (target)->InstanceTemplate()                                               \
        ->SetAccessor(String::NewSymbol(name), (function));

#define GET_STRING(source, name, property)                                     \
    String::Utf8Value name((source)->Get(String::NewSymbol(property)));

#define GET_INTEGER(source, name, property)                                    \
    int name = (source)->Get(String::NewSymbol(property))->Int32Value();


#define EXCEPTION(msg, errno, name)                                            \
    Local<Value> name = Exception::Error(                                      \
        String::Concat(                                                        \
            String::Concat(                                                    \
                String::NewSymbol(sqlite_code_string(errno)),                  \
                String::NewSymbol(": ")                                        \
            ),                                                                 \
            String::New(msg)                                                   \
        )                                                                      \
    );                                                                         \
    Local<Object> name ## _obj = name->ToObject();                             \
    name ## _obj->Set(NODE_PSYMBOL("errno"), Integer::New(errno));             \
    name ## _obj->Set(NODE_PSYMBOL("code"),                                    \
        String::NewSymbol(sqlite_code_string(errno)));

#endif
