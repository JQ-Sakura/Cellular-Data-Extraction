#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <fstream>

using namespace cv;

struct CellObject {
    int area;           // ���
    float diameter;     // ֱ��
    double circularity; // Բ��
    double fluorescence; // ӫ���
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

void countCells(const Mat& image, std::vector<CellObject>& cellObjects) {
    // ����ͼ��Ԥ��������ҶȻ�����ֵ����
    Mat grayImage;
    cvtColor(image, grayImage, COLOR_BGR2GRAY);
    threshold(grayImage, grayImage, 0, 255, THRESH_BINARY | THRESH_OTSU);

    // ִ�аߵ���
    std::vector<std::vector<Point>> contours;
    findContours(grayImage, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    // ��ͼ���ϻ���ϸ������������Բ�ȡ������ӫ���
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
        polylines(resultImage, { contour }, true, Scalar(0, 0, 255), 2);

        // ����Բ��
        double circularity = calculateCircularity(contour);

        // ����ϸ����ֱ����ʹ�ü򻯵ļ��㷽���������ο���
        float cellDiameter = sqrt(4 * cellArea / CV_PI);

        // ����ϸ����ӫ��ȣ���grayImage�м���ƽ��ֵ��
        double fluorescence = 0.0;
        if (!contour.empty()) {
            Rect boundingRect = cv::boundingRect(contour);
            if (boundingRect.x >= 0 && boundingRect.y >= 0 && boundingRect.x + boundingRect.width <= grayImage.cols && boundingRect.y + boundingRect.height <= grayImage.rows) {
                Mat cellRegion = grayImage(boundingRect);
                Scalar meanColor = mean(cellRegion);
                fluorescence = meanColor[0]; // Gͨ��
            }
        }

        // ��ͼ���ϱ��ϸ����š������Բ�Ⱥ�ӫ�����Ϣ
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

        // ����ϸ�����������
        CellObject cellObject;
        cellObject.area = cellArea;
        cellObject.diameter = cellDiameter;
        cellObject.circularity = circularity;
        cellObject.fluorescence = fluorescence;
        cellObjects.push_back(cellObject);

        cellCount++;
    }

    // ����ͼ��ߴ�����Ӧ��ʾ��
    double scaleFactor = 0.5;  // ��������
    Mat resizedImage;
    resize(resultImage, resizedImage, Size(), scaleFactor, scaleFactor);

    // ���������ͼ��
    imshow("Result Image", resizedImage);
    waitKey(0);
}


void saveCellDataToExcel(const std::string& filePath, const std::vector<CellObject>& cellObjects) {
    // ��������CSV�ļ�
    std::ofstream file(filePath);

    // д���ͷ
    file << "Cell Index,Area,Diameter,Circularity,Fluorescence\n";

    // ���д��ϸ������
    for (int i = 0; i < cellObjects.size(); i++) {
        const auto& cell = cellObjects[i];
        file << i + 1 << "," << cell.area << "," << cell.diameter << "," << cell.circularity << "," << cell.fluorescence << "\n";
    }

    // �ر��ļ�
    file.close();

    std::cout << "ϸ�������ѱ��浽�ļ���" << filePath << std::endl;
}


int main() {
    // ��ȡͼ��
    std::string filePath = "D:\\Study\\ECNU-Proj\\Cells\\Pics\\Image_20230607092503048.bmp";  //dark field black example
    //std::string filePath = "D:\\Study\\ECNU-Proj\\Cells\\Pics\\Image_20230607092455929.bmp";  //dark field green example
    //std::string filePath = "D:\\Study\\ECNU-Proj\\Cells\\Pics\\Image_20230607092533944.bmp";  //dark field red example
    //std::string filePath = "D:\\Study\\ECNU-Proj\\Cells\\Pics\\Image_20230607092817957.bmp";  //bright field white example
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
    countCells(image, cellObjects);

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

    // ��ϸ�����ݱ��浽Excel�ļ���
    std::string excelFilePath = "cell_data.csv";
    saveCellDataToExcel(excelFilePath, cellObjects);

    return 0;
}
