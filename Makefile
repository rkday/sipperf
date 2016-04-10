sipperf: sipperf.cpp sipua.cpp
	g++ -ggdb3 -std=c++11 -I/home/rkd//svn/libre/include sipperf.cpp sipua.cpp -o sipperf -L/home/rkd/svn/libre/ -lre -lpthread -lssl -lcrypto -lz -lresolv 
