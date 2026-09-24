# PS — MFC 图像处理软件

一个基于 MFC 的图像处理软件，可以打开 **BMP / JPG** 图片，并对其进行灰度变换、直方图处理、调色板变换、空间域滤波、边缘检测、噪声添加、图像增强与图像恢复，共 28 项图像处理功能；另有灰度直方图与像素信息两种查看方式。

## 功能

### 文件

- 打开（BMP / JPG）、保存、另存为
- 打印、打印预览、打印设置
- 最近打开的文件列表

BMP 由自研解析器读取（`CImageProc::LoadBmp`），JPG 经 `CImage` 解码（`CImageProc::LoadJpg`）。

### 视图

- **灰度直方图**：显示当前图像的灰度直方图
- **像素信息**：显示鼠标所在位置的像素坐标与 RGB 值
- 工具栏与停靠窗口、状态栏
- 界面主题：Windows 2000 / Office XP / Windows XP / Office 2003 / VS 2005 / VS 2008 / Office 2007（蓝、黑、银、水色）

### 处理（Process 菜单）

| 分类 | 功能 |
| --- | --- |
| 灰度变换 | 灰度线性变换、伽马变换 |
| 直方图处理 | 直方图均衡化、直方图规格化、局部直方图均衡化 |
| 调色板变换 | 默认（蓝绿红）、热金属、彩虹、冷色调、灰度反转 |
| 滤波 | 均值滤波、中值滤波、最大值滤波 |
| 边缘检测 | Sobel 边缘检测、Prewitt 边缘检测、Laplacian 边缘检测 |
| 噪声 | 椒盐噪声、脉冲噪声、高斯噪声、高斯白噪声 |
| 图像增强 | 图像相加、图像相乘、幂律变换、返回原图 |
| 图像恢复 | 运动模糊、大气湍流模糊、逆滤波、维纳滤波 |

其中「图像恢复」的退化模型与复原滤波器都在频域实现：`运动模糊` / `大气湍流模糊` 生成对应的退化 PSF，`逆滤波` 与 `维纳滤波` 再据此复原。

### 噪声（Noise 菜单）

菜单栏另设带快捷键的噪声快捷入口，与「处理 → 噪声」功能相同：

| 快捷键 | 功能 |
| --- | --- |
| `S` | 椒盐噪声 |
| `I` | 脉冲噪声 |
| `G` | 高斯噪声 |
| `W` | 高斯白噪声 |

## 编译运行

### 环境要求

- **Visual Studio 2026**（v18.x），需勾选「使用 C++ 的桌面开发」工作负载
- **MFC**：Debug 配置动态链接 MFC，Release 配置静态链接
- **平台工具集 `v145`**（随 VS 2026 提供，MSVC 14.51）
- 平台：**Win32**（仅提供 Win32 配置，无 x64）

### 关于平台工具集

项目文件中的平台工具集写死为 `v145`。如果本机 Visual Studio 提供的工具集不同（例如 VS 2022 是 `v143`），直接生成会失败：

```
error MSB8020: 无法找到 v145 的生成工具(平台工具集 = "v145")。
```

两种解决办法：

1. 在解决方案资源管理器中右键解决方案 → **重定解决方案目标**，选择本机已安装的工具集；
2. 或直接编辑 `BmpReader/BmpReader.vcxproj`，把两处 `<PlatformToolset>` 的值改成目标工具集。

### 编译

用 Visual Studio 打开 `BmpReader.sln`，选择 `Debug|Win32` 或 `Release|Win32` 后生成（F7）。输出在 `BmpReader/Debug/` 或 `BmpReader/Release/`。

也可以用 MSBuild 命令行：

```bat
msbuild BmpReader\BmpReader.vcxproj /p:Configuration=Release /p:Platform=Win32
```

### 运行

直接双击生成的 `BmpReader.exe`。

- **Release** 静态链接 MFC，可独立分发；
- **Debug** 动态链接 MFC，需要系统中有对应的 MFC 调试运行库。

## 项目结构

```
BmpReader.sln                     解决方案文件
BmpReader/
  BmpReader.cpp/.h                应用程序入口
  MainFrm.cpp/.h                  主框架窗口
  BmpReaderDoc.cpp/.h             文档类
  BmpReaderView.cpp/.h            视图：绘图、鼠标交互、菜单命令响应
  CImageProc.cpp/.h               图像处理核心算法
  ClassView / FileView            停靠窗口
  OutputWnd / PropertiesWnd       停靠窗口
  HistogramDlg                    灰度直方图对话框
  ColorInfoDlg                    像素信息对话框
  BlockSizeDlg / CKernelSizeDlg   参数输入对话框
  BmpReader.rc                    资源：菜单、对话框、图标、字符串
Setup_PhotoShopping/              安装包工程（.vdproj）
```

## 已知问题

1. **源文件编码与编译器字符集不匹配**

   源文件为 UTF-8（`BmpReaderView.cpp` 带 BOM，其余不带），而 Debug 配置指定了 `/source-charset:GBK`，编译时会产生大量 `warning C4819`，中文字符串常量也可能显示异常。如需彻底解决，可把该选项改为 `/source-charset:utf-8 /execution-charset:gbk`。这些警告不影响程序运行。

2. **Debug 与 Release 使用了不同的字符集**

   `Debug|Win32` 为 `Unicode`，`Release|Win32` 为 `MultiByte`（见 `BmpReader.vcxproj` 第 23、31 行）。两个配置的行为因此不完全一致，中文字符串的处理方式也可能不同，建议统一为 `Unicode`。

3. **安装包工程无法在新版 Visual Studio 中加载**

   `Setup_PhotoShopping/Setup_PhotoShopping.vdproj` 是旧版部署工程格式，VS 2022 及以后默认不再支持，打开解决方案时会提示「找不到此项目类型所基于的应用程序」。需安装 Visual Studio 市场的 **Microsoft Visual Studio Installer Projects** 扩展。该工程不影响主程序的编译与运行。

4. **频域功能已移除**

   实验四的频域功能（FFT/IFFT 频谱图显示、同态滤波）在当前版本中已删除，相关接口、成员变量与菜单入口均已清理。`CImageProc` 中保留的 `FFT1D` / `FFT2D` 仅用于生成运动模糊与大气湍流模糊的退化模型。

## 许可

课程实验项目，仅供学习参考。
