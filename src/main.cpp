#include <opencv2/opencv.hpp>
#include <raylib.h>

#include <thread>
#include <chrono>

// Max number of camera indices to try
static const int MAX_NUM_CAMERAS { 5 };

int main()
{
    // keep a count of avaiable video caption device IDs
    std::vector<cv::String> camera_ids;
    for (int i = 0; i < MAX_NUM_CAMERAS; ++i)
    {
        cv::VideoCapture cap(i);
        if (cap.isOpened())
            camera_ids.push_back(cap.getBackendName());
    }

    cv::VideoCapture cap;
    if (camera_ids.size())
    {
        int idx { 0 };
        for (const auto& c : camera_ids)
        {
            if (cap.open(idx))
            {
                std::cout << "Opened camera id: " << idx << "; " << c << std::endl;
                break;
            }
            else
            {
                ++idx;
                std::cout << "Failed opening camera id: " << idx << "; " << c << std::endl;
            }
        }
        if (idx == camera_ids.size())
        {
            std::cout << "Couldn't open a camera stream." << std::endl;
            return 0;
        }
    }

    auto width { static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH)) };
    auto height { static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT)) };

    InitWindow(width, height, "CardPicker");
    SetTargetFPS(30);

    Texture2D tex = { 0 };
    cv::Mat frame;

    while (!WindowShouldClose())
    {
        if (!cap.read(frame))
            continue;

        BeginDrawing();
        {
            ClearBackground(BLACK);

            // OpenCV uses diffente channel ordering than raylib
            cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
            Image img { frame.data, frame.cols, frame.rows, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8 };

            if (tex.id == 0)
                tex = LoadTextureFromImage(img);
            else
                UpdateTexture(tex, img.data);

            DrawTexture(tex, 0, 0, WHITE);
            DrawFPS(width - 80, 10);
        }
        EndDrawing();
    }

    CloseWindow();
}
