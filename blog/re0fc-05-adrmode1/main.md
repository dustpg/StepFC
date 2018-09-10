### STEP3: CPU 指令实现 - 寻址模式

终于进入了所谓的"本番", 通过阅读文档, 可知CPU拥有下列的寄存器:

 - [8位]累加寄存器 Accumulator 
 - [8位]X 变址寄存器 (X Index Register)
 - [8位]Y 变址寄存器 (Y Index Register)
 - [8位]状态寄存器 (Status Register)
 - [8位]栈指针 (Stack Pointer)
 - [16位]指令计数器 (Program Counter)
 - 其中栈是指"一页"(\$100-\$1FF)
 - 栈指针[初始化](http://wiki.nesdev.com/w/index.php/CPU_power_up_state)为\$FD即指向\$1FD
 - 不过由于内存太宝贵了, 把八分之一的内存作为栈实在太浪费. 程序猿一般会会把栈地址开始的一段作为其他用途, [比如这里](http://wiki.nesdev.com/w/index.php/Sample_RAM_map)

大致写为:

```c
 // 状态寄存器标记
enum sfc_status_flag {
    SFC_FLAG_C = 1 << 0,    // 进位标记(Carry flag)
    SFC_FLAG_Z = 1 << 1,    // 零标记 (Zero flag)
    SFC_FLAG_I = 1 << 2,    // 禁止中断(Irq disabled flag)
    SFC_FLAG_D = 1 << 3,    // 十进制模式(Decimal mode flag)
    SFC_FLAG_B = 1 << 4,    // 软件中断(BRK flag)
    SFC_FLAG_R = 1 << 5,    // 保留标记(Reserved) 一直为1
    SFC_FLAG_V = 1 << 6,    // 溢出标记(Overflow  flag)
    SFC_FLAG_S = 1 << 7,    // 信号标记(Sign flag)
    SFC_FLAG_N = SFC_FLAG_S,// 又叫(Negative Flag)
};
// CPU寄存器
typedef struct {
    // 指令计数器 Program Counter
    uint16_t    program_counter;
    // 状态寄存器 Status Register
    uint8_t     status;
    // 累加寄存器 Accumulator
    uint8_t     accumulator;
    // X 索引寄存器 X Index Register
    uint8_t     x_index;
    // Y 索引寄存器 Y Index Register
    uint8_t     y_index;
    // 栈指针  Stack Pointer
    uint8_t     stack_pointer;
    // 保留对齐用
    uint8_t     unused;
} sfc_cpu_register_t;
```


其中, 6502实际上只有6个FLAG可用, 其中一个就是FLAG_B, D4.
[CPU status flag behavior](https://wiki.nesdev.com/w/index.php/CPU_status_flag_behavior)

指令|D5 以及 D4|PUSH后影响
----|------------|---------
PHP | 11 | 没有
BRK | 11 | IF 设为1
IRQ | 10 | IF 设为1
NMI | 10 | IF 设为1

换句话说FLAG_B唯一有效果的情况就是: PHP-BRK和IRQ-NMI行为不一样.

有两条指令(PLP 及 RTI) 会从栈上设置P状态, 这时候会无视D4 D5.

其中**状态寄存器**可以考虑实现为uint8_t[8].

### CPU定制信息
> NES的6502并不包括对decimal模式的支持. CLD和SED指令正常工作, 但是p中的 'd' bit在ADC和SBC中并未被使用. 在游戏中将CLD先执行于代码是普遍的行为，就像启动和RESET时的 'd' 状态并不为人知一样.
>音频寄存器被放置于CPU内部; 所有波形的发生也都在CPU的内部.

简单地说就是原版6502支持10进制模式, 定制版去掉了10进制支持(, 并增加了音频支持, 暂且不谈). 状态寄存器中有一个标志位'D'用来标记是不是10进制, 清除(CLD, clear 'd')与设置(SED, set 'd')指令是正常工作的(其中十进制模式不必实现, 实现了也没有游戏给你跑).

### 寻址模式
寻址, 顾名思义**寻找地址**.
> 寻址方式就是处理器根据指令中给出的地址信息来寻找有效地址的方式，是确定本条指令的数据地址以及下一条要执行的指令地址的方法

获取操作码 伪C代码:
```c
opcode = READ(pc);
pc++;
```

0. **累加器寻址** Accumulator
    操作对象是累加器, 个人认为可以被划分至下面一条**隐含寻址**, 只是语法稍微不同

    ```
    $0A
    ASL A - (累加器A内容按位算术左移1位)
    ```

    **指令伪C代码**
        
    ```c
    // 空
    ```

1. **隐含寻址** Implied Addressing
    单字节指令, 指令已经隐含了操作地址
    ```
    $AA
    TAX - (将累加器中的值传给 X 寄存器, 即 X = A)
    ```
    **指令伪C代码**
    ```c
    // 空
    ```

2. **立即寻址** Immediate Addressing
    双字节指令, 指令的操作数部分给出的不是 操作数地址而是操作数本身,我们称为立即数(00-FF之间的任意数)
    在6502汇编中，这种模式以操作数(即**立即数**)前加 "#" 来标记.
    ```
    $A9 $0A
    LDA #$0A - (将内存值$0A载入累加器, 即 A = 0x0A)
    ```
    **指令伪C代码**
    ```c
    // 指令计数器的地址即为当前需要读取的地址
    address = pc;
    // 双字节指令(获取操作码已经+1)
    pc++;
    ```

3. **绝对寻址** Absolute Addressing 又称**直接寻址**
    三字节指令, 指令的操作数给出的是操作数, 在存储器中的有效地址
    ```
    $AD $F6 $31
    LDA $31F6 - (将地址为$31F6的值载入累加器, 即 A = [$31F6])
    ```
    **指令伪C代码**
    ```c
    // PC指向的两个字节解释为地址
    address = READ(pc++);
    address |= READ(pc++) << 8;
    ```

4. **零页寻址** 全称**绝对零页寻址** Zero-page Absolute Addressing
    双字节指令, 将地址\$00-\$FF称之为**零页**, 绝对寻址中如果高字节为0, 即可变为零页寻址, 直接能够节约一个字节, 速度较快, 所以经常使用的数据可以放在零页.
    ```
    $A5 $F4
    LDA $F4 - (将地址为$00F4的值载入累加器, 即 A = *0x00F4)
    ```
    **指令伪C代码**
    ```c
    // PC指向的一个字节解释为地址
    address = READ(pc++);
    ```

5. **绝对X变址** Absolute X Indexed Addressing
    三字节指令, 这种寻址方式是将一个16位的直接地址作为基地址, 然后和变址寄存器X的内容相加, 结果就是真正的有效地址
    ```
    $DD $F6 $31
    LDA $31F6, X - (将值$31F6加上X作为地址的值载入累加器, 即 A = 0x31F6[X])
    ```
    **指令伪C代码**
    ```c
    // PC指向的两个字节解释为基地址
    address = READ(pc++);
    address |= READ(pc++) << 8;
    // 加上X变址寄存器
    address += X;
    ```

6. **绝对Y变址** Absolute Y Indexed Addressing
    三字节指令, 同5, 就是把X换成Y而已

    **指令伪C代码**
    ```c
    // PC指向的两个字节解释为基地址
    address = READ(pc++);
    address |= READ(pc++) << 8;
    // 加上Y变址寄存器
    address += Y;
    ```

7. **零页X变址** Zero-page X Indexed Addressing
    双字节指令, 同5, 如果高地址是0, 可以节约一个字节.

    **指令伪C代码**
    ```c
    // PC指向的一个字节解释为零页基地址
    address = READ(pc++);
    // 加上X变址寄存器
    address += X;
    // 结果在零页
    address = address & 0xFF;
    ```

8. **零页Y变址** Zero-page Y Indexed Addressing
    双字节指令, 同7, 就是把X换成Y而已

    **指令伪C代码**
    ```c
    // PC指向的一个字节解释为零页基地址
    address = READ(pc++);
    // 加上Y变址寄存器
    address += Y;
    // 结果在零页
    address = address & 0xFF;
    ```

9. **间接寻址** Indirect Addressing
    三字节指令, 在 6502中,仅仅用于无条件跳转指令```JMP```这条指令该寻址方式中, 操作数给出的是间接地址, 间接地址是指存放操作数有效地址的地址
    ```
    $6C $5F $21
    JMP ($215F)  - 跳转至$215F地址开始两字节指向的地址
    ```
    有点拗口, 假如:

    地址|值
    -----|---
    $215F|$76
    $2160|$30
    这个指令将获取 \$215F, \$2160 两个字节中的值，然后把它当作转到的地址 - 也就是跳转至\$3076

    **已知硬件BUG/缺陷**

    这唯一一个用在一条指令的寻址方式有一个已知的BUG/缺陷: ```JMP ($xxFF)```无法正常工作. 

    例如```JMP ($10FF)```, 理论上讲是读取\$10FF和\$1100这两个字节的数据, 但是实际上是读取的\$10FF和\$1000这两个字节的数据. 虽然很遗憾但是我们必须刻意实现这个BUG, 这其实算是实现FC模拟器中相当有趣的一环.

    **指令伪C代码**
    ```c
    // PC指向的两个字节解释为间接地址
    tmp1 = READ(pc++);
    tmp1 |= READ(pc++) << 8;
    // 刻意实现6502的BUG
    tmp2 = (tmp1 & 0xFF00) | ((tmp1+1) & 0x00FF)
    // 读取间接地址
    address = READ(tmp1) | (READ(tmp2) << 8);
    ```

10. **间接X变址**(先变址X后间接寻址): Pre-indexed Indirect Addressing
    双字节指令, 比较麻烦的寻址方式
    ```
    $A1 $3E
    LDA ($3E, X)
    ```
    这种寻址方式是先以X作为变址寄存器和零页基地址IND(这里就是\$3E)相加```IND+X```, 不过这个变址计算得到的只是一个间接地址,还必须经过两次间接寻址才得到有效地址:
    - 第一次对 IND + X 间址得到有效地址低 8 位
    - 第二次对 IND + X + 1 间址得到有效地址高 8 位
    - 然后把两次的结果合起来,就得到有效地址.
    
    例如:

    地址|值
    -----|---
    X | $05
    $0043|$15
    $0044|$24
    $2415|$6E

    这条指令将被如下执行:
    - \$3E + \$05 = \$0043
    - 获取 \$0043, \$0044 两字节中保存的地址 = $2415
    - 读取 \$2415 中的内容 - \$6E
    - 执行LDA, 把值\$6E载入累加器

    **指令伪C代码**
    ```c
    // PC指向的一个字节解释为零页基地址再加上X寄存器
    tmp = READ(pc++) + X;
    // 读取该零页地址指向的两个字节
    address = READ(tmp) | (READ(tmp + 1) << 8);
    ```

11. **间接Y变址**(后变址Y间接寻址): Post-indexed Indirect Addressing
    双字节指令, 比较麻烦的寻址方式
    ```
    $B1 $4C
    LDA ($4C), Y
    ```
    - 这种寻址方式是对IND(这里就是\$4C)部分所指出的零页地址先做一次间接寻址, 得到一个低8位地址
    - 再对IND + 1 作一次间接寻址,得到一个高8位地址
    - 最后把这高,低两部分地址合起来作为16的基地址,和寄存器Y进行变址计算, 得到操作数的有效地址,注意的是这里IND是零页地址

    例如:

    地址|值
    -----|---
    $004C|$00
    $004D|$21
    Y|$05
    $2105|$6D

    这条指令将被如下执行:
    - 读取字节 \$4C, \$4D 中的内容 = \$2100
    - Y 寄存器中的内容相加 = \$2105
    - 读取字节 $2105 中的内容 - \$6D
    - 执行LDA, 把值\$6D载入累加器

    **指令伪C代码**
    ```c
    // PC指向的一个字节解释为零页地址
    tmp = READ(pc++);
    // 读取该零页地址指向的两个字节作为基地址
    address = READ(tmp) | (READ(tmp + 1) << 8);
    // 基地址再加上Y寄存器
    addres += Y;
    ```

12. **相对寻址**: Relative Addressing
    该寻址仅用于条件转移指令, 指令长度为2个字节.
    第1字节为操作码,第2字节为条件转移指令的跳转步长, 又叫偏移量D. 偏移量可正可负, D若为负用补码表示.
    ```
    $F0 $A7
    BEQ $A7 - (如果标志位中'Z'-被设置, 则向后跳转-39字节, 即前跳39字节)
    ```
    **指令伪C代码**
    ```c
    // 读取操作数
    tmp = READ(pc++);
    // PC加上有符号数据即可
    address = pc + (int8_t)tmp;
    ```
    注意, 这里并没有进行条件判断

### REF
 - [nesdev-archive](http://nesdev.com/archive.html)
 - [NES 文档2.00](http://nesdev.com/nestech_cn.txt)
 - [6502 微处理器](http://nesdev.com/6502_cn.txt)
