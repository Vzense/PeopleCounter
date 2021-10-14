#include <sys/timeb.h>
#include <time.h>

#include "DualCamera.h"

static const int DISTANCEOFDEVICE = 1200;
static const int HALFDISTANCEOFDEVICE = 1200 / 2;

struct ThreadFuncParams {
    DualCamera& dual;
    PeopleCountDeviceHandler device;
    VzPeopleInfoCount& info;
};

DualCamera::DualCamera():
    mEntrance(nullptr),
    mExit(nullptr),
    mThdEntrance(),
    mThdExit(),
    mCd(),
    mMutex(),
    mIsShowImg(false),
    mEntranceIsUpdate(false),
    mExitIsUpdate(false),
    mIsRuning(false),
    mContinuePeopleV(),
    mEntranceInfo(),
    mExitInfo(),
    mEntranceImgBuf(new uint8_t[IMG_W * IMG_H * 2]),
    mExitImgBuf(new uint8_t[IMG_W * IMG_H * 2])
{
    Vz_PCInitialize();
}

DualCamera::~DualCamera()
{
    if (nullptr != mEntranceImgBuf)
    {
        delete[] mEntranceImgBuf;
        mEntranceImgBuf = nullptr;
    }
    if (nullptr != mExitImgBuf)
    {
        delete[] mExitImgBuf;
        mExitImgBuf = nullptr;
    }

    Vz_PCShutdown();
}

bool DualCamera::OpenDevice()
{
    VzReturnStatus status = Vz_PCOpenDevice("./Alg_PCConfig_Entrance.ini", &mEntrance);
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

    status = Vz_PCOpenDevice("./Alg_PCConfig_Exit.ini", &mExit);
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

    mIsRuning = true;
    ThreadFuncParams paramsEntrance = { *this ,mEntrance, mEntranceInfo };

    mThdEntrance = thread(ThreadFunc, paramsEntrance);

    ThreadFuncParams paramsExit = { *this ,mExit, mExitInfo };
    mThdExit = thread(ThreadFunc, paramsExit);

    Vz_PCSetShowImg(mEntrance, mIsShowImg);
    Vz_PCSetShowImg(mExit, mIsShowImg);

    return true;
}

bool DualCamera::CloseDevice()
{
    mIsRuning = false;
    mThdEntrance.join();
    mThdExit.join();

    Vz_PCCloseDevice(&mEntrance);
    Vz_PCCloseDevice(&mExit);

    return false;
}

bool DualCamera::GetPeopleInfoCount(VzPeopleInfoCount_& peopleInfoCount)
{
    unique_lock<mutex> locker(mMutex);

    if (true == mCd.wait_for(locker, std::chrono::milliseconds(1000 / 15), [=] { return (true == mEntranceIsUpdate) && (true == mExitIsUpdate); }))
    {
        mEntranceIsUpdate = false;
        mExitIsUpdate = false;

        peopleInfoCount.entrance = mEntranceInfo;
        peopleInfoCount.exit = mExitInfo;

        vector<VzPeopleInfo> vectorNew;
        FusionPeopleInfo(peopleInfoCount.entrance, peopleInfoCount.exit, vectorNew);

        UpdataContinueInfo(mContinuePeopleV, vectorNew);

    }
    else
    {
        cout << "mEntranceIsUpdate:" << mEntranceIsUpdate << ", mExitIsUpdate:" << mExitIsUpdate << endl;
    }

    for (vector<VzPeopleInfoAndCount>::const_iterator i = mContinuePeopleV.cbegin(); i != mContinuePeopleV.cend() && (i - mContinuePeopleV.cbegin() < 6); i++)
    {
        peopleInfoCount.fusion.validPeopleCount++;
        peopleInfoCount.fusion.peopleInfo[i - mContinuePeopleV.cbegin()] = i->people;
    }

    peopleInfoCount.fusion.inCount = peopleInfoCount.entrance.inCount;
    peopleInfoCount.fusion.outCount = peopleInfoCount.exit.outCount;


    return true;
}

bool DualCamera::SetShowImg(bool isShow)
{
    mIsShowImg = isShow;

    Vz_PCSetShowImg(mEntrance, mIsShowImg);
    Vz_PCSetShowImg(mExit, mIsShowImg);

    return true;
}

bool DualCamera::SetSaveOfflineDataState(bool isSavingImg)
{
    Vz_PCSetSaveOfflineDataState(mEntrance, isSavingImg);
    Vz_PCSetSaveOfflineDataState(mExit, isSavingImg);

    return false;
}

void DualCamera::ThreadFunc(ThreadFuncParams& params)
{
    VzPeopleInfoCount infoCount = { 0 };
    while (true == params.dual.mIsRuning)
    {
        memset(&infoCount, 0, sizeof(infoCount));
        VzReturnStatus result = Vz_PCGetPeopleInfoCount(params.device, &infoCount);
        if (VzReturnStatus::VzRetOK == result)
        {
            params.dual.UpdataPeopleInfoCallback(infoCount, params.device);
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
}

void DualCamera::UpdataPeopleInfoCallback(const VzPeopleInfoCount& peopleInfoCount, const PeopleCountDeviceHandler peopleCount)
{
    unique_lock<mutex> locker(mMutex);

    if (mEntrance == peopleCount)
    {
        if (true == mExitIsUpdate)
        {
            if (true == IsSync(mExitInfo, peopleInfoCount))
            {
                mEntranceInfo = peopleInfoCount;

                if (true == mIsShowImg)
                {
                    mEntranceInfo.frame.pFrameData = mEntranceImgBuf;
                    memcpy(mEntranceInfo.frame.pFrameData, peopleInfoCount.frame.pFrameData, peopleInfoCount.frame.dataLen);
                }

                mEntranceIsUpdate = true;
                locker.unlock();
                mCd.notify_one();
            }
            else
            {
                mExitIsUpdate = false;
            }
        }
        else
        {
            mEntranceInfo = peopleInfoCount;

            if (true == mIsShowImg)
            {
                mEntranceInfo.frame.pFrameData = mEntranceImgBuf;
                memcpy(mEntranceInfo.frame.pFrameData, peopleInfoCount.frame.pFrameData, peopleInfoCount.frame.dataLen);
            }

            mEntranceIsUpdate = true;
        }
    }
    else if (mExit == peopleCount)
    {
        if (true == mEntranceIsUpdate)
        {
            if (true == IsSync(mEntranceInfo, peopleInfoCount))
            {
                mExitInfo = peopleInfoCount;

                if (true == mIsShowImg)
                {
                    mExitInfo.frame.pFrameData = mExitImgBuf;
                    memcpy(mExitInfo.frame.pFrameData, peopleInfoCount.frame.pFrameData, peopleInfoCount.frame.dataLen);
                }
                mExitIsUpdate = true;
                locker.unlock();
                mCd.notify_one();
            }
            else
            {
                mEntranceIsUpdate = false;
            }
        }
        else
        {
            mExitInfo = peopleInfoCount;

            if (true == mIsShowImg)
            {
                mExitInfo.frame.pFrameData = mExitImgBuf;
                memcpy(mExitInfo.frame.pFrameData, peopleInfoCount.frame.pFrameData, peopleInfoCount.frame.dataLen);
            }
            mExitIsUpdate = true;
        }
    }
    else
    {
        cout << "peopleCount:" << peopleCount << "is invalid." << endl;
    }
}

bool DualCamera::IsSync(const VzPeopleInfoCount& src, const VzPeopleInfoCount& dest)
{
    bool ret = false;

    if ((src.frame.timestamp.tm_hour == dest.frame.timestamp.tm_hour)
        && (src.frame.timestamp.tm_min == dest.frame.timestamp.tm_min)
        && (src.frame.timestamp.tm_sec == dest.frame.timestamp.tm_sec))
    {
        ret = true;
    }

    return ret;
}

void DualCamera::FusionPeopleInfo(VzPeopleInfoCount entrance, VzPeopleInfoCount exit, vector<VzPeopleInfo>& fusion)
{
    vector<VzPeopleInfo*> vectorEntranceToDo;
    vector<VzPeopleInfo*> vectorExitToDo;

    const int entranceCritical = IMG_H;

    for (size_t i = 0; i < entrance.validPeopleCount; i++)
    {
        entrance.peopleInfo[i].worldPostion.x -= HALFDISTANCEOFDEVICE;

        if (entrance.peopleInfo[i].pixelPostion.x < entranceCritical)
        {
            fusion.push_back(entrance.peopleInfo[i]);
        }
        else
        {
            vectorEntranceToDo.push_back(&entrance.peopleInfo[i]);
        }
    }

    for (size_t i = 0; i < exit.validPeopleCount; i++)
    {
        exit.peopleInfo[i].pixelPostion.x += IMG_W;
        exit.peopleInfo[i].worldPostion.x += HALFDISTANCEOFDEVICE;

        if (exit.peopleInfo[i].pixelPostion.x > (IMG_W + entranceCritical))
        {
            fusion.push_back(exit.peopleInfo[i]);
        }
        else
        {
            vectorExitToDo.push_back(&exit.peopleInfo[i]);
        }
    }

    if (0 == vectorEntranceToDo.size() || 0 == vectorExitToDo.size())
    {
        for (vector<VzPeopleInfo*>::iterator i = vectorEntranceToDo.begin(); i != vectorEntranceToDo.end(); i++)
        {
            fusion.push_back(**i);
        }

        for (vector<VzPeopleInfo*>::iterator i = vectorExitToDo.begin(); i != vectorExitToDo.end(); i++)
        {
            fusion.push_back(**i);
        }
    }
    else
    {
        static const int fusionPeopleDistanceMax = 250;

        for (vector<VzPeopleInfo*>::iterator i = vectorEntranceToDo.begin(); i != vectorEntranceToDo.end(); ++i)
        {
            VzPeopleInfo* pNew = nullptr;
            for (vector<VzPeopleInfo*>::iterator j = vectorExitToDo.begin(); j != vectorExitToDo.end(); j++)
            {
                int distance = cv::norm(cv::Point2f((*i)->worldPostion.x, (*i)->worldPostion.y) - cv::Point2f((*j)->worldPostion.x, (*j)->worldPostion.y));

                if (fusionPeopleDistanceMax > distance)
                {
                    if (abs((*i)->worldPostion.z - (*j)->worldPostion.z) < 50)
                    {
                        (*i)->pixelPostion.x = ((*i)->pixelPostion.x + (*j)->pixelPostion.x) / 2;
                        (*i)->pixelPostion.y = ((*i)->pixelPostion.y + (*j)->pixelPostion.y) / 2;
                        (*i)->worldPostion.x = ((*i)->worldPostion.x + (*j)->worldPostion.x) / 2;
                        (*i)->worldPostion.y = ((*i)->worldPostion.y + (*j)->worldPostion.y) / 2;
                        (*i)->worldPostion.z = ((*i)->worldPostion.z + (*j)->worldPostion.z) / 2;

                        pNew = *i;
                    }
                    else
                    {
                        if ((*i)->worldPostion.z < (*j)->worldPostion.z)
                        {
                            pNew = *i;
                        }
                        else
                        {
                            pNew = *j;
                        }
                    }

                    vectorExitToDo.erase(j);
                    break;
                }
            }

            if (nullptr == pNew)
            {
                pNew = *i;
            }

            fusion.push_back(*pNew);
        }

        for (vector<VzPeopleInfo*>::iterator i = vectorExitToDo.begin(); i != vectorExitToDo.end(); i++)
        {
            fusion.push_back(**i);
        }
    }
}

void DualCamera::UpdataContinueInfo(vector<VzPeopleInfoAndCount>& continuePeopleInfoV, vector<VzPeopleInfo> newPeopleInfoV)
{
    static const int cotiunedDistanceMax = 300;

    for (vector<VzPeopleInfoAndCount>::iterator i = continuePeopleInfoV.begin(); i != continuePeopleInfoV.end(); )
    {
        int minDistance = 0x0FFFFFFF;

        vector<VzPeopleInfo>::iterator findIterator = newPeopleInfoV.end();
        for (vector<VzPeopleInfo>::iterator j = newPeopleInfoV.begin(); j != newPeopleInfoV.end(); j++)
        {
            int distance = cv::norm(cv::Point(i->people.worldPostion.x, i->people.worldPostion.y) - cv::Point(j->worldPostion.x, j->worldPostion.y));
            if (minDistance > distance)
            {
                minDistance = distance;
                findIterator = j;
            }
        }

        if (minDistance < cotiunedDistanceMax)
        {
            if (i->continueCount < 15)
            {
                i->continueCount++;
                i->missCount = 0;
            }
            
            i->people.pixelPostion.x = (i->people.pixelPostion.x + findIterator->pixelPostion.x) / 2;
            i->people.pixelPostion.y = (i->people.pixelPostion.y + findIterator->pixelPostion.y) / 2;
            i->people.worldPostion.x = (i->people.worldPostion.x + findIterator->worldPostion.x) / 2;
            i->people.worldPostion.y = (i->people.worldPostion.y + findIterator->worldPostion.y) / 2;
            i->people.worldPostion.z = (i->people.worldPostion.z + findIterator->worldPostion.z) / 2;

            newPeopleInfoV.erase(findIterator);
            i++;
        }
        else
        {
            if (0 != i->continueCount)
            {
                i->continueCount--;
                if (0 == i->continueCount)
                {
                    i->missCount = GetMaxLostCount(cv::Point(i->people.pixelPostion.x, i->people.pixelPostion.y));
                    if (0 == i->missCount)
                    {
                        i = continuePeopleInfoV.erase(i);
                    }
                    else
                    {
                        i->missCount--;
                        i++;
                    }
                }
                else
                {
                    i++;
                }
            }
            else
            {
                i->missCount--;
                if (0 == i->missCount)
                {
                    i = continuePeopleInfoV.erase(i);
                }
                else
                {
                    i++;
                }
            }

        }
    }

    for (vector<VzPeopleInfo>::iterator i = newPeopleInfoV.begin(); i != newPeopleInfoV.end(); i++)
    {
        VzPeopleInfo peopleInfo = *i;

        static const uint64_t BUFF = 40;
        if ((IMG_W - BUFF) < peopleInfo.pixelPostion.x && peopleInfo.pixelPostion.x < (IMG_W + BUFF))
        {
            continue;
        }
        else
        {
            peopleInfo.id = RequestID();
            continuePeopleInfoV.push_back({ peopleInfo , 1, 0 });
        }

    }
}

uint16_t DualCamera::GetMaxLostCount(cv::Point2i& latestValidPos)
{
    uint16_t maxLostCount = 0;
    static const uint64_t BUFF = 40;

    if ((IMG_W - BUFF) < latestValidPos.x && latestValidPos.x < (IMG_W + BUFF))
    {
        maxLostCount = 15 * 2;
    }

    return maxLostCount;
}

uint32_t DualCamera::RequestID()
{
    static uint32_t id = 0;

    time_t t = time(0);
    struct timeb stTimeb;
    ftime(&stTimeb);

    struct tm ptm;
#ifdef _WIN32
    localtime_s(&ptm, &t);
#elif defined Linux
    localtime_r(&t, &ptm);
#endif

    uint32_t ret = id++;

    return ret;
}
