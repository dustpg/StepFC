### 南梦宫163

本文github[备份地址](https://github.com/dustpg/BlogFM/issues/47)

不过说到~~男猛攻~~南梦宫, 南梦宫在05年和万代合并了, 发行的游戏感觉上比较偏向于宅系. 为N163分配的Mapper编号是019, 同一个编号的还有N129. 但是存在很多的submapper的情况——这个情况还是到wiki查看吧. 使用了了N163扩展音频的游戏有10个. 例如:

- [数字恶魔物语 女神转生II(女神転生Ⅱ　デジタルデビル物語)](http://bootgod.dyndns.org:7777/profile.php?id=1605) (使用了4个声道)
- [万王之王(King of Kings)](http://bootgod.dyndns.org:7777/profile.php?id=3839) (使用了8个声道)


总览来看, 值得注意的是:

 - CHR window  1Kx8 (PT) + 1Kx4 (NT)
 - 这意味着支持使用ROM代替名称表, 好在上一节统一支持了这一特性
 -  These chips contain 128 bytes of internal RAM that can be used either for expansion audio or, together with a battery, for 128 bytes of save RAM.
 - 可能有些游戏没有8kb的SRAM, 但是用电池支持这内部128字节的RAM
 - 也就是如果没有扩展音频可以用这128字节保存游戏进度(?)

目前的存档信息:

```c
enum {
    // 需要储存进度SRAM-8KiB
    SFC_ROMINFO_SRAM_HasSRAM  = 0x01,
    // 该位为真的话, 储存的不是SRAM-8KiB, 而是扩展区的32KiB
    SFC_ROMINFO_SRAM_More8KiB = 0x02,
    // 该位为真的话, 储存SRAM-8KiB外, 还要储存扩展区的偏移8kiB后128字节
    SFC_ROMINFO_SRAM_M128_Of8 = 0x04,
};
```

所以又更新了保存接口, 暴力读写:

```c
/// <summary>
/// 数据
/// </summary>
typedef struct {
    // 地址
    void*       address;
    // 长度
    uintptr_t   length;
} sfc_data_set_t;


// 保存SRAM
void(*save_sram)(void*, const sfc_rom_info_t*, const sfc_data_set_t*, uint32_t);
// 读取SRAM
void(*load_sram)(void*, const sfc_rom_info_t*, const sfc_data_set_t*, uint32_t);
```

### Banks

 - CPU $6000-$7FFF: 8 KB PRG RAM bank, if WRAM is present
 - CPU $8000-$9FFF: 8 KB switchable PRG ROM bank
 - CPU $A000-$BFFF: 8 KB switchable PRG ROM bank
 - CPU $C000-$DFFF: 8 KB switchable PRG ROM bank
 - CPU $E000-$FFFF: 8 KB PRG ROM bank, fixed to the last bank
 - PPU $0000-$03FF: 1 KB switchable CHR bank
 - PPU $0400-$07FF: 1 KB switchable CHR bank
 - PPU $0800-$0BFF: 1 KB switchable CHR bank
 - PPU $0C00-$0FFF: 1 KB switchable CHR bank
 - PPU $1000-$13FF: 1 KB switchable CHR bank
 - PPU $1400-$17FF: 1 KB switchable CHR bank
 - PPU $1800-$1BFF: 1 KB switchable CHR bank
 - PPU $1C00-$1FFF: 1 KB switchable CHR bank
 - PPU $2000-$23FF: 1 KB switchable CHR bank
 - PPU $2400-$27FF: 1 KB switchable CHR bank
 - PPU $2800-$2BFF: 1 KB switchable CHR bank
 - PPU $2C00-$2FFF: 1 KB switchable CHR bank


### CHR and NT Select ($8000-$DFFF) w

写入数据:

 - ①-$00-$DF: 选择1kb的CHR-ROM
 - ②-$E0-$FF: 根据$E800的情况, 决定是否选择内部VRAM(偶数A面, 奇数B面)
 - ②-如果没有选择则按照①的情况进行


写入地址:

 - $8000-$87FF: BANK-0, ②-[$E800.6 = 0]
 - $8800-$8FFF: BANK-1, ②-[$E800.6 = 0]
 - $9000-$97FF: BANK-2  ②-[$E800.6 = 0]
 - $9800-$9FFF: BANK-3  ②-[$E800.6 = 0]
 - $A000-$A7FF: BANK-4  ②-[$E800.7 = 0]
 - $A800-$AFFF: BANK-5  ②-[$E800.7 = 0]
 - $B000-$B7FF: BANK-6  ②-[$E800.7 = 0]
 - $B800-$BFFF: BANK-7  ②-[$E800.7 = 0]
 - $C000-$C7FF: BANK-8  ②-一直允许
 - $C800-$CFFF: BANK-9  ②-一直允许
 - $D000-$D7FF: BANK-a  ②-一直允许
 - $D800-$DFFF: BANK-b  ②-一直允许


令人吃惊的是允许将内置2kb的VRAM(CIRAM)接入图样表作为CHR-RAM.


### PRG Select 1 ($E000-$E7FF) w

```
7  bit  0
---- ----
.MPP PPPP
 ||| ||||
 |++-++++- Select 8KB page of PRG-ROM at $8000
 +-------- Disable sound if set
```

```64*8=512```, D6是禁止位



### PRG Select 2 / CHR-RAM Enable ($E800-$EFFF) w

```
7  bit  0
---- ----
HLPP PPPP
|||| ||||
||++-++++- Select 8KB page of PRG-ROM at $A000
|+-------- Disable CHR-RAM at $0000-$0FFF
|            0: Pages $E0-$FF use NT RAM as CHR-RAM
|            1: Pages $E0-$FF are the last $20 banks of CHR-ROM
+--------- Disable CHR-RAM at $1000-$1FFF
             0: Pages $E0-$FF use NT RAM as CHR-RAM
             1: Pages $E0-$FF are the last $20 banks of CHR-ROM
```

D6D7位很强


### PRG Select 3 ($F000-$F7FF) w

```
7  bit  0
---- ----
..PP PPPP
  || ||||
  ++-++++- Select 8KB page of PRG-ROM at $C000
```

没什么好说的.


### Write Protect for External RAM AND Chip RAM Address Port ($F800-$FFFF) w

```
7  bit  0
---- ----
KKKK DCBA
|||| ||||
|||| |||+- 1: Write-protect 2kB window of external RAM from $6000-$67FF (0: write enable)
|||| ||+-- 1: Write-protect 2kB window of external RAM from $6800-$6FFF (0: write enable)
|||| |+--- 1: Write-protect 2kB window of external RAM from $7000-$77FF (0: write enable)
|||| +---- 1: Write-protect 2kB window of external RAM from $7800-$7FFF (0: write enable)
++++------ Additionally the upper nybble must be equal to b0100 to enable writes
```

这个写入保护有点烦人, 或许到现在自己还没有实现或许还是一个好主意(为懒人正言).


### IRQ Counter (low) ($5000-$57FF) r/w

```
7  bit  0
---- ----
IIII IIII
|||| ||||
++++-++++- Low 8 bits of IRQ counter
```

### IRQ Counter (high) / IRQ Enable ($5800-$5FFF) r/w

```
7  bit  0
---- ----
EIII IIII
|||| ||||
|+++-++++- High 7 bits of IRQ counter
+--------- IRQ Enable: (0: disabled; 1: enabled)
```

IRQ是一个15bit的计数器, 读取$5000和$5800能够读取计数器的当前值, 可以实时读取. 

IRQ这个计数器在每个CPU周期都会递增, 达到$7fff时就触发IRQ, 并停止计数. 写入这两个寄存器会确认IRQ.

一帧CPU周期大约是3万所以15bit还是足够了的.


### N163音频

N163音频(如果有的话)使用的就是内部的128字节数据.


### Address Port ($F800-$FFFF)

这个地址还有另一个用处: 为之后写入内部RAM的数据指定地址.

```
7  bit  0   (write only)
---- ----
IAAA AAAA
|||| ||||
|+++-++++- Address
+--------- Auto-increment
```

D7:I 为真的话, 写入或者读取'Data Port'地址会递增. 由于是在'Audio'里面的, 所以将储存在N163音频端, 而不是Mapper端.


### Data Port ($4800-$4FFF)

为内部RAM写入或者读取数据, 并且根据$F800:D7是否自动+1.


### Sound RAM $78 - Low Frequency

```
7  bit  0
---------
FFFF FFFF
|||| ||||
++++-++++- Low 8 bits of Frequency
```

### Sound RAM $79 - Low Phase

```
7  bit  0
---------
PPPP PPPP
|||| ||||
++++-++++- Low 8 bits of Phase
```

### Sound RAM $7A - Mid Frequency

```
7  bit  0
---------
FFFF FFFF
|||| ||||
++++-++++- Middle 8 bits of Frequency
```

吃惊, 别的芯片频率设置用11bit, 12bit, 这用16bit还不够.

### Sound RAM $7B - Mid Phase

```
7  bit  0
---------
PPPP PPPP
|||| ||||
++++-++++- Middle 8 bits of Phase
```

### Sound RAM $7C - High Frequency and Wave Length

```
7  bit  0
---------
LLLL LLFF
|||| ||||
|||| ||++- High 2 bits of Frequency
++++-++--- Length of waveform ((64-L)*4 4-bit samples)
```

18bit的频率与6bit的长度. 长度的单位是: 样本, 可以计算为:

```c
length = 256 - (value & 0xFC);
```

### Sound RAM $7D - High Phase

```
7  bit  0
---------
PPPP PPPP
|||| ||||
++++-++++- High 8 bits of Phase
```

24bit的相位数据决定了当前波的相位. 每次声道更新时, 18bit的频率数据就会添加到24bit的相位数据上.

写入这三个相位寄存器会立即重置相位. 对于开发者来说最好先将频率置0, 不然写入3个寄存器之间存在时间差(虽然很短就是了), 可能中途会更新(不禁想起了线程安全).

### Sound RAM $7E - Wave Address

```
7  bit  0
---------
AAAA AAAA
|||| ||||
++++-++++- Address of waveform (in 4-bit samples)
```

### Sound RAM $7F - Volume

```
7  bit  0
---------
.CCC VVVV
 ||| ||||
 ||| ++++- Linear Volume
 +++------ Enabled Channels (1+C)
```

'C'位只有$7F是有效的, 其他的对应地址是无效的.

 - 0: Ch7有效
 - 1: Ch76有效
 - ...
 - 7: Ch76543210都有效


### 其他声道

 - Channel 7:  $78-$7F
 - Channel 6:  $70-$77
 - Channel 5:  $68-$6F
 - Channel 4:  $60-$67
 - Channel 3:  $58-$5F
 - Channel 2:  $50-$57
 - Channel 1:  $48-$4F
 - Channel 0:  $40-$47
 - 可以考虑把编号掉一下头, 这样从零开始

### 波形信息

波形信息以一个样本深度为4bit的规则储存的, 一个字节可以保存2个样本, 小端形式保存:

```c

/// <summary>
/// StepFC: N163 采样
/// </summary>
/// <param name="famicom">The famicom.</param>
/// <param name="addr">The addr.</param>
/// <returns></returns>
static inline int8_t sfc_n163_sample(sfc_famicom_t* famicom, uint8_t addr) {
    const uint8_t data = sfc_n163_internal_chip(famicom)[addr >> 1];
    return (data >> ((addr & 1) << 2)) & 0xf;
}

```

'Sound RAM $7E - Wave Address'储存的样本的地址, 本身内部RAM是128字节. 换句话说: 在不考虑重叠的情况下, 如果使用2个N163声道, 每个声道能用56个字节, 差不多100个样本. 而使用8个N163声道, 就只能独立使用8个字节了(当然这种情况, 只能互相重叠或者重新利用).

'Sound RAM $7C - High Frequency and Wave Length'储存了波表的长度, 单位是样本. 不过由于4个样本对齐, 所以直接除以2就是字节长度.



```
$00:    00 00 00 A8 DC EE FF FF EF DE AC 58 23 11 00 00
$10:    10 21 53 00 00 00 00 00 00 00 00 00 00 00 00 00

Wave Address = $06, 表示从RAM:$03低地址开始

Wave Length = $38(变成高6bit的话就是$E0)
表示长度是32样本


F -       *****
E -     **     **
D -    *         *
C -   *           *
B -
A -  *             *
9 - 
8 - *               *
7 - 
6 -
5 -                  *             *
4 -
3 -                   *           *
2 -                    *         *
1 -                     **     **
0 -                       *****
```


### 更新

与其他扩展音频不同的是, 8个声道不在一起更新, 而是一个一个更新的. 每更新一个声道需要15个CPU周期:


声道数量 | 更新频率
--------|--------
   1  | 119318 Hz
   2  | 59659 Hz
   3  | 39773 Hz
   4  | 29830 Hz
   5  | 23864 Hz
   6  | 19886 Hz
   7  | 17045 Hz
   8  | 14915 Hz

可以看出如果只是用两个的话效果还行(>44100Hz), 使用8个的话反而会差一点, 噪音会有点重.

具体过程, wiki也是直接贴出代码:

```
* w[$80] = the 163's internal memory
* sample(x) = (w[x/2] >> ((x&1)*4)) & $0F
* phase = (w[$7D] << 16) + (w[$7B] << 8) + w[$79]
* freq = ((w[$7C] & $03) << 16) + (w[$7A] << 8) + w[$78]
* length = 256 - (w[$7C] & $FC)
* offset = w[$7E]
* volume = w[$7F] & $0F

phase = (phase + freq) % (length << 16)
output = (sample(((phase >> 16) + offset) & $FF) - 8) * volume
```

 - ```phase```信息会写回去.
 - ```output```会减去8也就是说, 是有符号的.
 - 所以```sfc_n163_sample```函数返回的```int8_t```.

### 频率计算

```
f = wave frequency
l = wave length
c = number of channels
p = 18-bit frequency value
n = CPU clock rate (≈1789773 Hz)

f = (n * p) / (15 * 65536 * l * c)
```

### 合并输出

与VRC7不同的是, N163如果只有一个声道, 这个声道会以近120kHz更新. 有8个, 这个声道就以15kHz更新. 所以理论上一个声道的最大声音, 其实和8个声道一起放的(最大声音)是一样大的.

精确的高频模拟比较困难, wiki提到可以简单将数据加在一起再除以当前声道数量. 不过不同游戏可能音量有差异, 详细的可以查看wiki. 这里列出submapper:

 - 3: N163比2A03方波大了大约11.0-13.0 dB
 - 4: N163比2A03方波大了大约16.0-17.5 dB
 - 5: N163比2A03方波大了大约18.0-19.5 dB
 - 一般来说, 可以简单看作12dB-4倍, 18dB-8倍

但是声道数量在6以上时, 这个时候几乎已经达到了人类可听的频率了. 其中就算放出01方波, 频率会再除以2以至于完全可闻.

这里自己就用'**3声道模式**'. 这里说3声道模式, 并不是指仅仅模拟3声道, 而是每次将最后处理的3个声道(接近40kHz)进行输出, 当然, 这时候采样输出不能低于40kHz(所以作为参数放入N163, 假设支持22kHz就用'6声道模式'). 

 - 声道模式为n
 - 总倍数为N
 - 单声道输出8bit有符号, 大约是方波的16倍
 - 输出```Sum(hist, n) / n / 16 * N```
 - 当然, 继续使用之前的context+per_sample模式

```c
// N163
if (g_famicom->rom_info.extra_sound & SFC_NSF_EX_N163) {
    // 3声道模式
    const float out = sfc_n163_per_sample(g_famicom, &n163_ctx, cpu_cycle_per_sample, 3);
    output += out * (0.00752f / 16.f) * n163_ctx.subweight;
}
```

### N163

N163看上去有8个声道简直逆天, 但是实际上拥有很大的限制:

 - 公用一个波形表地址空间
 - 如果声道少的话, 就可以用很长的波表了
 - 相反, 声道多就只能用短一点了
 - 每15CPU周期仅仅更新一个声道, 如果声道多了杂音会很重.
 - 上面这一点, 我们可以考虑进行声部强化
 - 总之, N163的特点就是用的声道越多, 限制就越大
 - 不过目前的游戏: 8个使用了4声道, 2个使用了8声道

### 编写中遇到的问题

N163的编写感觉一帆风顺, 除了一两个丢人的小问题, 比如Mapper编号19的16进制不小心弄成'15'.



### 模拟女神转生2出现的问题

都说了'一帆风顺'. 不过一开始....

![01](./01.gif)


一开始就花屏了, 由于只打开了N163声道, 还以为是IRQ触发有问题, 后来发现本来就是这样.


**精度问题**

进入游戏后, 人物上下移动瞬间, 背景错位(仅仅一帧). wiki提到修改是下一帧起效, 所以就把

```c
void sfc_ppu_do_end_of_vblank(sfc_ppu_t* ppu) {
    // v: .....F.. ...EDCBA = t: .....F.. ...EDCBA
    ppu->data.v = (ppu->data.v & (uint16_t)0x841F) | (ppu->data.t & (uint16_t)0x7BE0);
}
```

这段调用放在最后. (具体是精度还是本来就是这样, 等待验证)

### REF

 - [INES Mapper 019](https://wiki.nesdev.com/w/index.php/INES_Mapper_019)
 - [Namco 163 audio](https://wiki.nesdev.com/w/index.php/Namco_163_audio)



### 附录: FamiTracker对N163的支持

 FamiTracker可以粗暴地认为是NSF的创作软件.

 > FamiTracker allows waveforms between 4 and 32 samples long (in increments of four), though recent discoveries have shown the actual chip is capable of reading the entire channel/waveform memory space, for a total of 256 samples per wave.

应该是实现难度的问题, N163本身可以支持到256个样本(当然就包含声道7的寄存器数据了), 但是FT只支持4~32个样本. 手动的话, 自由度很大, 不仅能使用256个, 通道之间还能交叉使用部分.

```
*     *     ****
  * *  *****    
   *            *

[  CH0 ]
    [    CH1    ]
        [  CH2  ]
```