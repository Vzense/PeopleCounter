#pragma once

#include <condition_variable>
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <opencv2/opencv.hpp>

#include "Vz_API.h"

static const int IMG_W = 640;
static const int IMG_H = 480;

typedef struct
{
	VzPeopleInfoCount entrance;
	VzPeopleInfoCount exit;
	VzPeopleInfoCount fusion;
} VzPeopleInfoCount_;

struct VzPeopleInfoAndCount {
	VzPeopleInfo people;
	uint16_t continueCount;
	uint16_t missCount;
	uint16_t inOutLatestState;
	uint16_t continueStateFrameCount;
};

struct ThreadFuncParams;

using std::thread;
using std::cout;
using std::endl;
using std::vector;
using std::unique_lock;
using std::mutex;
using std::condition_variable;
using cv::Mat;


class DualCamera {
public:
	DualCamera();
	~DualCamera();
	bool OpenDevice();
	bool CloseDevice();
	bool GetPeopleInfoCount(VzPeopleInfoCount_& peopleInfoCount);
	bool SetShowImg(bool isShow);
	bool SetSaveOfflineDataState(bool isSavingImg);
	void ThreadFunc(const PeopleCountDeviceHandler device);
	void UpdataPeopleInfoCallback(const VzPeopleInfoCount& peopleInfoCount, const PeopleCountDeviceHandler peopleCount);

private:
	
	bool IsSync(const VzPeopleInfoCount& src, const VzPeopleInfoCount& dest);
	void FusionPeopleInfo(const VzPeopleInfoCount& entrance, const VzPeopleInfoCount& exit, vector<VzPeopleInfo>& fusion);
	void UpdataContinueInfo(vector<VzPeopleInfoAndCount>& continuePeopleInfoV, vector<VzPeopleInfo> newPeopleInfoV);
	uint16_t GetMaxLostCount(const cv::Point2i& latestValidPos);
	uint32_t RequestID();
	

private:
	PeopleCountDeviceHandler mEntrance;
	PeopleCountDeviceHandler mExit;

	thread mThdEntrance;
	thread mThdExit;

	condition_variable mCd;
	mutex mMutex;
	
	bool mIsShowImg;
	bool mEntranceIsUpdate;
	bool mExitIsUpdate;
	volatile bool mIsRuning;

	vector<VzPeopleInfoAndCount> mContinuePeopleV;

	VzPeopleInfoCount mEntranceInfo;
	VzPeopleInfoCount mExitInfo;

	uint8_t* mEntranceImgBuf;
	uint8_t* mExitImgBuf;
    int mMoveConfidence;
};