#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <fstream>

using namespace cv;

struct CellObject {
    int area;              // 面积
    float diameter;        // 直径
    float circularity;     // 圆度
    float fluorescence;    // 荧光度
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
    Mat hsvImage;
    cvtColor(image, hsvImage, COLOR_BGR2HSV);

    // 进行图像预处理，例如灰度化、二值化等
    Mat gray0Image;
    cvtColor(image, gray0Image, COLOR_BGR2GRAY);
    threshold(gray0Image, gray0Image, 0, 255, THRESH_BINARY | THRESH_OTSU);

    // 根据红色的HSV范围创建掩膜
    Mat mask;
    Scalar lowerRed(0, 150, 60);   // 红色的HSV下界
    Scalar upperRed(10, 255, 255);  // 红色的HSV上界
    inRange(hsvImage, lowerRed, upperRed, mask);

    // 应用掩膜，仅保留红色像素
    Mat redImage;
    bitwise_and(image, image, redImage, mask);

    // 对红色像素进行亮度增强
    Mat enhancedImage = redImage.clone();
    for (int row = 0; row < enhancedImage.rows; ++row) {
        for (int col = 0; col < enhancedImage.cols; ++col) {
            Vec3b& pixel = enhancedImage.at<Vec3b>(row, col);
            if (pixel[2] > 0) {
                // 增加红色通道的亮度
                pixel[2] = std::min(pixel[2] + 50, 255);
            }
        }
    }

    // 对增强后的图像进行进一步处理，例如灰度化、二值化等
    Mat grayImage;
    cvtColor(enhancedImage, grayImage, COLOR_BGR2GRAY);

    // 执行斑点检测
    Mat thresholdImage;
    threshold(grayImage, thresholdImage, 0, 255, THRESH_BINARY | THRESH_OTSU);

    std::vector<std::vector<Point>> contours;
    findContours(thresholdImage, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    // 在图像上绘制细胞轮廓并计算圆度、面积和荧光度
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
        polylines(resultImage, { contour }, true, Scalar(0, 0, 255), 2);

        // 计算圆度
        double circularity = calculateCircularity(contour);

        // 计算细胞的直径（使用简化的计算方法，仅供参考）
        float cellDiameter = sqrt(4 * cellArea / CV_PI);

        // 计算细胞的荧光度
        //Scalar meanColor = mean(redImage, mask);
        //float fluorescence = meanColor[2];  // 使用V通道的平均值作为荧光度

        // 计算细胞的荧光度（在redImage中计算平均值）
        double fluorescence = 0.0;
        if (!contour.empty()) {
            Rect boundingRect = cv::boundingRect(contour);
            if (boundingRect.x >= 0 && boundingRect.y >= 0 && boundingRect.x + boundingRect.width <= redImage.cols && boundingRect.y + boundingRect.height <= redImage.rows) {
                Mat cellRegion = redImage(boundingRect);
                Scalar meanColor = mean(cellRegion);
                fluorescence = meanColor[2]; // R通道
            }
        }

        // 在图像上标记细胞序号、面积、圆度和荧光度信息
        Rect boundingRect = cv::boundingRect(contour);
        Point textPosition(boundingRect.x, boundingRect.y - 10);
        putText(resultImage, "Cell Index: " + std::to_string(cellCount + 1), textPosition,
            FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 255), 2);
        textPosition.y += 20;
        putText(resultImage, "Area: " + std::to_string(cellArea), textPosition, FONT_HERSHEY_SIMPLEX,
            0.5, Scalar(0, 0, 255), 2);
        textPosition.y += 20;
        putText(resultImage, "Circularity: " + std::to_string(circularity), textPosition,
            FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 255), 2);
        textPosition.y += 20;
        putText(resultImage, "Fluorescence: " + std::to_string(fluorescence), textPosition,
            FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 255), 2);

        // 保存细胞对象的数据
        CellObject cellObject;
        cellObject.area = cellArea;
        cellObject.diameter = cellDiameter;
        cellObject.circularity = static_cast<float>(circularity);
        cellObject.fluorescence = fluorescence;
        cellObjects.push_back(cellObject);

        cellCount++;
    }

    // 调整图像尺寸以适应显示屏
    double scaleFactor = 0.5;  // 缩放因子
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
    file << "Cell Index,Area,Diameter,Circularity,Fluorescence\n";

    // 逐个写入细胞数据
    for (int i = 0; i < cellObjects.size(); i++) {
        const auto& cell = cellObjects[i];
        file << i + 1 << "," << cell.area << "," << cell.diameter << ","
            << cell.circularity << "," << cell.fluorescence << "\n";
    }

    // 关闭文件
    file.close();

    std::cout << "细胞数据已保存到文件：" << filePath << std::endl;
}

int main() {
    // 读取图像
    std::string filePath = "D:\\Study\\ECNU-Proj\\Cells\\Pics\\Image_20230607092533944.bmp";  //dark field red example
    //std::string filePath = "D:\\Study\\ECNU-Proj\\Cells\\Pics\\Image_20230607093241698.bmp";  //bright field example
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

    std::cout << "细胞圆度: ";
    for (const auto& cell : cellObjects) {
        std::cout << cell.circularity << " ";
    }
    std::cout << std::endl;

    std::cout << "细胞荧光度: ";
    for (const auto& cell : cellObjects) {
        std::cout << cell.fluorescence << " ";
    }
    std::cout << std::endl;

    // 将细胞数据保存到Excel文件中
    std::string excelFilePath = "cell_data.csv";
    saveCellDataToExcel(excelFilePath, cellObjects);

    return 0;
}
