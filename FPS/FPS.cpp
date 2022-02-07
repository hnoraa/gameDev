#include <Windows.h>
#include <string>
#include <chrono>
#include <vector>
#include <algorithm>

// screen 
int nScreenWidth = 120;
int nScreenHeight = 40;

// player
float fPlayerX = 8.0f;
float fPlayerY = 8.0f;
float fPlayerA = 0.0f;	// angle player is facing
float fRotationFactor = 0.8f;
float fWalkSpeed = 5.0f;

// map
int nMapHeight = 16;
int nMapWidth = 16;

// field of view
float fFOV = 3.14159 / 4.0;	// initially PI/4 - very narrow fov

// depth for distance limits
float fDepth = 16.0f;

int main()
{
	// unicode
	// create screen buffer
	wchar_t* screen = new wchar_t[nScreenWidth * nScreenHeight];	// 2d array for the screen

	// handle to the console - regular text mode buffer
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleActiveScreenBuffer(hConsole);	// the buffer is the target of the console
	DWORD dwBytesWritten = 0;

	// wstring is unicode
	std::wstring map;

	map += L"##########.....#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#.....##.......#";
	map += L"#.....##.......#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#.........######";
	map += L"#..............#";
	map += L"#..............#";
	map += L"#........#.....#";
	map += L"#........#.....#";
	map += L"################";

	auto tp1 = std::chrono::system_clock::now();
	auto tp2 = std::chrono::system_clock::now();


	while (1)
	{
		// time checks - get the elapsed time to get FPS
		// will give us a more controlled movement on the screen
		// this allows for consistent movement
		tp2 = std::chrono::system_clock::now();
		std::chrono::duration<float> elapsedTime = tp2 - tp1;
		tp1 = tp2;
		float fElapsedTime = elapsedTime.count();

		// controls - get player movements
		// quit
		if (GetAsyncKeyState((unsigned short)'Q') & 0x8000)
		{
			break;
		}

		// handle counter clockwise rotation
		if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
		{
			fPlayerA -= fRotationFactor * fElapsedTime;	// radians per frames
		}

		// handle clockwise rotation
		if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
		{
			fPlayerA += fRotationFactor * fElapsedTime; // radians per frames
		}

		// move forward
		if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
		{
			fPlayerX += sinf(fPlayerA) * fWalkSpeed * fElapsedTime;
			fPlayerY += cosf(fPlayerA) * fWalkSpeed * fElapsedTime;

			// collision detection - convert the players current x,y into integers and test against the map (which is integer based)
			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#')
			{
				fPlayerX -= sinf(fPlayerA) * fWalkSpeed * fElapsedTime;
				fPlayerY -= cosf(fPlayerA) * fWalkSpeed * fElapsedTime;
			}
		}

		// move backward
		if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
		{
			fPlayerX -= sinf(fPlayerA) * fWalkSpeed * fElapsedTime;
			fPlayerY -= cosf(fPlayerA) * fWalkSpeed * fElapsedTime;

			// collision detection - convert the players current x,y into integers and test against the map (which is integer based)
			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#')
			{
				fPlayerX += sinf(fPlayerA) * fWalkSpeed * fElapsedTime;
				fPlayerY += cosf(fPlayerA) * fWalkSpeed * fElapsedTime;
			}
		}

		// strafe left - invert the unit vector (cos and sin switched)
		if (GetAsyncKeyState((unsigned short)'Z') & 0x8000)
		{
			fPlayerX -= cosf(fPlayerA) * fWalkSpeed * fElapsedTime;
			fPlayerY -= sinf(fPlayerA) * fWalkSpeed * fElapsedTime;

			// collision detection - convert the players current x,y into integers and test against the map (which is integer based)
			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#')
			{
				fPlayerX += cosf(fPlayerA) * fWalkSpeed * fElapsedTime;
				fPlayerY += sinf(fPlayerA) * fWalkSpeed * fElapsedTime;
			}
		}

		// strafe right - invert the unit vector (cos and sin switched)
		if (GetAsyncKeyState((unsigned short)'C') & 0x8000)
		{
			fPlayerX += cosf(fPlayerA) * fWalkSpeed * fElapsedTime;
			fPlayerY += sinf(fPlayerA) * fWalkSpeed * fElapsedTime;

			// collision detection - convert the players current x,y into integers and test against the map (which is integer based)
			if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#')
			{
				fPlayerX -= cosf(fPlayerA) * fWalkSpeed * fElapsedTime;
				fPlayerY -= sinf(fPlayerA) * fWalkSpeed * fElapsedTime;
			}
		}

		// computation for each column on the screen
		for (int x = 0; x < nScreenWidth; x++)
		{
			// for each column, calculate the projected ray angle into world space
			// field of view is bisected by the player who is looking straight ahead, the fov to the left and right
			// hence / 2
			float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV;

			float fDistanceToWall = 0;
			bool bHitWall = false;		// has a wall been hit
			// is this the edge of a cell (fyi, we dont have any polygon info unlike sdl where you have rects) - these are the corners of the block
			bool bBoundary = false;

			// unit vector that represents the direction the player is looking in
			float fEyeX = sinf(fRayAngle);
			float fEyeY = cosf(fRayAngle);

			while (!bHitWall && fDistanceToWall < fDepth)
			{
				// if not a wall, increment distance to wall by steps
				fDistanceToWall += 0.1f;

				// create a line of a given distance, given the unit vector
				// only needs to be integer values because the wall tiles are on integer bounds
				// anything inbetween would also be a wall (ex: 1.5 would be in tile [1]
				int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
				int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

				// test if ray is out of bounds
				// this is ray casting
				if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
				{
					bHitWall = true;
					fDistanceToWall = fDepth;
				}
				else
				{
					// check individual cells of the environment
					// the ray is inbounds so check that the ray cell is a wall
					if (map[nTestY * nMapWidth + nTestX] == '#')
					{
						bHitWall = true;

						std::vector<std::pair<float, float>> p;	// <distance, dot product>

						// test the 4 corners of the wall
						for (int tX = 0; tX < 2; tX++)
						{
							for (int tY = 0; tY < 2; tY++)
							{
								// create a vector from the perfect corner (closest corner)
								// the corners are integers, cast them to floats for player position
								float vY = (float)nTestY + tY - fPlayerY;
								float vX = (float)nTestX + tX - fPlayerX;

								// calculate vector magnitude (length) - tells us how far away the corner is from the player
								float d = sqrt(vX * vX + vY * vY);

								// calculate the dot product between the unit vector and the vector of the perfect corner
								float dot = (fEyeX * vX / d) + (fEyeY * vY / d);

								// add to the vector
								p.push_back(std::make_pair(d, dot));
							}
						}

						// sort the vector (using lambda function)
						// this is sorted from closest to farthest, this sort is based on the first element of the vector pair which is the distance
						sort(p.begin(), p.end(), [](const std::pair<float, float>& left, const std::pair<float, float>& right) { return left.first < right.first; });
						float fBound = 0.01;	// radians

						// take inverse cos of the second part of our vector pair (the dot product)
						// the inverse cos of the dot product gives you the angle between the 2 rays
						// only need to test for the first 2 or 3 because the player never sees all 4 corners
						if (acos(p.at(0).second) < fBound) bBoundary = true;
						if (acos(p.at(1).second) < fBound) bBoundary = true;
						if (acos(p.at(2).second) < fBound) bBoundary = true;
					}
				}
			}

			// calculate distance to ceiling and floor (when looking at the walls,
			// the closer walls have less ceiling and floor visible and the farther
			// walls have more showing)
			// ceiling = midpoint - proportion of the screen height relative to the distance to the wall
			int nCeiling = (float)(nScreenHeight / 2.0f) - nScreenHeight / ((float)fDistanceToWall);
			int nFloor = nScreenHeight - nCeiling;	// the floor is a mirror of the ceiling

			// shading - walls farther away are darker
			short nShade = ' ';

			// shade the walls
			if (fDistanceToWall <= fDepth / 4.0f)
			{
				nShade = 0x2588;	 // very close
			}
			else if (fDistanceToWall < fDepth / 3.0f)
			{
				nShade = 0x2593;
			}
			else if (fDistanceToWall < fDepth / 2.0f)
			{
				nShade = 0x2592;
			}
			else if (fDistanceToWall < fDepth)
			{
				nShade = 0x2591;
			}
			/*else
			{
				nShade = ' ';
			}*/

			if (bBoundary)
			{
				nShade = ' ';
			}

			for (int y = 0; y < nScreenHeight; y++)
			{
				if (y < nCeiling)
				{
					// y is part of the ceiling, fill in in with blank space
					screen[y * nScreenWidth + x] = ' ';
				}
				else if (y > nCeiling && y <= nFloor)
				{
					// this is a wall
					screen[y * nScreenWidth + x] = nShade;
				} else {
					// this is the floor
					float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
					if (b < 0.25)
					{
						nShade = '#';
					}
					else if (b < 0.5)
					{
						nShade = 'x';
					}
					else if (b < 0.75)
					{
						nShade = '.';
					}
					else if (b < 0.9)
					{
						nShade = '-';
					}
					/*else
					{
						nShade = ' ';
					}*/
					screen[y * nScreenWidth + x] = nShade;
				}
			}
		}

		// display player stats
		swprintf_s(screen, 50, L"X=%3.2f, Y=%3.2F, A=%3.2f, FPS=%3.2f ", fPlayerX, fPlayerY, fPlayerA, (1.0f / fElapsedTime));

		// display the map
		for (int nX = 0; nX < nMapWidth; nX++)
		{
			for (int nY = 0; nY < nMapHeight; nY++)
			{
				screen[(nY + 1) * nScreenWidth + nX] = map[nY * nMapWidth + nX];
			}
		}

		// show player on mini-map
		screen[((int)fPlayerY + 1) * nScreenWidth + (int)fPlayerX] = 'P';

		// set last character of array to the escape character \0
		screen[nScreenWidth * nScreenHeight - 1] = '\0';

		// write to console, {0, 0} allows us to write only to the top left so it stops the console from scrolling down
		WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
	}

	return 0;
}
