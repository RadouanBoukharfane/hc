/* 


output routines of Hager & Connell code

$Id: hc_output.c,v 1.11 2006/01/22 01:11:34 becker Exp becker $


*/
#include "hc.h"
/* 


print the spherical harmonics version of a solution set


*/
void hc_print_spectral_solution(struct hcs *hc,struct sh_lms *sol,FILE *out,int sol_mode, 
				hc_boolean binary, hc_boolean verbose)
{
  int i,os;
  static int ntype = 3;			/* three sets of solutions, r/pol/tor */
  HC_PREC fac[3];
  if(!hc->spectral_solution_computed)
    HC_ERROR("hc_print_spectral_solution","spectral solution not computed");
  /* 
     number of solution sets of ntype solutions 
  */
  for(i=os=0;i < hc->nradp2;i++,os += ntype){
    /* 
       
    scale to cm/yr, or MPa  for stress solutions
    
    */
    hc_compute_solution_scaling_factors(hc,sol_mode,hc->r[i],hc->dvisc[i],fac);
    /* 
       write parameters, convert radius to depth in [km]  
    */
    sh_print_parameters_to_file((sol+os),ntype,i,hc->nradp2,
				HC_Z_DEPTH(hc->r[i]),
				out,FALSE,binary,verbose);
    /* 
       
       write the set of coefficients in D&T convention
       
    */
    sh_print_coefficients_to_file((sol+os),ntype,out,fac,
				  binary,verbose);
    if(verbose >= 2){
      switch(sol_mode){
      case HC_VEL:
	fprintf(stderr,"hc_print_spectral_solution: z: %8.3f vel: |r|: %11.3e |pol|: %11.3e |tor|: %11.3e (scale: %g cm/yr)\n",
		HC_Z_DEPTH(hc->r[i]),sqrt(sh_total_power((sol+os))),
		sqrt(sh_total_power((sol+os+1))),
		sqrt(sh_total_power((sol+os+2))),
		fac[0]/11.1194926644559);
	break;
      case HC_RTRACTIONS:
	fprintf(stderr,"hc_print_spectral_solution: z: %8.3f rtrac: |r|: %11.3e |pol|: %11.3e |tor|: %11.3e (scale: %g MPa)\n",
		HC_Z_DEPTH(hc->r[i]),sqrt(sh_total_power((sol+os))),
		sqrt(sh_total_power((sol+os+1))),
		sqrt(sh_total_power((sol+os+2))),
		fac[0]/(0.553073278428428/hc->r[i]));
	break;
      case HC_HTRACTIONS:
	fprintf(stderr,"hc_print_spectral_solution: z: %8.3f htrac: |r|: %11.3e |pol|: %11.3e |tor|: %11.3e (scale: %g MPa)\n",
		HC_Z_DEPTH(hc->r[i]),sqrt(sh_total_power((sol+os))),
		sqrt(sh_total_power((sol+os+1))),
		sqrt(sh_total_power((sol+os+2))),
		fac[0]/(0.553073278428428/hc->r[i]));
	break;
      default:
	fprintf(stderr,"hc_print_spectral_solution: sol mode %i undefined\n",sol_mode);
	exit(-1);
	break;
      }
    }
  }
  if(verbose)
    fprintf(stderr,"hc_print_spectral_solution: wrote solution at %i levels\n",
	    hc->nradp2);
}

/* 

print a single scalar field to file

*/

void hc_print_sh_scalar_field(struct sh_lms *sh, FILE *out, hc_boolean short_format,
			      hc_boolean binary, hc_boolean verbose)
{
  HC_CPREC fac[1] = {1.0};
  sh_print_parameters_to_file(sh,1,0,1,0.0,out,short_format,binary,verbose); /* parameters in long format */
  sh_print_coefficients_to_file(sh,1,out,fac,binary,verbose); /* coefficients */
}


/* 

print the spatial solution in 

lon lat vr vt vp 

format to nrad+2 files named filename.i.pre, where i is 0...nrad+1,
and pre is dat or bin, depending on ASCII or binary output.

will also write the corresponding depth layers to dfilename

*/
void hc_print_spatial_solution(struct hcs *hc, 
			       struct sh_lms *sol,
			       float *sol_x, char *name, 
			       char *dfilename, 
			       int sol_mode, hc_boolean binary, 
			       hc_boolean verbose)
{
  int i,j,k,os[3],los,np,np2,np3;
  FILE *file_dummy=NULL,*out,*dout;
  float flt_dummy=0,*xy=NULL,value[3];
  HC_PREC fac[3];
  char filename[300];
  if(!hc->spatial_solution_computed)
    HC_ERROR("hc_print_spatial_solution","spectral solution not computed");
  /* number of solution sets of ntype solutions */

  /* number of lateral points */
  np = sol[0].npoints;
  np2 = np*2;
  np3 = np*3;
  if(!np)
    HC_ERROR("hc_print_spatial_solution","npoints is zero");
  /* 
     compute the lateral coordinates
  */
  sh_compute_spatial_basis(sol, file_dummy, FALSE,flt_dummy, &xy,
			   1,verbose);

  /* depth file */
  dout = ggrd_open(dfilename,"w","hc_print_spatial_solution");
  if(verbose >= 2)
    fprintf(stderr,"hc_print_spatial_solution: writing depth levels to %s\n",
	    dfilename);
  for(i=0;i < hc->nradp2;i++){
    /* 

    compute the scaling factors, those do depend on radius
    in the case of the stresses, so leave inside loop!

    */
    hc_compute_solution_scaling_factors(hc,sol_mode,hc->r[i],
					hc->dvisc[i],fac);

    /* write depth in [km] to dout file */
    fprintf(dout,"%g\n",HC_Z_DEPTH(hc->r[i]));
    for(k=0;k < 3;k++)		/* pointers */
      os[k] = i * np3 + k*np;
    /* 

    format:


    lon lat vr vt vp   OR

    lon lat srr srt srp 

    
    */

    if(binary){
      /* binary output */
      sprintf(filename,"%s.%i.bin",name,i+1);
      out = ggrd_open(filename,"w","hc_print_spatial_solution");
      for(j=los=0;j < np;j++,los+=2){ /* loop through all points in layer */
	fwrite((xy+los),sizeof(float),2,out);
	for(k=0;k<3;k++)
	  value[k] = sol_x[os[k]] * fac[k];
	fwrite(value,sizeof(float),3,out);
	os[0]++;os[1]++;os[2]++;
      }     
      fclose(out);
    }else{
      /* ASCII output */
      sprintf(filename,"%s.%i.dat",name,i+1);
      out = ggrd_open(filename,"w","hc_print_spatial_solution");
      for(j=los=0;j < np;j++,los+=2){ /* loop through all points in layer */
	for(k=0;k<3;k++)
	  value[k] = sol_x[os[k]] * fac[k];
	fprintf(out,"%11g %11g\t%12.5e %12.5e %12.5e\n",
		xy[los],xy[los+1],value[0],value[1],value[2]);
	os[0]++;os[1]++;os[2]++;
      }
      fclose(out);
    }
    if(verbose >= 2)
      fprintf(stderr,"hc_print_spatial_solution: layer %3i: RMS: r: %12.5e t: %12.5e p: %12.5e file: %s\n",
	      i+1,hc_svec_rms((sol_x+i*np3),np),hc_svec_rms((sol_x+i*np3+np),np),
	      hc_svec_rms((sol_x+i*np3+np2),np),
	      filename);
  }
  fclose(dout);
  if(verbose)
    fprintf(stderr,"hc_print_spatial_solution: wrote solution at %i levels\n",
	    hc->nradp2);
  free(xy);
}

/* 

print the depth layers solution

*/
void hc_print_depth_layers(struct hcs *hc, FILE *out, 
			   hc_boolean verbose)
{
  int i;
  /* number of solution sets of ntype solutions */
  for(i=0;i < hc->nradp2;i++)
    fprintf(out,"%g\n",HC_Z_DEPTH(hc->r[i]));
}


/* 

print a [3][3] matrix

*/
void hc_print_3x3(HC_PREC a[3][3], FILE *out)
  {
  int i,j;
  for(i=0;i<3;i++){
    for(j=0;j<3;j++)
      fprintf(out,"%11.4e ",a[i][j]);
    fprintf(out,"\n");
  }
}
/* 

print a [6][4] solution matrix

*/
void hc_print_sm(HC_PREC a[6][4], FILE *out)
{
  int i,j;
  for(i=0;i < 6;i++){
    for(j=0;j<4;j++)
      fprintf(out,"%11.4e ",a[i][j]);
    fprintf(out,"\n");
  }
}

void hc_print_vector(HC_PREC *a, int n,FILE *out)
{
  int i;
  for(i=0;i<n;i++)
    fprintf(out,"%11.4e ",a[i]);
  fprintf(out,"\n");
}
void hc_print_vector_label(HC_PREC *a, int n,FILE *out,
			   char *label)
{
  int i;
  fprintf(out,"%s: ",label);
  for(i=0;i<n;i++)
    fprintf(out,"%11.4e ",a[i]);
  fprintf(out,"\n");
}
void hc_print_matrix_label(HC_PREC *a, int m,
			   int n,FILE *out,char *label)
{
  int i,j;
  for(j=0;j<m;j++){
    fprintf(out,"%s: ",label);
    for(i=0;i<n;i++)
      fprintf(out,"%11.4e ",a[j*n+i]);
    fprintf(out,"\n");
  }
}


void hc_print_vector_row(HC_PREC *a, int n,FILE *out)
{
  int i;
  for(i=0;i<n;i++)
    fprintf(out,"%11.4e\n",a[i]);
}

/* 
   compute the r, theta, phi fac[3] scaling factors for the solution
   output
*/
void hc_compute_solution_scaling_factors(struct hcs *hc,
					 int sol_mode,
					 HC_PREC radius,
					 HC_PREC viscosity,
					 HC_PREC *fac)
{

 switch(sol_mode){
  case HC_VEL:
    fac[0]=fac[1]=fac[2] = hc->vel_scale; /* go to cm/yr  */
    break;
  case HC_RTRACTIONS:		/* radial tractions */
    fac[0]=fac[1]=fac[2] = hc->stress_scale/radius; /* go to MPa */
    break;
  case HC_HTRACTIONS:		/* horizontal tractions, are actually
				   given as strain-rates  */
    fac[0]=fac[1]=fac[2] = 2.0*viscosity*hc->stress_scale/radius; /* go to MPa */
    break;
  default:
    HC_ERROR("hc_print_spectral_solution","mode undefined");
    break;
  }

}
/* 
   
output of poloidal solution up to l_max 

*/
void hc_print_poloidal_solution(struct sh_lms *pol_sol,
				struct hcs *hc,
				int l_max, char *filename,
				hc_boolean convert_to_dt, /* convert spherical harmonic coefficients 
							     to Dahlen & Tromp format */
				hc_boolean verbose)
{
  int l,m,i,j,a_or_b,ll,nl,os,alim;
  FILE *out;
  HC_PREC value[2];
  /* 
     output of poloidal solution vectors 
  */
  if(verbose)
    fprintf(stderr,"hc_print_poloidal_solution: printing poloidal solution vector %s to %s\n",
	    (convert_to_dt)?("(physical convention"):("(internal convention)"),filename);
  /* find max output degree */
  ll = HC_MIN(l_max,pol_sol[0].lmax);
  /* number of output layers */
  nl = hc->nrad + 2;
  
  out = ggrd_open(filename,"w","hc_print_poloidal_solution");
  for(l=1;l <= ll;l++){
    for(m=0;m <= l;m++){
      alim = (m==0)?(1):(2);
      for(a_or_b=0;a_or_b < alim;a_or_b++){
	for(i=os=0;i < nl;i++,os+=6){
	  fprintf(out,"%3i %3i %1i %3i %8.5f ",l,m,a_or_b,i+1,hc->r[i]);
	  for(j=0;j < 6;j++){
	    sh_get_coeff((pol_sol+os+j),l,m,a_or_b,convert_to_dt,value);
	    fprintf(out,"%11.4e ",value[0]);
	  } /* end u_1 .. u_4 nu_1 nu_2 loop */
	  fprintf(out,"\n");
	} /* end layer loop */
      } /* and A/B coefficient loop */
    }	/* end m loop */
  } /* end l loop */
  fclose(out);
}

/* 
   print toroidal solution vector (kernel), not expansion
*/
void hc_print_toroidal_solution(double *tvec,int lmax,
				struct hcs *hc,int l_max_out, 
				char *filename,
				hc_boolean verbose)
{
  FILE *out;
  int ll,l,i,nl,lmaxp1,os,os2;
  ll = HC_MIN(l_max_out,lmax); /* output lmax */
  nl = hc->nrad + 2;		/* number of layers */
  lmaxp1 = lmax + 1;		/* max expansion */
  os2 = lmaxp1 * nl;
  /* 
     kernel output
  */
  if(verbose)
    fprintf(stderr,"hc_print_toiroidal_solution: writing toroidal solutions 1 and 2 as f(l,r) to %s\n",
	    filename);
  out = ggrd_open(filename,"w","hc_toroidal_solution");
  for(l=1;l <= ll;l++){
    for(os=i=0;i < nl;i++,os+=lmaxp1)
      fprintf(out,"%3i %16.7e %16.7e %16.7e\n",
	      l,hc->r[i],tvec[os+l],tvec[os2+os+l]);
    fprintf(out,"\n");
  }
  fclose(out);
}
/* 

print a simple VTK file given already expanded input
(called from hc_print_spatial)


*/

void hc_print_vtk(FILE *out,float *xloc,float *xvec,
		  int npoints,int nlay,
		  hc_boolean binary)
{
  int i,ilay,ndata,poff;
  hc_boolean little_endian;
  /* determine machine type */
  little_endian = hc_is_little_endian();
  /*  */
  ndata = npoints*3;
  /* print VTK */
  fprintf(out,"# vtk DataFile Version 4.0\n");
  fprintf(out,"generated by hc_print_vtk\n");
  if(binary)
    fprintf(out,"BINARY\n");
  else
    fprintf(out,"ASCII\n");
  fprintf(out,"DATASET POLYDATA\n");

  fprintf(out,"POINTS %i float\n",npoints*nlay);
  for(ilay=0;ilay < nlay;ilay++){
    for(i=0;i < npoints;i++){
      poff = ilay * ndata + i*3;
      if(binary)
	hc_print_be_float((xloc+poff),3,out,little_endian);
      else
	fprintf(out,"%.6e %.6e %.6e\n",
		xloc[poff],xloc[poff+1],xloc[poff+2]);
    }
  }
  fprintf(out,"POINT_DATA %i\n",npoints*nlay);
  fprintf(out,"VECTORS velocity float\n");
  for(ilay=0;ilay < nlay;ilay++){
    for(i=0;i < npoints;i++){
      poff = ilay * ndata + i*3;
      if(binary)
	hc_print_be_float((xvec+poff),3,out,little_endian);
      else
	fprintf(out,"%.6e %.6e %.6e\n",xvec[poff],xvec[poff+1],
		xvec[poff+2]);
    }
  }
  
}
/* 
   print big endian binary 
*/
void hc_print_be_float(float *x, int n, FILE *out, hc_boolean little_endian)
{
  int i;
  float *xcopy;
  static size_t len = sizeof(float);
  if(little_endian){
    /* need to flip the byte order */
    hc_svecalloc(&xcopy,n,"hc_print_be_float");
    memcpy(xcopy,x,len*n);
    for(i=0;i < n;i++)
      hc_flip_byte_order((void *)(xcopy+i),len);
    fwrite(xcopy,len,n,out);
    free(xcopy);
  }else{
    /* can write as is */
    fwrite(x,len,n,out);
  }
}
hc_boolean hc_is_little_endian(void)
{
  static const unsigned long a = 1;
  return *(const unsigned char *)&a;
}

/* 

flip endianness of x

*/
void hc_flip_byte_order(void *x, size_t len)
{
  void *copy;
  copy = (void *)malloc(len);
  if(!copy){
    fprintf(stderr,"flip_byte_order: memerror with len: %i\n",(int)len);
    exit(-1);
  }
  memcpy(copy,x,len);
  hc_flipit(x,copy,len);
  free(copy);
}
/* this should not be called with (i,i,size i) */
void hc_flipit(void *d, void *s, size_t len)
{
  unsigned char *dest = d;
  unsigned char *src  = s;
  src += len - 1;
  for (; len; len--)
    *dest++ = *src--;
}

