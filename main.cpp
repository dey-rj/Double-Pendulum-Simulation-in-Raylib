#include "raylib.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <queue>
#include <cstdio>

using namespace std;

const int screenWidth = 1280;
const int screenHeight = 720;
const float g = 981.0;
const int maxTraceSize = 500;

class pendulum {
    public:
        float length;
        float thickness;
        Vector2 start;
        Vector2 end;
        Color color;
        float mass;
        float massRadius;
        Color massColor;
        float angle;
        float angle_d;
        float angle_dd;
        deque<Vector2> trace;

        Vector2 getEnd() {
            Vector2 temp;
            temp.x = this->start.x + this->length * sin(this->angle);
            temp.y = this->start.y + this->length * cos(this->angle);
            return temp;
        }
 
        pendulum(float length, float thickness, Vector2 start, Color color, float mass, float massRadius, Color massColor) {
            this->length = length;
            this->thickness = thickness;
            this->start = start;
            this->color = color;
            this->mass = mass;
            this->massRadius = massRadius;
            this->massColor = massColor;
            this->angle = (rand() % (90 + 1 + 90) - 90) * DEG2RAD;
            this->angle_d = 0;
            this->angle_dd = 0;
            this->end = getEnd();
        }
};

void drawPendulum(pendulum &p) {
    DrawLineEx(p.start, p.end, p.thickness, p.color);  
    DrawCircleV(p.end, p.massRadius, p.massColor); 
}

void drawTrace(pendulum &p) {
    int size = p.trace.size();

    for(int i=0; i<size; i++) {
        unsigned char alpha = (unsigned char)((float)i / size * 255);
        DrawCircleV(p.trace[i], 1, { 255, 145, 255, alpha});
    }
    if(size == maxTraceSize)
        p.trace.pop_front();
}

void update(pendulum &p1, pendulum &p2) {
    p2.trace.push_back(p2.end);

    float deltaTime = GetFrameTime();
    
    // Angular Accelaration (double derivative of angle)
    p1.angle_dd = (-g * (2 * p1.mass + p2.mass) * sin(p1.angle) - p2.mass * g * sin(p1.angle - 2 * p2.angle) - 2 * sin(p1.angle - p2.angle) * p2.mass * (p2.angle_d * p2.angle_d * p2.length + p1.angle_d * p1.angle_d * p1.length * cos(p1.angle - p2.angle))) / (p1.length * (2 * p1.mass + p2.mass - p2.mass * cos(2 * p1.angle - 2 * p2.angle)));
    p2.angle_dd = (2 * sin(p1.angle - p2.angle) * (p1.angle_d * p1.angle_d * p1.length * (p1.mass + p2.mass) + g * (p1.mass + p2.mass) * cos(p1.angle) + p2.angle_d * p2.angle_d * p2.length * p2.mass * cos(p1.angle - p2.angle))) / (p2.length * (2 * p1.mass + p2.mass - p2.mass * cos(2 * p1.angle - 2 * p2.angle)));
    
    // Damping
    float damping = 0.01f;
    p1.angle_dd -= damping * p1.angle_d;
    p2.angle_dd -= damping * p2.angle_d;

    // Angle = AngularVelocity * time
    p1.angle += p1.angle_d * deltaTime;
    p2.angle_d += p2.angle_dd * deltaTime;

    // Angular velocity (derivate of angle) = AngularAccelaration * time
    p1.angle_d += p1.angle_dd * deltaTime;
    p2.angle += p2.angle_d * deltaTime;

    p1.end = p1.getEnd();
    p2.start = p1.end;
    p2.end = p2.getEnd();
}

int main(void)
{
    
    srand ( time(NULL) );

    pendulum p1(screenHeight/1.9f, 3, {screenWidth/2.0f, screenHeight/9.0f}, WHITE, 1, 10, RED);
    pendulum p2(screenHeight/4.0f, 3, p1.end, WHITE, 1, 10, RED);

    InitWindow(screenWidth, screenHeight, "Double Pendulum Simulation");
    SetTargetFPS(60);               
    
    FILE* ffmpeg = nullptr;
    int frameCount = 0;
    const int totalFrames = 60 * 20; // 20 seconds at 60 FPS
    bool recording = false;

    while (!WindowShouldClose())    
    {
        // Press 'R' to start recording
        if (IsKeyPressed(KEY_R) && !recording) {
            recording = true;
            const char* cmd = "ffmpeg -y -f rawvideo -pix_fmt rgba -s 1280x720 -r 60 -i - "
                            "-c:v libx264 -preset ultrafast -pix_fmt yuv420p output.mp4";
            ffmpeg = popen(cmd, "w");
        }

        if (IsKeyPressed(KEY_SPACE)) {
            p1 = pendulum(screenHeight/1.7f, 3, {screenWidth/2.0f, screenHeight/9.0f}, WHITE, 1, 10, RED);
            p2 = pendulum(screenHeight/4.0f, 3, p1.end, WHITE, 1, 10, RED);
        } 
        
        update(p1, p2); 
        
        BeginDrawing();
            ClearBackground(BLACK);
            drawPendulum(p2);
            drawPendulum(p1); 
            drawTrace(p2);
            DrawText("PRESS SPACE TO RESTART", 20, 20, 20, RAYWHITE);
            if (recording) DrawText("RECORDING...", 20, 50, 20, RED);
        EndDrawing();

        if (recording && frameCount < totalFrames) {
            Image frame = LoadImageFromScreen(); 
            fwrite(frame.data, 1280 * 720 * 4, 1, ffmpeg);
            UnloadImage(frame); 
            frameCount++;
        } else if (frameCount >= totalFrames && recording) {
            pclose(ffmpeg);
            recording = false;
            TraceLog(LOG_INFO, "Recording finished: output.mp4 saved.");
        }
    }
    if (recording) pclose(ffmpeg);
    CloseWindow();       
    return 0;
}
