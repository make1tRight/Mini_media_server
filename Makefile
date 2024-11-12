SRC_DIR1 = ./live
SRC_DIR2 = ./log
SRC_DIR3 = ./media
SRC_DIR4 = ./rtp
SRC_DIR5 = ./rtsp
SRC_DIR6 = ./threadpool
SRC_DIR7 = ./timer

SRC = $(wildcard $(SRC_DIR1)/*.cpp) $(wildcard $(SRC_DIR2)/*.cpp) $(wildcard $(SRC_DIR3)/*.cpp) $(wildcard $(SRC_DIR4)/*.cpp) $(wildcard $(SRC_DIR5)/*.cpp) $(wildcard $(SRC_DIR6)/*.cpp) $(wildcard $(SRC_DIR7)/*.cpp)


# INC_DIR = ./include
# 变量定义
CXX = g++                   # 指定C++编译器
CXXFLAGS = -std=c++11 -Iinclude -pthread -g# 编译选项 - 使用 C++11 标准和包含头文件路径
TARGET = server                # 最终生成的可执行文件名

# 链接目标文件生成最终可执行文件
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) main.cpp rtsp_server.cpp -o $(TARGET) $(SRC)

# 清除编译生成的文件
clean:
	rm -f $(TARGET)