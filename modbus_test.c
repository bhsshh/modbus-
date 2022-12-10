

#include "myfunc.h"
#include "mystruct.h"

int node_port, i;
int sockfd;
pthread_t tid;
pthread_t tid1;
struct list_head head;
struct shm_param send_shm_param; // 共享内存结构体
struct std_node *std_node;
struct list_head *pos;
struct mb_node *tmp;

void *data_collection(void *arg)
{
    // 创建共享内存
    int ret_shm = shm_init(&send_shm_param, "shm_test", 1024);
    if (ret_shm < 0)
    {
        LOGE("shm_init err.");
        // return -1;
    }

    // 获取共享内存地址
    void *shm_addr = shm_getaddr(&send_shm_param);

    // 将结构体首地址指向共享内存地址
    int *num = shm_addr;
    *num = 8;
    std_node = (struct std_node *)(shm_addr + sizeof(int));

    // 实时采集
    while (1)
    {
        // 前序（指针向前走）遍历链表
        i = 0;
        list_for_each_prev(pos, &head)
        {
            tmp = list_entry(pos, struct mb_node, list);

            LOGW("%d", i);
            printf("type = %d  key = %d, addr = %d name = %s\n", tmp->type, tmp->key, tmp->addr, tmp->name);
            std_node[i].type = tmp->type;
            std_node[i].key = tmp->key;
            std_node[i].dev_type = 1;
            if (std_node[i].key == tmp->key)
            {
                switch (tmp->type)
                {
                case 1:
                {
                    int val_bool = coil_status(sockfd, 1, 1, tmp->addr, 1);
                    std_node[i].new_val.b_val = val_bool;
                    std_node[i].ret = 0;
                    LOGN("switch = %d", std_node[i].new_val.b_val);
                    break;
                }
                case 2:
                    break;
                case 3:
                    if (tmp->key == 104)
                    {
                        float val_float = my_read_registers(sockfd, 1, 3, tmp->addr, 2);
                        std_node[i].new_val.f_val = val_float;
                        std_node[i].ret = 0;
                        LOGN("air-temp = %0.2f", std_node[i].new_val.f_val);
                        break;
                    }
                    else
                    {
                        float val_float = my_read_registers(sockfd, 1, 4, tmp->addr, 2);
                        std_node[i].new_val.f_val = val_float;
                        std_node[i].ret = 0;
                        LOGN("temp & humi = %0.2f", std_node[i].new_val.f_val);
                        break;
                    }
                default:
                    break;
                }
            }
            i++;
        }
        sleep(3);
    }
    pthread_exit(NULL); //结束线程
}

void *msg_control(void *arg)
{
    cJSON *root;
    struct msgbuf send_buf;
    struct msgbuf recv_buf;
    struct list_head *pos;
    struct mb_node *tmp;

    send_buf.mtype = 1;

    // 创建消息队列
    while (1)
    {
        if (msg_queue_recv("msga", &recv_buf, sizeof(recv_buf), 0, 0) < 0)
        {
            LOGE("modbus queue control.");
            exit(0);
        }

        printf("%s\n", recv_buf.mdata);

        root = cJSON_Parse(recv_buf.mdata);

        if (NULL == root)
        {

            LOGE("root err.");
            exit(-1);
        }

        //后序遍历链表 寻找要操控的key值 (指针向后走)
        list_for_each(pos, &head)
        {
            tmp = list_entry(pos, struct mb_node, list);
            if (cJSON_GetObjectItem(root, "key")->valueint == tmp->key)
            {
                int func;
                int addr = tmp->addr;
                if (tmp->key == 104) // 空调温度,保持寄存器
                {
                    func = 6; // 06:写单个保持寄存器
                    float value = atof(cJSON_GetObjectItem(root, "val")->valuestring);
                    if (my_write_register(sockfd, 1, func, addr, value) < 0)
                    {
                        LOGE("modbus_write err");
                        exit(0);
                    }
                }
                else
                {
                    func = 5; // 05:写单个线圈
                    int value = atoi(cJSON_GetObjectItem(root, "val")->valuestring);
                    if (my_write_coil(sockfd, 1, func, addr, value) < 0)
                    {
                        LOGE("modbus_write err");
                        exit(0);
                    }
                }

                // 回复上报进程消息
                cJSON *root_snd = cJSON_CreateObject();
                cJSON_AddItemToObject(root_snd, "type", cJSON_CreateNumber(2));
                cJSON_AddItemToObject(root_snd, "result", cJSON_CreateNumber(0));
                cJSON_AddItemToObject(root_snd, "msg", cJSON_CreateString("控制成功"));
                memmove(send_buf.mdata, cJSON_Print(root_snd), strlen(cJSON_Print(root_snd)));
                
                if (msg_queue_send("msgb", &send_buf, sizeof(send_buf), 0) < 0)
                {
                    LOGE("msg_queue_send error\n");
                    exit(-1);
                }

                memset(recv_buf.mdata, 0, sizeof(recv_buf.mdata));
                memset(send_buf.mdata, 0, sizeof(send_buf.mdata));
                cJSON_Delete(root_snd);;
                cJSON_Delete(root);
                break;
            }
        }
    }
    pthread_exit(NULL); //结束线程
}

int main(int argc, char const *argv[])
{
    // 定义
    char node_addr_buf[64] = {0};
    struct mb_node mbr_node[32];

    // 打开并读取点表数据,根据文件大小,精准分配缓冲区并读取文件的内容
    long node_size = fileopt_getsize("/mnt/config/node.json");
    char *node_read_buf = (char *)malloc(node_size);
    int ret = fileopt_readall("/mnt/config/node.json", node_read_buf);
    if (ret < 0)
    {
        LOGE("fileopt_readall err");
        return -1;
    }

    // 解析json点表
    cJSON *root;
    root = cJSON_Parse(node_read_buf);
    if (root == NULL) // 判断转换是否成功
    {
        LOGE("cjson error...");
        return -1;
    }

    // 解析Modbus Slave端ip地址和端口号
    // 进入到mb_dev层,该层是modbus设备地址端口号配置，获取到ip地址和端口号
    sprintf(node_addr_buf, "%s", cJSON_GetObjectItem(cJSON_GetObjectItem(root, "mb_dev"), "addr")->valuestring);
    node_port = cJSON_GetObjectItem(cJSON_GetObjectItem(root, "mb_dev"), "port")->valueint;

    // 初始化内核链表
    INIT_LIST_HEAD(&head);

    // 获取设备的信息并添加到内核链表
    root = cJSON_GetObjectItem(root, "modbus");
    root = cJSON_GetObjectItem(root, "data");
    int DATA_size = cJSON_GetArraySize(root);
    cJSON *item = NULL;
    for (i = 0; i < DATA_size; i++)
    {
        item = cJSON_GetArrayItem(root, i);

        // 将点表中的key值存到结构体中
        mbr_node[i].key = cJSON_GetObjectItem(item, "key")->valueint;
        // 将点表中的name数据存到结构体中
        memmove(mbr_node[i].name, cJSON_GetObjectItem(item, "name")->valuestring, strlen(cJSON_GetObjectItem(item, "name")->valuestring));
        // 将点表中的addr值存到结构体中
        mbr_node[i].addr = cJSON_GetObjectItem(item, "addr")->valueint;
        // 将点表中的type值存到结构体中
        mbr_node[i].type = cJSON_GetObjectItem(item, "type")->valueint;
        // 在头节点插入节点，使其能够向后移动
        list_add(&mbr_node[i].list, &head);
    }

    // 创建连接
    sockfd = my_connect(node_addr_buf, node_port);
    if (sockfd < 0)
    {
        LOGE("connect error");
        return -1;
    }

    // 创建线程
    if (pthread_create(&tid, NULL, data_collection, NULL) != 0)
    {
        LOGE("data_collection pthread_create error");
        return -1;
    }

    if (pthread_create(&tid1, NULL, msg_control, NULL) != 0)
    {
        LOGE("msg_control pthread_create error");
        return -1;
    }

    pthread_join(tid, NULL);  // 阻塞等待线程退出
    pthread_join(tid1, NULL); // 阻塞等待线程退出
    close(sockfd);
    cJSON_Delete(root); // 清除结构体
    free(node_read_buf);
    node_read_buf = NULL;
    // shm_del(&send_shm_param); // 解除共享内存
    return 0;
}
