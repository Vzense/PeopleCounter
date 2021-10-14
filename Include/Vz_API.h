#ifndef VPEOPLECOUNTAPI_H
#define VPEOPLECOUNTAPI_H

#include "Vztypes.h"

/**
* @file Vz_API.h
* @brief Vzense API header file.
* Copyright (c) 2020-2021 Vzense Interactive, Inc.
*/

/*! \mainpage Vzense API Documentation
*
* \section Introduction
*
* Welcome to the Vzense API documentation. This documentation enables you to quickly get started in your development efforts to programmatically interact with People Counter.
*/

/**
* @brief 		Initializes the API on the device. This function must be invoked before any other Vzense APIs.
* @return		::PsRetOK if the function succeeded, or one of the error values defined by ::VzReturnStatus.
*/
VZENSE_C_API_EXPORT VzReturnStatus Vz_PCInitialize();

/**
* @brief 		Shuts down the API on the device and clears all resources allocated by the API. After invoking this function, no other Vzense APIs can be invoked.
* @return		::PsRetOK	if the function succeeded, or one of the error values defined by ::VzReturnStatus.
*/
VZENSE_C_API_EXPORT VzReturnStatus Vz_PCShutdown(void);

/**
* @brief 		Opens device.   The device must be subsequently closed using Vz_CloseDevice().
* @param[in]	pConfigPath:    The path of the camera configuration file
* @param[out]	pDevice:        the handler of the device on which to open.
* @return: 		::VzRetOK	    if the function succeeded, or one of the error values defined by ::VzReturnStatus.
*/
VZENSE_C_API_EXPORT VzReturnStatus Vz_PCOpenDevice(const char* pConfigPath, PeopleCountDeviceHandler* pDevice);

/**
* @brief 		Closes the device that was opened using Vz_OpenDevice.
* @param[in/out]	pDeviceHandler: The handler of the device to close.After that, *pDeviceHandler will be set to NULL and can no longer be used.
* @return: 		::VzRetOK           if the function succeeded, or one of the error values defined by ::VzReturnStatus.
*/
VZENSE_C_API_EXPORT VzReturnStatus Vz_PCCloseDevice(PeopleCountDeviceHandler* pDevice);

/**
* @brief 		Get information about the person detected.
* @param[in]	deviceHandler:	    The handler of the device on which to get information about the person detected.
* @param[out]	VzPeopleInfoCount:	The Pointer to a buffer in which to store information about the person detected.
* @return: 		::VzRetOK	        if the function succeeded, or one of the error values defined by ::VzReturnStatus.
*/
VZENSE_C_API_EXPORT VzReturnStatus Vz_PCGetPeopleInfoCount(const PeopleCountDeviceHandler device, VzPeopleInfoCount* pPeopleInfoCount);

/**
*  @brief       Sets whether to return images for debugging
*  @param[in]	deviceHandler:  The handler of the device on which to set the switch to display debug image.
*  @param[In]   isShow:         true means it returns the image, false means it doesn't.
*  @return:     ::VzRetOK	if the function succeeded, or one of the error values defined by ::VzReturnStatus.
*/
VZENSE_C_API_EXPORT VzReturnStatus Vz_PCSetShowImg(const PeopleCountDeviceHandler device, bool isShow);

/**
*  @brief       Sets whether to save offline data
*  @param[in]	deviceHandler:  The handler of the device on which to set the status of saving offline data.
*  @param[In]   isSaved:  true means it save offline data, false means it doesn't.
*  @return:     ::VzRetOK	if the function succeeded, or one of the error values defined by ::VzReturnStatus.
*/
VZENSE_C_API_EXPORT VzReturnStatus Vz_PCSetSaveOfflineDataState(const PeopleCountDeviceHandler device, bool enable);

#endif //VPEOPLECOUNTAPI_H