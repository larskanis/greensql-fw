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
	cp greensql-fw ${DESTDIR}/usr/sbin
	cp scripts/greensql-create-db.sh ${DESTDIR}/usr/sbin/
	cp conf/greensql.conf ${DESTDIR}/etc/greensql/
	cp conf/mysql.conf ${DESTDIR}/etc/greensql/
	touch ${DESTDIR}/var/log/greensql.log
