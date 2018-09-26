### Eagle
Eagle本身是一个比较初级的想法[Eagle (idea)](https://everything2.com/index.pl?node_id=1859453), 称不上真正的算法, 但是有些后续的算法就是基于Eagle的. Eagle是放大至2倍, 这里称之为eagle2x.

Eagle想得很简单, 放大两倍后, 那个像素旁边原本3个像素颜色一样的话就设定为该颜色, 否则就是最邻的.
```
first:        |Then
. . . --\ CC  |S T U  --\ 1 2
. C . --/ CC  |V C W  --/ 3 4
. . .         |X Y Z
              | IF V==S==T => 1=S
              | IF T==U==W => 2=U
              | IF V==X==Y => 3=X
              | IF W==Z==Y => 4=Z
```
同样可以根据同样的思想派生出eagle3x, 像素着色器可以简单实现为:
```hlsl
float4 eagle2x(uint2 pos) {
    const uint2 real_pos = pos / 2;
    float4 A, B, C;
    switch ((pos.x & 1) | ((pos.y & 1) << 1))
    {
    case 0:
        // AB
        // C
        A = InputTexture[real_pos - uint2(1, 1)];
        B = InputTexture[real_pos - uint2(0, 1)];
        C = InputTexture[real_pos - uint2(1, 0)];
        break;
    case 1:
        //  AB
        //   C
        A = InputTexture[real_pos - uint2(0, 1)];
        B = InputTexture[real_pos + int2(1, -1)];
        C = InputTexture[real_pos + uint2(1, 0)];
        break;
    case 2:
        // A
        // BC
        A = InputTexture[real_pos - uint2(1, 0)];
        B = InputTexture[real_pos + int2(-1, 1)];
        C = InputTexture[real_pos + uint2(0, 1)];
        break;
    case 3:
        //  A
        // BC
        A = InputTexture[real_pos + uint2(1, 0)];
        B = InputTexture[real_pos + uint2(0, 1)];
        C = InputTexture[real_pos + uint2(1, 1)];
        break;
    }
    return (eq(A, B) && eq(A, C)) ? A : InputTexture[real_pos];
}
```

来看看效果:

![eagle2x](./eagle2x.png)
(eagle2x后4倍最邻插值)

![eagle2x_vs](./eagle2x_vs.gif)
(与8倍最邻插值比较)

可以看出处理后很多东西变得粗细不一, 不适合缩放曲线条, 甚至可以看到一些单独的点被去掉了, 这个算法(想法)只能测试用, 没法实际使用. 

### 2×SaI
全称```2× Scale and Interpolation engine```, 作者```Derek Liauw Kie Fa(Kreed)```看到Eagle后想到的. 这里称为sai2x. 作者以GPL形式发布, (作者的)是一个完整的实现, 并不能算是算法, 维基提到可以以完全重写的方式避免被GPL病毒传染.

通过阅读源代码, emmm, 上古代码, 还是16bit的颜色, 对于显卡用的float4感觉差了一个世纪.

sai2x有一个重要步骤: 插值, 会生成新的颜色, 作者实现的插值感觉很魔幻(实际就是右移看作除以2, 然后提前打码避免移动到其他颜色通道, 都是奇数的话会产生1的误差, 所以再做了低位处理, 值得学习), 这里由于是```float4```直接取平均值.

```
I|E F|J
G|A B|K   A -\  A0 A1
H|C D|L     -/  A2 A3
M|N O|P

    A0 = A

    IF  A == D AND B != C, THEN
        IF (A == E AND B == L) OR (A == C AND A == F AND B != E AND B == J), THEN
            A1 = A
        ELSE
            A1 = INTERPOLATE(A, B)
        ENDIF
        IF (A == G AND C == O) OR (A == B AND A == H AND G != C AND C == M), THEN
            A2 = A
        ELSE
            A2 = INTERPOLATE(A, C)
        ENDIF
        A3 = A
    ELSIF B == C AND A != D, THEN
        IF (B == F AND A == H) OR (B == E AND B == D AND A != F AND A == I), THEN
            A1 = B
        ELSE
            A1 = INTERPOLATE(A, B)
        ENDIF
        IF (C == H AND A == F) OR (C == G AND C == D AND A != H AND A == I), THEN
            A2 = C
        ELSE
            A2 = INTERPOLATE(A, C)
        ENDIF
        A3 = B
    ELSIF A == D AND B == C, THEN
        IF A == B, THEN
            A1 = A2 = A3 = A
        ELSE
            A1 = INTERPOLATE(A, B)
            A2 = INTERPOLATE(A, C)
            A3 = A3_SP_PROC(A, B, C, D, ...)
        ENDIF
    ELSE
        IF A == C AND A == F AND B != E AND B == J, THEN
            A1 = A
        ELSIF B == E AND B == D AND A != F AND A == I, THEN
            A1 = B
        ELSE
            A1 = INTERPOLATE(A, B)
        ENDIF
        IF A == B AND A == H AND G != C AND C == M, THEN
            A2 = A
        ELSIF C == G AND C == D && A != H AND A == I, THEN
            A2 = C
        ELSE
            A2 = INTERPOLATE(A, C)
        ENDIF
        A3 = INTERPOLATE4(A, B, C, D);
    ENDIF
```
可以看出作者将图像分为几个情况分别处理, 比如最外面的4个分支分别对应, 这个点可能是'\', '/', 'X', 以及其他情况. 导致分支非常多, 中间还有一个作者自己发明的插值公式, 在这简写为```A3_SP_PROC```.


由于分支实在太多, 自己有可能重写错了, 如果错了的话, 这个算法算是自己派生的, 这个理由不错. 自己没错, 错的是键盘.

![sai2x](./sai2x.png)
(sai2x后4倍最邻插值)


![sai2x_vs](./sai2x_vs.gif)
(与8倍最邻插值, 以及2倍线性插值后4倍最邻插值比较)


可以看出因为A0点总是原来值, 所以放大后看起来像是往左上角偏了一个像素. 由于插值, 新加入了中间色, 实际上以100%比例看更合适, 所以像素风不再.

对于圆的处理效果很不错, 单独的点被处理成'星'状. 效果最差的是'点状网', 中间和边缘的效果不一致. 

### REF
