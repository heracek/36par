# all after symbol '#' is comment

# === which communication library to use ===
CC	=	mpiCC
CFLAGS	=      
LIBS	=	-lmpich -lstdc++

default:	main

main:main.cpp
	$(CC) $(CFLAGS) -o main main.cpp $(LIBS)

clear:
	\rm main

run:main
	mpiexec -n 2 ./main in-t1-n70-k30-03.txt
