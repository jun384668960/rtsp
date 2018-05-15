#CROSS := arm-hisiv300-linux-
CC := $(CROSS)c++
CFLAG := -DALLOW_SERVER_PORT_REUSE -Wno-write-strings -lpthread -lm -lliveMedia -lgroupsock -lBasicUsageEnvironment -lUsageEnvironment -lgssliveconn -lp2p -lssl -lcrypto -lrt -ldl -L./lib -L/root/test/openssl-1.0.2o/openssllib/lib\
		-I./live555/BasicUsageEnvironment/include \
		-I./live555/groupsock/include \
		-I./live555/liveMedia/include \
		-I./live555/UsageEnvironment/include \
                -I./live555/GssLiveConn/include \
		-I./include
		
BINS := rtspServer

SRCS := $(wildcard ./src/*.cpp)
OBJS := $(wildcard ./*.o ./src/*.o)

all: $(BINS)

$(BINS):
	$(CC) live555MediaServer.cpp $(SRCS) -g -o $(BINS) $(CFLAG)
	
clean:
	@rm -rf $(BINS)
	@rm -rf $(OBJS)
