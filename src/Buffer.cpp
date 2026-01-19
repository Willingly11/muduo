#include <sys/uio.h>
#include <unistd.h>

#include "Buffer.h"
#include "Logger.h"

int Buffer::readFd(int fd, int* savedErrno)
{
    //
    char extrabuf[65536];
    struct iovec vec[2];

    const size_t writeable = writeableBytes();
    vec[0].iov_base = buffer_.data() + writeIndex_;
    vec[0].iov_len = writeable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovCnt = (writeable < sizeof extrabuf ) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovCnt); // 分散读
    if(n < 0)
    {
        *savedErrno = errno;
    }
    else if(static_cast<size_t>(n) <= writeable)
    {
        writeIndex_ += n;
    }
    else
    {
        writeIndex_ = buffer_.size();
        append(extrabuf, n - writeable);
    }
    return n;
}

//将buffer_中的数据写入fd
int Buffer::writeFd(int fd, int* savedErrno)
{
    int n = ::write(fd, peek(), readableBytes());
    if(n < 0)
    {
        *savedErrno = errno;
    }
    return n;
    /*保持现状：不移动 readIndex_。
    原因：在非阻塞写的情况下，可能只写入了一部分数据。
    如果在这里retrieve，一旦上层逻辑复杂，很难控制重发逻辑。
    应该遵循：Write -> 根据返回值 n -> Retrieve(n) 的模式。*/
}