# $^  代表所有依赖文件
# $@  代表所有目标文件
# $<  代表第一个依赖文件
# %   代表通配符

TARGET      := web_bench
STATIC_LIB  := libweb_bench.a 
SHARED_LIB  := libweb_bench.so 

CXX      	:= g++
CXXFLAGS 	:= -std=gnu++11 -Wfatal-errors -Wno-unused-parameter
LDFLAGS 	:= -Wl,-rpath=./lib 
INC_DIR  	:= -I ./include
LIB_DIR  	:= -L ./
LIBS     	:= -lpthread

DEBUG 	 	:= 0

ifeq ($(DEBUG), 1)
	CXXFLAGS += -g -DDEBUG
else
	CXXFLAGS += -O3 -DNDEBUG
endif

all: $(STATIC_LIB) $(TARGET) 

#源文件
SOURCES := web_bench.cpp
#所有.c源文件结尾变为.o 放入变量OBJS
OBJS := $(patsubst %.cpp, %.o, $(SOURCES))
#main源文件
MAIN := main.cpp
MAIN_OBJ := main.o

#编译
%.o: %.cpp
	@echo -e '\e[1;32mBuilding CXX object: $@\e[0m'
	$(CXX) -c $^ -o $@ $(CXXFLAGS) $(INC_DIR)

#链接生成静态库(这只是将源码归档到一个库文件中，什么flag都不加) @执行shell
$(STATIC_LIB): $(OBJS)
	@echo -e '\e[1;36mLinking CXX static library: $@\e[0m'
	ar -rcs -o $@ $^ 
	@echo -e '\e[1;35mBuilt target: $@\e[0m'
	
#链接生成动态库
$(SHARED_LIB): $(OBJS)
	@echo -e '\e[1;36mLinking CXX shared library: $@\e[0m'
	$(CXX) -fPIC -shared -o $@ $^ 
	@echo -e '\e[1;35mBuilt target: $@\e[0m'
	
#链接生成可执行文件
$(TARGET): $(MAIN_OBJ) $(STATIC_LIB) 
	@echo -e '\e[1;36mLinking CXX executable: $@\e[0m'
	$(CXX) -o $@ $^ $(LDFLAGS) 
	@echo -e '\e[1;35mBuilt target: $@\e[0m'

#声明这是伪目标文件
.PHONY: all clean 

install: $(STATIC_LIB) $(TARGET) 
	@if (test ! -d lib); then mkdir -p lib; fi
	@mv $(STATIC_LIB) lib
	@if (test ! -d bin); then mkdir -p bin; fi
	@mv $(TARGET) bin

clean:
	rm -f $(OBJS) $(MAIN_OBJ) $(TARGET) $(STATIC_LIB) $(SHARED_LIB) ./bin/$(TARGET) ./lib/$(STATIC_LIB) ./lib/$(SHARED_LIB)

