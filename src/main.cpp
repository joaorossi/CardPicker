#include <opencv2/opencv.hpp>
#include <raylib.h>

#include <thread>
#include <chrono>

// Max number of camera indices to try
static const int MAX_NUM_CAMERAS { 5 };

// Area threshold for card detection
static const double AREA_THRES { 1000.0 };

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
    cv::Mat frame, gray, blurImg, edges;
    std::vector<std::vector<cv::Point>> contours;
    std::vector<std::vector<cv::Point>> detected_cards;
    std::vector<cv::Point> approx;

    while (!WindowShouldClose())
    {
        if (!cap.read(frame))
            continue;

        {
            cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
            cv::GaussianBlur(gray, blurImg, cv::Size(5, 5), 0.0);
            cv::Canny(blurImg, edges, 75, 200);

            contours.clear();
            cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

            detected_cards.clear();
            for (auto& contour : contours)
            {
                approx.clear();
                const auto peri { cv::arcLength(contour, true) };
                cv::approxPolyDP(contour, approx, 0.02 * peri, true);

                if (approx.size() == 4 && cv::isContourConvex(approx))
                {
                    const auto area { cv::contourArea(approx) };
                    if (area > 1000.0)
                        detected_cards.push_back(approx);
                }
            }
        }

        // OpenCV uses diffente channel ordering than raylib
        cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
        Image img { frame.data, frame.cols, frame.rows, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8 };

        // Load image to GPU texture
        if (tex.id == 0)
            tex = LoadTextureFromImage(img);
        else
            UpdateTexture(tex, img.data);

        BeginDrawing();
        {
            ClearBackground(BLACK);

            // Draw video feed
            DrawTexture(tex, 0, 0, WHITE);

            // Draw card detection overlay
            for (const auto& card : detected_cards)
            {
                for (int i = 0; i < 4; i++)
                {
                    Vector2 p1 = {(float)card[i].x, (float)card[i].y};
                    Vector2 p2 = {(float)card[(i + 1) % 4].x, (float)card[(i + 1) % 4].y};
                    DrawLineEx(p1, p2, 3.f, RED);
                }
            }

            DrawFPS(10, 10);
        }
        EndDrawing();
    }

    UnloadTexture(tex);
    CloseWindow();
}
