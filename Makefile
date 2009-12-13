MAINTAINER=stremovsky@gmail.com
CPP=g++
LBITS := $(shell getconf LONG_BIT)

greensql-fw: 
	rm -rf greensql-fw src/greensql-fw 
	cd src; make; cp greensql-fw ../

clean:
	cd src; make clean; cd mysql; rm -rf *.o;
	rm -rf greensql-fw *.o
	rm -rf configure-stamp build-stamp
	rm -rf src/greensql-fw
	rm -rf debian/greensql-fw
	if test -d "../greensql-console"; then if test -d "greensql-console"; then rm -rf "greensql-console"; fi; fi

install:
	if ! test -d "greensql-console"; then if test -d "../greensql-console"; then cp -R ../greensql-console/ .; fi; fi
	cp greensql-fw ${DESTDIR}/usr/sbin/greensql-fw

ifeq ($(LBITS),64)
	cp src/lib/libgsql-mysql.so.1 ${DESTDIR}/usr/lib64/libgsql-mysql.so.1
	cp src/lib/libgsql-pgsql.so.1 ${DESTDIR}/usr/lib64/libgsql-pgsql.so.1
	ln -sf /usr/lib64/libgsql-mysql.so.1 ${DESTDIR}/usr/lib64/libgsql-mysql.so
	ln -sf /usr/lib64/libgsql-pgsql.so.1 ${DESTDIR}/usr/lib64/libgsql-pgsql.so
	ln -sf /usr/lib64/libgsql-mysql.so.1 ${DESTDIR}/usr/lib/libgsql-mysql.so
	ln -sf /usr/lib64/libgsql-pgsql.so.1 ${DESTDIR}/usr/lib/libgsql-pgsql.so
else
	cp src/lib/libgsql-mysql.so.1  ${DESTDIR}/usr/lib/libgsql-mysql.so.1
	cp src/lib/libgsql-pgsql.so.1 ${DESTDIR}/usr/lib/libgsql-pgsql.so.1
	ln -sf /usr/lib/libgsql-mysql.so.1 ${DESTDIR}/usr/lib/libgsql-mysql.so
	ln -sf /usr/lib/libgsql-pgsql.so.1 ${DESTDIR}/usr/lib/libgsql-pgsql.so
endif

	mkdir -p ${DESTDIR}/etc/greensql/
	cp scripts/greensql-create-db.sh ${DESTDIR}/usr/sbin/greensql-create-db
	ln -sf /usr/bin/greensql-create-db ${DESTDIR}/usr/bin/greensql-create-db.sh
	cp conf/greensql.conf ${DESTDIR}/etc/greensql/greensql.conf
	cp conf/mysql.conf ${DESTDIR}/etc/greensql/mysql.conf
	cp conf/pgsql.conf ${DESTDIR}/etc/greensql/pgsql.conf
	touch ${DESTDIR}/var/log/greensql.log

install-web:
	cp conf/greensql-apache.conf ${DESTDIR}/etc/greensql/
	mkdir -p ${DESTDIR}/usr/share/greensql-fw/
	if test -d "../greensql-console"; then cp -R ../greensql-console/* ${DESTDIR}/usr/share/greensql-fw/; fi 
	if test -d "greensql-console"; then cp -R greensql-console/* ${DESTDIR}/usr/share/greensql-fw/; fi
