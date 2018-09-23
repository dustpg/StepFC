### StepC: 录制过程
现在有这么一个玩意, TAS, 差不多就是配合SL甚至放慢速度达到速通或者其他娱乐性目的. 这是一个有意思的东西, 这回来实现吧!

### 按键
这东西的关键点就是将按键信息实时保存下来, 比如第50帧按下了A键什么的, 这个由于每个模拟器可能会存在报道偏差(当然, 如果都是高精度的模拟器自然没问题), 作为中等精度的模拟器, 几乎只能自产自销.

另外有意思的是, 这个东西完全是外部接口控制的, 核心部分根本不管是不是重播, 还是人在玩, 所以, 本步骤不会修改核心部分.

可以从项目中看出, 非核心也就是接口实现的部分, 实现得很随意, 因为反正以后都会重写没必要认真写, 这部分也是. 

### 数据量
一帧按键是8字节, 可以打包到1个字节. 这样记录1小时需要```3600*60=216000```, "仅仅"200kb, 双人再乘上2...  所以完全可以在内存处理然后一次性记录到文件中.

当然, 我们可以设定上限就是一小时什么的.

### SL
因为重点是处理SL, 好在SL中我们保存了当前的帧ID, 32bit可以存2年够用了.

不过值得注意的是录制中不能读取以前不在录制中的档案, 这个需要单独出来一下, 最简单的是用文件夹隔离. 用户如果手动复制过去, 自己作死就不管我们的事情了!(就连这个也没事现!)

### 插入点
很简单在```sfc_render_frame_easy```调用前处理即可:
```c
static void ib_try_record_replay() {
    // 记录
    if (IB_IS_RECORD) {
        const uint8_t state = ib_pack_u8x8(g_famicom->button_states);
        ib_set_keys(g_famicom->frame_counter, state);
    }
    // 回放
    else if (IB_IS_REPLAY) {
        const uint8_t state = g_states.input_buffer_1mb[g_famicom->frame_counter];
        ib_unpack_u8x8(state, g_famicom->button_states);
    }
}
```

### 回放
来欣赏自己魂斗罗一命通(第一)关的流程吧! 实际上第一关太简单了, 不过没有连发键, 只有在前面断桥时用了几下SL大法.

![replay](./replay.gif)

虽然很简单, 看是看着回放自动运行, 还是有点成就感! 

项目地址[Github-StepFC-StepC](https://github.com/dustpg/StepFC/tree/master/stepc)

### 作业
 - 太简单了, 重写吧!

### REF
 - [TAS竞速](https://zh.wikipedia.org/wiki/TAS競速)