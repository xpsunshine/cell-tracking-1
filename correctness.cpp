#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <cmath>

using namespace std;

#ifndef dist(a, b)
#define dist(a, b) sqrt(pow((double)(a.x - b.x), 2) + pow((double)(a.y - b.y), 2))
#endif

#ifndef POINT_H
#define POINT_H
typedef struct
{
  int x;
  int y;
} point;
#endif

int indf(point p, vector<point> pl);

/* int main ()
{
  int pointcount = 1;
  
  ifstream filea ("input.csv");
  
  string coord;
  while (getline(filea, coord))
  {
	pointcount++;
  }
  
  ifstream file ("input.csv");  

  int id = 0;
  while (getline(file, coord, ','))
  {
	if (id == pointcount - 1) break;
	int x1;
	int y1;
	int x2;
	int y2;
  
	stringstream sa(coord);
	sa >> x1;

	getline(file, coord, ',');
	stringstream sb(coord);
	sb >> y1;

	getline(file, coord, ',');
	stringstream sc(coord);
	sc >> x2;

	getline(file, coord);
	stringstream sd(coord);
	sd >> y2;

	point p1 = {x1,y1};

	linp[id] = p1;
	lout[id] = p2;
	
	id++;
	//cout << p1.x << endl;
  }
} */

double computeMatches(vector<point> linp, vector<point> lout, vector<point> ainp, vector<point> aout) {
  // linp, lout hand-annotated first and last frame; ainp, aout algorithm first and last frame
  // take in the input and store it as two vectors of points
  int numcorrect = 0;
  assert(ainp.size() == aout.size() && linp.size() == lout.size());
  vector<int> counted;
  for (int i = 0; i < ainp.size(); i++)
  {
	int ind1 = indf (ainp[i], linp);
	int ind2 = indf (aout[i], lout);
	if (ind1 == ind2) numcorrect++;
	cout << "pre:" << i << "->" << ind1 << endl;
	cout << "post:" << i << "->" << ind2 << endl;
	//   {
	// 	bool found = false;
	// 	for (int j = 0; j < numcorrect; j++)
	// 	{
	// 		if (counted[j] == ind1)
	// 		{
	// 			found = true;
	// 			break;
	// 		}
	// 	}
	// 	if (!found)
	// 	{
	// 		numcorrect++;
	// 		counted.push_back(ind1);
	// 	}
	// }
  }

  return (numcorrect * 1.0 / max(linp.size(), ainp.size()));
}

int
indf(point p, vector<point> pl)
{
	int minp = 0;
	int mindist = dist(p, pl[0]);
	for (int i = 1; i < pl.size(); i++)
	{
		int ndist = dist(p, pl[i]);
		if (ndist < mindist)
		{
			mindist = ndist;
			minp = i;
		}
	}
	
	return minp;
}