LIBJVM = $(shell find ${JAVA_HOME}/ -name libjvm* | head -n 1)
LIBJVM_PATH = $(shell dirname $(LIBJVM))
OS = $(shell uname)
LIBHDFS =
INCLUDE_DIRS =
ifeq ($(OS), Linux)
  LIBHDFS = -L/usr/lib -L/usr/lib/hadoop/lib/native -L/opt/cloudera/parcels/CDH/lib/hadoop/lib/native
  INCL = -I/usr/include -I/opt/cloudera/parcels/CDH/include
endif

pipefish: pipefish.c java_home
	gcc -Wall -O2 -ggdb pipefish.c -o pipefish `mysql_config --include` $(INCL) $(LIBHDFS) -lhdfs `mysql_config --libs` -L$(LIBJVM_PATH) -ljvm

.PHONY : clean
clean:
	rm -rf pipefish pipefish.dSYM hdfs.h

.PHONY : install
install:
	install pf /usr/local/bin/pf
	install pipefish /usr/local/bin/pipefish
	install pipefish_classpath /usr/local/bin/pipefish_classpath

.PHONY : java_home
java_home:
	# Is JAVA_HOME set?
	test -n "${JAVA_HOME}"
