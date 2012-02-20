/***************************************************************************
 *   Copyright (C) 2007 by Graziano Giuliani                               *
 *   graziano.giuliani at poste.it                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details. (see COPYING)            *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   LIC: GPL                                                              *
 *                                                                         *
 ***************************************************************************/
// $Id: $

#include <meteosalute.h>

#include <cmath>

using namespace std;
using namespace Meteo;

float *Meteosalute::tapparent(float *t, float *rh, float *wind)
{
  if (vector_size <= 0)
    return 0;
  float *tapparent = new float[vector_size];

  for (int i = 0; i < vector_size; i ++)
  {
    tapparent[i] = 999.9;
    if (rh[i] > 100.1|| rh[i] < 0.0)
      continue;
    else if (wind[i] > 130.0|| wind[i] < 0.0)
      continue;
    else if (t[i] > 100.0|| t[i] < -100.0)
      continue;
    else if (t[i] > 15.0&& t[i] < 20.0)
      tapparent[i] = rintf(t[i]);
    else if (t[i] <= 15.0&& wind[i] >= 1.3)
      tapparent[i] = windchill(t[i], wind[i]);
    else if (t[i] <= 15.0&& wind[i] < 1.3)
      tapparent[i] = rintf(t[i]);
    else
      tapparent[i] = heatindex(t[i], rh[i]) + hi_wind(t[i], wind[i]);
  }

  return tapparent;
}

#ifdef TEST_ME

#include <iostream>

int main(int argc, char *argv[])
{
  const int V_SIZE = 1;
  Meteosalute m(V_SIZE);
  float t[V_SIZE] =
  { 30.0};
  float rh[V_SIZE] =
  { 75.0};
  float wind[V_SIZE] =
  { 12.0};
  float *ta = m.tapparent(t, rh, wind);

  float *res = ta;
  for (int i = 0; i < V_SIZE; i ++)
  {
    std::cout << res[i] << std::endl;
  }
  return 0;
}

#endif
