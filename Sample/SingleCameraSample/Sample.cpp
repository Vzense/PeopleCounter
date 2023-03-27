#include <iostream>
#include <opencv2/opencv.hpp>
#include <thread>
#include <stdio.h>
#include "Vz_API.h"

using namespace std;
using namespace cv;

static const int IMG_W = 640;
static const int IMG_H = 480;

bool InitDevice();
void ShowMenu(void);
void ShowInfo(const VzPeopleInfoCount& peopleInfoCount);

enum DeviceState
{
    ST_None = 0,
    ST_Opened = 1,
};

bool g_isRunning = true;
DeviceState g_deviceState = ST_None;
bool g_showPeopleInfo = false;
PeopleCountDeviceHandler g_deviceHandle = 0;
bool g_bShowImg = true;
static bool isSavingImg = false;

int main(int argc, char *argv[])
{
    Vz_PCInitialize();
    cout << "Vz_Initialize" << endl;

OPEN:
    g_isRunning = InitDevice();
    if (false == g_isRunning)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        goto OPEN;
    }

    VzPeopleInfoCount peopleInfoCount = { 0 };
    while (g_isRunning)
    {

        if (ST_Opened == g_deviceState)
        {
            peopleInfoCount = { 0 };

            VzReturnStatus result = Vz_PCGetPeopleInfoCount(g_deviceHandle ,&peopleInfoCount);
            if (VzReturnStatus::VzRetOK == result)
            {
                ShowInfo(peopleInfoCount);
            }
            else if (VzReturnStatus::VzRetCalibrateFailed == result)
            {
                cout << "Calibration is failed.Please clear the obstacle in the detection environment." << result << endl;
            }
            else
            {
                cout << "Vz_PCGetPeopleInfoCount error:" << result << endl;
            }
        }

        unsigned char key = waitKey(1);
        switch (key)
        {
        case 'S':
        case 's':
        {
            //Turn image display on and off
            g_bShowImg = !g_bShowImg;
            Vz_PCSetShowImg(g_deviceHandle, g_bShowImg);
            cout << ((true == g_bShowImg) ? "start" : "stop") << "show img." << endl;
        }
        break;
        case 'M':
        case 'm':
        {
            //Show menu
            ShowMenu();
        }
        break;
        case 'R':
        case 'r':
        {
            //Save Img
            
            isSavingImg = !isSavingImg;
            Vz_PCSetSaveOfflineDataState(g_deviceHandle, isSavingImg);
        }
        break;
        case 27: //ESC
            g_isRunning = false;
            break;
        default:
            break;
        }
    }

    cv::destroyAllWindows();

    Vz_PCCloseDevice(&g_deviceHandle);
    cout << "Vz_PCCloseDevice" << endl;
    Vz_PCShutdown();
    cout << "Vz_Shutdown" << endl;
    return 0;
}

void ShowMenu(void)
{
    cout << "Press following key to set corresponding feature:" << endl;
    cout << "S/s: Turn image display on and off" << endl;
    cout << "R/r: Save image once" << endl;
    cout << "M/m: Show menu" << endl;
    return;
}

bool InitDevice()
{
    VzReturnStatus status = Vz_PCOpenDevice("", &g_deviceHandle);
    if (status != VzReturnStatus::VzRetOK)
    {
        if (VzReturnStatus::VzRetNoDeviceConnected == status)
        {
            cout << "Please connect the device first!" << endl;
        }
        else
        {
            cout << "Vz_PCOpenDevice failed: " << status << endl;
        }
        return false;
    }
    Vz_PCSetShowImg(g_deviceHandle, g_bShowImg);
    ShowMenu();
    g_deviceState = ST_Opened;

    return true;
}

void ShowInfo(const VzPeopleInfoCount& peopleInfoCount)
{
    static const int BUFLEN = 50;
    char temp[BUFLEN] = { 0 };

    if (0 != peopleInfoCount.frame.height && 0 != peopleInfoCount.frame.width)
    {
        cv::Mat img = cv::Mat(peopleInfoCount.frame.height, peopleInfoCount.frame.width, CV_16UC1, peopleInfoCount.frame.pFrameData);

        img.convertTo(img, CV_8U, 255.0 / 2800);
        applyColorMap(img, img, cv::COLORMAP_RAINBOW);

        if (true == isSavingImg)
        {
            static const int BUFLEN = 50;
            char temp[BUFLEN] = "-REC";
            cv::putText(img, temp,
                cv::Point(0, 20),
                cv::FONT_HERSHEY_SIMPLEX,
                0.6,
                Scalar(0, 0, 0),
                2,
                9);
        }

        cv::line(img, cv::Point(IMG_W / 2, 0), cv::Point(IMG_W / 2, IMG_H) , Scalar(255, 255, 255), 2);
        snprintf(temp, BUFLEN, "%s", "IN:");
        cv::putText(img, temp,
        cv::Point(IMG_W / 2 + 10, 20),
        cv::FONT_HERSHEY_SIMPLEX,
        0.6,
        Scalar(255, 255, 0),
        1,
        9);

        memset(temp, BUFLEN, 0);
        snprintf(temp, BUFLEN, "%d", peopleInfoCount.inCount);
        cv::putText(img, temp,
        cv::Point(IMG_W / 2 + 40, 20),
        cv::FONT_HERSHEY_SIMPLEX,
        0.6,
        Scalar(255, 255, 0),
        1,
        9);

        memset(temp, BUFLEN, 0);
        snprintf(temp, BUFLEN, "%s", "OUT:");
        cv::putText(img, temp,
        cv::Point(IMG_W / 2 - 80, 20),
        cv::FONT_HERSHEY_SIMPLEX,
        0.6,
        Scalar(255, 255, 0),
        1,
        9);
        memset(temp, BUFLEN, 0);
        snprintf(temp, BUFLEN, "%d", peopleInfoCount.outCount);
        cv::putText(img, temp,
        cv::Point(IMG_W / 2 -40, 20),
        cv::FONT_HERSHEY_SIMPLEX,
        0.6,
        Scalar(255, 255, 0),
        1,
        9);

        for (int i = 0; i < peopleInfoCount.validPeopleCount; i++)
        {
			Point headpoint = Point(peopleInfoCount.peopleInfo[i].pixelPostion.x, peopleInfoCount.peopleInfo[i].pixelPostion.y);

            Scalar colorText = Scalar(0, 0, 255);
            Scalar colorCircle = Scalar(255, 255, 255);

            memset(temp, BUFLEN, 0);
            snprintf(temp, BUFLEN, "ID:%X ", peopleInfoCount.peopleInfo[i].id);
            cv::putText(img, temp,
                cv::Point(headpoint.x + 12, headpoint.y - 15),
                cv::FONT_HERSHEY_SIMPLEX,
                0.6,
                colorText,
                1,
                9);

            memset(temp, BUFLEN, 0);
            snprintf(temp, BUFLEN, "H:%d", peopleInfoCount.peopleInfo[i].worldPostion.z);

            cv::putText(img, temp,
                cv::Point(headpoint.x + 12, headpoint.y - 35),
                cv::FONT_HERSHEY_SIMPLEX,
                0.6,
                colorText,
                1,
                9);

            cv::circle(img, headpoint, 11, colorCircle, -1, 8);
            cv::rectangle(img, cv::Point(peopleInfoCount.peopleInfo[i].topleft.x, peopleInfoCount.peopleInfo[i].topleft.y),
                cv::Point(peopleInfoCount.peopleInfo[i].bottomright.x, peopleInfoCount.peopleInfo[i].bottomright.y), colorCircle, 2);
        }
        
        cv::imshow("img", img);
    }
}