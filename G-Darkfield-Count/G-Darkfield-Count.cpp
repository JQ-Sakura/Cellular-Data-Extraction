#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <fstream>

using namespace cv;

struct CellObject {
    int area;       // 面积
    float diameter; // 直径
    double circularity; // 圆度
    double fluorescence; // 荧光度
    int rectX;      // 最小外接矩形左上角的 x 坐标
    int rectY;      // 最小外接矩形左上角的 y 坐标
    int rectWidth;  // 最小外接矩形的宽度
    int rectHeight; // 最小外接矩形的高度
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
    threshold(grayImage, grayImage, 0, 255, THRESH_BINARY | THRESH_OTSU);

    // 执行斑点检测
    std::vector<std::vector<Point>> contours;
    findContours(grayImage, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    // 在图像上绘制细胞轮廓并计算圆度、面积、荧光度和最小外接矩形
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

        // 计算细胞的荧光度（在grayImage中计算平均值）
        double fluorescence = 0.0;
        if (!contour.empty()) {
            Rect boundingRect = cv::boundingRect(contour);
            if (boundingRect.x >= 0 && boundingRect.y >= 0 && boundingRect.x + boundingRect.width <= grayImage.cols && boundingRect.y + boundingRect.height <= grayImage.rows) {
                Mat cellRegion = grayImage(boundingRect);
                Scalar meanColor = mean(cellRegion);
                fluorescence = meanColor[0]; // G通道
            }
        }

        // 计算细胞的最小外接矩形
        RotatedRect minRect = minAreaRect(contour);
        Point2f rectPoints[4];
        minRect.points(rectPoints);

        // 在图像上绘制最小外接矩形
        for (int i = 0; i < 4; i++) {
            line(resultImage, rectPoints[i], rectPoints[(i + 1) % 4], Scalar(0, 255, 0), 2);
        }

        // 在图像上标记细胞序号、面积、圆度、荧光度和最小外接矩形信息
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
        textPosition.y += 20;
        putText(resultImage, "Rect Position: (" + std::to_string(boundingRect.x) + ", " + std::to_string(boundingRect.y) + ")", textPosition,
            FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 255), 2);
        textPosition.y += 20;
        putText(resultImage, "Rect Size: " + std::to_string(boundingRect.width) + " x " + std::to_string(boundingRect.height), textPosition,
            FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 255), 2);

        // 保存细胞对象的数据
        CellObject cellObject;
        cellObject.area = cellArea;
        cellObject.diameter = cellDiameter;
        cellObject.circularity = circularity;
        cellObject.fluorescence = fluorescence;
        cellObject.rectX = boundingRect.x;
        cellObject.rectY = boundingRect.y;
        cellObject.rectWidth = boundingRect.width;
        cellObject.rectHeight = boundingRect.height;
        cellObjects.push_back(cellObject);

        cellCount++;
    }

    return resultImage;
}



void saveCellDataToExcel(const std::string& filePath, const std::vector<CellObject>& cellObjects) {
    // 创建并打开CSV文件
    std::ofstream file(filePath);

    // 写入表头
    file << "Cell Index,Area,Diameter,Circularity,Fluorescence,Rect X,Rect Y,Rect Width,Rect Height\n";

    // 逐个写入细胞数据
    for (int i = 0; i < cellObjects.size(); i++) {
        const auto& cell = cellObjects[i];
        file << i + 1 << "," << cell.area << "," << cell.diameter << "," << cell.circularity << "," << cell.fluorescence << ","
            << cell.rectX << "," << cell.rectY << "," << cell.rectWidth << "," << cell.rectHeight << "\n";
    }

    // 关闭文件
    file.close();

    std::cout << "细胞数据已保存到文件：" << filePath << std::endl;
}



int main() {
    // 读取图像
    std::string filePath = "D:\\Study\\ECNU-Proj\\Cells\\Pics\\Image_20230607092503048.bmp";  //dark field black example
    //std::string filePath = "D:\\Study\\ECNU-Proj\\Cells\\Pics\\Image_20230607092455929.bmp";  //dark field green example
    //std::string filePath = "D:\\Study\\ECNU-Proj\\Cells\\Pics\\Image_20230607092533944.bmp";  //dark field red example
    //std::string filePath = "D:\\Study\\ECNU-Proj\\Cells\\Pics\\Image_20230607092817957.bmp";  //bright field white example
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
    Mat resultImage = countCells(image, cellObjects);  // 调用 countCells 函数，并将返回的图像保存到 resultImage 变量中

    uchar* outputImg = new uchar[resultImage.total() * resultImage.channels()];
    memcpy(outputImg, resultImage.data, resultImage.total() * resultImage.channels() * sizeof(uchar));

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
    
    // 调整图像尺寸以适应显示屏
    double scaleFactor = 0.5;  // 缩放因子
    Mat resizedImage;
    resize(resultImage, resizedImage, Size(), scaleFactor, scaleFactor);

    // 输出处理后的图像
    imshow("Processed Image", resizedImage);
    waitKey(0);

    delete[] outputImg; //释放内存

    return 0;
}
