#ifndef PD_UI_H
#define PD_UI_H

#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

struct mCore;

void PokeDaisy_InitUI(void);
void PokeDaisy_DrawBottomScreen(C3D_RenderTarget* bottomScreen, struct mCore* core);
void PokeDaisy_CleanupUI(void);

#endif // PD_UI_H
