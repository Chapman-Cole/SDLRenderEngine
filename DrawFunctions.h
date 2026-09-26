#ifndef DRAWFUNCTIONS_H
#define DRAWFUNCTIONS_H

#include "RenderContext.h"
#include "GPUText.h"

void DrawRectangle(RenderContext* renderContext, float center[2], float size[2], float color[4]);

void DrawRectangleRounded(RenderContext* renderContext, float center[2], float size[2], float color[4], float roundness);

void DrawCircle(RenderContext* renderContext, float center[2], float radius, float color[4]);

void DrawText(RenderContext* renderContext, GPUText* text, float position[2]);

#endif