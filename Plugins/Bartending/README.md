# 原资产调酒移植 · UE 5.6

关卡：`/Game/Demos/Bartending/Maps/L_Bartending`。

`R` 清空，`1–5` 倒入对应配方，`P` 倒入下一配方，空格搅拌，`I` 切换冰块。倒入与搅拌各持续 5 秒；过程中拒绝重复启动操作。

## 资产与实现

- Cup、Liquid、Ice、Spoon、Decoration 原 FBX 与全部调酒贴图保存在 ArtSource，并已导入 Meshes / Textures。场景当前绑定原杯、液体、冰块和 Bar 背景；勺子与装饰模型也保留在资产目录供进一步装配。
- Reference/Unity 保留原材质参数、Prefab、脚本及自定义 Shader，供逐项对照。
- ABartendingDemo 保存五层配方，控制倒入、搅拌、浮冰与动态材质参数。
- 液体从原 FBX 的 LOD0 / Section0 读取顶点，以动画高度裁剪三角形并构建液面；UV0 保留原贴图坐标，UV1 保存归一化高度和液面标记。模型须启用 Allow CPU Access。
- LayeredLiquid.hlsl 使用原气泡、噪声、遮罩、环境贴图与原配方数值；搅拌在 CPU 更新颜色与遮罩混合权重。
- 杯子和冰块使用原 Normal / MatCap / 环境贴图。透明排序依次为冰块、液体、杯子。

## 移植边界

原艺术资产已保留，跨引擎渲染仍在对齐：Unity LiquidPass 的颜色/深度 RT 与冰块合成尚未移植，当前使用 UE 原生透明材质，因此交叠、折射、色调映射与液面反射存在差异。场景装饰与镜头也尚未逐项复刻 Unity。五层按高度划分，不代表不规则杯体的等体积液体；浮力为演示近似，没有倾倒或溢出求解。

## 工具

`Scripts/create_bartending_demo.py` 重建本插件的材质并绑定原资产，会覆盖相应材质图和场景配置。先保存自己的修改再执行。

自动化测试：`Automation RunTests Bartending.Recipe`。
