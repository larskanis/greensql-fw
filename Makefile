
CPP=g++

greensql-fw: 
	rm greensql-fw src/greensql-fw; cd src; make; cp greensql-fw ../

clean:
	cd src; rm *.o -rf; cd mysql; rm *.o -rf;
	rm greensql-fw *.o -rf
	rm src/greensql-fw -rf
	
