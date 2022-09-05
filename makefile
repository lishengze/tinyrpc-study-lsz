##################################
# makefile
# ikerli
# 2022-05-23
##################################

PATH_BIN = bin
PATH_LIB = lib

PATH_TINYRPC = tinyrpc
PATH_COMM = $(PATH_TINYRPC)/comm
PATH_COROUTINE = $(PATH_TINYRPC)/coroutine
PATH_NET = $(PATH_TINYRPC)/net
PATH_HTTP = $(PATH_TINYRPC)/net/http
PATH_TCP = $(PATH_TINYRPC)/net/tcp
PATH_TINYPB = $(PATH_TINYRPC)/net/tinypb
PATH_TINYXML = $(PATH_TINYRPC)/tinyxml

PATH_TESTCASES = testcases

# will install lib to /usr/lib/libtinyrpc.a
PATH_INSTALL_LIB_ROOT = /usr/lib

# will install all header file to /usr/include/tinyrpc
PATH_INSTALL_INC_ROOT = /usr/include

PATH_INSTALL_INC_COMM = $(PATH_INSTALL_INC_ROOT)/$(PATH_COMM)
PATH_INSTALL_INC_COROUTINE = $(PATH_INSTALL_INC_ROOT)/$(PATH_COROUTINE)
PATH_INSTALL_INC_NET = $(PATH_INSTALL_INC_ROOT)/$(PATH_NET)
PATH_INSTALL_INC_HTTP = $(PATH_INSTALL_INC_ROOT)/$(PATH_HTTP)
PATH_INSTALL_INC_TCP = $(PATH_INSTALL_INC_ROOT)/$(PATH_TCP)
PATH_INSTALL_INC_TINYPB = $(PATH_INSTALL_INC_ROOT)/$(PATH_TINYPB)



# PATH_PROTOBUF = /usr/include/google
# PATH_TINYXML = /usr/include/tinyxml

CXX := g++

CXXFLAGS += -g -O0 -std=c++11 -Wall -Wno-deprecated -Wno-unused-but-set-variable
# add lib plugin
# CXXFLAGS += -g -O0 -std=c++11 -Wall -Wno-deprecated -Wno-unused-but-set-variable -D DECLARE_MYSQL_PLUGIN
CXXFLAGS += -I./ -I$(PATH_TINYRPC)	-I$(PATH_COMM) -I$(PATH_COROUTINE) -I$(PATH_NET) -I$(PATH_HTTP) -I$(PATH_TCP) -I$(PATH_TINYPB)

# LIBS += /usr/lib/libprotobuf.a	/usr/lib/libtinyxml.a

# MYSQL_LIB = /usr/lib/libmysqlclient.a


LIBS += /usr/lib/libprotobuf.so

MYSQL_LIB = /usr/lib/libmysqlclient.so


PLUGIN_LIB =
# PLUGIN_LIB = $(MYSQL_LIB)

COMM_OBJ := $(patsubst $(PATH_COMM)/%.cc, $(PATH_COMM)/%.o, $(wildcard $(PATH_COMM)/*.cc))
COROUTINE_OBJ := $(patsubst $(PATH_COROUTINE)/%.cc, $(PATH_COROUTINE)/%.o, $(wildcard $(PATH_COROUTINE)/*.cc))
NET_OBJ := $(patsubst $(PATH_NET)/%.cc, $(PATH_NET)/%.o, $(wildcard $(PATH_NET)/*.cc))
HTTP_OBJ := $(patsubst $(PATH_HTTP)/%.cc, $(PATH_HTTP)/%.o, $(wildcard $(PATH_HTTP)/*.cc))
TCP_OBJ := $(patsubst $(PATH_TCP)/%.cc, $(PATH_TCP)/%.o, $(wildcard $(PATH_TCP)/*.cc))
TINYPB_OBJ := $(patsubst $(PATH_TINYPB)/%.cc, $(PATH_TINYPB)/%.o, $(wildcard $(PATH_TINYPB)/*.cc))
TINYXML_OBJ := $(patsubst $(PATH_TINYXML)/%.cc, $(PATH_TINYXML)/%.o, $(wildcard $(PATH_TINYXML)/*.cc))

COR_CTX_SWAP := coctx_swap.o

# ALL_TESTS : $(PATH_BIN)/test_rpc_server1 $(PATH_BIN)/test_rpc_server2 $(PATH_BIN)/test_http_server\

# TEST_CASE_OUT := $(PATH_BIN)/test_rpc_server1 $(PATH_BIN)/test_rpc_server2 $(PATH_BIN)/test_http_server\

LIB_OUT := $(PATH_LIB)/libtinyrpc.a

# $(PATH_BIN)/test_rpc_server1: $(LIB_OUT)
# 	$(CXX) $(CXXFLAGS) $(PATH_TESTCASES)/test_rpc_server1.cc $(PATH_TESTCASES)/tinypb.pb.cc -o $@ $(LIB_OUT) $(LIBS) -ldl -pthread $(PLUGIN_LIB)

# $(PATH_BIN)/test_rpc_server2: $(LIB_OUT)
# 	$(CXX) $(CXXFLAGS) $(PATH_TESTCASES)/test_rpc_server2.cc $(PATH_TESTCASES)/tinypb.pb.cc -o $@ $(LIB_OUT) $(LIBS) -ldl -pthread $(PLUGIN_LIB)

# $(PATH_BIN)/test_http_server: $(LIB_OUT)
# 	$(CXX) $(CXXFLAGS) $(PATH_TESTCASES)/test_http_server.cc $(PATH_TESTCASES)/tinypb.pb.cc -o $@ $(LIB_OUT) $(LIBS) -ldl -pthread $(PLUGIN_LIB)

$(PATH_LIB)/libtinyrpc.a : $(COMM_OBJ) $(COROUTINE_OBJ) $(PATH_COROUTINE)/coctx_swap.o $(NET_OBJ) $(HTTP_OBJ) $(TCP_OBJ) $(TINYPB_OBJ)
	@ar crsvT $@ $^

$(PATH_COMM)/%.o : $(PATH_COMM)/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(PATH_COROUTINE)/%.o : $(PATH_COROUTINE)/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(PATH_COROUTINE)/coctx_swap.o : $(PATH_COROUTINE)/coctx_swap.S
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(PATH_NET)/%.o : $(PATH_NET)/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(PATH_HTTP)/%.o : $(PATH_HTTP)/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(PATH_TCP)/%.o : $(PATH_TCP)/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(PATH_TINYPB)/%.o : $(PATH_TINYPB)/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(PATH_TINYXML)/%.o : $(PATH_TINYXML)/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@


# print something test
# like this: make PRINT-PATH_BIN, and then will print variable PATH_BIN
PRINT-% : ; @echo $* = $($*)


# to clean 
clean :
	rm -f $(COMM_OBJ) $(COROUTINE_OBJ) $(NET_OBJ) $(HTTP_OBJ) $(TCP_OBJ) $(TINYPB_OBJ) $(TESTCASES) $(PATH_COROUTINE)/coctx_swap.o $(TEST_CASE_OUT) $(PATH_LIB)/libtinyrpc.a

# install
install:
	mkdir -p $(PATH_INSTALL_INC_COMM) $(PATH_INSTALL_INC_COROUTINE) $(PATH_INSTALL_INC_NET) \
		&& mkdir -p $(PATH_INSTALL_INC_TCP) $(PATH_INSTALL_INC_HTTP) $(PATH_INSTALL_INC_TINYPB) \
		&& cp $(PATH_COMM)/*.h $(PATH_INSTALL_INC_COMM) \
		&& cp $(PATH_COROUTINE)/*.h $(PATH_INSTALL_INC_COROUTINE) \
		&& cp $(PATH_NET)/*.h $(PATH_INSTALL_INC_NET) \
		&& cp $(PATH_HTTP)/*.h $(PATH_INSTALL_INC_HTTP) \
		&& cp $(PATH_TCP)/*.h $(PATH_INSTALL_INC_TCP) \
		&& cp $(PATH_TINYPB)/*.h $(PATH_INSTALL_INC_TINYPB) \
		&& cp $(LIB_OUT) $(PATH_INSTALL_LIB_ROOT)/


# uninstall
uninstall:
	rm -rf $(PATH_INSTALL_INC_ROOT)/tinyrpc && rm -f $(PATH_INSTALL_LIB_ROOT)/libtinyrpc.a