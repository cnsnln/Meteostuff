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

float *Meteosalute::steadman_outdoor_sun(float *t,
                                         float *rh,
                                         float *wind,
                                         float *rshort,
                                         float *sunelev)
{
  if (vector_size <= 0) return 0;
  float *steadman_outdoor_sun = new float[vector_size];
  for (int i = 0; i < vector_size; i ++)
  {
    steadman_outdoor_sun[i] = 999.9;
    if (rh[i] > 100.1 || rh[i] < 0.0)
      continue;
    else if (t[i] > 100.0 || t[i] < -100.0)
      continue; 
    else
    {
      float e = (rh[i]/100.0)*(6.105*exp((t[i]*17.27)/(237.7+t[i])));
      float q_glob = 0.56*(0.386-0.0032*sunelev[i])*rshort[i] + 
             0.224*(0.1*rshort[i])+ 0.028*rshort[i] + 
             150.0*(0.38-0.16*(pow(e/10.0,0.5)));
      if (q_glob > 0.0)
        steadman_outdoor_sun[i] = t[i]+3.48*(e/10.0)-0.7*wind[i] + 
                                  0.7*q_glob/(wind[i]+10.0)-4.25;
    }
  }
  return steadman_outdoor_sun;
}

#ifdef TEST_ME

#include <iostream>

int main(int argc, char *argv[])
{
  const int V_SIZE = 1;
  Meteosalute m(V_SIZE);
  float t[V_SIZE] = { 30.0};
  float rh[V_SIZE] = { 75.0};
  float wind[V_SIZE] = { 12.0};
  float rshort[V_SIZE] = { 12.0};
  float sunelev[V_SIZE] = { 12.0};
  float *we = m.steadman_outdoor_sun(t, rh, wind);

  float *res = we;
  for (int i = 0; i < V_SIZE; i ++)
  {
    std::cout << res[i] << std::endl;
  }
  return 0;
}

#endif
