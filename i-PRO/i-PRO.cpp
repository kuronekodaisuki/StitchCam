// i-PRO.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//

#include <opencv2/opencv.hpp>

const char* camera1 = "rtsp://192.168.131.22/Src/MediaInput/stream_1";

int main()
{
    cv::VideoCapture cam1, cam2, cam3, cam4;
    cv::Mat image1, image2, image3, image4;

    if (cam1.open(camera1))
    {
        for (bool loop = true; loop;)
        {
            cam1.read(image1);
            cv::imshow("cam1", image1);

            switch (cv::waitKey(10))
            {
            case 'q':
                loop = false;
                break;
            }
        }
    }
}

