#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/tracking/tracking_legacy.hpp>
#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace cv;
using namespace std;

int main(int argc, char **argv)
{
    // выбор интересующей области на изображении 
    Rect2d bbox0(287, 23, 86, 320);
    Rect2d bbox(287, 23, 86, 320);
    Rect2d bbox_trust(287, 23, 86, 320); 
    // List of tracker types in OpenCV 3.4.1
    string trackerTypes[4] = {"MEDIANFLOW", "KCF", "MOSSE", "CSRT"};
    int N_len = 4;
    // vector <string> trackerTypes(types, std::end(types));
    for(int number=2; number < 6; number++)
    {
        // Создание нового текстового файла
        //описывает поток для записи данных в файл
        ofstream out_text;
        //открываем файл в режиме записи,
        //режим ios::out устанавливается по умолчанию
        out_text.open("Performance_test_crop_"+to_string(number)+".txt", ios::out);
        out_text << "\t\tFPS\tLOSS RATE\tLOSS TIME\tTrue/False" << endl;
        for(int j=0; j<N_len; j++)
        {
            // Create a tracker
            string trackerType = trackerTypes[j];
        
            Ptr<cv::legacy::Tracker> tracker;
            Ptr<cv::legacy::Tracker> tracker_trust;
        
            #if (CV_MINOR_VERSION < 3)
            {
                tracker = Tracker::create(trackerType);
            }
            #else
            {
                if (trackerType == "KCF")
                    tracker = cv::legacy::TrackerKCF::create();
                if (trackerType == "MEDIANFLOW")
                    tracker = cv::legacy::TrackerMedianFlow::create();
                if (trackerType == "MOSSE")
                    tracker = cv::legacy::TrackerMOSSE::create();
                if (trackerType == "CSRT")
                    tracker = cv::legacy::TrackerCSRT::create();
            }
            #endif
            // Установка доверительного трекера
            tracker_trust = legacy::TrackerCSRT::create();
            // Read video
            VideoCapture video("crop_"+to_string(number)+".mp4");
            
            // Exit if video is not opened
            if(!video.isOpened())
            {
                cout << "Could not read video file" << endl; 
                return 1; 
            } 
        
            // Read first frame 
            Mat frame; 
            bool ok = video.read(frame); 
    
            if(j == 0)
                bbox0 = selectROI(frame, false); 
        
            // Uncomment the line below to select a different bounding box 
            bbox = bbox0; 
            // Display bounding box. 
            //rectangle(frame, bbox, Scalar( 255, 0, 0 ), 2, 1 ); 
        
            // Uncomment the line below to select a different bounding box 
            bbox_trust = bbox0; 

            int i=0;                         // Число итераций
            int fps0=0;                      // Усредненное 
            int f = 0;                       // Флаг потери обЪекта
            string time_loss = "None";  // Впремя потери обЪекта (вычисляется не корректно)
            int LOSS_RATE = 0;               // Число потерь обЪекта
            int false_number = 0;            // Число отличий от доверительного трекера
            int true_number = 0;             // Число совпадений с доверительным трекером
        
            imshow("Tracking", frame); 
            tracker->init(frame, bbox);
            tracker_trust->init(frame, bbox_trust);

            int fourcc = VideoWriter::fourcc('m', 'p', '4', 'v');
            VideoWriter outputVideo("output_cpp_"+to_string(number)+"_"+trackerType+"_type}.mp4", fourcc, 30.0, Size(1920, 1080)); // For writing the video

            double timer0 = (double)getTickCount();
            
            while(video.read(frame))
            {     
                i++;
                // Start timer
                double timer = (double)getTickCount();
                
                // Update the tracking result
                bool ok = tracker->update(frame, bbox);
                
                // Calculate Frames per second (FPS)
                float fps = getTickFrequency() / ((double)getTickCount() - timer);
                fps0 += int(fps);

                // Обновление доверительного трекера
                bool ok_trust = tracker_trust->update(frame, bbox_trust);

                // Отслеживание точности трекера
                if(ok and ok_trust)
                {
                    if(bbox.x <= (bbox_trust.x + bbox_trust.width/2) and (bbox.x + bbox.width) >= (bbox_trust.x + bbox_trust.width/2))
                    {
                        if(bbox.y <= (bbox_trust.y + bbox_trust.height/2) and (bbox.y + bbox.height) >= (bbox_trust.y + bbox_trust.height/2))
                            true_number ++;
                        else
                            false_number ++;
                    }        
                    else
                        false_number ++;
                }        
                
                if (ok)
                {
                    // Отслеживание успеха
                    if(f == 1)
                    {
                        f = 0;
                    }
                    // Tracking success : Draw the tracked object
                    rectangle(frame, bbox, Scalar( 255, 0, 0 ), 2, 1 );
                    putText(frame, "x: "+ to_string(int(bbox.x + bbox.width/2)) + "; y " + to_string(int(bbox.y + bbox.height/2)), Point(100,80), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(0,0,255),2);
                }
                else
                {
                    if(f == 0)
                    {
                        LOSS_RATE ++;
                        time_loss = to_string((double)getTickCount() - timer0);
                        f = 1;
                    }
                    // Tracking failure detected.
                    putText(frame, "Tracking failure detected", Point(100,80), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(0,0,255),2);
                }
                
                // Display tracker type on frame
                putText(frame, trackerType + " Tracker", Point(100,20), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(50,170,50),2);
                
                // Display FPS on frame
                putText(frame, "FPS : " + to_string(int(fps)), Point(100,50), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(50,170,50), 2);
        
                // Display frame.
                imshow("Tracking", frame);
                outputVideo.write(frame);
                
                // Exit if ESC pressed.
                int k = waitKey(1);
                if(k == 27)
                {
                    break;
                }
            }
            video.release();
            outputVideo.release();
            destroyAllWindows();
            fps0 = fps0/i;
            out_text << trackerType +"\t";
            out_text << to_string((int)(fps0))+"\t";
            out_text << to_string(LOSS_RATE)+"\t\t";
            out_text << time_loss +"\t";
            out_text << to_string(round(true_number/(i-1)*10000)/100)+"/"+to_string(round(false_number/(i-1)*10000)/100) << endl;
        }
        out_text.close(); 
    }
}