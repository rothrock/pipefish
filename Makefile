pipefish: pipefish.c java_home
	gcc -Wall -O2 -ggdb pipefish.c -o pipefish -L/usr/lib/hadoop/lib/native -lhdfs -lmysqlclient -L${JAVA_HOME}/jre/lib/amd64/server -ljvm

.PHONY : clean
clean:
	rm pipefish

.PHONY : install
install:
	install pf /usr/local/bin/pf
	install pipefish /usr/local/bin/pipefish
	install pipefish_classpath /usr/local/bin/pipefish_classpath

.PHONY : java_home
java_home:
	# Is JAVA_HOME set?
	test -n "${JAVA_HOME}"
