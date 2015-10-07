#ifndef	_BE_ST_SDL_
#define _BE_ST_SDL_

// WARNING: This struct is used by BE_ST_SDL ONLY (except for controller stuff)

typedef enum { VSYNC_AUTO, VSYNC_OFF, VSYNC_ON } VSyncSettingType;
typedef enum { SCALE_ASPECT, SCALE_FILL } ScaleTypeSettingType;
typedef enum { LAUNCHER_WINDOW_DEFAULT, LAUNCHER_WINDOW_FULL, LAUNCHER_WINDOW_SOFTWARE } LauncherWindowSettingType;

typedef struct
{
	bool isFullscreen;
	int fullWidth, fullHeight;
	int winWidth, winHeight;
	int launcherWinWidth, launcherWinHeight;
	LauncherWindowSettingType launcherWinType;
	int displayNum;
	int sdlRendererDriver;
	VSyncSettingType vSync;
	bool isBilinear;
	ScaleTypeSettingType scaleType;
	int scaleFactor;
	bool autolockCursor;
	int sndSampleRate;
	bool sndSubSystem;
	bool oplEmulation;
	struct
	{
		int actionMappings[9]; // Buttons/triggers
		bool useDpad;
		bool useLeftStick;
		bool useRightStick;
		bool analogMotion;
		bool isEnabled;
	} altControlScheme;
	unsigned int farPtrSegOffset; // Actually used just in The Catacomb Armageddon/Apocalypse
} RefKeenConfig;

extern RefKeenConfig g_refKeenCfg;

#endif
