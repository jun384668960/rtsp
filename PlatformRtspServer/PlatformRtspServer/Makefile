#CROSS := arm-hisiv300-linux-
CC := $(CROSS)c++
CFLAG := -DALLOW_SERVER_PORT_REUSE -Wno-write-strings \
		-lliveMedia -lgroupsock -lBasicUsageEnvironment -lUsageEnvironment \
		-lssl -lcrypto -lgssliveconn -lmysqlclient -lz -lp2p -lrt -ldl -lpthread \
		-lm -L./lib \
		-I./live555/BasicUsageEnvironment/include \
		-I./live555/groupsock/include \
		-I./live555/liveMedia/include \
		-I./live555/UsageEnvironment/include \
        -I./live555/GssLiveConn/include \
		-I./include -I../live/extend/include -I../live/extend/include/openssl/
		
BINS := rtspServer

SRCS := $(wildcard ./src/*.cpp)
SRCS += $(wildcard ./src/*.c)
OBJS := $(wildcard ./*.o ./src/*.o)

all: $(BINS)

$(BINS):
	$(CC) live555MediaServer.cpp $(SRCS) -g -o $(BINS) $(CFLAG)
	
clean:
	@rm -rf $(BINS)
	@rm -rf $(OBJS)
