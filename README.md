# StylePointNOS
## Note: This project was assisted by an LLM.<br><br>
A plugin for **Need for Speed: Underground (v1.4.0)** that turns style points into nitrous.<br>
## How it works
Every frame the plugin diffs the style score total (the same value the HUD shows) and converts the change into the game's internal NOS units, scaled by `NOSPerPoint`, For example:<br>
- a drift awarding `+123` points at the default `10.0` → `+1230` ticks (1 tick = 1/4000 s of boost time)<br>
- a crash penalty of `-50` points → `-500` ticks<br>
## Installation
Requires an ASI loader — if you already run other `.asi` plugins, you are set.<br>
1. Copy `StylePointNOS.asi` and `StylePointNOS.ini` into the game's `Scripts` folder.<br>
2. Optionally tweak `NOSPerPoint` in the `.ini`.<br>
## Building
Open `StylePointNOS.sln` in Visual Studio, build **Release | x86**.<br>
Output: `Release\StylePointNOS.asi`. MinHook is bundled in `MinHook/`.<br>
## Known Issues
None found so far, if any exist, please submit an issue.<br><br>
# StylePointNOS_Chinese
## 注意：此项目使用了 LLM 辅助开发.
适用于 **Need for Speed: Underground (v1.4.0)** 的改进插件，能将每局赛事中即时获取的“风格点数”转化为可用氮气 (NOS).<br>
## 工作原理
插件会在每一帧检测风格总分（即 HUD 上显示的数值）的变化，并将该变化量按 `NOSPerPoint` 的比例转换为游戏内部的 NOS 单位，例如：<br>
- 漂移获得 `+123` 点数（默认 `NOSPerPoint` 为 `10.0`）→ 增加 `+1230` 个单位（1 个单位相当于 1/4000 秒使用量）<br>
- 事故扣除 `-50` 点数 → 减少 `-500` 个单位<br>
## 安装方法
需要 ASI 加载器（ASI loader）如果你已经在运行其他 `.asi` 插件，则无需额外安装.<br>
1. 将 `StylePointNOS.asi` 和 `StylePointNOS.ini` 复制到游戏的 `Scripts` 文件夹中.<br>
2. 可选：在 `.ini` 文件中调整 `NOSPerPoint` 参数.<br>
## 编译方法
在 Visual Studio 中打开 `StylePointNOS.sln`，选择 **Release | x86** 配置进行编译.<br>
输出文件：`Release\StylePointNOS.asi`，MinHook 库已包含在 `MinHook/` 目录下.<br>
## 已知问题
目前尚未发现任何问题；如有发现，请提交问题报告.
