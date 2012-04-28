//***************************************************************//
//* Blob analysis package  Version1.8 4 May 2008                *//
//* History:                                                    *//
//* - Version 1.0 8 August 2003                                 *//
//* - Version 1.2 3 January 2008                                *//
//* - Version 1.3 5 January 2008 Add BLOBCOLOR                  *//
//* - Version 1.4 13 January 2008 Add ROI function              *//
//* - Version 1.5 13 April 2008 Fix perimeter on Region 0       *//
//* - Version 1.6 1 May 2008 Reduce size of working storage     *//
//* - Version 1.7 2 May 2008 Speed up run code initialization   *//
//* - Version 1.8 4 May 2008 Fix bugs in perimeter comp & Reg 0 *//
//* - Version 2.0 3 Jan 2009 Add labeling functionality         *//
//*                                                             *//
//* Input: IplImage* binary image                               *//
//* Output: attributes of each connected region                 *//
//* Author: Dave Grossman                                       *//
//* Email: dgrossman2@earthlink.net                             *//
//* Acknowledgement: the algorithm has been around > 30 yrs     *//
//***************************************************************//

#ifndef blobs_h
#define blobs_h

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>

#include <iostream>
#include <vector>

// defines for blob array sizes and indices
#ifdef ROWS
#define BLOBROWCOUNT ROWS
#endif

#ifdef COLS
#define BLOBCOLCOUNT COLS
#endif

#ifndef BLOBROWCOUNT
#define BLOBROWCOUNT 2040
#endif

#ifndef BLOBCOLCOUNT
#define BLOBCOLCOUNT 2040
#endif

#ifndef BLOBTOTALCOUNT
#define BLOBTOTALCOUNT (BLOBROWCOUNT + BLOBCOLCOUNT) * 5
#endif

#define WORKINGSTORAGE 2*BLOBCOLCOUNT

#define BLOBPARENT 0
#define BLOBCOLOR 1
#define BLOBAREA 2
#define BLOBPERIMETER 3
#define BLOBSUMX 4
#define BLOBSUMY 5
#define BLOBSUMXX 6
#define BLOBSUMYY 7
#define BLOBSUMXY 8
#define BLOBMINX 9
#define BLOBMAXX 10
#define BLOBMINY 11
#define BLOBMAXY 12
#define BLOBDATACOUNT 13

// Global variables to avoid memory leak
int WorkingStorage[WORKINGSTORAGE];				// Working storage
float RegionData[BLOBTOTALCOUNT][BLOBDATACOUNT];	// Blob result array

// Subroutine prototypes
void PrintRegionDataArray(float[BLOBTOTALCOUNT][BLOBDATACOUNT]);
void Subsume(float[BLOBTOTALCOUNT][BLOBDATACOUNT], int, int[BLOBTOTALCOUNT], int, int);
int BlobAnalysis(IplImage*, int[WORKINGSTORAGE], float[BLOBTOTALCOUNT][BLOBDATACOUNT], int, int, int, int, uchar, int, IplImage* = 0);
void Fill(int, int, int, int, int[BLOBTOTALCOUNT], IplImage*);

// Transfer fields from subsumed region to correct one
void Subsume(float RegionData[BLOBTOTALCOUNT][BLOBDATACOUNT],
			int HighRegionNum,
			int SubsumedRegion[BLOBTOTALCOUNT],
			int HiNum,
			int LoNum)
{
	if(HiNum > BLOBTOTALCOUNT) return;

	// cout << "Subsuming from " << HiNum << " to " << LoNum << endl;

	int iTargetTest;
	int iTargetValid = LoNum;

	while(true)	// Follow subsumption chain to lowest number source
	{
		iTargetTest = SubsumedRegion[iTargetValid];
		if(iTargetTest < 0) break;
		iTargetValid = iTargetTest;
	}
	LoNum = iTargetValid;

	int i;
	for(i = BLOBAREA; i < BLOBDATACOUNT; i++)	// Skip over BLOBCOLOR
	{
		if(i == BLOBMINX || i == BLOBMINY)
		{
			if(RegionData[LoNum][i] > RegionData[HiNum][i]) { RegionData[LoNum][i] = RegionData[HiNum][i]; }
		}
		else if(i == BLOBMAXX || i == BLOBMAXY)
		{
		 	if(RegionData[LoNum][i] < RegionData[HiNum][i]) { RegionData[LoNum][i] = RegionData[HiNum][i]; }
		}
		else // Area, Perimeter, SumX, SumY, SumXX, SumYY, SumXY
		{
			RegionData[LoNum][i] += RegionData[HiNum][i];
		}
	}
	
	SubsumedRegion[HiNum] = LoNum;	// Mark dead region number for future compression
}

void PrintRegionDataArray(float RegionData[BLOBTOTALCOUNT][BLOBDATACOUNT])
{
	int ThisRegion;
	printf("\nRegionDataArray\nParent-Color--Area---Perimeter---X-----Y-------2nd Moments-------BoundingBox\n");
	for(ThisRegion = 0; ThisRegion < BLOBTOTALCOUNT; ThisRegion++)
	{
		if(ThisRegion > 0 && RegionData[ThisRegion][0] < 0) break;
		if(RegionData[ThisRegion][BLOBAREA] <= 0) break;
		printf("Region=%3d: ", ThisRegion);
		printf("%3d  ", (int) RegionData[ThisRegion][BLOBPARENT]);		//parent
		printf("%3d  ", (int) RegionData[ThisRegion][BLOBCOLOR]);		//color
		printf("%6d  ", (int) RegionData[ThisRegion][BLOBAREA]);		//area
		printf("%6d  ", (int) RegionData[ThisRegion][BLOBPERIMETER]);	//perimeter
		printf("%6.1f  ", (float) RegionData[ThisRegion][BLOBSUMX]);	//sumx
		printf("%6.1f  ", (float) RegionData[ThisRegion][BLOBSUMY]);	//sumy

		printf("%6.1f  ", (float) RegionData[ThisRegion][BLOBSUMXX]);	//sumxx
		printf("%6.1f  ", (float) RegionData[ThisRegion][BLOBSUMYY]);	//sumyy
		printf("%6.1f  ", (float) RegionData[ThisRegion][BLOBSUMXY]);	//sumxy

		printf("%6.1f  ", (float) RegionData[ThisRegion][BLOBMINX]);	//minx
		printf("%6.1f  ", (float) RegionData[ThisRegion][BLOBMAXX]);	//maxx
		printf("%6.1f  ", (float) RegionData[ThisRegion][BLOBMINY]);	//miny
		printf("%6.1f  ", (float) RegionData[ThisRegion][BLOBMAXY]);	//maxy
		printf("\n");
	}
	printf("\n");
}

int BlobAnalysis(IplImage* ImageHdr,	// input image
	int Transition[WORKINGSTORAGE],	// array for intermediate result (pass in WorkingStorage)
	float RegionData[BLOBTOTALCOUNT][BLOBDATACOUNT],	// region data array to be output
	int Col0, int Row0,					// start of ROI
	int Cols, int Rows,					// size of ROI (+2 for Tran array)
	uchar Border,						// border color
	int MinArea,						// max trans in any row
	IplImage* LabeledImage)
{
 	// Display Gray image
	//cvNamedWindow("BlobInput", CV_WINDOW_AUTOSIZE);
	//cvShowImage("BlobInput", ImageHdr);

	if(Cols > BLOBCOLCOUNT) { return(-1); } // Bounds check - Error in column count
	if(Rows > BLOBROWCOUNT) { return(-2); } // Bounds check - Error in row count
	
	char* Image = ImageHdr->imageData;
	int WidthStep = ImageHdr->widthStep; 

	int LabelFlag = 0;
	if(LabeledImage != 0) LabelFlag = 1;

	// row 0 and row Rows+1 represent the border
	int iTran, Tran;				// Data for a given run
	uchar ThisCell, LastCell;		// Contents within this row
	int ImageOffset = WidthStep * Row0 - WidthStep - 1;	// Performance booster avoids mult
	int	BorderCode = 0; if(Border > 128) { BorderCode = 1; }	// 0 = black; 1 = white
	int i;

	// Initialize Transition array
	int LastOffset = Cols;		// For performance to avoid multiplication
	int ThisOffset = 0;		// For performance to avoid multiplication
	for(i = 0; i < Cols; i++) { Transition[ThisOffset + i] = 0; };
	Transition[ThisOffset] = Cols+2;
	Transition[ThisOffset+1] = -1;	// Flag end of run code

	// Process transition code depending on Last row and This row
	//
	// Last ---++++++--+++++++++++++++-----+++++++++++++++++++-----++++++-------+++---
	// This -----+++-----++++----+++++++++----+++++++---++------------------++++++++--
	//
	// There are various possibilities:
	//
	// Case     1       2       3       4       5       6       7       8
	// Last |xxx    |xxxxoo |xxxxxxx|xxxxxxx|ooxxxxx|ooxxx  |ooxxxxx|    xxx|
	// This |    yyy|    yyy|  yyyy |  yyyyy|yyyyyyy|yyyyyyy|yyyy   |yyyy   |
	// Here o is optional
	// 
	// Here are the primitive tests to distinguish these 6 cases:
	//   A) Last end < This start - 1 OR NOT		Note: -1
	//   B) This end < Last start OR NOT
	//   C) Last start < This start OR NOT
	//   D) This end < Last end OR NOT
	//   E) This end = Last end OR NOT
	//
	// Here is how to use these tests to determine the case:
	//   Case 1 = A [=> NOT B AND C AND NOT D AND NOT E]
	//   Case 2 = C AND NOT D AND NOT E [AND NOT A AND NOT B]
	//   Case 3 = C AND D [=> NOT E] [AND NOT A AND NOT B]
	//   Case 4 = C AND NOT D AND E [AND NOT A AND NOT B]
	//   Case 5 = NOT C AND E [=> NOT D] [AND NOT A AND NOT B]
	//   Case 6 = NOT C AND NOT D AND NOT E [AND NOT A AND NOT B]
	//   Case 7 = NOT C AND D [=> NOT E] [AND NOT A AND NOT B]
	//   Case 8 = B [=> NOT A AND NOT C AND D AND NOT E]
	//
	// In cases 2,3,4,5,6,7 the following additional test is needed:
	//   Match) This color = Last color OR NOT
	//
	// In cases 5,6,7 the following additional test is needed:
	//   Known) This region was already matched OR NOT
	//
	// Here are the main tests and actions:
	//   Case 1: LastIndex++;
	//   Case 2: if(Match) {y = x;}
	//           LastIndex++;
	//   Case 3: if(Match) {y = x;}
	//           else {y = new}
	//           ThisIndex++;
	//   Case 4: if(Match) {y = x;}
	//           else {y = new}
	//           LastIndex++;
	//           ThisIndex++;
	//   Case 5: if(Match AND NOT Known) {y = x}
	//           else if(Match AND Known) {Subsume(x,y)}
	//           LastIndex++;ThisIndex++
	//   Case 6: if(Match AND NOT Known) {y = x}
	//           else if(Match AND Known) {Subsume(x,y)}
	//           LastIndex++;
	//   Case 7: if(Match AND NOT Known) {y = x}
	//           else if(Match AND Known) {Subsume(x,y)}
	//           ThisIndex++;
	//   Case 8: ThisIndex++;

	// BLOBTOTALCOUNT is max num of regions incl all temps and background
	// BLOBROWCOUNT is the number of rows in the image
	// BLOBCOLCOUNT is the number of columns in the image
	// BLOBDATACOUNT is number of data elements for each region as follows:
	// BLOBPARENT 0	these are the respective indices for the data elements
	// BLOBCOLOR 1		0=background; 1=non-background
	// BLOBAREA 2
	// BLOBPERIMETER 3
	// BLOBSUMX 4		means
	// BLOBSUMY 5
	// BLOBSUMXX 6		2nd moments
	// BLOBSUMYY 7
	// BLOBSUMXY 8
	// BLOBMINX 9		bounding rectangle
	// BLOBMAXX 10
	// BLOBMINY 11
	// BLOBMAXY 12

	int SubsumedRegion[BLOBTOTALCOUNT];			// Blob result array
	int RenumberedRegion[BLOBTOTALCOUNT];		// Blob result array
	
	float ThisParent;	// These data can change when the line is current
	float ThisArea;
	float ThisPerimeter;
	float ThisSumX;
	float ThisSumY;
	float ThisSumXX;
	float ThisSumYY;
	float ThisSumXY;
	float ThisMinX;
	float ThisMaxX;
	float ThisMinY;
	float ThisMaxY;
	float LastPerimeter;	// This is the only data for retroactive change
	
	int HighRegionNum = 0;
	int RegionNum = 0;
	int ErrorFlag = 0;
	
	int LastRow, ThisRow;			// Row number
	int LastStart, ThisStart;		// Starting column of run
	int LastEnd, ThisEnd;			// Ending column of run
	int LastColor, ThisColor;		// Color of run

	int LastIndex, ThisIndex;		// Which run are we up to?
	int LastCode, ThisCode; // Index flipped if Border is 255;
	int LastIndexCount, ThisIndexCount;	// Out of these runs
	int LastRegionNum, ThisRegionNum;	// Which assignment?
	int LastRegion[BLOBCOLCOUNT+2];	// Row assignment of region number
	int ThisRegion[BLOBCOLCOUNT+2];	// Row assignment of region number
	
	int ComputeData;
	int j;

	for(i = 0; i < BLOBTOTALCOUNT; i++)	// Initialize result arrays
	{
		RenumberedRegion[i] = i;			// Initially no region is renumbered
		SubsumedRegion[i] = -1;				// Flag indicates region is not subsumed
		RegionData[i][0] = (float) -1;		// Flag indicates null region
		for(j = 1; j < BLOBDATACOUNT; j++)
		{
			if(j == BLOBMINX || j == BLOBMINY) RegionData[i][j] = (float) 1000000.0;
			else RegionData[i][j] = (float) 0.0;
		}
	}
	for(i = 0; i < BLOBROWCOUNT + 2; i++)	// Initialize result arrays
	{
		LastRegion[i] = -1;
		ThisRegion[i] = -1;
	}

	RegionData[0][BLOBPARENT] = (float) -1;
	RegionData[0][BLOBAREA] = (float) Cols+2;
	RegionData[0][BLOBPERIMETER] = (float) Cols + 2;		// Only the previous row

	ThisIndexCount = 1;
	ThisRegion[0] = 0;	// Border region

	// Initialize left border column
	for(i = Row0 + 1; i < Row0 + Rows + 2; i++) { ThisRegion[i] = -1; } // Flag as uninit

	//----  Loop over all rows -------------------------------------------
	for(ThisRow = Row0 + 1; ThisRow < Row0 + Rows + 2; ThisRow++)
	{

		// Toggle rows in Transition array
		LastOffset = Cols - LastOffset;
		ThisOffset = Cols - ThisOffset;

		for(i = 0; i < Cols; i++)	// Reset new row to 0
		{
			int T = Transition[ThisOffset+i];
			Transition[ThisOffset+i] = 0;
			if(T < 0) break;		// Remainder of row is already 0
		};
		Transition[ThisOffset] = Cols+2; 
		Transition[ThisOffset+1] = -1; 
	
		// Until border row, fill transition array for new row 
		if(ThisRow < Row0 + Rows +1)
		{
			ImageOffset += WidthStep;	// Performance booster to avoid multiplication
			iTran = 0;				// Index into Transition array
			Tran = 0;				// No transitions at row start

			LastCell = Border;
			for(i = Col0; i < Col0 + Cols + 2; i++)	// Scan new row of Bordered image
			{
				if(i == Col0 || i == Col0 + Cols + 1) ThisCell = Border;
				else ThisCell = Image[ImageOffset + i];
				if(ThisCell != LastCell)	// Different color
				{
					Transition[ThisOffset + iTran] = Tran;	// Save completed Tran
					iTran++;					// Prepare new index
					if(iTran > Cols-2) break;	// Bounds check
					LastCell = ThisCell;		// With this color
				}
				Tran++;	// Tran continues
			}
			Transition[ThisOffset + iTran] = Tran;	// Save completed run
			Transition[ThisOffset + iTran + 1] = -1;	// Flag end of run code
		}

		// Transition[ThisOffset ... ThisOffset+Cols-1] now contains something that looks
		// like "12,23,27,84,96,-1,0,0,...0" where the -1 flags the end of run code, and
		// the preceding numbers are column numbers of transitions.

		// Start region analysis based on run codes
		LastRow = ThisRow - 1;
		LastIndexCount = ThisIndexCount;	// total runs >= 0
		ThisIndex = LastIndex = 0;

		int EndLast = 0;
		int EndThis = 0;
		for(j = 0; j < Cols; j++)
		{
			int Index = ThisOffset + j;
			int Tran = Transition[Index];			// Run length
			if(Tran > 0) ThisIndexCount = j + 1;	 

			if(ThisRegion[j] == -1)  { EndLast = 1; }
			if(Tran < 0) { EndThis = 1; }

			if(EndLast > 0 && EndThis > 0) { break; }

			LastRegion[j] = ThisRegion[j];
			ThisRegion[j] = -1;		// Flag indicates region is not initialized
		}

		int MaxIndexCount = LastIndexCount;
		if(ThisIndexCount > MaxIndexCount) MaxIndexCount = ThisIndexCount;

		// Main loop over runs within Last and This rows
		while (LastIndex < LastIndexCount && ThisIndex < ThisIndexCount)
		{
			ComputeData = 0;
		
			if(LastIndex == 0) LastStart = 0;
			else LastStart = Transition[LastOffset + LastIndex - 1];
			LastEnd = Transition[LastOffset + LastIndex] - 1;
			LastCode = LastIndex + BorderCode;
			LastColor = LastCode - 2 * (LastCode / 2);
			LastRegionNum = LastRegion[LastIndex];

			if(ThisIndex == 0) ThisStart = 0;
			else ThisStart = Transition[ThisOffset + ThisIndex - 1];
			ThisEnd = Transition[ThisOffset + ThisIndex] - 1;
			ThisCode = ThisIndex + BorderCode;
			ThisColor = ThisCode - 2 * (ThisCode / 2);
			ThisRegionNum = ThisRegion[ThisIndex];

			if(ThisRegionNum >= BLOBTOTALCOUNT)	// Bounds check
			{
				ErrorFlag = -2; // Too many regions found - You must increase BLOBTOTALCOUNT
				break;
			}

			int TestA = (LastEnd < ThisStart - 1);	// initially false
			int TestB = (ThisEnd < LastStart);		// initially false
			int TestC = (LastStart < ThisStart);	// initially false
			int TestD = (ThisEnd < LastEnd);
			int TestE = (ThisEnd == LastEnd);

			int TestMatch = (ThisColor == LastColor);		// initially true
			int TestKnown = (ThisRegion[ThisIndex] >= 0);	// initially false

			int Case = 0;
			if(TestA) Case = 1;
			else if(TestB) Case = 8;
			else if(TestC)
			{
				if(TestD) Case = 3;
				else if(!TestE) Case = 2;
				else Case = 4;
			}
			else
			{
				if(TestE) Case = 5;
				else if(TestD) Case = 7;
				else Case = 6;
			}

			// Initialize common variables
			ThisArea = (float) 0.0;
			ThisSumX = ThisSumY = (float) 0.0;
			ThisSumXX = ThisSumYY = ThisSumXY = (float) 0.0;
			ThisMinX = ThisMinY = (float) 1000000.0;
			ThisMaxX = ThisMaxY = (float) -1.0;
			LastPerimeter = ThisPerimeter = (float) 0.0;
			ThisParent = (float) -1;

			// Determine necessary action and take it
			switch (Case)
			{ 
				case 1: //|xxx    |
					  	//|    yyy|

					printf("ERROR: IMPOSSIBLE CASE 1\n"); exit(2);
					LastIndex++;
					break;
					
					
				case 2: //|xxxxoo |
					  	//|    yyy|
					
					if(TestMatch)	// Same color
					{
						ThisRegionNum = LastRegionNum;
						ThisArea = (float) ThisEnd - ThisStart + 1;
					}
					else	// Different color but not previously known
					{
						ThisParent = (float) LastRegionNum;
						ThisRegionNum = ++HighRegionNum;
						ThisArea = (float) ThisEnd - ThisStart + 1;
						RegionData[LastRegionNum][BLOBPERIMETER] += 2 + LastEnd - ThisStart;
						ThisPerimeter = ThisArea;
					}

					ThisRegion[ThisIndex] = ThisRegionNum;
					ComputeData = 1;	// Fixing a bug. Previously this was only done for TestMatch
					LastIndex++;
					break;
					
					
				case 3: //|xxxxxxx| 
					  	//|  yyyy |
					
					if(TestMatch)	// Same color
					{
						ThisRegionNum = LastRegionNum;
						ThisArea = (float) ThisEnd - ThisStart + 1;
					}
					else		// Different color => New region
					{
						ThisParent = (float) LastRegionNum;
						ThisRegionNum = ++HighRegionNum;
						ThisArea = (float) ThisEnd - ThisStart + 1;
						RegionData[LastRegionNum][BLOBPERIMETER] += ThisArea;
						ThisPerimeter = ThisArea;
					}
					
					ThisRegion[ThisIndex] = ThisRegionNum;
					ComputeData = 1;
					ThisIndex++;
					break;
					
					
				case 4:	//|xxxxxxx|
						//|  yyyyy|
					
					if(TestMatch)	// Same color
					{
						ThisRegionNum = LastRegionNum;
						ThisArea = (float) ThisEnd - ThisStart + 1;
						RegionData[LastRegionNum][BLOBPERIMETER] += 1;				// LastEnd
					}
					else		// Different color => New region
					{
						ThisParent = (float) LastRegionNum;
						ThisRegionNum = ++HighRegionNum;
						ThisArea = (float) ThisEnd - ThisStart + 1;
						RegionData[LastRegionNum][BLOBPERIMETER] += 1 + ThisArea;	// LastEnd
						ThisPerimeter = 2 + 2 * ThisArea;
					}
					
					ThisRegion[ThisIndex] = ThisRegionNum;
					ComputeData = 1;
					LastIndex++;
					ThisIndex++;
					break;
					
					
				case 5:	//|ooxxxxx|
						//|yyyyyyy|
					
					if(TestMatch && !TestKnown)	// Same color and unknown
					{
						ThisRegionNum = LastRegionNum;
						ThisArea = (float) ThisEnd - ThisStart + 1;
						RegionData[LastRegionNum][BLOBPERIMETER] += 2;	// Last Start and LastEnd
						ComputeData = 1;
					}
					else if(!TestMatch && !TestKnown)	// Different color and unknown => new region
					{
						ThisParent = (float) LastRegionNum;
						ThisRegionNum = ++HighRegionNum;
						ThisArea = (float) ThisEnd - ThisStart + 1;
						RegionData[LastRegionNum][BLOBPERIMETER] += 3 + LastEnd - LastStart;	// LastStart&Bot&LastEnd
						ThisPerimeter = LastEnd - LastStart + 1;
						ComputeData = 1;
					}
					else if(!TestMatch && TestKnown)	// Different color and known => new region
					{
						RegionData[LastRegionNum][BLOBPERIMETER] += 3 + LastEnd - LastStart;	//LastStart&Bot&LastEnd
						ThisPerimeter = LastEnd - LastStart + 1;
						//printf("ERROR: IMPOSSIBLE CASE 5 0 1\n"); exit(1);
					}
					else if(TestMatch && TestKnown)		// Same color and known
					{
						RegionData[LastRegionNum][BLOBPERIMETER] += 2;	// Last Start and LastEnd
						if(ThisRegionNum > LastRegionNum)
						{
							int iOld;
							Subsume(RegionData, HighRegionNum, SubsumedRegion, ThisRegionNum, LastRegionNum);
							for(iOld = 0; iOld < MaxIndexCount; iOld++)
							{
								if(ThisRegion[iOld] == ThisRegionNum) ThisRegion[iOld] = LastRegionNum;
								if(LastRegion[iOld] == ThisRegionNum) LastRegion[iOld] = LastRegionNum;
							}					
							ThisRegionNum = LastRegionNum;
						}
						else if(ThisRegionNum < LastRegionNum)
						{
							int iOld;
							Subsume(RegionData, HighRegionNum, SubsumedRegion, LastRegionNum, ThisRegionNum);
							for(iOld = 0; iOld < MaxIndexCount; iOld++)
							{
								if(ThisRegion[iOld] == LastRegionNum) ThisRegion[iOld] = ThisRegionNum;
								if(LastRegion[iOld] == LastRegionNum) LastRegion[iOld] = ThisRegionNum;
							}					
							LastRegionNum = ThisRegionNum;
						}
					}

					ThisRegion[ThisIndex] = ThisRegionNum;
					LastRegion[LastIndex] = LastRegionNum;
					LastIndex++;
					ThisIndex++;
					break;
					
					
				case 6:	//|ooxxx  |
						//|yyyyyyy|

					if(TestMatch && !TestKnown)
					{
						ThisRegionNum = LastRegionNum;
						ThisArea = (float) ThisEnd - ThisStart + 1;
						RegionData[LastRegionNum][BLOBPERIMETER] += 1;						//LastEnd
						if(ThisStart == LastStart) RegionData[LastRegionNum][BLOBPERIMETER] += 1;	//LastStart
						ComputeData = 1;
					}
					if(!TestMatch && !TestKnown)
					{
						RegionData[LastRegionNum][BLOBPERIMETER] += 3 + LastEnd - LastStart;		//Bot&LastEnd
						if(ThisStart == LastStart) RegionData[LastRegionNum][BLOBPERIMETER] += 1;	//LastStart
						ThisPerimeter = LastEnd - LastStart + 1;
						ComputeData = 1;
					}
					if(!TestMatch && TestKnown)	// Different color and known => new region
					{
						RegionData[LastRegionNum][BLOBPERIMETER] += 3 + LastEnd - LastStart;		//Bot&LastEnd
						if(ThisStart == LastStart) RegionData[LastRegionNum][BLOBPERIMETER] += 1;	//LastStart
						ThisPerimeter = LastEnd - LastStart + 1;
					}
					else if(TestMatch && TestKnown)
					{
						RegionData[LastRegionNum][BLOBPERIMETER] += 1;						//LastEnd
						if(ThisStart == LastStart) RegionData[LastRegionNum][BLOBPERIMETER] += 1;	//LastStart
						if(ThisRegionNum > LastRegionNum)
						{
							int iOld;
							Subsume(RegionData, HighRegionNum, SubsumedRegion, ThisRegionNum, LastRegionNum);
							for(iOld = 0; iOld < MaxIndexCount; iOld++)
							{
								if(ThisRegion[iOld] == ThisRegionNum) ThisRegion[iOld] = LastRegionNum;
								if(LastRegion[iOld] == ThisRegionNum) LastRegion[iOld] = LastRegionNum;
							}					
							ThisRegionNum = LastRegionNum;
						}
						else if(ThisRegionNum < LastRegionNum)
						{
							Subsume(RegionData, HighRegionNum, SubsumedRegion, LastRegionNum, ThisRegionNum);
							int iOld;
							for(iOld = 0; iOld < MaxIndexCount; iOld++)
							{
								if(ThisRegion[iOld] == LastRegionNum) ThisRegion[iOld] = ThisRegionNum;
								if(LastRegion[iOld] == LastRegionNum) LastRegion[iOld] = ThisRegionNum;
							}					
							LastRegionNum = ThisRegionNum;
						}
					}

					ThisRegion[ThisIndex] = ThisRegionNum;
					LastRegion[LastIndex] = LastRegionNum;
					LastIndex++;
					break;
					
					
				case 7:	//|ooxxxxx|
						//|yyyy   |
					
					if(TestMatch && !TestKnown)
					{
						ThisRegionNum = LastRegionNum;
						ThisArea = (float) ThisEnd - ThisStart + 1;
						if(ThisStart == LastStart) RegionData[LastRegionNum][BLOBPERIMETER] += 1;	// LastStart
						ComputeData = 1;
					}
					else if(!TestMatch && !TestKnown)	// Different color and unknown => new region
					{
						ThisParent = (float) LastRegionNum;
						ThisRegionNum = ++HighRegionNum;
						ThisArea = (float) ThisEnd - ThisStart + 1;
						RegionData[LastRegionNum][BLOBPERIMETER] += 1 + ThisEnd - LastStart;		// Bot
						if(ThisStart == LastStart) RegionData[LastRegionNum][BLOBPERIMETER] += 1;	// LastStart
						ThisPerimeter = ThisArea;
						ComputeData = 1;
					}
					else if(!TestMatch && TestKnown)
					{
						RegionData[LastRegionNum][BLOBPERIMETER] += 1 + ThisEnd - LastStart;		// Bot
						if(ThisStart == LastStart) RegionData[LastRegionNum][BLOBPERIMETER] += 1;	// LastStart
						ThisPerimeter = ThisArea;
					}
					else if(TestMatch && TestKnown)
					{

						if(ThisStart == LastStart) RegionData[LastRegionNum][BLOBPERIMETER] += 1;	// LastStart
						if(ThisRegionNum > LastRegionNum)
						{
							Subsume(RegionData, HighRegionNum, SubsumedRegion, ThisRegionNum, LastRegionNum);
							int iOld;
							for(iOld = 0; iOld < MaxIndexCount; iOld++)
							{
								if(ThisRegion[iOld] == ThisRegionNum) ThisRegion[iOld] = LastRegionNum;
								if(LastRegion[iOld] == ThisRegionNum) LastRegion[iOld] = LastRegionNum;
							}					
							ThisRegionNum = LastRegionNum;
						}
						else if(ThisRegionNum < LastRegionNum)
						{
							Subsume(RegionData, HighRegionNum, SubsumedRegion, LastRegionNum, ThisRegionNum);
							int iOld;
							for(iOld = 0; iOld < MaxIndexCount; iOld++)
							{
								if(ThisRegion[iOld] == LastRegionNum) ThisRegion[iOld] = ThisRegionNum;
								if(LastRegion[iOld] == LastRegionNum) LastRegion[iOld] = ThisRegionNum;
							}					
							LastRegionNum = ThisRegionNum;
						}
					}

					ThisRegion[ThisIndex] = ThisRegionNum;
					LastRegion[LastIndex] = LastRegionNum;
					ThisIndex++;
					break;
					
				case 8:	//|    xxx|
						//|yyyy   |
					
					ThisIndex++;
					break;
					
				default:
					ErrorFlag = -1;	// Impossible case 
					break;
			}	// end switch case
			if(ErrorFlag != 0) break;

			if(ComputeData > 0)
			{
				int k;
				for(k = ThisStart; k <= ThisEnd; k++)
				{
					ThisSumX += (float) (k - 1);
					ThisSumXX += (float) (k - 1) * (k - 1);
				}
				float ImageRow = (float) (ThisRow - 1);

				ThisSumXY = ThisSumX * ImageRow;
				ThisSumY = ThisArea * ImageRow;
				ThisSumYY = ThisSumY * ImageRow;
					
				if(ThisStart - 1 < (int) ThisMinX) ThisMinX = (float) (ThisStart - 1);
				if(ThisMinX < (float) 0.0) ThisMinX = (float) 0.0;
				if(ThisEnd - 1 > (int) ThisMaxX) ThisMaxX = (float) (ThisEnd - 1);

				if(ImageRow < ThisMinY) ThisMinY = ImageRow;
				if(ThisMinY < (float) 0.0) ThisMinY = (float) 0.0;
				if(ImageRow > ThisMaxY) ThisMaxY = ImageRow;
			}

			if(ThisRegionNum >= 0)
			{

				if(ThisRegionNum >= BLOBTOTALCOUNT) // Too many regions found - You must increase BLOBTOTALCOUNT
				{
					ErrorFlag = -2;
					break;
				}

				if(ThisParent >= 0) { RegionData[ThisRegionNum][BLOBPARENT] = (float) ThisParent; }
				RegionData[ThisRegionNum][BLOBCOLOR] = (float) ThisColor;
				RegionData[ThisRegionNum][BLOBAREA] += ThisArea;
				RegionData[ThisRegionNum][BLOBPERIMETER] += ThisPerimeter;


				if(ComputeData > 0)
				{
					RegionData[ThisRegionNum][BLOBSUMX] += ThisSumX;
					RegionData[ThisRegionNum][BLOBSUMY] += ThisSumY;
					RegionData[ThisRegionNum][BLOBSUMXX] += ThisSumXX;
					RegionData[ThisRegionNum][BLOBSUMYY] += ThisSumYY;
					RegionData[ThisRegionNum][BLOBSUMXY] += ThisSumXY;
					if(RegionData[ThisRegionNum][BLOBMINX] > ThisMinX) RegionData[ThisRegionNum][BLOBMINX] = ThisMinX;
					if(RegionData[ThisRegionNum][BLOBMAXX] < ThisMaxX) RegionData[ThisRegionNum][BLOBMAXX] = ThisMaxX;
					if(RegionData[ThisRegionNum][BLOBMINY] > ThisMinY) RegionData[ThisRegionNum][BLOBMINY] = ThisMinY;
					if(RegionData[ThisRegionNum][BLOBMAXY] < ThisMaxY) RegionData[ThisRegionNum][BLOBMAXY] = ThisMaxY;
				}

			}

		}	// end Main loop
		if(ErrorFlag != 0) break;

	}	// end Loop over all rows
	RegionData[0][BLOBPERIMETER] += Cols + 4;	// Termination of border

	//-----------------------------------------------------------------------------------------------

	// Subsume regions that have too small area
	int HiNum;
	for(HiNum = HighRegionNum; HiNum > 0; HiNum--)
	{
		
		if(SubsumedRegion[HiNum] < 0 && RegionData[HiNum][BLOBAREA] < (float) MinArea)
		{
			Subsume(RegionData, HighRegionNum, SubsumedRegion, HiNum, (int) RegionData[HiNum][BLOBPARENT]);
		}
	}

	// Compress region numbers to eliminate subsumed regions
	int RawHighRegionNum = HighRegionNum;	// Save for labeling
	int iOld;
	int iNew = 1;
	for(iOld = 1; iOld <= HighRegionNum; iOld++)
	{
		if(SubsumedRegion[iOld] >= 0) {	continue; }	// Region subsumed, empty, no further action
		else
		{
			int iTargetTest;
			int iTargetValid = (int) RegionData[iOld][BLOBPARENT];

			while(true)	// Follow subsumption chain
			{
				iTargetTest = SubsumedRegion[iTargetValid];
				if(iTargetTest < 0) break;
				iTargetValid = iTargetTest;
			}
			RegionData[iOld][BLOBPARENT] = (float) RenumberedRegion[iTargetValid];

			// Move data from old region number to new region number
			int j;
			for(j = 0; j < BLOBDATACOUNT; j++) { RegionData[iNew][j] = RegionData[iOld][j]; }
			RenumberedRegion[iOld] = iNew;
			iNew++;
		}
	}
	HighRegionNum = iNew - 1;				// Update where the data ends
	RegionData[HighRegionNum + 1][0] = -1;	// and set end of array flag

	// Normalize summation fields into moments 
	for(ThisRegionNum = 0; ThisRegionNum <= HighRegionNum; ThisRegionNum++)
	{
		// Extract fields
		float Area = RegionData[ThisRegionNum][BLOBAREA];
		float SumX = RegionData[ThisRegionNum][BLOBSUMX];
		float SumY = RegionData[ThisRegionNum][BLOBSUMY];
		float SumXX = RegionData[ThisRegionNum][BLOBSUMXX];
		float SumYY = RegionData[ThisRegionNum][BLOBSUMYY];
		float SumXY = RegionData[ThisRegionNum][BLOBSUMXY];
	
		// Get averages
		SumX /= Area;
		SumY /= Area;
		SumXX /= Area;
		SumYY /= Area;
		SumXY /= Area;

		// Create moments
		SumXX -= SumX * SumX;
		SumYY -= SumY * SumY;
		SumXY -= SumX * SumY;
		if(SumXY > -1.0E-14 && SumXY < 1.0E-14)
		{
			SumXY = (float) 0.0; // Eliminate roundoff error
		}
		RegionData[ThisRegionNum][BLOBSUMX] = SumX;
		RegionData[ThisRegionNum][BLOBSUMY] = SumY;
		RegionData[ThisRegionNum][BLOBSUMXX] = SumXX;
		RegionData[ThisRegionNum][BLOBSUMYY] = SumYY;
		RegionData[ThisRegionNum][BLOBSUMXY] = SumXY;

		RegionData[ThisRegionNum][BLOBSUMX] += Col0;
		RegionData[ThisRegionNum][BLOBMINX] += Col0;
		RegionData[ThisRegionNum][BLOBMAXX] += Col0;
	}

	for(ThisRegionNum = HighRegionNum; ThisRegionNum > 0 ; ThisRegionNum--)
	{
		// Subtract interior perimeters
		int ParentRegionNum = (int) RegionData[ThisRegionNum][BLOBPARENT];
		RegionData[ParentRegionNum][BLOBPERIMETER] -= RegionData[ThisRegionNum][BLOBPERIMETER];
	}

	RegionData[HighRegionNum+1][BLOBPARENT] = -2;


	if(LabelFlag)	// New code to generate labeled image
	{
		// Compute array for relabeling every region number
		int RelabeledRegion[BLOBTOTALCOUNT];
		for(i=0; i <= RawHighRegionNum; i++)
		{
			int iTargetValid = i;
			while(true)		// Follow subsumption chain to lowest number source
			{
				int iTargetTest = SubsumedRegion[iTargetValid];
				if(iTargetTest < 0 || iTargetTest == iTargetValid) break;
				iTargetValid = iTargetTest;
			}
			RelabeledRegion[i] = RenumberedRegion[iTargetValid];
		}

		//---- Prepare for 2nd loop over all rows -----------------------------------------------
		int HighRegionNum2 = 0;
		ImageOffset = WidthStep * Row0 - WidthStep - 1;
		RegionNum = 0;
		LastOffset = Cols;		// For performance to avoid multiplication
		ThisOffset = 0;			// For performance to avoid multiplication
		for(i = 0; i < Cols; i++) { Transition[ThisOffset + i] = 0; };
		Transition[ThisOffset] = Cols+2;
		Transition[ThisOffset+1] = -1;	// Flag end of run code

		for(i = 0; i < BLOBROWCOUNT + 2; i++)	// Initialize result arrays
		{
			LastRegion[i] = -1;
			ThisRegion[i] = -1;
		}

		ThisIndexCount = 1;
		ThisRegion[0] = 0;	// Border region

		// Initialize left border column
		for(i = Row0 + 1; i < Row0 + Rows + 2; i++) { ThisRegion[i] = -1; } // Flag as uninit

		//----  Loop again over all rows - This time to generate label image - Note Skip bottom border row ----
		for(ThisRow = Row0 + 1; ThisRow < Row0 + Rows + 1; ThisRow++)
		{
			// Toggle rows in Transition array
			LastOffset = Cols - LastOffset;
			ThisOffset = Cols - ThisOffset;

			for(i = 0; i < Cols; i++)	// Reset new row to 0
			{
				int T = Transition[ThisOffset+i];
				Transition[ThisOffset+i] = 0;
				if(T < 0) break;		// Remainder of row is already 0
			};
			Transition[ThisOffset] = Cols+2; 
			Transition[ThisOffset+1] = -1; 
	
			// Until border row, fill transition array for new row 
			if(ThisRow < Row0 + Rows +1)
			{
				ImageOffset += WidthStep;	// Performance booster to avoid multiplication
				iTran = 0;				// Index into Transition array
				Tran = 0;				// No transitions at row start

				LastCell = Border;
				for(i = Col0; i < Col0 + Cols + 2; i++)	// Scan new row of Bordered image
				{
					if(i == Col0 || i == Col0 + Cols + 1) ThisCell = Border;
					else ThisCell = Image[ImageOffset + i];
					if(ThisCell != LastCell)	// Different color
					{
						Transition[ThisOffset + iTran] = Tran;	// Save completed Tran
						iTran++;					// Prepare new index
						if(iTran > Cols-2) break;	// Bounds check
						LastCell = ThisCell;		// With this color
					}
					Tran++;	// Tran continues
				}
				Transition[ThisOffset + iTran] = Tran;	// Save completed run
				Transition[ThisOffset + iTran + 1] = -1;	// Flag end of run code
			}

			// Start region analysis based on run codes
			LastRow = ThisRow - 1;
			LastIndexCount = ThisIndexCount;	// total runs >= 0
			ThisIndex = LastIndex = 0;

			int EndLast = 0;
			int EndThis = 0;
			for(j = 0; j < Cols; j++)
			{
				int Index = ThisOffset + j;
				int Tran = Transition[Index];			// Run length
				if(Tran > 0) ThisIndexCount = j + 1;	 

				if(ThisRegion[j] == -1) { EndLast = 1; }
				if(Tran < 0) { EndThis = 1; }

				if(EndLast > 0 && EndThis > 0) { break; }

				LastRegion[j] = ThisRegion[j];
				ThisRegion[j] = -1;		// Flag indicates region is not initialized
			}

			int MaxIndexCount = LastIndexCount;
			if(ThisIndexCount > MaxIndexCount) MaxIndexCount = ThisIndexCount;

			// Main loop over runs within Last and This rows
			while (LastIndex < LastIndexCount && ThisIndex < ThisIndexCount)
			{
				if(LastIndex == 0) LastStart = 0;
				else LastStart = Transition[LastOffset + LastIndex - 1];
				LastEnd = Transition[LastOffset + LastIndex] - 1;
				LastCode = LastIndex + BorderCode;
				LastColor = LastCode - 2 * (LastCode / 2);
				LastRegionNum = LastRegion[LastIndex];

				if(ThisIndex == 0) ThisStart = 0;
				else ThisStart = Transition[ThisOffset + ThisIndex - 1];
				ThisEnd = Transition[ThisOffset + ThisIndex] - 1;
				ThisCode = ThisIndex + BorderCode;
				ThisColor = ThisCode - 2 * (ThisCode / 2);
				ThisRegionNum = ThisRegion[ThisIndex];

				int TestA = (LastEnd < ThisStart - 1);	// initially false
				int TestB = (ThisEnd < LastStart);		// initially false
				int TestC = (LastStart < ThisStart);	// initially false
				int TestD = (ThisEnd < LastEnd);
				int TestE = (ThisEnd == LastEnd);

				int TestMatch = (ThisColor == LastColor);		// initially true
				int TestKnown = (ThisRegion[ThisIndex] >= 0);	// initially false

				int Case = 0;
				if(TestA) Case = 1;
				else if(TestB) Case = 8;
				else if(TestC)
				{
					if(TestD) Case = 3;
					else if(!TestE) Case = 2;
					else Case = 4;
				}
				else
				{
					if(TestE) Case = 5;
					else if(TestD) Case = 7;
					else Case = 6;
				}

				// Determine necessary action and take it
				switch (Case)
				{ 
					case 1: //|xxx    |
						  	//|    yyy|

						LastIndex++;
						break;
					
					case 2: //|xxxxoo |
						  	//|    yyy|
					
						if(TestMatch) { ThisRegionNum = LastRegionNum; }
						else { ThisRegionNum = ++HighRegionNum2; }

						ThisRegion[ThisIndex] = ThisRegionNum;
						Fill(ThisRow, ThisStart, ThisEnd, ThisRegionNum, RelabeledRegion, LabeledImage);	// Implant labels
						LastIndex++;
						break;
					
					
					case 3: //|xxxxxxx| 
						  	//|  yyyy |
					
						if(TestMatch) { ThisRegionNum = LastRegionNum; }
						else { ThisRegionNum = ++HighRegionNum2; }
					
						ThisRegion[ThisIndex] = ThisRegionNum;
						Fill(ThisRow, ThisStart, ThisEnd, ThisRegionNum, RelabeledRegion, LabeledImage);	// Implant labels
						ThisIndex++;
						break;
					
					
					case 4:	//|xxxxxxx|
							//|  yyyyy|
					
						if(TestMatch) { ThisRegionNum = LastRegionNum; }
						else { ThisRegionNum = ++HighRegionNum2; }
					
						ThisRegion[ThisIndex] = ThisRegionNum;
						Fill(ThisRow, ThisStart, ThisEnd, ThisRegionNum, RelabeledRegion, LabeledImage);	// Implant labels
						LastIndex++;
						ThisIndex++;
						break;
					
					
					case 5:	//|ooxxxxx|
							//|yyyyyyy|
					
						if(TestMatch && !TestKnown) { ThisRegionNum = LastRegionNum; }
						else if(!TestMatch && !TestKnown) { ThisRegionNum = ++HighRegionNum2; }
						else if(TestMatch && TestKnown)
						{
							if(ThisRegionNum > LastRegionNum) { ThisRegionNum = LastRegionNum; }
							else if(ThisRegionNum < LastRegionNum) { LastRegionNum = ThisRegionNum; }
						}

						ThisRegion[ThisIndex] = ThisRegionNum;
						Fill(ThisRow, ThisStart, ThisEnd, ThisRegionNum, RelabeledRegion, LabeledImage);	// Implant labels
						LastRegion[LastIndex] = LastRegionNum;		// Already implanted
						LastIndex++;
						ThisIndex++;
						break;
					
					
					case 6:	//|ooxxx  |
							//|yyyyyyy|

						if(TestMatch && !TestKnown) { ThisRegionNum = LastRegionNum; }
						else if(TestMatch && TestKnown)
						{
							if(ThisRegionNum > LastRegionNum) { ThisRegionNum = LastRegionNum; }
							else if(ThisRegionNum < LastRegionNum) { LastRegionNum = ThisRegionNum; }
						}

						ThisRegion[ThisIndex] = ThisRegionNum;
						Fill(ThisRow, ThisStart, ThisEnd, ThisRegionNum, RelabeledRegion, LabeledImage);	// Implant labels
						LastRegion[LastIndex] = LastRegionNum;		// Already implanted
						LastIndex++;
						break;
					
					
					case 7:	//|ooxxxxx|
							//|yyyy   |
					
						if(TestMatch && !TestKnown) { ThisRegionNum = LastRegionNum; }
						else if(!TestMatch && !TestKnown) { ThisRegionNum = ++HighRegionNum2; }
						else if(TestMatch && TestKnown)
						{
							if(ThisRegionNum > LastRegionNum) { ThisRegionNum = LastRegionNum; }
							else if(ThisRegionNum < LastRegionNum) { LastRegionNum = ThisRegionNum; }
						}

						ThisRegion[ThisIndex] = ThisRegionNum;
						Fill(ThisRow, ThisStart, ThisEnd, ThisRegionNum, RelabeledRegion, LabeledImage);	// Implant labels
						LastRegion[LastIndex] = LastRegionNum;		// Already implanted
						ThisIndex++;
						break;
					
					case 8:	//|    xxx|
							//|yyyy   |
					
						ThisIndex++;
						break;
					
					default:
						break;
				}	// end switch case

			}	// end Main loop

		}	// end Loop over all rows

	}	// end if(LabelFlag)
	//-----------------------------------------------------------------------------------------------

	if(ErrorFlag != 0) return(ErrorFlag);

	return(HighRegionNum);
}

// Implant labels into LabelImage
void Fill(int Row, int Col0, int Col1, int iOld, int RelabeledRegion[BLOBTOTALCOUNT], IplImage* LabeledImage)
{
	static int Debug = 0;
	char* LabeledImageData = LabeledImage->imageData;
	int LabeledWidthStep = LabeledImage->widthStep; 
	int LabeledImageOffset = LabeledWidthStep * Row - LabeledWidthStep - 1;
	int Offset0 = LabeledImageOffset + Col0;
	int Offset1 = LabeledImageOffset + Col1;
	int iNew = RelabeledRegion[iOld];
	if(iNew > 255) iNew = 255;		// Max possible in 8 bit unsigned image
	int i;
	for(i=Offset0; i<=Offset1; i++) { LabeledImageData[i] = iNew; }	
}

// h = (XX + YY) / 2
// Major axis = h + sqrt ( h^2 - XX * YY + XY^2)
// Minor axis = h - sqrt ( h^2 - XX * YY2 + XY^2) 
// Eccentricity = (sqrt(abs(XX - YY)) + 4 * XY)/AREA

#endif