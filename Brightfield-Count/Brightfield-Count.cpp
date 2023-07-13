#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <fstream>

using namespace cv;

struct CellObject {
    int area;       // ���
    float diameter; // ֱ��
    float circularity; // Բ��
    int x; // �������Ͻǵ�x����
    int y; // �������Ͻǵ�y����
    int width; // ���ε����ؿ��
    int height; // ���ε����ظ߶�
};

double calculateCircularity(const std::vector<Point>& contour) {
    // ���Ƽ�����������״
    std::vector<Point> approxCurve;
    double epsilon = 0.02 * arcLength(contour, true);
    approxPolyDP(contour, approxCurve, epsilon, true);

    // ����������������ܳ�
    double area = contourArea(approxCurve);
    double perimeter = arcLength(approxCurve, true);

    // ����Բ��
    double circularity = (4 * CV_PI * area) / (perimeter * perimeter);

    return circularity;
}

Mat countCells(const Mat& image, std::vector<CellObject>& cellObjects) {
    // ����ͼ��Ԥ��������ҶȻ�����ֵ����
    Mat grayImage;
    cvtColor(image, grayImage, COLOR_BGR2GRAY);

    
    // Ӧ����ֵ�ָ������ɫ����������
    Mat thresholdImage;
    //threshold(grayImage, thresholdImage, 200, 255, THRESH_BINARY);
    threshold(grayImage, thresholdImage, 187, 255, THRESH_BINARY);

    // ִ�аߵ���
    std::vector<std::vector<Point>> contours;
    findContours(thresholdImage, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    // ��ͼ���ϻ���ϸ������������Բ�Ⱥ����
    Mat resultImage = image.clone();
    int cellCount = 0;

    for (const auto& contour : contours) {
        // ����ϸ�����������
        int cellArea = static_cast<int>(contourArea(contour));

        // ���ϸ�����Ϊ0����������ϸ��
        if (cellArea == 0) {
            continue;
        }

        // ����ϸ������
        polylines(resultImage, { contour }, true, Scalar(0, 255, 0), 2);

        // ����Բ��
        double circularity = calculateCircularity(contour);

        // ����ϸ����ֱ����ʹ�ü򻯵ļ��㷽���������ο���
        float cellDiameter = sqrt(4 * cellArea / CV_PI);

        // ����ϸ������С��Ӿ���
        RotatedRect boundingRect = minAreaRect(contour);

        // ��ȡ���εĶ���
        Point2f rectPoints[4];
        boundingRect.points(rectPoints);

        // ����������ת��Ϊ��������
        Point rectIntPoints[4];
        for (int i = 0; i < 4; i++) {
            rectIntPoints[i] = Point(static_cast<int>(rectPoints[i].x), static_cast<int>(rectPoints[i].y));
        }

        // ��ȡ�������Ͻǵ�����Ϳ�ȡ��߶�
        int rectX = static_cast<int>(boundingRect.boundingRect().x);
        int rectY = static_cast<int>(boundingRect.boundingRect().y);
        int rectWidth = static_cast<int>(boundingRect.boundingRect().width);
        int rectHeight = static_cast<int>(boundingRect.boundingRect().height);

        // ��ͼ���ϻ�����С��Ӿ���
        for (int i = 0; i < 4; i++) {
            line(resultImage, rectPoints[i], rectPoints[(i + 1) % 4], Scalar(0, 0, 255), 2);
        }

        // ��ͼ���ϱ�Ǿ���λ�úͳߴ���Ϣ
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

        // ����ϸ�����������
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

    // ����ͼ��ߴ�����Ӧ��ʾ��
    double scaleFactor = 0.4;  // ��������
    Mat resizedImage;
    resize(resultImage, resizedImage, Size(), scaleFactor, scaleFactor);

    // ���������ͼ��
    imshow("Result Image", resizedImage);
    waitKey(0);

    return resultImage;
}

void saveCellDataToExcel(const std::string& filePath, const std::vector<CellObject>& cellObjects) {
    // ��������CSV�ļ�
    std::ofstream file(filePath);

    // д���ͷ
    file << "Cell Index,Area,Diameter,Circularity,Rect X,Rect Y,Rect Width,Rect Height\n";

    // ���д��ϸ������
    for (int i = 0; i < cellObjects.size(); i++) {
        const auto& cell = cellObjects[i];
        file << i + 1 << "," << cell.area << "," << cell.diameter << ","
            << cell.circularity << "," << cell.x << "," << cell.y << "," 
            << cell.width << "," << cell.height << "\n";
    }

    // �ر��ļ�
    file.close();

    std::cout << "ϸ�������ѱ��浽�ļ���" << filePath << std::endl;
}

int main() {
    // ��ȡͼ��
    //std::string filePath = "D:\\Study\\ECNU-Proj\\Cells\\bf\\Image_20230605151036919.bmp";
    //std::string filePath = "D:\\Study\\ECNU-Proj\\Cells\\bf\\Image_20230605151139255.bmp";
    std::string filePath = "D:\\Study\\ECNU-Proj\\Cells\\bf\\Image_20230605163213362.bmp";
    Mat image = imread(filePath, IMREAD_COLOR);

    // ���ͼ���Ƿ�ɹ�����
    if (image.empty()) {
        std::cout << "�޷���ȡͼ���ļ�" << std::endl;
        return -1;
    }

    // ���ͼ��ߴ�
    if (image.cols <= 0 || image.rows <= 0) {
        std::cout << "ͼ��ߴ���Ч" << std::endl;
        return -1;
    }

    std::vector<CellObject> cellObjects;
    Mat outputImage = countCells(image, cellObjects);

    // ���洦����Ĵ��б�ע��ͼ��
    uchar* outputImg = outputImage.data;

    std::cout << "ϸ������: " << cellObjects.size() << std::endl;

    // ���ϸ�����������
    std::cout << "ϸ�����: ";
    for (const auto& cell : cellObjects) {
        std::cout << cell.area << " ";
    }
    std::cout << std::endl;

    std::cout << "ϸ��ֱ��: ";
    for (const auto& cell : cellObjects) {
        std::cout << cell.diameter << " ";
    }
    std::cout << std::endl;

    std::cout << "ϸ��Բ��: ";
    for (const auto& cell : cellObjects) {
        std::cout << cell.circularity << " ";
    }
    std::cout << std::endl;

    // ��ϸ�����ݱ��浽Excel�ļ���
    std::string excelFilePath = "cell_data.csv";
    saveCellDataToExcel(excelFilePath, cellObjects);

    return 0;
}
