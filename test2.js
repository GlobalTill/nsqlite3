var fs = require("fs");

process.mixin(GLOBAL, require("assert"));
process.mixin(GLOBAL, require("sys"));
var sqlite = require("./sqlite3_bindings");

var db = new sqlite.Database();

throws(function () {
  db.open();
});

throws(function () {
  db.open("my.db");
});

throws(function () {
  db.open("foo.db");
});

function testBinds() {
  db.open("mydatabase.db", function (err) {
    puts(inspect(arguments));
    if (err) {
      puts("There was an error");
      throw err;
    }

    puts("open callback");
    db.printIt();

    db.prepare("SELECT bar FROM foo; SELECT baz FROM foo;", function (error, statement) {
      puts("prepare callback");
      db.prepare("SELECT bar FROM foo WHERE bar = $x and (baz = $y or baz = $z)", function (error, statement) {
      });

      db.prepare("SELECT bar FROM foo WHERE bar = $x and (baz = $y or baz = $z)", function (error, statement) {
        puts("prepare callback");

        statement.bind(0, 666, function () {
          puts("bind callback");

          statement.bind('$y', 666, function () {
            puts("bind callback");

            statement.bind('$x', "hello", function () {
              puts("bind callback");

              statement.bind('$z', 3.141, function () {
                puts("bind callback");

  //               statement.bind('$a', 'no such param', function (err) {
  //                 puts(inspect(arguments));
                  puts("bind callback");
                  statement.step(function () {
                    puts("step callback");  
                  });
  //               });
              });
            });
          });
        });
      });
    });
  });
}

db.open("mydatabase.db", function () {
  db.prepare("SELECT bar, baz FROM foo WHERE baz > 5", function (error, statement) {
    puts(inspect(arguments));
    
    statement.step(function () {
      puts('query callback');
    });
  });
});

puts("done");
