CXXFLAGS = -std=c++11 -I.. -Wall -Wextra -pthread

OPENSSL_DIR = /usr/local/opt/openssl
OPENSSL_SUPPORT = -DCPPHTTPLIB_OPENSSL_SUPPORT -I$(OPENSSL_DIR)/include -L$(OPENSSL_DIR)/lib -lssl -lcrypto
INFLUXDB = -I /usr/local/include -L /usr/local/lib 

ZLIB_SUPPORT = -DCPPHTTPLIB_ZLIB_SUPPORT -lz

BROTLI_DIR = /usr/local/opt/brotli



all : main.cpp ./cpp-httplib/httplib.h Makefile
	$(CXX) -o server $(CXXFLAGS) main.cpp $(OPENSSL_SUPPORT) $(ZLIB_SUPPORT) $(BROTLI_SUPPORT)
