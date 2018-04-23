/* Timer for calculating time concumption of all functions.
*  All rights reserved. KandaoVR 2017.
*  Contributor(s): Neil Z. Shao, H. Deng
*/
#pragma once
#include <string>
#include <time.h>
#include <map>
#include <vector>
#include <iostream>

#define startCpuTimer(name) \
	clock_t start_##name## = clock();

#define stopCpuTimer(name) \
	printf("[cpu timer] " #name " %.2f ms\n", double(clock() - start_##name##) / CLOCKS_PER_SEC * 1000);
