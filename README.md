# PeopleCounter

1. Install the camera

   Please install the camera 2.4 meters above the ground and make sure the front panel of the camera is parallel to the ground

2. Run Tools/ Sample. exe to view the detection result

3. DualCamera Sample

   a.Please refer to the picture below to fix the camera position

   ![dualCameraInstall](https://gitee.com/Vzense/PeopleCounter/raw/master/Assets/dualCameraInstall.png)

   b.Please write the camera SN to the CameraAlias field of Alg_PCConfig_Entrance.ini and Alg_PCConfig_Exit.ini respectively.

   c.Run Tools/ DualCameraSample. exe to view the detection result
4. The algorithm supports three kinds of detection accuracy:low, normal, high. It can be changed by modifying Alg_PCConfig/ALG/DetectionAccuracy in the config file.
