CC = riscv64-unknown-linux-gnu-gcc
#CC     = gcc

multi_thread = ./run/multi_thread
CFlags_m = -fopenmp -D_REENTRANT -Wall -O2 -lrt#-std=c99
objs_m = main_multi_thread.o md5.o $(objs_my_template_arr)
Include_m = -I./ -I../../my_template_arr/

single_thread = ./run/single_thread
CFlags_s = -Wall -O2 -lrt#-std=c99
objs_s = main_single_thread.o md5.o $(objs_my_template_arr)
Include_s = -I./ -I../../my_template_arr/

objs_my_template_arr = ../../my_template_arr/my_string.o


$(multi_thread) : $(objs_m)
	$(CC) -o $(multi_thread) $(CFlags_m) $(Include_m) $(objs_m)
main_multi_thread.o: main.c ../../my_template_arr/my_string.h my_data.h md5.h
	$(CC) -c main.c $(CFlags_m) $(Include_m) -o main_multi_thread.o
md5.o: md5.c md5.h
	$(CC) -c md5.c $(CFlags_m) $(Include_m)

$(single_thread) : $(objs_s)
	$(CC) -o $(single_thread) $(CFlags_s) $(Include_s) $(objs_s)
main_single_thread.o: main.c ../../my_template_arr/my_string.h my_data.h md5.h
	$(CC) -c main.c $(CFlags_s) $(Include_s) -o main_single_thread.o

../../my_template_arr/my_string.o: ../../my_template_arr/my_string.h
	$(CC) -c ../../my_template_arr/my_string.c $(CFlags_s) $(Include_s) -o ../../my_template_arr/my_string.o

.PHONY : all
all: $(multi_thread) $(single_thread)

.PHONY : m_th
m_th: $(multi_thread)

.PHONY : s_th
s_th: $(single_thread)

.PHONY : clean
clean:
	rm -rf *.o
	rm -f ../../my_template_arr/*.o
