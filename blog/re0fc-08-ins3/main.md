### STEP3: CPU 指令实现 - 扩展指令

这节是指令介绍的最后一节.

扩展指令, 或者说非法/未被记录的指令: 一般是组合指令. 由于非官方记录, 所以指令名称可能不一致.

本节很多指令并没有在STEP3中实现, 原因是因为使用的测试ROM并没有测试这些指令, 所以'忘记'实现了, 这些指令直到测试了'blargg's CPU test rom v5'才实现.

### NOP - No OP
除了基本的NOP, 还有高级的NOP. 一般来讲6502指令是根据寻址方式排序的(偏移), 所以在其他寻址方式对应NOP的地方还有

### ALR[ASR] - 'And' then Logical Shift Right - AND+LSR
助记符号: A &= M; C = A & 1; A >>= 1;

| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
| 立即 | ASR #Oper | 4B | 2 | 2 |

只有两个指令周期(NOP你看看你), 应该是都消耗在了读取数据上

AND+LSR 的组合指令 - 逻辑与运算再右移一位:
影响FLAG: C(arry), S(ign), Z(ero), 伪C代码:
```c
A &= READ(address);
CHEKC_CFLAG(A & 1);
A >>= 1;
CHECK_ZSFLAG(A);
```

例子: ALR #\$FE 基本等价于 LSR A + CLC. 前者2字节2周期, 后者2字节4周期

### ANC[AAC] - 'And' then copy N(S) to C
助记符号: A &= M; C = N(S);

| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
| 立即 | ANC #Oper | 0B | 2 | 2 |


N(egative)或者说S(ign)位的值复制到C(arry)位, 
影响FLAG: C(arry), S(ign), Z(ero), 伪C代码:
```c
A &= READ(address);
CHECK_ZSFLAG(A);
CF = SF; 
```

例子: ANC #\$FF 有符号扩展; ANC #\$00 基本等价于 LDA #$00 + CLC.

### ARR - 'AND' then Rotate Right - AND+ROR [类似]
助记符号: A &= M; A = (A>>1)|(C<<7); C = (A>>6)&1; V = ((A>>5)^(A>>6)&1);

| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
| 立即 | ARR #Oper | 6B | 2 | 2 |

基本等价于 AND+ROR, 除了FALG设置.
> C is bit 6 and, V is bit 6 xor bit 5.

影响FLAG: C(arry), S(ign), Z(ero), (o)V(erflow), 伪C代码:
```c
A &= READ(address);
A = (A >> 1) | (CF << 7);
CHECK_ZSFLAG(A);
CHECK_CFLAG(A>>6);
CHECK_VFLAG(((A>>5)^(A>>6))&1);
```


### AXS[SBX] - A 'And' X, then Subtract memory, to X
助记符号: X = (A&X) - M;

| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
| 立即 | AXS #Oper | CB | 2 | 2 |

影响FLAG: C(arry), S(ign), Z(ero), 伪C代码:
```c
uint16_t tmp = (SFC_A & SFC_X) - SFC_READ_PC(address);
X = (uint8_t)tmp;
CHECK_ZSFLAG(X);
SET_CF((tmp & 0x8000) == 0);
```

### LAX - Load 'A' then Transfer X - LDA  + TAX
助记符号: X = A = M;

| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
|  立即    | ADC #Oper      |  69  |    2    |    2     |
|  零页    | ADC Oper       |  65  |    2    |    3     |
|  零页,X  | ADC Oper,X     |  75  |    2    |    4     |
|  绝对    | ADC Oper       |  60  |    3    |    4     |
|  绝对,X  | ADC Oper,X     |  70  |    3    |    4*    |
|  绝对,Y  | ADC Oper,Y     |  79  |    3    |    4*    |
| (间接,X) | LAX (Oper,X)   | A3  |    2    |    6     |
| (间接),Y | ADC (Oper),Y   |  71  |    2    |    5*    |

将储存器数据载入A, 然后传给X.
影响FLAG:  S(ign), Z(ero), 伪C代码:
```c
X = A = READ(address);
CHECK_ZSFLAG(X);
```

### SAX - Store A 'And' X
助记符号: M = A & X;

| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 

将累加器A和变址寄存器X '与' 的结果保留在储存器上. 影响FLAG: (无), 伪C代码:
```c
WRITE(address, A & X);
```

### DCP - Decrement memory then Compare with A - DEC + CMP
助记符号: M -= 1; A - M ? 0

| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 

读改写(RMW)指令, 存储器值-1再与累加器比较.
影响FLAG: C(arry), Z(ero), S(ign). 伪C代码:
```c
tmp = READ(address);
--tmp;
WRITE(address, tmp);

uint16_t result16 = (uint16_t)A - (uint16_t)tmp;
CF = result16 < 0x100;
CHECK_ZSFLAG((uint8_t)result16);
```

### ISC(ISB) - Increment memory then Subtract with Carry - INC + SBC
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 

读改写(RMW)指令, 存储器值+1再用累加器减.
影响FLAG: C(arry), (o)V(erflow), Z(ero), S(ign). 伪C代码:
```c
// INC
tmp = READ(address);
++tmp;
WRITE(address, tmp);

// SBC
uint16_t result16 = A - tmp - (CF ? 0 : 1);
CHECK_CFLAG(!(result16>>8));
uint8_t result8 = result16;
CHECK_VFLAG(((A ^ result8) & 0x80) && ((A ^ tmp) & 0x80));
A = result8;
CHECK_ZSFLAG(A);
```


### RLA - Rotate Left then 'And' - ROL + AND
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 


读改写(RMW)指令, 储存器数据循环左移再与累加器A做'与'运算.
影响FLAG: C(arry), Z(ero), S(ign). 伪C代码:
```c
// ROL
uint16_t src = READ(address);
src <<= 1;
if (CF) src |= 0x1;
CHECK_CFLAG(src > 0xff);
uint8_t result8 = src;
WRITE(address, result8);
// AND
A &= result8;
CHECK_ZSFLAG(A);
```

### RRA - Rotate Right then Add with Carry - ROR + ADC
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 


读改写(RMW)指令, 储存器数据循环右移再加上累加器A和进位标记.
拿来用来计算``` A+V/2```, 其中V是9位(支持到512)数据.
影响FLAG: C(arry), S(ign), Z(ero), (o)V(erflow), 伪C代码:
```
// ROR
uint16_t src = READ(address);
if (CF) src |= 0x100;
CF = src & 1;
src >> 1;
WRITE((uint8_t)src);
// ADC
uint16_t result16 = A + src + (CF ? 1 : 0);
CHECK_CFLAG(result16>>8);
uint8_t result8 = result16;
CHECK_VFLAG(!((A ^ src) & 0x80) && ((A ^ result8) & 0x80));
A = result8;
CHECK_ZSFLAG(A);
```

值得注意的是ADC的第一行的 (CF? 1: 0) 就是继承于ROR的第三行CF = 操作.
所以可以实现为:
```c
// ROR
// ...
tmp_CF = src & 1;
// ...
// ADC
uint16_t result16 = A + src + tmp_CF;
// ...
```


### SLO - Shift Left then 'Or' - ASL + ORA
助记符号: A |= (M <<= 1)

| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 


读改写(RMW)指令, 储存器数据算术左移一位然后和累加器A做'或'运算, 
由于要和累加器计算, 所以没有单字节指令```SLO A```, 即寻址方式为'累加器A'的.
影响FLAG: C(arry), Z(ero), S(ign). 伪C代码:
```c
// ASL
tmp = READ(adress);
CHECK_CFLAG(tmp>>7);
tmp <<= 1;
WRITE(address, tmpdt);
// ORA
A |= tmp;
CHECK_ZSFLAG(A);
```
### SRE - Shift Right then "Exclusive-Or" - LSR + EOR
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------|

读改写(RMW)指令, 储存器数据逻辑右移一位然后和累加器A做'异或'运算, 
由于要和累加器计算, 所以没有单字节指令```SRE A```, 即寻址方式为'累加器A'的.
影响FLAG: C(arry), Z(ero), S(ign). 伪C代码:
```c
// LSR
tmp = READ(address);
CHECK_CFLAG(tmp & 1);
tmp >>= 1;
WRITE(address, tmpdt);
// EOR
A ^= tmp;
CHECK_ZSFLAG(A);
```

### SHX[SXA] - 行为可能不一致, 不做实现
[SHX & SHY](https://forums.nesdev.com/viewtopic.php?f=3&t=8107)
"blargg's CPU test rom v5"中测试了该指令

### SHY[SYA] - 行为可能不一致, 不做实现
[SHX & SHY](https://forums.nesdev.com/viewtopic.php?f=3&t=8107)
"blargg's CPU test rom v5"中测试了该指令

### blargg's CPU test rom v5
测试ROM: "blargg's CPU test rom v5"中, 仅仅不通过以上两个指令.

### LAS - 行为可能不一致, 不做实现

### XAA - 行为可能不一致, 不做实现

### AHX[SHA] - 行为可能不一致, 不做实现

### TAS - 行为可能不一致, 不做实现

### KIL(STP) Kill Stop - 指令会导致CPU停下来, 不做实现

### REF
 - [nesdev-archive](http://nesdev.com/archive.html)
 - [NES 文档2.00](http://nesdev.com/nestech_cn.txt)
 - [Programming with unofficial opcodes](http://wiki.nesdev.com/w/index.php/Programming_with_unofficial_opcodes)
 - [6502_cpu](http://nesdev.com/6502_cpu.txt)