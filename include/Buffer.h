#pragma once

#include "noncopyable.h"
#include <cstddef>
#include <string>
#include <vector>

#include "noncopyable.h"
class Buffer : noncopyable
{
public:
    static const size_t kInitialSize = 1024;
    static const size_t kCheapPrepend = 8;

    Buffer()
        : buffer_(kCheapPrepend + kInitialSize),
          readIndex_(kCheapPrepend),
          writeIndex_(kCheapPrepend)
    {
    }

    int readFd(int fd, int* savedErrno); //从fd中读取数据到buffer_
    int writeFd(int fd, int* savedErrno); //将buffer_中的数据写入fd

    //偷看：返回当前可读数据的指针，可以使用数据，但不会移动 readIndex_ 游标
    const char* peek() const { return buffer_.data() + readIndex_; }

    size_t readableBytes() const { return writeIndex_ - readIndex_; }
    size_t writeableBytes() const { return buffer_.size() - writeIndex_; }
    size_t prependableBytes() const { return readIndex_; }

    //添加数据：将data写入buffer_
    void append(const char* data, size_t len)
    {
        if(writeableBytes() < len) makeSpace(len);
        std::copy(data, data + len, buffer_.begin() + writeIndex_);
        writeIndex_ += len;
    }

    //消费数据：移动 readIndex_ 游标
    void retrieve(size_t len)
    {
        if(len < readableBytes())
            readIndex_ += len;
        else
            retrieveAll();
    }
    //消费所有数据
    void retrieveAll()
    {
        readIndex_ = kCheapPrepend;
        writeIndex_ = kCheapPrepend;    
    }

    //将可读数据以string形式返回
    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

private:
    void makeSpace(size_t len)
    {
        if(prependableBytes() + writeableBytes() - kCheapPrepend >= len)
        {
            size_t readable = readableBytes();
            std::copy(buffer_.begin() + readIndex_, 
                        buffer_.begin() + writeIndex_, 
                        buffer_.begin() + kCheapPrepend);
            readIndex_ = kCheapPrepend;
            writeIndex_ = readIndex_ + readable;
        }
        else
        {
            buffer_.resize(writeIndex_ + len);
        }
    }

    std::vector<char> buffer_;
    int readIndex_;
    int writeIndex_;
};