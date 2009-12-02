MAINTAINER=stremovsky@gmail.com
CPP=g++

greensql-fw: 
	rm -rf greensql-fw src/greensql-fw 
	cd src; make; cp greensql-fw ../

clean:
	cd src; make clean; cd mysql; rm -rf *.o;
	rm -rf greensql-fw *.o
	rm -rf configure-stamp build-stamp
	rm -rf src/greensql-fw
	rm -rf debian/greensql-fw

install:
	cp greensql-fw ${DESTDIR}/usr/sbin/greensql-fw
	cp src/lib/libgsql-mysql.so.1  ${DESTDIR}/usr/lib/libgsql-mysql.so.1
	cp src/lib/libgsql-pgsql.so.1 ${DESTDIR}/usr/lib/libgsql-pgsql.so.1
	cp scripts/greensql-create-db.sh ${DESTDIR}/usr/sbin/greensql-create-db
	cp conf/greensql.conf ${DESTDIR}/etc/greensql/
	cp conf/mysql.conf ${DESTDIR}/etc/greensql/
	cp conf/pgsql.conf ${DESTDIR}/etc/greensql/
	touch ${DESTDIR}/var/log/greensql.log

install-web:
	cp conf/greensql-apache.conf ${DESTDIR}/etc/greensql/
	cp -R ../greensql-console/* ${DESTDIR}/usr/share/greensql-fw/
