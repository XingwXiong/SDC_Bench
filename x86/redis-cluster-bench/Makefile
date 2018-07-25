CXX = g++
CXXFLAGS = -g -std=c++11 -pthread
INCS = -I $(ACL_PATH)/lib_acl_cpp/include -I .
ACL_PATH = ../acl

.PHONY: deps clean

#all: deps redis-cluster-bench
all: redis-cluster-bench redis-openloop-bench

deps:
	make -j16 -C ../acl/lib_acl
	make -j16 -C ../acl/lib_protocol
	make -j16 -C ../acl/lib_acl_cpp
	mkdir -p ./bin

redis-cluster-bench: redis-cluster-bench.cpp $(ACL_PATH)/lib_acl_cpp/lib/libacl_cpp.a $(ACL_PATH)/lib_acl/lib/libacl.a 
	$(CXX) $(CXXFLAGS) $(INCS) $^ -o bin/$@

redis-openloop-bench: redis-openloop-bench.cpp $(ACL_PATH)/lib_acl_cpp/lib/libacl_cpp.a $(ACL_PATH)/lib_acl/lib/libacl.a 
	$(CXX) $(CXXFLAGS) $(INCS) $^ -o bin/$@

clean:
	rm -f bin/* *.csv *.qps *.lat
