var sqlite3 = require('sqlite3');
var assert = require('assert');
var Step = require('step');

exports['test simple prepared statement with invalid SQL'] = function(beforeExit) {
    var error = false;
    var db = new sqlite3.Database(':memory:')
    db.prepare('CRATE TALE foo text bar)', function(err, statement) {
        if (err && err.errno == sqlite3.ERROR &&
            err.message === 'SQLITE_ERROR: near "CRATE": syntax error') {
            error = true;
        }
        else throw err;
    }).finalize();
    db.close();

    beforeExit(function() {
        assert.ok(error, 'No error thrown for invalid SQL');
    });
};

exports['test simple prepared statement'] = function(beforeExit) {
    var run = false;
    var db = new sqlite3.Database(':memory:');
    db.prepare("CREATE TABLE foo (text bar)")
        .run(function(err) {
            if (err) throw err;
            run = true;
        })
        .finalize()
    .close();

    beforeExit(function() {
        assert.ok(run, 'Database query did not run');
    });
};

exports['test inserting and retrieving rows'] = function(beforeExit) {
    var db = new sqlite3.Database(':memory:');

    var inserted = 0;
    var retrieved = 0;

    // We insert and retrieve that many rows.
    var count = 5000;

    Step(
        function() {
            db.prepare("CREATE TABLE foo (txt text, num int, flt float, blb blob)").run(this);
        },
        function(err) {
            if (err) throw err;
            var group = this.group();
            for (var i = 0; i < count; i++) {
                db.prepare("INSERT INTO foo VALUES(?, ?, ?, ?)")
                  .bind(
                      'String ' + i,
                      i,
                      i * Math.PI
                      // null (SQLite sets this implicitly)
                ).run(group());
            }
        },
        function(err, rows) {
            if (err) throw err;
            inserted += rows.length;
            var stmt = db.prepare("SELECT txt, num, flt, blb FROM foo ORDER BY num");

            var group = this.group();
            for (var i = 0; i < count + 5; i++) {
                stmt.run(group());
            }
        },
        function(err, rows) {
            if (err) throw err;
            assert.equal(count + 5, rows.length, "Didn't run all queries");

            for (var i = 0; i < count; i++) {
                assert.equal(rows[i][0], 'String ' + i);
                assert.equal(rows[i][1], i);
                assert.equal(rows[i][2], i * Math.PI);
                assert.equal(rows[i][3], null);
                retrieved++;
            }

            for (var i = rows; i < count + 5; i++) {
                assert.ok(!rows[i]);
            }
        }
    );

    beforeExit(function() {
        assert.equal(count, inserted, "Didn't insert all rows");
        assert.equal(count, retrieved, "Didn't retrieve all rows");
    })
};
