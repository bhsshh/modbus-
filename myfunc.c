#include "myfunc.h"

// 字节序置换函数
int conver_endian_long(unsigned char *dst, const unsigned char *src, int len)
{
    int i = 0;

    if (len % 4 != 0)
    {
        printf("err len\n");
        return -1;
    }

    while (i < len)
    {
        dst[i] = src[i + 3];
        dst[i + 1] = src[i + 2];
        dst[i + 2] = src[i + 1];
        dst[i + 3] = src[i];
        i += 4;
    }

    return 0;
}

/**
 * @description: 连接函数
 * @param {char} *ipaddr ip地址
 * @param {int} port 端口号
 * @return {*}
 */
int my_connect(char *ipaddr, int port)
{
    int sockfd;
    // 1. 创建一个套接字--socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        LOGE("socket err"); // APP_ERR适用于出现问题并且会导致出错的输出
        exit(-1);
    }
    // 2. 指定服务器地址--sockaddr_in   填充结构体
    int addrlen = sizeof(struct sockaddr);
    struct sockaddr_in serveraddr;
    bzero(&serveraddr, addrlen);
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(ipaddr);
    serveraddr.sin_port = htons(port);
    // 3. 连接服务器--connect
    if (connect(sockfd, (struct sockaddr *)&serveraddr, addrlen) < 0)
    {
        LOGE("connect err");
        exit(-1);
    }
    return sockfd;
}

/**
 * @description: 读输入寄存器函数 04 Input Registers & 读保持寄存器 03 Holding Registers
 * @param {int} sockfd 套接字
 * @param {int} id 单元标识符，即ID
 * @param {int} function 功能码
 * @param {int} addr 读取的起始寄存器地址
 * @param {int} num 读取的寄存器个数
 * @return {*}
 */
float my_read_registers(int sockfd, int id, int function, int addr, int num)
{
    //发送命令缓冲区
    unsigned char send_buf[128] = {0x08, 0x0B, 0x00, 0x00, 0x00, 0x06};
    //接收缓冲区
    unsigned char recv_buf[128] = {0};

    char addr_buf[16];
    int addr_t = addr % 10000 - 1;     // 取后4位再减1
    sprintf(addr_buf, "%04x", addr_t); // 转换为4位十六进制地址
    int lhex_addr, hhex_addr;          // 低位、高位地址
    lhex_addr = atoi(addr_buf) & 0xFF;
    hhex_addr = atoi(addr_buf) >> 8 & 0xFF;

    char num_buf[16];
    sprintf(num_buf, "%04x", num); // 转换为4位十六进制数
    int lhex_num, hhex_num;        // 低位、高位个数
    lhex_num = atoi(num_buf) & 0xFF;
    hhex_num = atoi(num_buf) >> 8 & 0xFF;

    *(send_buf + 6) = id;
    *(send_buf + 7) = function;
    *(send_buf + 8) = hhex_addr;
    *(send_buf + 9) = lhex_addr;
    *(send_buf + 10) = hhex_num;
    *(send_buf + 11) = lhex_num;

    //这里send的12个字节一定注意，不能多发，协议非常严格，要求多少就是多少，发多了会认为是错误帧，如果连发两帧错误振，那么会被强制断开连接。
    send(sockfd, send_buf, 12, 0);
    int ret = recv(sockfd, recv_buf, 128, 0);
    if (ret < 0)
    {
        perror("recv err");
        return -1;
    }
    //解析出需要的数据来
    float *p = recv_buf + 9; //将指针偏移到数据位置

    //因为接收的数据是按照大端ABCD存储的，而我们的CPU是小端模式，所以要将字节序置换
    float data;
    conver_endian_long(&data, p, 4);
    return data;
}

/**
 * @description: 线圈状态寄存器 01 coil_status
 * @param {int} sockfd 套接字
 * @param {int} id 单元标识符，即ID
 * @param {int} function 功能码
 * @param {int} addr 读取的起始寄存器地址
 * @param {int} num 读取的寄存器个数
 * @return {*}
 */
int coil_status(int sockfd, int id, int function, int addr, int num)
{
    //发送命令缓冲区
    unsigned char send_buf[128] = {0x08, 0x0B, 0x00, 0x00, 0x00, 0x06};
    //接收缓冲区
    unsigned char recv_buf[128] = {0};

    char addr_buf[16];
    int addr_t = addr % 10000 - 1;     // 取后4位再减1
    sprintf(addr_buf, "%04x", addr_t); // 转换为4位十六进制地址
    int lhex_addr, hhex_addr;          // 低位、高位地址
    lhex_addr = atoi(addr_buf) & 0xFF;
    hhex_addr = atoi(addr_buf) >> 8 & 0xFF;

    char num_buf[16];
    sprintf(num_buf, "%04x", num); // 转换为4位十六进制数
    int lhex_num, hhex_num;        // 低位、高位个数
    lhex_num = atoi(num_buf) & 0xFF;
    hhex_num = atoi(num_buf) >> 8 & 0xFF;

    *(send_buf + 6) = id;
    *(send_buf + 7) = function;
    *(send_buf + 8) = hhex_addr;
    *(send_buf + 9) = lhex_addr;
    *(send_buf + 10) = hhex_num;
    *(send_buf + 11) = lhex_num;

    //这里send的12个字节一定注意，不能多发，协议非常严格，要求多少就是多少，发多了会认为是错误帧，如果连发两帧错误振，那么会被强制断开连接。
    send(sockfd, send_buf, 12, 0);
    int ret = recv(sockfd, recv_buf, 128, 0);
    if (ret < 0)
    {
        perror("recv err");
        return -1;
    }
    return recv_buf[9];
}

long FloatTohex(float HEX) // 浮点数到十六进制转换
{
    return *(long *)&HEX;
}

// 写线圈函数
int my_write_coil(int sockfd, int id, int function, int addr, int value)
{
    //发送命令缓冲区
    unsigned char send_buf[128] = {0x08, 0x0B, 0x00, 0x00, 0x00, 0x06};
    //接收缓冲区
    unsigned char recv_buf[128] = {0};

    char addr_buf[16];
    int addr_t = addr % 10000 - 1;     // 取后4位再减1
    sprintf(addr_buf, "%04x", addr_t); // 转换为4位十六进制地址
    int lhex_addr, hhex_addr;          // 低位、高位地址
    lhex_addr = atoi(addr_buf) & 0xFF;
    hhex_addr = atoi(addr_buf) >> 8 & 0xFF;

    if (function == 5 && value == 1) // 5:写线圈,1:线圈开关 开
    {
        value = 0xFF00; // 断通标志为0xFF00表示置位

        int lval_num, hval_num; // 低位、高位个数
        lval_num = value & 0xFF;
        hval_num = (value & 0xFF00) >> 8;

        *(send_buf + 6) = id;
        *(send_buf + 7) = function;
        *(send_buf + 8) = hhex_addr;
        *(send_buf + 9) = lhex_addr;
        *(send_buf + 10) = hval_num;
        *(send_buf + 11) = lval_num;
    }
    else if (function == 5 && value == 0)
    {
        int lval_num, hval_num; // 低位、高位个数
        lval_num = value & 0xFF;
        hval_num = (value & 0xFF00) >> 8;

        *(send_buf + 6) = id;
        *(send_buf + 7) = function;
        *(send_buf + 8) = hhex_addr;
        *(send_buf + 9) = lhex_addr;
        *(send_buf + 10) = hval_num;
        *(send_buf + 11) = lval_num;
    }

    send(sockfd, send_buf, 12, 0);
    recv(sockfd, recv_buf, 64, 0);
    return 0;
}

// 写寄存器函数
int my_write_register(int sockfd, int id, int function, int addr, float value)
{
    //发送命令缓冲区
    unsigned char send_buf[128] = {0x08, 0x0B, 0x00, 0x00, 0x00, 0x08};
    //接收缓冲区
    unsigned char recv_buf[128] = {0};

    char addr_buf[16];
    int addr_t = addr % 10000 - 1;     // 取后4位再减1
    sprintf(addr_buf, "%04x", addr_t); // 转换为4位十六进制地址
    int lhex_addr, hhex_addr;          // 低位、高位地址
    lhex_addr = atoi(addr_buf) & 0xFF;
    hhex_addr = atoi(addr_buf) >> 8 & 0xFF;

    float Hdecimal = value;
    long hX = FloatTohex(Hdecimal); // 浮点数转换为十六进制

    *(send_buf + 6) = id;
    *(send_buf + 7) = function;
    *(send_buf + 8) = hhex_addr;
    *(send_buf + 9) = lhex_addr;
    *(send_buf + 10) = (hX & 0xFF000000) >> 24;
    *(send_buf + 11) = (hX & 0x00FF0000) >> 16;
    *(send_buf + 12) = (hX & 0x0000FF00) >> 8;
    *(send_buf + 13) = (hX & 0x000000FF);

    send(sockfd, send_buf, 14, 0);
    recv(sockfd, recv_buf, 64, 0);
    return 0;
}
