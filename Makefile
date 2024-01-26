CC=gcc

all: CovertChannel ExceptionSuppression Meltdown

CovertChannel: CovertChannel.c
	$(CC) CovertChannel.c -o CovertChannel

ExceptionSuppression: ExceptionSuppression.c
	$(CC) ExceptionSuppression.c -o ExceptionSuppression

Meltdown: Meltdown.c
	$(CC) Meltdown.c -o Meltdown

clean:
	rm CovertChannel ExceptionSuppression Meltdown