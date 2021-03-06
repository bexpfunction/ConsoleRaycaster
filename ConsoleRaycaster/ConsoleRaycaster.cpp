// Tests.cpp : Defines the entry point for the console application.
//
#include <iostream>
#include <string>
#include <ctime>
#include <algorithm>
#include <clocale>
#include <chrono>
#include <cwchar>
#include <math.h>
#include <vector>
#include <utility>
#include <Windows.h>

//Explanations
/*****************Character codes*****************
0x2588 = █	Full block
0x2593 = ▓	Dark shade
0x2592 = ▒	Medium shade
0x2591 = ░	Light shade
0x0020 =	Space
**************************************************/

//Definitions
#define TO_RAD 0.01745

//Globals
int sWidth = 120;
int sHeight = 40;
int mWidth = 22;
int mHeight = 20;
bool quit = 0;
//System globals
wchar_t *pScreenBuffer;
HANDLE hConsole;
HDC hConsoleDevice;
HWND hConsoleWindow;
DWORD dwBytesWritten;
//Enums
enum Shade {
	SPACE = 0,
	LIGHT = 1,
	MEDIUM = 2,
	DARK = 3,
	FULL = 4
};
//Structs
struct Player
{
	float xPos, yPos;
	float angle;
	float health;
	float walkSpeed, turnSpeed;
	float fov;
};
//Game globals
Player bjBlaz;
std::wstring map;

//Func prototypes
void ClearScreen();
void HandleInput();
void Update();
void UpdateMap();
void Draw();
void DrawVertical(int xPos, int length, int shade);
Player FindPlayerOnMap();
void PrintPlainMap();
void Quit();


int main()
{
	//Setup
	//Console screen buffer
	pScreenBuffer = new wchar_t[sWidth * sHeight];
	hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);
	hConsoleWindow = GetConsoleWindow();
	hConsoleDevice = GetDC(hConsoleWindow);
	dwBytesWritten = 0;

	//Create map
	map += L"######################";
	map += L"#....................#";
	map += L"#....................#";
	map += L"#....................#";
	map += L"#....................#";
	map += L"#....................#";
	map += L"#....................#";
	map += L"#....................#";
	map += L"#....................#";
	map += L"#.........P...####...#";
	map += L"#.............####...#";
	map += L"#.............####...#";
	map += L"#.............####...#";
	map += L"#.............####...#";
	map += L"#.............####...#";
	map += L"#.............####...#";
	map += L"#.............####...#";
	map += L"#.............####...#";
	map += L"#.............####...#";
	map += L"######################";

	//Create player
	bjBlaz.fov = 60.0f*3.14159f/180.0f;
	bjBlaz.xPos = 10.f; bjBlaz.yPos = 2.0f;
	bjBlaz.health = 100.0f; bjBlaz.angle = 0.0f;
	bjBlaz.walkSpeed = 0.1f; bjBlaz.turnSpeed = 0.02f;
	bjBlaz = FindPlayerOnMap();


	ClearScreen();

	//Loop
	while (!quit)
	{
		//Handle input
		HandleInput();

		Update();

		//Write player info
		swprintf_s(pScreenBuffer, 50, L"Xpos: %3.1f, Ypos: %3.1f, Angle: %3.1f", bjBlaz.xPos, bjBlaz.yPos, bjBlaz.angle);
		//Write the buffer
		WriteConsoleOutputCharacter(hConsole, pScreenBuffer, sWidth * sHeight, { 0,0 }, &dwBytesWritten);
	}

	return 0;
}

void ClearScreen()
{
	for (int i = 0; i < sWidth*sHeight; i++)
		pScreenBuffer[i] = ' ';
}

void HandleInput()
{
	//Quit for escape
	if (GetAsyncKeyState(VK_ESCAPE) || GetAsyncKeyState((unsigned short)'Q') & 0x8000) {
		Quit();
	}
	//Rotate
	if (GetAsyncKeyState((unsigned short)'D') & 0x8000) {
		bjBlaz.angle -= bjBlaz.turnSpeed;
		//if (bjBlaz.angle < 0.0f)	bjBlaz.angle = 360.0f;
	}
	if (GetAsyncKeyState((unsigned short)'A') & 0x8000) {
		bjBlaz.angle += bjBlaz.turnSpeed;
		//if (bjBlaz.angle > 360.0f)	bjBlaz.angle = 0.0f;
	}
	//Forward backward
	if (GetAsyncKeyState((unsigned short)'W') & 0x8000) {
		float tmpXp, tmpYp;
		tmpXp = bjBlaz.xPos + (sinf(bjBlaz.angle) * bjBlaz.walkSpeed);
		tmpYp = bjBlaz.yPos + (cosf(bjBlaz.angle) * bjBlaz.walkSpeed);
		std::clamp(tmpXp, 0.0f, (float)mWidth-1.0f);
		std::clamp(tmpYp, 0.0f, (float)mHeight-1.0f);
		if (map.c_str()[(int)tmpYp * mWidth + (int)tmpXp] == '#') tmpXp = bjBlaz.xPos;
		if (map.c_str()[(int)tmpYp * mWidth + (int)tmpXp] == '#') tmpYp = bjBlaz.yPos;
		bjBlaz.xPos = tmpXp;
		bjBlaz.yPos = tmpYp;
	}
	if (GetAsyncKeyState((unsigned short)'S') & 0x8000) {
		float tmpXp, tmpYp;
		tmpXp = bjBlaz.xPos - (sinf(bjBlaz.angle) * bjBlaz.walkSpeed);
		tmpYp = bjBlaz.yPos - (cosf(bjBlaz.angle) * bjBlaz.walkSpeed);
		std::clamp(tmpXp, 0.0f, (float)mWidth-1.0f);
		std::clamp(tmpYp, 0.0f, (float)mHeight-1.0f);
		if (map.c_str()[(int)tmpYp * mWidth + (int)tmpXp] == '#') tmpXp = bjBlaz.xPos;
		if (map.c_str()[(int)tmpYp * mWidth + (int)tmpXp] == '#') tmpYp = bjBlaz.yPos;
		bjBlaz.xPos = tmpXp;
		bjBlaz.yPos = tmpYp;
	}
}

void Update()
{
	ClearScreen();
	UpdateMap();
	Draw();
	PrintPlainMap();
}

void UpdateMap()
{
	Player tmp = FindPlayerOnMap();
	map[(int)tmp.yPos * mWidth + (int)tmp.xPos] = '.';
	map[(int)bjBlaz.yPos * mWidth + (int)bjBlaz.xPos] = 'P';
}

void Draw()
{
	//Raycast from left to right
	for (int sX = 0; sX < sWidth; sX++)
	{
		bool rayHit = false;
		float z = 0.0f, rayX = 0.0f, rayY = 0.0f, rayZ = 0.0f;
		float rayAng = (bjBlaz.angle - bjBlaz.fov / 2.0f) + (((float)sX / (float)sWidth) * bjBlaz.fov);
		while (!rayHit && z<20.0f)
		{
			rayX = bjBlaz.xPos + (z*sinf(rayAng));
			rayY = bjBlaz.yPos + (z*cosf(rayAng));

			if (map.c_str()[(int)rayY * mWidth + (int)rayX] == '#') {
				rayZ = sqrtf(rayX*rayX + rayY * rayY) * cosf(bjBlaz.angle);
				if (rayZ >= 0.0f && rayZ < 5.0f) DrawVertical(sX, (int)( rayZ), Shade::FULL);
				else if (rayZ >= 5.0f && rayZ < 10.0f) DrawVertical(sX, (int)(rayZ), Shade::DARK);
				else if (rayZ >= 10.0f && rayZ < 15.0f) DrawVertical(sX, (int)(rayZ), Shade::MEDIUM);
				else if (rayZ >= 15.0f && rayZ < 20.0f) DrawVertical(sX, (int)(rayZ), Shade::LIGHT);
				else DrawVertical(sX, (int)(rayZ), Shade::LIGHT);
				rayHit = true;
			}

			z += 0.1f;
		}
	}
}

void DrawVertical(int xPos, int length, int shade)
{
	for (size_t y = sHeight / 2 - length / 2; y < sHeight / 2 + length / 2; y++)
	{
		switch (shade) {
		case 0: {
			pScreenBuffer[y*sWidth + xPos] = 0x0020;
			break;
		}
		case 1: {
			pScreenBuffer[y*sWidth + xPos] = 0x2591;
			break;
		}
		case 2: {
			pScreenBuffer[y*sWidth + xPos] = 0x2592;
			break;
		}
		case 3: {
			pScreenBuffer[y*sWidth + xPos] = 0x2593;
			break;
		}
		case 4: {
			pScreenBuffer[y*sWidth + xPos] = 0x2588;
			break;
		}
		default: {
			pScreenBuffer[y*sWidth + xPos] = 0x0020;
			break;
		}
		}
	}
}

Player FindPlayerOnMap()
{
	Player tmp = bjBlaz;
	for (size_t i = 0; i < mHeight; i++)
	{
		for (size_t j = 0; j < mWidth; j++)
		{
			if (map[i * mWidth + j] == 'P') {
				tmp.xPos = (float)j;
				tmp.yPos = (float)i;
				return tmp;
			}
		}
	}
	return bjBlaz;
}

void PrintPlainMap()
{
	for (size_t y = 0; y < mHeight; y++)
	{
		for (size_t x = 0; x < mWidth; x++)
		{
			pScreenBuffer[(y + 1)*sWidth + x] = map[y * mWidth + x];
		}
	}
}

void Quit()
{
	quit = 1;
}

