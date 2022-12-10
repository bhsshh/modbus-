
#ifndef _MYSTRUCT_H_
#define _MYSTRUCT_H_

#include "myfunc.h"

// 采集进程与上报进程的通信
union val_t
{
    int b_val;   // bool类型存储空间
    int i_val;   //整形值存储空间
    float f_val; //浮点值存储空间
};

struct std_node
{
    int key;             //唯一键值
    int type;            //数据点类型
    int dev_type;        //数据点属于哪个设备，根据网关支持的设备自行定义
    union val_t old_val; //变化上报后需要更新旧值
    union val_t new_val; //从共享内存取出最新数据，放到new_val中
    int ret;             //默认为-1，采集成功后设置为0，采集失败再置-1
};

// modbus用来存储点表设备信息的结构体
struct mb_node
{
    int key;        // 数据点唯一标识(确保数据点表内的唯一性)
    char name[128]; // 数据点名称(确保单个设备内的唯一性)
    int type;       // 数据点值的类型,1：bool类型 2：int型  3：float型
    int addr;       // modbus地址,根据设备类型选择
    struct list_head list;
};

struct msgbuf
{
    long mtype;
    char mdata[256];
};

#endif