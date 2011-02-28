var sqlite3 = module.exports = exports = require('./sqlite3_bindings');

var Database = sqlite3.Database;
var Statement = sqlite3.Statement;

function errorCallback(args) {
    if (typeof args[args.length - 1] === 'function') {
        var callback = args[args.length - 1];
        return function(err) { if (err) callback(err); };
    }
}

// Database#prepare(sql, [bind1, bind2, ...], [callback])
Database.prototype.prepare = function(sql) {
    var params = Array.prototype.slice.call(arguments, 1);

    if (!params.length || (params.length === 1 && typeof params[0] === 'function')) {
        return new Statement(this, sql, params[0]);
    }
    else {
        var statement = new Statement(this, sql, errorCallback(params));
        return statement.bind.apply(statement, params);
    }
};

// Database#run(sql, [bind1, bind2, ...], [callback])
Database.prototype.run = function(sql) {
    var params = Array.prototype.slice.call(arguments, 1);
    var statement = new Statement(this, sql, errorCallback(params));
    statement.run.apply(statement, params).finalize();
    return this;
}

// Database#get(sql, [bind1, bind2, ...], [callback])
Database.prototype.get = function(sql) {
    var params = Array.prototype.slice.call(arguments, 1);
    var statement = new Statement(this, sql, errorCallback(params));
    statement.get.apply(statement, params).finalize();
    return this;
}

// Database#all(sql, [bind1, bind2, ...], [callback])
Database.prototype.all = function(sql) {
    var params = Array.prototype.slice.call(arguments, 1);
    var statement = new Statement(this, sql, errorCallback(params));
    statement.all.apply(statement, params).finalize();
    return this;
}

// Database#each(sql, [bind1, bind2, ...], [callback], [complete])
Database.prototype.each = function(sql) {
    var params = Array.prototype.slice.call(arguments, 1);
    var statement = new Statement(this, sql, errorCallback(params));
    statement.each.apply(statement, params).finalize();
    return this;
}

var isVerbose = false;

// Save the stack trace over EIO callbacks.
sqlite3.verbose = function() {
    if (!isVerbose) {
        var trace = require('./trace');
        trace.extendTrace(Database.prototype, 'prepare');
        trace.extendTrace(Database.prototype, 'get');
        trace.extendTrace(Database.prototype, 'run');
        trace.extendTrace(Database.prototype, 'all');
        trace.extendTrace(Database.prototype, 'each');
        trace.extendTrace(Database.prototype, 'exec');
        trace.extendTrace(Database.prototype, 'close');
        trace.extendTrace(Statement.prototype, 'bind');
        trace.extendTrace(Statement.prototype, 'get');
        trace.extendTrace(Statement.prototype, 'run');
        trace.extendTrace(Statement.prototype, 'all');
        trace.extendTrace(Statement.prototype, 'each');
        trace.extendTrace(Statement.prototype, 'reset');
        trace.extendTrace(Statement.prototype, 'finalize');
        isVerbose = true;
    }

    return this;
};
