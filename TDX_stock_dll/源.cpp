#include "stdafx.h"
#include "TCalcFuncSets.h"
#include <cassert>

//生成的dll及相关依赖dll请拷贝到通达信安装目录的T0002/dlls/下面,再在公式管理器进行绑定
static int const START_FROM = 13;
static int const ZIG_PERCENT = 5;

void TOPRANGE_PERCENT(int DataLen, float* pfOUT, float* HIGH, float* HIGH_ADJUST)
{
	memset(pfOUT, 0, DataLen * sizeof(float));

	for (int i = DataLen - 1; i >= 0; i--)
	{
		float val = HIGH_ADJUST[i];
		int idx = i - 1;
		while (idx >= 0 && val > HIGH[idx])
			idx--;
		pfOUT[i] = i - idx - 1;
	}
}

void LOWRANGE_PERCENT(int DataLen, float* pfOUT, float* LOW, float* LOW_ADJUST)
{
	memset(pfOUT, 0, DataLen * sizeof(float));

	for (int i = DataLen - 1; i >= 0; i--)
	{
		float val = LOW_ADJUST[i];
		int idx = i - 1;
		while (idx >= 0 && val < LOW[idx])
			idx--;
		pfOUT[i] = i - idx - 1;
	}
}

void TROUGH_BARS_JUNXIAN(int DataLen, float* pfOUT, float* VAL, float* JUNXIAN)
{
	// 将 跌破、升破 60分 控盘线，作为划分顶、底的 标准，而不是 ZIG(K,N)采用的按照百分比来确定转向 

	memset(pfOUT, -1, DataLen * sizeof(float));

	float curBotVal(0xffffffff), newBotVal(0xffffffff);
	int curBotBars(-1), newBotBars(-1);

	for (int i = START_FROM - 1; i < DataLen; i++)
	{
		static int modify_Done = 0;

		if (VAL[i] <= JUNXIAN[i])
		{
			modify_Done = 0;

			if (VAL[i] < newBotVal)
			{
				newBotVal = VAL[i];
				newBotBars = i;
			}
			if (curBotBars > 0)
			{
				pfOUT[i] = i - curBotBars;
			}
		}
		else
		{
			if (!modify_Done && newBotBars>0)
			{
				for (int j = i; j >= newBotBars; j--)
				{
					pfOUT[j] = j - newBotBars;
				}
				curBotBars = newBotBars;
				curBotVal = newBotVal;
				newBotVal = 0xffffffff;
				newBotBars = -1;

				modify_Done = 1;
			}
			else if (curBotBars >0)
			{
				pfOUT[i] = i - curBotBars;
			}
			//else
			//	assert(curTopBars >0);

		}
	}
}

void PEAK_BARS_JUNXIAN(int DataLen, float* pfOUT, float* VAL, float* JUNXIAN)
{
	// 将 跌破、升破 60分 控盘线，作为划分顶、底的 标准，而不是 ZIG(K,N)采用的按照百分比来确定转向 

	memset(pfOUT, -1, DataLen * sizeof(float));

	float curTopVal(-0xffff), newTopVal(-0xffff);
	int curTopBars(-1), newTopBars(-1);

	for (int i = START_FROM -1; i < DataLen; i++)
	{
		static int modify_Done = 0;

		if (VAL[i] >= JUNXIAN[i])
		{
			modify_Done = 0;

			if (VAL[i] > newTopVal)
			{
				newTopVal = VAL[i];
				newTopBars = i;
			}
			if (curTopBars > 0)
			{
				pfOUT[i] = i - curTopBars;
			}
		}
		else
		{
			if (!modify_Done && newTopBars>0)
			{
				for (int j = i; j >= newTopBars; j--)
				{
					pfOUT[j] = j - newTopBars;
				}
				curTopBars = newTopBars;
				curTopVal = newTopVal;
				newTopVal = -0xffff;
				newTopBars = -1;

				modify_Done = 1;
			}
			else if (curTopBars >0)
			{
				pfOUT[i] = i - curTopBars;
			}
			//else
			//	assert(curTopBars >0);

		}
	}
}

void TROUGH_BARS_ZIG(int DataLen, float* pfOUT, float* VAL, float* criticalPoint)
{
	// criticalPoint是指： 判定ZIG拐点 的那个决策点，也就是比拐点 要晚一点的那个位置，实现了5个点的拐点幅度
	bool criticalPnt = ((int)*criticalPoint == 1);

	memset(pfOUT, -1, DataLen * sizeof(float));

	float possibleBot(VAL[0]), possibleTop(VAL[0]);
	int posBotBars(0), posTopBars(0);
	int curBotBars(-1), curTopBars(-1);

	enum {UNSURE, SEARCHING_TOP, SEARCHING_BOT} goal = UNSURE;

	for (int i = 1; i < DataLen; i++)
	{
		static int modify_Done = 0;

		if (goal == UNSURE)
		{
			if (VAL[i] < possibleBot)
			{
				possibleBot = VAL[i];
				posBotBars = i;
			}
			else if (VAL[i] >= possibleBot*(1 + (float)ZIG_PERCENT / 100))
			{
				curBotBars = posBotBars;
				posBotBars = -1;
				possibleTop = VAL[i];
				posTopBars = i;
				goal = SEARCHING_TOP;
				continue;
			}


			if (VAL[i] > possibleTop)
			{
				possibleTop = VAL[i];
				posTopBars = i;
			}
			else if (VAL[i] <= possibleTop*(1 - (float)ZIG_PERCENT / 100))
			{
				curTopBars = posTopBars;
				posTopBars = -1;
				possibleBot = VAL[i];
				posBotBars = i;
				goal = SEARCHING_BOT;
				continue;
			}
		}
		else if (goal == SEARCHING_TOP)
		{
			if (VAL[i] > possibleTop)
			{
				posTopBars = i;
				possibleTop = VAL[i];
				if (!criticalPnt)
					pfOUT[i] = i - curBotBars;
			}
			else 
			{
				if (!criticalPnt)
					pfOUT[i] = i - curBotBars;
				if (VAL[i] <= possibleTop*(1 - (float)ZIG_PERCENT / 100))
				{
					curTopBars = posTopBars;
					posTopBars = -1;
					posBotBars = i;
					possibleBot = VAL[i];
					goal = SEARCHING_BOT;
					continue;
				}
			}
		}
		else
		{ // SEARCHING_BOT
			if (VAL[i] < possibleBot)
			{
				posBotBars = i;
				possibleBot = VAL[i];
				if (!criticalPnt)
					pfOUT[i] = i - curBotBars;
			}
			else
			{
				
				if (VAL[i] >= possibleBot*(1 + (float)ZIG_PERCENT / 100))
				{
					if (criticalPnt)
						pfOUT[i] = 0;
					else 
					{
						for (int j = i; j >= posBotBars; j--)
						{
							pfOUT[j] = j - posBotBars;
						}
					}

					curBotBars = posBotBars;
					posBotBars = -1;
					possibleTop = VAL[i];
					posTopBars = i;
					goal = SEARCHING_TOP;
					continue;
				}
				else
				{
					if (!criticalPnt)
						pfOUT[i] = i - curBotBars;
				}

			}
		}
	}
}


void PEAK_BARS_ZIG(int DataLen, float* pfOUT, float* VAL, float* criticalPoint)
{
	// criticalPoint是指： 判定ZIG拐点 的那个决策点，也就是比拐点 要晚一点的那个位置，实现了5个点的拐点幅度
	bool criticalPnt = ((int)*criticalPoint == 1);

	memset(pfOUT, -1, DataLen * sizeof(float));

	float possibleBot(VAL[0]), possibleTop(VAL[0]);
	int posBotBars(0), posTopBars(0);
	int curBotBars(-1), curTopBars(-1);

	enum { UNSURE, SEARCHING_TOP, SEARCHING_BOT } goal = UNSURE;

	for (int i = 1; i < DataLen; i++)
	{
		static int modify_Done = 0;

		if (goal == UNSURE)
		{
			if (VAL[i] < possibleBot)
			{
				possibleBot = VAL[i];
				posBotBars = i;
			}
			else if (VAL[i] >= possibleBot*(1 + (float)ZIG_PERCENT / 100))
			{
				curBotBars = posBotBars;
				posBotBars = -1;
				possibleTop = VAL[i];
				posTopBars = i;
				goal = SEARCHING_TOP;
				continue;
			}


			if (VAL[i] > possibleTop)
			{
				possibleTop = VAL[i];
				posTopBars = i;
			}
			else if (VAL[i] <= possibleTop*(1 - (float)ZIG_PERCENT / 100))
			{
				curTopBars = posTopBars;
				posTopBars = -1;
				possibleBot = VAL[i];
				posBotBars = i;
				goal = SEARCHING_BOT;
				continue;
			}
		}
		else if (goal == SEARCHING_TOP)
		{
			if (VAL[i] > possibleTop)
			{
				posTopBars = i;
				possibleTop = VAL[i];
				if (!criticalPnt)
					pfOUT[i] = i - curTopBars;
			}
			else
			{
				if (VAL[i] <= possibleTop*(1 - (float)ZIG_PERCENT / 100))
				{
					if (criticalPnt)
						pfOUT[i] = 0;
					else
					{
						for (int j = i; j >= posTopBars; j--)
						{
							pfOUT[j] = j - posTopBars;
						}
					}

					curTopBars = posTopBars;
					posTopBars = -1;
					posBotBars = i;
					possibleBot = VAL[i];
					goal = SEARCHING_BOT;
					continue;
				}
				else 
				{
					if (!criticalPnt)
						pfOUT[i] = i - curTopBars;
				}
			}
		}
		else
		{ // SEARCHING_BOT
			if (!criticalPnt)
				pfOUT[i] = i - curTopBars;
			if (VAL[i] < possibleBot)
			{
				posBotBars = i;
				possibleBot = VAL[i];
			}
			else
			{
				if (VAL[i] >= possibleBot*(1 + (float)ZIG_PERCENT / 100))
				{
					curBotBars = posBotBars;
					posBotBars = -1;
					possibleTop = VAL[i];
					posTopBars = i;
					goal = SEARCHING_TOP;
					continue;
				}

			}
		}
	}
}

static void merge(int DataLen, float* pfOUT, float* VAL1, float* VAL2)
{
	for (int i = 0; i < DataLen; i++)
	{
		if (VAL1[i] == 0 && VAL2[i] == 0)
		{
			pfOUT[i] = 1;
		}
		else
		{
			pfOUT[i] = 0;
		}
	}
}

void FENBI(int DataLen, float* pfOUT, float* LOW, float* HIGH, float* JUNXIAN)
{
	float inflectionPoint = 0;
	float* MID = new float[DataLen] {0};
	for (int i = 0; i < DataLen; i++)
	{
		MID[i] = (LOW[i] + HIGH[i]) / 2;
	}

	float* peaks_junxian = new float[DataLen] {0};
	float* peaks_zig = new float[DataLen] {0};
	float* peaks = new float[DataLen] {0};

	PEAK_BARS_ZIG(DataLen, peaks_zig, MID, &inflectionPoint);
	PEAK_BARS_JUNXIAN(DataLen, peaks_junxian, MID, JUNXIAN);
	merge(DataLen, peaks, peaks_zig, peaks_junxian);

	float* trough_junxian = new float[DataLen] {0};
	float* trough_zig = new float[DataLen] {0};
	float* troughs = new float[DataLen] {0};

	TROUGH_BARS_ZIG(DataLen, trough_zig, MID, &inflectionPoint);
	TROUGH_BARS_JUNXIAN(DataLen, trough_junxian, MID, JUNXIAN);
	merge(DataLen, troughs, trough_zig, trough_junxian);

	memset(pfOUT, 0, DataLen * sizeof(float));
	for (int i = 0; i < DataLen; i++)
	{
		assert(((int)peaks[i] & (int)troughs[i]) == 0);
		pfOUT[i] = (peaks[i] ? 1 : (troughs[i] ? -1 : 0));
		
	}


	delete[] peaks, peaks_zig, peaks_junxian;
	delete[] troughs, trough_zig, trough_junxian;
	delete[] MID;
}

void DEBUG_BARS_JUNXIAN(int DataLen, float* pfOUT, float* LOW, float* HIGH, float* JUNXIAN)
{
	float* MID = new float[DataLen] {0};
	for (int i = 0; i < DataLen; i++)
	{
		MID[i] = (LOW[i] + HIGH[i]) / 2;
	}

	float* peaks_junxian = new float[DataLen] {0};
	float* trough_junxian = new float[DataLen] {0};

	PEAK_BARS_JUNXIAN(DataLen, peaks_junxian, MID, JUNXIAN);
	TROUGH_BARS_JUNXIAN(DataLen, trough_junxian, MID, JUNXIAN);

	memset(pfOUT, 0, DataLen * sizeof(float));
	for (int i = 0; i < DataLen; i++)
	{
		if (peaks_junxian[i] == 0)
			assert(trough_junxian[i] != 0);
		if (trough_junxian[i] == 0)
			assert(peaks_junxian[i] != 0);

		pfOUT[i] = (peaks_junxian[i] == 0 ? 1 : (trough_junxian[i] == 0 ? -1 : 0));
	}

	delete[] MID, peaks_junxian, trough_junxian;

}
void DEBUG_FENBI(int DataLen, float* pfOUT, float* LOW, float* HIGH, float* JUNXIAN)
{
	float* peaks_junxian = new float[DataLen] {0};
	float* trough_junxian = new float[DataLen] {0};

	PEAK_BARS_JUNXIAN(DataLen, peaks_junxian, HIGH, JUNXIAN);
	TROUGH_BARS_JUNXIAN(DataLen, trough_junxian, LOW, JUNXIAN);

	memset(pfOUT, 0, DataLen * sizeof(float));
	for (int i = 0; i < DataLen; i++)
	{
		if (peaks_junxian[i] == 0)
			assert(trough_junxian[i] != 0);
		if (trough_junxian[i] == 0)
			assert(peaks_junxian[i] != 0);

		pfOUT[i] = (peaks_junxian[i] == 0 ? 1 : (trough_junxian[i] == 0 ? -1 : 0));
	}

	delete[]  peaks_junxian, trough_junxian;

}

void DEBUG_BARS_ZIG(int DataLen, float* pfOUT, float* VAL, float* criticalPoint)
{
	float* peaks_zig = new float[DataLen] {0};
	float* trough_zig = new float[DataLen] {0};

	PEAK_BARS_ZIG(DataLen, peaks_zig, VAL, criticalPoint);
	TROUGH_BARS_ZIG(DataLen, trough_zig, VAL, criticalPoint);

	memset(pfOUT, 0, DataLen * sizeof(float));
	for (int i = 0; i < DataLen; i++)
	{
		pfOUT[i] = (peaks_zig[i] == 0 ? 1 : (trough_zig[i] == 0 ? -1 : 0));
	}


	delete[] peaks_zig, trough_zig;
}

//加载的函数
PluginTCalcFuncInfo g_CalcFuncSets[] =
{
	{ 1,(pPluginFUNC)&TOPRANGE_PERCENT },
	{ 2,(pPluginFUNC)&LOWRANGE_PERCENT },

	{ 3,(pPluginFUNC)&PEAK_BARS_JUNXIAN },
	{ 4,(pPluginFUNC)&TROUGH_BARS_JUNXIAN },

	{ 5,(pPluginFUNC)&PEAK_BARS_ZIG },
	{ 6,(pPluginFUNC)&TROUGH_BARS_ZIG },

	{ 7,(pPluginFUNC)&FENBI },

	{ 8,(pPluginFUNC)&DEBUG_BARS_JUNXIAN },
	{ 9,(pPluginFUNC)&DEBUG_FENBI },
	{ 10,(pPluginFUNC)&DEBUG_BARS_ZIG },

	{ 0,NULL },
};

//导出给TCalc的注册函数
BOOL RegisterTdxFunc(PluginTCalcFuncInfo** pFun)
{
	if (*pFun == NULL)
	{
		(*pFun) = g_CalcFuncSets;
		return TRUE;
	}
	return FALSE;
}
