#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "warpfield.h"
#include "gcamorph.h"
#include "matrix.h"

// constructor
Warpfield::Warpfield()
{
  __warpmap = NULL;  __warpmap_inv = NULL;
  __invert = 0;
  __mgzVersion = ((MGZ_WARPMAP & 0xff ) << 8) | MGH_VERSION;
  __dataformat = WarpfieldDTFMT::WARPFIELD_DTFMT_UNKNOWN;
  __srcRas2Vox = NULL;
}


// destructor
Warpfield::~Warpfield()
{
  if (__warpmap != NULL)
    MRIfree(&__warpmap);
}


int Warpfield::convert(const char *fname, const int dataformat, int doGCAMsampleMorph)
{
  int type = TransformFileNameType((char *)fname);
  if (type != MORPH_3D_TYPE)
  {
    printf("ERROR: %s is not in m3z format\n", fname);
    exit(1);
  }
    
  GCA_MORPH *gcam = GCAMread(fname);

  return convert(gcam, __dataformat);
}

// convert GCAM
int Warpfield::convert(GCA_MORPH *gcam, const int dataformat, int doGCAMsampleMorph)
{
  int (*nintfunc)( double );
  nintfunc = &nint;

  printf("[INFO] Warpfield::convert(): converting GCAM%s ...\n", (doGCAMsampleMorph) ? " (do GCAMsampleMorph)" : "");
  
  // create MRI using atlas vol_geom
  __warpmap = new MRI(gcam->atlas, MRI_FLOAT, 3, 0);  //__warpmap = new MRI({gcam->atlas.width, gcam->atlas.height, gcam->atlas.depth, 3}, MRI_FLOAT);
  __dataformat = dataformat;
  __srcRas2Vox = gcam->image.get_RAS2Vox();

  int out_of_gcam_count = 0;
  for (int c = 0; c < __warpmap->width; c++)
  {
    for (int r = 0; r < __warpmap->height; r++)
    {
      for (int s = 0; s < __warpmap->depth; s++)
      {
	float fcs = 0, frs = 0, fss = 0;
	if (doGCAMsampleMorph)
	{
          // (c, r, s) is in atlas (target) volume, (fcs, frs, fss) is in image (source) volume
	  // (c, r, s) => (fcs, frs, fss)	
	  int out_of_gcam = GCAMsampleMorph(gcam, (float)c, (float)r, (float)s, &fcs, &frs, &fss);
	  if (out_of_gcam)
	  {
	    out_of_gcam_count++;
	    continue;
	  }
	}
	else
	{
	  // crash at [c, r, s] = [0, 128, 0] running bert/mri/talairach.m3z
	  fcs = gcam->nodes[c][r][s].x;
	  frs = gcam->nodes[c][r][s].y;
	  fss = gcam->nodes[c][r][s].z;
	}
	
        if (__dataformat == WarpfieldDTFMT::WARPFIELD_DTFMT_ABS_CRS)
        {
	  // in source (unmorphed, image) voxel space
          MRIsetVoxVal(__warpmap, c, r, s, 0, fcs);
	  MRIsetVoxVal(__warpmap, c, r, s, 1, frs);
	  MRIsetVoxVal(__warpmap, c, r, s, 2, fss);
	}
	else if (__dataformat == WarpfieldDTFMT::WARPFIELD_DTFMT_DISP_CRS)
	{
	  // convert CRS0 in target (atlas) voxel space => source (image) voxel space
	  MATRIX *atlas_CRS0 = MatrixAlloc(4, 1, MATRIX_REAL);
	  atlas_CRS0->rptr[1][1] = c;
          atlas_CRS0->rptr[2][1] = r;
          atlas_CRS0->rptr[3][1] = s;
          atlas_CRS0->rptr[4][1] = 1;

	  MATRIX *atlas2image_vox = MatrixMultiply(gcam->image.get_RAS2Vox(), gcam->atlas.get_Vox2RAS(), NULL);
	  MATRIX *image_CRS0 = MatrixMultiply(atlas2image_vox, atlas_CRS0, NULL);  // CRS0 is now in source (image) voxel space

	  // set the displacement in source (image) voxel space
	  MRIsetVoxVal(__warpmap, c, r, s, 0, fcs - image_CRS0->rptr[1][1]);
	  MRIsetVoxVal(__warpmap, c, r, s, 1, frs - image_CRS0->rptr[2][1]);
	  MRIsetVoxVal(__warpmap, c, r, s, 2, fss - image_CRS0->rptr[3][1]);
	}
	else if (__dataformat == WarpfieldDTFMT::WARPFIELD_DTFMT_ABS_RAS ||
                 __dataformat == WarpfieldDTFMT::WARPFIELD_DTFMT_DISP_RAS)
	{
	  // convert (fcs, frs, fss) to image_RAS
	  MATRIX *image_CRS = MatrixAlloc(4, 1, MATRIX_REAL);
	  image_CRS->rptr[1][1] = nintfunc(fcs);
          image_CRS->rptr[2][1] = nintfunc(frs);
          image_CRS->rptr[3][1] = nintfunc(fss);
          image_CRS->rptr[4][1] = 1;

	  MATRIX *image_RAS = MatrixMultiply(gcam->image.get_Vox2RAS(), image_CRS, NULL);

	  if (__dataformat == WarpfieldDTFMT::WARPFIELD_DTFMT_ABS_RAS)
	  {
	    // in source (unmorphed, image) RAS space
	    MRIsetVoxVal(__warpmap, c, r, s, 0, image_RAS->rptr[1][1]);
	    MRIsetVoxVal(__warpmap, c, r, s, 1, image_RAS->rptr[2][1]);
	    MRIsetVoxVal(__warpmap, c, r, s, 2, image_RAS->rptr[3][1]);
	  }
	  else // __dataformat == WARPFIELD_DTFMT_DISP_RAS
	  {
	    // convert CRS0 in target (atlas) voxel space => source (image) voxel space
	    MATRIX *atlas_CRS0 = MatrixAlloc(4, 1, MATRIX_REAL);
	    atlas_CRS0->rptr[1][1] = c;
            atlas_CRS0->rptr[2][1] = r;
            atlas_CRS0->rptr[3][1] = s;
            atlas_CRS0->rptr[4][1] = 1;

	    MATRIX *atlas2image_vox = MatrixMultiply(gcam->image.get_RAS2Vox(), gcam->atlas.get_Vox2RAS(), NULL);
	    MATRIX *image_CRS0 = MatrixMultiply(atlas2image_vox, atlas_CRS0, NULL);  // CRS0 is now in source (image) voxel space
            MATRIX *image_RAS0 = MatrixMultiply(gcam->image.get_Vox2RAS(), image_CRS0, NULL); // RAS0 is now in source (image) RAS space
	    
	    // set the displacement in source (image) RAS space
	    MRIsetVoxVal(__warpmap, c, r, s, 0, image_RAS->rptr[1][1] - image_RAS0->rptr[1][1]);
	    MRIsetVoxVal(__warpmap, c, r, s, 1, image_RAS->rptr[2][1] - image_RAS0->rptr[2][1]);
	    MRIsetVoxVal(__warpmap, c, r, s, 2, image_RAS->rptr[3][1] - image_RAS0->rptr[3][1]);
	  }
	}  // WARPFIELD_DTFMT_ABS_RAS || WARPFIELD_DTFMT_DISP_RAS
      }  // s
    }  // r
  }  // c

  printf("[INFO] Warpfield::convert(): total out of range voxel count: %d\n", out_of_gcam_count);
  
  return 0;
}


// invert M3z into 3-fram MRI warp map
int Warpfield::invert(const char *fname, const int dataformat)
{
  int type = TransformFileNameType((char *)fname);
  if (type != MORPH_3D_TYPE)
  {
    printf("ERROR: %s is not in m3z format\n", fname);
    exit(1);
  }
    
  GCA_MORPH *gcam = GCAMread(fname);

  return invert(gcam, __dataformat);  
}

// invert GCAM
int Warpfield::invert(GCA_MORPH *gcam, const int dataformat)
{
  int (*nintfunc)( double );
  nintfunc = &nint;

  printf("[INFO] Warpfield::invert(): inverting GCAM ...\n");
  __invert = 1;
  
  // create GCAM inverse
  gcam->spacing = 1;
  
  // purpose of tempMri is just to pass image dimensions to GCAMinvert()
  MRI *tempMri = new MRI(gcam->image, MRI_FLOAT, 3, 0);
  GCAMinvert(gcam, tempMri);
  MRIfree(&tempMri);

  // create MRI using image vol_geom
  __warpmap = new MRI(gcam->image, MRI_FLOAT, 3, 0);
  __dataformat = dataformat;
  __srcRas2Vox = gcam->atlas.get_RAS2Vox();
  
  for (int c = 0; c < __warpmap->width; c++)
  {
    for (int r = 0; r < __warpmap->height; r++)
    {
      for (int s = 0; s < __warpmap->depth; s++)
      {
	float fct = 0, frt = 0, fst = 0;
        // (c, r, s) is in image (source) volume, (fct, frt, fst) is in atlas (target) volume
	// (c, r, s) => (fct, frt, fst)	
	int out_of_gcam = GCAMsampleInverseMorph(gcam, (float)c, (float)r, (float)s, &fct, &frt, &fst);
	if (out_of_gcam)
	  continue;
	
        if (__dataformat == WarpfieldDTFMT::WARPFIELD_DTFMT_ABS_CRS)
        {
          MRIsetVoxVal(__warpmap, c, r, s, 0, fct);
	  MRIsetVoxVal(__warpmap, c, r, s, 1, frt);
	  MRIsetVoxVal(__warpmap, c, r, s, 2, fst);
	}
	else if (__dataformat == WarpfieldDTFMT::WARPFIELD_DTFMT_DISP_CRS)
	{
	  // delta = src - dst
	  MRIsetVoxVal(__warpmap, c, r, s, 0, c - fct);
	  MRIsetVoxVal(__warpmap, c, r, s, 1, r - frt);
	  MRIsetVoxVal(__warpmap, c, r, s, 2, s - fst);
	}
	else if (__dataformat == WarpfieldDTFMT::WARPFIELD_DTFMT_ABS_RAS ||
                 __dataformat == WarpfieldDTFMT::WARPFIELD_DTFMT_DISP_RAS)
	{
	  // convert (fct, frt, fst) to dst_RAS
	  MATRIX *dst_CRS = MatrixAlloc(4, 1, MATRIX_REAL);
	  dst_CRS->rptr[1][1] = nintfunc(fct);
          dst_CRS->rptr[2][1] = nintfunc(frt);
          dst_CRS->rptr[3][1] = nintfunc(fst);
          dst_CRS->rptr[4][1] = 1;

	  MATRIX *dst_RAS = MatrixAlloc(4, 1, MATRIX_REAL);	  
	  dst_RAS = MatrixMultiply(gcam->atlas.get_Vox2RAS(), dst_CRS, dst_RAS);

	  if (__dataformat == WarpfieldDTFMT::WARPFIELD_DTFMT_ABS_RAS)
	  {
	    MRIsetVoxVal(__warpmap, c, r, s, 0, dst_RAS->rptr[1][1]);
	    MRIsetVoxVal(__warpmap, c, r, s, 1, dst_RAS->rptr[2][1]);
	    MRIsetVoxVal(__warpmap, c, r, s, 2, dst_RAS->rptr[3][1]);
	  }
	  else // __dataformat == WARPFIELD_DTFMT_DISP_RAS
	  {
	    // convert (c, r, s) to src_RAS
	    MATRIX *src_CRS = MatrixAlloc(4, 1, MATRIX_REAL);
	    src_CRS->rptr[1][1] = c;
            src_CRS->rptr[2][1] = r;
            src_CRS->rptr[3][1] = s;
            src_CRS->rptr[4][1] = 1;

	    MATRIX *src_RAS = MatrixAlloc(4, 1, MATRIX_REAL);	    
	    src_RAS = MatrixMultiply(gcam->image.get_Vox2RAS(), src_CRS, src_RAS);

	    // delta = src_RAS - dst_RAS
	    MRIsetVoxVal(__warpmap, c, r, s, 0, src_RAS->rptr[1][1] - dst_RAS->rptr[1][1]);
	    MRIsetVoxVal(__warpmap, c, r, s, 1, src_RAS->rptr[2][1] - dst_RAS->rptr[2][1]);
	    MRIsetVoxVal(__warpmap, c, r, s, 2, src_RAS->rptr[3][1] - dst_RAS->rptr[3][1]);
	  }
	}  // WARPFIELD_DTFMT_ABS_RAS || WARPFIELD_DTFMT_DISP_RAS
      }  // s
    }  // r
  }  // c
  
  return 0;
}


// read 3-frame MRI warp map into __warpmap
int Warpfield::read(const char *fname)
{
  int ret = 0;
  
  __warpmap = MRIread(fname);
  if (__warpmap == NULL)
  {
    printf("ERROR: Warpfield::read(%s)\n", fname);
    ret = 1;
  }
  
  return ret;
}


// write out the 3-frame MRI warping map
int Warpfield::write(const char *fname)
{
  if (__invert)
    __mgzVersion = ((MGZ_WARPMAP_INV & 0xff ) << 8) | MGH_VERSION;

  printf("[DEBUG] Warpfield::write(): __mgzVersion = %d\n", __mgzVersion);
  __warpmap->setWarpfieldMeta(__warpmap, __mgzVersion, __dataformat, __srcRas2Vox);

  int ret = MRIwrite(__warpmap, fname);
  if (ret)
    printf("ERROR: Warpfield::write(%s)\n", fname);
  
  return ret;
}


// apply warpmap to MRI
int Warpfield::applyWarp(const MRI *inmri, MRI *outmri)
{
  return 0;
}


// apply warpmap to surface
int Warpfield::applyWarp(const MRIS *insurf, MRIS *outsurf)
{
  return 0;
}
