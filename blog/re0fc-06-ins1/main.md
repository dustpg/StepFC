### STEP3: CPU 指令实现 - 基础指令
本文github[备份地址](https://github.com/dustpg/BlogFM/issues/10)

这节就详细谈谈基础指令, 所谓'基础指令'只是自己随便命名的, 避免一节过长, 请勿对号入座.

### 指令周期
不同指令需要消耗不同的周期, 这很好理解. 不过就算相同的指令环境不同也会消耗不同周期:

  - 页面边界交叉(Page Boundary Crossed)
    - 页面边界交叉是指6502将内存划分为256个页面(8位机但是拥有16位地址空间). 
    - 当访问不同页面时, 需要额外的指令周期去读取. 
  - 跳转到不同页面(Branch Occurs Different Page)
    - 意思和上面的差不多
  - 这两个会在这一步的最后一节详细谈谈

### LDA - Load "A"
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|----------|-----------|----|---------|----------|
|  立即     | LDA #Oper    | A9 |    2    |    2     |
|  零页     | LDA Oper     | A5 |    2    |    3     |
|  零页,X   | LDA Oper,X   | B5 |    2    |    4     |
|  绝对     | LDA Oper     | AD |    3    |    4     |
|  绝对,X   | LDA Oper,X   | BD |    3    |    4*    |
|  绝对,Y   | LDA Oper,Y   | B9 |    3    |    4*    |
|  (间接,X) | LDA (Oper,X) | A1 |    2    |    6     |
|  (间接),Y | LDA (Oper),Y | B1 |    2    |    5*    |

\* 在页面边界交叉时 +1s

由存储器取数送入累加器A, 影响FLAG: Z(ero),S(ign), 伪C代码:
```c
A = READ(address);
CHECK_ZSFLAG(A);
```

### LDX - Load 'X'
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|--------------|----|---------|----------|
|  立即     |   LDX #Oper    |  A2  |    2    |    2     |
|  零页     |   LDX Oper     |  A6  |    2    |    3     |
|  零页, Y  |   LDX Oper,Y   |  B6  |    2    |    4     |
|  绝对     |   LDX Oper     |  AE  |    3    |    4     |
|  绝对, Y  |   LDX Oper,Y   |  BE  |    3    |    4*    |

\* 在页面边界交叉时 +1s

由存储器取数送入变址寄存器X, 影响FLAG: Z(ero),S(ign), 伪C代码:
```c
X = READ(address);
CHECK_ZSFLAG(X);
```

### LDY - Load 'Y'
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|--------------|----|---------|----------|
|  立即   |   LDX #Oper   |    A2   |    2    |    2     |
|  零页   |   LDX Oper    |    A6   |    2    |    3     |
|  零页,Y |   LDX Oper,Y  |    B6   |    2    |    4     |
|  绝对   |   LDX Oper    |    AE   |    3    |    4     |
|  绝对,Y |   LDX Oper,Y  |    BE   |    3    |    4*    |

\* 在页面边界交叉时 +1s

由存储器取数送入变址寄存器Y, 影响FLAG: Z(ero),S(ign), 伪C代码:
```c
Y = READ(address);
CHECK_ZSFLAG(Y);
```

### STA - Store 'A'

| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------|
|  零页   |   STA Oper       |    85   |    2    |    3     |
|  零页,X |   STA Oper,X     |    95   |    2    |    4     |
|  绝对   |   STA Oper       |    80   |    3    |    4     |
|  绝对,X |   STA Oper,X     |    90   |    3    |    5     |
|  绝对,Y |   STA Oper, Y    |    99   |    3    |    5     |
| (间接,X)|   STA (Oper,X)   |    81   |    2    |    6     |
| (间接),Y|   STA (Oper),Y   |    91   |    2    |    6     |

将累加器A的数送入存储器, 影响FLAG:(无), 伪C代码:
```c
WRTIE(address, A);
```

### STX - Store 'X'

| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------|
|  零页  |  STX Oper    |  86 |    2    |    3     |
|  零页,Y|  STX Oper,Y  |  96 |    2    |    4     |
|  绝对  |  STX Oper    |  8E |    3    |    4     |

将变址寄存器X的数送入存储器, 影响FLAG:(无), 伪C代码:
```c
WRTIE(address, X);
```
### STY - Store 'Y'

| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------|
|  零页  |   STY Oper     |  84  |   2   |    3     |
| 零页,X |   STY Oper,X   |  94  |   2   |    4     |
| 绝对 |   STY Oper       |  8C  |   3   |    4     |

将变址寄存器Y的数送入存储器, 影响FLAG:(无), 伪C代码:
```c
WRTIE(address, Y);
```

### ADC - Add with Carry
助记符号 A = A + M +C

| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------|
|  立即    | ADC #Oper      |  69  |    2    |    2     |
|  零页    | ADC Oper       |  65  |    2    |    3     |
|  零页,X  | ADC Oper,X     |  75  |    2    |    4     |
|  绝对    | ADC Oper       |  60  |    3    |    4     |
|  绝对,X  | ADC Oper,X     |  70  |    3    |    4*    |
|  绝对,Y  | ADC Oper,Y     |  79  |    3    |    4*    |
| (间接,X) | ADC (Oper,X)   | 61  |    2    |    6     |
| (间接),Y | ADC (Oper),Y   |  71  |    2    |    5*    |

\* 在页面边界交叉时 +1s

累加器,存储器,进位标志C相加,结果送累加器A.
影响FLAG: S(ign), Z(ero), C(arry), (o)V(erflow), 伪C代码:
```c
src = READ(address);
uint16_t result16 = A + src + (CF ? 1 : 0);
CHECK_CFLAG(result16>>8);
uint8_t result8 = result16;
CHECK_VFLAG(!((A ^ src) & 0x80) && ((A ^ result8) & 0x80));
A = result8;
CHECK_ZSFLAG(A);
```

### SBC - Subtract with Carry
助记符号  A = A - M - (1-C)

| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------|
|  立即   | SBC #Oper     |  E9   |    2    |    2     |
|  零页   | SBC Oper      |  E5   |    2    |    3     |
|  零页,X | SBC Oper,X    |  F5   |    2    |    4     |
|  绝对   | SBC Oper      |  ED   |    3    |    4     |
|  绝对,X | SBC Oper,X    |  FD   |    3    |    4*    |
|  绝对,Y | SBC Oper,Y    |  F9   |    3    |    4*    |
| (间接,X)| SBC (Oper,X)  |  E1   |    2    |    6     |
| (间接),Y| SBC (Oper),Y  |  F1   |    2    |    5     |

从累加器减去存储器和进位标志C,结果送累加器A.
影响FLAG: S(ign), Z(ero), C(arry), (o)V(erflow), 伪C代码:
```c
src = READ(address);
uint16_t result16 = A - src - (CF ? 0 : 1);
CHECK_CFLAG(!(result16>>8));
uint8_t result8 = result16;
CHECK_VFLAG(((A ^ result8) & 0x80) && ((A ^ src) & 0x80));
A = result8;
CHECK_ZSFLAG(A);
```

### INC - Increment memory
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------|  
|  零页   |   INC Oper    |   E6   |    2    |    5  |
|  零页,X |   INC Oper,X  |   F6   |    2    |    6  |
|  绝对   |   INC Oper    |   EE   |    3    |    6  |
|  绝对,X |   INC Oper,X  |   FE   |    3    |    7  |

存储器单元内容+1, 影响FLAG:Z(ero),S(ign), 伪C代码:
```
tmp = READ(address);
++tmp;
WRITE(address, tmp);
CHECK_ZSFLAG(tmp);
```

### DEC - Decrement memory
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------|  
|  零页   |  DEC Oper   |  C6   |    2    |    5     |
|  零页,X |  DEC Oper,X |  D6   |    2    |    6     |
|  绝对   |  DEC Oper   |  CE   |    3    |    6     |
|  绝对,X |  DEC Oper,X |  DE   |    3    |    7     |

存储器单元内容-1, 影响FLAG:Z(ero),S(ign), 伪C代码:
```
tmp = READ(address);
--tmp;
WRITE(address, tmp);
CHECK_ZSFLAG(tmp);
```

### AND - 'And' memory with A
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
|  立即   |  AND #Oper   |   29   |    2    |    2     |
|  零页   |  AND Oper    |   25   |    2    |    3     |
|  零页,X |  AND Oper,X  |   35   |    2    |    4     |
|  绝对   |  AND Oper    |   2D   |    3    |    4     |
|  绝对,X |  AND Oper,X  |   3D   |    3    |    4*    |
|  绝对,Y |  AND Oper,Y  |   39   |    3    |    4*    |
| (间接,X)| AND (Oper,X) |   21   |    2    |    6     |
| (间接),Y| AND (Oper),Y |   31   |    2    |    5     |

\* 在页面边界交叉时 +1s

存储器单元与累加器做与运算, 影响FLAG:Z(ero),S(ign), 伪C代码:
```c
A &= READ(address);
CHECK_ZSFLAG(A);
```
### ORA - 'Or' memory with A
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
|  立即   |  ORA #Oper   |  09  |   2   |   2    |
|  零页   |  ORA Oper    |  05  |   2   |    3   |
|  零页,X |  ORA Oper,X  |  15  |   2   |    4   |
|  绝对   |  ORA Oper    |  0D  |   3   |    4   |
|  绝对,X |  ORA Oper,X  |  10  |   3   |    4*  |
|  绝对,Y |  ORA Oper,Y  |  19  |   3   |    4*  |
| (间接,X)|  ORA (Oper,X)|  01  |   2   |    6   |
| (间接),Y|  ORA (Oper),Y|  11  |   2   |    5   |

\* 在页面边界交叉时 +1s

存储器单元与累加器做或运算, 影响FLAG:Z(ero),S(ign), 伪C代码:
```c
A |= READ(address);
CHECK_ZSFLAG(A);
```
  
### ERA - "Exclusive-Or" memory with A
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
|  立即   |  EOR #Oper   |   49   |    2    |    2     |
|  零页   |  EOR Oper    |   45   |    2    |    3     |
|  零页,X |  EOR Oper,X  |   55   |    2    |    4     |
|  绝对   |  EOR Oper    |   40   |    3    |    4     |
|  绝对,X |  EOR Oper,X  |   50   |    3    |    4*    |
|  绝对,Y |  EOR Oper,Y  |   59   |    3    |    4*    |
| (间接,X)|  EOR (Oper,X)|   41   |    2    |    6     |
| (间接),Y|  EOR (Oper),Y|   51   |    2    |    5*    |

  \* 在页面边界交叉时 +1s

存储器单元与累加器做或运算, 影响FLAG:Z(ero),S(ign), 伪C代码:
```c
A ^= READ(address);
CHECK_ZSFLAG(A);
```

## INX - Increment X
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
|  隐含  |   INX    |    E8   |    1    |    2     |

变址寄存器X内容+1, 影响FLAG:Z(ero),S(ign), 伪C代码:
```c
++X;
CHECK_ZSFLAG(X);
```

### DEX - Decrement X
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
|  隐含  |  DEX    |    CA   |    1    |    2    |

变址寄存器X内容-1, 影响FLAG:Z(ero),S(ign), 伪C代码:
```c
--X;
CHECK_ZSFLAG(X);
```

### INY - Increment Y
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
|  隐含  |   INY    |    C8   |    1    |    2     |

变址寄存器Y内容+1, 影响FLAG:Z(ero),S(ign), 伪C代码:
```c
++Y;
CHECK_ZSFLAG(Y);
```

### DEY - Decrement Y
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
 |  隐含   |   DEY    |    88   |    1    |    2     |

变址寄存器Y内容-1, 影响FLAG:Z(ero),S(ign), 伪C代码:
```c
--Y;
CHECK_ZSFLAG(Y);
```

### TAX - Transfer A to X
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------|
|  隐含   |   TAX      |   AA  |   1   |   2    |

将累加器A的内容送入变址寄存器X, 影响FLAG:Z(ero),S(ign), 伪C代码:
```c
X = A;
CHECK_ZSFLAG(X);
```

### TXA - Transfer X to A
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------|
|  隐含   |   TXA      |    8A   |    1    |    2     |

将累加器A的内容送入变址寄存器X, 影响FLAG:Z(ero),S(ign), 伪C代码:
```c
A = X;
CHECK_ZSFLAG(A);
```

### TAY - Transfer A to Y
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------|
|  隐含  |   TAY     |   A8  |   1    |    2     |

将累加器A的内容送入变址寄存器Y, 影响FLAG:Z(ero),S(ign), 伪C代码:
```c
Y = A;
CHECK_ZSFLAG(Y);
```

### TYA - Transfer Y to A
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------|
|  隐含   |   TYA   |    98   |    1    |    2     |

将变址寄存器Y的内容送入累加器A, 影响FLAG:Z(ero),S(ign), 伪C代码:
```c
A = Y;
CHECK_ZSFLAG(A);
```

### TSX - Transfer SP to X
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------|
|  隐含  |   TSX     |  BA  |   1    |    2     |

将栈指针SP内容送入变址寄存器X, 影响FLAG:Z(ero),S(ign), 伪C代码:
```c
X = SP;
CHECK_ZSFLAG(X);
```
### TXS - Transfer X to SP
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------|
|  隐含  |   TXS    |   9A   |    1    |    2     |

将变址寄存器X内容送入栈指针SP, 影响FLAG:**无**, 伪C代码:
```c
SP = X;
```

### CLC - Clear Carry 
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
|  隐含  |   CLC     |    18   |    1    |    2    |

清除进位标志C,  影响FLAG: C(arry), 伪C代码:
```c
CF = 0;
```
### SEC - Set Carry
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
|  隐含  |   SEC     |   38   |    1    |    2    |

设置进位标志C,  影响FLAG: C(arry), 伪C代码:
```c
CF = 1;
```

### CLD - Clear Decimal
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
|  隐含  |   CLD     |    D8   |    1    |    2     |

清除十进制模式标志D,  影响FLAG: D(Decimal), 伪C代码:
```c
DF = 0;
```

### SED - Clear Decimal
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
|  隐含  |   SED    |    F8   |    1    |    2     |

设置十进制模式标志D,  影响FLAG: D(Decimal), 伪C代码:
```c
DF = 1
```

### CLV - Clear Overflow
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
|  隐含  |    CLV     |    B8   |    1    |    2     |

清除溢出标志V,  影响FLAG: (o)V(erflow), 伪C代码:
```c
VF = 0;
```

### CLI - Clear Interrupt-disable
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
|  隐含  |    CLI   |    58   |    1    |    2     |

清除中断禁止标志I,  影响FLAG: I(nterrupt-disable), 伪C代码:
```c
IF = 0;
```

### SEI - Set Interrupt-disable
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
|  隐含  |    SEI   |    78   |    1    |    2     |

设置中断禁止标志I,  影响FLAG: I(nterrupt-disable), 伪C代码:
```c
IF = 1;
```

### CMP - Compare memory with A
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
|  立即      |   CMP #Oper           |    C9   |    2    |    2     |
|  零页      |   CMP Oper            |    C5   |    2    |    3     |
|  零页,X    |   CMP Oper,X          |    D5   |    2    |    4     |
|  绝对      |   CMP Oper            |    CD   |    3    |    4     |
|  绝对,X    |   CMP Oper,X          |    DD   |    3    |    4*    |
|  绝对,Y    |   CMP Oper,Y          |    D9   |    3    |    4*    |
|  (间接,X)  |   CMP (Oper,X)        |    C1   |    2    |    6     |
|  (间接),Y  |   CMP (Oper),Y        |    D1   |    2    |    5*    |

比较储存器值与累加器A.
影响FLAG: C(arry), S(ign), Z(ero). 伪C代码: 
```c
uint16_t result16 = (uint16_t)A - (uint16_t)READ(address);
CF = result16 < 0x100;
CHECK_ZSFLAG((uint8_t)result16);
```


### CPX - Compare memory with X
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
|  立即     |   CPX #Oper           |    E0   |    2    |    2     |
|  零页     |   CPX Oper            |    E4   |    2    |    3     |
|  绝对     |   CPX Oper            |    EC   |    3    |    4     |

比较储存器值与变址寄存器X.
影响FLAG: C(arry), S(ign), Z(ero). 伪C代码: 
```c
uint16_t result16 = (uint16_t)X - (uint16_t)READ(address);
CF = result16 < 0x100;
CHECK_ZSFLAG((uint8_t)result16);
```

### CPY - Compare memory with Y
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
|  立即     |   CPY #Oper           |    C0   |    2    |    2     |
|  零页     |   CPY Oper            |    C4   |    2    |    3     |
|  绝对     |   CPY Oper            |    CC   |    3    |    4     |

比较储存器值与变址寄存器Y.
影响FLAG: C(arry), S(ign), Z(ero). 伪C代码: 
```c
uint16_t result16 = (uint16_t)Y - (uint16_t)READ(address);
CF = result16 < 0x100;
CHECK_ZSFLAG((uint8_t)result16);
```

### BIT - Bit test memory with A
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
|  零页     |   BIT Oper            |    24   |    2    |    3     |
|  绝对     |   BIT Oper            |    2C   |    3    |    4     |

位测试
 - 若 A&M 结果 =0, 那么Z=1
 - 若 A&M 结果!=0, 那么Z=0
 - S = M的第7位 
 - V = M的第6位

影响FLAG: (o)V(erflow), S(ign), Z(ero). 伪C代码: 
```c
tmp = READ(address);
VF = (tmp >> 6) & 1;
SF = (tmp >> 7) & 1;
ZF = A & tmp ? 0 : 1;
```

### ASL - Arithmetic Shift Left
助记符号: C <- |7|6|5|4|3|2|1|0| <- 0

| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------| 
|  累加器A  |   ASL A               |    0A   |    1    |    2     |
|  零页     |   ASL Oper            |    06   |    2    |    5     |
|  零页,X   |   ASL Oper,X          |    16   |    2    |    6     |
|  绝对     |   ASL Oper            |    0E   |    3    |    6     |
|  绝对, X  |   ASL Oper,X          |    1E   |    3    |    7     |

累加器A, 或者存储器单元算术按位左移一位. 最高位移动到C, 最低位0. 影响FLAG: S(ign) Z(ero) C(arry), 伪C代码:
```c
// ASL A:
CHECK_CFLAG(A>>7);
A <<= 1;
CHECK_ZSFLAG(A);

// 其他情况
tmp = READ(address);
CHECK_CFLAG(tmp>>7);
tmp <<= 1;
WRITE(address, tmp);
CHECK_ZSFLAG(tmp);

```

### LSR - Logical Shift Right
助记符号: 0 -> |7|6|5|4|3|2|1|0| -> C

| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------|
|  累加器A  |   LSR A               |    4A   |    1    |    2     |
|  零页     |   LSR Oper            |    46   |    2    |    5     |
|  零页,X   |   LSR Oper,X          |    56   |    2    |    6     |
|  绝对     |   LSR Oper            |    4E   |    3    |    6     |
|  绝对,X   |   LSR Oper,X          |    5E   |    3    |    7     |

累加器A, 或者存储器单元逻辑按位右移一位. 最低位回移进C, 最高位变0, 影响FLAG: S(ign) Z(ero) C(arry), 伪C代码:
```c
// LSR A:
CHECK_CFLAG(A & 1);
A >>= 1;
CHECK_ZSFLAG(A);

// 其他情况
tmp = READ(address);
CHECK_CFLAG(tmp & 1);
tmp >>= 1;
WRITE(address, tmp);
CHECK_ZSFLAG(tmp);
```

### ROL - Rotate Left
助记符号: ...|0| <- C <- |7|6|5|4|3|2|1|0| <- C <- |7|...

| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------|
|  累加器A  |   ROL A               |    2A   |    1    |    2     |
|  零页     |   ROL Oper            |    26   |    2    |    5     |
|  零页,X   |   ROL Oper,X          |    36   |    2    |    6     |
|  绝对     |   ROL Oper            |    2E   |    3    |    6     |
|  绝对,X   |   ROL Oper,X          |    3E   |    3    |    7     |

累加器A, 或者储存器内容 连同C位 按位循环左移一位, 影响FLAG: S(ign) Z(ero) C(arry), 伪C代码:
```c
// A_M 意思到了就行
uint16_t src = A_M;
src <<= 1;
if (CF) src |= 0x1;
CHECK_CFLAG(src > 0xff);
A_M = src;
CHECK_ZSFLAG(A_M);
```

### ROR - Rotate Right
助记符号: ...|0| -> C -> |7|6|5|4|3|2|1|0| -> C -> |7|...

| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------|
|  累加器A  |   ROR A               |    6A   |    1    |    2     |
|  零页     |   ROR Oper            |    66   |    2    |    5     |
|  零页,X   |   ROR Oper,X          |    76   |    2    |    6     |
|  绝对     |   ROR Oper            |    6E   |    3    |    6     |
|  绝对,X   |   ROR Oper,X          |    7E   |    3    |    7     |


累加器A, 或者储存器内容 连同C位 按位循环左移一位, 影响FLAG: S(ign) Z(ero) C(arry), 伪C代码:
```c
// A_M 意思到了就行
uint16_t src = A_M;
if (CF) src |= 0x100;
CF = src & 1;
src >> 1;
A_M = src;
CHECK_ZSFLAG(A_M);
```

### PHA - Push A
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------|
|  隐含       |   PHA                 |    48   |    1    |    3     |

累加器A压入栈顶(栈指针SP-1). 影响FLAG:(无), 伪C代码:
```c
PUSH(A);
```

### PLA - Pull(Pop) A
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------|
|  隐含       |   PLA                 |    68   |    1    |    4     |


push的反操作, 在6502汇编称为pull, 不过自己还是习惯称为pop.
[6502_cn](http://nesdev.com/6502_cn.txt)这篇文档提到没有FLAG修改, 可能是笔误, 其他文档([比如这个](http://obelisk.me.uk/6502/reference.html))提到会影响S(ign) Z(ero) , 伪C代码:
```c
A = POP();
CHECK_ZSFLAG(A);
```

### PHP - Push Processor-status 
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------|
|  隐含       |   PHP                 |    08   |    1    |    3     |

将状态FLAG压入栈顶, 影响FLAG:(无), 伪C代码:
```c
PUSH(P | FLAG_B | FLAG_R);
```

### PLP - Pull Processor-status
| 寻址模式| 汇编格式| OP代码 |指令字节|指令周期|
|--------|-----------|----|---------|----------|
|  隐含       |   PLP                 |    28   |    1    |    4     |

PHP逆操作, 影响FLAG: (是的), 伪C代码:
```c
p = POP();
// 无视BIT4 BIT5
RF = 1;
BF = 0;
```

### REF
 - [nesdev-archive](http://nesdev.com/archive.html)
 - [NES 文档2.00](http://nesdev.com/nestech_cn.txt)
 - [6502 微处理器](http://nesdev.com/6502_cn.txt)
 - [6502_cpu](http://nesdev.com/6502_cpu.txt)
