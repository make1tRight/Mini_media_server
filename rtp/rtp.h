#ifndef RTP_H
#define RTP_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const int RTP_VERSION = 2;
const int RTP_PAYLOAD_TYPE_H264 = 96;
const int RTP_PAYLOAD_TYPE_AAC = 97;
const int RTP_HEADER_SIZE = 12;
const int RTP_MAX_PKT_SIZE = 1400;


struct Rtp_header {
    /* byte 0 */
    // CSRC计数器占4bit, 指示CSRC标识符个数
    uint8_t csrc_len:4;
    
    // 填充标志占1bit, x=1 -> RTP报头后跟有一个扩展头部
    uint8_t extionsion:1;
    
    // 填充标志占1bit, P=1 -> 在报文尾部填充一个或多个字节
    uint8_t padding:1;

    // RTP协议版本号占2位
    uint8_t version:2;


    /* byte 1 */
    // 有效载荷类型占7bit, 用于说明RTP报文中有效载荷的类型(GSM音频, JPEM图像等)
    uint8_t payload_type:7;

    // 标记占1位, 如果是视频 -> 标记一帧的结束, 如果是音频 -> 标记会话的开始
    uint8_t marker:1;


    /* byte 2, 3 */
    // 序号占16bit, 用于标识发送者所发送的RTP报文序号, 每发送一个报文, 序号+1
    uint16_t seq;


    /* byte 4-7 */
    // 时间戳占32bit, 反映RTP报文的第一个字节的采样时刻;
    // 可用于计算延迟和延迟抖动、进行同步控制
    uint32_t timestamp;


    /* byte 8-11 */
    // 标识同步源占32bit, 标识同步信号源, 数值是随机选择的
    // 参加统一视频会议的两个同步信源不能有相同的SSRC
    uint32_t ssrc;

    // RTP Header还可能存在0-15个特约信源(CSRC)标识符, 每个CSRC标识符占32位
    // 长度是前面csrc_len定义的
    // 每个CSRC标识了包含在RTP报文有效载荷中的所有特约信源

    // data
    uint8_t payload[0];
};

struct Rtcp_header {
    /* byte 0 */
    // 标识数据包个数
    uint8_t rc:5;

    // 填充标识
    uint8_t padding:1;

    // 版本号
    uint8_t version:2;


    /* byte 1*/
    // 数据包类型
    uint8_t packet_type;


    /* byte 2-3 */
    // 数据包长度(包含header)
    uint16_t length;
};

struct Rtp_packet {
public:
    Rtp_packet();
    ~Rtp_packet();
public:
    uint8_t *m_buf;
    // 可直接访问RTP报文的内容
    uint8_t *m_buf4;
    // 顶层const
    Rtp_header *const m_rtp_header;
    int m_size;
};

void parse_rtp_header(uint8_t *buf, struct Rtp_header *rtp_header);
void parse_rtcp_header(uint8_t *buf, struct Rtcp_header *rtcp_header);
#endif //RTP_H