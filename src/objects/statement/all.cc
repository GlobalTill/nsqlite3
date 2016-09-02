// .all(Function callback) -> this

NAN_METHOD(Statement::All) {
	Statement* stmt = Nan::ObjectWrap::Unwrap<Statement>(info.This());
	if (!stmt->readonly) {
		return Nan::ThrowTypeError("This statement is not read-only. Use run() instead.");
	}
	REQUIRE_LAST_ARGUMENT_FUNCTION(func_index, func);
	STATEMENT_START(stmt);
	STATEMENT_BIND(stmt, func_index);
	AllWorker* worker = new AllWorker(stmt, _handle, _i, new Nan::Callback(func));
	STATEMENT_END(stmt, worker);
}
