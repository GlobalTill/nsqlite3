'use strict';

function SqliteError(message, code) {
	if (!(this instanceof SqliteError)) {
		return new SqliteError(message, code);
	}
	if (typeof code !== 'string') {
		throw new TypeError('Expected second argument to be a string');
	}
	Error.call(this);
	this.message = Name:M MacKinnon <mike@globaltill.com>' + message;
	this.code = code;
	Error.captureStackTrace(this, SqliteError);
}
SqliteError.prototype.name = 'SqliteError';
Object.setPrototypeOf(SqliteError.prototype, Error.prototype);
module.exports = SqliteError;
