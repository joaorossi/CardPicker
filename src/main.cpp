#include <opencv2/opencv.hpp>
#include <raylib.h>

#include <thread>
#include <chrono>

std::vector<cv::Point2f> order_points(const std::vector<cv::Point>& pts)
{
    std::vector<cv::Point2f> ordered(4);
    // Sum and diff to find corners
    std::vector<cv::Point2f> f(pts.begin(), pts.end());
    sort(f.begin(), f.end(), [](cv::Point2f a, cv::Point2f b){ return a.x + a.y < b.x + b.y; });
    cv::Point2f tl = f[0];
    cv::Point2f br = f[3];
    sort(f.begin(), f.end(), [](cv::Point2f a, cv::Point2f b){ return a.y - a.x < b.y - b.x; });
    cv::Point2f tr = f[0];
    cv::Point2f bl = f[3];
    ordered = {tl, tr, br, bl};
    return ordered;
}

// Max number of camera indices to try
static const int MAX_NUM_CAMERAS { 5 };

// Area threshold for card detection
static const double AREA_THRES { 6000.0 };

// Max number of cards with thumb preview
static const int MAX_CARD_PREVIEW { 3 };

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

    // just the first one
    // TODO: allow choosing between all the devices found
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

    const auto width { static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH)) };
    const auto height { static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT)) };

    InitWindow(width, height, "Card Picker 3000");
    SetTargetFPS(30); // On Release build is goes smooth even with 60FPS

    // Video feed texture
    Texture2D video_tex { 0 };

    // Card preview textures
    Texture2D card_tex[MAX_CARD_PREVIEW] { { 0 }, { 0 }, { 0 } };

    // Aux buffers
    cv::Mat frame, gray, blurImg, edges;
    std::vector<std::vector<cv::Point>> contours;
    std::vector<std::vector<cv::Point>> detected_cards;
    std::vector<cv::Point> approx;

    // For preview thumbnails
    std::vector<cv::Mat> card_images;

    while (!WindowShouldClose())
    {
        if (!cap.read(frame))
            continue;

        // Card detection
        {
            cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
            cv::GaussianBlur(gray, blurImg, cv::Size(5,5), 0);
            cv::adaptiveThreshold(blurImg, edges, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 11, 2);

            contours.clear();
            cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

            detected_cards.clear();
            card_images.clear();
            for (auto& contour : contours)
            {
                // We just want a limited number of cards
                if (card_images.size() >= MAX_CARD_PREVIEW)
                    break;

                approx.clear();
                const auto peri { cv::arcLength(contour, true) };
                cv::approxPolyDP(contour, approx, 0.02 * peri, true);

                if (approx.size() == 4 && cv::isContourConvex(approx))
                {
                    const auto area = cv::contourArea(approx);
                    if (area > AREA_THRES)
                    {
                        detected_cards.push_back(approx);
                        std::vector<cv::Point2f> ordered = order_points(approx);

                        // TODO: this is just a workaround to make things easier, the preview size should be detected
                        const auto preview_width { 250.f };
                        const auto preview_height { 350.f };
                        std::vector<cv::Point2f> dst_pts { { 0.f, 0.f }, { preview_width - 1.f, 0.f }, { preview_width - 1.f, preview_height - 1.f }, { 0.f, preview_height - 1.f } };

                        cv::Mat M = cv::getPerspectiveTransform(ordered, dst_pts);
                        cv::Mat warped;
                        cv::warpPerspective(frame, warped, M, cv::Size(preview_width, preview_height));

                        card_images.push_back(warped);
                    }
                }
            }
        }

        // OpenCV uses diffente channel ordering than raylib
        cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
        Image video_img { frame.data, frame.cols, frame.rows, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8 };

        // Load image to GPU texture
        if (video_tex.id == 0)
            video_tex = LoadTextureFromImage(video_img);
        else
            UpdateTexture(video_tex, video_img.data);

        BeginDrawing();
        {
            ClearBackground(BLACK);

            // Draw video feed
            DrawTexture(video_tex, 0, 0, WHITE);

            // Draw card detection overlay
            for (const auto& card : detected_cards)
            {
                for (int i = 0; i < 4; i++)
                {
                    Vector2 p1 = { static_cast<float>(card[i].x), static_cast<float>(card[i].y) };
                    Vector2 p2 = { static_cast<float>(card[(i + 1) % 4].x), static_cast<float>(card[(i + 1) % 4].y) };
                    DrawLineEx(p1, p2, 3.f, RED);
                }
            }

            // Draw detected card preview thumb
            int card_idx { 0 };
            int preview_Y { 20 };

            for (auto& card : card_images)
            {
                cv::cvtColor(card, card, cv::COLOR_BGR2RGB);
                Image card_img { card.data, card.cols, card.rows, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8 };

                if (card_tex[card_idx].id == 0)
                    card_tex[card_idx] = LoadTextureFromImage(card_img);
                else
                    UpdateTexture(card_tex[card_idx], card_img.data);

                DrawTexture(card_tex[card_idx], width - card_img.width - 20, preview_Y, WHITE);
                preview_Y += card_img.height + 20;
            }

            DrawFPS(10, 10);
        }
        EndDrawing();
    }

    // Unload as many card textures were needed
    for (int i = 0; i < MAX_CARD_PREVIEW; ++i)
        if (card_tex[i].id)
            UnloadTexture(card_tex[i]);

    // Unload video feed texture
    UnloadTexture(video_tex);

    // Bye!
    CloseWindow();
}
