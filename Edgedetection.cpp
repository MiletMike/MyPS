/*
 * 实验三 第3部分：两种边缘检测算法实现
 * 编程环境：Visual Studio 2022 (C++)
 * 功能：对灰度BMP图像进行Sobel和Prewitt边缘检测
 * 特点：滤波器模板大小可调，不依赖外部库
 */

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

 // BMP文件头结构定义
#pragma pack(push, 1)
typedef struct {
    unsigned short bfType;      // 文件类型 'BM'
    unsigned int bfSize;        // 文件大小
    unsigned short bfReserved1; // 保留
    unsigned short bfReserved2; // 保留
    unsigned int bfOffBits;     // 数据偏移
} BITMAPFILEHEADER;

typedef struct {
    unsigned int biSize;          // 头大小
    int biWidth;                  // 图像宽度
    int biHeight;                 // 图像高度
    unsigned short biPlanes;      // 色彩平面数
    unsigned short biBitCount;    // 每像素位数
    unsigned int biCompression;   // 压缩方式
    unsigned int biSizeImage;     // 图像数据大小
    int biXPelsPerMeter;          // X分辨率
    int biYPelsPerMeter;          // Y分辨率
    unsigned int biClrUsed;       // 使用颜色数
    unsigned int biClrImportant;  // 重要颜色数
} BITMAPINFOHEADER;
#pragma pack(pop)

// 图像数据结构
typedef struct {
    int width;
    int height;
    unsigned char* data;  // 灰度数据，大小为 width * height
} GrayImage;

// ============================================
// 函数声明
// ============================================
GrayImage* readBMP(const char* filename);
int saveBMP(const char* filename, GrayImage* img);
GrayImage* createImage(int width, int height);
void freeImage(GrayImage* img);
GrayImage* sobelEdgeDetection(GrayImage* src, int threshold);
GrayImage* prewittEdgeDetection(GrayImage* src, int threshold);
GrayImage* sobelEdgeDetectionMultiScale(GrayImage* src, int kernelSize, int threshold);
GrayImage* prewittEdgeDetectionMultiScale(GrayImage* src, int kernelSize, int threshold);
void generateSobelKernels(float* gx, float* gy, int size);
void generatePrewittKernels(float* gx, float* gy, int size);

// ============================================
// 主函数
// ============================================
int main(int argc, char* argv[]) {
    // 输入输出文件名（可修改为自己的路径）
    const char* inputFile = "C:\\Users\\PC\\source\\repos\\Edgedetection\\x64\\Debug\\contact_lens_original.bmp";
    const char* outputSobel = "C:\\Users\\PC\\source\\repos\\Edgedetection\\x64\\Debug\\output_sobel.bmp";
    const char* outputPrewitt = "C:\\Users\\PC\\source\\repos\\Edgedetection\\x64\\Debug\\output_prewitt.bmp";

    printf("========================================\n");
    printf("  实验三：图像边缘检测\n");
    printf("========================================\n\n");

    // 1. 读取BMP图像
    printf("[1] 正在读取图像: %s\n", inputFile);
    GrayImage* img = readBMP(inputFile);
    if (img == NULL) {
        printf("错误：无法读取图像文件！\n");
        printf("提示：请将 contact_lens_original.bmp 放在程序同级目录下\n");
        printf("      或修改代码中的 inputFile 路径为绝对路径\n");
        system("pause");
        return -1;
    }
    printf("    图像尺寸: %d x %d\n\n", img->width, img->height);

    // 2. 使用 Sobel 算子进行边缘检测 (3x3模板，阈值可调)
    printf("[2] 正在执行 Sobel 边缘检测 (3x3模板)...\n");
    int sobelThreshold = 80;  // 边缘检测阈值，可根据效果调整 (0-255)
    GrayImage* sobelResult = sobelEdgeDetection(img, sobelThreshold);
    if (saveBMP(outputSobel, sobelResult) == 0) {
        printf("    Sobel检测结果已保存: %s\n\n", outputSobel);
    }

    // 3. 使用 Prewitt 算子进行边缘检测 (3x3模板，阈值可调)
    printf("[3] 正在执行 Prewitt 边缘检测 (3x3模板)...\n");
    int prewittThreshold = 80;  // 边缘检测阈值，可根据效果调整
    GrayImage* prewittResult = prewittEdgeDetection(img, prewittThreshold);
    if (saveBMP(outputPrewitt, prewittResult) == 0) {
        printf("    Prewitt检测结果已保存: %s\n\n", outputPrewitt);
    }

    // 4. 扩展功能：可变模板大小的边缘检测
    printf("[4] 扩展功能 - 5x5模板 Sobel 边缘检测...\n");
    int largeThreshold = 120;
    GrayImage* sobel5x5 = sobelEdgeDetectionMultiScale(img, 5, largeThreshold);
    if (saveBMP("output_sobel_5x5.bmp", sobel5x5) == 0) {
        printf("    5x5 Sobel检测结果已保存: output_sobel_5x5.bmp\n\n");
    }

    printf("[5] 扩展功能 - 5x5模板 Prewitt 边缘检测...\n");
    GrayImage* prewitt5x5 = prewittEdgeDetectionMultiScale(img, 5, largeThreshold);
    if (saveBMP("output_prewitt_5x5.bmp", prewitt5x5) == 0) {
        printf("    5x5 Prewitt检测结果已保存: output_prewitt_5x5.bmp\n\n");
    }

    // 释放内存
    freeImage(img);
    freeImage(sobelResult);
    freeImage(prewittResult);
    freeImage(sobel5x5);
    freeImage(prewitt5x5);

    printf("========================================\n");
    printf("  边缘检测完成！\n");
    printf("========================================\n");
    system("pause");
    return 0;
}

// ============================================
// BMP图像读取函数
// ============================================
GrayImage* readBMP(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) return NULL;

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    // 读取文件头
    fread(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fp);
    if (fileHeader.bfType != 0x4D42) {  // 检查是否为'BM'
        fclose(fp);
        return NULL;
    }

    // 读取信息头
    fread(&infoHeader, sizeof(BITMAPINFOHEADER), 1, fp);

    // 仅支持8位（灰度）和24位（彩色）BMP
    if (infoHeader.biBitCount != 8 && infoHeader.biBitCount != 24) {
        fclose(fp);
        return NULL;
    }

    int width = infoHeader.biWidth;
    int height = infoHeader.biHeight;
    int bitCount = infoHeader.biBitCount;

    // 如果是8位灰度图，跳过调色板
    if (bitCount == 8) {
        fseek(fp, 256 * 4, SEEK_CUR);  // 跳过256个RGBQUAD
    }

    // 计算每行字节数（4字节对齐）
    int rowSize = ((width * bitCount + 31) / 32) * 4;

    GrayImage* img = createImage(width, height);
    if (img == NULL) {
        fclose(fp);
        return NULL;
    }

    // 分配行缓冲区
    unsigned char* row = (unsigned char*)malloc(rowSize);

    // 读取图像数据（BMP是从下到上存储）
    for (int i = height - 1; i >= 0; i--) {
        fread(row, 1, rowSize, fp);
        if (bitCount == 8) {
            // 8位灰度图直接复制
            for (int j = 0; j < width; j++) {
                img->data[i * width + j] = row[j];
            }
        }
        else if (bitCount == 24) {
            // 24位彩色图转灰度: Y = 0.299R + 0.587G + 0.114B
            for (int j = 0; j < width; j++) {
                unsigned char b = row[j * 3];
                unsigned char g = row[j * 3 + 1];
                unsigned char r = row[j * 3 + 2];
                img->data[i * width + j] = (unsigned char)(0.299 * r + 0.587 * g + 0.114 * b);
            }
        }
    }

    free(row);
    fclose(fp);
    return img;
}

// ============================================
// BMP图像保存函数（保存为8位灰度图）
// ============================================
int saveBMP(const char* filename, GrayImage* img) {
    if (img == NULL) return -1;

    FILE* fp = fopen(filename, "wb");
    if (fp == NULL) return -1;

    int width = img->width;
    int height = img->height;
    int rowSize = ((width * 8 + 31) / 32) * 4;  // 8位灰度，4字节对齐
    int imageSize = rowSize * height;
    int paletteSize = 256 * 4;  // 灰度调色板

    BITMAPFILEHEADER fileHeader;
    BITMAPINFOHEADER infoHeader;

    // 填充文件头
    fileHeader.bfType = 0x4D42;  // 'BM'
    fileHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + paletteSize + imageSize;
    fileHeader.bfReserved1 = 0;
    fileHeader.bfReserved2 = 0;
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + paletteSize;

    // 填充信息头
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 8;
    infoHeader.biCompression = 0;
    infoHeader.biSizeImage = imageSize;
    infoHeader.biXPelsPerMeter = 2835;
    infoHeader.biYPelsPerMeter = 2835;
    infoHeader.biClrUsed = 256;
    infoHeader.biClrImportant = 256;

    fwrite(&fileHeader, sizeof(BITMAPFILEHEADER), 1, fp);
    fwrite(&infoHeader, sizeof(BITMAPINFOHEADER), 1, fp);

    // 写入灰度调色板
    for (int i = 0; i < 256; i++) {
        unsigned char palette[4] = { (unsigned char)i, (unsigned char)i, (unsigned char)i, 0 };
        fwrite(palette, 4, 1, fp);
    }

    // 写入图像数据（需要4字节对齐，从下到上存储）
    unsigned char* row = (unsigned char*)calloc(rowSize, 1);
    for (int i = height - 1; i >= 0; i--) {
        memset(row, 0, rowSize);
        for (int j = 0; j < width; j++) {
            row[j] = img->data[i * width + j];
        }
        fwrite(row, 1, rowSize, fp);
    }

    free(row);
    fclose(fp);
    return 0;
}

// ============================================
// 创建空图像
// ============================================
GrayImage* createImage(int width, int height) {
    GrayImage* img = (GrayImage*)malloc(sizeof(GrayImage));
    if (img == NULL) return NULL;
    img->width = width;
    img->height = height;
    img->data = (unsigned char*)calloc(width * height, sizeof(unsigned char));
    if (img->data == NULL) {
        free(img);
        return NULL;
    }
    return img;
}

// ============================================
// 释放图像内存
// ============================================
void freeImage(GrayImage* img) {
    if (img != NULL) {
        if (img->data != NULL) free(img->data);
        free(img);
    }
}

// ============================================
// Sobel边缘检测（3x3固定模板）
// ============================================
// Sobel算子：
// Gx = [-1  0  1]    Gy = [-1 -2 -1]
//      [-2  0  2]         [ 0  0  0]
//      [-1  0  1]         [ 1  2  1]
// ============================================
GrayImage* sobelEdgeDetection(GrayImage* src, int threshold) {
    if (src == NULL) return NULL;

    int width = src->width;
    int height = src->height;
    GrayImage* dst = createImage(width, height);
    if (dst == NULL) return NULL;

    // Sobel 3x3 核
    int gx[3][3] = { {-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1} };
    int gy[3][3] = { {-1, -2, -1}, {0, 0, 0}, {1, 2, 1} };

    // 遍历每个像素（跳过边界）
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int sumX = 0, sumY = 0;

            // 应用卷积核
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int pixel = src->data[(y + ky) * width + (x + kx)];
                    sumX += pixel * gx[ky + 1][kx + 1];
                    sumY += pixel * gy[ky + 1][kx + 1];
                }
            }

            // 计算梯度幅值
            int magnitude = (int)sqrt((double)sumX * sumX + (double)sumY * sumY);

            // 阈值判断
            if (magnitude > threshold) {
                dst->data[y * width + x] = 255;  // 边缘点（白色）
            }
            else {
                dst->data[y * width + x] = 0;    // 非边缘点（黑色）
            }
        }
    }

    return dst;
}

// ============================================
// Prewitt边缘检测（3x3固定模板）
// ============================================
// Prewitt算子：
// Gx = [-1  0  1]    Gy = [-1 -1 -1]
//      [-1  0  1]         [ 0  0  0]
//      [-1  0  1]         [ 1  1  1]
// ============================================
GrayImage* prewittEdgeDetection(GrayImage* src, int threshold) {
    if (src == NULL) return NULL;

    int width = src->width;
    int height = src->height;
    GrayImage* dst = createImage(width, height);
    if (dst == NULL) return NULL;

    // Prewitt 3x3 核
    int gx[3][3] = { {-1, 0, 1}, {-1, 0, 1}, {-1, 0, 1} };
    int gy[3][3] = { {-1, -1, -1}, {0, 0, 0}, {1, 1, 1} };

    // 遍历每个像素（跳过边界）
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int sumX = 0, sumY = 0;

            // 应用卷积核
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int pixel = src->data[(y + ky) * width + (x + kx)];
                    sumX += pixel * gx[ky + 1][kx + 1];
                    sumY += pixel * gy[ky + 1][kx + 1];
                }
            }

            // 计算梯度幅值
            int magnitude = (int)sqrt((double)sumX * sumX + (double)sumY * sumY);

            // 阈值判断
            if (magnitude > threshold) {
                dst->data[y * width + x] = 255;  // 边缘点（白色）
            }
            else {
                dst->data[y * width + x] = 0;    // 非边缘点（黑色）
            }
        }
    }

    return dst;
}

// ============================================
// 生成可变大小的Sobel核
// ============================================
void generateSobelKernels(float* gx, float* gy, int size) {
    int half = size / 2;

    // 生成平滑系数（基于帕斯卡三角形/二项式系数）
    float* smooth = (float*)malloc(size * sizeof(float));
    float* deriv = (float*)malloc(size * sizeof(float));

    // 平滑系数 (二项式系数)
    for (int i = 0; i <= half; i++) {
        smooth[half + i] = smooth[half - i] = 1.0f;
    }
    // 迭代生成高阶平滑系数
    for (int n = 1; n < half; n++) {
        for (int i = half; i > 0; i--) {
            smooth[half + i] = smooth[half + i] + smooth[half + i - 1];
            smooth[half - i] = smooth[half + i];  // 对称
        }
    }

    // 导数系数 [-1, 0, 1] 模式扩展
    for (int i = 0; i < size; i++) {
        deriv[i] = (float)(i - half);
    }

    // 构造 Gx = deriv(x) * smooth(y) 和 Gy = smooth(x) * deriv(y)
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            gx[y * size + x] = deriv[x] * smooth[y];
            gy[y * size + x] = smooth[x] * deriv[y];
        }
    }

    free(smooth);
    free(deriv);
}

// ============================================
// 生成可变大小的Prewitt核
// ============================================
void generatePrewittKernels(float* gx, float* gy, int size) {
    int half = size / 2;

    // Prewitt核：导数方向全1，平滑方向均匀权重
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            // Gx: x方向导数，y方向均匀平滑
            gx[y * size + x] = (float)(x - half);
            // Gy: y方向导数，x方向均匀平滑
            gy[y * size + x] = (float)(y - half);
        }
    }
}

// ============================================
// 可变模板大小的Sobel边缘检测
// ============================================
GrayImage* sobelEdgeDetectionMultiScale(GrayImage* src, int kernelSize, int threshold) {
    if (src == NULL || kernelSize % 2 == 0) return NULL;

    int width = src->width;
    int height = src->height;
    int half = kernelSize / 2;
    GrayImage* dst = createImage(width, height);
    if (dst == NULL) return NULL;

    // 生成核
    float* gx = (float*)malloc(kernelSize * kernelSize * sizeof(float));
    float* gy = (float*)malloc(kernelSize * kernelSize * sizeof(float));
    generateSobelKernels(gx, gy, kernelSize);

    // 归一化因子
    float scale = 0;
    for (int i = 0; i < kernelSize * kernelSize; i++) {
        scale += fabsf(gx[i]);
    }
    if (scale < 1.0f) scale = 1.0f;

    // 遍历每个像素
    for (int y = half; y < height - half; y++) {
        for (int x = half; x < width - half; x++) {
            float sumX = 0, sumY = 0;

            // 应用可变大小的卷积核
            for (int ky = -half; ky <= half; ky++) {
                for (int kx = -half; kx <= half; kx++) {
                    int pixel = src->data[(y + ky) * width + (x + kx)];
                    int kidx = (ky + half) * kernelSize + (kx + half);
                    sumX += pixel * gx[kidx];
                    sumY += pixel * gy[kidx];
                }
            }

            // 归一化并计算梯度幅值
            sumX /= scale;
            sumY /= scale;
            float magnitude = sqrtf(sumX * sumX + sumY * sumY);

            // 阈值判断
            if ((int)magnitude > threshold) {
                dst->data[y * width + x] = 255;
            }
            else {
                dst->data[y * width + x] = 0;
            }
        }
    }

    free(gx);
    free(gy);
    return dst;
}

// ============================================
// 可变模板大小的Prewitt边缘检测
// ============================================
GrayImage* prewittEdgeDetectionMultiScale(GrayImage* src, int kernelSize, int threshold) {
    if (src == NULL || kernelSize % 2 == 0) return NULL;

    int width = src->width;
    int height = src->height;
    int half = kernelSize / 2;
    GrayImage* dst = createImage(width, height);
    if (dst == NULL) return NULL;

    // 生成核
    float* gx = (float*)malloc(kernelSize * kernelSize * sizeof(float));
    float* gy = (float*)malloc(kernelSize * kernelSize * sizeof(float));
    generatePrewittKernels(gx, gy, kernelSize);

    // 归一化因子
    float scale = 0;
    for (int i = 0; i < kernelSize * kernelSize; i++) {
        scale += fabsf(gx[i]);
    }
    if (scale < 1.0f) scale = 1.0f;

    // 遍历每个像素
    for (int y = half; y < height - half; y++) {
        for (int x = half; x < width - half; x++) {
            float sumX = 0, sumY = 0;

            // 应用可变大小的卷积核
            for (int ky = -half; ky <= half; ky++) {
                for (int kx = -half; kx <= half; kx++) {
                    int pixel = src->data[(y + ky) * width + (x + kx)];
                    int kidx = (ky + half) * kernelSize + (kx + half);
                    sumX += pixel * gx[kidx];
                    sumY += pixel * gy[kidx];
                }
            }

            // 归一化并计算梯度幅值
            sumX /= scale;
            sumY /= scale;
            float magnitude = sqrtf(sumX * sumX + sumY * sumY);

            // 阈值判断
            if ((int)magnitude > threshold) {
                dst->data[y * width + x] = 255;
            }
            else {
                dst->data[y * width + x] = 0;
            }
        }
    }

    free(gx);
    free(gy);
    return dst;
}
