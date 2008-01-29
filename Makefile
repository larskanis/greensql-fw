
CPP=g++

greensql-fw: 
	rm greensql-fw src/greensql-fw; cd src; make; cp greensql-fw ../

clean:
	cd src; make clean; cd mysql; rm *.o -rf;
	rm greensql-fw *.o -rf
	rm src/greensql-fw -rf

install:
	cp greensql-fw ${DESTDIR}/usr/sbin
	cp conf/greensql.conf ${DESTDIR}/etc/greensql/
	cp conf/mysql.conf ${DESTDIR}/etc/greensql/
	touch ${DESTDIR}/var/log/greensql.log
