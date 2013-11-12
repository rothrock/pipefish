#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "mysql.h"
#include "hdfs.h"

typedef struct opt_arg_struct {
  char* host;
  char* user;
  char* db;
  char* password;
  char* hdfs_path;
  char* sql;
  char* defaults_file;
} opt_arg_struct_t;

opt_arg_struct_t args;

int usage() {
  printf("usage: pf --defaults_file='/path/to/my.cnf' --user='user' --host='host' --db='db name' --password='password' --hdfs_path='/path/to/file' --sql='sql statement'\n");
  printf("user, host, db, and password settings override those specified in defaults_file.\n");
  exit(1);
}

int main(int argc, char* argv[]) {

  MYSQL *conn;
  MYSQL_RES *result;
  MYSQL_ROW row;

  hdfsFS fs;
  hdfsFile file_handle;

  char *output_buffer;
  char *cursor;
  unsigned long row_length = 0;
  unsigned long bytes_written = 0;
  unsigned long *lengths;
  unsigned int num_fields;
  unsigned int i;
  int opt = 0;
  int opt_index = 0; 

  static struct option long_opts[] =
    {
      {"host",           required_argument, 0, 1},
      {"user",           required_argument, 0, 2},
      {"db",             required_argument, 0, 3},
      {"password",       required_argument, 0, 4},
      {"hdfs_path",      required_argument, 0, 5},
      {"sql",            required_argument, 0, 6},
      {"defaults_file",  required_argument, 0, 7},
      {"help",           optional_argument, 0, 8},
      {0, 0, 0, 0}
    };

  opt = getopt_long_only(argc, argv, "1:2:3:4:5:6:7:8", long_opts, &opt_index);
  while( opt != -1) {
    switch(opt) {
      case 1: // db hostname
        args.host = optarg;
        break;

      case 2: // db user
        args.user = optarg;
        break;

      case 3: // database name
        args.db = optarg;
        break;

      case 4: // password
        args.password = optarg;
        break;

      case 5: // path to hdfs
        args.hdfs_path = optarg;
        break;

      case 6: // SQL
        args.sql = optarg;
        break;

      case 7: // my.cnf
        args.defaults_file = optarg;
        break;

      case 8: // help
        usage();

      default:
        usage();
    }
    opt = getopt_long_only(argc, argv, "1:2:3:4:5:6:7", long_opts, &opt_index);
  }

  conn = mysql_init(NULL);
  if (args.defaults_file)  mysql_options(conn, MYSQL_READ_DEFAULT_FILE, args.defaults_file);
  if (!mysql_real_connect(conn,
                          args.host ? args.host : NULL,
                          args.user ? args.user : NULL,
                          args.password ? args.password : NULL,
                          args.db ? args.db : NULL,
                          0, NULL, 0)) {
    fprintf(stderr, "Error connecting to MySQL. %u: %s\n", mysql_errno(conn), mysql_error(conn));
    exit(1);
  }
  if (mysql_query(conn, args.sql) != 0) {
    fprintf(stderr, "Error running SQL statement. %u: %s\n", mysql_errno(conn), mysql_error(conn));
    exit(1);
  }
  result = mysql_use_result(conn);

  if (!(fs = hdfsConnect("default", 0))) {
    fprintf(stderr, "Cannot connect to HDFS.\n");
    exit(1);
  }
  if (!(file_handle = hdfsOpenFile(fs, args.hdfs_path, O_WRONLY|O_CREAT, 0, 0, 0))) {
    fprintf(stderr, "Failed to open %s for writing.\n", args.hdfs_path);
    exit(1);
  }

  num_fields = mysql_num_fields(result);
  while ((row = mysql_fetch_row(result))) {
    row_length = 0;
    lengths = mysql_fetch_lengths(result);
    for(i = 0; i < num_fields; i++)
      row_length += lengths[i] +  (row[i] == NULL ? 4 : 0) + 1;
    output_buffer = (char*)malloc(sizeof(char) * row_length + sizeof(char));
    cursor = output_buffer;
    for(i = 0; i < num_fields; i++) {
      if (lengths[i] == 0) {
        if (row[i] == NULL) {
          memcpy(cursor, "NULL", 4);
          cursor += 4;
        } 
      } else {
        memcpy(cursor, row[i], lengths[i]);
        cursor += lengths[i];
      }
      *cursor++ = '\t';
    }
    *cursor = '\n';
    bytes_written = hdfsWrite(fs, file_handle, (void*)output_buffer, row_length + 1);
    if (bytes_written < row_length) {
      fprintf(stderr, "truncated row.\n");
    }
    free(output_buffer);
  }

  if (hdfsFlush(fs, file_handle)) {
    fprintf(stderr, "Call to hdfsFlush using %s failed..\n", args.hdfs_path);
  }
  hdfsCloseFile(fs, file_handle);
  mysql_free_result(result);
  mysql_close(conn);

  exit(0);
}
