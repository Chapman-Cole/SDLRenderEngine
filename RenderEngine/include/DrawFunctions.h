#ifndef DRAWFUNCTIONS_H
#define DRAWFUNCTIONS_H

#include "RenderContext.h"
#include "GPUText.h"

// Position refers to the top left corner of the rectangle. Both the position and size
// parameters should be in pixels.
void DrawRectangle(RenderContext* renderContext, float position[2], float size[2], float color[4]);

// Position refers to the top left corner of the rectangle. Both the position and size
// parameters should be in pixels. The cornerRadii should also be in pixels.
void DrawRectangleRounded(RenderContext* renderContext, float position[2], float size[2], float color[4], float cornerRadii[4]);

void DrawCircle(RenderContext* renderContext, float center[2], float radius, float color[4]);

void DrawText(RenderContext* renderContext, GPUText* text, float position[2]);

void DrawInputContext(RenderContext* renderContext, InputContext* inputContext);

#endif
