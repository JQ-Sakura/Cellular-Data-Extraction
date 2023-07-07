#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <fstream>

using namespace cv;

struct CellObject {
    int area;       // 面积
    float diameter; // 直径
    std::vector<Point> contour; // 轮廓
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

void countCells(const Mat& image, std::vector<CellObject>& cellObjects) {
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

        // 在图像上标记细胞序号、面积和圆度信息
        Rect boundingRect = cv::boundingRect(contour);
        Point textPosition(boundingRect.x, boundingRect.y - 10);
        putText(resultImage, "Cell Index: " + std::to_string(cellCount + 1), textPosition,
            FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 2);
        textPosition.y += 20;
        putText(resultImage, "Area: " + std::to_string(cellArea), textPosition, FONT_HERSHEY_SIMPLEX,
            0.5, Scalar(0, 255, 0), 2);
        textPosition.y += 20;
        putText(resultImage, "Circularity: " + std::to_string(circularity), textPosition,
            FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 255, 0), 2);

        // 保存细胞对象的数据
        CellObject cellObject;
        cellObject.area = cellArea;
        cellObject.diameter = cellDiameter;
        cellObject.contour = contour; // 存储轮廓信息
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
}

void saveCellDataToExcel(const std::string& filePath, const std::vector<CellObject>& cellObjects) {
    // 创建并打开CSV文件
    std::ofstream file(filePath);

    // 写入表头
    file << "Cell Index,Area,Diameter,Circularity\n";

    // 逐个写入细胞数据
    for (int i = 0; i < cellObjects.size(); i++) {
        const auto& cell = cellObjects[i];
        file << i + 1 << "," << cell.area << "," << cell.diameter << "," << calculateCircularity(cell.contour) << "\n";
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
    countCells(image, cellObjects);

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

    // 将细胞数据保存到Excel文件中
    std::string excelFilePath = "cell_data.csv";
    saveCellDataToExcel(excelFilePath, cellObjects);

    return 0;
}
