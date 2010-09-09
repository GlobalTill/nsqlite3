exports.insertMany = function (db, table, fields, rows, callback) {
  var columns_fragment = fields.join(",");
  var placeholders_fragment = [];
  var i = fields.length;

  while (i--) {
    placeholders_fragment.push('?');
  }
  placeholders_fragment = placeholders_fragment.join(", ");

  var sql = 'INSERT INTO ' + table
           + ' (' + columns_fragment + ')'
           + ' VALUES (' + placeholders_fragment + ')';

  var i = rows.length;

  var statement;

  function doStep(i) {
    statement.bindArray(rows[i], function () {
      statement.step(function (error, row) {
        if (error) throw error;
        statement.reset();
        if (i) {
          doStep(--i);
        }
        else {
          statement.finalize(function () {
            callback();
          });
        }
      });
    });
  }

  db.prepare(sql, function (error, stmt) {
    statement = stmt;
    doStep(--i);
  });
}
