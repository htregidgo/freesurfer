/**
 * @file  mrivoxel.c
 * @brief utilities for MRI data structure
 *
 */
/*
 * Original Author: Bruce Fischl
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2007/12/10 20:30:07 $
 *    $Revision: 1.6.2.1 $
 *
 * Copyright (C) 2002-2007,
 * The General Hospital Corporation (Boston, MA). 
 * All rights reserved.
 *
 * Distribution, usage and copying of this software is covered under the
 * terms found in the License Agreement file named 'COPYING' found in the
 * FreeSurfer source code root directory, and duplicated here:
 * https://surfer.nmr.mgh.harvard.edu/fswiki/FreeSurferOpenSourceLicense
 *
 * General inquiries: freesurfer@nmr.mgh.harvard.edu
 * Bug reports: analysis-bugs@nmr.mgh.harvard.edu
 *
 */

/*-----------------------------------------------------
                    INCLUDE FILES
-------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <memory.h>

#include "error.h"
#include "proto.h"
#include "mri.h"
#include "macros.h"
#include "diag.h"
#include "volume_io.h"

/*-----------------------------------------------------
                    MACROS AND CONSTANTS
-------------------------------------------------------*/

/*-----------------------------------------------------
                    STATIC PROTOTYPES
-------------------------------------------------------*/
/*-----------------------------------------------------
                    GLOBAL FUNCTIONS
-------------------------------------------------------*/
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
float
MRIvoxelDx(MRI *mri, int x, int y, int z)
{
  float   dx ;
  int     width, height, xm1, xp1, total, left, right, ym1, yp1 ;

  width = mri->width ;
  height = mri->height ;

  total = 0 ;

  x = mri->xi[x] ;
  xm1 = mri->xi[x-1] ;
  xp1 = mri->xi[x+1] ;
  y = mri->yi[y] ;
  ym1 = mri->yi[y-1] ;
  yp1 = mri->yi[y+1] ;
  z = mri->zi[z] ;

  left = 2 * MRIvox(mri, xm1, y, z) ;
  left += MRIvox(mri, xm1, ym1, z) ;
  left += MRIvox(mri, xm1, yp1, z) ;

  right = 2 * MRIvox(mri, xp1, y, z) ;
  right += MRIvox(mri, xp1, ym1, z) ;
  right += MRIvox(mri, xp1, yp1, z) ;

  dx = ((float)right - (float)left) / 8.0f ;
  return(dx) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
float
MRIvoxelDy(MRI *mri, int x, int y, int z)
{
  float   dy ;
  int     width, height, xm1, xp1, total, top, bottom, ym1, yp1 ;

  width = mri->width ;
  height = mri->height ;

  total = 0 ;

  x = mri->xi[x] ;
  xm1 = mri->xi[x-1] ;
  xp1 = mri->xi[x+1] ;
  y = mri->yi[y] ;
  ym1 = mri->yi[y-1] ;
  yp1 = mri->yi[y+1] ;
  z = mri->zi[z] ;

  top = 2 * MRIvox(mri, xm1, ym1, z) ;
  top += MRIvox(mri, x, ym1, z) ;
  top += MRIvox(mri, xp1, ym1, z) ;

  bottom = 2 * MRIvox(mri, xm1, yp1, z) ;
  bottom += MRIvox(mri, x, yp1, z) ;
  bottom += MRIvox(mri, xp1, yp1, z) ;

  dy = ((float)bottom - (float)top) / 8.0f ;
  return(dy) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
float
MRIvoxelDz(MRI *mri, int x, int y, int z)
{
  float   dz ;
  int     width, depth, xm1, xp1, total, top, bottom, zm1, zp1 ;

  width = mri->width ;
  depth = mri->depth ;

  total = 0 ;

  x = mri->xi[x] ;
  xm1 = mri->xi[x-1] ;
  xp1 = mri->xi[x+1] ;
  z = mri->zi[z] ;
  zm1 = mri->zi[z-1] ;
  zp1 = mri->zi[z+1] ;
  y = mri->yi[y] ;

  top = 2 * MRIvox(mri, xm1, y, zm1) ;
  top += MRIvox(mri, x, y, zm1) ;
  top += MRIvox(mri, xp1, y, zm1) ;

  bottom = 2 * MRIvox(mri, xm1, y, zp1) ;
  bottom += MRIvox(mri, x, y, zp1) ;
  bottom += MRIvox(mri, xp1, y, zp1) ;

  dz = ((float)bottom - (float)top) / 8.0f ;
  return(dz) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
float
MRIvoxelGradient(MRI *mri, int x,int y,int z,float *pdx,float *pdy,float *pdz)
{
  float    mag, dx, dy, dz ;

  *pdx = dx = MRIvoxelDx(mri, x, y, z) ;
  *pdy = dy = MRIvoxelDy(mri, x, y, z) ;
  *pdz = dz = MRIvoxelDz(mri, x, y, z) ;
  mag = sqrt(dx * dx + dy*dy + dz*dz) ;

  return(mag) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
float
MRIvoxelMean(MRI *mri, int x0, int y0, int z0, int wsize)
{
  float   mean, total  ;
  int     whalf, width, height, depth, x, y, z, npix, xmin, xmax,
  ymin, ymax, zmin, zmax ;
  BUFTYPE *psrc ;

  whalf = wsize/2 ;
  width = mri->width ;
  height = mri->height ;
  depth = mri->depth ;

  total = 0 ;
  zmin = MAX(0, z0-whalf) ;
  zmax = MIN(depth-1, z0+whalf) ;
  ymin = MAX(0, y0-whalf) ;
  ymax = MIN(height-1, y0+whalf) ;
  xmin = MAX(0, x0-whalf) ;
  xmax = MIN(width-1, x0+whalf) ;
  npix = (zmax - zmin + 1) * (ymax - ymin + 1) * (xmax - xmin + 1) ;

  for (z = zmin ; z <= zmax ; z++)
  {
    for (y = ymin ; y <= ymax ; y++)
    {
      psrc = &MRIvox(mri, xmin, y, z) ;
      for (x = xmin ; x <= xmax ; x++)
        total += MRIgetVoxVal(mri, x, y, z, 0) ;
    }
  }
  if (npix > 0)
    mean = (float)total / (float)npix ;
  else
    mean = 0.0f ;
  return(mean) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
float
MRIvoxelMin(MRI *mri, int x0, int y0, int z0, int wsize)
{
  float   min_val, val ;
  int     whalf, width, height, depth, x, y, z, xmin, xmax,
          ymin, ymax, zmin, zmax ;

  whalf = wsize/2 ;
  width = mri->width ;
  height = mri->height ;
  depth = mri->depth ;

  zmin = MAX(0, z0-whalf) ;
  zmax = MIN(depth-1, z0+whalf) ;
  ymin = MAX(0, y0-whalf) ;
  ymax = MIN(height-1, y0+whalf) ;
  xmin = MAX(0, x0-whalf) ;
  xmax = MIN(width-1, x0+whalf) ;

  min_val = MRIgetVoxVal(mri, x0, y0, z0, 0) ;
  for (z = zmin ; z <= zmax ; z++)
  {
    for (y = ymin ; y <= ymax ; y++)
    {
      for (x = xmin ; x <= xmax ; x++)
      {
        val = MRIgetVoxVal(mri, x, y, z, 0) ;
        if (val < min_val)
          min_val = val ;
      }
    }
  }
  return(min_val) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
float
MRIvoxelStd(MRI *mri, int x0, int y0, int z0, float mean, int wsize)
{
  float   std, total, var ;
  int     whalf, width, height, depth, x, y, z, npix,
  xmin, xmax, ymin, ymax, zmin, zmax ;
  BUFTYPE *psrc ;

  whalf = wsize/2 ;
  width = mri->width ;
  height = mri->height ;
  depth = mri->depth ;

  zmin = MAX(0, z0-whalf) ;
  zmax = MIN(depth-1, z0+whalf) ;
  ymin = MAX(0, y0-whalf) ;
  ymax = MIN(height-1, y0+whalf) ;
  xmin = MAX(0, x0-whalf) ;
  xmax = MIN(width-1, x0+whalf) ;
  npix = (zmax - zmin + 1) * (ymax - ymin + 1) * (xmax - xmin + 1) ;

  total = 0.0 ;
  for (z = zmin ; z <= zmax ; z++)
  {
    for (y = ymin ; y <= ymax ; y++)
    {
      psrc = &MRIvox(mri, xmin, y, z) ;
      for (x = xmin ; x <= xmax ; x++)
      {
        var = mean - (float)(*psrc++) ;
        total += var*var ;
      }
    }
  }
  if (npix > 0)
    std = sqrt(total) / (float)npix ;
  else
    std = 0.0f ;

  return(std) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
#define MAX_WINDOW  7
#define MAX_LEN     (MAX_WINDOW*MAX_WINDOW*MAX_WINDOW)
float
MRIvoxelDirection(MRI *mri, int x0, int y0, int z0, int wsize)
{
  int     whalf, width, height, depth, x, y, z, npix, total, xmin, xmax,
  ymin, ymax, zmin, zmax ;
  float   dx_win[MAX_LEN], dy_win[MAX_LEN], dz_win[MAX_LEN],dx,dy,dz,
  odx, ody, odz, *pdx, *pdy, *pdz, dir ;

  if (wsize > MAX_WINDOW)
    ErrorReturn(0.0f,
                (ERROR_BADPARM, "MRIvoxelDirection: window size %d too big",
                 wsize)) ;
  whalf = wsize/2 ;
  width = mri->width ;
  height = mri->height ;
  depth = mri->depth ;

  total = 0 ;
  zmax = MIN(depth-1, z0+whalf) ;
  ymax = MIN(height-1, y0+whalf) ;
  xmax = MIN(width-1, x0+whalf) ;

  zmin = MAX(0, z0-whalf) ;
  ymin = MAX(0, y0-whalf) ;
  xmin = MAX(0, x0-whalf) ;
  npix = (zmax - zmin + 1) * (ymax - ymin + 1) * (xmax - xmin + 1) ;

  /* should do something smarter than this about border conditions */
#if 0
  memset(dx_win, 0, MAX_LEN*sizeof(float)) ;
  memset(dy_win, 0, MAX_LEN*sizeof(float)) ;
  memset(dz_win, 0, MAX_LEN*sizeof(float)) ;
#endif
  pdx = dx_win ;
  pdy = dy_win ;
  pdz = dz_win ;
  for (z = z0-whalf ; z <= zmax ; z++)
  {
    for (y = y0-whalf ; y <= ymax ; y++)
    {
      for (x = x0-whalf ; x <= xmax ; x++)
      {
        *pdx++ = MRIvoxelDx(mri, x, y, z) ;
        *pdy++ = MRIvoxelDy(mri, x, y, z) ;
        *pdz++ = MRIvoxelDz(mri, x, y, z) ;
      }
    }
  }
  odx = MRIvoxelDx(mri, x0, y0, z0) ;
  ody = MRIvoxelDy(mri, x0, y0, z0) ;
  odz = MRIvoxelDz(mri, x0, y0, z0) ;

  pdx = dx_win ;
  pdy = dy_win ;
  pdz = dz_win ;
  dir = 0.0f ;
  for (z = -whalf ; z <= whalf ; z++)
  {
    for (y = -whalf ; y <= whalf ; y++)
    {
      for (x = -whalf ; x <= whalf ; x++)
      {
        dx = *pdx++ ;
        dy = *pdy++ ;
        dz = *pdz++ ;
        dir += (x*odx + y*ody + z*odz) * (dx*odx + dy*ody + dz*odz) ;
      }
    }
  }
#define FSCALE 1000.0f
  return(dir*(1.0f / (FSCALE*27.0f))) ;
}

/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
float
MRIvoxelGradientDir2ndDerivative(MRI *mri, int x0, int y0, int z0, int wsize)
{
  int     whalf, width, height, depth ;
  float   odx, ody, odz, len,
  d2I_dg2, xf, yf, zf, d ;
  Real    val ;

  whalf = wsize/2 ;
  width = mri->width ;
  height = mri->height ;
  depth = mri->depth ;


  odx = MRIvoxelDx(mri, x0, y0, z0) ;
  ody = MRIvoxelDy(mri, x0, y0, z0) ;
  odz = MRIvoxelDz(mri, x0, y0, z0) ;
  len = sqrt(odx*odx + ody*ody + odz*odz) ;
  if (FZERO(len))
    return(0.0f) ;
  odx /= len ;
  ody /= len ;
  odz /= len ;


  /* second derivative of intensity in gradient direction */
  MRIsampleVolume(mri, (Real)x0, (Real)y0, (Real)z0, &val) ;
  d2I_dg2 = (wsize-1)*val ;
  for (d = 1 ; d <= whalf ; d++)
  {
    xf = x0 + d*odx ;
    yf = y0 + d*ody ;
    zf = z0 + d*odz ;
    MRIsampleVolume(mri, xf, yf, zf, &val) ;
    d2I_dg2 -= val ;

    xf = x0 - d*odx ;
    yf = y0 - d*ody ;
    zf = z0 - d*odz ;
    MRIsampleVolume(mri, xf, yf, zf, &val) ;
    d2I_dg2 -= val ;
  }

  d2I_dg2 /= (float)(wsize) ;
  return(d2I_dg2) ;
}

float
MRIvoxelZscore(MRI *mri, int x, int y, int z, int wsize)
{
  float   mean, std, zscore, src ;

  src = (float)MRIvox(mri, x, y, z) ;
  mean = MRIvoxelMean(mri, x, y, z, wsize) ;
  std = MRIvoxelStd(mri, x, y, z, mean, wsize) ;
  if (!FZERO(std))
    zscore = (src - mean) / std ;
  else
    zscore = 0.0f ;

  return(zscore) ;
}

