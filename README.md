# IOTClock


This is an enhanced version of the Digital clock shared by DIY Machines
https://www.diymachines.co.uk/how-to-build-a-giant-hidden-shelf-edge-clock

Thanks a lot DIY Machines for your project and sharing it with others.

There are some changes made to the original version below. 

Hardware used: 
1. D1 mini
2. photo register

Code changes explained: 

1. Wifi module introduced to update the code wirelessly without opening the clock
2. Used local api to get the real-time time which will take care of DST settings so that no need to add a battery powered clock module.
3. Used complimentary color calculated to distinguish between hour and minute 


Code have sections for calling a local server time api, and complementary color calculation.

To support D1mini, the pins have been changed accordingly.
Added OTA update code and the below new methods. The color changes every one minute. 

    getComplementaryColor( int color)
    callTimeApi()
    changeColor()


## Sample Photo
![alt text](https://github.com/leninmohanty/IOTClock/blob/main/photo.jpeg?raw=true)
