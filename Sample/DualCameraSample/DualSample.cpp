#include <stdio.h>
#include "DualCamera.h"

using cv::Scalar;
using cv::Point;

bool InitDevice();
void ShowMenu(void);
void ShowInfo(const VzPeopleInfoCount_& peopleInfoCount);

enum DeviceState
{
    ST_HotPlugIn = -2,
    ST_HotPlugOut = -1,
    ST_None = 0,
    ST_Opened = 1,
    ST_Upgraded = 2,
};

bool g_isRunning = true;
DeviceState g_deviceState = ST_None;

DualCamera g_DualCamra;
bool g_bShowImg = true;
bool isSavingImg = false;

static int gColorIndex = 0;

Scalar gColor[]=
{
    Scalar(255, 0, 0),
    Scalar(0, 255, 0),
    Scalar(0, 0, 255),

    Scalar(255, 255, 0),
    Scalar(255, 0, 255),
    Scalar(0, 255, 255),
};

struct headPointVByID
{
    uint32_t id;
    bool isUpdated;
    Scalar color;
    std::vector<Point> headPointV;
};

std::vector<headPointVByID> g_headPointIDVec;

int main(int argc, char *argv[])
{

OPEN:
    g_isRunning = InitDevice();
    if (false == g_isRunning)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        goto OPEN;
    }

    VzPeopleInfoCount_ peopleInfoCount = { 0 };
    cv::Mat imageMat;
    while (g_isRunning)
    {
        if (ST_Opened == g_deviceState)
        {
            peopleInfoCount = { 0 };
            
            if (true == g_DualCamra.GetPeopleInfoCount(peopleInfoCount))
            {
                ShowInfo(peopleInfoCount);
            }
        }

        unsigned char key = cv::waitKey(1);
        switch (key)
        {
        case 'S':
        case 's':
        {
            //Turn image display on and off
            g_bShowImg = !g_bShowImg;
            g_DualCamra.SetShowImg(g_bShowImg);
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
            g_DualCamra.SetSaveOfflineDataState(isSavingImg);
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

    g_DualCamra.CloseDevice();
    cout << "Vz_PCCloseDevice" << endl;

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
    bool ret = false;
    if (true == g_DualCamra.OpenDevice())
    {
        g_deviceState = ST_Opened;
        g_DualCamra.SetShowImg(g_bShowImg);

        ShowMenu();
        ret = true;
    }

    return ret;
}

void ShowInfo(const VzPeopleInfoCount_& peopleInfoCount)
{
    static const int BUFLEN = 50;
    char temp[BUFLEN] = { 0 };

    if (0 != peopleInfoCount.entrance.frame.height && 0 != peopleInfoCount.entrance.frame.width)
    {
        cv::Mat entranceImg = cv::Mat(peopleInfoCount.entrance.frame.height, peopleInfoCount.entrance.frame.width, CV_16UC1, peopleInfoCount.entrance.frame.pFrameData);

		entranceImg.convertTo(entranceImg, CV_8U, 255.0 / 2800);
        applyColorMap(entranceImg, entranceImg, cv::COLORMAP_RAINBOW);

        if (true == isSavingImg)
        {
            static const int BUFLEN = 50;
            char temp[BUFLEN] = "-REC";
            cv::putText(entranceImg, temp,
                cv::Point(0, 20),
                cv::FONT_HERSHEY_SIMPLEX,
                0.6,
                Scalar(0, 0, 0),
                2,
                9);
        }

        for (int i = 0; i < peopleInfoCount.entrance.validPeopleCount; i++)
        {
			Point headpoint = Point(peopleInfoCount.entrance.peopleInfo[i].pixelPostion.x, peopleInfoCount.entrance.peopleInfo[i].pixelPostion.y);
            cv::circle(entranceImg, headpoint, 11, Scalar(255, 255, 255), -1, 8);
        }

        cv::imshow("Entrance", entranceImg);
    }

    if (0 != peopleInfoCount.exit.frame.height && 0 != peopleInfoCount.exit.frame.width)
	{
        cv::Mat exitImg = cv::Mat(peopleInfoCount.exit.frame.height, peopleInfoCount.exit.frame.width, CV_16UC1, peopleInfoCount.exit.frame.pFrameData);
		exitImg.convertTo(exitImg, CV_8U, 255.0 / 2800);
		applyColorMap(exitImg, exitImg, cv::COLORMAP_RAINBOW);

		if (true == isSavingImg)
		{
			static const int BUFLEN = 50;
			char temp[BUFLEN] = "-REC";
			cv::putText(exitImg, temp,
				cv::Point(0, 20),
				cv::FONT_HERSHEY_SIMPLEX,
				0.6,
				Scalar(0, 0, 0),
				2,
				9);
		}

		for (int i = 0; i < peopleInfoCount.exit.validPeopleCount; i++)
		{
			Point headpoint = Point(peopleInfoCount.exit.peopleInfo[i].pixelPostion.x, peopleInfoCount.exit.peopleInfo[i].pixelPostion.y);
            cv::circle(exitImg, headpoint, 11, Scalar(255, 255, 255), -1, 8);
		}

		cv::imshow("Exit", exitImg);
	}


	cv::Mat Img = cv::Mat::zeros(IMG_H * 3 / 4, IMG_W * 3 / 2, CV_8UC3);
    cv::line(Img, cv::Point(IMG_W * 3 / 8, 0), cv::Point(IMG_W * 3 / 8, IMG_H * 3 / 4) , Scalar(255, 0, 0), 3);
    snprintf(temp, BUFLEN, "%s", "IN:");
    cv::putText(Img, temp,
        cv::Point(IMG_W * 3 / 8, 20),
        cv::FONT_HERSHEY_SIMPLEX,
        0.6,
        Scalar(255, 255, 0),
        1,
        9);

    memset(temp, BUFLEN, 0);
    snprintf(temp, BUFLEN, "%d", peopleInfoCount.fusion.inCount);
    cv::putText(Img, temp,
        cv::Point(IMG_W * 3 / 8 + 30, 20),
        cv::FONT_HERSHEY_SIMPLEX,
        0.6,
        Scalar(255, 255, 0),
        1,
        9);

    cv::line(Img, cv::Point(IMG_W * 9 / 8, 0), cv::Point(IMG_W * 9 / 8, IMG_H * 3 / 4), Scalar(255, 0, 0), 3);
    memset(temp, BUFLEN, 0);
    snprintf(temp, BUFLEN, "%s", "OUT:");
    cv::putText(Img, temp,
        cv::Point(IMG_W * 9 / 8, 20),
        cv::FONT_HERSHEY_SIMPLEX,
        0.6,
        Scalar(255, 255, 0),
        1,
        9);
    memset(temp, BUFLEN, 0);
    snprintf(temp, BUFLEN, "%d", peopleInfoCount.fusion.outCount);
    cv::putText(Img, temp,
        cv::Point(IMG_W * 9 / 8 + 45, 20),
        cv::FONT_HERSHEY_SIMPLEX,
        0.6,
        Scalar(255, 255, 0),
        1,
        9);

    for (int i = 0; i < peopleInfoCount.fusion.validPeopleCount; i++)
    {
        cv::Point headpoint = Point(peopleInfoCount.fusion.peopleInfo[i].pixelPostion.x * 3 / 4, peopleInfoCount.fusion.peopleInfo[i].pixelPostion.y * 3 / 4);

        snprintf(temp, BUFLEN, "%X ", peopleInfoCount.fusion.peopleInfo[i].id);
        cv::putText(Img, temp,
            cv::Point(headpoint.x + 12, headpoint.y - 15),
            cv::FONT_HERSHEY_SIMPLEX,
            0.6,
            Scalar(0, 255, 0),
            1,
            9);

        vector<headPointVByID>::iterator it = g_headPointIDVec.begin();
        for (; it != g_headPointIDVec.end(); it++)
        {
            if (peopleInfoCount.fusion.peopleInfo[i].id == it->id )
            {
                break;
            }
        }
        
        if (it == g_headPointIDVec.end())
        {
            g_headPointIDVec.push_back({ peopleInfoCount.fusion.peopleInfo[i].id, true, gColor[gColorIndex++%6], vector<Point>{headpoint}});
            it = g_headPointIDVec.end() - 1;
        }
        else
        {
            it->headPointV.push_back(headpoint);
            it->isUpdated = true;
        }

        for (vector<Point>::iterator iPoint = it->headPointV.begin() + 1;  iPoint != it->headPointV.end(); iPoint++)
        {
            cv::line(Img, *iPoint, *(iPoint - 1), it->color, 2);
        }

        cv::circle(Img, headpoint, 11, it->color, -1, 8);
    }

    for (vector<headPointVByID>::iterator it = g_headPointIDVec.begin(); it != g_headPointIDVec.end(); )
    {
        if (true == it->isUpdated)
        {
            it->isUpdated = false;
            it++;
        }
        else
        {
            it = g_headPointIDVec.erase(it);
        }
    }


	cv::imshow("Info", Img);
}