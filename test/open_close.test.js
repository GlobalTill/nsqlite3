var sqlite3 = require('sqlite3');
var assert = require('assert');
var fs = require('fs');
var helper = require('./support/helper');

exports['constants'] = function() {
    assert.ok(sqlite3.OPEN_READONLY === 1);
    assert.ok(sqlite3.OPEN_READWRITE === 2);
    assert.ok(sqlite3.OPEN_CREATE === 4);
};

exports['open and close non-existent database'] = function(beforeExit) {
    var opened, closed;

    helper.deleteFile('test/tmp/test_create.db');
    var db = new sqlite3.Database('test/tmp/test_create.db');

    db.open(function(err) {
        if (err) throw err;
        assert.ok(!opened);
        assert.ok(!closed);
        opened = true;
    });
    db.close(function(err) {
        if (err) throw err;
        assert.ok(opened);
        assert.ok(!closed);
        closed = true;
    });

    beforeExit(function() {
        assert.ok(opened, 'Database not opened');
        assert.ok(closed, 'Database not closed');
        assert.fileExists('test/tmp/test_create.db');
        helper.deleteFile('test/tmp/test_create.db');
    });
};

exports['open inaccessible database'] = function(beforeExit) {
    var notOpened;

    var db = new sqlite3.Database('/usr/bin/test.db');
    db.open(function(err) {
        if (err && err.code === 'SQLITE_CANTOPEN') {
            notOpened = true;
        }
        else if (err) throw err;
    });

    beforeExit(function() {
        assert.ok(notOpened, 'Database could be opened');
    });
};


exports['open non-existent database without create'] = function(beforeExit) {
    var notOpened;

    helper.deleteFile('tmp/test_readonly.db');
    var db = new sqlite3.Database('tmp/test_readonly.db', sqlite3.OPEN_READONLY);

    db.open(function(err) {
        if (err && err.code === 'SQLITE_CANTOPEN') {
            notOpened = true;
        }
        else if (err) throw err;
    });

    beforeExit(function() {
        assert.ok(notOpened, 'Database could be opened');
        assert.fileDoesNotExist('tmp/test_readonly.db');
    });
};

exports['open and close memory database queuing'] = function(beforeExit) {
    var opened = 0, closed = 0;

    var db = new sqlite3.Database(':memory:');

    function openedCallback(err) {
        if (err) throw err;
        opened++;
    }

    function closedCallback(err) {
        if (err) console.warn(err);
        if (err) throw err;
        closed++;
    }

    db.open(openedCallback);
    db.close(closedCallback);
    db.open(openedCallback);
    db.close(closedCallback);
    db.open(openedCallback);
    db.close(closedCallback);
    db.open(openedCallback);
    db.close(closedCallback);
    db.open(openedCallback);

    beforeExit(function() {
        assert.equal(opened, 5, 'Database not opened');
        assert.equal(closed, 4, 'Database not closed');
    });
};

exports['two opens in a row'] = function(beforeExit) {
    var opened = 0, openErrors = 0;

    var db = new sqlite3.Database(':memory:');

    function openedCallback(err) {
        if (err) throw err;
        opened++;
    }

    db.on('error', function(err) {
        openErrors++;
    });

    db.open(openedCallback);
    db.open(openedCallback);

    beforeExit(function() {
        assert.equal(opened, 1, 'Database not opened');
        assert.equal(openErrors, 1, 'Second open succeeded');
    });
};
