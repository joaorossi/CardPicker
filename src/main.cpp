#include <opencv2/opencv.hpp>
#include <raylib.h>

#include <thread>
#include <chrono>

using namespace std::literals::chrono_literals;

Image MatToImage(const cv::Mat& mat)
{
    cv::cvtColor(mat, mat, cv::COLOR_BGR2RGB);
    Image img = { mat.data, mat.cols, mat.rows, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8 };
    return img;
}

int main()
{
    std::string url = "rtsp://127.0.0.1:8554/live";
    std::cout << "Waiting for Larix Broadcaster to connect to: " << url << "\n";

    cv::VideoCapture cap;
    while (true)
    {
        cap.open(url);
        if (cap.isOpened())
            break;

        std::cerr << "Could not open stream, trying again is 2s...\n";
        std::this_thread::sleep_for(2s);
    }

    int width  = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));

    InitWindow(width, height, "CardVision - iPhone RTSP Feed");
    SetTargetFPS(60);

    Texture2D tex = { 0 };
    cv::Mat frame;

    while (!WindowShouldClose())
    {
        if (!cap.read(frame))
            continue;

        BeginDrawing();
        ClearBackground(BLACK);

        Image img = MatToImage(frame);
        if (tex.id == 0)
            tex = LoadTextureFromImage(img);
        else
            UpdateTexture(tex, img.data);

        DrawTexture(tex, 0, 0, WHITE);
        DrawText("Live: iPhone Stream", 10, 10, 20, GREEN);
        EndDrawing();
    }

    CloseWindow();
}
