CXX = g++
CXXFLAGS = -g -std=c++11 -pthread
INCS = -I$(ACL_PATH)/lib_acl_cpp/include
ACL_PATH = ../acl

.PHONY: deps clean

all: deps redis-cluster-bench

deps:
	make -j16 -C ../acl/lib_acl
	make -j16 -C ../acl/lib_protocol
	make -j16 -C ../acl/lib_acl_cpp

redis-cluster-bench: redis-cluster-bench.c $(ACL_PATH)/lib_acl_cpp/lib/libacl_cpp.a $(ACL_PATH)/lib_acl/lib/libacl.a 
	$(CXX) $(CXXFLAGS) $(INCS) $^ -o $@

clean:
	rm -f redis-cluster-bench
