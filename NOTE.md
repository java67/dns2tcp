# libev 简记
头文件 `#include <ev.h>`

两个核心概念：`evloop`事件循环、`watcher`事件监听器。
每个线程都应该拥有各自独立的 evloop 事件循环，互不相干。

两种常用watcher：`ev_io`I/O监听器(可读/可写)，`ev_timer`相对定时器(定时器到期)。

# 创建loop
默认循环：`struct ev_loop* ev_default_loop(unsigned int flags)`，单线程程序建议用这个，flags 同 ev_loop_new() 的 flags。
新建循环：`struct ev_loop* ev_loop_new(unsigned int flags)`，多线程程序建议用这个，flags 用 0 就行，其它的用不到，不用看。

# 运行loop
`ev_run(loop, 0)`，调用线程将一直阻塞在这里，执行事件循环，通常这将是该线程的最后一个语句。
当一个回调执行很长时间时，建议在回调执行后，执行一次 `ev_now_update(loop)` 来更新 libev 的内部时间。

# loop->data
`ev_set_userdata(loop, void *data)`、`void *ev_userdata(loop)`
可以用来设置 loop 结构体上的自定义数据，等价于 libuv 的 loop->data 字段。比如可以用来实现 threadlocal，当然在 gcc 中建议直接使用 `__thread` 魔术关键字。

# 通用/工具API
- `ev_sleep(ev_tstamp interval)`：跨平台 sleep()，ev_tstamp 实际是 `double` 类型，单位是秒 `second`。

# watcher
所有 watcher 的回调函数签名都是一样的：`void (*watcher_cb)(struct ev_loop *loop, struct ev_TYPE *watcher, int events)`
- EV_READ：fd 可读。
- EV_WRITE：fd 可写。
- EV_TIMER：定时器到期。
- EV_CUSTOM：自定义事件，由用户使用，libev不使用它，可以通过 `ev_feed_event()` 来汇入此 event。

所有 watcher 的基本使用流程：
- 通用初始化：`ev_init(struct ev_TYPE *watcher, callback)`，所有类型的 watcher 都需要先调用它，这样才能被 libev 使用。
- 设置 watcher：`ev_TYPE_set(struct ev_TYPE *watcher, args...)`，为特定类型的 watcher 设置自己的参数/数据，是可变参数。
- 初始化并设置：`ev_TYPE_init(struct ev_TYPE *watcher, callback, args...)`，组合宏，第一次创建 watcher 时调用它非常方便。
- 将 watcher 注册到 loop：`ev_TYPE_start(struct ev_loop *loop, struct ev_TYPE *watcher)`，注册到了事件循环中才实际开始监听它。
- 当对应的监听事件发生时，libev 将会调用我们传递给它的事件回调，也就是 `callback(loop, watcher, events)`，回调中不应该执行阻塞操作。
- 当我们不需要监听对应事件时，需要调用 `ev_TYPE_stop(struct ev_loop *loop, struct ev_TYPE *watcher)` 方法来从 loop 中取消注册该 watcher。
- 最后我们可以释放 watcher 占用的内存，需要注意的是，在 libev 和 libuv 中，watcher 占用的内存由我们自己管理，包括申请和释放，这是一个很好的设计。
- watcher 一旦 start 了，就不能再调用 `ev_init()`、`ev_TYPE_set()`，如果需要重新调用 `ev_TYPE_set()`，需要先 stop，然后再进行其他任意操作（对，任意）。
> 牢记：只有 ev_TYPE_start()、ev_TYPE_stop() 才会实际影响到事件循环对象，ev_init()、ev_TYPE_set() 都仅仅是操作 watcher 所在的内存块，没有做其他任何操作。

ev_TYPE_start() 和 ev_TYPE_stop() 可以多次调用，但是只有第一次调用有效，后续的调用都会被忽略。

watcher 的几个状态：
- initialised：已初始化。也即调用过 `ev_init()` + `ev_TYPE_set()`。
- started/running/active：已注册的。也即调用过 `ev_TYPE_start()`，但还未 stop。
- pending：已注册的，且有待调用的 callback 正在等待执行，一旦callback被执行，则恢复为active状态。
- stopped：等价于 initialised 状态，调用 `ev_TYPE_stop()` 时，就处于 stopped 状态，对于 I/O watcher，stop 不会改变我们的 ev_TYPE_init() 设定值。
> 但是 timer watcher 建议每次 stop 之后，都要手动调用一次 ev_timer_set()，当然这在大多数情况下都不会有问题，因为你停止了定时器肯定是会做一些设定改变的。

watcher 其它一些 API：
- `bool ev_is_active(struct ev_TYPE *watcher)`：是否为 active 状态，也即 start 了，但还未 stop。
- `bool ev_is_pending (ev_TYPE *watcher)`：是否为 pending 状态，也就是说是否有 callback 在等待执行。
- `callback ev_cb(ev_TYPE *watcher)`：获取给定 watcher 的 callback 指针。
- `ev_set_cb(ev_TYPE *watcher, callback)`：设置给定 watcher 的 callback，随时都可以修改它。
- `ev_invoke(loop, ev_TYPE *watcher, int revents)`：手动调用给定 watcher 的回调，revents 可以是任意值，因为它仅仅做一个简单的参数传递并调用给定cb。
- `ev_feed_event(loop, ev_TYPE *watcher, int revents)`：将给定的 events 汇入对应的 watcher，就如同发生了此事件一样，注意此函数返回不意味着回调已调用。

每个 watcher 身上都有一个 `void *data` 字段，用于存放自定义数据，libev 不会使用它。如果自定义数据只有一个，那么可以直接利用 data 字段。如果有多个自定义数据，更好的方式是自己定义一个 watcher 类型， 其中第一个字段为底层实际的 watcher，后续的字段就是我们的自定义数据，这样实际能够减少一些内存碎片，提高性能。例子：
```
struct my_io {
    struct ev_io io; //第一个字段放原生的watcher
    int otherfd; //自定义数据1
    void *somedata; //自定义数据2
    struct whatever *mostinteresting; //自定义数据3
};
```
结构体的一个成员始终在 offset 为 0 的位置，因此 my_io 结构体和其第一个成员 io 的起始地址是一样的，对于指针类型，我们只需进行强制类型转换即可。

# io_watcher
相关函数：
ev_io_set(ev_io *watcher, int fd, int events)
ev_io_init(ev_io *watcher, callback, int fd, int events)

只读成员：
int fd [read-only]：调用 ev_io_set() 时设置的 fd。
int events [read-only]：调用 ev_io_set() 是设置的 events。

`ev_feed_fd_event(loop, int fd, int revents)`：将给定 events 汇入到给定 fd 关联的 watcher 上，实际是 ev_feed_event() 的封装函数.

# timer_watcher
相关函数：
ev_timer_set(ev_timer *watcher, ev_tstamp after, ev_tstamp repeat)：after表示多少秒后发生第1次超时，如果repeat为0则为一次性定时器，非0则为重复定时器，即第1次超时后，将使用repeat值作为后续的超时时间。
ev_timer_init(ev_timer *watcher, callback, ev_tstamp after, ev_tstamp repeat)：同上。只是一个组合宏。
ev_timer_again(loop, ev_timer *watcher)：启动/重置定时器，用例见后，用它替代 ev_timer_start() 在某些情况下性能更佳。
> 如果 timer->repeat 为 0，那么他就是一次性定时器，当发送超时且回调被调用后，libev 将自动 stop 这个 timer。

相关成员：
ev_tstamp repeat [read-write]：可读可写，主要用于 ev_timer_again()，修改下次超时时间用的。

在现实问题中，经常有这么一个需求，要用到定时器，而且经常“重启定时器”。比如：HTTP 服务器中，对于每个 HTTP 客户端连接，都需要一个 60 秒的定时器，定时器的作用是：如果客户端在 60 秒内未进行任何操作，那么就关闭该 HTTP 连接，避免资源浪费以及恶意攻击。可以知道，当一个新的 HTTP 连接被创建时，同时也会创建一个对应的 Timer，并且设置超时为 60秒，然后随机启动这个 Timer。当 HTTP 客户端做了一些“活动操作”（比如发送了一些数据），那么这个 Timer 就会重置，也即 stop 然后 start，超时时间被重置为 60秒，可以理解为刷新定时器。

假定程序使用 Libev 进行开发，那么最简单的实现方式就是：
- 最开始时，创建 timer，并启动它：
```
ev_timer_init(timer, callback, 60, 0);
ev_timer_start(loop, timer);
```
- 当客户端活动时，重置定时器：
```
ev_timer_stop(loop, timer);
ev_timer_set(timer, 60, 0); //此步不能省略
ev_timer_start(loop, timer);
```
但这个实现方式的性能却不是最佳，因为 stop 的时候，需要完全从 libev 中移除这个 timer，然后又要将它添加进 libev。

另一个官方建议的方法，性能比上面的好很多，如下：
- 最开始时：
```
ev_timer_init(timer, callback, 0, 60); //注意只使用repeat字段,delay字段为0
ev_timer_again(loop, timer); //注意不使用start，而是使用again启动/重置timer
```
- 客户端活动时：
```
//也可以更改超时时间，如改为30s，则在again调用前，调用ev_timer_set(timer, 0, 30)来修改
ev_timer_again(loop, timer); //重置定时器
```
使用方式比第一种原始方法更加简单，且性能也更优异。因为不需要频繁从定时器堆中移除、添加、移除、添加。

# socket option
- IPV6_V6ONLY：监听地址为 `::` 时，不监听 `0.0.0.0` ipv4通配地址，仅监听 `::` ipv6通配地址。
- SO_REUSEADDR：地址重用，TCP 监听套接字必备，UDP 套接字如果需要临时偷用端口，也请使用此选项。
- SO_REUSEPORT：端口重用，内核级别的负载均衡，注意使用前请先检查系统是否支持，貌似是 v3.9+ 才有。
- TCP_NODELAY：TCP 连接套接字必备，禁用 nagle 算法。影响：当我们调用 send() 时，尽快将数据发送给对方。
- TCP_QUICKACK：TCP 连接套接字必备，禁用 delayed ack。影响：本端收到 tcp-segment 时，尽快将 ACK 发给对方。
- TCP_SYNCNT：仅限 Linux 系统，可以用来设置 connect 的超时时间，默认超时时间太长了，强烈建议设置此选项来修改。
- SO_LINGER：用于 TCP 连接套接字，用来产生 TCP-RST 效果，即：启用 so_linger 选项并将超时设为 0，然后调用 close()。
- TCP_FASTOPEN：TCP 快速连接，仅内核 3.7+ 以上支持，同时需要设置内核选项，其值设置为 3 表示支持客户端和服务端的 TFO。

**TCP_SYNCNT** 详解：
这个选项可以用来调整 TCP 的连接超时时间，那么连接超时时间是怎么算出来的呢？首先来看系统默认的超时时间：
1. 第 1 次发送 SYN 报文后等待 1s (2^0)，等待时间内若未收到对端的 SYN+ACK 报文，则发生超时，再次重试
2. 第 2 次发送 SYN 报文后等待 2s (2^1)，等待时间内若未收到对端的 SYN+ACK 报文，则发生超时，再次重试
3. 第 3 次发送 SYN 报文后等待 4s (2^2)，等待时间内若未收到对端的 SYN+ACK 报文，则发生超时，再次重试
4. 第 4 次发送 SYN 报文后等待 8s (2^3)，等待时间内若未收到对端的 SYN+ACK 报文，则发生超时，再次重试
5. 第 5 次发送 SYN 报文后等待 16s(2^4)，等待时间内若未收到对端的 SYN+ACK 报文，则发生超时，再次重试
6. 第 6 次发送 SYN 报文后等待 32s(2^5)，等待时间内若未收到对端的 SYN+ACK 报文，则发生超时，再次重试
7. 第 7 次发送 SYN 报文后等待 64s(2^6)，等待时间内若未收到对端的 SYN+ACK 报文，则发生超时，放弃连接
可以看到内核总共会尝试发送 7 次 SYN 报文，如果仍然未收到 SYN+ACK 报文，则告知应用层，发送连接超时(Connection timed out)，超时时间有点长。
我们可以通过 `net.ipv4.tcp_syn_retries` 内核选项来全局调整重试次数，Linux 的默认重试次数为 6，因此默认超时时间为 1+2+4+8+16+32+64=127 秒。
很多时候我们并没有权限去修改内核参数，好在可以通过 TCP_SYNCNT 套接字选项来设置给定 socket 的 SYN 重试次数，比如改为 2，即 1 + 2 + 4 = 7 秒。

**SO_REUSEADDR** 详解：
// TODO

**SO_REUSEPORT** 详解：
// TODO

**TCP_FASTOPEN** 详解：
// TODO
