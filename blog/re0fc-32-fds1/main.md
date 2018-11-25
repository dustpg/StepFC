### FC磁碟机

本篇博客github[备份地址](https://github.com/dustpg/BlogFM/issues/45), 以及[项目地址](https://github.com/dustpg/StepFC)

Family Computer Disk System(FDS)是一种基于FC的磁碟机系统, 其中部分探讨会在下一步《全部成为F》简述. 

这里仅仅是完成扩展音频部分, 自己并没有完全模拟FDS的计划. 毕竟从未见过, 甚至都是接触过模拟器后才发现才有这玩意. ~~没有感情, 对, 我是一台没有感情的程序猿.~~

可以看出FDS与其他不同的地方是: 别的扩展音频是卡带上的硬件, FDS音频是磁碟机上的. 所以FDS的游戏自然是: 不用白不用, 很多FDS的游戏都用上了.

### FDS

iNES为FDS分配的Mapper编号是20, 不过仅仅作为保留用, 让模拟器知道自己在模拟FDS而已.

FDS扩展音频只有一个声道. 简单地说, 就是将一个长度64的自定义波形信息, 安装预定的方式进行调制. 所以别说模拟乐器了, 完全可以用来模拟人声. 就是精度差点(6bit).


### Master I/O enable ($4023)

```
7  bit  0
---------
xxxx xxSD
       ||
       |+- Enable disk I/O registers
       +-- Enable sound I/O registers
```

D1位写入1才能让音频相关寄存器启动(懒得去实现).


### Wavetable RAM ($4040-$407F)

```
7  bit  0  (read/write)
---- ----
OOSS SSSS
|||| ||||
||++-++++- Sample
++-------- Returns 01 on read, likely from open bus
```

这里保存了64步长的波形数据, 这部分可读, 不过声音播放中不可写.  读取时高两位是01, 所以实现上可以写入```01SS SSSS```.

### Volume envelope ($4080)

```
7  bit  0  (write; read through $4090)
---- ----
MDVV VVVV
|||| ||||
||++-++++- (M=0) Volume envelope speed
||         (M=1) Volume gain and envelope speed.
|+-------- Volume change direction (0: decrease; 1: increase)
+--------- Volume envelope mode (0: on; 1: off)
```

 - M=0: 音量包络速度
 - M=1: 音量增益**和**包络速度

超过32是有效的, 但是输出前会被钳制到32(根据全文, 超过32的数据仅仅**可能**用于数据读取).

### Frequency low ($4082)

```
7  bit  0  (write)
---- ----
FFFF FFFF
|||| ||||
++++-++++- Bits 0-7 of frequency
```

### Frequency high ($4083)

```
7  bit  0  (write)
---- ----
MExx FFFF
||   ||||
||   ++++- Bits 8-11 of frequency
|+-------- Disable volume and sweep envelopes (but not modulation)
+--------- Halt waveform and reset phase to 0, disable envelopes
```

暂停波形的话, 会一直输出```$4040```的值, 也就是可以认为: 重置相位, 然后播发周期无穷大-频率为0Hz.

D6位仅仅会暂停包络而不是波形, 不过会重置这两个的计时器.

### Mod envelope ($4084)

调制器(modulator), 或者调制(modulation)

```
7  bit  0  (write; read through $4092)
---- ----
MDSS SSSS
|||| ||||
||++-++++- (M=0) Mod envelope speed
||         (M=1) Mod gain and envelope speed.
|+-------- Mod envelope direction (0: decrease; 1: increase)
+--------- Mod envelope mode (0: on; 1: off)
```

 - M=0: 调制包络速度
 - M=1: 调制增益**和**包络速度

### Mod counter ($4085)

```
7  bit  0  (write)
---- ----
xBBB BBBB
 ||| ||||
 +++-++++- Mod counter (7-bit signed; minimum $40; maximum $3F)
```

这是一个7bit有符号的数据,


### Mod frequency low ($4086)

```
7  bit  0  (write)
---- ----
FFFF FFFF
|||| ||||
++++-++++- Bits 0-7 of modulation unit frequency
```

### Mod frequency high ($4087)

```
7  bit  0  (write)
---- ----
Dxxx FFFF
|    ||||
|    ++++- Bits 8-11 of modulation frequency
+--------- Disable modulation
```

最高的禁用位能够禁用调制, 同样如果12bit的频率为0也能禁用调制.

通过禁用位暂停调制后才能够写入调制表($4088)

### Mod table write ($4088)

```
7  bit  0  (write)
---- ----
xxxx xMMM
      |||
      +++- Modulation input
```

必须通过禁用$4087的相关禁用位才能正常写入, 否则无效.

调制表是一个64长的环形缓冲区, 每次会写入表中相邻的两位, 也就是说连续写入32次就能完整地写入一次(当然每次写入会推进位置).





### Wave write / master volume ($4089)

```
7  bit  0  (write)
---- ----
Wxxx xxVV
|      ||
|      ++- Master volume (0: full; 1: 2/3; 2: 2/4; 3: 2/5)
|          Output volume = current volume (see $4080 above) * master volume
+--------- Wavetable write enable
           (0: write protect RAM; 1: write enable RAM and hold channel)
```

1, 3, 4, 5最小公倍数为 30.

D7为1时, 波形会保持当前的输出, 直到D7=0(自己的实现是在推进波索引时检查是否输出).

### Envelope speed ($408A)

```
7  bit  0  (write)
---- ----
SSSS SSSS
|||| ||||
++++-++++- Sets speed of volume envelope and sweep envelope
           (0: disable them)
```

为音量/调制包络设置时钟倍频, 很少会使用(不过不要小看NSF). BIOS将其初始化到```$E8```.


### Volume gain ($4090)

```
7  bit  0  (read; write through $4080)
---- ----
OOVV VVVV
|||| ||||
||++-++++- Current volume gain level
++-------- Returns 01 on read, likely from open bus
```

### Mod gain ($4092)

```
7  bit  0  (read; write through $4084)
---- ----
OOVV VVVV
|||| ||||
||++-++++- Current mod gain level
++-------- Returns 01 on read, likely from open bus
```

### 频率计算

**包络**, 在```n```个CPU周期后, 包络单元会tick一次:

```
n = CPU clocks per tick
e = envelope speed ($4080/4084)
m = master envelope speed ($408A)

n =  8 * (e + 1) * m
```

由于FDS只在日本发售, 自然是N制式. ```3+6+8=17```, 看来必须用32bit整数保存.

一般地, 写入相关寄存器重置计时器, 要到下一次Tick才能正常重置(貌似没有实现).


**波形表**, 波输出和调制器内部拥有一个12bit的频率值. 还有一个16bit的累加器, 通过每次CPU时钟累加频率值, 超过16bit范围时推进一次位置.

```
f = frequency of tick
n = CPU clock rate (≈1789773 Hz)
p = current pitch value ($4082/$4083 or $4086/$4087) plus modulation if wave output

f = n * p / 65536

f*: 对于波形表的频率需要再除以64
```

### TICK

**包络单元**.  启用时, 会被自身计时器tick, 根据```$4080/$4084:D6```位:

 - 增: 如果增益小于32就+1
 - 减: 如果增益大于零就-1
 - 增益可以手动设置超过32

**调制单元**, 当调制单元被tick时, 会被根据调制计数器当前的调制表前进指定的次数:

```
0 = %000 -->  0
1 = %001 --> +1
2 = %010 --> +2
3 = %011 --> +4
4 = %100 --> reset to 0
5 = %101 --> -4
6 = %110 --> -2
7 = %111 --> -1
```

调制计数器($4085)是一个7bit有符号的数据, 于是就有```-64 - 1 = 63```之类的操作. 实际实现中我们可以利用8bit有符号```int8_t```实现: ```-128 - 2 = 126```.


调制计数器实际使用中还是比较麻烦的, wiki都直接给出代码:

```c
// pitch   = $4082/4083 (12-bit unsigned pitch value)
// counter = $4085 (7-bit signed mod counter)
// gain    = $4084 (6-bit unsigned mod gain)

// 1. multiply counter by gain, lose lowest 4 bits of result but "round" in a strange way
temp = counter * gain;
remainder = temp & 0xF;
temp >>= 4;
if ((remainder > 0) && ((temp & 0x80) == 0))
{
    if (counter < 0) temp -= 1;
    else temp += 2;
}

// 2. wrap if a certain range is exceeded
if (temp >= 192) temp -= 256;
else if (temp < -64) temp += 256;

// 3. multiply result by pitch, then round to nearest while dropping 6 bits
temp = pitch * temp;
remainder = temp & 0x3F;
temp >>= 6;
if (remainder >= 32) temp += 1;

// final mod result is in temp
wave_pitch = pitch + temp;
```

最终值上限可能超过12bit, 低于0的话会(presumably)被钳制到0.

### 大致过程

包络:

1. 每经过```8 * (e + 1) * m```CPU周期会Tick一次音量/调制包络
2. 目的是为了调制音量/调制增益
3. 注意相关禁用位```$4080 $4084 :D7```才能进行增益处理
4. 这里写出音量包络的处理, 调制包络也是非常相似的

```c
/// <summary>
/// StepFC: FDS Tick一次音量包络
/// </summary>
/// <param name="famicom">The famicom.</param>
void sfc_fds_tick_volenv(sfc_famicom_t* famicom) {
    sfc_fds1_data_t* const fds = &famicom->apu.fds;
    assert(fds->flags_4083 == 0);
    assert((fds->modenv_4084 & SFC_FDS_4084_GainMode) == 0);
    // 增
    if (fds->modenv_4084 & SFC_FDS_4084_Increase) {
        if (fds->modenv_gain < 32) {
            fds->modenv_gain++;
            sfc_fds_update_modenv_gain(famicom);
        }
    }
    // 减
    else {
        if (fds->modenv_gain) {
            fds->modenv_gain--;
            sfc_fds_update_modenv_gain(famicom);
        }
    }
}
```

波输出与调制:

1. 每次个CPU周期增加一个12bit的频率值, 增加到16bit就Tick一次波输出与调制.
2. *波输出* 每次个CPU周期还会额外增加一个增益数据
3. *波输出*被tick时, 输出当前数据, 然后往前推进一次索引
4. *调制单元*被tick时, 根据当前的调制表的信息增减调制计数器
5. 然后根据前面wiki贴出的代码计算出为*波输出*提供的增益值
6. 这里贴出*调制单元*Tick时的代码, *波输出*还稍微简单点

```c

/// <summary>
/// StepFC: FDS Tick一次调制单元
/// </summary>
/// <param name="famicom">The famicom.</param>
void sfc_fds_tick_modunit(sfc_famicom_t* famicom) {
    assert(famicom->apu.fds.mod_enabled);
    const uint8_t* const table = sfc_get_fds1modtbl(famicom);
    const int8_t value = table[famicom->apu.fds.modtbl_index++];
    sfc_fds1_data_t* const fds = &famicom->apu.fds;
    fds->modtbl_index &= 0x3f;
    fds->mod_counter_x2 += value;

    fds->freq_gained = sfc_fds_get_mod_pitch_gain(
        fds->freq,
        fds->mod_counter_x2 / 2,
        fds->modenv_gain
    );
}
```


### 合并输出

主要影响值: 波输出数据, 音量增益, 以及主音量.

 - 波输出数据(6bit波数据)
 - 音量增益(0-32)
 - 主音量(2/2, 2/3, 2/4, 2/5)
 - 最大音量大约是2A03方波的2.4倍
 - 输出6bit才2.4倍, 可以算出权重大致是```0.0045```(并不确定, 交给用户控制)
 - 最后通过一个滤波, 可近视为截止频率2kHz的低通滤波
 - 输出被视为线性的
 - 这里由于滤波器的存在, 可以暂时不用考虑重采样的问题


### 修改点

可以看出可以读取$4040-$4092(大致), 这部分刚好在前面定义的自定义BUS区. 上次刚好将VRC7数据转进来, 这里只好挪一下, 目前BUS布局:

 - (自定义BUS)$4000-$403F: 储存调制表
 - (自定义BUS)$4040-$407F: 储存波形数据(FDS可读)
 - (自定义BUS)$4080-$4082: FDS读取用(FDS可读)
 - (自定义BUS)$4100-$4105: NSF程序PALY用
 - (自定义BUS)$4106-$410C: NSF程序INIT用(以后谈)
 - (自定义BUS)$4180-$41FF: VRC7 PATCH表

```c
static inline void sfc_fds_update_volenv_gain(sfc_famicom_t* famicom) {
    famicom->bus_memory[0x90] = famicom->apu.fds.volenv_gain | 0x40;
}
static inline void sfc_fds_update_modenv_gain(sfc_famicom_t* famicom) {
    famicom->bus_memory[0x92] = famicom->apu.fds.modenv_gain | 0x40;
}
```

### 编写FDS出现的问题

**新的思路**: 这一次尝试用新的思路处理```audio_changed```事件. 会在音频时间修改前处理(这个事件名称之后应该会继续修改), 然后用最小的临时'context'(上下文)数据而不是保存整个状态. 这个新思路可能会让全部的音频事件模仿.

**禁止位**: 有一点特别注意, 几乎所有相关bit位是禁止位, 而不是使能位. 实际编写中自己几乎全部弄反了.

**调制增益**: 调制的目的就是动态修改波输出频率, 不过可能会发生溢出的现象:

-  ``` freq + gain < 0```
- 自己使用的是```uint16_t```这就让频率变得异常高
- 根据wiki的说法应该(可能会)钳制到0
- 也可以考虑使用```int16_t```


**效果对比**: 测试用ROM, 不对, 测试用NSF文件是一位叫做w7n的~~作者~~(四斋)cover的《[初音ミクの消失](http://forums.famitracker.com/viewtopic.php?t=401)》, 这是来自famitracker论坛. 标题提到是66Hz, 不过实际上60Hz还是可以听一下.

 - 这个是NSF. 之所以用这个文件, 是因为音频是2A03+FDS的, 没有其他扩展音频支持. 直接将NSF嫁接到FDS接口就行.
 - 仅仅将FDS声道打开就能听到FDS模拟的乐器, 以及最重要——模拟的ミク的声线
 - 与自己手上的VirtualNES作比较, 会发现声音大致正确但是**杂音有点重**
 - 其实说是杂音应该是之前提到的'跳跃点':

```
            **  
              *
               *       **
************    *     *  *
                 **  *
           ^       **
         跳跃点
```

这首曲子中, 几乎每次说完一个字就会出现. 这个东西通过低通滤波能够减弱, 但是还不能完全消除, 可能性:

 - 实现有问题
 - VirtuaNES的滤波器实现很不错
 - 66Hz/64Hz(NSF文件里写着是这个频率)
 - 频率的问题最好验证, 但是*似乎*没有什么效果
 - wiki整篇除了$4083会暂停波外似乎没有有效的处理, 但是又不像三角波暂停后继续, 这里是暂停是为了写入新的数据.
 - 这个问题先放着, 毕竟没有接触过FDS

### REF

 - [FDS audio](https://wiki.nesdev.com/w/index.php/FDS_audio)
 - [FDS, 66Hz] Hatsune Miku no Shoushitsu (Project Famicaloid)


### 附录: 初音ミクの消失

《初音ミクの消失》是由[cosMo@暴走P Official Channel](https://www.youtube.com/user/cosmobsp/)创作的一首知名曲目.

![title](title.png)

[https://www.youtube.com/watch?v=sMrY0KSPtuM](https://www.youtube.com/watch?v=sMrY0KSPtuM)

这首NSF曲子可以在[https://www.bilibili.com/video/av3908758/?p=5](https://www.bilibili.com/video/av3908758/?p=5 )在线听到. 


作为对比, 曲子长度5分钟左右, 但是NSF文件不到300kb, 对比44.1kHz-8bit的wav文件大概需要13mb, 但是包含的信息感觉上却比wav还多. 