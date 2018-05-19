to make:
cd src/omp/wordcount
make all

to help:
cd src/omp/wordcount/run
./multi_thread
./single_thread

to run: NOTE: thread_cnt is the last arg for multi_thread!
cd src/omp/wordcount/run
./multi_thread ../dataset/test.data.01 ./output/m.01 3
./single_thread ../dataset/test.data.01 ./output/s.01

run:
  input: file or dir;
  output: one file;

