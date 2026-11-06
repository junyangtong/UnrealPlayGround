# 彩窗投射 · UE 5.6

场景：`/Game/Demos/StainedGlass/Maps/L_StainedGlass`。点击 Play 后，C++ 创建投影体、深度捕获和动态材质。

## 实现

1. `AStainedGlassProjector` 从窗户坐标系、尺寸和方向光构建斜投影矩阵。
2. 矩阵写入组件 Custom Primitive Data；其他参数通过动态材质更新。
3. 原 `M_Proj` 材质 Custom 节点 使用引擎 `WorldPositionBehindTranslucency` 还原不透明接收点，投影到窗户 UV 后采样彩色纹理。
4. 正交 SceneCapture2D 输出 R32f 场景深度，Shader 在光空间比较接收点距离和遮挡深度。
5. 原 `M_Volume` 材质 Custom 节点 沿观察射线作 96 次采样，累积投影范围内可见的光束。

场景保留原彩窗、墙面、地板、蓝图与所需材质贴图。资产按 Maps / Blueprints / Meshes / Materials / Textures 分类，建筑依赖放在 Materials/Environment 和 Textures/Environment。

## 调整

- `SunLight`：控制投射方向的 DirectionalLight。
- `BaseWindowSize`、`MeshNormalAxis`、`WindowFrameSource`：窗户坐标和尺寸。
- `ProjectionDistance`、`Intensity`：投射距离和亮度。
- `ShadowMapResolution`、`ShadowDepthBias`、`ShadowStrength`：遮挡质量、偏移和强度。
- `ProjMaterial`、`VolumeMaterial`：可分别替换表面投影与光束材质；将 VolumeMaterial 清空可关闭光束。

投射器的网格在 Play 中创建，编辑器静止状态不会显示完整投射。若要改变窗口尺寸，注意 `BaseWindowSize` 还会乘玻璃组件缩放。

## 限制

屏幕空间接收位置只来自可见的不透明几何。深度捕获与屏幕射线采样都有分辨率误差，不能当作真实彩色阴影、全局光照或物理体积散射。当前 96 步光束采样没有时域累积，复杂遮挡下可能出现条纹；增加窗户数量也会增加捕获和透明填充成本。
