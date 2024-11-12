#ifndef LOG_H
#define LOG_H
#include <string>
#include <vector>

static std::string get_time() {
    const char *time_format = "%Y-%m-%d %H:%M:%S";
    // 获取当前时间戳
    time_t t = time(nullptr);
    
    char time_str[64];
    // 将tm结构(localtime(&t)转化)的时间格式化为字符串, 存储在time_str
    strftime(time_str, sizeof(time_str), time_format, localtime(&t));

    return time_str; //string的data方法
}

static std::string get_file(std::string file) {
    std::string pattern = "/";

    std::string::size_type pos;
    std::vector<std::string> result;

    file += pattern;
    int size = file.size();
    for (int i = 0; i < size; ++i) {
        pos = file.find(pattern, i);

        if (pos < size) {
            std::string s = file.substr(i, pos - i); //提取当前位置到/位置之间的字符串
            result.push_back(s);
            i = pos + pattern.size() - 1;
        }
    }

    return result.back(); //路径最后一个/后面就是文件名
}
#define LOGINFO(format, ...) fprintf(stderr, "[INFO]%s [%s:%d] " format "\n", get_time().data(), __FILE__, __LINE__, ##__VA_ARGS__)
#define LOGERR(format, ...) fprintf(stderr, "[ERROR]%s [%s:%d] " format "\n", get_time().data(), __FILE__, __LINE__, ##__VA_ARGS__)
#endif