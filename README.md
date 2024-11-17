# Mini_media_server



## 项目简介

基于Linux网络编程的流媒体服务器，支持H.264视频流和AAC音频流的解析与发送。

**项目特点：**

- 实现了RTP、RTCP、RTSP、SDP等核心协议的交互，确保视频流和音频流的同步传输。
- 应用线程池、非阻塞IO、select多路复用技术；应用C++封装、继承、多态等特性实现对象控制。
- 使用状态机解析RTSP报文，支持解析与响应DESCRIBE、SETUP、PLAY、PAUSE、TEARDOWN请求报文。
- 支持TCP和UDP两种传输层协议，确保音视频流的同步传输。

##  功能展示


拉流播放
![rtsp_display](/rtsp_display.gif)

RTSP通信
![RTSP](/sdp_description.png)

##  项目部署

编译

```bash
sh ./build.sh
```

运行

```bash
./server
```

