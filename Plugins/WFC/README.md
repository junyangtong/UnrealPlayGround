# WFC / Town / Marching Squares · UE 5.6

## 三组实验

| 内容 | 关卡 |
| --- | --- |
| 四方向二维 WFC | `/Game/Demos/WFC/Maps/L_WFC` |
| 不规则四边形建造 | `/Game/Demos/WFC/Town/L_TownPrototype` |
| 顶点占用与模块变形 | `/Game/Demos/WFC/MarchingSquares/L_MarchingSquares` |
| 16 种编码模型库 | `/Game/Demos/WFC/MarchingSquares/L_SquareModelGallery` |

### 二维 WFC

`UWFCSolver` 选择候选数最少的未坍缩格，根据权重选 Tile，再传播四方向约束；矛盾时按 `MaxRetries` 重新求解。不是回溯搜索。

`UWFCTileSet` 保存 Tile 与上/右/下/左 Socket。示例使用原工程的 Tile 图集、16 张切片和 Tile_TileSet 数据资产。固定 Seed 可复现，Seed=0 使用随机值。

选择 `WFCGridGenerator`，通过 Details 的 Generate / Regenerate 重新生成；可更改 GridWidth、GridHeight、Seed、TileSet。`InitInteractiveMode` 可以切换为点击坍缩。插件还提供 Tile Cutter 与 Rule Editor 编辑器窗口。

### 城镇原型

`FWFCTownGrid`：六边形点阵 → 三角形 → 合并 → 四边形细分 → 邻接 → 平滑。`AWFCTownBuilder` 负责输入、拾取、撤销重做与组件提交。

Play 中：左键加层、右键减层，中键旋转、滚轮缩放，Z/Y 撤销重做。编辑器保存的建造状态随关卡保存；PIE 修改不自动保存。

### Marching Squares

共享顶点占用编码为 4 位掩码，匹配 16 种情况（0 为空），再把 100×100 的样例模型双线性变形到不规则四边形。对角情况 5/10 固定采用断开的两块。

模块资产在 `Content/Demos/WFC/MarchingSquares/Models`，对应表为 `DA_SquareSamples`。替换模型需保留角点和边中点位置、打开 Allow CPU Access，当前读取 LOD0 / Section0，使用统一顶点色材质。

**城镇和 Marching Squares 尚未接入 WFC 选模；当前没有多层体素、Marching Cubes 或游戏存档。**

## 目录

- `Source/WFCRuntime`：求解器、数据、网格、建造、模块变形。
- `Source/WFCEditor`：Slate 工具、图集处理、样例资产生成。
- `Scripts`：独立场景生成与邻接断言。
- `Content/Demos/WFC/Textures`：原图集与切片；`Data` 保存原 TileSet。
- `Source/WFCRuntime/Private/Tests`：拓扑、拾取与四角编码测试。

自动化入口：`Automation RunTests WFC.Town`。
