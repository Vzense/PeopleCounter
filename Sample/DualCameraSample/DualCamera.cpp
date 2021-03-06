#include <sys/timeb.h>
#include <time.h>

#include "DualCamera.h"
#include "JsonConfig.h"

static const int DISTANCEOFDEVICE = 1200;
static const int HALFDISTANCEOFDEVICE = 1200 / 2;

int GetMoveConfidenceFromConfig();

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
    mExitImgBuf(new uint8_t[IMG_W * IMG_H * 2]),
    mMoveConfidence(GetMoveConfidenceFromConfig())
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
    VzReturnStatus status = Vz_PCOpenDevice("./Alg_PCConfig_Entrance.json", &mEntrance);
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

    status = Vz_PCOpenDevice("./Alg_PCConfig_Exit.json", &mExit);
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

    mThdEntrance = thread(&DualCamera::ThreadFunc, this, mEntrance);

    mThdExit = thread(&DualCamera::ThreadFunc, this, mExit);

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

    peopleInfoCount.fusion.validPeopleCount = 0;
    memset(peopleInfoCount.fusion.peopleInfo, 0, sizeof(peopleInfoCount.fusion.peopleInfo));
    for (vector<VzPeopleInfoAndCount>::const_iterator i = mContinuePeopleV.cbegin(); i != mContinuePeopleV.cend() && (peopleInfoCount.fusion.validPeopleCount < (sizeof(peopleInfoCount.fusion.peopleInfo)/sizeof(peopleInfoCount.fusion.peopleInfo[0]))); i++)
    {
        if (i->people.confidence > mMoveConfidence)
        {
            peopleInfoCount.fusion.peopleInfo[peopleInfoCount.fusion.validPeopleCount++] = i->people;
        }
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

void DualCamera::ThreadFunc(const PeopleCountDeviceHandler device)
{
    VzPeopleInfoCount infoCount = { 0 };
    while (true == mIsRuning)
    {
        memset(&infoCount, 0, sizeof(infoCount));
        VzReturnStatus result = Vz_PCGetPeopleInfoCount(device, &infoCount);
        if (VzReturnStatus::VzRetOK == result)
        {
            UpdataPeopleInfoCallback(infoCount, device);
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

void DualCamera::FusionPeopleInfo(const VzPeopleInfoCount& entrance, const VzPeopleInfoCount& exit, vector<VzPeopleInfo>& fusion)
{
    vector<VzPeopleInfo> vectorEntranceToDo;
    vector<VzPeopleInfo> vectorExitToDo;

    static const int CriticalValue = IMG_W / 4;

    VzPeopleInfo info = {0};
    for (size_t i = 0; i < entrance.validPeopleCount; i++)
    {
        info = entrance.peopleInfo[i];
        info.worldPostion.x -= HALFDISTANCEOFDEVICE;

        if (info.pixelPostion.x < (IMG_W - CriticalValue))
        {
            fusion.push_back(info);
        }
        else
        {
            vectorEntranceToDo.push_back(info);
        }
    }

    for (size_t i = 0; i < exit.validPeopleCount; i++)
    {
        info = exit.peopleInfo[i];
        info.pixelPostion.x += IMG_W;
        info.worldPostion.x += HALFDISTANCEOFDEVICE;

        if (info.pixelPostion.x > (IMG_W + CriticalValue))
        {
            fusion.push_back(info);
        }
        else
        {
            vectorExitToDo.push_back(info);
        }
    }

    if (0 == vectorEntranceToDo.size() || 0 == vectorExitToDo.size())
    {
        for (vector<VzPeopleInfo>::iterator i = vectorEntranceToDo.begin(); i != vectorEntranceToDo.end(); i++)
        {
            fusion.push_back(*i);
        }

        for (vector<VzPeopleInfo>::iterator i = vectorExitToDo.begin(); i != vectorExitToDo.end(); i++)
        {
            fusion.push_back(*i);
        }
    }
    else
    {
        static const int FusionPeopleDistanceMax = 450;

        for (vector<VzPeopleInfo>::iterator i = vectorEntranceToDo.begin(); i != vectorEntranceToDo.end(); ++i)
        {
            bool isFoundfusionPeople = false;
            for (vector<VzPeopleInfo>::iterator j = vectorExitToDo.begin(); j != vectorExitToDo.end(); j++)
            {
                int distance = cv::norm(cv::Point2f(i->worldPostion.x, i->worldPostion.y) - cv::Point2f(j->worldPostion.x, j->worldPostion.y));
                if (FusionPeopleDistanceMax > distance)
                {
                    isFoundfusionPeople = true;
                    VzPeopleInfo* pNew = nullptr;

                    if (abs(i->worldPostion.z - j->worldPostion.z) < 50)
                    {
                        i->pixelPostion.x = (i->pixelPostion.x + j->pixelPostion.x) / 2;
                        i->pixelPostion.y = (i->pixelPostion.y + j->pixelPostion.y) / 2;
                        i->worldPostion.x = (i->worldPostion.x + j->worldPostion.x) / 2;
                        i->worldPostion.y = (i->worldPostion.y + j->worldPostion.y) / 2;
                        i->worldPostion.z = (i->worldPostion.z + j->worldPostion.z) / 2;
                        i->confidence = (i->confidence > j->confidence) ? i->confidence : j->confidence;
                        pNew = &(*i);
                    }
                    else
                    {
                        if (i->worldPostion.z > j->worldPostion.z)
                        {
                            pNew = &(*i);
                        }
                        else
                        {
                            pNew = &(*j);
                        }
                        pNew->confidence = (i->confidence > j->confidence) ? i->confidence : j->confidence;
                    }

                    fusion.push_back(*pNew);
                    vectorExitToDo.erase(j);
                    break;
                }
            }

            if (false == isFoundfusionPeople)
            {
                fusion.push_back(*i);
            }
        }

        for (vector<VzPeopleInfo>::iterator i = vectorExitToDo.begin(); i != vectorExitToDo.end(); i++)
        {
            fusion.push_back(*i);
        }
    }
}

void DualCamera::UpdataContinueInfo(vector<VzPeopleInfoAndCount>& continuePeopleInfoV, vector<VzPeopleInfo> newPeopleInfoV)
{
    static const int cotiunedDistanceMax = 450;

    for (vector<VzPeopleInfoAndCount>::iterator i = continuePeopleInfoV.begin(); i != continuePeopleInfoV.end(); )
    {
        int minDistance = INT_MAX;

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
            
            if (i->people.confidence < findIterator->confidence)
            {
                i->people.confidence = findIterator->confidence;
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

uint16_t DualCamera::GetMaxLostCount(const cv::Point2i& latestValidPos)
{
    uint16_t maxLostCount = 0;
    static const uint64_t BUFF = 40;

    if ((IMG_W - BUFF) < latestValidPos.x && latestValidPos.x < (IMG_W + BUFF))
    {
        maxLostCount = 15 * 3;
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

int GetMoveConfidenceFromConfig()
{
    const int defaultValue = 20;
    int ret = defaultValue;

    std::unique_ptr<JsonConfig> pConfig = std::make_unique<JsonConfig>();
    if (1 == pConfig->Read("./Alg_PCConfig_Entrance.json"))
    {
        string detectionAccuracy = pConfig->GetValue("ALG", "DetectionAccuracy", "normal");
        std::transform(detectionAccuracy.begin(), detectionAccuracy.end(), detectionAccuracy.begin(), ::tolower);
        if (string("high") == detectionAccuracy)
        {
            ret = defaultValue;
        }
        else if (string("low") == detectionAccuracy)
        {
            ret = 8;
        }
        else if (string("undefined") == detectionAccuracy)
        {
            ret = pConfig->GetInt("ALG", "MoveConfidence", defaultValue);;
        }
        else //normal
        {
            ret = 15;
        }
    }

    return ret;
}