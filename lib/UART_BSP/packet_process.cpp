#include <packet_process.h>

/* 初始化缓冲区 */
void CommandBuffer_Init(CommandBuffer_T *cbuf, uint8_t *ext_buf, uint8_t buf_size, CommandConfig config)
{
    cbuf->buffer = ext_buf;
    cbuf->buf_size = buf_size;
    cbuf->read_index = 0;
    cbuf->write_index = 0;
    cbuf->config = config;
    memset(ext_buf, 0, buf_size);
}
/* 计算有效数据长度 */
static inline uint8_t get_data_length(const CommandBuffer_T *cbuf)
{
    return (cbuf->write_index + cbuf->buf_size - cbuf->read_index) % cbuf->buf_size;
}
/* 计算剩余空间 */
static inline uint8_t get_free_space(const CommandBuffer_T *cbuf)
{
    return cbuf->buf_size - get_data_length(cbuf);
}
/* 索引递增（带自动回绕） */
static inline void index_increment(CommandBuffer_T *cbuf, uint8_t *index, uint8_t inc)
{
    *index = (*index + inc) % cbuf->buf_size;
}
/* 从指定位置读取数据 */
static uint8_t read_buffer(const CommandBuffer_T *cbuf, uint8_t pos)
{
    return cbuf->buffer[pos % cbuf->buf_size];
}
/* 写入数据到缓冲区 */
uint8_t CommandBuffer_Write(CommandBuffer_T *cbuf, const uint8_t *data, uint8_t length)
{
    if (get_free_space(cbuf) < length)
        return 0;

    uint8_t remain = cbuf->buf_size - cbuf->write_index;

    if (remain >= length)
    {
        memcpy(&cbuf->buffer[cbuf->write_index], data, length);
    }
    else
    {
        memcpy(&cbuf->buffer[cbuf->write_index], data, remain);
        memcpy(cbuf->buffer, data + remain, length - remain);
    }

    index_increment(cbuf, &cbuf->write_index, length);
    return length;
}

/* 解析完整指令 */
uint8_t CommandBuffer_Parse(CommandBuffer_T *cbuf, uint8_t *output, uint8_t max_output)
{
    while (get_data_length(cbuf) >= cbuf->config.min_length)
    {
        // 包头检测
        uint8_t start = cbuf->read_index;
        if (read_buffer(cbuf, start) != cbuf->config.header)
        {
            index_increment(cbuf, &cbuf->read_index, 1);
            continue;
        }
        // 长度校验
        uint8_t pkg_len = read_buffer(cbuf, start + 1);
        if (pkg_len < cbuf->config.min_length ||
            pkg_len > max_output)
        {
            index_increment(cbuf, &cbuf->read_index, 1);
            continue;
        }
        // 数据完整性检查
        if (get_data_length(cbuf) < pkg_len)
            return 0;
        // 校验和验证（我还是那么喜欢异或校验和）
        uint8_t checksum = 0;
        for (uint8_t i = 0; i < pkg_len - 1; ++i)
        {
            checksum ^= read_buffer(cbuf, start + i);
        }
        if (checksum != read_buffer(cbuf, start + pkg_len - 1))
        {
            index_increment(cbuf, &cbuf->read_index, 1);
            continue;
        }
        // 提取有效指令
        for (uint8_t i = 0; i < pkg_len; ++i)
        {
            output[i] = read_buffer(cbuf, start + i);
        }

        index_increment(cbuf, &cbuf->read_index, pkg_len);
        return pkg_len;
    }
    return 0;
}

