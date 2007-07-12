#include "hc.h"

/* //
// compute Legendre function (l,m) evaluated on nlat points 
// in latitude and their derviatives with respect to theta, if 
// ivec is set to 1
// */

void rick_compute_allplm(int lmax,int ivec,float *plm,
			 float *dplm, struct rick_module *rick) 
{
  int i,os;
  
  if (lmax != rick->nlat-1) {
    fprintf(stderr,"rick_compute_allplm: error: lmax mismatch: nlat/lmax %i %i \n",rick->nlat,lmax);
    /*     print *,nlat,lmax */
    exit(-1);
  }
  os=0;				/* changed this to 0 TWB */
  for (i=0;i < rick->nlat;i++) { /*changed from 1->nlat to 0->nlat-1 - need change in plmbar1 also */
    rick_plmbar1((plm+os),(dplm+os),ivec,lmax,rick->gauss_z[i],rick); /*note change in gauss_z[i] */
    os += rick->lmsize;
  }
}

/* // compute Legendre function (l,m) evaluated on npoints points in
// theta array their derviatives with respect to theta, if ivec is set
// to 1 */

void rick_compute_allplm_irreg(int lmax,int ivec,SH_RICK_PREC *plm,
			       SH_RICK_PREC *dplm, struct rick_module *rick, 
			       float *theta, int ntheta) 
{
  int i,os;
  
  os=0;				/* changed this to 0 TWB */
  for (i=0;i < ntheta;i++) { /*changed from 1->nlat to 0->nlat-1 - need change in plmbar1 also */
    rick_plmbar1((plm+os),(dplm+os),ivec,lmax,cos(theta[i]),rick); /*note change in gauss_z[i] */
    os += rick->lmsize;
  }
}


/* //
// detemine the colatidude and longitude of PIxel index
// where index goes from 0 ... nlon * nlat-1
// */
void rick_pix2ang(int index, int lmax, SH_RICK_PREC *theta, 
		  SH_RICK_PREC *phi, struct rick_module *rick) {
  int  i,j;
  if(!rick->initialized){
    fprintf(stderr,"rick_pix2ang: error: rick module not initialized\n");
    exit(-1);
  }
  
  j = index;
  i=0;
  while(j > rick->nlonm1) { /* */
    j -= rick->nlon;
    i++;
  }
  *theta = rick->gauss_theta[i];
  *phi = rick->dphi * (SH_RICK_PREC)(j);
}


void rick_shc2d(SH_RICK_PREC *cslm,SH_RICK_PREC *dslm,
		int lmax,int ivec,
		SH_RICK_PREC *rdatax,SH_RICK_PREC *rdatay,
		struct rick_module *rick)
{
  //
  // Transforms spherical harmonic coefficients of a scalar (ivec = 0)
  // or a vector (ivec=1) field
  // to data points on a grid. Reverse transform of subroutine
  // shd2c.f so long as the same degree is used and the points
  // in latitude are Gaussian integration points. Maximum degree
  // must be 2**n-1 in order for the FFT to work; this determines
  // the grid spacing in longitude. nlat = lmax+1, nlon = 2*nlat
  //
  // INPUT:
  //
  //		lmax	spherical harmonic degree used in expansion
  //		cslm	(lmsize2) spherical harmonic coefficients
  //             dslm    (lmsize2)
  //                     if ivec, will hold poloidal and toroidal coeff,else
  //                     dslm will not be referenced
  //
  //             ivec   0: scalar 1: vector field
  // OUTPUT:
  //		rdatax	(nlon * nlat) values on grid points
  //             rdatay  (nlon * nlat). x and y will be theta and phi for 
  //                     ivec = 1, else rdatay will not be referenced
  //
  // if Ivec is set, assume velocities to be expanded instead
  //
  // input: lmax,ivec
  // local
  SH_RICK_PREC *plm,*dplm;
  if(!rick->initialized){
    fprintf(stderr,"rick_shc2d: error: initialize first\n");
    exit(-1);
  }
  // compute the Plm first
  if(lmax != rick->nlat-1){
    fprintf(stderr,"rick_shc2d: error: lmax mismatch: nlat: %i lmax: %i\n",
	    rick->nlat,lmax);
    exit(-1);
  }
  /* allocate memory */
  hc_svecalloc(&plm,rick->nlat*rick->lmsize,"rick_shc2d: mem 1");
  if(ivec)
    hc_svecalloc(&dplm,rick->nlat*rick->lmsize,"rick_shc2d: mem 2");
  //
  // compute the Plm first
  rick_compute_allplm(lmax,ivec,plm,dplm,rick);

  //
  // call the precomputed subroutine
  rick_shc2d_pre(cslm,dslm,lmax,plm,dplm,ivec,rdatax,rdatay,rick);

  /* free legendre functions if not needed */
  free(plm);
  if(ivec)
    free(dplm);
}
/* 

converts on irregular basis with locations cos(theta)[], phi[]  long

 */
void rick_shc2d_irreg(SH_RICK_PREC *cslm,SH_RICK_PREC *dslm,
		      int lmax,int ivec,
		      SH_RICK_PREC *rdatax,SH_RICK_PREC *rdatay,
		      struct rick_module *rick,float *theta, int ntheta, 
		      float *phi,int nphi, hc_boolean save_sincos_fac)
{
  SH_RICK_PREC *plm,*dplm;
  if(!rick->initialized){
    fprintf(stderr,"rick_shc2d: error: initialize first\n");
    exit(-1);
  }
  /* allocate memory */
  hc_svecalloc(&plm,ntheta*rick->lmsize,"rick_shc2d_irreg: mem 1");
  if(ivec)
    hc_svecalloc(&dplm,ntheta*rick->lmsize,"rick_shc2d_irreg: mem 2");
  //
  // compute the Plm first
  rick_compute_allplm_irreg(lmax,ivec,plm,dplm,rick,theta,ntheta);
  //
  // call the precomputed subroutine
  rick_shc2d_pre_irreg(cslm,dslm,lmax,plm,dplm,ivec,rdatax,rdatay,rick,theta,ntheta,
		       phi,nphi,save_sincos_fac);

  /* free legendre functions if not needed */
  free(plm);
  if(ivec)
    free(dplm);
}



/* //
// the actual routine to go from spectral to spatial: added structure rick
// */
void rick_shc2d_pre(SH_RICK_PREC *cslm,SH_RICK_PREC *dslm,
		    int lmax,SH_RICK_PREC *plm, SH_RICK_PREC *dplm,
		    int ivec,float *rdatax,float *rdatay, 
		    struct rick_module *rick)
{
  /* //
  // Legendre functions are precomputed
  // */
  SH_RICK_PREC  *valuex, *valuey;
  SH_RICK_PREC  dpdt,dpdp;
  static int negunity = -1;	/* an actual constant */
  int  i,j,m,m2,j2,ios1,l,lmaxp1,lmaxp1t2,oplm,nlon2,lm1;
  if(!rick->initialized){
    fprintf(stderr,"rick_shc2d_pre: error: initialize modules first\n");
    exit(-1);
  }
  // check bounds 
  lmaxp1 = lmax + 1;                // this is nlat
  lmaxp1t2 = 2 * lmaxp1;               // this is nlon
  if((rick->nlat != lmaxp1)||(rick->nlon != lmaxp1t2)){
    fprintf(stderr,"rick_shc2d_pre: dimension mismatch: lmax: %i nlon: %i nlat:%i\n",
	    lmax,rick->nlon,rick->nlat);
    exit(-1);
  }
  if(ivec){
    if(!rick->vector_sh_fac_init){
      fprintf(stderr,"rick_shc2d_pre: error: vector harmonics factors not initialized\n");
      exit(-1);
    }
  }
  nlon2 = rick->nlon + 2;
  /* allocate value arrays */
  rick_vecalloc(&valuex,nlon2,"rick_shc2d_pre 1");
  if(ivec)
    rick_vecalloc(&valuey,nlon2,"rick_shc2d_pre 2");
  
  for (i=0;i < rick->nlat; i++) {	
    /*  
	loop through latitudes 
    */
    oplm = i * rick->lmsize;        /*  offset for Plm array. (changed indices TWB) */
    ios1 = i * rick->nlon;        /* offset for data array */
    if(!ivec){          
      /* 
	 scalar
      */
      for(j=0;j < nlon2;j++)
	valuex[j] = 0.0;               /* init with 0.0es */
      l = 0; m = -1;
      for (j=j2=0; j < rick->lmsize; j++,j2+=2) { /* loop through l,m */
	m++;
	if (m > l) {
	  m=0;
	  l++;
	}
	m2 = 2*m;
	//fprintf(stderr,"%11i %11i %11g %11g\n",l,m,cslm[j2],cslm[j2+1]);
	/*  add up contributions from all l,m  */
	valuex[m2]   +=  /* cos term */
	  plm[oplm+j] * (SH_RICK_PREC)cslm[j2]; /* A coeff */
	valuex[m2+1] +=  /* sin term */
	  plm[oplm+j] * (SH_RICK_PREC)cslm[j2+1];   /* B coeff */
      }

      /* compute inverse FFT  */
#ifdef NO_RICK_FORTRAN      
      rick_cs2ab(valuex,rick->nlon);
      rick_realft_nr((valuex-1),rick->nlat,negunity);	
#else
      rick_f90_cs2ab(valuex,&rick->nlon);	
      rick_f90_realft(valuex,&rick->nlat,&negunity);	
#endif

      for (j=0; j < rick->nlon; j++) { /* can't vectorize */
	rdatax[ios1 + j] = valuex[j]/(SH_RICK_PREC)(rick->nlat);
      }
      /* end scalar part */
    } else {
      /* 
	 vector harmonics
      */
      for(j=0;j < nlon2;j++){
	valuex[j] = valuey[j] = 0.0;
      }
      l = 1; 
      m = -1;       
      /* start at l = 1 */
      for (j=1,j2=2; j < rick->lmsize; j++,j2+=2) { 
	/* loop through l,m */
	m++;
	if (m > l) {
	  m=0;
	  l++;
	}
	m2  = 2*m;
	/* derivative factors */
	lm1 = l - 1;
	dpdt = dplm[oplm+j] * (SH_RICK_PREC)rick->ell_factor[lm1]; /* d_theta(P_lm) factor */
	dpdp  = ((SH_RICK_PREC)m) * plm[oplm+j]/ (SH_RICK_PREC)rick->sin_theta[i];
	dpdp *= (SH_RICK_PREC)rick->ell_factor[lm1]; /* d_phi (P_lm) factor */
	/* add up contributions from all l,m 

	u_theta

	*/
	valuex[m2] +=   /* cos term */
	  cslm[j2]   * dpdt + dslm[j2+1] * dpdp;
	valuex[m2+1] +=   /* sin term */
	  cslm[j2+1] * dpdt - dslm[j2]   * dpdp;
	/* 
	   u_phi
	*/
	valuey[m2] +=  // cos term
	  cslm[j2+1] * dpdp  - dslm[j2]   * dpdt;
	valuey[m2+1] +=   // sin term
	  - cslm[j2] * dpdp  - dslm[j2+1] * dpdt;
      }	/* end l,m loop */
        /* do inverse FFTs */
#ifdef NO_RICK_FORTRAN
      rick_cs2ab(valuex,rick->nlon);
      rick_cs2ab(valuey,rick->nlon);
      rick_realft_nr((valuex-1),rick->nlat,negunity);
      rick_realft_nr((valuey-1),rick->nlat,negunity);
#else
      rick_f90_cs2ab(valuex,&rick->nlon);
      rick_f90_cs2ab(valuey,&rick->nlon);
      rick_f90_realft(valuex,&rick->nlat,&negunity);
      rick_f90_realft(valuey,&rick->nlat,&negunity);
#endif
      /* assign to output array */
      for (j=0; j < rick->nlon; j++) {   
	rdatax[ios1 + j] = valuex[j]/(SH_RICK_PREC)(rick->nlat);
	rdatay[ios1 + j] = valuey[j]/(SH_RICK_PREC)(rick->nlat);
      }
    }
  } /* end latitude loop */

  /* free temporary arrays */
  free(valuex);
  if(ivec)
    free(valuey);
  
}

/* //

irregular version

// */
void rick_shc2d_pre_irreg(SH_RICK_PREC *cslm,SH_RICK_PREC *dslm,
			  int lmax,SH_RICK_PREC *plm, SH_RICK_PREC *dplm,
			  int ivec,float *rdatax,float *rdatay, 
			  struct rick_module *rick, float *theta, int ntheta,
			  float *phi,int nphi,my_boolean save_sincos_fac)
{
  /* //
  // Legendre functions are precomputed
  // */
  double  dpdt,dpdp,mphi,sin_theta;
  float *loc_plma=NULL,*loc_plmb=NULL;
  int  i,j,k,k2,m,ios1,ios2,ios3,l,lmaxp1,lm1,idata;
  if(!rick->initialized){
    fprintf(stderr,"rick_shc2d_pre_irreg: error: initialize modules first\n");
    exit(-1);
  }

  lmaxp1 = lmax + 1;                // this is nlat

  if(ivec){
    if(!rick->vector_sh_fac_init){
      fprintf(stderr,"rick_shc2d_pre_irreg: error: vector harmonics factors not initialized\n");
      exit(-1);
    }
  }
  if((lmax+1)*(lmax+2)/2 > rick->lmsize){
    fprintf(stderr,"rick_shc2d_pre_irreg: error: lmax %i out of bounds\n",lmax);
      exit(-1);
  }

  /* 
     compute sin/cos factors
  */
  hc_svecrealloc(&rick->sfac,nphi*lmaxp1,"rick_shc2d_pre_irreg");
  hc_svecrealloc(&rick->cfac,nphi*lmaxp1,"rick_shc2d_pre_irreg");
  for(ios1=i=0;i < nphi;i++){
    for(m=0;m <= lmax;m++,ios1++){
      mphi = (double)m*(double)phi[i];
      rick->sfac[ios1] = (float)sin(mphi);
      rick->cfac[ios1] = (float)cos(mphi);
    }
  }
  if(ivec == 0){
    /* 

    scalar

    */
    hc_svecrealloc(&loc_plma,ntheta*rick->lmsize,"rick_shc2d_pre_irreg 3");
    hc_svecrealloc(&loc_plmb,ntheta*rick->lmsize,"rick_shc2d_pre_irreg 4");
    for(i=ios1=0;i < ntheta;i++){
      for(k=k2=0;k < rick->lmsize;k++,k2+=2,ios1++){
	loc_plma[ios1] =  cslm[k2  ] * plm[ios1];
	loc_plmb[ios1] =  cslm[k2+1] * plm[ios1];
      }
    }
    for (idata=i=ios2=0;i < ntheta; i++,ios2 += rick->lmsize) { /* theta
								   loop */
      for(ios3=j=0;j < nphi;j++,idata++,ios3 += lmaxp1){ /* phi loop */
	
	/* m = 0 , l = 0*/
	l=0;m=0;
	rdatax[idata] = loc_plma[ios2];
	for (k=1; k < rick->lmsize; k++) { 
	  m++;
	  if (m > l) {
	    m=0;l++;
	  }
	  rdatax[idata] += loc_plma[ios2+k] * rick->cfac[ios3+m];
	  rdatax[idata] += loc_plmb[ios2+k] * rick->sfac[ios3+m];
	}
      }
    }

    free(loc_plma);free(loc_plmb);
    /* end scalar part */
  } else {
    /* 
       vector harmonics
    */
    for (idata=i=ios2=0;i < ntheta; i++,ios2 += rick->lmsize) { /* theta
								   loop */
      sin_theta = sin(theta[i]);
      for(ios3=j=0;j < nphi;j++,idata++,ios3 += lmaxp1){ /* phi loop */
	
	rdatax[idata] = rdatay[idata] = 0.0;

	l = 0;m = -1;       
	/* start at l = 1 */
	for (k=1,k2=2; k < rick->lmsize; k++,k2+=2) { 
	/* loop through l,m */
	  m++;
	  if (m > l) {
	    m=0;l++;
	  }
	  lm1 = l - 1;
	  dpdt = dplm[ios2+k] * rick->ell_factor[lm1]; /* d_theta(P_lm) factor */
	  dpdp  = ((SH_RICK_PREC)m) * plm[ios2+k]/ sin_theta;
	  dpdp *= (SH_RICK_PREC)rick->ell_factor[lm1]; /* d_phi (P_lm) factor */
	  
	  /* 
	     
	  add up contributions from all l,m 
	  
	  u_theta
	  
	*/
	  rdatax[i] +=   /* cos term */
	    (cslm[k2]   * dpdt + dslm[k2+1] * dpdp) * rick->cfac[ios3+m];
	  rdatax[i] +=   /* sin term */
	    (cslm[k2+1] * dpdt - dslm[k2]   * dpdp) * rick->sfac[ios3+m];
	  /* 
	     u_phi
	  */
	  rdatay[i] +=  // cos term
	    (cslm[k2+1] * dpdp  - dslm[k2]   * dpdt) * rick->cfac[ios3+m];
	  rdatay[i] +=   // sin term
	    (- cslm[k2] * dpdp  - dslm[k2+1] * dpdt) * rick->sfac[ios3+m];
	}
      }
    }
  }

  if(!save_sincos_fac){
    hc_svecrealloc(&rick->sfac,1,"");
    hc_svecrealloc(&rick->cfac,1,"");
  }
}

void rick_shd2c(SH_RICK_PREC *rdatax,SH_RICK_PREC *rdatay,
		int lmax,int ivec,SH_RICK_PREC *cslm,
		SH_RICK_PREC *dslm,struct rick_module *rick)
{
  //
  //	Calculates spherical harmonic coefficients cslm(l,m) of
  //	a scalar (ivec = 0) or vector (ivec=1) function on a sphere. 
  //
  //     if ivec == 1, then cslm will be poloidal, and dslm toroidal
  //                   rdata will be nlon*nlat
  //
  //     Coefficients stored with
  //	cosine term and sine term alternating, starting at l=0
  //	and m=0 with m ranging from 0 to l before l is incremented.
  //
  //     Harmonics normalized with mean square = 1.0. Expansion is:
  //     SUM( Plm(cos(theta))*(cp(l,m)*cos(m*phi)+sp(l,m)*sin(m*phi))
  //
  //     Uses FFT in longitude and gaussian integration of spectral
  //     coefficients (for fixed m) times associated Legendre
  //     functions (of same order m) to get expansion coefficients.
  //
  //     Uses Num Rec routine for Gaussian points and weights, which should
  //     be initialized first by a call to rick_init. 
  //
  //     Uses recursive routine to generate associated Legendre functions.
  //     
  //     INPUT:
  //
  //     lmax    maximum possible degree of expansion (2**n-1). There
  //             are nlon = 2*lmax+2 data points in longitude for each latitude
  //             which has nlat = lmax+1 points
  //
  //     rdatax,rdatay   data((nlon * nlat)) arrays for theta and phi
  //                     components
  //
  //     ic_pd: should be either 0.0_cp or 1.0_cp
  //            if set to 1.0_cp, will expand velocities instead
  //            in this case, data should hold the theta and phi components
  //            of a vector field and cslm will hold the poloidal and toroidal
  //            on output components, respectively
  //
  //     OUTPUT:
  //
  //     cslm,dslm    coefficients, (2*lmsize)
  //
  //     dslm and rdatay will only be referenced when ivec = 1
  //
  //
  // local
  SH_RICK_PREC *plm,*dplm;
  /* allocate memory */
  hc_svecalloc(&plm,rick->nlat*rick->lmsize,"rick_shd2c: mem 1");
  hc_svecalloc(&dplm,rick->nlat*rick->lmsize,"rick_shd2c: mem 2");
  // check
  if(!rick->initialized){
    fprintf(stderr,"rick_shd2c: error: initialize first\n");
    exit(-1);
  }
  if(lmax != rick->nlat-1){
    fprintf(stderr,"rick_shd2c: error: lmax mismatch: nlat: %i lmax: %i\n",
	    rick->nlat,lmax);
    exit(-1);
  }
  
  // compute the Plm first
  rick_compute_allplm(lmax,ivec,plm,dplm,rick);
  //
  // call the precomputed version
  rick_shd2c_pre(rdatax,rdatay,lmax,plm,dplm,ivec,cslm,dslm,rick);
  /* free legendre functions if not needed */
  free(plm);
  free(dplm);
}
//
// the actual routine to go from spatial to spectral, 
// for comments, see above
//
void rick_shd2c_pre(SH_RICK_PREC *rdatax,SH_RICK_PREC *rdatay,
		    int lmax,SH_RICK_PREC *plm,SH_RICK_PREC *dplm,int ivec,
		    SH_RICK_PREC *cslm,SH_RICK_PREC *dslm, 
		    struct rick_module *rick)
{
  // local
  SH_RICK_PREC *valuex, *valuey;
  SH_RICK_PREC dfact,dpdt,dpdp;
  static int unity = 1;		/* constant */
  //
  int  lmaxp1,lmaxp1t2,i,j,l,m,ios1,m2,j2,oplm,nlon2,lm1;
  // check
  if(!rick->initialized){
    fprintf(stderr,"rick_shd2c_pre: error: initialize first\n");
    exit(-1);
  }
  // check some more and compute bounds
  lmaxp1 = lmax + 1;
  lmaxp1t2 = lmaxp1 * 2;
  nlon2 = rick->nlon + 2;
  if((lmaxp1 != rick->nlat)||(rick->nlon != lmaxp1t2)||
     ((lmax+1)*(lmax+2)/2 != rick->lmsize)){
    fprintf(stderr,"rick_shd2c_pre: dimension error, lmax %i\n",lmax);
    fprintf(stderr,"rick_shd2c_pre: nlon %i nlat %i\n",rick->nlon,rick->nlat);
    fprintf(stderr,"rick_shd2c_pre: lmsize %i\n",rick->lmsize);
    exit(-1);
  }
  /* allocate */
  rick_vecalloc(&valuex,nlon2,"rick_shd2c_pre 1");
  if(ivec)
    rick_vecalloc(&valuey,nlon2,"rick_shd2c_pre 2");
  
  //
  // initialize the coefficients
  //
  for(i=0;i < rick->lmsize2;i++)
    cslm[i] = 0.0;
  if(ivec){
    if(! rick->vector_sh_fac_init){
      fprintf(stderr,"rick_shd2c_pre: error: vector harmonics factors not initialized\n");
      exit(-1);
    }
    for(i=0;i < rick->lmsize2;i++)
      dslm[i] = 0.0;
  }
  for(i=0;i < rick->nlat;i++){
    //
    // loop through latitudes
    //
    ios1 = i * rick->nlon;          // offset for data array
    oplm = i * rick->lmsize;        // offset for Plm array
    //
    if(!ivec){
      //
      // scalar expansion
      //
      for(j=0;j < rick->nlon;j++){      
	valuex[j] = rdatax[ios1 + j];
      }
      //
      // compute the FFT 
      //

#ifdef NO_RICK_FORTRAN
      rick_realft_nr((valuex-1),rick->nlat,unity);
      rick_ab2cs(valuex,rick->nlon);
#else
      rick_f90_realft(valuex,&rick->nlat,&unity);
      rick_f90_ab2cs(valuex,&rick->nlon);
#endif
      // sum up for integration
      l = 0;m = -1;
      for(j=j2=0;j < rick->lmsize;j++,j2+=2){
	m++;
	if( m >  l ) {
	  l++;m=0;
	}
	// we incorporate the Gauss integration weight and Plm factors here
	if (m == 0) {
	  dfact = ((SH_RICK_PREC)rick->gauss_w[i] * plm[oplm+j])/2.0;
	}else{
	  dfact = ((SH_RICK_PREC)rick->gauss_w[i] * plm[oplm+j])/4.0;
	}
	m2 = m * 2;
	cslm[j2]   += valuex[m2]   * dfact; // A coefficient
	cslm[j2+1] += valuex[m2+1] * dfact; // B coefficient
      }	/* end lmsize loop */
      /* end scalar */
    }else{
      //
      // vector field expansion
      //
      for(j=0;j < rick->nlon;j++){    
	valuex[j] = rdatax[ios1 + j]; // theta
	valuey[j] = rdatay[ios1 + j]; // phi
      }
      // perform the FFTs on both components
#ifdef NO_RICK_FORTRAN
      rick_realft_nr((valuex-1),rick->nlat,unity);
      rick_realft_nr((valuey-1),rick->nlat,unity);
      rick_ab2cs(valuex,rick->nlon);
      rick_ab2cs(valuey,rick->nlon);
#else
      rick_f90_realft(valuex,&rick->nlat,&unity);
      rick_f90_realft(valuey,&rick->nlat,&unity);
      rick_f90_ab2cs(valuex,&rick->nlon);
      rick_f90_ab2cs(valuey,&rick->nlon);
#endif
      //
      l=1;m=-1;                // there's no l=0 term
      //
      for(j = 1,j2 = 2;j < rick->lmsize;j++,j2+=2){
	m++;
	if( m >l ) {
	  l++;m=0;
	}
	lm1 = l - 1;
	if (m == 0){ // ell_factor is 1/sqrt(l(l+1))
	  dfact = rick->gauss_w[i] * rick->ell_factor[lm1]/2.0;
	}else{
	  dfact = rick->gauss_w[i] * rick->ell_factor[lm1]/4.0;
	}
	//
	// some more factors
	//
	// d_theta(P_lm) factor
	dpdt = dplm[oplm+j];
	// d_phi (P_lm) factor
	dpdp = ((SH_RICK_PREC)m) * plm[oplm+j]/(SH_RICK_PREC)rick->sin_theta[i];
	//
	m2 = m * 2;
	//           print *,m,l,dpdt*dfact,dpdp*dfact
	/* poloidal */
	cslm[j2]   += // poloidal A
	  (dpdt * valuex[m2]   - dpdp * valuey[m2+1])*dfact;
	cslm[j2+1] += //   poloidal B
	  (dpdt * valuex[m2+1] + dpdp * valuey[m2]  )*dfact;
	/* toroidal */
	dslm[j2]   += // toroidal A 
	  (-dpdp * valuex[m2+1] - dpdt * valuey[m2]  )*dfact;
	dslm[j2+1] += // toroidal B 
	  ( dpdp * valuex[m2]   - dpdt * valuey[m2+1])*dfact;
      }
    }                      // end vector field
  }                        // end latitude loop

  free(valuex);
  if(ivec)
    free(valuey);
}


//
// initialize all necessary arrays for Rick type expansion
//
// if ivec == 1, will initialize for velocities/polarizations
//
/* if regular is set, will not initialize the Gauss quadrature
   points */
void rick_init(int lmax,int ivec,int *npoints,int *nplm,
	       int *tnplm, struct rick_module *rick,
	       hc_boolean regular)
{

  // input: lmax,ivec			
  // output: npoints,nplm,tnplm
  // local 
  SH_RICK_PREC xtemp;
  int i,l;


  if(!rick->was_called){
    if(lmax == 0){
      fprintf(stderr,"rick_init: error: lmax is zero: %i\n",lmax);
      exit(-1);
    }
    //
    // test if lmax is 2**n-1
    //
    xtemp = log((SH_RICK_PREC)(lmax+1))/log(2.0);
    if(fabs((int)(xtemp+.5)-xtemp) > 1e-7){
      fprintf(stderr,"rick_init: error: lmax has to be 2**n-1\n");
      fprintf(stderr,"rick_init: maybe use lmax = %i\n",
	      (int)pow(2,(int)(xtemp)+1)-1);
      fprintf(stderr,"rick_init: instead of %i\n",lmax);
      exit(-1);
    }
    //
    // number of longitudinal and latitudinal points
    //
    rick->nlat = lmax + 1;
    rick->nlon = 2 * rick->nlat;
    //
    // number of points in one layer
    //
    *npoints = rick->nlat * rick->nlon; 
    rick->old_npoints = *npoints;
    //
    //
    // for coordinate computations
    //
    rick->dphi = TWOPI / (SH_RICK_PREC)(rick->nlon);
    rick->nlonm1 = rick->nlon - 1;
    //
    // size of tighly packed arrays with l,m indices
    rick->lmsize  = (lmax+1)*(lmax+2)/2;
    rick->lmsize2 = rick->lmsize * 2;          //for A and B
    //
    //
    // size of the Plm array 
    *nplm = rick->lmsize * rick->nlat;
    *tnplm = *nplm * (1+ivec);           // for all layers
    rick->old_tnplm = *tnplm;
    rick->old_nplm = *nplm;
    //
    // initialize the Gauss points, at which the latitudes are 
    // evaluated
    //
    if(!regular){
      rick_vecalloc(&rick->gauss_z,rick->nlat,"rick_init 1");
      rick_vecalloc(&rick->gauss_w,rick->nlat,"rick_init 2");
      rick_vecalloc(&rick->gauss_theta,rick->nlat,"rick_init 3");
      /* 
	 gauss weighting 
      */
      rick_gauleg(-1.0,1.0,rick->gauss_z,rick->gauss_w,rick->nlat);
      //
      // theta values of the Gauss quadrature points
      //
      for(i=0;i < rick->nlat;i++)
	rick->gauss_theta[i] = acos(rick->gauss_z[i]);
    }
    //
    // those will be used by plmbar to store some of the factors
    //
    hc_svecalloc(&rick->plm_f1,rick->lmsize,"rick_init 4");
    hc_svecalloc(&rick->plm_f2,rick->lmsize,"rick_init 5");
    hc_svecalloc(&rick->plm_fac1,rick->lmsize,"rick_init 6");
    hc_svecalloc(&rick->plm_fac2,rick->lmsize,"rick_init 7");
    hc_svecalloc(&rick->plm_srt,rick->nlon,"rick_init 8");
    if(ivec){
      //
      // additional arrays for vector spherical harmonics
      // (perform the computations in SH_RICK_PREC precision)
      //
      rick_vecalloc(&rick->ell_factor,rick->nlat,"rick init 9");
      if(!regular)
	rick_vecalloc(&rick->sin_theta,rick->nlat,"rick init 9");
      
      // 1/(l(l+1)) factors
      for(i=0,l=1;i < rick->nlat;i++,l++){
	// no l=0 term, obviously
	// go from l=1 to l=lmax+1
	rick->ell_factor[i] = 1.0/sqrt((SH_RICK_PREC)(l*(l+1)));
	if(!regular){
	  rick->sin_theta[i] = sqrt((1.0 - rick->gauss_z[i])*
				    (1.0+rick->gauss_z[i]));
	}
	rick->vector_sh_fac_init = TRUE;
      }
    }else{
      rick->vector_sh_fac_init = FALSE;
    }
    //
    // logic flags
    //
    rick->computed_legendre = FALSE;
    rick->initialized = TRUE;
    rick->was_called = TRUE;
    /* 
       save initial call lmax and ivec settings
    */
    rick->old_lmax = lmax;
    rick->old_ivec = ivec;

    /* for irregular expansions */
    rick->cfac = rick->sfac = NULL;

    /* end initial branch */
  }else{
    if(lmax != rick->old_lmax){
      fprintf(stderr,"rick_init: error: was init with lmax %i, now: %i. (ivec: %i, now: %i)\n",
	      rick->old_lmax,lmax,rick->old_ivec,ivec);
      exit(-1);
    }
    if(ivec > rick->old_ivec){
      fprintf(stderr,"rick_init: error: original ivec %i, now %i\n",rick->old_ivec,ivec);
      exit(-1);
    }
    *npoints = rick->old_npoints;
    *nplm = rick->old_nplm;
    *tnplm = rick->old_tnplm;
  }
} /* end rick init */


//
// free all arrays that were allocate for the module   
//
void rick_free_module(struct rick_module *rick, int ivec)
{
  // input: ivec
  
  free(rick->gauss_z);
  free(rick->gauss_w);
  free(rick->gauss_theta);
  if(rick->computed_legendre){
    // those are from the Legendre function action

    free(rick->plm_f1);free(rick->plm_f2);
    free(rick->plm_fac1);free(rick->plm_fac2);
    free(rick->plm_srt);
  }
  if(ivec){
    free(rick->ell_factor);free(rick->sin_theta);
  }
  
}
void rick_plmbar1(SH_RICK_PREC  *p,SH_RICK_PREC *dp,int ivec,int lmax,
		  SH_RICK_PREC z, struct rick_module *rick)
{
  //
  //     Evaluates normalized associated Legendre function P(l,m), plm,
  //     as function of  z=cos(colatitude); also derivative dP/d(colatitude),
  //     dp, if ivec is set to 1.0
  //
  //     Uses recurrence relation starting with P(l,l) and { 
  //     increasing l keePIng m fixed.
  //
  //     p(k) contains p(l,m)
  //     with k=(l+1)*l/2+m+1; i.e. m increments through range 0 
  //     to l before
  //     incrementing l. 
  //
  //     Normalization is:
  //
  //     Integral(P(l,m)*P(l,m)*d(cos(theta)))=2.*(2-delta(0,m)),
  //     where delta(i,j) is the Kronecker delta.
  //
  //     This normalization is incorporated into the
  //     recurrence relation which eliminates overflow. 
  //     Routine is stable in single and SH_RICK_PREC 
  //     precision to
  //     l,m = 511 at least; timing proportional to lmax**2
  //     R.J.O'Connell 7 Sept. 1989; added dp(z) 10 Jan. 1990.
  //
  //     Added precalculation and storage of square roots 
  //     srt(k) 31 Dec 1992
  //
  //     this C version by Thorsten Becker, twb@usc.edu
  //
  //     - ALL RICK-> ARRAYS AND P[] AS WELL AS DP[] HAVE BEEN 
  //       CONVERTED TO BE ADDRESSED C STYLE 0...N-1
  //    
  //
  //
  //  int, intent(in)  lmax, ivec
  //  SH_RICK_PREC,intent(in)  z
  //  SH_RICK_PREC,intent(inout), dimension (lmsize)  p, dp
  //
  // local
  double plm,pm1,pm2,pmm,sintsq,fnum,fden;
  //
  int i,l,m,k,kstart,l2,mstop,lmaxm1;
  if(!rick->initialized){
    fprintf(stderr,"rick_plmbar1: error: module not initialized, call rick_init first\n");
    exit(-1);
  }
  if ((lmax < 0) || (fabs(z) > 1.0)) {
    fprintf(stderr,"rick_plmbar1: error: bad arguments\n");
    exit(-1);
  }
  if(!rick->computed_legendre) {
    /* 
       need to initialize the legendre factors 
    */
    if(rick->nlon != (lmax+1)*2){
      fprintf(stderr,"rick_plmbar1: factor mismatch, lmax: %i vs nlon: %i (needs to be (lmax+1)*2)\n",
	      lmax,rick->nlon);
      exit(-1);
    }
    //
    // first call, set up some factors. the arrays were allocated in rick_init
    //
    for(k=0,i=1;k < rick->nlon;k++,i++){
      rick->plm_srt[k] = sqrt((SH_RICK_PREC)(i));
    }
    // initialize plm factors
    for(i=0;i < rick->lmsize;i++){
      rick->plm_f1[i] = rick->plm_fac1[i] = 0.0;
      rick->plm_f2[i] = rick->plm_fac2[i] = 0.0;
    }
    //     --case for m > 0
    kstart = 0;
    for(m=1;m <= lmax;m++){
      //     --case for P(m,m) 
      kstart += m+1;
      if (m != lmax) {
	//     --case for P(m+1,m)
	k = kstart+m+1;		
	//     --case for P(l,m) with l > m+1
	if (m < (lmax-1)) {
	  for(l = m+2;l <= lmax;l++){
	    l2 = l * 2;	
	    k = k+l;
	    rick->plm_f1[k] = rick->plm_srt[l2] * rick->plm_srt[l2-2]/
	      (rick->plm_srt[l+m-1] * rick->plm_srt[l-m-1]);
	    rick->plm_f2[k]=(rick->plm_srt[l2] * rick->plm_srt[l-m-2]*rick->plm_srt[l+m-2])/
	      (rick->plm_srt[l2-4] * rick->plm_srt[l+m-1] * rick->plm_srt[l-m-1]);
	  }
	}
      }
    }
    if(ivec){
      //
      // for derivative of Plm with resp. to theta
      // (if we forget to call Plm with ivec=1, but use
      // ivec=1 later, we will notice as all should be zero)
      //
      k=2;			
      for(l=2;l<=lmax;l++){
	k++;
	mstop = l - 1;
	for(m=1;m <= mstop;m++){
	  k++;
	  rick->plm_fac1[k] = rick->plm_srt[l-m-1] * rick->plm_srt[l+m];
	  rick->plm_fac2[k] = rick->plm_srt[l+m-1] * rick->plm_srt[l-m];
	  if(m == 1){
	    rick->plm_fac2[k] = rick->plm_fac2[k] * rick->plm_srt[1];
	  }
	}
	k++;
      }
    } /* end ivec==1 */
    rick->old_lmax = lmax;
    rick->old_ivec = ivec;
    rick->computed_legendre = TRUE;
    /* 
       end first call 
    */
  }else{
    /* 
       subsequent call 
    */
    // test if lmax has changed
    if(lmax != rick->old_lmax){
      fprintf(stderr,"rick_plmbar1: error: factors were computed for lmax %in",rick->old_lmax);
      fprintf(stderr,"rick_plmbar1: error: now, lmax is %i\n",lmax);
      exit(-1);
    }
    if(ivec > rick->old_ivec){
      fprintf(stderr,"rick_plmbar1: error: init with %i, now ivec %i\n",rick->old_ivec,ivec);
      exit(-1);
    }
  }
  /* 

  what follows will be executed for each z

  */
  //     --start calculation of Plm etc.
  //     --case for P(l,0) 
  
  pm2  = 1.0;
  p[0] = 1.0;                   // (0,0)
  if(ivec)
    dp[0] = 0.0;// else, don't refer to this array
  if (lmax == 0){
    fprintf(stderr,"lmax is zero. what the hell?//\n");
    exit(-1);
  }
  pm1  = z;
  p[1] = rick->plm_srt[2] * pm1;             // (1,0)
  k=1;
  for(l = 2;l<=lmax;l++){               // now all (l,0)
    k  += l;
    l2 = l * 2;
    plm = ((double)(l2-1) * z * pm1 - (double)(l-1) * pm2)/((double)l);
    p[k] = rick->plm_srt[l2] * plm;
    pm2 = pm1;
    pm1 = plm;
  }
  //       --case for m > 0
  pmm = 1.0;
  sintsq = (1.0 - z) * (1.0 + z);
  fnum = -1.0;
  fden =  0.0;
  kstart = 0;
  lmaxm1 = lmax - 1;
  for(m = 1;m<=lmax;m++){
    //     --case for P(m,m) 
    kstart += m+1;
    fnum += 2.0;
    fden += 2.0;
    pmm = pmm * sintsq * fnum / fden;
    pm2 = sqrt((SH_RICK_PREC)(4*m+2)*pmm);
    p[kstart] = pm2;
    if (m != lmax) {
      //     --case for P(m+1,m)
      pm1 = z * rick->plm_srt[2*m+2]*pm2;
      k = kstart + m + 1;
      p[k] = pm1;
      //     --case for P(l,m) with l > m+1
      if (m < lmaxm1) {
	for(l = m+2;l <= lmax;l++){
	  k += l;
	  plm = z * rick->plm_f1[k] * pm1 - 
	    rick->plm_f2[k] * pm2;
	  p[k] = plm;
	  pm2 = pm1;
	  pm1 = plm;
	}
      }
    }
  }
  if(ivec){
    // 
    // derivatives
    //     ---derivatives of P(z) wrt theta, where z=cos(theta)
    //     
    dp[1] = -p[2];
    dp[2] =  p[1];
    k = 2;
    for(l=2;l <= lmax;l++){
      k++;
      //     treat m=0 and m=l separately
      dp[k] =  -rick->plm_srt[l-1] * rick->plm_srt[l] / rick->plm_srt[1] * p[k+1];
      dp[k+l] = rick->plm_srt[l-1] / rick->plm_srt[1] * p[k+l-1];
      mstop = l-1;
      for(m=1;m <= mstop;m++){
	k++;
	dp[k] = rick->plm_fac2[k] * p[k-1] - 
	  rick->plm_fac1[k] * p[k+1];
	dp[k] *= 0.5;
      }
      k++;
    }
  }
}



//
// Returns arrays X and W with N points and weights for
// Gaussian quadrature over interval X1,X2.
// we call this routine with n = nlat = lmax+1
//
// this is from Numerical Recipes, but we changed the indexing to 
// 0..n-1
//       
//     
void rick_gauleg(SH_RICK_PREC x1, SH_RICK_PREC x2, 
		 SH_RICK_PREC *x, SH_RICK_PREC *w,int n)
{
  //
  // local variables
  //
  int i,j,m;
  double p1,p2,p3,pp,xl,xm,z,z1;
  //
  pp = 0.0;			/* for copiler */
  m=(n+1)/2;
  xm=0.5*(x2+x1);
  xl=0.5*(x2-x1);
  for(i=0;i < m;i++){
    z = cos(RICK_PI * (i+.75)/(n+.5));
    z1 = -2.0;
    while(fabs(z-z1) > 5.e-15){
      p1 = 1.0;
      p2 = 0.0;
      for(j = 1;j <= n;j++){
	p3 = p2;
	p2 = p1;
	p1 = ((2.0*(double)j - 1.0) * z * p2 -(j- 1.0)*p3)/(double)j;
      }
      pp = (double)n*(z*p1-p2)/(z*z-1.0);
      z1 = z;
      z = z1-p1/pp;
    }
    x[i] = xm - xl*z;
    x[n-i-1] = xm+xl*z;
    w[i] = 2.0*xl/((1.0-z*z)*pp*pp);
    w[n-i-1] = w[i];
  }
}
