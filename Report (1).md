# Report
项目开源链接：https://github.com/JKQJQ/
## Design

### 运行依赖
* g++ (GCC) 8.5.0 20210514 (Red Hat 8.5.0-4)
* cmake version 3.20.2
* zstd version 1.5.2

### 架构设计

我们将整体项目划分为文件传输、数据处理、模拟撮合三部分。

考虑到比赛时限是$45min$，而每支股票有全量一亿条order数据，平均到两台trader机器上是五千万条，在相同存储格式下，trade条数约是order的一半，所以在真正发生交易时，两者的传输是overlap的，因此总体耗时就是传输order时间。

如果trader将自己全量的数据上传至exchange，在三条$1.5Mbps$ TCP连接的条件下，正好需要传输约$45min$（比赛规定时间）：

$$500 \times 1000 \times 1000 \times (4int + 1 double)/1024/1024/4.5MB/60(sec/min) = 42.38min$$


考虑到比赛中的一些突发情况，因此我们考虑对文件传输做优化，**通过一些精妙的设计**。trader1将一半的数据通过exchange 1传输，另一半数据通过exchange 2传输，直接将order信息转发到trader2本地。在两台trader本地对全量order分别进行模拟撮合。所以我们需要的时间是原来的一半，为$21.19min$。

此外我们在数据处理部分对数据进行了两种类型的压缩，数据压缩率约为31.4%。所以理论传输速度是

$$21.19min \times 31.4\%=6.65min$$

我们实际测试中我们测得的传输时间为8分钟以内。

我们的数据压缩、数据传输和数据解压串行和并行结合，在程序中大量采用了"pending when a file is not ready"的机制，提前启动各部分程序，等待其他部分做完处理，把数据压缩、文件传输、撮合整个pipeline在测试环境下的时间控制在了15分钟以内。



## 文件传输（File Transfer Duplex）
我们把数据分成了500份，以保证传输过程中能够重传。当传输完一个文件a，我们会额外在磁盘内写入一个```a.success```文件，以标记a传输完成。

我们把两个exchange当成数据的forwarder，两个trader通过两台exchanger来中转数据，最终数据在两个trader中分别撮合。

```
                   orders1 =>         orders1 =>
[0, 250):   trader 1 <><><> exchange 1 <><><> trader 2
                    <= orders2       <= orders2
                    orders1 =>         orders1 =>
[250, 500): trader 1 <><><> exchange 2 <><><> trader 2
                    <= orders2       <= orders2
```

以上是我们的程序架构,"<>"代表一条```1.5MB/s```的channel，"<><><>"代表3条这样的channel并联，达到```4.5MB/s```的带宽。为了充分利用每一条```4.5MB/s```的带宽，我们设计了一个**双向的全双工**发送文件的程序，把它命名为```File Transfer Duplex```。（后面简称FTD）
上图中Trade1到Exchange1，Exchange1到Trade2传输几乎是并行的，总时间几乎是串行时间的一半。

为了解决TCP的粘包问题，我们采用了等长的包，它的长度为$N$（目前是$1400 Bytes$）。

为了方便部署，我们在每台trader上部署6个server进程，每台exchange上部署6个client进程。虽然有server和client的区分，它们的区别仅在于建立连接时，建立连接后它们是平等的地位，我们采用同一个request handler去发送、接收和处理网络包。所有的trader和exchange既是发送者又是接收者，因此下面我们用发送者和接收者，或者worker来指代它们。

我们生成的500份文件是有固定文件名的，FTD会在运行的时候产生需要从另一端获取的文件名队列，后面如果发现需要请求文件的对应.success文件存在，我们就不会再去发送请求获取，这保证了出现失误后重新启动程序重传的正确性。

在建立连接后，一个worker会先用socket的`send()`发送请求，然后再用`recv()`来获取请求，它们得到的数据量每次都是$N$，以确保每个$N$长度的包内容都能被我们控制。包头有三类，一类是请求文件的，一类是传输数据的，还有一类是没有任何意义的空包（但它的长度还是$N$，不足的会用内存里的随机数据填满）。请求文件的包头会比其他包头有更高优先级，为了节约空间，请求的文件名后面可以跟其他数据包内容，如果跟其他包头共存此类包头会在包的最前面。所有包头及格式罗列如下：

1. request packets: `R:requested_file_name:other packet`, other packets could either be data packets or nil packets. if `R::other packet`, this means no file is being requested. If no data, the other packet should be a nil packet(`N:`).
2. data packets: `B:file_size:payload(begin)` or `M:payload(middle)`. 有两类，前一类是起始数据包，会包含当前传输文件大小，后一类是中间数据包，包头后面直接是数据，一个文件的最后一个中间数据包后面不足的会用内存随机数据填充。
3. nil packets: `N:`, used when there is no data to transfer, only instruction is transferred.
4. end packets: `E:`, this packet is sent to end a connection when the transfer is finished.
5. pending packets: `P:` the packet is sent when the file is not ready, almost equivalent to nil packet.


接收者在起始数据包内获取到当前传输文件大小之后有义务去记录一共获取了多少数据以及一个文件到哪个包终止，所有无意义的填充数据都应该被丢弃。

接收者也都是请求者（发送者），反之亦然。被请求的文件总是在请求者请求队列的最前面，处理完了之后才会从队列里面拿出，然后再请求下一个文件。

发送者有义务保证发送的每一个包都是定长的$N$，如果数据不足，需要用随机数据填满。

为了保证发送者和接收者处理的数据都是长度$N$的，在获取到 `recv()`和`send()`后，会判断它的长度是不是$N$，如果不是，会不断尝试直到获取到的数据填满长度是N的缓存数组。

为了将传输任务几乎均匀地分散到6条链路上，FTD提供了workerId、nWorkers以及fileId三个参数，fileId是```[0,500)```内的一个小范围，一般来说是Exchange 1中转```[0,250)```的数据，Exchange 2中转```[250,500)```范围内的数据。分成两份之后还会根据任务数对$nWorkers$求余来获取单个任务被哪个workerId的worker来向远端请求获取。

在所有的传输完成后，cient和server都会相互传结束连接的数据包来保证连接正确关闭。

通过压缩和充分利用网络的上下行带宽，我们在测试环境中传输文件使得两个Trader上都有全量数据所需的时间是8分钟以内。

> 附：英文版设计文档原稿可见附件file-transfer.zip中的readme文件


## 数据处理（data process）
数据处理是为了解决三个目的：
### **1.重组数据并保证数据的有序性**
已有的数据是分散，且无序的
通过读入分散的H5，重组成Order类型，并按Order_id进行排序。

### **2.减少数据网络和IO传输的代价**
我们采用了两步压缩：

第一步针对已有输入数据是
```cpp=
struct order {
    int stk_code;
    int order_id;
    int direction;
    int type;
    double price;
    int volume;
};
```
其中```stk_code:[1, 10], direction:{-1,1}, type:[0 ,6)```
price两位小数
将其编码转成
```cpp=
struct Order {
    int order_id;
    int price;
    int volume;
    unsigned char combined;
}__attribute__((packed));
```

其中 ```combined = stk_code << 4 | (direction == 1?1:0) << 3 | type```，price乘100四舍五入为整数
数据压缩率 = ```(3int + 1unsigned char) / (4int + 1double)=13/24```

因此在已有的网络传输的优化下，将传输时间提升为

$$21.1 min * 13/24 = 11.47min$$

第二步采用**ZSTD**在C++代码层对数据进行压缩，采取快速压缩方式，数据压缩率在测试数据**上达到58%**。

进一步将传输时间提升为$$11.47min*58\% = 6.66min$$

经过这两步之后，我们需要传输的数据是初始数据的**31.4%**。

### **3.传输过程中数据出错的恢复能力**
为了预防网络传输中出现不可知的错误，我们将数据划分成500份，其中每份股票50份,发生错误恢复代价为$0.2\%$.





## 模拟撮合
考虑到最终两台trader服务器需要获得完全一致的全量答案，我们选择将模拟撮合的逻辑放于trader本地，两台trader分别对全量数据进行撮合。
因此我们不需要花费传输Trade的代价。

### 运行脚本：
```cpp=
g++ merge_cuo.cpp -o merge_cuo -O2
./merge_cuo
```
### 初始化配置
* 处理hook矩阵，存于```map<int, special_info> special_hook[10];```
* 处理prev_price，严格按照深交所规则进行四舍五入

### 开始撮合
* 每次将```chunksize=1e6```的order_info载入内存，对十只股票进行滚动处理
* 将卖方与买方的挂单存于```map < pair<int, int>, int > bids[10], asks[10];```
* 对于不同的挂单类型，实现了不同的定价逻辑
```cpp=
if (type == 0) {
        // limit order
        // is invalid
        if (price > prev_high[stk_code]) return -1;
        if (price < prev_low[stk_code]) return -1;
        return price;
    }
    if (type == 1) {
        // 对手方最优
        return (dir == 1)? get_best_ask(): get_best_bid();
    }
    if (type == 2) {
        // 本方最优
        return (dir == 1)? get_best_bid(): get_best_ask();
    }
    if (type == 3) {
        // 最优五档, 对手方
        quota = inf;
        isLeave = false;
        return (dir == 1)? get_best_ask(): get_best_bid();
    }
    if (type == 4) {
        // 吃光对手方所有价格
        quota = inf;
        isLeave = false;
        return (dir == 1)? get_best_ask(): get_best_bid();
    }
    if (type == 5) {
        // 全额成交, 以对手方为成交价
        quota = inf;
        isLeave = false;
        long long cvl = (dir == 1)? get_cvl(asks[stk_code]): get_cvl(bids[stk_code]);
        if (cvl >= volume) {
            return (dir == 1)? get_best_ask(): get_best_bid();
        } else {
            //cout << "LOG: can not eat all " << stk_code << " " << order_id << " " << endl;
            return -1;
        }
    }
```

### 写入答案文件
* 答案存于```vector<trade> ans[10];```
* 撮合结束以后，一次性```write_trade```（测试结果证明这一做法比边撮边写的耗时直接提升了**数十倍**）

## 预期结果
在三份测试数据集中，通过```cmp```命令来比较二进制答案文件，答案达到了$100\%$正确。



## 我们所做的优化
### 异常恢复
由于Order数据划分成了$500$份binary file，因此对于某一份文件发生错误，重启file-client重新**恢复代价为$0.2\%$**。
此外由于我们采用了ZSTD压缩数据，在接受完数据进行解压时，一旦发生错误，可以**轻易定位错误**的某一份文件。

### 可扩展性
在运维过程中，我们将所有的部署写成了脚本形式，实现了高度自动化运行。
在只有两台Trader的时候，我们的程序可以轻易扩展支持**更多的Exchange**。随着Exchange的数量增加，我们的**传输速度成线性增加**。

### 传输
实现了**全双工传输**。为了解决其中的粘包问题，我们设置了定长消息，服务端每次读取既定长度的内容作为一条完整消息。可以将网线看做一块隔板，隔开了两段stream，但是相对位置不会变。如果让隔板间距固定，所有问题就都解决了。
像上面提到的那样，我们的时间相当于把Trade1传输到Trade2的时间。
另外，在测试环境下，观察到我们的全双工file-transfer可以打满**9MB/s**带宽！

### 内存
撮合过程中，将price全程用int表示，只有在输出答案的时候转换为double类型，同时将$0-9$的stk_code用```unsigned char```表示，可将trade占用大小从从$24$字节压缩到了$17$字节。

### 复盘
1. 网络传输程序里connect没有写retry
2. 比赛的时候启动client太急，trigger了问题1
3. 更换端口的时候有文件破损，更换单个文件没有很好的演练
