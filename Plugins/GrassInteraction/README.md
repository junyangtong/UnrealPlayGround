# GPU 草地交互 · UE 5.6

场景：`/Game/Demos/Grass/Maps/L_GrassInteraction`。点击 Play，再点击游戏视口。

| 操作 | 结果 |
| --- | --- |
| 空格 | 开关自动施力，观察撤力后的振荡与回弹 |
| 按住左键 | 在鼠标指向的地面处施力 |
| R | 清零两张状态 RT |
| Actor 的 Interactor 属性 | 用指定角色或 Actor 的位置施力 |

## 两张 RT

两张 `128×128 RGBA16f` 是同一份状态的前后版本，**不是一张速度、一张受力**。

| 通道 | 内容 |
| --- | --- |
| RG | XY 偏移，cm |
| BA | XY 速度，cm/s |

每步读取 A、写入 B，然后交换。每张约 128 KiB，允许负值；初始化清零、关闭 sRGB 和 mip、边缘 Clamp。力在当前步骤计算，不另存 RT。

```hlsl
acceleration = force - stiffness * displacement - damping * velocity;
velocity += acceleration * dt;
displacement += velocity * dt;
```

固定 `dt=1/120 s`，一帧最多 8 个子步，超过时丢弃多余积累时间。材质按草根 UV 读取偏移，使用 `height²` 权重弯曲，草根固定、草尖响应最大。

## 代码入口

- `Shaders/GrassSpring.usf`：弹簧积分。
- `Private/GrassSpringSimulation.cpp`：GPU Compute / RDG 调度。
- `Private/GrassInteractionDemo.cpp`：4096 簇交叉草片、输入、双 RT 交换。
- `Shaders/GrassWPO.hlsl`：偏移转 WPO。
- `Scripts/create_grass_demo.py`：生成独立材质与场景。

上述 C++ 文件位于 `Source/GrassInteraction/`。两块调试板分别显示当前状态的偏移、速度。

这是固定水平场地的实验：不包含大场景移动窗口、地形适配、风场、草间耦合或生产级植被系统。Shader 修改后需要同步到生成材质的 Custom 节点。

自动化入口：`Automation RunTests GrassInteraction.Spring`。测试直接调度 Compute Shader 并读回结果，检查双 RT、带符号通道和撤力回弹。
