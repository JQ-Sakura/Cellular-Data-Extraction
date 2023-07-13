#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <fstream>

using namespace cv;

struct CellObject {
    int area;       // 面积
    float diameter; // 直径
    float circularity; // 圆度
    int x; // 矩形左上角的x坐标
    int y; // 矩形左上角的y坐标
    int width; // 矩形的像素宽度
    int height; // 矩形的像素高度
};

double calculateCircularity(const std::vector<Point>& contour) {
    // 近似计算轮廓的形状
    std::vector<Point> approxCurve;
    double epsilon = 0.02 * arcLength(contour, true);
    approxPolyDP(contour, approxCurve, epsilon, true);

    // 计算轮廓的面积和周长
    double area = contourArea(approxCurve);
    double perimeter = arcLength(approxCurve, true);

    // 计算圆度
    double circularity = (4 * CV_PI * area) / (perimeter * perimeter);

    return circularity;
}

Mat countCells(const Mat& image, std::vector<CellObject>& cellObjects) {
    // 进行图像预处理，例如灰度化、二值化等
    Mat grayImage;
    cvtColor(image, grayImage, COLOR_BGR2GRAY);

    
    // 应用阈值分割，将亮黄色区域分离出来
    Mat thresholdImage;
    //threshold(grayImage, thresholdImage, 200, 255, THRESH_BINARY);
    threshold(grayImage, thresholdImage, 187, 255, THRESH_BINARY);

    // 执行斑点检测
    std::vector<std::vector<Point>> contours;
    findContours(thresholdImage, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    // 在图像上绘制细胞轮廓并计算圆度和面积
    Mat resultImage = image.clone();
    int cellCount = 0;

    for (const auto& contour : contours) {
        // 计算细胞的像素面积
        int cellArea = static_cast<int>(contourArea(contour));

        // 如果细胞面积为0，则跳过该细胞
        if (cellArea == 0) {
            continue;
        }

        // 绘制细胞轮廓
        polylines(resultImage, { contour }, true, Scalar(0, 255, 0), 2);

        // 计算圆度
        double circularity = calculateCircularity(contour);

        // 计算细胞的直径（使用简化的计算方法，仅供参考）
        float cellDiameter = sqrt(4 * cellArea / CV_PI);

        // 计算细胞的最小外接矩形
        RotatedRect boundingRect = minAreaRect(contour);

        // 获取矩形的顶点
        Point2f rectPoints[4];
        boundingRect.points(rectPoints);

        // 将顶点坐标转换为整数坐标
        Point rectIntPoints[4];
        for (int i = 0; i < 4; i++) {
            rectIntPoints[i] = Point(static_cast<int>(rectPoints[i].x), static_cast<int>(rectPoints[i].y));
        }

        // 获取矩形左上角的坐标和宽度、高度
        int rectX = static_cast<int>(boundingRect.boundingRect().x);
        int rectY = static_cast<int>(boundingRect.boundingRect().y);
        int rectWidth = static_cast<int>(boundingRect.boundingRect().width);
        int rectHeight = static_cast<int>(boundingRect.boundingRect().height);

        // 在图像上绘制最小外接矩形
        for (int i = 0; i < 4; i++) {
            line(resultImage, rectPoints[i], rectPoints[(i + 1) % 4], Scalar(0, 0, 255), 2);
        }

        // 在图像上标记矩形位置和尺寸信息
        Point textPosition(rectX, rectY - 10);
        putText(resultImage, "Cell Index: " + std::to_string(cellCount + 1), textPosition,
            FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 2);
        textPosition.y += 20;
        putText(resultImage, "Area: " + std::to_string(cellArea), textPosition, FONT_HERSHEY_SIMPLEX,
            0.5, Scalar(0, 255, 0), 2);
        textPosition.y += 20;
        putText(resultImage, "Circularity: " + std::to_string(circularity), textPosition,
            FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 2);
        textPosition.y += 20;
        putText(resultImage, "Diameter: " + std::to_string(cellDiameter), textPosition,
            FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 2);
        textPosition.y += 20;
        putText(resultImage, "Rect (x, y): (" + std::to_string(rectX) + ", " + std::to_string(rectY) + ")", textPosition,
            FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 2);
        textPosition.y += 20;
        putText(resultImage, "Rect Size: " + std::to_string(rectWidth) + " * " + std::to_string(rectHeight), textPosition,
            FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 2);

        // 保存细胞对象的数据
        CellObject cellObject;
        cellObject.area = cellArea;
        cellObject.diameter = cellDiameter;
        cellObject.circularity = circularity;
        cellObject.x = rectX;
        cellObject.y = rectY;
        cellObject.width = rectWidth;
        cellObject.height = rectHeight;
        cellObjects.push_back(cellObject);

        cellCount++;
    }

    // 调整图像尺寸以适应显示屏
    double scaleFactor = 0.4;  // 缩放因子
    Mat resizedImage;
    resize(resultImage, resizedImage, Size(), scaleFactor, scaleFactor);

    // 输出处理后的图像
    imshow("Result Image", resizedImage);
    waitKey(0);

    return resultImage;
}

void saveCellDataToExcel(const std::string& filePath, const std::vector<CellObject>& cellObjects) {
    // 创建并打开CSV文件
    std::ofstream file(filePath);

    // 写入表头
    file << "Cell Index,Area,Diameter,Circularity,Rect X,Rect Y,Rect Width,Rect Height\n";

    // 逐个写入细胞数据
    for (int i = 0; i < cellObjects.size(); i++) {
        const auto& cell = cellObjects[i];
        file << i + 1 << "," << cell.area << "," << cell.diameter << ","
            << cell.circularity << "," << cell.x << "," << cell.y << "," 
            << cell.width << "," << cell.height << "\n";
    }

    // 关闭文件
    file.close();

    std::cout << "细胞数据已保存到文件：" << filePath << std::endl;
}

int main() {
    // 读取图像
    //std::string filePath = "D:\\Study\\ECNU-Proj\\Cells\\bf\\Image_20230605151036919.bmp";
    //std::string filePath = "D:\\Study\\ECNU-Proj\\Cells\\bf\\Image_20230605151139255.bmp";
    std::string filePath = "D:\\Study\\ECNU-Proj\\Cells\\bf\\Image_20230605163213362.bmp";
    Mat image = imread(filePath, IMREAD_COLOR);

    // 检查图像是否成功加载
    if (image.empty()) {
        std::cout << "无法读取图像文件" << std::endl;
        return -1;
    }

    // 检查图像尺寸
    if (image.cols <= 0 || image.rows <= 0) {
        std::cout << "图像尺寸无效" << std::endl;
        return -1;
    }

    std::vector<CellObject> cellObjects;
    Mat outputImage = countCells(image, cellObjects);

    // 保存处理完的带有标注的图像
    uchar* outputImg = outputImage.data;

    std::cout << "细胞数量: " << cellObjects.size() << std::endl;

    // 输出细胞对象的数据
    std::cout << "细胞面积: ";
    for (const auto& cell : cellObjects) {
        std::cout << cell.area << " ";
    }
    std::cout << std::endl;

    std::cout << "细胞直径: ";
    for (const auto& cell : cellObjects) {
        std::cout << cell.diameter << " ";
    }
    std::cout << std::endl;

    std::cout << "细胞圆度: ";
    for (const auto& cell : cellObjects) {
        std::cout << cell.circularity << " ";
    }
    std::cout << std::endl;

    // 将细胞数据保存到Excel文件中
    std::string excelFilePath = "cell_data.csv";
    saveCellDataToExcel(excelFilePath, cellObjects);

    return 0;
}
