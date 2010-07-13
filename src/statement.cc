/*
Copyright (c) 2010, Orlando Vazquez <ovazquez@gmail.com>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <string.h>

#include "statement.h"
#include "sqlite3_bindings.h"

static Persistent<String> callback_sym;
Persistent<FunctionTemplate> Statement::constructor_template;

void Statement::Init(v8::Handle<Object> target) {
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(New);

  constructor_template = Persistent<FunctionTemplate>::New(t);
  constructor_template->Inherit(EventEmitter::constructor_template);
  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("Statement"));

  NODE_SET_PROTOTYPE_METHOD(t, "bind", Bind);
  NODE_SET_PROTOTYPE_METHOD(t, "finalize", Finalize);
  NODE_SET_PROTOTYPE_METHOD(t, "reset", Reset);
  NODE_SET_PROTOTYPE_METHOD(t, "step", Step);

  callback_sym = Persistent<String>::New(String::New("callback"));
}

Handle<Value> Statement::New(const Arguments& args) {
  HandleScope scope;
  REQ_EXT_ARG(0, stmt);
  int first_rc = args[1]->IntegerValue();

  Statement *sto = new Statement((sqlite3_stmt*)stmt->Value(), first_rc);
  sto->Wrap(args.This());
  sto->Ref();

  return args.This();
}

int Statement::EIO_AfterBind(eio_req *req) {
  ev_unref(EV_DEFAULT_UC);

  HandleScope scope;
  struct bind_request *bind_req = (struct bind_request *)(req->data);

  Local<Value> argv[1];
  bool err = false;
  if (req->result) {
    err = true;
    argv[0] = Exception::Error(String::New("Error binding parameter"));
  }

  TryCatch try_catch;

  bind_req->cb->Call(Context::GetCurrent()->Global(), err ? 1 : 0, argv);

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }

  bind_req->cb.Dispose();

  free(bind_req->key);
  free(bind_req->value);
  free(bind_req);

  return 0;
}

int Statement::EIO_Bind(eio_req *req) {
  struct bind_request *bind_req = (struct bind_request *)(req->data);
  Statement *sto = bind_req->sto;

  int index(0);
  switch(bind_req->key_type) {
    case KEY_INT:
      index = *(int*)(bind_req->key);
      break;

    case KEY_STRING:
      index = sqlite3_bind_parameter_index(sto->stmt_,
                                           (char*)(bind_req->key));
      break;

     default: {
       // this SHOULD be unreachable
     }
  }

  if (!index) {
    req->result = SQLITE_MISMATCH;
    return 0;
  }

  int rc = 0;
  switch(bind_req->value_type) {
    case VALUE_INT:
      rc = sqlite3_bind_int(sto->stmt_, index, *(int*)(bind_req->value));
      break;
    case VALUE_DOUBLE:
      rc = sqlite3_bind_double(sto->stmt_, index, *(double*)(bind_req->value));
      break;
    case VALUE_STRING:
      rc = sqlite3_bind_text(sto->stmt_, index, (char*)(bind_req->value),
                        bind_req->value_size, SQLITE_TRANSIENT);
      break;
    case VALUE_NULL:
      rc = sqlite3_bind_null(sto->stmt_, index);
      break;

    default: {
      // should be unreachable
    }
  }

  if (rc) {
    req->result = rc;
    return 0;
  }

  return 0;
}

Handle<Value> Statement::Bind(const Arguments& args) {
  HandleScope scope;
  Statement* sto = ObjectWrap::Unwrap<Statement>(args.This());

  REQ_ARGS(2);
  REQ_FUN_ARG(2, cb);

  if (!args[0]->IsString() && !args[0]->IsInt32())
    return ThrowException(Exception::TypeError(
           String::New("First argument must be a string or integer")));

  struct bind_request *bind_req = (struct bind_request *)
      calloc(1, sizeof(struct bind_request));

  // setup key
  if (args[0]->IsString()) {
     String::Utf8Value keyValue(args[0]);
     bind_req->key_type = KEY_STRING;

     char *key = (char *) calloc(1, keyValue.length() + 1);
     bind_req->value_size = keyValue.length() + 1;
     strcpy(key, *keyValue);

     bind_req->key = key;
  }
  else if (args[0]->IsInt32()) {
    bind_req->key_type = KEY_INT;
    int *index = (int *) malloc(sizeof(int));
    *index = args[0]->Int32Value();
    bind_req->value_size = sizeof(int);

    // don't forget to `free` this
    bind_req->key = index;
  }

  // setup value
  if (args[1]->IsInt32()) {
    bind_req->value_type = VALUE_INT;
    int *value = (int *) malloc(sizeof(int));
    *value = args[1]->Int32Value();
    bind_req->value = value;
  }
  else if (args[1]->IsNumber()) {
    bind_req->value_type = VALUE_DOUBLE;
    double *value = (double *) malloc(sizeof(double));
    *value = args[1]->NumberValue();
    bind_req->value = value;
  }
  else if (args[1]->IsString()) {
    bind_req->value_type = VALUE_STRING;
    String::Utf8Value text(args[1]);
    char *value = (char *) calloc(text.length()+1, sizeof(char*));
    strcpy(value, *text);
    bind_req->value = value;
    bind_req->value_size = text.length()+1;
  }
  else if (args[1]->IsNull() || args[1]->IsUndefined()) {
    bind_req->value_type = VALUE_NULL;
    bind_req->value = NULL;
  }
  else {
    free(bind_req->key);
    return ThrowException(Exception::TypeError(
           String::New("Unable to bind value of this type")));
  }

  bind_req->cb = Persistent<Function>::New(cb);
  bind_req->sto = sto;

  eio_custom(EIO_Bind, EIO_PRI_DEFAULT, EIO_AfterBind, bind_req);

  ev_ref(EV_DEFAULT_UC);

  return Undefined();
}

int Statement::EIO_AfterFinalize(eio_req *req) {
  ev_unref(EV_DEFAULT_UC);
  Statement *sto = (class Statement *)(req->data);

  HandleScope scope;

  Local<Function> cb = sto->GetCallback();

  TryCatch try_catch;

  cb->Call(sto->handle_, 0, NULL);

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }

  sto->Unref();

  return 0;
}

int Statement::EIO_Finalize(eio_req *req) {
  Statement *sto = (class Statement *)(req->data);

  assert(sto->stmt_);
  req->result = sqlite3_finalize(sto->stmt_);
  sto->stmt_ = NULL;

  return 0;
}

Handle<Value> Statement::Finalize(const Arguments& args) {
  HandleScope scope;

  Statement* sto = ObjectWrap::Unwrap<Statement>(args.This());

  if (sto->HasCallback()) {
    return ThrowException(Exception::Error(String::New("Already stepping")));
  }

  REQ_FUN_ARG(0, cb);

  sto->SetCallback(cb);

  eio_custom(EIO_Finalize, EIO_PRI_DEFAULT, EIO_AfterFinalize, sto);

  ev_ref(EV_DEFAULT_UC);

  return Undefined();
}

Handle<Value> Statement::Reset(const Arguments& args) {
  HandleScope scope;
  Statement* sto = ObjectWrap::Unwrap<Statement>(args.This());
  SCHECK(sqlite3_reset(sto->stmt_));
  return Undefined();
}

int Statement::EIO_AfterStep(eio_req *req) {
  ev_unref(EV_DEFAULT_UC);

  HandleScope scope;

  Statement *sto = (class Statement *)(req->data);

  Local<Value> argv[2];

  if (sto->error_) {
    argv[0] = Exception::Error(
        String::New(sqlite3_errmsg(sqlite3_db_handle(sto->stmt_))));
  }
  else {
    argv[0] = Local<Value>::New(Undefined());
  }

  if (req->result == SQLITE_DONE) {
    argv[1] = Local<Value>::New(Null());
  }
  else {
    Local<Object> row = Object::New();

    for (int i = 0; i < sto->column_count_; i++) {
      assert(sto->column_data_);
      assert(((void**)sto->column_data_)[i]);
      assert(sto->column_names_[i]);
      assert(sto->column_types_[i]);

      switch (sto->column_types_[i]) {
        // XXX why does using String::New make v8 croak here?
        case SQLITE_INTEGER:
          row->Set(String::NewSymbol((char*) sto->column_names_[i]),
                   Int32::New(*(int*) (sto->column_data_[i])));
          break;

        case SQLITE_FLOAT:
          row->Set(String::New(sto->column_names_[i]),
                   Number::New(*(double*) (sto->column_data_[i])));
          break;

        case SQLITE_TEXT:
          assert(strlen((char*)sto->column_data_[i]));
          row->Set(String::New(sto->column_names_[i]),
                   String::New((char *) (sto->column_data_[i])));
          // don't free this pointer, it's owned by sqlite3
          break;

        // no default
      }
    }

    argv[1] = row;
  }

  TryCatch try_catch;

  Local<Function> cb = sto->GetCallback();

  cb->Call(sto->handle_, 2, argv);

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }

  if (req->result == SQLITE_DONE && sto->column_count_) {
    sto->FreeColumnData();
  }

  return 0;
}

void Statement::FreeColumnData(void) {
  if (!column_count_) return;
  for (int i = 0; i < column_count_; i++) {
    switch (column_types_[i]) {
      case SQLITE_INTEGER:
        free(column_data_[i]);
        break;
      case SQLITE_FLOAT:
        free(column_data_[i]);
        break;
    }
    column_data_[i] = NULL;
  }

  free(column_data_);
  column_data_ = NULL;
}

int Statement::EIO_Step(eio_req *req) {
  Statement *sto = (class Statement *)(req->data);
  sqlite3_stmt *stmt = sto->stmt_;
  assert(stmt);
  int rc;

  // check if we have already taken a step immediately after prepare
  if (sto->first_rc_ != -1) {
    // This is the first one! Let's just use the rc from when we called
    // it in EIO_Prepare
    rc = req->result = sto->first_rc_;
    // Now we set first_rc_ to -1 so that on the next step, it won't
    // think this is the first.
    sto->first_rc_ = -1;
  }
  else {
    rc = req->result = sqlite3_step(stmt);
  }

  sto->error_ = false;

  if (rc == SQLITE_ROW) {
    // If these pointers are NULL, look up and store the number of columns
    // their names and types.
    // Otherwise that means we have already looked up the column types and
    // names so we can simply re-use that info.
    if (!sto->column_types_ && !sto->column_names_) {
      sto->column_count_ = sqlite3_column_count(stmt);
      assert(sto->column_count_);
      sto->column_types_ = (int *) calloc(sto->column_count_, sizeof(int));
      sto->column_names_ = (char **) calloc(sto->column_count_,
                                            sizeof(char *));

      if (sto->column_count_) {
        sto->column_data_ = (void **) calloc(sto->column_count_,
                                             sizeof(void *));
      }

      for (int i = 0; i < sto->column_count_; i++) {
        sto->column_types_[i] = sqlite3_column_type(stmt, i);
        sto->column_names_[i] = (char *) sqlite3_column_name(stmt, i);

        switch(sto->column_types_[i]) {
          case SQLITE_INTEGER:
            sto->column_data_[i] = (int *) malloc(sizeof(int));
            break;

          case SQLITE_FLOAT:
            sto->column_data_[i] = (double *) malloc(sizeof(double));
            break;

          // no need to allocate memory for strings

          default: {
            // unsupported type
          }
        }
      }
    }

    assert(sto->column_types_ && sto->column_data_ && sto->column_names_);

    for (int i = 0; i < sto->column_count_; i++) {
      int type = sto->column_types_[i];

      switch(type) {
        case SQLITE_INTEGER: 
          *(int*)(sto->column_data_[i]) = sqlite3_column_int(stmt, i);
          assert(sto->column_data_[i]);
          break;

        case SQLITE_FLOAT:
          *(double*)(sto->column_data_[i]) = sqlite3_column_double(stmt, i);
          break;

        case SQLITE_TEXT: {
            // It shouldn't be necessary to copy or free() this value,
            // according to http://www.sqlite.org/c3ref/column_blob.html
            // it will be reclaimed on the next step, reset, or finalize.
            // I'm going to assume it's okay to keep this pointer around
            // until it is used in `EIO_AfterStep`
            sto->column_data_[i] = (char *) sqlite3_column_text(stmt, i);
          }
          break;

        default: {
          assert(0 && "unsupported type");
        }
      }
      assert(sto->column_data_[i]);
      assert(sto->column_names_[i]);
      assert(sto->column_types_[i]);
    }
    assert(sto->column_data_);
    assert(sto->column_names_);
    assert(sto->column_types_);
  }
  else if (rc == SQLITE_DONE) {
    // nothing to do in this case
  }
  else {
    sto->error_ = true;
  }

  return 0;
}

Handle<Value> Statement::Step(const Arguments& args) {
  HandleScope scope;

  Statement* sto = ObjectWrap::Unwrap<Statement>(args.This());

  if (sto->HasCallback()) {
    return ThrowException(Exception::Error(String::New("Already stepping")));
  }

  REQ_FUN_ARG(0, cb);

  sto->SetCallback(cb);

  eio_custom(EIO_Step, EIO_PRI_DEFAULT, EIO_AfterStep, sto);

  ev_ref(EV_DEFAULT_UC);

  return Undefined();
}

// The following three methods must be called inside a HandleScope

bool Statement::HasCallback() {
  return ! handle_->GetHiddenValue(callback_sym).IsEmpty();
}

void Statement::SetCallback(Local<Function> cb) {
  handle_->SetHiddenValue(callback_sym, cb);
}

Local<Function> Statement::GetCallback() {
  Local<Value> cb_v = handle_->GetHiddenValue(callback_sym);
  assert(cb_v->IsFunction());
  Local<Function> cb = Local<Function>::Cast(cb_v);
  handle_->DeleteHiddenValue(callback_sym);
  return cb;
}
